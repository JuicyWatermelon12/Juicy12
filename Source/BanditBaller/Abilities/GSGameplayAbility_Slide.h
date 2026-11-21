// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_Slide.generated.h"

class UGSSlideSettings;

/**
 * Drops into a low-capsule slide with a speed boost that decays over the slide (AGSCharacter drives
 * the movement). Owns State.Sliding. Jumping out keeps momentum (slide-hop). Cooldown: GE_Cooldown_Slide.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_Slide : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_Slide();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual float GetCooldownDuration(const FGameplayAbilityActorInfo* ActorInfo) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Slide")
	TObjectPtr<UGSSlideSettings> Settings;

private:
	void OnSlideFinished();

	FDelegateHandle SlideFinishedHandle;
};
