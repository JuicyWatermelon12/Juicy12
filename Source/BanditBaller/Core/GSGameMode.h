// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameState.h"
#include "GSGameMode.generated.h"

class UGSRoundSettings;
class UGSRunStats;
class UGSSpawnSettings;
class ULevelStreaming;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnEnemyKilledSignature, AActor*, Enemy, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnPlayerDiedSignature, APawn*, Player, AActor*, Killer);

/**
 * Server-only game rules. Runs the round loop as a state machine over EGSRoundPhase (SetPhase /
 * AdvancePhase) and owns area travel between the Town and the Market District.
 * Replicated round state lives in AGSGameState.
 * Receives deaths from UGSHealthComponent: broadcasts OnEnemyKilled (cash/rounds listen here)
 * and shows the death screen + restart flow for players.
 */
UCLASS()
class BANDITBALLER_API AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	/** Called by UGSHealthComponent when a non-player actor dies. */
	void HandleEnemyKilled(AActor* Enemy, AActor* Killer);

	/** Called by UGSHealthComponent when a player-controlled pawn dies. */
	void HandlePlayerDeath(APawn* Player, AActor* Killer);

	/** A dead player asked to restart (death screen). Restarts the level. */
	void RequestRestart(AController* Requester);

	/** Server-only: an enemy (anything with a health component that isn't a player) died. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGSOnEnemyKilledSignature OnEnemyKilled;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGSOnPlayerDiedSignature OnPlayerDied;

	// --- Round loop (server) ---
	//
	//   PreRound -> Combat -> RoundClear -> MarketDecision -> (MarketVisit) -> PreRound (round + 1)
	//
	// Timed phases advance on their own (durations from UGSRoundSettings); MarketDecision resolves instantly.

	virtual void StartPlay() override;

	/** The only way the phase changes: logs, writes AGSGameState (phase + end time) and arms the phase timer. */
	void SetPhase(EGSRoundPhase NewPhase);

	/** Leave the current phase for the next one in the loop (also used by gs.Round.Skip). */
	void AdvancePhase();

	/** Jump to round N (>= 1) and restart it from PreRound (gs.Round.Set). */
	void SetRound(int32 NewRound);

	/** Timer length for a phase; 0 = untimed (waits for code / commands). */
	float GetPhaseDuration(EGSRoundPhase Phase) const;

	const UGSSpawnSettings* GetSpawnSettings() const { return SpawnSettings; }

	// --- Run (seed + stats) ---

	/** The run's seeded stream. Use it through UGSRunRandom, never directly with FMath::Rand. */
	FRandomStream& GetRunStream() { return RunStream; }
	int32 GetRunSeed() const { return RunSeed; }

	/** Reseed the run stream and reset run stats. Source is logged ("random", "-GSSeed", "gs.Run.Seed"). */
	void StartRun(int32 Seed, const TCHAR* Source);

	/** gs.Run.Seed <N>: start a fresh run with Seed (stream + stats reset) from round 1. */
	void RestartRunWithSeed(int32 Seed);

	/** A non-zero positive seed from the clock (used when no fixed seed is given). */
	static int32 MakeRandomSeed();

	/** Called by UGSAttributeSet when damage actually removed Health (server): feeds run stats. */
	void HandleDamageApplied(AActor* Victim, AActor* DamageInstigator, float Amount);

	// --- Area travel (server) ---

	/**
	 * Fade out, stream in the Market District sublevel, move every player to the "MarketArrival"
	 * target point, set CurrentArea = MarketDistrict, fade in. Returns false if travel can't start.
	 */
	bool TravelToMarketDistrict();

	/** Fade out, move every player to the "TownReturn" target point, unload the district, CurrentArea = Town. */
	bool ReturnToTown();

	bool IsAreaTravelInProgress() const { return TravelTimer.IsValid(); }

	/** Streaming level of the Market District in this world (nullptr if the map has none). */
	ULevelStreaming* GetMarketDistrictStreamingLevel() const;

	/** First actor in any visible level with Tag (e.g. "MarketArrival", "TownReturn"). */
	static AActor* FindTaggedActor(const UWorld* World, FName Tag);

	static const FName TownReturnTag;
	static const FName MarketArrivalTag;

protected:
	/** Shown to a player when they die (WBP_DeathScreen). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DeathScreenWidgetClass;

	/** Round timing (DA_RoundSettings). Falls back to UGSRoundSettings defaults if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Round")
	TObjectPtr<UGSRoundSettings> RoundSettings;

	/** Spawn ring / spacing / attempts for UGSSpawnLocationFinder (DA_SpawnSettings). Class defaults if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Round")
	TObjectPtr<UGSSpawnSettings> SpawnSettings;

	/** Start round 1 automatically when play begins. */
	UPROPERTY(EditDefaultsOnly, Category = "Round")
	bool bAutoStartRounds = true;

	/** Short package name of the Market District sublevel in the town map. */
	UPROPERTY(EditDefaultsOnly, Category = "Market District")
	FName MarketDistrictLevelName = TEXT("L_MarketDistrict_Graybox");

	/** Fade out / in time for area teleports. */
	UPROPERTY(EditDefaultsOnly, Category = "Market District", meta = (ClampMin = "0", Units = "s"))
	float AreaFadeTime = 0.35f;

	/** Give up (and fade back in where we are) if the district hasn't streamed in by then. */
	UPROPERTY(EditDefaultsOnly, Category = "Market District", meta = (ClampMin = "1", Units = "s"))
	float AreaStreamTimeout = 10.f;

	/** Players are placed in a ring of this radius around the arrival point. */
	UPROPERTY(EditDefaultsOnly, Category = "Market District", meta = (ClampMin = "0", Units = "cm"))
	float PartySpawnSpacing = 150.f;

private:
	const UGSRoundSettings& GetRoundSettings() const;
	void StartNextRound();
	void RunMarketDecision();

	FTimerHandle PhaseTimer;

	FRandomStream RunStream;
	int32 RunSeed = 0;

	UGSRunStats* GetRunStats() const;

	void FadeAllPlayers(bool bFadeOut) const;
	void MovePartyTo(const AActor& Destination) const;
	void TickTravelToMarket();
	void TickReturnToTown();
	void FinishTravel();

	FTimerHandle TravelTimer;
	double TravelStartTime = 0.0;
};
