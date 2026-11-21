// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_MeleeAttack.generated.h"

class UGameplayEffect;

/**
 * Enemy melee: face the target, telegraph for AttackWindUp, sphere-sweep in front and apply
 * DamageEffect (GE_Damage, SetByCaller Data.Damage) to players hit, then recover.
 * All numbers come from the avatar's UGSEnemyData. Server-only.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_MeleeAttack : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_MeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** GE_Damage. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	UFUNCTION()
	void OnWindUpFinished();

	UFUNCTION()
	void OnRecoveryFinished();

	void PerformSweep();
};
