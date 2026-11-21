// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GSSpawnLocationFinder.generated.h"

class APlayerController;
class UGSSpawnSettings;

/** One chosen spawn location. */
USTRUCT(BlueprintType)
struct FGSSpawnPoint
{
	GENERATED_BODY()

	/** On the NavMesh (feet level; add the capsule half height when spawning a character). */
	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	FVector Location = FVector::ZeroVector;

	/** True when no ring point worked and an "EnemySpawn" Target Point was used instead. */
	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	bool bFallback = false;

	/** Ring attempts used for this point. */
	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	int32 Attempts = 0;
};

/** Per-call summary (for logs, tests and tuning). */
USTRUCT(BlueprintType)
struct FGSSpawnSearchStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 Requested = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 Ring = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 Fallback = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 Failed = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 Attempts = 0;

	// Rejection reasons across all attempts.
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 RejectedNoNavMesh = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 RejectedOutsideRing = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 RejectedUnreachable = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 RejectedSpacing = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Spawn") int32 RejectedOnScreen = 0;

	FString ToString() const;
};

/**
 * Picks spawn locations for round spawns (and gs.Spawn). Server only.
 *
 * For each point: try up to MaxAttempts random points in the [MinRadius, MaxRadius] ring around the player
 * (run stream, UGSRunRandom), projected to the NavMesh — which only covers the town, so the Market District
 * can't be picked. Reject points that leave the ring, have no complete path to the player, are closer than
 * MinSpacing to an already chosen point, or (bPreferOffScreen, first half of the attempts only) are visible to
 * any player. If all attempts fail, use the "EnemySpawn" Target Point farthest from the player that is on the
 * NavMesh (warning). Refuses entirely while the party is in the Market District.
 * Settings come from AGSGameMode's SpawnSettings (DA_SpawnSettings). gs.Debug.Spawns 1 draws the result.
 */
UCLASS()
class BANDITBALLER_API UGSSpawnLocationFinder : public UObject
{
	GENERATED_BODY()

public:
	/** Up to Count locations around PC's pawn. Fewer if the player has no pawn or nothing valid exists. */
	static TArray<FGSSpawnPoint> FindSpawnLocations(APlayerController* PC, int32 Count, FGSSpawnSearchStats* OutStats = nullptr);

	/** Settings in use for this world (GameMode's asset, or class defaults). */
	static const UGSSpawnSettings& GetSettings(const UWorld* World);

	static const FName EnemySpawnTag;

private:
	static bool IsVisibleToAnyPlayer(const UWorld& World, const FVector& Location);
};
