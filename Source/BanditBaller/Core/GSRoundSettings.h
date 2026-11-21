// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSRoundSettings.generated.h"

/** Round loop timing (DA_RoundSettings), read by AGSGameMode. */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSRoundSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Countdown before combat starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round", meta = (ClampMin = "0", Units = "s"))
	float PreRoundDuration = 3.f;

	/** Combat ends after this long even if enemies remain (fallback until "all enemies dead" ends it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round", meta = (ClampMin = "1", Units = "s"))
	float CombatFallbackDuration = 60.f;

	/** Pause after combat (results / breather) before the next round. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round", meta = (ClampMin = "0", Units = "s"))
	float RoundClearDuration = 4.f;
};
