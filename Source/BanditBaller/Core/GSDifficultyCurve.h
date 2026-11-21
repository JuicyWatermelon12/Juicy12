// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "GSDifficultyCurve.generated.h"

/**
 * Enemy difficulty by round (DA_DifficultyCurve), read by AGSGameMode when an enemy spawns and applied through
 * GE_EnemyRoundScaling. X = round, Y = multiplier on the enemy's base stats from its UGSEnemyData.
 * A curve with no keys means x1.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSDifficultyCurve : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	float GetHealthMultiplier(int32 Round) const { return Evaluate(HealthMultiplierByRound, Round); }
	float GetDamageMultiplier(int32 Round) const { return Evaluate(DamageMultiplierByRound, Round); }

	/** MaxHealth multiplier by round. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty")
	FRuntimeFloatCurve HealthMultiplierByRound;

	/** Damage attribute multiplier by round (scales melee AttackDamage and ranged weapon Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty")
	FRuntimeFloatCurve DamageMultiplierByRound;

private:
	static float Evaluate(const FRuntimeFloatCurve& Curve, int32 Round)
	{
		const FRichCurve* Rich = Curve.GetRichCurveConst();
		return (Rich && Rich->GetNumKeys() > 0) ? FMath::Max(0.01f, Rich->Eval(static_cast<float>(Round))) : 1.f;
	}
};
