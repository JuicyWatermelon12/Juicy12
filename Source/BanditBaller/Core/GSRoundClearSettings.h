// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSRoundClearSettings.generated.h"

/**
 * Round-clear payout (DA_RoundClearSettings, set as RoundClearSettings on BP_GSGameMode), paid by AGSGameMode when a
 * round's Combat ends (RoundClear) into the profile.
 * - Every round pays the same amounts: nothing here grows with the round number (flat economy).
 * - No-damage bonus: nobody lost health this round.
 * - Fast-clear bonus: Combat lasted at most FastClearBaseTime + FastClearTimePerEnemy x the round's wave size. The time
 *   limit follows the wave size so the bonus stays reachable in big rounds; set FastClearTimePerEnemy to 0 for a fixed
 *   limit.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSRoundClearSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Paid for every cleared round. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bonus", meta = (ClampMin = "0"))
	int32 BaseBonus = 20;

	/** Paid when no damage was taken this round (health actually lost; blocked hits don't count). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bonus", meta = (ClampMin = "0"))
	int32 NoDamageBonus = 15;

	/** Paid when Combat was cleared within the fast-clear time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bonus", meta = (ClampMin = "0"))
	int32 FastClearBonus = 15;

	/** Fast-clear time limit: this much ... */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fast Clear", meta = (ClampMin = "0", Units = "s"))
	float FastClearBaseTime = 8.f;

	/** ... plus this much per enemy in the round's wave. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fast Clear", meta = (ClampMin = "0", Units = "s"))
	float FastClearTimePerEnemy = 1.5f;

	/** Seconds of Combat that still count as a fast clear for a wave of WaveEnemies. */
	float GetFastClearTime(int32 WaveEnemies) const
	{
		return FMath::Max(0.f, FastClearBaseTime + FastClearTimePerEnemy * FMath::Max(WaveEnemies, 0));
	}
};
