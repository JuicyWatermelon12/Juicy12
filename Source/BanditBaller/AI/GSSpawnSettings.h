// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSSpawnSettings.generated.h"

/** Where round spawns may appear (DA_SpawnSettings), read by UGSSpawnLocationFinder via AGSGameMode. */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSSpawnSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Spawns are at least this far from the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0", Units = "cm"))
	float MinRadius = 1200.f;

	/** ...and at most this far. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0", Units = "cm"))
	float MaxRadius = 2500.f;

	/** Minimum distance between spawns chosen in the same batch. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0", Units = "cm"))
	float MinSpacing = 200.f;

	/** Random ring points tried per spawn before falling back to an EnemySpawn point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "1"))
	int32 MaxAttempts = 20;

	/** Reject points a player can see; this rule is dropped after half of MaxAttempts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring")
	bool bPreferOffScreen = true;
};
