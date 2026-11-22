// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSLuckyCoinSettings.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

/**
 * Lucky Coin tuning (DA_LuckyCoinSettings, set as LuckyCoinSettings on BP_GSGameMode): the weapon flips a coin at the
 * nearest enemy in range with a clear line; after a hit the coin bounces to the nearest other enemy within BounceRange,
 * never hitting the same enemy twice in one flip (UGSLuckyCoinComponent, AGSProjectile).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSLuckyCoinSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Flipping ---

	/** Damage per hit, before the power bonus, the player's Damage attribute and a critical hit (Loaded Dice). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping", meta = (ClampMin = "0"))
	float Damage = 10.f;

	/** Seconds between flips (exact schedule; power doesn't change it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping", meta = (ClampMin = "0.05", Units = "s"))
	float FlipInterval = 1.5f;

	/** The first target: the nearest living enemy this close to the player with a clear line (world geometry blocks it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping", meta = (ClampMin = "0", Units = "cm"))
	float Range = 2000.f;

	/** A bounce goes to the nearest other living enemy this close to the enemy just hit, with a clear line. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping", meta = (ClampMin = "0", Units = "cm"))
	float BounceRange = 800.f;

	/** Bounces after the first hit at power 0 (so a flip hits up to 1 + bounces enemies). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping", meta = (ClampMin = "0"))
	int32 BaseBounces = 3;

	/** Every this many full points of weapon power add one bounce (2: power 2 = +1, power 4 = +2; fractions don't count). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping|Power", meta = (ClampMin = "0.01"))
	float PowerPerBounce = 2.f;

	/**
	 * Each point of weapon power (the LuckyCoinPower attribute) adds this much damage (0.1 = +10% of the base per point,
	 * added up, not compounded; fractions count). The card's "+10%" comes from DA_Upgrade_LuckyCoin's DisplayScale; the
	 * Lucky Coin test checks that they match.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping|Power", meta = (ClampMin = "0"))
	float DamageBonusPerPower = 0.1f;

	/** How often it looks for the nearest target (it also checks right before each flip). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping", meta = (ClampMin = "0", Units = "s"))
	float TargetRefreshInterval = 0.1f;

	/** The damage effect (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Where coins leave the player: from the capsule center, in the player's facing frame (X forward, Z up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipping")
	FVector ThrowOffset = FVector(30.f, 0.f, 40.f);

	// --- The coin (a homing projectile) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "1", Units = "cm/s"))
	float CoinSpeed = 2500.f;

	/** How fast a coin turns toward its target (degrees a second). Its speed stays the same. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "0"))
	float HomingTurnRate = 1440.f;

	/** Seconds before a coin that hit nothing disappears (the flip ends there). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "0.01", Units = "s"))
	float CoinLifetime = 1.f;

	/** Radius of the coin's sweep. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "0", Units = "cm"))
	float CoinRadius = 12.f;

	/** Projectiles created up front the first time a coin is flipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "0"))
	int32 PoolPrewarm = 6;

	// --- Look (placeholder) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> CoinMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> CoinMaterial;

	/** Mesh scale: a small flat disc (the engine cylinder is 100 cm wide). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector CoinScale = FVector(0.12f, 0.12f, 0.02f);

	/** Spin while it flies (degrees a second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	float CoinSpinRate = 1440.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> FlipSound;

	/** Played where a coin hits something. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector ImpactEffectScale = FVector(0.15f);

	float GetDamageMultiplier(float Power) const { return 1.f + DamageBonusPerPower * FMath::Max(Power, 0.f); }

	/** Bounces at weapon power Power: BaseBounces + one per PowerPerBounce full points. */
	int32 GetBounces(float Power) const
	{
		return FMath::Max(BaseBounces, 0) + FMath::FloorToInt(FMath::Max(Power, 0.f) / FMath::Max(PowerPerBounce, 0.01f) + 1e-4f);
	}
};
