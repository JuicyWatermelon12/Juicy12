// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "GSXPCurve.generated.h"

/**
 * Player level curve (DA_XPCurve, set on BP_GSGameMode). Read on the server by UGSProgressionComponent.
 * XPToReachLevel: X = level, Y = XP needed to go from level X-1 to level X (so X = 2 is the first level-up).
 * New assets start with 100 XP for level 2 and +25% per level up to level 50; edit the keys to tune.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSXPCurve : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSXPCurve()
	{
		// Starting values for new assets (the asset's own keys are what the game reads).
		FRichCurve* Rich = XPToReachLevel.GetRichCurve();
		for (int32 Level = 2; Level <= 50; ++Level)
		{
			const float XP = FMath::RoundToFloat(100.f * FMath::Pow(1.25f, static_cast<float>(Level - 2)));
			Rich->SetKeyInterpMode(Rich->AddKey(static_cast<float>(Level), XP), RCIM_Linear);
		}
	}

	/** XP needed to go from Level-1 to Level. 0 for Level <= 1 or past MaxLevel. */
	int32 GetXPToReachLevel(int32 Level) const
	{
		if (Level <= 1 || Level > MaxLevel)
		{
			return 0;
		}
		const FRichCurve* Rich = XPToReachLevel.GetRichCurveConst();
		if (!Rich || Rich->GetNumKeys() == 0)
		{
			return 0;
		}
		return FMath::Max(1, FMath::RoundToInt(Rich->Eval(static_cast<float>(Level))));
	}

	/** Total XP from level 1 (0 XP) to reach Level. */
	int64 GetTotalXPToReachLevel(int32 Level) const
	{
		int64 Total = 0;
		for (int32 L = 2; L <= FMath::Min(Level, MaxLevel); ++L)
		{
			Total += GetXPToReachLevel(L);
		}
		return Total;
	}

	int32 GetMaxLevel() const { return FMath::Max(1, MaxLevel); }

	/** X = level, Y = XP needed from the previous level. Past the last key the last value repeats. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP")
	FRuntimeFloatCurve XPToReachLevel;

	/** Players stop levelling here; XP past it still counts as earned but fills no bar. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP", meta = (ClampMin = "1"))
	int32 MaxLevel = 50;
};
