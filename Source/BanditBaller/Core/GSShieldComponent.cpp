// BanditBaller

#include "GSShieldComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BanditBaller.h"
#include "Engine/World.h"
#include "GSAttributeSet.h"
#include "GSCombatSettings.h"
#include "GSGameMode.h"
#include "TimerManager.h"

UGSShieldComponent::UGSShieldComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGSShieldComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		AttributeChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetShieldsAttribute())
			.AddUObject(this, &UGSShieldComponent::HandleShieldsAttributeChanged);
	}
	SyncShieldCount();
}

void UGSShieldComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetShieldsAttribute()).Remove(AttributeChangedHandle);
	}
	if (UWorld* World = GetWorld())
	{
		for (FGSShieldSlot& Slot : Slots)
		{
			World->GetTimerManager().ClearTimer(Slot.ReturnTimer);
		}
	}
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* UGSShieldComponent::GetASC() const
{
	const IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(GetOwner());
	return Owner ? Owner->GetAbilitySystemComponent() : nullptr;
}

float UGSShieldComponent::GetCooldown() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	const UGSCombatSettings* Settings = (GameMode && GameMode->GetCombatSettings()) ? GameMode->GetCombatSettings() : GetDefault<UGSCombatSettings>();
	return FMath::Max(Settings->ShieldCooldown, 0.1f);
}

void UGSShieldComponent::HandleShieldsAttributeChanged(const FOnAttributeChangeData& Data)
{
	SyncShieldCount();
}

void UGSShieldComponent::SyncShieldCount()
{
	const UAbilitySystemComponent* ASC = GetASC();
	const int32 Wanted = ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetShieldsAttribute()))) : 0;
	if (Wanted == Slots.Num())
	{
		return;
	}
	// More shields: the new ones are ready at once. Fewer: drop from the end, with their recharge.
	while (Slots.Num() > Wanted)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(Slots.Last().ReturnTimer);
		}
		Slots.Pop();
	}
	while (Slots.Num() < Wanted)
	{
		Slots.AddDefaulted();
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[shield] %s: %d floating shield(s), %d ready"), *GetNameSafe(GetOwner()), Slots.Num(), GetNumReady());
	OnShieldCountChanged.Broadcast(Slots.Num());
}

int32 UGSShieldComponent::GetNumReady() const
{
	int32 Ready = 0;
	for (const FGSShieldSlot& Slot : Slots)
	{
		Ready += Slot.bReady ? 1 : 0;
	}
	return Ready;
}

float UGSShieldComponent::GetCooldownRemaining(int32 Index) const
{
	const UWorld* World = GetWorld();
	if (!Slots.IsValidIndex(Index) || Slots[Index].bReady || !World)
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(Slots[Index].ReadyAtTime - World->GetTimeSeconds()));
}

bool UGSShieldComponent::TryAbsorbHit(float Amount, const AActor* Instigator)
{
	UWorld* World = GetWorld();
	if (!World || !GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.f)
	{
		return false;
	}
	SyncShieldCount();
	const int32 Index = Slots.IndexOfByPredicate([](const FGSShieldSlot& Slot) { return Slot.bReady; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	const float Cooldown = GetCooldown();
	FGSShieldSlot& Slot = Slots[Index];
	Slot.bReady = false;
	Slot.ReadyAtTime = World->GetTimeSeconds() + Cooldown;
	World->GetTimerManager().SetTimer(Slot.ReturnTimer, FTimerDelegate::CreateUObject(this, &UGSShieldComponent::HandleShieldReturned, Index), Cooldown, false);
	++HitsBlocked;

	UE_LOG(LogBanditBaller, Log, TEXT("[shield] %s: shield %d blocked %.0f damage from %s (%d of %d ready), back in %.1f s"),
		*GetNameSafe(GetOwner()), Index + 1, Amount, *GetNameSafe(Instigator), GetNumReady(), Slots.Num(), Cooldown);
	OnShieldBlocked.Broadcast(Index);
	return true;
}

void UGSShieldComponent::HandleShieldReturned(int32 Index)
{
	if (!Slots.IsValidIndex(Index) || Slots[Index].bReady)
	{
		return;
	}
	Slots[Index].bReady = true;
	Slots[Index].ReadyAtTime = 0.0;
	UE_LOG(LogBanditBaller, Log, TEXT("[shield] %s: shield %d is back (%d of %d ready)"), *GetNameSafe(GetOwner()), Index + 1, GetNumReady(), Slots.Num());
	OnShieldReturned.Broadcast(Index);
}

void UGSShieldComponent::ResetShields()
{
	if (UWorld* World = GetWorld())
	{
		for (FGSShieldSlot& Slot : Slots)
		{
			World->GetTimerManager().ClearTimer(Slot.ReturnTimer);
		}
	}
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (!Slots[Index].bReady)
		{
			Slots[Index].bReady = true;
			Slots[Index].ReadyAtTime = 0.0;
			OnShieldReturned.Broadcast(Index);
		}
	}
	HitsBlocked = 0;
	SyncShieldCount();
}
