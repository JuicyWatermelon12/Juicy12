// BanditBaller

#include "GSPlayerState.h"
#include "GSAbilitySystemComponent.h"
#include "GSAttributeSet.h"
#include "GSProgressionComponent.h"
#include "Net/UnrealNetwork.h"

AGSPlayerState::AGSPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Subobject of the ASC owner, so the ASC registers it automatically.
	AttributeSet = CreateDefaultSubobject<UGSAttributeSet>(TEXT("AttributeSet"));

	Progression = CreateDefaultSubobject<UGSProgressionComponent>(TEXT("Progression"));

	// ASC on a PlayerState needs a higher update rate than the PlayerState default.
	SetNetUpdateFrequency(100.f);
}

void AGSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSPlayerState, Cash);
}

void AGSPlayerState::OnRep_Cash(int32 OldCash)
{
	OnCashChanged.Broadcast(Cash, Cash - OldCash);
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

void AGSPlayerState::SetCashFromProfile(int32 NewCash)
{
	const int32 Delta = NewCash - Cash;
	if (!HasAuthority() || Delta == 0)
	{
		return;
	}

	Cash = NewCash;
	OnCashChanged.Broadcast(Cash, Delta);
}
