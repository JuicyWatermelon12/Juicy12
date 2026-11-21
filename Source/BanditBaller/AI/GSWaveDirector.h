// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "GSWaveDirector.generated.h"

class AGSEnemyCharacter;
class UGSEnemyData;

/** One enemy type in a wave. */
USTRUCT(BlueprintType)
struct FGSWaveEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TObjectPtr<UGSEnemyData> EnemyData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0"))
	int32 Count = 1;
};

/** DT_Waves row, one per round ("Round1", "Round2", ...). Rounds past the table reuse the last row. */
USTRUCT(BlueprintType)
struct FGSWaveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FGSWaveEntry> Enemies;

	/** Seconds between spawn batches (the first batch spawns when Combat starts). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.1", Units = "s"))
	float SpawnInterval = 2.f;

	/** Enemies spawned per batch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "1"))
	int32 SpawnBatchSize = 2;
};

DECLARE_MULTICAST_DELEGATE(FGSOnWaveCleared);

/**
 * Server-only component on AGSGameMode that runs a round's wave during Combat:
 * reads the round's FGSWaveRow from DT_Waves, shuffles the enemy list with the run stream, spawns
 * SpawnBatchSize enemies every SpawnInterval at UGSSpawnLocationFinder points around a random living
 * player, tracks spawned / killed / alive, mirrors the counts onto AGSGameState, and fires OnWaveCleared
 * once every enemy of the wave has spawned and none are left alive.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSWaveDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSWaveDirector();

	/** Begin the wave for Round (spawns the first batch immediately). */
	void StartWave(int32 Round);

	/** Stop spawning. bDespawnLeftovers removes still-alive wave enemies (not counted as kills). */
	void StopWave(bool bDespawnLeftovers);

	bool IsWaveActive() const { return bWaveActive; }
	int32 GetTotal() const { return Total; }
	int32 GetSpawned() const { return Spawned; }
	int32 GetKilled() const { return Killed; }
	int32 GetAlive() const { return WaveEnemies.Num(); }
	/** Not yet killed (alive + still to spawn). */
	int32 GetRemaining() const { return FMath::Max(0, Total - Killed - Lost); }

	/** The row for Round (or the last row for rounds past the table); nullptr if the table is empty / unset. */
	const FGSWaveRow* FindWaveRow(int32 Round, FName* OutRowName = nullptr) const;

	/** Fires when every enemy of the active wave has spawned and died. */
	FGSOnWaveCleared OnWaveCleared;

protected:
	virtual void BeginPlay() override;

	/** DT_Waves (row struct FGSWaveRow). */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (RequiredAssetDataTags = "RowStructure=/Script/BanditBaller.GSWaveRow"))
	TObjectPtr<UDataTable> WavesTable;

private:
	void SpawnNextBatch();
	void CheckCleared();
	void PushCounts() const;
	bool RemoveWaveEnemy(const AActor* Actor);

	UFUNCTION()
	void HandleEnemyKilled(AActor* Enemy, AActor* Killer);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	/** Enemies still to spawn (shuffled; spawned from the back). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGSEnemyData>> SpawnQueue;

	/** Spawned wave enemies that are still alive. */
	TArray<TWeakObjectPtr<AGSEnemyCharacter>> WaveEnemies;

	FTimerHandle SpawnTimer;
	bool bWaveActive = false;
	int32 WaveRound = 0;
	int32 Total = 0;
	int32 Spawned = 0;
	int32 Killed = 0;
	/** Removed without dying (fell out of the world, debug removal). */
	int32 Lost = 0;
	int32 BatchSize = 1;
	double WaveStartTime = 0.0;

	// Fairness bookkeeping for the wave summary log.
	int32 OnScreenSpawns = 0;
	int32 FallbackSpawns = 0;
	float MinSpawnDistance = 0.f;
};
