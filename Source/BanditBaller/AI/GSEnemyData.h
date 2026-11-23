// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSMeshAttachment.h"
#include "GSEnemyData.generated.h"

class UAnimInstance;
class UGSAbilitySet;
class UGSWeaponData;
class UMaterialInterface;
class USkeletalMesh;
class UStateTree;
class UUserWidget;

/**
 * Everything that defines one enemy type (DA_Enemy_*): stats, look, abilities, AI and reward.
 * AGSEnemyCharacter applies it on spawn.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSEnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FText DisplayName;

	// --- Stats (written to UGSAttributeSet on spawn) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
	float MaxHealth = 60.f;

	/** Base walk speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0", Units = "cm/s"))
	float MoveSpeed = 520.f;

	/** +/- fraction applied randomly per spawn so groups spread out (0.15 = +/-15%). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0", ClampMax = "0.9"))
	float MoveSpeedVariance = 0.15f;

	/** Damage attribute (multiplier on attack damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	// --- Look ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TSubclassOf<UAnimInstance> AnimClass;

	/**
	 * Material per body slot, by the mesh's slot name (SK_LowPolyBody: Skin, Top, Trousers, Shoes, Gloves, Headwear).
	 * Each needs a "TelegraphAmount" scalar parameter (M_LowPoly has one) for the attack wind-up glow.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TMap<FName, TObjectPtr<UMaterialInterface>> SlotMaterials;

	/** Fallback for slots with no SlotMaterials entry (unset = the mesh's own material). Needs "TelegraphAmount" too. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	/**
	 * Props worn or held on the body (a cap, a baton), the same struct the player uses (FGSMeshAttachment). A ranged
	 * enemy's gun comes from Weapon->Mesh instead, so it isn't listed here.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TArray<FGSMeshAttachment> MeshAttachments;

	/** Health bar shown above the head after taking damage (WBP_EnemyHealthBar). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	// --- Abilities / AI ---

	/** Abilities (e.g. GA_MeleeAttack) granted on spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UGSAbilitySet> AbilitySet;

	/** Behavior run by AGSEnemyAIController (ST_BeatCop). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> StateTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", Units = "cm"))
	float SightRadius = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", Units = "cm"))
	float LoseSightRadius = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float PeripheralVisionAngle = 70.f;

	// --- Melee attack (GA_MeleeAttack) ---

	/** Start attacking when the target is within this distance (center to center). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee", meta = (ClampMin = "0", Units = "cm"))
	float AttackRange = 170.f;

	/** Damage per hit, before the Damage multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee", meta = (ClampMin = "0"))
	float AttackDamage = 12.f;

	/** Telegraph time before the hit lands. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee", meta = (ClampMin = "0", Units = "s"))
	float AttackWindUp = 0.55f;

	/** Pause after the hit before the enemy acts again. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee", meta = (ClampMin = "0", Units = "s"))
	float AttackRecovery = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee", meta = (ClampMin = "1", Units = "cm"))
	float AttackSweepRadius = 70.f;

	/** How far in front of the enemy the sweep reaches. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee", meta = (ClampMin = "0", Units = "cm"))
	float AttackSweepDistance = 130.f;

	// --- Ranged attack (GA_EnemyFire) + ranged AI ---

	/**
	 * A ranged type: counts toward a generated wave's ranged share (DA_WaveSettings MaxRangedShare). Set it on every
	 * type that attacks from range (the Patrol Officer; later the Grenadier, Sniper and Drone).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged")
	bool bRanged = false;

	/**
	 * Gun (same data type as player weapons): Damage, FireRateRPM (within a burst), HipSpread, Range, mesh, effects, and
	 * the projectile each shot fires (ProjectileSpeed, look).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged")
	TObjectPtr<UGSWeaponData> Weapon;

	/** Shots aim this far above the target's capsule centre (its chest), where it is when the shot is fired. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (Units = "cm"))
	float AimHeightOffset = 35.f;

	/** Closest distance the enemy likes to fight from. Reposition points are picked between Min and Max. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", Units = "cm"))
	float PreferredRangeMin = 1500.f;

	/** Starts shooting once within this distance and with line of sight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", Units = "cm"))
	float PreferredRangeMax = 2500.f;

	/** Backs off when the target gets closer than this. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", Units = "cm"))
	float RetreatRange = 800.f;

	/** Max time spent backing off before shooting anyway. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", Units = "s"))
	float RetreatMaxTime = 1.5f;

	/** Aim telegraph (laser + body glow) before the burst. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", Units = "s"))
	float AimTelegraphTime = 0.4f;

	/** Shots per burst; spacing comes from Weapon->FireRateRPM. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "1"))
	int32 BurstCount = 4;

	/** Pause after a burst before the enemy repositions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", Units = "s"))
	float BurstRecovery = 0.5f;

	/** Reposition: angle (around the target) moved per reposition, min..max. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float RepositionAngleMin = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float RepositionAngleMax = 60.f;

	/** Give up on a reposition move after this long. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0", Units = "s"))
	float RepositionMaxTime = 2.5f;

	/** Material for the aim laser shown during the telegraph (unlit, emissive). Laser hidden if unset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged")
	TObjectPtr<UMaterialInterface> AimLaserMaterial;

	// --- Wave budget (generated waves past the last DT_Waves row) ---

	/** Budget points this enemy costs when the wave director buys a generated wave. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0.1"))
	float SpawnCost = 1.f;

	/** First round this enemy may appear in a generated wave. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "1"))
	int32 MinRound = 1;

	// --- Reward / death ---

	/** Cash awarded on kill (read by the cash system via OnEnemyKilled). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 CashValue = 10;

	/** XP granted to the killing player right away (x their XPGain attribute). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 XPValue = 10;

	/** Seconds the ragdoll stays before the actor is destroyed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0", Units = "s"))
	float CorpseLifetime = 5.f;
};
