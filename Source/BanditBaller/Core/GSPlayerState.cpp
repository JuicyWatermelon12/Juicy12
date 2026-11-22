// BanditBaller

#include "GSPlayerState.h"
#include "GSAbilitySystemComponent.h"
#include "GSAttributeSet.h"
#include "GSComboComponent.h"
#include "GSProgressionComponent.h"
#include "GSUpgradeComponent.h"

AGSPlayerState::AGSPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Subobject of the ASC owner, so the ASC registers it automatically.
	AttributeSet = CreateDefaultSubobject<UGSAttributeSet>(TEXT("AttributeSet"));

	Progression = CreateDefaultSubobject<UGSProgressionComponent>(TEXT("Progression"));
	Upgrades = CreateDefaultSubobject<UGSUpgradeComponent>(TEXT("Upgrades"));
	Combo = CreateDefaultSubobject<UGSComboComponent>(TEXT("Combo"));

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
