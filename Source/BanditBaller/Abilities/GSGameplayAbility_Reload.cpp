// BanditBaller

#include "GSGameplayAbility_Reload.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "GSAttributeSet.h"
#include "GSGameplayTags.h"
#include "GSWeaponComponent.h"
#include "GSWeaponData.h"

UGSGameplayAbility_Reload::UGSGameplayAbility_Reload()
{
	ActivationInputTag = GSGameplayTags::Ability_Reload;
	SetAssetTags(FGameplayTagContainer(GSGameplayTags::Ability_Reload));
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Reloading);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Firing);
}

bool UGSGameplayAbility_Reload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	return Weapons && Weapons->GetCurrentWeaponData() && !Weapons->IsCurrentMagazineFull();
}

void UGSGameplayAbility_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	const UGSWeaponData* WeaponData = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	if (!WeaponData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ReloadingIndex = Weapons->GetCurrentIndex();

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const float ReloadSpeed = ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetReloadSpeedAttribute()) : 1.f;
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, GetReloadDuration(WeaponData->ReloadTime, ReloadSpeed));
	Wait->OnFinish.AddDynamic(this, &UGSGameplayAbility_Reload::OnReloadFinished);
	Wait->ReadyForActivation();
}

float UGSGameplayAbility_Reload::GetReloadDuration(float BaseReloadTime, float ReloadSpeed)
{
	return BaseReloadTime / FMath::Max(ReloadSpeed, UGSAttributeSet::MinReloadSpeed);
}

void UGSGameplayAbility_Reload::OnReloadFinished()
{
	UGSWeaponComponent* Weapons = GetWeaponComponent(CurrentActorInfo);
	if (Weapons && CurrentActorInfo->IsNetAuthority() && Weapons->GetCurrentIndex() == ReloadingIndex)
	{
		Weapons->RefillCurrentMagazine();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
