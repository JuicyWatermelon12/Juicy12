// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "GSWaveSettings.generated.h"

class AGSSpawnTelegraph;
class UGSEnemyData;
class UMaterialInterface;

/**
 * Wave tuning for UGSWaveDirector (DA_WaveSettings): budget for generated waves (rounds past the last DT_Waves
 * row), the enemy pool they buy from, the alive cap and the spawn telegraph.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSWaveSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Budget for a generated round: BudgetCurve(round) if the curve has keys, else BudgetBase + BudgetPerRound * round. */
	float GetBudget(int32 Round) const
	{
		const FRichCurve* Curve = BudgetCurve.GetRichCurveConst();
		if (Curve && Curve->GetNumKeys() > 0)
		{
			return FMath::Max(0.f, Curve->Eval(static_cast<float>(Round)));
		}
		return FMath::Max(0.f, BudgetBase + BudgetPerRound * Round);
	}

	// --- Generated waves ---

	/** Enemies a generated wave can buy (each needs SpawnCost and MinRound set in its UGSEnemyData). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget")
	TArray<TObjectPtr<UGSEnemyData>> EnemyPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "0"))
	float BudgetBase = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "0"))
	float BudgetPerRound = 2.5f;

	/** Optional: budget by round (X = round, Y = budget). Overrides Base + PerRound when it has keys. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget")
	FRuntimeFloatCurve BudgetCurve;

	/**
	 * At most this share of a generated wave is ranged (UGSEnemyData::bRanged, all ranged types together), by count.
	 * Budget the wave can't spend on ranged enemies goes to the other types. DT_Waves rows are not capped.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxRangedShare = 0.4f;

	/** Safety cap on how many enemies one generated wave may contain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "1"))
	int32 MaxGeneratedEnemies = 150;

	// The spawn interval and batch size were removed in Prompt 59: every round's enemies arrive one at a time over
	// DA_RoundSettings RoundSpawnWindow.

	// --- All waves ---

	/** Max enemies alive at once, every open round together (spawns showing their telegraph count too); extra spawns wait. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Limits", meta = (ClampMin = "1"))
	int32 MaxAliveEnemies = 30;

	/** Ground marker shown at a spawn point this long before the enemy appears (0 = spawn instantly). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0", Units = "s"))
	float SpawnTelegraphTime = 0.75f;

	/** Marker material (unlit emissive works best, e.g. M_EnemyLaser). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	TObjectPtr<UMaterialInterface> TelegraphMaterial;
};
