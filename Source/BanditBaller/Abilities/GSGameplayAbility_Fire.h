// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSGameplayAbility.h"
#include "GSGameplayAbility_Fire.generated.h"

class UGameplayEffect;
class UGSWeaponData;
struct FOnAttributeChangeData;

/**
 * Fires hitscan shots from the equipped weapon and owns State.Firing while active.
 *
 * Each activation fires one shot on the press. A full-auto weapon then keeps firing inside the same activation while
 * the fire input is held (a held burst):
 * - Every shot is scheduled one refire interval after the previous shot's scheduled time, not after the frame a timer
 *   happened to fire on, so the rate is exact at any frame rate. A frame that spans several intervals fires every shot
 *   it owes.
 * - The refire interval (weapon RPM x the owner's FireRate) is read again for every shot, and the pending shot is
 *   re-timed when FireRate changes, so a change mid-burst applies from the next shot.
 * - Before each shot it checks what an activation checks (alive, not reloading, no safe zone, a weapon equipped), then
 *   that the input is still held and the magazine has a round. Otherwise it ends at the time the next shot would have
 *   been due, so State.Firing always lasts one refire interval past the last shot (reload and weapon swap stay blocked
 *   until then). Ending on an empty magazine starts the auto-reload.
 * - The shot timer is a world timer, so pausing the game pauses a burst.
 * Semi-auto weapons fire once per press. Tapping can't beat the refire interval: a press while active does nothing.
 *
 * Trace: camera ray (with spread) finds the aim point, then muzzle -> aim point finds the hit.
 * Server applies DamageEffect with SetByCaller Data.Damage = weapon damage * owner Damage attribute.
 *
 * TODO(co-op): per-shot server sync for a held burst. The burst runs where the input is held (the local player); a
 * client's later shots need to reach the server.
 */
UCLASS()
class BANDITBALLER_API UGSGameplayAbility_Fire : public UGSGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility_Fire();

	//~ UGSGameplayAbility
	/** Full auto: starts a new burst while the input stays held after the last one ended (e.g. after the auto-reload). */
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
	/** What happens when the next shot is due. */
	enum class ENextShot : uint8
	{
		Fire,
		Done,     // released, semi-auto or out of ammo: end normally (an empty magazine auto-reloads)
		Blocked,  // dead, reloading, in a safe zone or no weapon: end as cancelled
	};

	void FireShot(const FGameplayAbilityActorInfo* ActorInfo, const UGSWeaponData& WeaponData);
	void ApplyRecoil(const FGameplayAbilityActorInfo* ActorInfo, const UGSWeaponData& WeaponData) const;

	/** Seconds between shots for WeaponData at the owner's FireRate right now. */
	float GetCurrentRefireInterval(const UGSWeaponData& WeaponData) const;

	ENextShot CheckNextShot() const;

	/** Arms the shot timer for LastShotTime + the refire interval read now (next tick if that time has passed). */
	void ScheduleNextShot();

	/** Timer: fires every shot due by now, then schedules the next one or ends the activation. */
	void OnShotDue();

	void OnFireRateChanged(const FOnAttributeChangeData& Data);

	/** Scheduled world time of the latest shot (the next is due one refire interval later). */
	double LastShotTime = 0.0;

	FTimerHandle ShotTimerHandle;
	FDelegateHandle FireRateChangedHandle;
};
