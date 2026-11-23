// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameState.h"
#include "GSRunStats.h"
#include "GSGameMode.generated.h"

class UGSRoundSettings;
class UGSRunStats;
class UGSSpawnSettings;
class UGSWaveDirector;
class UGSDifficultyCurve;
class UGSCashSettings;
class UGSFloatingGunSettings;
class UGSRazorCardsSettings;
class UGSBarbedWireSettings;
class UGSDynamiteSettings;
class UGSLuckyCoinSettings;
class UGSMolotovSettings;
class UGSTomahawkSettings;
class UGSComboSettings;
class UGSCombatSettings;
class UGSRoundClearSettings;
class UGSXPCurve;
class UGSLevelRewards;
class UGSUpgradePool;
class AGSPlayerState;
class UGameplayEffect;
class AGSEnemyCharacter;
class UGSRunSummaryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnEnemyKilledSignature, AActor*, Enemy, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnPlayerDiedSignature, APawn*, Player, AActor*, Killer);

/**
 * Server-only game rules of the town map (L_Town_Graybox; the global default GameMode). A run starts when the map
 * loads (StartPlay: seed from ?GSSeed=N, -GSSeed=N or random; run stats; round 1). Runs the round loop as a state
 * machine over EGSRoundPhase (SetPhase / AdvancePhase). The hub is its own map with AGSHubGameMode.
 * Replicated round state lives in AGSGameState.
 * Receives deaths from UGSHealthComponent: an enemy kill counts towards the killer's combo (UGSComboComponent), the
 * enemy drops cash (x the combo's multiplier) and an XP orb (UGSCashPickupSubsystem) and OnEnemyKilled is broadcast;
 * a player's death ends the run (GameOver, run summary, then back to the hub).
 */
UCLASS()
class BANDITBALLER_API AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	/** Called by UGSHealthComponent when a non-player actor dies. */
	void HandleEnemyKilled(AActor* Enemy, AActor* Killer);

	/**
	 * Called by UGSHealthComponent when a player-controlled pawn dies: the run is over. Enters GameOver (stops the wave:
	 * nothing more spawns and no round-clear bonus is paid), freezes the run timer, saves the profile and shows that
	 * player the run summary (RunSummaryWidgetClass). TODO(co-op): end the run when the whole party is dead (M6).
	 */
	void HandlePlayerDeath(APawn* Player, AActor* Killer);

	/**
	 * A dead player chose "Return to Market" on the run summary (button, R, gamepad): fade everyone out over
	 * ReturnToHubFadeTime, then open the hub map (OpenHubMap), where the next run starts from the portal. Nothing from the
	 * run carries over; the cash is in the profile, saved at death. Falls back to ?Restart only if HubMap is unset.
	 * Refused while the requester is alive or a return is already under way. Returns true if it started.
	 */
	bool RequestReturnToHub(AController* Requester);

	/** A return to the hub is fading out (RequestReturnToHub). */
	bool IsReturningToHub() const { return bReturningToHub; }

	/** Server-only: an enemy (anything with a health component that isn't a player) died. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGSOnEnemyKilledSignature OnEnemyKilled;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGSOnPlayerDiedSignature OnPlayerDied;

	// --- Round loop (server; Prompt 59: roll-over rounds) ---
	//
	//   WaitingToStart -> PreRound (round 1's countdown) -> Combat
	//   Combat: round N runs; when it is down to RollOverRemaining enemies (and one of its own has died), round N + 1
	//           starts at once (no countdown; N's leftovers stay and still belong to N). A round clears when its own last
	//           enemy dies: its bonus is paid, the pickups fly in and the profile is saved (once that vacuum has landed).
	//   After every BreatherEveryRounds-th round nothing rolls over: once every open round is cleared -> Breather
	//   (BreatherDuration, nothing spawns) -> Combat with the next round. A safety guard (CombatFallbackDuration) removes
	//   stuck leftovers if that wait drags on.
	//   any phase -> GameOver (the player died; final): nothing more spawns, no bonus for a round that wasn't cleared.

	virtual void StartPlay() override;

	/** The only way the phase changes: logs, writes AGSGameState (phase + end time) and arms the phase timer. */
	void SetPhase(EGSRoundPhase NewPhase);

	/**
	 * Debug (gs.Round.Skip): PreRound -> Combat; in Combat, remove what is left of every open round so each clears now
	 * (bonus, vacuum), then the run goes on (the next round, or the breather); Breather -> the next round.
	 */
	void AdvancePhase();

	/** Jump to round N (>= 1): every open round stops (leftovers removed, no bonus), then N's countdown (gs.Round.Set). */
	void SetRound(int32 NewRound);

	/** Debug / tuning (gs.Round.Pause): freeze the phase timers (countdown, breather, safety guard) and hold roll-over. */
	void SetRoundTimerPaused(bool bPaused);
	bool IsRoundTimerPaused() const { return bRoundTimerPaused; }

	/** Timer length for a phase; 0 = untimed (Combat runs until its rounds are cleared). */
	float GetPhaseDuration(EGSRoundPhase Phase) const;

	/** Round timing, roll-over and breather settings (DA_RoundSettings, or the class defaults). */
	const UGSRoundSettings& GetRoundSettings() const;

	/** Tests: use Settings instead of DA_RoundSettings until cleared (nullptr), e.g. a copy with RollOverRemaining 0. */
	void SetRoundSettingsOverride(const UGSRoundSettings* Settings) { RoundSettingsOverride = Settings; }

	/** After Round the next round doesn't roll over but waits for a breather (every BreatherEveryRounds rounds). */
	bool IsBreatherRound(int32 Round) const;

	/** Seconds left on the safety guard before a breather (0 = not armed). */
	float GetSafetyTimeRemaining() const;

	/** Round-clear saves of the profile so far (tests). */
	int32 GetRoundClearSaves() const { return RoundClearSaves; }

	/** A round-clear save is waiting for its vacuum to land (tests). */
	bool IsClearSavePending() const;

	/** "open rounds 9 (2 left, 1 alive), 10 (...) | roll-over at 3 | ..." for gs.Debug.Round. */
	FString DescribeRoundLoop() const;

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
	void HandleDamageApplied(AActor* Victim, AActor* DamageInstigator, float Amount, EGSDamageSource Source = EGSDamageSource::Other);

	// --- Cash (server) ---

	/** Cash pickup tuning (DA_CashSettings). Enemies drop no cash if unset. */
	const UGSCashSettings* GetCashSettings() const { return CashSettings; }

	/**
	 * A cash pickup reached Player: UGSProfileSubsystem::AddCash (permanent balance; local player only for now), run
	 * stats CashEarned (for SourceRound, the round of the enemy that dropped it; 0 = the newest), and the "+$N" popup on
	 * that player's screen (Location = where it was collected).
	 */
	void AwardCash(APlayerController* Player, int32 Amount, const FVector& Location, int32 SourceRound = 0);

	/**
	 * An XP orb reached Player: AwardXP for that player (XPGain applies; level-ups, rewards and upgrade offers happen
	 * now), with the "+N XP" popup at Location (where it was collected). Returns the XP granted.
	 */
	int32 AwardXPPickup(APlayerController* Player, int32 BaseXP, const FVector& Location, int32 SourceRound = 0);

	/**
	 * A health orb reached Player: heals HealAmount through the instant heal effect (DA_CashSettings HealthOrbHealEffect,
	 * else UGSHealEffect), never above MaxHealth (the rest is lost; nothing for a dead player). Records run stats
	 * (orbs collected, health healed), plays the orb sound and shows the "+N" popup at Location. Returns the health
	 * actually healed. TODO(co-op): it heals whoever collects it.
	 */
	float AwardHealthPickup(APlayerController* Player, int32 HealAmount, const FVector& Location, int32 SourceRound = 0);

	/** The round an enemy's stats count for: its wave round, or the newest round (gs.Spawn enemies, dummies). */
	int32 GetEnemyStatsRound(const AActor* Enemy) const;

	/** Chance an enemy the player kills drops a health orb: the test override if set (>= 0), else DA_CashSettings. */
	float GetHealthOrbDropChance() const;

	/** Tests / debug: use Chance (0..1) instead of DA_CashSettings HealthOrbDropChance; negative clears it. */
	void SetHealthOrbDropChanceOverride(float Chance) { HealthOrbDropChanceOverride = Chance; }

	/** gs.HealthOrb.Spawn: drop Count health orbs around Player's pawn (no chance roll). Returns how many dropped. */
	int32 SpawnHealthOrbsForDebug(APlayerController* Player, int32 Count);

	// --- Kill combo (server; state on the player's UGSComboComponent) ---

	/** Combo window, cash multiplier steps, end on damage, XP option (DA_ComboSettings, or the test override). */
	const UGSComboSettings* GetComboSettings() const;

	/** DA_CombatSettings (crit multiplier); may be null (callers fall back to the class defaults). */
	const UGSCombatSettings* GetCombatSettings() const;

	/** Floating Gun tuning (DA_FloatingGunSettings); no guns if unset. */
	const UGSFloatingGunSettings* GetFloatingGunSettings() const { return FloatingGunSettings; }

	/** Razor Cards tuning (DA_RazorCardsSettings); nothing is thrown if unset. */
	const UGSRazorCardsSettings* GetRazorCardsSettings() const { return RazorCardsSettings; }

	/** Barbed Wire tuning (DA_BarbedWireSettings); no ring if unset. */
	const UGSBarbedWireSettings* GetBarbedWireSettings() const { return BarbedWireSettings; }

	/** Dynamite tuning (DA_DynamiteSettings, or the test override); nothing is thrown if unset. */
	const UGSDynamiteSettings* GetDynamiteSettings() const { return DynamiteSettingsOverride ? DynamiteSettingsOverride.Get() : DynamiteSettings.Get(); }

	/** Lucky Coin tuning (DA_LuckyCoinSettings, or the test override); nothing is flipped if unset. */
	const UGSLuckyCoinSettings* GetLuckyCoinSettings() const { return LuckyCoinSettingsOverride ? LuckyCoinSettingsOverride.Get() : LuckyCoinSettings.Get(); }

	/** Molotov tuning (DA_MolotovSettings, or the test override); nothing is thrown if unset. */
	const UGSMolotovSettings* GetMolotovSettings() const { return MolotovSettingsOverride ? MolotovSettingsOverride.Get() : MolotovSettings.Get(); }

	/** Tomahawk tuning (DA_TomahawkSettings, or the test override); nothing is thrown if unset. */
	const UGSTomahawkSettings* GetTomahawkSettings() const { return TomahawkSettingsOverride ? TomahawkSettingsOverride.Get() : TomahawkSettings.Get(); }

	/**
	 * Tests: use Settings instead of the weapon's settings asset until cleared (nullptr), e.g. a copy with a shorter
	 * interval or another LeadFactor. Lives as long as this world.
	 */
	void SetDynamiteSettingsOverride(const UGSDynamiteSettings* Settings) { DynamiteSettingsOverride = Settings; }
	void SetLuckyCoinSettingsOverride(const UGSLuckyCoinSettings* Settings) { LuckyCoinSettingsOverride = Settings; }
	void SetMolotovSettingsOverride(const UGSMolotovSettings* Settings) { MolotovSettingsOverride = Settings; }
	void SetTomahawkSettingsOverride(const UGSTomahawkSettings* Settings) { TomahawkSettingsOverride = Settings; }

	// --- Powder Keg (server) ---

	/**
	 * Blast at Location for Player's Powder Keg: damages every living enemy within BlastRadius (no line of sight, never a
	 * player) through the blast damage effect, as the player's damage (kills are theirs). BlastDamage x the player's
	 * Damage attribute; the same at every stack count. Enemies it kills roll their own chance and, if it hits, explode
	 * ChainDelay later. Returns how many enemies it damaged. Does nothing without stacks.
	 */
	int32 ExplodePowderKeg(const FVector& Location, const AGSPlayerState* Player);

	/** Chance (0..1) that an enemy Player kills explodes: PowderKeg stacks x DA_CombatSettings ChancePerStack, clamped. */
	float GetPowderKegChance(const AGSPlayerState* Player) const;

	/**
	 * One kill's Powder Keg roll for Player on the run stream: false without stacks (no roll), true at 100% (no roll).
	 * Every enemy the player kills calls it once; tests call it directly to measure the rate.
	 */
	bool RollPowderKegChance(const AGSPlayerState* Player);

	/**
	 * One blast's double-size roll for Player on the run stream: the explode chance past 100% (130% -> 30%). False below
	 * 100% and true from 200%, both without a roll. ExplodePowderKeg calls it once per blast.
	 */
	bool RollPowderKegDoubleSize(const AGSPlayerState* Player);

	/** Chain blasts scheduled and not yet fired (tests). */
	int32 GetPendingPowderKegs() const;

	/** Blasts so far this run (tests / debug). */
	int32 GetPowderKegBlasts() const { return PowderKegBlasts; }

	/** Double-size blasts so far this run (Powder Keg past 100%), and the radius of the last blast (tests / debug). */
	int32 GetDoublePowderKegBlasts() const { return DoublePowderKegBlasts; }
	float GetLastPowderKegRadius() const { return LastPowderKegRadius; }

	/** Tests / debug: use Settings instead of DA_ComboSettings until cleared (nullptr). Lives as long as this world. */
	void SetComboSettingsOverride(const UGSComboSettings* Settings) { ComboSettingsOverride = Settings; }

	/** Tests: use Settings instead of DA_CombatSettings until cleared (nullptr). */
	void SetCombatSettingsOverride(const UGSCombatSettings* Settings) { CombatSettingsOverride = Settings; }

	// --- Round-clear bonus (server) ---

	/** Base / no-damage / fast-clear bonus amounts and the fast-clear time (DA_RoundClearSettings, or the override). */
	const UGSRoundClearSettings* GetRoundClearSettings() const;

	/** Tests / debug: use Settings instead of DA_RoundClearSettings until cleared (nullptr). */
	void SetRoundClearSettingsOverride(const UGSRoundClearSettings* Settings) { RoundClearSettingsOverride = Settings; }

	// --- XP / levels (server) ---

	/** Player level curve (DA_XPCurve). Levels don't advance if unset. */
	const UGSXPCurve* GetXPCurve() const { return XPCurve; }

	/** Level-up rewards (DA_LevelRewards). Level-ups grant nothing if unset. */
	const UGSLevelRewards* GetLevelRewards() const { return LevelRewards; }

	/**
	 * Grant XP to a player: round(BaseXP x XPGain attribute) when bApplyXPGain, else round(BaseXP). Goes through
	 * UGSProgressionComponent::AddXP and feeds run stats (XP earned, level reached). Returns the XP granted.
	 */
	int32 AwardXP(AGSPlayerState* Player, float BaseXP, bool bApplyXPGain = true, int32 SourceRound = 0);

	/** Debug (gs.XP.SetLevel): reset Player to level 1, then AddXP exactly the total XP for Level. */
	void SetPlayerLevelForDebug(AGSPlayerState* Player, int32 Level);

	// --- Level-up upgrade offers (server; state on the player's UGSUpgradeComponent) ---

	/** Upgrade pool for level-up offers (DA_UpgradePool, or the test override). Level-ups offer nothing if unset. */
	const UGSUpgradePool* GetUpgradePool() const { return UpgradePoolOverride ? UpgradePoolOverride.Get() : UpgradePool.Get(); }

	/** Tests: use Pool instead of DA_UpgradePool until cleared (nullptr), e.g. a copy with a lower MaxWeapons. */
	void SetUpgradePoolOverride(const UGSUpgradePool* Pool) { UpgradePoolOverride = Pool; }

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

	// --- Map travel ---

	/**
	 * Leave the run for the hub map (HubMap) now. Single-player: OpenLevel. Used by the run summary's return (after its
	 * fade) and by gs.Market.Teleport. TODO(co-op): ServerTravel so the whole party goes (M6).
	 */
	bool OpenHubMap();

	/** URL option (?GSSeed=N) the hub's Start Run portal passes when gs.Run.Seed chose the next run's seed. */
	static const TCHAR* RunSeedOption;

protected:
	/** Shown to a player when they die: the run summary with "Return to Market" (WBP_RunSummary). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSRunSummaryWidget> RunSummaryWidgetClass;

	/** Seconds the screen fades to black between "Return to Market" and opening the hub map. */
	UPROPERTY(EditDefaultsOnly, Category = "Travel", meta = (ClampMin = "0", Units = "s"))
	float ReturnToHubFadeTime = 0.5f;

	/** Round timing (DA_RoundSettings). Falls back to UGSRoundSettings defaults if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Round")
	TObjectPtr<UGSRoundSettings> RoundSettings;

	/** Enemy Health / Damage multipliers by round (DA_DifficultyCurve). */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	TObjectPtr<UGSDifficultyCurve> DifficultyCurve;

	/** GE_EnemyRoundScaling (UGSEnemyRoundScalingEffect child); SetByCaller Data.HealthMultiplier / Data.DamageMultiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	TSubclassOf<UGameplayEffect> EnemyRoundScalingEffect;

	/** Leech: instant heal of the killer's own HealthOnKill, applied for each enemy kill (UGSHealthOnKillEffect). */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrades")
	TSubclassOf<UGameplayEffect> HealthOnKillEffect;

	/** Enemy drops: cash split, scatter, lifetime, magnet, looks; XP and health orbs (DA_CashSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Economy")
	TObjectPtr<UGSCashSettings> CashSettings;

	/** Kill combo: window, cash multiplier steps, end on damage, XP option (DA_ComboSettings). No combos if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Economy")
	TObjectPtr<UGSComboSettings> ComboSettings;

	/** Player combat tuning: crit multiplier (DA_CombatSettings). The class defaults apply if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSCombatSettings> CombatSettings;

	/** Floating Gun: damage, fire rate, range, projectile, placement, crosshair clearance, look (DA_FloatingGunSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSFloatingGunSettings> FloatingGunSettings;

	/** Razor Cards: damage, fire rate, range, the homing card, look (DA_RazorCardsSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSRazorCardsSettings> RazorCardsSettings;

	/** Barbed Wire: radius, damage, tick, power bonuses, look (DA_BarbedWireSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSBarbedWireSettings> BarbedWireSettings;

	/** Dynamite: damage, blast radius, throw interval, range, the lobbed stick, power bonuses, look (DA_DynamiteSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSDynamiteSettings> DynamiteSettings;

	/** Lucky Coin: damage, flip interval, range, bounces, the homing coin, power bonuses, look (DA_LuckyCoinSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSLuckyCoinSettings> LuckyCoinSettings;

	/** Molotov: throw interval, range, the lobbed bottle, the fire patch, power bonuses, look (DA_MolotovSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSMolotovSettings> MolotovSettings;

	/** Tomahawk: damage, throw interval, range, flight, tomahawks per throw, power bonuses, look (DA_TomahawkSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UGSTomahawkSettings> TomahawkSettings;

	/** Round-clear bonus: base, no damage, fast clear (DA_RoundClearSettings). No bonus if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Economy")
	TObjectPtr<UGSRoundClearSettings> RoundClearSettings;

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

	/** The hub map (L_Hub_Graybox) the run returns to. */
	UPROPERTY(EditDefaultsOnly, Category = "Travel")
	TSoftObjectPtr<UWorld> HubMap;

private:
	/** Round 1: round number 1, its PreRound countdown. */
	void StartNextRound();
	/** Round starts now (Combat): the round number, its stats record and its wave. */
	void StartRound(int32 Round);
	/** Start the next round if the newest one is ready to roll over (or cleared), or the breather if it is due. */
	void TryRollOver();
	void HandleRollOverReady(int32 Round);
	void HandleRoundCleared(int32 Round);
	/** PreRound / Breather timer expiry. */
	void HandlePhaseTimerExpired();
	/** The wait before a breather dragged on: the stuck leftovers are removed and the open rounds clear. */
	void HandleSafetyTimerExpired();
	/** Save the profile once the round-clear vacuum has landed (at most ClearSaveMaxWait). */
	void ScheduleClearSave();
	void CheckClearSave();

	FTimerHandle PhaseTimer;
	FTimerHandle SafetyTimer;
	FTimerHandle ClearSaveTimer;
	double ClearSaveStartTime = 0.0;
	int32 RoundClearSaves = 0;
	/** Rounds cleared whose "[round] ... cleared" line waits for their vacuum to land (written with the clear's save). */
	TArray<int32> PendingRoundLogs;
	/** Write the waiting "[round] ... cleared" lines now. */
	void FlushPendingRoundLogs();
	bool bRoundTimerPaused = false;

	/** SetRoundSettingsOverride (tests). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSRoundSettings> RoundSettingsOverride;

	/** Logged once: remote players' cash pickups are not credited yet (co-op TODO). */
	bool bWarnedRemoteCash = false;

	/** SetHealthOrbDropChanceOverride (tests); < 0 = use DA_CashSettings. */
	float HealthOrbDropChanceOverride = -1.f;

	/** An enemy the player killed at Location: roll the health orb chance (run stream; none at 0, no dice at 1). */
	void RollHealthOrbDrop(const FVector& Location);

	/** This GameMode paused the game for an upgrade choice (so it only unpauses its own pause). */
	bool bPausedForUpgradeChoice = false;

	/** True while a Powder Keg blast applies its damage: enemies it kills then explode after ChainDelay, not at once. */
	bool bApplyingPowderKeg = false;

	/** Chain blasts waiting for their ChainDelay (cleared when a run starts). */
	TArray<FTimerHandle> PendingPowderKegs;

	int32 PowderKegBlasts = 0;
	int32 DoublePowderKegBlasts = 0;
	float LastPowderKegRadius = 0.f;

	/** Player killed an enemy at Location: roll the chance; on a hit, blast now, or after ChainDelay if a blast made the kill. */
	void TriggerPowderKeg(const FVector& Location, const AGSPlayerState* Player);

	/** SetComboSettingsOverride (tests). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSComboSettings> ComboSettingsOverride;

	/** SetCombatSettingsOverride (tests). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSCombatSettings> CombatSettingsOverride;

	/** SetUpgradePoolOverride (tests). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSUpgradePool> UpgradePoolOverride;

	/** SetRoundClearSettingsOverride (tests). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSRoundClearSettings> RoundClearSettingsOverride;

	/** Set<Weapon>SettingsOverride (tests). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSDynamiteSettings> DynamiteSettingsOverride;

	UPROPERTY(Transient)
	TObjectPtr<const UGSLuckyCoinSettings> LuckyCoinSettingsOverride;

	UPROPERTY(Transient)
	TObjectPtr<const UGSMolotovSettings> MolotovSettingsOverride;

	UPROPERTY(Transient)
	TObjectPtr<const UGSTomahawkSettings> TomahawkSettingsOverride;

	/** Round was cleared: work out its bonus (its own start, clear and damage taken), pay it and record it. */
	void PayRoundClearBonus(int32 Round);

	/** The return-to-hub fade finished: open the hub (or ?Restart without a HubMap). */
	void FinishReturnToHub();

	/** "[round] Round N cleared|died: time, wave, kills, cash + bonus, XP, damage, level, fallback spawns". */
	void LogRoundSummary(int32 Round, const TCHAR* Ending) const;

	FTimerHandle ReturnToHubTimer;
	bool bReturningToHub = false;

	FRandomStream RunStream;
	int32 RunSeed = 0;

	UGSRunStats* GetRunStats() const;
};
