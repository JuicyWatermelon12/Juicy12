// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "GSWaveDirector.generated.h"

class AGSEnemyCharacter;
class AGSSpawnTelegraph;
class UGSEnemyData;
class UGSWaveSettings;

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

/** DT_Waves row, one per round ("Round1", "Round2", ...). Rounds past the last row are generated from a budget. */
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

/** A spawn point that is showing its telegraph; the enemy appears at SpawnAt. */
USTRUCT()
struct FGSPendingSpawn
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UGSEnemyData> EnemyData;

	FVector Location = FVector::ZeroVector;
	FVector FaceTowards = FVector::ZeroVector;
	double SpawnAt = 0.0;
	TWeakObjectPtr<AGSSpawnTelegraph> Telegraph;
};

DECLARE_MULTICAST_DELEGATE(FGSOnWaveCleared);

/**
 * Server-only component on AGSGameMode that runs a round's wave during Combat.
 *
 * Wave contents: the round's DT_Waves row ("RoundN"); for rounds past the last row, a wave bought from a budget
 * (UGSWaveSettings: Base + PerRound x round, or a curve) out of the enemy pool using each enemy's SpawnCost and
 * MinRound and the run stream. The list is shuffled with the run stream.
 *
 * Spawning: a batch every interval at UGSSpawnLocationFinder points around a random living player, limited by
 * MaxAliveEnemies (alive + telegraphing; extra spawns wait). Each point shows an AGSSpawnTelegraph ground marker
 * for SpawnTelegraphTime before the enemy appears. Tracks spawned / killed / alive, mirrors counts onto
 * AGSGameState and fires OnWaveCleared once every enemy has spawned and died.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSWaveDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSWaveDirector();

	/** Begin the wave for Round (the first batch is chosen immediately). */
	void StartWave(int32 Round);

	/** Stop spawning and cancel telegraphs. bDespawnLeftovers removes still-alive wave enemies (not kills). */
	void StopWave(bool bDespawnLeftovers);

	bool IsWaveActive() const { return bWaveActive; }
	int32 GetTotal() const { return Total; }
	int32 GetSpawned() const { return Spawned; }
	int32 GetKilled() const { return Killed; }
	int32 GetAlive() const { return WaveEnemies.Num(); }
	/** Spawn points currently showing a telegraph. */
	int32 GetPending() const { return PendingSpawns.Num(); }
	/** Not yet killed (alive + telegraphing + still to spawn). */
	int32 GetRemaining() const { return FMath::Max(0, Total - Killed - Lost); }

	/** Alive cap in effect (gs.Wave.MaxAlive overrides DA_WaveSettings). */
	int32 GetMaxAlive() const;

	/** DT_Waves row "RoundN"; nullptr if there is none (the round is generated). */
	const FGSWaveRow* FindWaveRow(int32 Round) const;

	/** Highest N with a "RoundN" row in DT_Waves (0 if none). */
	int32 GetLastTableRound() const;

	const UGSWaveSettings& GetSettings() const;

	/**
	 * Buy enemies for a generated wave: repeatedly pick (run stream) a random pool enemy that is unlocked
	 * (MinRound <= Round) and affordable, until nothing fits the budget or MaxCount is reached.
	 */
	static void PurchaseWave(const UObject* WorldContextObject, const TArray<TObjectPtr<UGSEnemyData>>& Pool, float Budget, int32 Round,
		int32 MaxCount, TArray<UGSEnemyData*>& OutEnemies, float* OutBudgetLeft = nullptr);

	/** Fires when every enemy of the active wave has spawned and died. */
	FGSOnWaveCleared OnWaveCleared;

protected:
	virtual void BeginPlay() override;

	/** DT_Waves (row struct FGSWaveRow). */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (RequiredAssetDataTags = "RowStructure=/Script/BanditBaller.GSWaveRow"))
	TObjectPtr<UDataTable> WavesTable;

	/** Budget, pool, alive cap and telegraph (DA_WaveSettings). Class defaults if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	TObjectPtr<UGSWaveSettings> WaveSettings;

private:
	void SpawnNextBatch();
	void ResolvePendingSpawns();
	void CheckCleared();
	void PushCounts() const;
	bool RemoveWaveEnemy(const AActor* Actor);
	void ClearPendingSpawns();

	UFUNCTION()
	void HandleEnemyKilled(AActor* Enemy, AActor* Killer);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	/** Enemies still to place (shuffled; taken from the back). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGSEnemyData>> SpawnQueue;

	UPROPERTY(Transient)
	TArray<FGSPendingSpawn> PendingSpawns;

	/** Spawned wave enemies that are still alive. */
	TArray<TWeakObjectPtr<AGSEnemyCharacter>> WaveEnemies;

	FTimerHandle SpawnTimer;
	FTimerHandle PendingTimer;
	bool bWaveActive = false;
	bool bLoggedCapThisWave = false;
	int32 WaveRound = 0;
	int32 Total = 0;
	int32 Spawned = 0;
	int32 Killed = 0;
	/** Removed without dying (fell out of the world, debug removal). */
	int32 Lost = 0;
	int32 BatchSize = 1;
	double WaveStartTime = 0.0;
	int32 PeakAlive = 0;

	// Fairness bookkeeping for the wave summary log.
	int32 OnScreenSpawns = 0;
	int32 FallbackSpawns = 0;
	float MinSpawnDistance = 0.f;
};
