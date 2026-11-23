// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_EnemyFire.generated.h"

class AGSEnemyCharacter;
class UGameplayEffect;

/**
 * Enemy ranged attack: face the target, aim telegraph (laser + body glow) for AimTelegraphTime,
 * then a burst of BurstCount shots spaced by the weapon's fire rate, then BurstRecovery. Each shot is a pooled,
 * visible projectile (AGSProjectile, bEnemyShot) aimed at the target's chest where it is when that shot is fired (no
 * lead) inside the weapon's HipSpread cone, flying at Weapon->ProjectileSpeed for up to Weapon->Range. A player it hits
 * gets DamageEffect (GE_Damage, SetByCaller Data.Damage = Weapon->Damage x Damage attribute). Numbers come from the
 * avatar's UGSEnemyData / UGSWeaponData. Server-only.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_EnemyFire : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_EnemyFire();

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
	void OnAimFinished();

	UFUNCTION()
	void OnShotTimer();

	UFUNCTION()
	void OnRecoveryFinished();

	/** Turn to face the current target; returns the target (may be null). */
	AActor* FaceTarget(AGSEnemyCharacter& Enemy) const;

	void FireShot(AGSEnemyCharacter& Enemy);

	int32 ShotsRemaining = 0;

public:
	/** Shots this ability instance has fired (tests). */
	int32 GetShotsFired() const { return ShotsFired; }

private:
	int32 ShotsFired = 0;
};
