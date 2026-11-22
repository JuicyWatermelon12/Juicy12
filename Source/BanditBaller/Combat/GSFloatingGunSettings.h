// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSFloatingGunSettings.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

/**
 * Floating Gun tuning (DA_FloatingGunSettings, set as FloatingGunSettings on BP_GSGameMode): shooting, the projectile,
 * where the gun floats, keeping it off the crosshair, and the placeholder look.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSFloatingGunSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Shooting ---

	/** Damage per hit, before the level bonus, the player's Damage attribute and a critical hit (Loaded Dice). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting", meta = (ClampMin = "0"))
	float Damage = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting", meta = (ClampMin = "0.01"))
	float ShotsPerSecond = 1.5f;

	/** It only targets living enemies this close (from the gun) with a clear line (world geometry blocks it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting", meta = (ClampMin = "0", Units = "cm"))
	float Range = 2000.f;

	/** How often the gun looks for the nearest target (it also checks right before each shot). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting", meta = (ClampMin = "0", Units = "s"))
	float TargetRefreshInterval = 0.1f;

	/** Aim where a moving target will be when the projectile arrives (one step of lead). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting")
	bool bLeadTargets = true;

	/**
	 * Each point of weapon power (the FloatingGunPower attribute: Common picks after the two guns add 1, a Rare one
	 * 1.5, ...) adds this much damage and fire rate to both guns (0.1 = +10% of the base per point, added up, not
	 * compounded: power 3 = x1.3; fractions count). The upgrade card's "+10%" text is typed by hand; the Floating Gun
	 * test checks that it matches these.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting|Power", meta = (ClampMin = "0"))
	float DamageBonusPerPower = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting|Power", meta = (ClampMin = "0"))
	float FireRateBonusPerPower = 0.1f;

	/** The damage effect (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// --- Projectile ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1", Units = "cm/s"))
	float ProjectileSpeed = 6000.f;

	/** Seconds before an unspent projectile disappears. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.01", Units = "s"))
	float ProjectileLifetime = 0.75f;

	/** Radius of the projectile's sweep. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0", Units = "cm"))
	float ProjectileRadius = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UMaterialInterface> ProjectileMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector ProjectileScale = FVector(0.12f);

	/** Projectiles created up front the first time one is fired. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0"))
	int32 ProjectilePoolPrewarm = 16;

	// --- Where it floats ---

	/**
	 * Offset from the player's capsule center in the camera's yaw frame (X forward, Y right, Z up), so it always floats
	 * over the right shoulder as the player sees it, whichever way the character faces. The capsule center is at the
	 * hips; the top of the head is about 90 cm above it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	FVector ShoulderOffset = FVector(-15.f, 45.f, 115.f);

	/** How fast it catches up with where it should be (VInterpTo speed), and how fast it turns to its target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0"))
	float FollowSpeed = 14.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0"))
	float TurnSpeed = 12.f;

	/**
	 * Keep it off the crosshair (local player only): if the gun is closer to the screen center than this (a fraction of
	 * the screen height), it slides away from the center until it is clear, and eases back once it is well clear.
	 * 0 turns it off.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0", ClampMax = "0.5"))
	float CrosshairClearRadius = 0.1f;

	/** How fast it slides away from the crosshair, and the furthest it may slide (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0", Units = "cm/s"))
	float CrosshairAvoidSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0", Units = "cm"))
	float CrosshairMaxAvoid = 150.f;

	// --- Look (placeholder) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> GunMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> GunMaterial;

	/** Mesh scale; the mesh's X axis is the barrel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector GunScale = FVector(0.4f, 0.1f, 0.12f);

	/** Where projectiles start: this far in front of the gun's center. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look", meta = (ClampMin = "0", Units = "cm"))
	float MuzzleOffset = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> FireSound;

	/** Played where a projectile hits something. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector ImpactEffectScale = FVector(0.25f);

	/** The rule (not tuning): the first pick = one gun (right shoulder), the second and later = two guns, never more. */
	static constexpr int32 MaxGuns = 2;
	static int32 GetGunCount(int32 Picks) { return FMath::Clamp(Picks, 0, MaxGuns); }

	float GetDamageMultiplier(float Power) const { return 1.f + DamageBonusPerPower * FMath::Max(Power, 0.f); }
	float GetFireRateMultiplier(float Power) const { return 1.f + FireRateBonusPerPower * FMath::Max(Power, 0.f); }

	/** Seconds between one gun's shots at weapon power Power. */
	double GetShotInterval(float Power = 0.f) const
	{
		return 1.0 / FMath::Max(static_cast<double>(ShotsPerSecond) * GetFireRateMultiplier(Power), 0.01);
	}
};
