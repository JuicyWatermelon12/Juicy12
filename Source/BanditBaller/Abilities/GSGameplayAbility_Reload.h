// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_Reload.generated.h"

/**
 * Reloads the equipped weapon after ReloadTime / ReloadSpeed attribute (owns State.Reloading).
 * Blocked while firing or dead and when the magazine is full; cancelled by weapon swap.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_Reload : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_Reload();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Seconds a reload takes: BaseReloadTime / ReloadSpeed (ReloadSpeed clamped to UGSAttributeSet::MinReloadSpeed). */
	static float GetReloadDuration(float BaseReloadTime, float ReloadSpeed);

private:
	UFUNCTION()
	void OnReloadFinished();

	/** Weapon slot being reloaded, so a swap mid-reload can't refill the wrong weapon. */
	int32 ReloadingIndex = INDEX_NONE;
};
