// BanditBaller

#include "GSGameplayAbility_Slide.h"
#include "GSCharacter.h"
#include "GSGameplayEffects.h"
#include "GSGameplayTags.h"
#include "GSMovementAbilitySettings.h"

UGSGameplayAbility_Slide::UGSGameplayAbility_Slide()
{
	ActivationInputTag = GSGameplayTags::Ability_Slide;
	SetAssetTags(FGameplayTagContainer(GSGameplayTags::Ability_Slide));
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Sliding);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dashing);
	CooldownGameplayEffectClass = UGSCooldownEffect_Slide::StaticClass();
}

float UGSGameplayAbility_Slide::GetCooldownDuration(const FGameplayAbilityActorInfo* ActorInfo) const
{
	return Settings ? Settings->Cooldown : 0.f;
}

bool UGSGameplayAbility_Slide::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Settings || !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AGSCharacter* Character = GetGSCharacter(ActorInfo);
	return Character && Character->CanStartSlide(Settings->MinStartSpeed);
}

void UGSGameplayAbility_Slide::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AGSCharacter* Character = GetGSCharacter(ActorInfo);
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SlideFinishedHandle = Character->OnSlideFinished.AddUObject(this, &UGSGameplayAbility_Slide::OnSlideFinished);
	Character->StartSlide(*Settings);
}

void UGSGameplayAbility_Slide::OnSlideFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGSGameplayAbility_Slide::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AGSCharacter* Character = GetGSCharacter(ActorInfo))
	{
		Character->OnSlideFinished.Remove(SlideFinishedHandle);
		SlideFinishedHandle.Reset();

		// Cancelled (death, dash): make sure the character leaves the slide.
		if (Character->IsSliding())
		{
			Character->StopSlide();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
