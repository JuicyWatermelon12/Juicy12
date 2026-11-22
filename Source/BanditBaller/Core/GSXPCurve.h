// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSXPCurve.generated.h"

/**
 * Player level curve (DA_XPCurve, set on BP_GSGameMode). Read on the server by UGSProgressionComponent.
 * Levels are endless (no cap). The XP needed from level L-1 to level L grows by a fixed amount each level:
 * BaseXP for level 2, then + XPIncreasePerLevel for every level after that (100, 130, 160, ...).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSXPCurve : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * XP needed to go from Level-1 to Level: BaseXP + XPIncreasePerLevel x (Level - 2), at least 1. 0 for Level <= 1.
	 * Clamped to the int32 range (only reached around level 70 million).
	 */
	int32 GetXPToReachLevel(int32 Level) const
	{
		if (Level <= 1)
		{
			return 0;
		}
		const int64 XP = static_cast<int64>(BaseXP) + static_cast<int64>(XPIncreasePerLevel) * (Level - 2);
		return static_cast<int32>(FMath::Clamp<int64>(XP, 1, MAX_int32));
	}

	/** Total XP from level 1 (0 XP) to reach Level (arithmetic series; no loop, so it is cheap at any level). */
	int64 GetTotalXPToReachLevel(int32 Level) const
	{
		if (Level <= 1)
		{
			return 0;
		}
		const int64 Steps = Level - 1; // level-ups from 1 to Level
		const int64 Total = Steps * FMath::Max(BaseXP, 1) + static_cast<int64>(XPIncreasePerLevel) * Steps * (Steps - 1) / 2;
		return FMath::Max<int64>(Total, Steps);
	}

	/** XP needed for the first level-up (level 1 -> 2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP", meta = (ClampMin = "1"))
	int32 BaseXP = 100;

	/** How much more XP each level after level 2 needs than the one before it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP", meta = (ClampMin = "0"))
	int32 XPIncreasePerLevel = 30;
};
