// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSEnemyData.generated.h"

class UAnimInstance;
class UGSAbilitySet;
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

	/** Applied to every material slot (placeholder readable color). Needs a "TelegraphAmount" scalar param for attack wind-ups. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	/** Health bar shown above the head after taking damage (WBP_EnemyHealthBar). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	// --- Abilities / AI ---

	/** Abilities (e.g. GA_MeleeAttack) granted on spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UGSAbilitySet> AbilitySet;

	/** Behavior run by AGSEnemyAIController (ST_MeleeBandit). */
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

	// --- Reward / death ---

	/** Cash awarded on kill (read by the cash system via OnEnemyKilled). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 CashValue = 10;

	/** Seconds the ragdoll stays before the actor is destroyed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0", Units = "s"))
	float CorpseLifetime = 5.f;
};
