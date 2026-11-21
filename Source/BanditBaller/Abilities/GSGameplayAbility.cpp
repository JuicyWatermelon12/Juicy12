// BanditBaller

#include "GSGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GSCharacter.h"
#include "GSGameplayTags.h"
#include "GSWeaponComponent.h"

UGSGameplayAbility::UGSGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

AGSCharacter* UGSGameplayAbility::GetGSCharacter(const FGameplayAbilityActorInfo* ActorInfo)
{
	return ActorInfo ? Cast<AGSCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
}

UGSWeaponComponent* UGSGameplayAbility::GetWeaponComponent(const FGameplayAbilityActorInfo* ActorInfo)
{
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Avatar ? Avatar->FindComponentByClass<UGSWeaponComponent>() : nullptr;
}

void UGSGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	const float Duration = GetCooldownDuration(ActorInfo);
	if (!CooldownEffect || Duration <= 0.f)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownEffect->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Cooldown, Duration);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

void UGSGameplayAbility::GetCooldownDisplayInfo(const FGameplayAbilityActorInfo* ActorInfo, float& OutRemaining, float& OutDuration,
	int32& OutCharges, int32& OutMaxCharges) const
{
	OutRemaining = 0.f;
	OutDuration = 0.f;
	OutMaxCharges = 1;
	OutCharges = 1;

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (!ASC || !CooldownTags || CooldownTags->IsEmpty())
	{
		return;
	}

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);
	for (const TPair<float, float>& RemainingAndDuration : ASC->GetActiveEffectsTimeRemainingAndDuration(Query))
	{
		if (RemainingAndDuration.Key > OutRemaining)
		{
			OutRemaining = RemainingAndDuration.Key;
			OutDuration = RemainingAndDuration.Value;
		}
	}

	OutCharges = OutRemaining > 0.f ? 0 : 1;
}
