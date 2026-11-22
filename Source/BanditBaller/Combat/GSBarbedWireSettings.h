// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSBarbedWireSettings.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;

/**
 * Barbed Wire tuning (DA_BarbedWireSettings, set as BarbedWireSettings on BP_GSGameMode): a ring on the ground around
 * the player that damages every living enemy inside it on a fixed tick (UGSBarbedWireComponent).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSBarbedWireSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- The ring ---

	/** Ring radius at power 0 (an enemy is inside when its center is this close to the player's, measured flat). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 350.f;

	/** Enemies more than this far above or below the player's center aren't inside (a ring on the ground, not a dome). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0", Units = "cm"))
	float MaxHeightDifference = 200.f;

	/** Damage to each enemy inside per tick, before the power bonus, the player's Damage attribute and a crit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0"))
	float Damage = 5.f;

	/** Seconds between ticks (exact schedule). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0.05", Units = "s"))
	float TickInterval = 0.5f;

	/**
	 * Each point of weapon power (the BarbedWirePower attribute) adds this much damage and radius (0.1 = +10% of the base
	 * per point, added up, not compounded; fractions count). The card's "+10%" / "+5%" come from DA_Upgrade_BarbedWire's
	 * DisplayScale / SecondaryDisplayScale; the Barbed Wire test checks that they match.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring|Power", meta = (ClampMin = "0"))
	float DamageBonusPerPower = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring|Power", meta = (ClampMin = "0"))
	float RadiusBonusPerPower = 0.05f;

	/** The damage effect (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// --- Look (placeholder) ---

	/** The ring is drawn as this many short segments of SegmentMesh (the engine cube), laid in a circle at the feet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look", meta = (ClampMin = "8", ClampMax = "256"))
	int32 RingSegments = 48;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> SegmentMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> SegmentMaterial;

	/** Thickness (width and height) of the ring's wire, in cm. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look", meta = (ClampMin = "0.5", Units = "cm"))
	float WireThickness = 4.f;

	/** How high above the floor the ring floats (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look", meta = (Units = "cm"))
	float HeightAboveFloor = 3.f;

	/** Played on each enemy the ring damages (light). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector HitEffectScale = FVector(0.12f);

	float GetDamageMultiplier(float Power) const { return 1.f + DamageBonusPerPower * FMath::Max(Power, 0.f); }
	float GetRadiusMultiplier(float Power) const { return 1.f + RadiusBonusPerPower * FMath::Max(Power, 0.f); }
	float GetRadius(float Power) const { return Radius * GetRadiusMultiplier(Power); }
};
