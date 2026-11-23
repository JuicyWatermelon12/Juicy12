// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSWeaponData.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

UENUM(BlueprintType)
enum class EGSFireMode : uint8
{
	SemiAuto,
	FullAuto
};

/**
 * All tuning for one weapon. Effects/sound/mesh are optional placeholders.
 * Final damage = Damage * owner's Damage attribute; fire rate = FireRateRPM * owner's FireRate attribute.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Seconds between shots after applying the owner's FireRate multiplier. */
	float GetRefireInterval(float FireRateMultiplier) const
	{
		const float Rpm = FMath::Max(FireRateRPM * FMath::Max(FireRateMultiplier, 0.01f), 1.f);
		return 60.f / Rpm;
	}

	// --- Presentation ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TObjectPtr<UStaticMesh> Mesh;

	/** Relative transform of the weapon actor on the attach socket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FTransform MeshTransform;

	/** Character mesh socket/bone the weapon attaches to. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FName AttachSocket = TEXT("hand_r");

	/** Muzzle position in weapon-mesh space, used when the mesh has no "Muzzle" socket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FVector MuzzleOffset = FVector(50.f, 0.f, 0.f);

	// --- Firing ---

	/** Base damage per hit, before the owner's Damage multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0"))
	float Damage = 20.f;

	/** Rounds per minute, before the owner's FireRate multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "1"))
	float FireRateRPM = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing")
	EGSFireMode FireMode = EGSFireMode::FullAuto;

	/** Cone half-angle (degrees) when firing from the hip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0", Units = "Degrees"))
	float HipSpread = 2.f;

	/** Cone half-angle (degrees) while aiming down sights. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0", Units = "Degrees"))
	float ADSSpread = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0", Units = "cm"))
	float Range = 10000.f;

	// --- Ammo ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0", Units = "s"))
	float ReloadTime = 1.6f;

	// --- Recoil ---

	/** Upward camera kick per shot (degrees). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0", Units = "Degrees"))
	float RecoilKick = 0.6f;

	/** Random horizontal kick range per shot (+/- degrees). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0", Units = "Degrees"))
	float RecoilYaw = 0.25f;

	// --- Effects (optional) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<USoundBase> FireSound;

	// --- Projectile (enemy weapons: GA_EnemyFire fires a dodgeable projectile per shot, never a hit-scan) ---

	/** Flight speed (cm/s). The shot flies straight for up to Range (lifetime = Range / speed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "100", Units = "cm/s"))
	float ProjectileSpeed = 2800.f;

	/** Radius of the sphere the shot sweeps (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1", Units = "cm"))
	float ProjectileRadius = 10.f;

	/** The bolt's look: mesh (pointing along +X), material and scale. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UMaterialInterface> ProjectileMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector ProjectileScale = FVector(1.f);

	/** Played where the shot hits the player or the world (not when it runs out of range). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UNiagaraSystem> ProjectileImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector ProjectileImpactEffectScale = FVector(0.2f);

	/** Played where the shot hits (optional). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USoundBase> ProjectileImpactSound;
};
