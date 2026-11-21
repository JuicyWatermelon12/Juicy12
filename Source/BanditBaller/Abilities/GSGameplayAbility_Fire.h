// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_Fire.generated.h"

class UGameplayEffect;
class UGSWeaponData;

/**
 * Fires one hitscan shot from the equipped weapon, then stays active for the refire interval
 * (owns State.Firing). Full-auto weapons are re-triggered while the input is held.
 *
 * Trace: camera ray (with spread) finds the aim point, then muzzle -> aim point finds the hit.
 * Server applies DamageEffect with SetByCaller Data.Damage = weapon damage * owner Damage attribute.
 * Auto-reloads when the magazine is empty.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_Fire : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_Fire();

	//~ UGSGameplayAbility
	virtual bool ShouldRetriggerWhileInputHeld(const FGameplayAbilityActorInfo* ActorInfo) const override;

	//~ UGameplayAbility
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** Instant GE using SetByCaller Data.Damage into IncomingDamage (GE_Damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	void FireShot(const FGameplayAbilityActorInfo* ActorInfo, const UGSWeaponData& WeaponData);
	void ApplyRecoil(const FGameplayAbilityActorInfo* ActorInfo, const UGSWeaponData& WeaponData) const;

	UFUNCTION()
	void OnRefireReady();
};
