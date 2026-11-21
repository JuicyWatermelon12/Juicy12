// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GSGameState.generated.h"

class UGSRunStats;
class UGSLevelRewards;

/**
 * Phase of the round loop run by AGSGameMode:
 * PreRound -> Combat -> RoundClear -> PreRound (next round) ... until the run ends (GameOver).
 * There are no market visits during a run; the Market District is the hub between runs.
 */
UENUM(BlueprintType)
enum class EGSRoundPhase : uint8
{
	/** Before the first round starts. */
	WaitingToStart,
	/** Short countdown before combat. */
	PreRound,
	/** Enemies fight the party in the town. */
	Combat,
	/** Combat is over: results / breather, then the next round. */
	RoundClear,
	/** The run is over (not used yet; player death still reloads the map). */
	GameOver
};

/** Where the party currently is. Combat and enemy spawns happen only in the Town. */
UENUM(BlueprintType)
enum class EGSArea : uint8
{
	Town,
	/** Safe, separate area reached by teleport (streamed sublevel L_MarketDistrict_Graybox). */
	MarketDistrict
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnAreaChangedSignature, EGSArea, OldArea, EGSArea, NewArea);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnPhaseChangedSignature, EGSRoundPhase, OldPhase, EGSRoundPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSOnRoundChangedSignature, int32, NewRound);

/**
 * Shared, replicated round state: round number, phase, phase end time and current area.
 * Written only by AGSGameMode on the server; the delegates fire on the server and on clients (OnRep).
 */
UCLASS()
class BANDITBALLER_API AGSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGSGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Round ---

	UFUNCTION(BlueprintPure, Category = "Round")
	int32 GetRoundNumber() const { return RoundNumber; }

	UFUNCTION(BlueprintPure, Category = "Round")
	EGSRoundPhase GetRoundPhase() const { return RoundPhase; }

	/** Server world time when the current phase ends; <= 0 means the phase has no timer. */
	UFUNCTION(BlueprintPure, Category = "Round")
	double GetPhaseEndTime() const { return PhaseEndTime; }

	/** Seconds until the phase ends (0 if untimed), using the replicated server clock. */
	UFUNCTION(BlueprintPure, Category = "Round")
	float GetPhaseTimeRemaining() const;

	/** Server only (AGSGameMode). */
	void SetRoundPhase(EGSRoundPhase NewPhase, double NewPhaseEndTime);
	void SetRoundNumber(int32 NewRound);

	UPROPERTY(BlueprintAssignable, Category = "Round")
	FGSOnPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Round")
	FGSOnRoundChangedSignature OnRoundChanged;

	// --- Area ---

	UFUNCTION(BlueprintPure, Category = "Area")
	EGSArea GetCurrentArea() const { return CurrentArea; }

	/** Server only (AGSGameMode). */
	void SetCurrentArea(EGSArea NewArea);

	/** Fires on server and clients when CurrentArea changes. */
	UPROPERTY(BlueprintAssignable, Category = "Area")
	FGSOnAreaChangedSignature OnAreaChanged;

	/** "Round 2 | Combat | 41.3 s" for debug displays and logs. */
	FString GetRoundDebugString() const;

	// --- Wave (written by UGSWaveDirector on the server) ---

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetWaveEnemiesTotal() const { return WaveEnemiesTotal; }

	/** Enemies of this round not yet killed (alive + still to spawn). */
	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetWaveEnemiesRemaining() const { return WaveEnemiesRemaining; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetWaveEnemiesAlive() const { return WaveEnemiesAlive; }

	/** Server only. */
	void SetWaveCounts(int32 InTotal, int32 InRemaining, int32 InAlive);

	// --- Enemy difficulty (from UGSDifficultyCurve for the current round) ---

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyHealthMultiplier() const { return EnemyHealthMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyDamageMultiplier() const { return EnemyDamageMultiplier; }

	/** Server only (AGSGameMode, on every phase change). */
	void SetEnemyRoundMultipliers(float InHealth, float InDamage);

	/** Party-wide stats for the current run (replicated). */
	UFUNCTION(BlueprintPure, Category = "Run")
	UGSRunStats* GetRunStats() const { return RunStats; }

	/** Seed of the current run (replicated so clients can show / report it). */
	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetRunSeed() const { return RunSeed; }

	/** Server only (AGSGameMode). */
	void SetRunSeed(int32 InSeed) { RunSeed = InSeed; }

	/** The GameMode's level-up rewards (DA_LevelRewards), replicated so clients can describe them (level-up banner). */
	const UGSLevelRewards* GetLevelRewards() const { return LevelRewards; }

	/** Server only (AGSGameMode::StartPlay). */
	void SetLevelRewards(UGSLevelRewards* InRewards) { LevelRewards = InRewards; }

protected:
	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Progression")
	TObjectPtr<UGSLevelRewards> LevelRewards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run")
	TObjectPtr<UGSRunStats> RunStats;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Run")
	int32 RunSeed = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Wave")
	int32 WaveEnemiesTotal = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Wave")
	int32 WaveEnemiesRemaining = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Wave")
	int32 WaveEnemiesAlive = 0;

	/** Multipliers applied to enemies spawning this round (GE_EnemyRoundScaling). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Difficulty")
	float EnemyHealthMultiplier = 1.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Difficulty")
	float EnemyDamageMultiplier = 1.f;

	/** Current round, 0 before the first round starts. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_RoundNumber, Category = "Round")
	int32 RoundNumber = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_RoundPhase, Category = "Round")
	EGSRoundPhase RoundPhase = EGSRoundPhase::WaitingToStart;

	/** Server world time (GetServerWorldTimeSeconds) when the phase ends; <= 0 if untimed. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Round")
	double PhaseEndTime = 0.0;

	/** Where the party is. Enemies only target players while this is Town. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentArea, Category = "Area")
	EGSArea CurrentArea = EGSArea::Town;

private:
	UFUNCTION()
	void OnRep_RoundNumber();

	UFUNCTION()
	void OnRep_RoundPhase(EGSRoundPhase OldPhase);

	UFUNCTION()
	void OnRep_CurrentArea(EGSArea OldArea);
};
