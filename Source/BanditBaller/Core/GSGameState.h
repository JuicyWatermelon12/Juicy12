// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GSGameState.generated.h"

class UGSRunStats;
class UGSLevelRewards;

/**
 * Phase of the round loop run by AGSGameMode (Prompt 59: rounds roll over into each other inside Combat):
 * PreRound (round 1 only) -> Combat (rounds 1, 2, ... roll over) -> Breather (after every 10th round) -> Combat ...
 * until the run ends (GameOver).
 */
UENUM(BlueprintType)
enum class EGSRoundPhase : uint8
{
	/** Before the first round starts. */
	WaitingToStart,
	/** The countdown before round 1 (the only countdown). */
	PreRound,
	/** Rounds run, the next starting while the newest has a few enemies left. */
	Combat,
	/** After every BreatherEveryRounds rounds, once all are cleared: nothing spawns, then the next round starts. */
	Breather,
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSOnWaveCountsChangedSignature, int32, Total, int32, Remaining, int32, Alive);

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

	/** Size of the newest round's wave (rounds overlap since Prompt 59). */
	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetWaveEnemiesTotal() const { return WaveEnemiesTotal; }

	/** Enemies of the newest round not yet killed (alive + still to spawn). */
	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetWaveEnemiesRemaining() const { return WaveEnemiesRemaining; }

	/** Wave enemies alive, every open round together. */
	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetWaveEnemiesAlive() const { return WaveEnemiesAlive; }

	/** Server only. Broadcasts OnWaveCountsChanged when any count changed. */
	void SetWaveCounts(int32 InTotal, int32 InRemaining, int32 InAlive);

	/**
	 * The wave counts changed (server). TODO(co-op): the counts replicate without a RepNotify, so clients don't get
	 * this event yet (single-player first).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Wave")
	FGSOnWaveCountsChangedSignature OnWaveCountsChanged;

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
