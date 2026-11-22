// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSFloatingGun.generated.h"

class AGSCharacter;
class AGSEnemyCharacter;
class UGSFloatingGunSettings;
class UStaticMeshComponent;

/**
 * One Floating Gun (spawned by UGSFloatingGunComponent on the player's character). Visual only: no collision.
 *
 * - Placement: over the player's shoulder (Side +1 right, -1 left) at DA_FloatingGunSettings ShoulderOffset in the
 *   camera's yaw frame, followed smoothly. For the local player it slides away from the crosshair when it gets within
 *   CrosshairClearRadius of the screen center.
 * - Targeting (server): the nearest living enemy within Range of the gun with a clear line (world geometry blocks it),
 *   refreshed every TargetRefreshInterval and checked again before each shot. With two guns each picks on its own and
 *   skips the other gun's target unless it is the only one. It turns to face it.
 * - Level (FloatingGunLevel): from level 3 on, DA_FloatingGunSettings' per-level damage and fire rate bonus.
 * - Shooting (server): ShotsPerSecond (x the level bonus) on an exact schedule like GA_Fire's held burst (Prompt 31b): each shot is due one
 *   interval after the previous shot's scheduled time, never after the frame it happened to fire on; a long frame fires
 *   every shot it owes, and a late shot starts that much further along its path. While it can't shoot, nothing is
 *   banked: the next target is shot at once, but never sooner than one interval after the last shot.
 * - It doesn't shoot while the player is dead or in a safe zone; a paused game doesn't tick it.
 * - Projectiles: UGSProjectileSubsystem, Damage x the player's Damage attribute, crits rolled per hit, GE_Damage from
 *   the player's ASC so its kills are the player's. Piercing Rounds doesn't apply.
 *
 * TODO(co-op): not replicated (single-player first); clients need the gun and its target in M6.
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSFloatingGun : public AActor
{
	GENERATED_BODY()

public:
	AGSFloatingGun();

	virtual void Tick(float DeltaSeconds) override;

	/** Set up for OwnerCharacter on Side (+1 right shoulder, -1 left) and snap into place. */
	void InitGun(AGSCharacter* OwnerCharacter, const UGSFloatingGunSettings* InSettings, float InSide);

	/** Where projectiles start. */
	FVector GetMuzzleLocation() const;

	/**
	 * The nearest living enemy in range with a clear line, preferring one the other gun isn't shooting at (it only takes
	 * the other gun's target when nothing else qualifies), or nullptr (server).
	 */
	AGSEnemyCharacter* FindTarget() const;

	/** FindTarget, avoiding Avoid instead of the other gun's target. */
	AGSEnemyCharacter* FindTargetAvoiding(const AActor* Avoid) const;

	/** The owner's FloatingGunLevel (0 without an owner). */
	int32 GetLevel() const;

	/** Seconds between shots and the damage multiplier at the current level (DA_FloatingGunSettings level bonus). */
	double GetShotInterval() const;
	float GetDamageMultiplier() const;

	/** The owner's other Floating Gun, if there is one. */
	AGSFloatingGun* FindOtherGun() const;

	/** The player can't make it shoot now: dead, in a safe zone, or no settings / owner. */
	bool CanShoot() const;

	// --- State for tests / debug ---
	AGSEnemyCharacter* GetTarget() const { return Target.Get(); }
	int32 GetShotsFired() const { return ShotsFired; }
	/** Scheduled time of the first shot of the current burst (shots at one target without a pause), and its shots. */
	double GetBurstStartTime() const { return BurstStartTime; }
	int32 GetBurstShots() const { return BurstShots; }
	/** The interval the current burst's schedule uses (it starts over when the fire rate changes). */
	double GetBurstInterval() const { return BurstInterval; }
	/** Where it floats without the crosshair push, and the current push (cm). */
	FVector GetRestLocation() const { return RestLocation; }
	FVector GetCrosshairAvoidOffset() const { return AvoidOffset; }
	/** Distance of the gun from the screen center as a fraction of the screen height (-1 off screen / no local view). */
	float GetCrosshairDistance() const { return LastCrosshairDistance; }
	float GetSide() const { return Side; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Floating Gun")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	void UpdatePlacement(float DeltaSeconds, bool bSnap);
	void UpdateCrosshairClearance(float DeltaSeconds);
	void UpdateShooting(float DeltaSeconds);
	void FireAt(AGSEnemyCharacter* AtTarget, float Lateness);
	bool HasClearLine(const AActor* Candidate) const;

	TWeakObjectPtr<AGSCharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<const UGSFloatingGunSettings> Settings;

	float Side = 1.f;
	FVector RestLocation = FVector::ZeroVector;
	FVector AvoidOffset = FVector::ZeroVector;
	float LastCrosshairDistance = -1.f;

	TWeakObjectPtr<AGSEnemyCharacter> Target;
	float TargetRefreshRemaining = 0.f;
	double NextShotTime = 0.0;
	bool bShooting = false;
	double BurstStartTime = 0.0;
	double BurstInterval = 0.0;
	double LastShotTime = 0.0;
	int32 BurstShots = 0;
	int32 ShotsFired = 0;
};
