// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSRoundSettings.generated.h"

/**
 * Round loop timing (DA_RoundSettings), read by AGSGameMode and UGSWaveDirector.
 *
 * Since Prompt 59 rounds roll over: the next round starts while the current one still has a few enemies left, with no
 * countdown (only round 1 has one). Every BreatherEveryRounds rounds the next round waits for a full clear, then a
 * breather of BreatherDuration seconds.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSRoundSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Gap between a round's spawns: RoundSpawnWindow / wave size, kept between MinSpawnGap and MaxSpawnGap. */
	float GetSpawnGap(int32 WaveSize) const
	{
		const float Gap = WaveSize > 0 ? RoundSpawnWindow / static_cast<float>(WaveSize) : MaxSpawnGap;
		return FMath::Clamp(Gap, FMath::Max(MinSpawnGap, 0.01f), FMath::Max(MaxSpawnGap, MinSpawnGap));
	}

	/** Round after which a breather comes (and into which nothing rolls over): 10, 20, 30, ... */
	bool IsBreatherRound(int32 Round) const
	{
		return BreatherEveryRounds > 0 && Round > 0 && Round % BreatherEveryRounds == 0;
	}

	/** Countdown before round 1 (the only countdown: later rounds roll over). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round", meta = (ClampMin = "0", Units = "s"))
	float PreRoundDuration = 3.f;

	/**
	 * Safety guard for the wait before a breather: if the breather round still has enemies this long after it would
	 * have rolled over, the stuck leftovers are removed and the open rounds clear, so one stuck enemy can't hold the run.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round", meta = (ClampMin = "1", Units = "s"))
	float CombatFallbackDuration = 180.f;

	/**
	 * The next round starts as soon as the newest round has this many enemies or fewer left (alive + not spawned yet)
	 * and at least one of its own enemies has died. 0 = only once it is fully cleared.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll-over", meta = (ClampMin = "0"))
	int32 RollOverRemaining = 3;

	/** A round's enemies arrive one at a time, evenly spread over this window... */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll-over", meta = (ClampMin = "0", Units = "s"))
	float RoundSpawnWindow = 20.f;

	/** ...with the gap between two of them kept within these limits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll-over", meta = (ClampMin = "0.01", Units = "s"))
	float MinSpawnGap = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll-over", meta = (ClampMin = "0.01", Units = "s"))
	float MaxSpawnGap = 2.f;

	/** After every this many rounds the next round doesn't roll over: a full clear, then a breather (0 = never). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breather", meta = (ClampMin = "0"))
	int32 BreatherEveryRounds = 10;

	/** Length of the breather (nothing spawns; a placeholder for a boss later). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breather", meta = (ClampMin = "0", Units = "s"))
	float BreatherDuration = 10.f;

	/** After a round's clear the profile is saved once its vacuum has landed, waiting at most this long. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round", meta = (ClampMin = "0", Units = "s"))
	float ClearSaveMaxWait = 3.f;
};
