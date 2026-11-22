// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSDynamiteSettings.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

/**
 * Dynamite tuning (DA_DynamiteSettings, set as DynamiteSettings on BP_GSGameMode): the weapon lobs a stick of dynamite
 * at the enemy in range with the most other enemies around it; the stick explodes where it lands and hurts every enemy
 * in the blast (UGSDynamiteComponent, AGSLobbedThrow).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSDynamiteSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Throwing ---

	/** Damage to each enemy in the blast, before the power bonus, the player's Damage attribute and a critical hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0"))
	float Damage = 40.f;

	/** Blast radius before the power bonus (3D distance from the blast center, no line of sight, like Powder Keg). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "cm"))
	float BlastRadius = 300.f;

	/** Seconds between throws (exact schedule; power doesn't change it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0.05", Units = "s"))
	float ThrowInterval = 4.f;

	/** It only throws at living enemies this close to the player. No clear line is needed: it lobs over cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "cm"))
	float Range = 2000.f;

	/** How often it checks for an enemy in range while idle (it picks the target right before each throw). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "s"))
	float TargetRefreshInterval = 0.1f;

	/**
	 * Each point of weapon power (the DynamitePower attribute) adds this much damage and blast radius (0.1 = +10% of the
	 * base per point, added up, not compounded; fractions count). The card's "+10%" / "+5%" come from DA_Upgrade_Dynamite's
	 * DisplayScale / SecondaryDisplayScale; the Dynamite test checks that they match.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing|Power", meta = (ClampMin = "0"))
	float DamageBonusPerPower = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing|Power", meta = (ClampMin = "0"))
	float RadiusBonusPerPower = 0.05f;

	/** The damage effect (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Where sticks leave the player: from the capsule center, in the player's facing frame (X forward, Z up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing")
	FVector ThrowOffset = FVector(20.f, 0.f, 60.f);

	// --- The stick (a lobbed throw) ---

	/** Seconds from the throw to the landing (and the blast). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stick", meta = (ClampMin = "0.05", Units = "s"))
	float FlightTime = 0.6f;

	/** Height of the arc's apex above the straight line to the landing point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stick", meta = (ClampMin = "0", Units = "cm"))
	float ArcHeight = 300.f;

	// --- Look (placeholder) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> StickMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> StickMaterial;

	/** Mesh scale: a short red stick (the engine cylinder is 100 cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector StickScale = FVector(0.08f, 0.08f, 0.3f);

	/** Tumble while it flies (degrees a second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	float StickSpinRate = 720.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> ThrowSound;

	/** The explosion, scaled to the blast radius (BlastEffect at scale 1 is BlastEffectRadius wide, like Powder Keg's). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UNiagaraSystem> BlastEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look", meta = (ClampMin = "1", Units = "cm"))
	float BlastEffectRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> BlastSound;

	float GetDamageMultiplier(float Power) const { return 1.f + DamageBonusPerPower * FMath::Max(Power, 0.f); }
	float GetRadiusMultiplier(float Power) const { return 1.f + RadiusBonusPerPower * FMath::Max(Power, 0.f); }
	float GetBlastRadius(float Power) const { return BlastRadius * GetRadiusMultiplier(Power); }
};
