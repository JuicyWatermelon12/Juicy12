// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_Dash.generated.h"

class UGSDashSettings;

/**
 * Short burst in the movement-input direction (camera forward with no input); works in the air.
 * Owns State.Dashing; adds State.Invulnerable for the i-frame window (GE_Damage is blocked by it).
 * Charges: each dash stacks GE_Cooldown_Dash; usable while stacks < max charges = DA_DashSettings MaxCharges + the
 * owner's ExtraDashCharges attribute (Dash Charge upgrade). The attribute is read live, so a pick counts at once.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_Dash : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_Dash();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void GetCooldownDisplayInfo(const FGameplayAbilityActorInfo* ActorInfo, float& OutRemaining, float& OutDuration,
		int32& OutCharges, int32& OutMaxCharges) const override;

	/** DA_DashSettings MaxCharges (at least 1) + the owner's ExtraDashCharges (rounded, never negative). */
	int32 GetMaxCharges(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** Charges ready to use now: max charges - charges still recharging (never below 0). */
	int32 GetAvailableCharges(const FGameplayAbilityActorInfo* ActorInfo) const;

	const UGSDashSettings* GetSettings() const { return Settings; }

	/** The GA_Dash granted to ASC (the ability CDO, which GetMaxCharges / GetAvailableCharges are called on), or null. */
	static const UGSGameplayAbility_Dash* FindGrantedDash(const UAbilitySystemComponent* ASC);

protected:
	virtual float GetCooldownDuration(const FGameplayAbilityActorInfo* ActorInfo) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	TObjectPtr<UGSDashSettings> Settings;

private:
	int32 GetUsedCharges(const FGameplayAbilityActorInfo* ActorInfo) const;

	void OnDashFinished();

	UFUNCTION()
	void OnInvulnerabilityExpired();

	void RemoveInvulnerability();
	void TryEnd();

	FDelegateHandle DashFinishedHandle;
	bool bDashMoveDone = false;
	bool bInvulnerable = false;
};
