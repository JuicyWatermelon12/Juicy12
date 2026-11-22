// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GSGameplayAbility.generated.h"

class AGSCharacter;
class UGSWeaponComponent;

/**
 * Base class for all BanditBaller gameplay abilities.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility();

	const FGameplayTag& GetActivationInputTag() const { return ActivationInputTag; }

	/**
	 * If true, the ASC re-activates this ability every frame its input is held (once the previous
	 * activation has ended). Full-auto fire uses it to start a new burst (a held burst itself keeps
	 * firing inside GA_Fire). Called on the ability CDO.
	 */
	virtual bool ShouldRetriggerWhileInputHeld(const FGameplayAbilityActorInfo* ActorInfo) const { return false; }

	/**
	 * Cooldown state for UI. Remaining/Duration describe the current cooldown (or the charge being
	 * recharged); Charges/MaxCharges are 0/1 or 1/1 for single-charge abilities. Called on the CDO.
	 */
	virtual void GetCooldownDisplayInfo(const FGameplayAbilityActorInfo* ActorInfo, float& OutRemaining, float& OutDuration,
		int32& OutCharges, int32& OutMaxCharges) const;

	//~ UGameplayAbility
	/** Applies CooldownGameplayEffectClass with SetByCaller Data.Cooldown = GetCooldownDuration(). */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	/** Cooldown length in seconds (from the ability's settings asset). 0 = no cooldown. */
	virtual float GetCooldownDuration(const FGameplayAbilityActorInfo* ActorInfo) const { return 0.f; }

	/** Input tag that activates this ability. Added to the spec's dynamic source tags when granted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FGameplayTag ActivationInputTag;

	static AGSCharacter* GetGSCharacter(const FGameplayAbilityActorInfo* ActorInfo);
	static UGSWeaponComponent* GetWeaponComponent(const FGameplayAbilityActorInfo* ActorInfo);
};
