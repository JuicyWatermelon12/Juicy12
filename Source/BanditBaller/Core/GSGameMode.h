// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameState.h"
#include "GSGameMode.generated.h"

class UGSRoundSettings;
class UGSRunStats;
class UGSSpawnSettings;
class UGSWaveDirector;
class UGSDifficultyCurve;
class UGSCashSettings;
class UGSComboSettings;
class UGSXPCurve;
class UGSLevelRewards;
class UGSUpgradePool;
class AGSPlayerState;
class UGameplayEffect;
class AGSEnemyCharacter;
class ULevelStreaming;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnEnemyKilledSignature, AActor*, Enemy, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnPlayerDiedSignature, APawn*, Player, AActor*, Killer);

/**
 * Server-only game rules. Runs the round loop as a state machine over EGSRoundPhase (SetPhase /
 * AdvancePhase) and owns area travel between the Town and the Market District.
 * Replicated round state lives in AGSGameState.
 * Receives deaths from UGSHealthComponent: an enemy kill counts towards the killer's combo (UGSComboComponent), the
 * enemy drops cash (x the combo's multiplier) and an XP orb (UGSCashPickupSubsystem) and OnEnemyKilled is broadcast;
 * players get the death screen + restart flow.
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
	//   PreRound -> Combat -> RoundClear -> PreRound (round + 1) ...
	//
	// Timed phases advance on their own (durations from UGSRoundSettings). Combat also ends when its wave is cleared.

	virtual void StartPlay() override;

	/** The only way the phase changes: logs, writes AGSGameState (phase + end time) and arms the phase timer. */
	void SetPhase(EGSRoundPhase NewPhase);

	/** Leave the current phase for the next one in the loop (also used by gs.Round.Skip). */
	void AdvancePhase();

	/** Jump to round N (>= 1) and restart it from PreRound (gs.Round.Set). */
	void SetRound(int32 NewRound);

	/** Debug / tuning (gs.Round.Pause): freeze the phase timer so the current phase never times out. */
	void SetRoundTimerPaused(bool bPaused);
	bool IsRoundTimerPaused() const { return bRoundTimerPaused; }

	/** Timer length for a phase; 0 = untimed (waits for code / commands). */
	float GetPhaseDuration(EGSRoundPhase Phase) const;

	const UGSSpawnSettings* GetSpawnSettings() const { return SpawnSettings; }

	/** Runs each round's wave during Combat (DT_Waves). */
	UGSWaveDirector* GetWaveDirector() const { return WaveDirector; }

	// --- Enemy difficulty scaling ---

	/** Multipliers from DifficultyCurve for Round (x1 if no curve asset). */
	float GetEnemyHealthMultiplier(int32 Round) const;
	float GetEnemyDamageMultiplier(int32 Round) const;

	/**
	 * Server: apply GE_EnemyRoundScaling to a freshly initialized enemy using the current round's multipliers,
	 * then fill Health to the scaled MaxHealth. Called from AGSEnemyCharacter stat initialization.
	 */
	void ApplyEnemyRoundScaling(AGSEnemyCharacter& Enemy) const;

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

	// --- Cash (server) ---

	/** Cash pickup tuning (DA_CashSettings). Enemies drop no cash if unset. */
	const UGSCashSettings* GetCashSettings() const { return CashSettings; }

	/**
	 * A cash pickup reached Player: UGSProfileSubsystem::AddCash (permanent balance; local player only for now), run
	 * stats CashEarned, and the "+$N" popup on that player's screen (Location = where it was collected).
	 */
	void AwardCash(APlayerController* Player, int32 Amount, const FVector& Location);

	/**
	 * An XP orb reached Player: AwardXP for that player (XPGain applies; level-ups, rewards and upgrade offers happen
	 * now), with the "+N XP" popup at Location (where it was collected). Returns the XP granted.
	 */
	int32 AwardXPPickup(APlayerController* Player, int32 BaseXP, const FVector& Location);

	// --- Kill combo (server; state on the player's UGSComboComponent) ---

	/** Combo window, cash multiplier steps, end on damage, XP option (DA_ComboSettings, or the test override). */
	const UGSComboSettings* GetComboSettings() const;

	/** Tests / debug: use Settings instead of DA_ComboSettings until cleared (nullptr). Lives as long as this world. */
	void SetComboSettingsOverride(const UGSComboSettings* Settings) { ComboSettingsOverride = Settings; }

	// --- XP / levels (server) ---

	/** Player level curve (DA_XPCurve). Levels don't advance if unset. */
	const UGSXPCurve* GetXPCurve() const { return XPCurve; }

	/** Level-up rewards (DA_LevelRewards). Level-ups grant nothing if unset. */
	const UGSLevelRewards* GetLevelRewards() const { return LevelRewards; }

	/**
	 * Grant XP to a player: round(BaseXP x XPGain attribute) when bApplyXPGain, else round(BaseXP). Goes through
	 * UGSProgressionComponent::AddXP and feeds run stats (XP earned, level reached). Returns the XP granted.
	 */
	int32 AwardXP(AGSPlayerState* Player, float BaseXP, bool bApplyXPGain = true);

	/** Debug (gs.XP.SetLevel): reset Player to level 1, then AddXP exactly the total XP for Level. */
	void SetPlayerLevelForDebug(AGSPlayerState* Player, int32 Level);

	// --- Level-up upgrade offers (server; state on the player's UGSUpgradeComponent) ---

	/** Upgrade pool for level-up offers (DA_UpgradePool). Level-ups offer nothing if unset. */
	const UGSUpgradePool* GetUpgradePool() const { return UpgradePool; }

	/** Player takes choice Index of their open offer; the next queued offer opens. False if refused. */
	bool ChooseUpgrade(AGSPlayerState* Player, int32 Index);

	/** Player spends a reroll on their open offer. False with no open offer or no rerolls left. */
	bool RerollUpgradeOffer(AGSPlayerState* Player);

	/** Debug (gs.Upgrade.Offer): queue an offer for Player as a level-up would, without the level. */
	void QueueUpgradeOfferForDebug(AGSPlayerState* Player);

	/**
	 * Pause the game while any player has an open upgrade offer, unpause when none is left (called by
	 * UGSUpgradeComponent on every offer change). Only undoes a pause it made itself.
	 */
	void UpdateUpgradeChoicePause();

	bool IsPausedForUpgradeChoice() const { return bPausedForUpgradeChoice; }

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

	/** Enemy Health / Damage multipliers by round (DA_DifficultyCurve). */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	TObjectPtr<UGSDifficultyCurve> DifficultyCurve;

	/** GE_EnemyRoundScaling (UGSEnemyRoundScalingEffect child); SetByCaller Data.HealthMultiplier / Data.DamageMultiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	TSubclassOf<UGameplayEffect> EnemyRoundScalingEffect;

	/** Cash drops: split, scatter, lifetime, magnet, looks (DA_CashSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Economy")
	TObjectPtr<UGSCashSettings> CashSettings;

	/** Kill combo: window, cash multiplier steps, end on damage, XP option (DA_ComboSettings). No combos if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Economy")
	TObjectPtr<UGSComboSettings> ComboSettings;

	/** XP needed per player level + max level (DA_XPCurve). */
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TObjectPtr<UGSXPCurve> XPCurve;

	/** Gameplay Effects granted on level-up + the full heal after them (DA_LevelRewards). */
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TObjectPtr<UGSLevelRewards> LevelRewards;

	/** Upgrades, rarity weights, choices per offer and rerolls per run for level-up offers (DA_UpgradePool). */
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TObjectPtr<UGSUpgradePool> UpgradePool;

	/** Pause the game while an upgrade is being chosen (design rule; the choice screen relies on it). */
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	bool bPauseDuringUpgradeChoice = true;

	/** Spawn ring / spacing / attempts for UGSSpawnLocationFinder (DA_SpawnSettings). Class defaults if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Round")
	TObjectPtr<UGSSpawnSettings> SpawnSettings;

	/** Spawns and tracks each round's enemies; Combat ends when its wave is cleared. Set WavesTable on BP_GSGameMode. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round")
	TObjectPtr<UGSWaveDirector> WaveDirector;

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
	void HandleWaveCleared();
	/** Phase timer expiry; for Combat this is only the safety fallback (warns if the wave isn't done). */
	void HandlePhaseTimerExpired();

	FTimerHandle PhaseTimer;
	bool bRoundTimerPaused = false;

	/** Logged once: remote players' cash pickups are not credited yet (co-op TODO). */
	bool bWarnedRemoteCash = false;

	/** This GameMode paused the game for an upgrade choice (so it only unpauses its own pause). */
	bool bPausedForUpgradeChoice = false;

	/** SetComboSettingsOverride (tests). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSComboSettings> ComboSettingsOverride;

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
