// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_SwapWeapon.generated.h"

/**
 * Cycles to the next weapon (server-only; equipped index replicates). Cancels an in-progress reload.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_SwapWeapon : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_SwapWeapon();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
