// BanditBaller

#include "GSGameplayAbility_SwapWeapon.h"
#include "GSGameplayTags.h"
#include "GSWeaponComponent.h"

UGSGameplayAbility_SwapWeapon::UGSGameplayAbility_SwapWeapon()
{
	ActivationInputTag = GSGameplayTags::Ability_SwapWeapon;
	SetAssetTags(FGameplayTagContainer(GSGameplayTags::Ability_SwapWeapon));
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Firing);
	CancelAbilitiesWithTag.AddTag(GSGameplayTags::Ability_Reload);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool UGSGameplayAbility_SwapWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
		&& Weapons && Weapons->GetNumWeapons() > 1;
}

void UGSGameplayAbility_SwapWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		if (UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo))
		{
			Weapons->CycleWeapon();
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
