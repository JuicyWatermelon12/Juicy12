// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSTomahawkSettings.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

/**
 * Tomahawk tuning (DA_TomahawkSettings, set as TomahawkSettings on BP_GSGameMode): the weapon throws a tomahawk at the
 * nearest enemy in range with a clear line; it flies straight out, hurting every enemy it passes, then comes back to
 * the player and hurts them again (UGSTomahawkComponent, AGSTomahawk).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSTomahawkSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Throwing ---

	/** Damage per hit, before the power bonus, the player's Damage attribute and a critical hit (Loaded Dice). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0"))
	float Damage = 12.f;

	/** Seconds between throws (exact schedule; power doesn't change it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0.05", Units = "s"))
	float ThrowInterval = 3.f;

	/** It only throws at living enemies this close to the throw point with a clear line (world geometry blocks it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "cm"))
	float Range = 1800.f;

	/** How often it looks for targets while idle (it picks the targets again right before each throw). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "s"))
	float TargetRefreshInterval = 0.1f;

	/** Tomahawks per throw at power 0, each at the next nearest target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "1"))
	int32 BaseTomahawks = 1;

	/** Every this many full points of weapon power add one tomahawk (3: power 3 = +1, power 6 = +2; fractions don't count). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing|Power", meta = (ClampMin = "0.01"))
	float PowerPerTomahawk = 3.f;

	/**
	 * Each point of weapon power (the TomahawkPower attribute) adds this much damage (0.1 = +10% of the base per point,
	 * added up, not compounded; fractions count). The card's "+10%" comes from DA_Upgrade_Tomahawk's DisplayScale; the
	 * Tomahawk test checks that they match.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing|Power", meta = (ClampMin = "0"))
	float DamageBonusPerPower = 0.1f;

	/** The damage effect (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Where tomahawks leave the player: from the capsule center, in the player's facing frame (X forward, Z up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing")
	FVector ThrowOffset = FVector(30.f, 0.f, 40.f);

	// --- The tomahawk ---

	/** How far it flies out (level, toward the target) before it turns back; a wall turns it back sooner. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tomahawk", meta = (ClampMin = "0", Units = "cm"))
	float Distance = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tomahawk", meta = (ClampMin = "1", Units = "cm/s"))
	float Speed = 2000.f;

	/** Radius of its sweep (enemies whose capsule it touches are hit). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tomahawk", meta = (ClampMin = "0", Units = "cm"))
	float SweepRadius = 30.f;

	/** It is caught (gone) this close to the player on the way back. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tomahawk", meta = (ClampMin = "0", Units = "cm"))
	float CatchDistance = 100.f;

	/** A safety limit: it is gone after this long in the air whatever happens. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tomahawk", meta = (ClampMin = "0.1", Units = "s"))
	float MaxFlightTime = 4.f;

	// --- Look (placeholder) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> TomahawkMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> TomahawkMaterial;

	/** Mesh scale: a flat grey wedge (the engine cone, 100 cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector TomahawkScale = FVector(0.3f, 0.05f, 0.3f);

	/** Spin while it flies (degrees a second, end over end). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	float SpinRate = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> ThrowSound;

	/** Played on every enemy hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector HitEffectScale = FVector(0.1f);

	float GetDamageMultiplier(float Power) const { return 1.f + DamageBonusPerPower * FMath::Max(Power, 0.f); }

	/** Tomahawks per throw at weapon power Power: BaseTomahawks + one per PowerPerTomahawk full points. */
	int32 GetTomahawks(float Power) const
	{
		return FMath::Max(BaseTomahawks, 1) + FMath::FloorToInt(FMath::Max(Power, 0.f) / FMath::Max(PowerPerTomahawk, 0.01f) + 1e-4f);
	}
};
