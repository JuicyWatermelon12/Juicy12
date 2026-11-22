// BanditBaller

#include "GSUpgradeComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "GameplayEffect.h"
#include "GSGameplayTags.h"
#include "GSGameMode.h"
#include "GSPlayerState.h"
#include "GSProgressionComponent.h"
#include "GSRunRandom.h"
#include "GSUpgradeData.h"
#include "GSUpgradePool.h"

UGSUpgradeComponent::UGSUpgradeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGSUpgradeComponent::BeginPlay()
{
	Super::BeginPlay();

	const AGSPlayerState* PS = Cast<AGSPlayerState>(GetOwner());
	if (UGSProgressionComponent* Progression = PS ? PS->GetProgressionComponent() : nullptr)
	{
		Progression->OnLevelUp.AddUniqueDynamic(this, &UGSUpgradeComponent::HandleLevelUp);
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const UGSUpgradePool* Pool = GetPool();
		SetRerollsLeft(Pool ? Pool->RerollsPerRun : 0);
	}
}

void UGSUpgradeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const AGSPlayerState* PS = Cast<AGSPlayerState>(GetOwner());
	if (UGSProgressionComponent* Progression = PS ? PS->GetProgressionComponent() : nullptr)
	{
		Progression->OnLevelUp.RemoveDynamic(this, &UGSUpgradeComponent::HandleLevelUp);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Stacks
// ---------------------------------------------------------------------------

const UGSUpgradePool* UGSUpgradeComponent::GetPoolOrDefault() const
{
	const UGSUpgradePool* Pool = GetPool();
	return Pool ? Pool : GetDefault<UGSUpgradePool>();
}

bool UGSUpgradeComponent::ApplyEntryEffect(UAbilitySystemComponent* ASC, FGSUpgradeStack& Entry) const
{
	TMap<FGameplayTag, float> Values;
	Values.Add(GSGameplayTags::Data_UpgradeValue, Entry.TotalValue);
	Values.Add(GSGameplayTags::Data_UpgradeMultiplier, 1.f + Entry.TotalValue);
	Values.Add(GSGameplayTags::Data_UpgradePicks, static_cast<float>(Entry.Stacks));

	if (Entry.EffectHandle.IsValid() && ASC->GetActiveGameplayEffect(Entry.EffectHandle))
	{
		// Later picks: the same active effect, new totals. The attribute goes straight to the new value (it never dips,
		// so shields, dash charges and the guns keep their state).
		ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(Entry.EffectHandle, Values);
		return true;
	}
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Entry.Upgrade->Effect, 1.f, ASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return false;
	}
	for (const TPair<FGameplayTag, float>& Value : Values)
	{
		Spec.Data->SetSetByCallerMagnitude(Value.Key, Value.Value);
	}
	Entry.EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	return Entry.EffectHandle.IsValid();
}

bool UGSUpgradeComponent::GrantUpgrade(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity, const TCHAR* Source)
{
	AActor* Owner = GetOwner();
	if (!Upgrade || !Owner || !Owner->HasAuthority())
	{
		return false;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (!ASC || !Upgrade->Effect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: can't grant %s (no ASC or no Effect on the upgrade)."), *GetNameSafe(Owner), *GetNameSafe(Upgrade));
		return false;
	}
	const UGSUpgradePool* Pool = GetPoolOrDefault();
	if (!Pool->HasRarity(Upgrade, Rarity))
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: %s has no %s version (whole units: Common, Epic, Legendary); not granted."),
			*GetNameSafe(Owner), *Upgrade->GetName(), *UGSUpgradeData::GetRarityName(Rarity));
		return false;
	}

	const int32 EntryIndex = Stacks.IndexOfByPredicate([Upgrade](const FGSUpgradeStack& S) { return S.Upgrade == Upgrade; });
	const int32 Current = EntryIndex != INDEX_NONE ? Stacks[EntryIndex].Stacks : 0;

	FGSUpgradeStack NewEntry;
	FGSUpgradeStack& Entry = EntryIndex != INDEX_NONE ? Stacks[EntryIndex] : NewEntry;
	Entry.Upgrade = Upgrade;
	const FGSUpgradePick Pick{ Rarity, Pool->GetPickValue(Upgrade, Rarity, Current) };
	Entry.Picks.Add(Pick);
	Entry.Stacks = Current + 1;
	Entry.TotalValue += Pick.Value;
	if (!ApplyEntryEffect(ASC, Entry))
	{
		Entry.Picks.Pop();
		Entry.Stacks = Current;
		Entry.TotalValue -= Pick.Value;
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: applying %s failed."), *GetNameSafe(Owner), *GetNameSafe(Upgrade->Effect));
		return false;
	}
	if (EntryIndex == INDEX_NONE)
	{
		Stacks.Add(MoveTemp(NewEntry));
	}
	const FGSUpgradeStack& Added = EntryIndex != INDEX_NONE ? Stacks[EntryIndex] : Stacks.Last();

	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: %s x%d (%s, %s %s, total %s) -> %s"), *GetNameSafe(Owner), *Upgrade->GetName(), Added.Stacks,
		Source, *UGSUpgradeData::GetRarityName(Rarity), *Upgrade->FormatValue(Pick.Value), *Upgrade->FormatValue(Added.TotalValue),
		*Upgrade->GetFormattedDescription(Current).ToString()); // the text of the pick just made
	OnUpgradeStacksChanged.Broadcast(Upgrade, Added.Stacks);
	return true;
}

const UGSUpgradeData* UGSUpgradeComponent::RemoveStackByEffect(TSubclassOf<UGameplayEffect> EffectClass, const TCHAR* Reason)
{
	AActor* Owner = GetOwner();
	if (!EffectClass || !Owner || !Owner->HasAuthority())
	{
		return nullptr;
	}
	const int32 Index = Stacks.IndexOfByPredicate([EffectClass](const FGSUpgradeStack& S)
	{
		return S.Upgrade && S.Upgrade->Effect && S.Upgrade->Effect->IsChildOf(EffectClass) && S.Stacks > 0;
	});
	if (Index == INDEX_NONE)
	{
		return nullptr;
	}
	FGSUpgradeStack& Entry = Stacks[Index];
	const UGSUpgradeData* Upgrade = Entry.Upgrade;
	const FGSUpgradePick Removed = Entry.Picks.Num() > 0 ? Entry.Picks.Pop() : FGSUpgradePick();
	Entry.TotalValue = FMath::Max(Entry.TotalValue - Removed.Value, 0.f);
	const int32 Left = --Entry.Stacks;
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (Left <= 0)
	{
		if (ASC)
		{
			ASC->RemoveActiveGameplayEffect(Entry.EffectHandle);
		}
		Stacks.RemoveAt(Index);
	}
	else if (ASC)
	{
		ApplyEntryEffect(ASC, Entry); // the same effect, one pick less
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: %s used up (%s), %d left"), *GetNameSafe(Owner), *GetNameSafe(Upgrade), Reason, Left);
	OnUpgradeStacksChanged.Broadcast(Upgrade, Left);
	return Upgrade;
}

const FGSUpgradeStack* UGSUpgradeComponent::FindEntry(const UGSUpgradeData* Upgrade) const
{
	return Stacks.FindByPredicate([Upgrade](const FGSUpgradeStack& S) { return S.Upgrade == Upgrade; });
}

int32 UGSUpgradeComponent::GetStacks(const UGSUpgradeData* Upgrade) const
{
	const FGSUpgradeStack* Entry = FindEntry(Upgrade);
	return Entry ? Entry->Stacks : 0;
}

float UGSUpgradeComponent::GetTotalValue(const UGSUpgradeData* Upgrade) const
{
	const FGSUpgradeStack* Entry = FindEntry(Upgrade);
	return Entry ? Entry->TotalValue : 0.f;
}

int32 UGSUpgradeComponent::GetTotalPicks() const
{
	int32 Total = 0;
	for (const FGSUpgradeStack& Entry : Stacks)
	{
		Total += Entry.Stacks;
	}
	return Total;
}

// ---------------------------------------------------------------------------
// Offers
// ---------------------------------------------------------------------------

const UGSUpgradePool* UGSUpgradeComponent::GetPool() const
{
	const UWorld* World = GetWorld();
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetUpgradePool() : nullptr;
}

TArray<FGSUpgradeChoice> UGSUpgradeComponent::RollChoices() const
{
	const UGSUpgradePool* Pool = GetPool();
	if (!Pool)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] No UpgradePool on the GameMode; no offer rolled."));
		return {};
	}
	return Pool->RollOffer(UGSRunRandom::GetStream(this), [this](const UGSUpgradeData* Upgrade) { return GetStacks(Upgrade); }, /*bWeaponCard*/ bFirstOfferPending);
}

void UGSUpgradeComponent::HandleLevelUp(int32 NewLevel)
{
	// OnLevelUp also fires on clients from replication; offers are rolled on the server only.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		QueueOffer(*FString::Printf(TEXT("level %d"), NewLevel));
	}
}

void UGSUpgradeComponent::QueueOffer(const TCHAR* Source)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const UWorld* World = GetWorld();
	const AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
	if (GS && GS->GetRoundPhase() == EGSRoundPhase::GameOver)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: no offer (%s): the run is over."), *GetNameSafe(GetOwner()), Source);
		return;
	}
	++QueuedOffers;
	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: offer queued (%s)."), *GetNameSafe(GetOwner()), Source);
	if (!HasOpenOffer())
	{
		OpenNextOffer();
	}
}

void UGSUpgradeComponent::OpenNextOffer()
{
	while (QueuedOffers > 0)
	{
		--QueuedOffers;
		TArray<FGSUpgradeChoice> Choices = RollChoices();
		if (Choices.Num() > 0)
		{
			CurrentChoices = Choices;
			UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: offer opened. %s"), *GetNameSafe(GetOwner()), *DescribeOffer());
			BroadcastOfferChanged(EGSUpgradeOfferChange::Opened);
			return;
		}
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: no upgrade available; offer dropped."), *GetNameSafe(GetOwner()));
	}
}

bool UGSUpgradeComponent::ChooseUpgrade(int32 Index)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (!HasOpenOffer() || !CurrentChoices.IsValidIndex(Index))
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: can't choose %d (%s)."), *GetNameSafe(GetOwner()), Index,
			HasOpenOffer() ? *FString::Printf(TEXT("choices 0-%d"), CurrentChoices.Num() - 1) : TEXT("no offer open"));
		return false;
	}

	const FGSUpgradeChoice Choice = CurrentChoices[Index];
	const UGSUpgradeData* Picked = Choice.Upgrade;
	if (!GrantUpgrade(Picked, Choice.Rarity, TEXT("LevelUp"))) // the card's rolled rarity
	{
		return false; // e.g. capped meanwhile by gs.Upgrade.Grant; the offer stays open
	}

	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: chose [%d] %s (%s)."), *GetNameSafe(GetOwner()), Index, *Picked->GetName(), *UGSUpgradeData::GetRarityName(Choice.Rarity));
	bFirstOfferPending = false; // later offers have no guaranteed weapon card
	CurrentChoices.Reset();
	// The next queued offer replaces this one (one Opened event); Closed only when nothing is left, so the choice
	// screen and the pause stay up between queued offers.
	OpenNextOffer();
	if (!HasOpenOffer())
	{
		BroadcastOfferChanged(EGSUpgradeOfferChange::Closed);
	}
	return true;
}

bool UGSUpgradeComponent::RerollOffer()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (!HasOpenOffer() || RerollsLeft <= 0)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: reroll refused (%s)."), *GetNameSafe(GetOwner()),
			HasOpenOffer() ? TEXT("no rerolls left") : TEXT("no offer open"));
		return false;
	}

	TArray<FGSUpgradeChoice> Choices = RollChoices();
	if (Choices.Num() == 0)
	{
		return false; // can't happen while the open offer's upgrades are still available; keep the offer
	}
	SetRerollsLeft(RerollsLeft - 1);
	CurrentChoices = Choices;
	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: offer rerolled. %s"), *GetNameSafe(GetOwner()), *DescribeOffer());
	BroadcastOfferChanged(EGSUpgradeOfferChange::Rerolled);
	return true;
}

float UGSUpgradeComponent::GetChoiceValue(const FGSUpgradeChoice& Choice) const
{
	return GetPoolOrDefault()->GetPickValue(Choice.Upgrade, Choice.Rarity, GetStacks(Choice.Upgrade));
}

TArray<UGSUpgradeData*> UGSUpgradeComponent::GetCurrentChoices() const
{
	TArray<UGSUpgradeData*> Result;
	for (const FGSUpgradeChoice& Choice : CurrentChoices)
	{
		Result.Add(Choice.Upgrade);
	}
	return Result;
}

FString UGSUpgradeComponent::DescribeOffer() const
{
	FString Text = HasOpenOffer() ? TEXT("Offer:") : TEXT("No offer open.");
	for (int32 Index = 0; Index < CurrentChoices.Num(); ++Index)
	{
		const FGSUpgradeChoice& Choice = CurrentChoices[Index];
		Text += FString::Printf(TEXT(" [%d] %s (%s %s: %s, now %s, x%d)"), Index, *Choice.Upgrade->GetName(), *UGSUpgradeData::GetRarityName(Choice.Rarity),
			*Choice.Upgrade->GetCategoryName(), *Choice.Upgrade->FormatValue(GetChoiceValue(Choice)), *Choice.Upgrade->FormatValue(GetTotalValue(Choice.Upgrade)), GetStacks(Choice.Upgrade));
	}
	return Text + FString::Printf(TEXT(" | %d queued | %d reroll(s) left"), QueuedOffers, RerollsLeft);
}

void UGSUpgradeComponent::SetRerollsLeft(int32 NewRerollsLeft)
{
	if (NewRerollsLeft == RerollsLeft)
	{
		return;
	}
	RerollsLeft = NewRerollsLeft;
	OnRerollsLeftChanged.Broadcast(RerollsLeft);
	OnRerollsLeftChangedNative.Broadcast(RerollsLeft);
}

void UGSUpgradeComponent::BroadcastOfferChanged(EGSUpgradeOfferChange Change)
{
	// Gameplay first (pause while any offer is open), then the UI.
	const UWorld* World = GetWorld();
	if (AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr)
	{
		GameMode->UpdateUpgradeChoicePause();
	}
	OnUpgradeOfferChanged.Broadcast(Change);
	OnUpgradeOfferChangedNative.Broadcast(Change);
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void UGSUpgradeComponent::ResetUpgrades()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
	{
		for (const FGSUpgradeStack& Entry : Stacks)
		{
			ASC->RemoveActiveGameplayEffect(Entry.EffectHandle);
		}
	}
	const TArray<FGSUpgradeStack> Removed = MoveTemp(Stacks);
	Stacks.Reset();
	if (Removed.Num() > 0)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: reset, removed %d upgrade(s)."), *GetNameSafe(Owner), Removed.Num());
	}
	for (const FGSUpgradeStack& Entry : Removed)
	{
		OnUpgradeStacksChanged.Broadcast(Entry.Upgrade, 0);
	}

	// Offers and rerolls are per run too.
	DropOffers(TEXT("new run"));
	bFirstOfferPending = true;
	const UGSUpgradePool* Pool = GetPool();
	SetRerollsLeft(Pool ? Pool->RerollsPerRun : 0);
}

void UGSUpgradeComponent::DropOffers(const TCHAR* Reason)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const bool bHadOffers = HasOpenOffer() || QueuedOffers > 0;
	QueuedOffers = 0;
	if (HasOpenOffer())
	{
		CurrentChoices.Reset();
		BroadcastOfferChanged(EGSUpgradeOfferChange::Closed); // the GameMode unpauses, the choice screen hides
	}
	if (bHadOffers)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: offers dropped (%s)."), *GetNameSafe(GetOwner()), Reason);
	}
}

TArray<const UGSUpgradeData*> UGSUpgradeComponent::GetOwnedWeapons() const
{
	TArray<const UGSUpgradeData*> Weapons;
	for (const FGSUpgradeStack& Entry : Stacks)
	{
		if (Entry.Upgrade && Entry.Upgrade->Category == EGSUpgradeCategory::Weapon && Entry.Stacks > 0)
		{
			Weapons.Add(Entry.Upgrade);
		}
	}
	return Weapons;
}
