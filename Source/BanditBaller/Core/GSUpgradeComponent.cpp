// BanditBaller

#include "GSUpgradeComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "GameplayEffect.h"
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

bool UGSUpgradeComponent::GrantUpgrade(const UGSUpgradeData* Upgrade, const TCHAR* Source)
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

	FGSUpgradeStack* Entry = Stacks.FindByPredicate([Upgrade](const FGSUpgradeStack& S) { return S.Upgrade == Upgrade; });
	const int32 Current = Entry ? Entry->Stacks : 0;
	if (!Upgrade->CanAddStack(Current))
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: %s is at its cap (%d/%d); not granted."), *GetNameSafe(Owner), *Upgrade->GetName(), Current, Upgrade->MaxStacks);
		return false;
	}

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Upgrade->Effect, 1.f, ASC->MakeEffectContext());
	const FActiveGameplayEffectHandle Handle = Spec.IsValid() ? ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data) : FActiveGameplayEffectHandle();
	if (!Handle.IsValid())
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: applying %s failed."), *GetNameSafe(Owner), *GetNameSafe(Upgrade->Effect));
		return false;
	}

	if (!Entry)
	{
		Entry = &Stacks.AddDefaulted_GetRef();
		Entry->Upgrade = Upgrade;
	}
	Entry->Stacks = Current + 1;
	Entry->EffectHandle = Handle; // stacking by target: every pick lands on the same active effect

	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: %s x%d (%s) -> %s"), *GetNameSafe(Owner), *Upgrade->GetName(), Entry->Stacks,
		Source, *Upgrade->GetFormattedDescription().ToString());
	OnUpgradeStacksChanged.Broadcast(Upgrade, Entry->Stacks);
	return true;
}

int32 UGSUpgradeComponent::GetStacks(const UGSUpgradeData* Upgrade) const
{
	const FGSUpgradeStack* Entry = Stacks.FindByPredicate([Upgrade](const FGSUpgradeStack& S) { return S.Upgrade == Upgrade; });
	return Entry ? Entry->Stacks : 0;
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

TArray<UGSUpgradeData*> UGSUpgradeComponent::RollChoices() const
{
	const UGSUpgradePool* Pool = GetPool();
	if (!Pool)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] No UpgradePool on the GameMode; no offer rolled."));
		return {};
	}
	return Pool->RollOffer(UGSRunRandom::GetStream(this), [this](const UGSUpgradeData* Upgrade) { return GetStacks(Upgrade); });
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
		TArray<UGSUpgradeData*> Choices = RollChoices();
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

	const UGSUpgradeData* Picked = CurrentChoices[Index];
	if (!GrantUpgrade(Picked, TEXT("LevelUp")))
	{
		return false; // e.g. capped meanwhile by gs.Upgrade.Grant; the offer stays open
	}

	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: chose [%d] %s."), *GetNameSafe(GetOwner()), Index, *Picked->GetName());
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

	TArray<UGSUpgradeData*> Choices = RollChoices();
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

TArray<UGSUpgradeData*> UGSUpgradeComponent::GetCurrentChoices() const
{
	TArray<UGSUpgradeData*> Result;
	for (const TObjectPtr<UGSUpgradeData>& Choice : CurrentChoices)
	{
		Result.Add(Choice);
	}
	return Result;
}

FString UGSUpgradeComponent::DescribeOffer() const
{
	FString Text = HasOpenOffer() ? TEXT("Offer:") : TEXT("No offer open.");
	for (int32 Index = 0; Index < CurrentChoices.Num(); ++Index)
	{
		const UGSUpgradeData* Choice = CurrentChoices[Index];
		Text += FString::Printf(TEXT(" [%d] %s (%s, x%d)"), Index, *Choice->GetName(), *UGSUpgradeData::GetRarityName(Choice->Rarity), GetStacks(Choice));
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
			ASC->RemoveActiveGameplayEffect(Entry.EffectHandle, /*StacksToRemove*/ -1);
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
