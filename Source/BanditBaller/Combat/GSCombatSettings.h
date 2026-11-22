// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSCombatSettings.generated.h"

/** Player combat tuning shared by every damage source (DA_CombatSettings, set as CombatSettings on BP_GSGameMode). */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSCombatSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Damage multiplier of a critical hit (CritChance, Loaded Dice). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Critical Hits", meta = (ClampMin = "1"))
	float CritMultiplier = 2.f;
};
