// BanditBaller

#include "GSPlayerState.h"
#include "GSAbilitySystemComponent.h"
#include "GSAttributeSet.h"

AGSPlayerState::AGSPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Subobject of the ASC owner, so the ASC registers it automatically.
	AttributeSet = CreateDefaultSubobject<UGSAttributeSet>(TEXT("AttributeSet"));

	// ASC on a PlayerState needs a higher update rate than the PlayerState default.
	SetNetUpdateFrequency(100.f);
}

UAbilitySystemComponent* AGSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AGSPlayerState::GrantStartupAbilitySet(const UGSAbilitySet* AbilitySet)
{
	if (!HasAuthority() || bStartupAbilitySetGranted || !AbilitySet)
	{
		return false;
	}

	AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &StartupAbilitySetHandles, this);
	bStartupAbilitySetGranted = true;
	return true;
}

void AGSPlayerState::AddCash(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return;
	}

	// Saturate instead of overflowing.
	const int64 NewCash = FMath::Min<int64>(static_cast<int64>(Cash) + Amount, MAX_int32);
	SetCash(static_cast<int32>(NewCash));
}

bool AGSPlayerState::TrySpendCash(int32 Amount)
{
	if (!HasAuthority() || Amount < 0 || Amount > Cash)
	{
		return false;
	}

	if (Amount > 0)
	{
		SetCash(Cash - Amount);
	}
	return true;
}

void AGSPlayerState::SetCash(int32 NewCash)
{
	const int32 Delta = NewCash - Cash;
	if (Delta == 0)
	{
		return;
	}

	Cash = NewCash;
	OnCashChanged.Broadcast(Cash, Delta);
}
