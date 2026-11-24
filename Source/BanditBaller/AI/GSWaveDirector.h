// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "GSWaveDirector.generated.h"

class AGSEnemyCharacter;
class AGSSpawnTelegraph;
class UGSEnemyData;
class UGSRoundSettings;
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

/**
 * DT_Waves row, one per round ("Round1", "Round2", ...). Rounds past the last row are generated from a budget.
 * (The spawn interval and batch size were removed in Prompt 59: a round's enemies now arrive one at a time over
 * DA_RoundSettings RoundSpawnWindow.)
 */
USTRUCT(BlueprintType)
struct FGSWaveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FGSWaveEntry> Enemies;
};

/** A spawn point that is showing its telegraph; the enemy appears at SpawnAt and belongs to Round. */
USTRUCT()
struct FGSPendingSpawn
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UGSEnemyData> EnemyData;

	FVector Location = FVector::ZeroVector;
	FVector FaceTowards = FVector::ZeroVector;
	double SpawnAt = 0.0;
	int32 Round = 0;
	TWeakObjectPtr<AGSSpawnTelegraph> Telegraph;
};

/** One open round's wave (several can be open at once: rounds roll over, Prompt 59). */
USTRUCT()
struct FGSRoundWave
{
	GENERATED_BODY()

	int32 Round = 0;

	/** Enemies still to place, shuffled (taken from the back). */
	UPROPERTY()
	TArray<TObjectPtr<UGSEnemyData>> Queue;

	int32 Total = 0;
	int32 Spawned = 0;
	int32 Killed = 0;
	/** Removed without dying (fell out of the world, debug removal, the safety guard). */
	int32 Lost = 0;
	/** Its spawns showing a telegraph. */
	int32 Pending = 0;

	/** Its spawned enemies still alive. */
	TArray<TWeakObjectPtr<AGSEnemyCharacter>> Enemies;

	/** Server world time the round started, its spawn gap and when its next enemy is due. */
	double StartTime = 0.0;
	float Gap = 1.f;
	double NextSpawnAt = 0.0;
	/** World time each of its spawns started its telegraph (tests: the spacing). */
	TArray<double> SpawnTimes;

	/** OnRollOverReady was sent for it. */
	bool bRollOverSent = false;

	// Spawn-point bookkeeping for the round's log line and tests.
	int32 OnScreenSpawns = 0;
	int32 FallbackSpawns = 0;
	int32 PeakAlive = 0;
	float MinSpawnDistance = TNumericLimits<float>::Max();
	float MaxSpawnDistance = 0.f;

	/** Not yet killed (alive + telegraphing + still to spawn). */
	int32 GetRemaining() const { return FMath::Max(0, Total - Killed - Lost); }
	int32 GetAlive() const { return Enemies.Num(); }
	/** Every enemy has spawned and none is alive: the round is over. */
	bool IsFinished() const { return Queue.IsEmpty() && Pending == 0 && Enemies.IsEmpty(); }
	float GetClosestSpawnDistance() const { return MinSpawnDistance < TNumericLimits<float>::Max() ? MinSpawnDistance : 0.f; }
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGSOnRoundWaveEvent, int32 /*Round*/);

/**
 * Server-only component on AGSGameMode that runs the waves of the open rounds (Prompt 59: rounds overlap).
 *
 * Wave contents: the round's DT_Waves row ("RoundN"); for rounds past the last row, a wave bought from a budget
 * (UGSWaveSettings: Base + PerRound x round, or a curve) out of the enemy pool using each enemy's SpawnCost and
 * MinRound, with at most MaxRangedShare ranged. The purchase and the order use a stream made from the run seed and the
 * round (UGSRunRandom::MakeRoundStream), so the same seed gives the same waves however fast the player kills.
 *
 * Spawning: each open round places its enemies one at a time, the next due one gap after the last (DA_RoundSettings:
 * RoundSpawnWindow / wave size, kept between MinSpawnGap and MaxSpawnGap), older rounds first, at
 * UGSSpawnLocationFinder points around a random living player. MaxAliveEnemies (alive + telegraphing, every round
 * together) holds spawns back; nothing is banked meanwhile: when room appears one enemy comes and the next is due one gap
 * later. Each point shows an AGSSpawnTelegraph for SpawnTelegraphTime first. Every enemy carries its round.
 *
 * Events: OnRollOverReady(Round) once, when a round is down to RollOverRemaining enemies (alive + to spawn) and at least
 * one of its own enemies has died; OnRoundCleared(Round) when its last enemy is gone. Both are broadcast after the
 * director's state is consistent, so a handler may start the next round. Mirrors the newest round's counts and every
 * round's alive enemies onto AGSGameState.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSWaveDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSWaveDirector();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Start Round's wave (its first enemy is due now). An open wave of the same round is replaced. */
	void StartRoundWave(int32 Round);

	/**
	 * Stop every open round without clearing it (no events): cancel the telegraphs and what is left to spawn.
	 * bDespawnLeftovers removes their living enemies (gs.Round.Set); otherwise they stay (death: GameOver).
	 */
	void StopAll(bool bDespawnLeftovers);

	/**
	 * Remove what is left of every open round (unspawned, telegraphing and living enemies, counted as lost) so each one
	 * clears now (OnRoundCleared). gs.Round.Skip and the safety guard before a breather. Returns the rounds cleared.
	 */
	int32 ForceClearOpenRounds(const TCHAR* Why);

	/** Any round open. */
	bool IsWaveActive() const { return !ActiveRounds.IsEmpty(); }
	const TArray<FGSRoundWave>& GetActiveRounds() const { return ActiveRounds; }
	const FGSRoundWave* FindRoundWave(int32 Round) const;
	/** Highest open round (0 if none). */
	int32 GetNewestRound() const;

	/** Round is open, down to RollOverRemaining (alive + to spawn) and at least one of its enemies died. */
	bool IsRollOverReady(int32 Round) const;

	// --- The newest open round (or, with none open, the last one that finished) ---
	int32 GetTotal() const;
	int32 GetSpawned() const;
	int32 GetKilled() const;
	/** Not yet killed (alive + telegraphing + still to spawn). */
	int32 GetRemaining() const;
	int32 GetFallbackSpawns() const;
	int32 GetOnScreenSpawns() const;
	/** Closest / farthest chosen spawn point from its player (cm); 0 before the first spawn of the round. */
	float GetClosestSpawnDistance() const;
	float GetFarthestSpawnDistance() const;

	// --- Every open round together (what the alive cap counts) ---
	int32 GetAlive() const;
	/** Spawn points currently showing a telegraph. */
	int32 GetPending() const { return PendingSpawns.Num(); }

	/** Alive cap in effect (gs.Wave.MaxAlive overrides DA_WaveSettings), for every round together. */
	int32 GetMaxAlive() const;

	// --- Tests (seed replays) ---

	/** The newest round's enemies not yet telegraphed (shuffled; taken from the back). */
	const TArray<TObjectPtr<UGSEnemyData>>& GetSpawnQueue() const;

	/** Spawns currently telegraphing (every round): enemy type, location and round. */
	const TArray<FGSPendingSpawn>& GetPendingSpawns() const { return PendingSpawns; }

	/** Telegraph the newest round's next enemy now instead of when it is due (same rules: alive cap, finder, run stream). */
	void SpawnNextBatchNow();

	/** DT_Waves row "RoundN"; nullptr if there is none (the round is generated). */
	const FGSWaveRow* FindWaveRow(int32 Round) const;

	/** Highest N with a "RoundN" row in DT_Waves (0 if none). */
	int32 GetLastTableRound() const;

	const UGSWaveSettings& GetSettings() const;

	/**
	 * What Round's wave is made of, in spawn order reversed (the queue is taken from the back), from the round stream:
	 * the DT_Waves row or the bought wave, shuffled. Same seed and round, same wave. OutSource describes it for logs.
	 */
	void BuildRoundWave(int32 Round, TArray<TObjectPtr<UGSEnemyData>>& OutQueue, FString* OutSource = nullptr) const;

	/**
	 * Buy enemies for a generated wave: repeatedly pick (run stream) a random pool enemy that is unlocked
	 * (MinRound <= Round) and affordable, until nothing fits the budget or MaxCount is reached.
	 * Ranged cap: a ranged enemy (bRanged) can only be picked while (ranged + 1) <= MaxRangedShare x (count + 1), so the
	 * wave never ends above that share; otherwise the pick is among the other affordable types. 1 = no cap.
	 */
	static void PurchaseWave(const UObject* WorldContextObject, const TArray<TObjectPtr<UGSEnemyData>>& Pool, float Budget, int32 Round,
		int32 MaxCount, TArray<UGSEnemyData*>& OutEnemies, float* OutBudgetLeft = nullptr, float MaxRangedShare = 1.f);

	/** The same purchase drawing from Stream (a round's own stream). */
	static void PurchaseWaveFromStream(FRandomStream& Stream, const TArray<TObjectPtr<UGSEnemyData>>& Pool, float Budget, int32 Round,
		int32 MaxCount, TArray<UGSEnemyData*>& OutEnemies, float* OutBudgetLeft = nullptr, float MaxRangedShare = 1.f);

	/** Share of Enemies that is ranged (0..1; 0 for an empty list). */
	static float GetRangedShare(const TArray<UGSEnemyData*>& Enemies);

	/**
	 * Server: Enemy just became a ragdoll corpse (any enemy, wave or not). Keeps at most DA_WaveSettings MaxCorpses: the
	 * oldest corpses are destroyed first.
	 */
	void RegisterCorpse(AGSEnemyCharacter* Enemy);

	/** Ragdoll corpses in the world now (tests), and how many the limit removed early this session. */
	int32 GetCorpseCount() const;
	int32 GetCorpsesRemovedByLimit() const { return CorpsesRemovedByLimit; }

	/** A round is down to its roll-over point (see IsRollOverReady). */
	FGSOnRoundWaveEvent OnRollOverReady;

	/** A round's last enemy is gone (killed, or removed by ForceClearOpenRounds). */
	FGSOnRoundWaveEvent OnRoundCleared;

protected:
	virtual void BeginPlay() override;

	/** DT_Waves (row struct FGSWaveRow). */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (RequiredAssetDataTags = "RowStructure=/Script/BanditBaller.GSWaveRow"))
	TObjectPtr<UDataTable> WavesTable;

	/** Budget, pool, ranged cap, alive cap and telegraph (DA_WaveSettings). Class defaults if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	TObjectPtr<UGSWaveSettings> WaveSettings;

private:
	const UGSRoundSettings& GetRoundSettings() const;
	FGSRoundWave* FindRoundWaveMutable(int32 Round);
	/** The newest open round, else the last finished one (for the "current wave" getters). */
	const FGSRoundWave* GetReportedRound() const;

	/** Place every enemy that is due (older rounds first), within the alive cap. */
	void SpawnDueEnemies();
	/** Telegraph Wave's next enemy at a ring point. DueTime is when it was due (for the even spacing). */
	bool SpawnOne(FGSRoundWave& Wave, double DueTime);
	void ResolvePendingSpawns();
	/** Check Wave for roll-over and clear (events queued, sent by FlushEvents). */
	void CheckRound(FGSRoundWave& Wave);
	/** Remove the finished rounds, push the counts and send the queued events. */
	void FlushEvents();
	void PushCounts() const;
	void UpdateTickEnabled();
	void LogRoundFinished(const FGSRoundWave& Wave) const;

	UFUNCTION()
	void HandleEnemyKilled(AActor* Enemy, AActor* Killer);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	UPROPERTY(Transient)
	TArray<FGSRoundWave> ActiveRounds;

	UPROPERTY(Transient)
	TArray<FGSPendingSpawn> PendingSpawns;

	/** The last round that finished (the "current wave" getters with no round open). */
	UPROPERTY(Transient)
	FGSRoundWave LastFinished;

	/** Corpses oldest first. */
	TArray<TWeakObjectPtr<AGSEnemyCharacter>> Corpses;
	int32 CorpsesRemovedByLimit = 0;

	TArray<int32> QueuedRollOvers;
	TArray<int32> QueuedClears;
	bool bFlushing = false;
	bool bLoggedCap = false;
};
