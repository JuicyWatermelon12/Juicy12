// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSMolotovSettings.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

/**
 * Molotov tuning (DA_MolotovSettings, set as MolotovSettings on BP_GSGameMode): the weapon throws a bottle at the nearest
 * enemy in range; where it lands it leaves a patch of fire that burns the enemies standing in it on a fixed tick
 * (UGSMolotovComponent, AGSLobbedThrow, AGSFirePatch).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSMolotovSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Throwing ---

	/** Seconds between throws (exact schedule; power doesn't change it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0.05", Units = "s"))
	float ThrowInterval = 5.f;

	/** It only throws at living enemies this close to the player. No clear line is needed: it lobs over cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "cm"))
	float Range = 1500.f;

	/** How often it looks for the nearest target while idle (it picks the target again right before each throw). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "s"))
	float TargetRefreshInterval = 0.1f;

	/** Seconds from the throw to the landing (and the fire). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0.05", Units = "s"))
	float FlightTime = 0.6f;

	/** Height of the arc's apex above the straight line to the landing point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "cm"))
	float ArcHeight = 300.f;

	/**
	 * How far ahead of a moving target the bottle lands: its flat velocity x the flight time x this, on the ground under
	 * that point (the patch is centered there). 1 = where it will be when the bottle lands, 0 = where it stood at the throw.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0"))
	float LeadFactor = 1.f;

	/** Where bottles leave the player: from the capsule center, in the player's facing frame (X forward, Z up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing")
	FVector ThrowOffset = FVector(20.f, 0.f, 60.f);

	// --- The fire patch ---

	/** Damage to each enemy in the patch per burn, before the power bonus, the player's Damage attribute and a crit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0"))
	float BurnDamage = 4.f;

	/** Seconds between burns (the first one when the bottle lands). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.05", Units = "s"))
	float BurnInterval = 0.5f;

	/** How long a patch burns (game time; it pauses with the game). 4 s at 0.5 s = 8 burns. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.05", Units = "s"))
	float PatchDuration = 4.f;

	/** Patch radius before the power bonus: an enemy burns while its center is this close to the patch center, flat. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0", Units = "cm"))
	float PatchRadius = 250.f;

	/** ... and no more than this far above or below the patch (an enemy on a roof above it doesn't burn). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0", Units = "cm"))
	float MaxHeightDifference = 200.f;

	/**
	 * Each point of weapon power (the MolotovPower attribute) adds this much burn damage and patch radius (0.1 = +10% of
	 * the base per point, added up, not compounded; fractions count). The card's "+10%" / "+5%" come from
	 * DA_Upgrade_Molotov's DisplayScale / SecondaryDisplayScale; the Molotov test checks that they match.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire|Power", meta = (ClampMin = "0"))
	float DamageBonusPerPower = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire|Power", meta = (ClampMin = "0"))
	float RadiusBonusPerPower = 0.05f;

	/** The damage effect (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// --- Look (placeholder) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> BottleMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> BottleMaterial;

	/** Mesh scale: a small bottle (the engine cylinder is 100 cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector BottleScale = FVector(0.08f, 0.08f, 0.22f);

	/** Tumble while it flies (degrees a second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	float BottleSpinRate = 540.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> ThrowSound;

	/** The patch: a flat disc of PatchMesh (the engine cylinder, 100 cm wide) scaled to the radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> PatchMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> PatchMaterial;

	/** The disc's thickness and how high it sits above the floor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look", meta = (ClampMin = "0", Units = "cm"))
	float PatchThickness = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look", meta = (ClampMin = "0", Units = "cm"))
	float PatchHeightAboveFloor = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> LandSound;

	float GetDamageMultiplier(float Power) const { return 1.f + DamageBonusPerPower * FMath::Max(Power, 0.f); }
	float GetRadiusMultiplier(float Power) const { return 1.f + RadiusBonusPerPower * FMath::Max(Power, 0.f); }
	float GetPatchRadius(float Power) const { return PatchRadius * GetRadiusMultiplier(Power); }
};
