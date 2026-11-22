// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GSRunStats.generated.h"

/** Where a player's damage came from (the Damage.Source.* tag on the damage spec). */
enum class EGSDamageSource : uint8
{
	/** The player's own guns (GA_Fire: rifle, pistol). */
	Gun,
	FloatingGun,
	RazorCards,
	BarbedWire,
	PowderKeg,
	/** Anything untagged (debug damage such as gs.KillAll). */
	Other,
	Count
};

namespace GSDamageSource
{
	constexpr int32 Num = static_cast<int32>(EGSDamageSource::Count);
	/** The source named by a spec's Damage.Source.* tag (Other if none). */
	BANDITBALLER_API EGSDamageSource FromTags(const FGameplayTagContainer& Tags);
	/** "rifle/pistol", "Floating Gun", ... for logs. */
	BANDITBALLER_API const TCHAR* GetName(EGSDamageSource Source);
}

/** Kills of one enemy type (TMap doesn't replicate). */
USTRUCT(BlueprintType)
struct FGSEnemyKillCount
{
	GENERATED_BODY()

	/** Enemy type, e.g. "MeleeBandit" (UGSEnemyData name without DA_Enemy_). */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FName EnemyType;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Count = 0;
};

/** The current round so far: from its PreRound until the next round's PreRound (so RoundClear's vacuum counts). */
USTRUCT(BlueprintType)
struct FGSRoundStats
{
	GENERATED_BODY()

	/** Round number (0 before the first round starts). */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Round = 0;

	/** Enemies the party killed. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Kills = 0;

	/** Cash from pickups (the round-clear bonus is not included). */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 CashCollected = 0;

	/** XP granted to players (after XPGain). */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 XPEarned = 0;

	/** Health players lost. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float DamageTaken = 0.f;

	/** Size of the round's wave when Combat started. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 WaveEnemies = 0;

	/** Server world time when Combat started; < 0 until it does. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	double CombatStartTime = -1.0;

	/** Health orbs enemies dropped, orbs collected, and the health they healed (overheal not counted). */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 HealthOrbsDropped = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 HealthOrbsCollected = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float HealthHealedByOrbs = 0.f;

	/** Damage the players dealt this round, by source (EGSDamageSource). */
	float DamageBySource[GSDamageSource::Num] = {};

	float GetDamageBy(EGSDamageSource Source) const { return DamageBySource[static_cast<int32>(Source)]; }
};

/** The bonus paid when a round's Combat ended (RoundClear). A bonus that wasn't earned is 0. */
USTRUCT(BlueprintType)
struct FGSRoundClearPayout
{
	GENERATED_BODY()

	/** Round it was paid for (0 = none yet this run). */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Round = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 BaseBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 NoDamageBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 FastClearBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bNoDamage = false;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bFastClear = false;

	/** Seconds from Combat start to RoundClear. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float ClearTime = 0.f;

	/** The fast-clear limit for this round's wave. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float FastClearTime = 0.f;

	int32 GetTotal() const { return BaseBonus + NoDamageBonus + FastClearBonus; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSOnRunStatsChanged);

/**
 * Party-wide stats for the current run, on AGSGameState (replicated). Written only by AGSGameMode on the
 * server: rounds cleared, kills (total + per enemy type), cash earned/spent, damage dealt/taken, XP earned,
 * highest player level, best kill combo, run time.
 * Also the current round (FGSRoundStats: kills, cash collected, XP, damage taken, Combat start) and the last
 * round-clear payout (FGSRoundClearPayout), for the payout and the RoundClear summary.
 * Damage is the health actually removed (overkill not counted).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSRunStats : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSRunStats();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Server-only mutations (AGSGameMode) ---

	void ResetStats(double InRunStartTime);
	void RecordRoundCleared();
	void RecordKill(FName EnemyType);
	/** Cash from pickups: run CashEarned and this round's CashCollected. */
	void AddCashEarned(int32 Amount);
	void AddCashSpent(int32 Amount);
	/** Health the players removed from enemies, from Source (run total, by source, and this round by source). */
	void AddDamageDealt(float Amount, EGSDamageSource Source = EGSDamageSource::Other);
	/** An enemy dropped a health orb (run and round). */
	void RecordHealthOrbDropped();
	void AddDamageTaken(float Amount);
	void AddXPEarned(int32 Amount);
	/** Highest player level reached this run (keeps the max). */
	void RecordLevelReached(int32 Level);
	/** A player's kill combo reached Count kills (keeps the max). */
	void RecordCombo(int32 Count);
	/** A health orb was collected and healed Healed (0 if the player was already full, e.g. from the vacuum). */
	void RecordHealthOrbCollected(float Healed);
	/** A new round's PreRound: this round's stats start over. */
	void BeginRound(int32 Round);
	/** This round's Combat started at server time Time with a wave of WaveEnemies. */
	void RecordCombatStarted(double Time, int32 WaveEnemies);
	/** The round-clear bonus was paid: keeps it for the summary and adds its total to CashEarned. */
	void RecordRoundClearPayout(const FGSRoundClearPayout& Payout);
	/** Freezes the run timer (game over). */
	void EndRun(double InRunEndTime);

	// --- Queries ---

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetRoundsCleared() const { return RoundsCleared; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetTotalKills() const { return TotalKills; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetKillsOf(FName EnemyType) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	const TArray<FGSEnemyKillCount>& GetKillsByType() const { return KillsByType; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetCashEarned() const { return CashEarned; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetCashSpent() const { return CashSpent; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetDamageDealt() const { return DamageDealt; }

	/** Damage dealt this run from one source. */
	float GetDamageDealtBy(EGSDamageSource Source) const { return DamageDealtBySource[static_cast<int32>(Source)]; }
	const float* GetDamageDealtBySourceArray() const { return DamageDealtBySource; }

	/** "rifle/pistol 1200, Floating Gun 300, ..." (sources with damage only). */
	FString DescribeDamageBySource(const float* BySource) const;

	/** Health orbs enemies dropped this run. */
	int32 GetHealthOrbsDropped() const { return HealthOrbsDropped; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetDamageTaken() const { return DamageTaken; }

	/** XP granted to players this run (all players, after XPGain). */
	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetXPEarned() const { return XPEarned; }

	/** Highest level any player reached this run. */
	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetLevelReached() const { return LevelReached; }

	/** Longest kill combo any player reached this run (0 = no kills). */
	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetBestCombo() const { return BestCombo; }

	/** Health orbs collected this run (all players). */
	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetHealthOrbsCollected() const { return HealthOrbsCollected; }

	/** Health the orbs actually healed this run (overheal not counted). */
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealthHealedByOrbs() const { return HealthHealedByOrbs; }

	/** The current round so far (until the next round's PreRound). */
	UFUNCTION(BlueprintPure, Category = "Stats")
	const FGSRoundStats& GetCurrentRound() const { return CurrentRound; }

	/** The last round-clear payout this run (Round 0 = none yet). */
	UFUNCTION(BlueprintPure, Category = "Stats")
	const FGSRoundClearPayout& GetLastRoundClearPayout() const { return LastRoundClearPayout; }

	/** Seconds since the run started (server clock), frozen once the run ends. */
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetRunTime() const;

	/** Multi-line summary for gs.Run.Stats. */
	FString ToDebugString() const;

	/** Fires on server and clients whenever any stat changes. */
	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FGSOnRunStatsChanged OnStatsChanged;

protected:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	int32 RoundsCleared = 0;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	int32 TotalKills = 0;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	TArray<FGSEnemyKillCount> KillsByType;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	int32 CashEarned = 0;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	int32 CashSpent = 0;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	float DamageDealt = 0.f;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	float DamageTaken = 0.f;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	int32 XPEarned = 0;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	int32 LevelReached = 1;

	/** TODO(co-op): not replicated yet (single-player first); replicate it like the stats above in M6. */
	UPROPERTY(VisibleInstanceOnly, Category = "Stats")
	int32 BestCombo = 0;

	/** TODO(co-op): not replicated yet (single-player first). */
	UPROPERTY(VisibleInstanceOnly, Category = "Stats")
	int32 HealthOrbsCollected = 0;

	/** TODO(co-op): not replicated yet (single-player first). */
	UPROPERTY(VisibleInstanceOnly, Category = "Stats")
	float HealthHealedByOrbs = 0.f;

	/** TODO(co-op): not replicated yet (single-player first). */
	UPROPERTY(VisibleInstanceOnly, Category = "Stats")
	int32 HealthOrbsDropped = 0;

	/** Damage dealt by source (EGSDamageSource). TODO(co-op): not replicated yet (single-player first). */
	float DamageDealtBySource[GSDamageSource::Num] = {};

	/** TODO(co-op): not replicated yet (single-player first); clients' RoundClear summary needs it in M6. */
	UPROPERTY(VisibleInstanceOnly, Category = "Stats")
	FGSRoundStats CurrentRound;

	/** TODO(co-op): not replicated yet (single-player first); clients' RoundClear summary needs it in M6. */
	UPROPERTY(VisibleInstanceOnly, Category = "Stats")
	FGSRoundClearPayout LastRoundClearPayout;

	/** Server world time when the run started / ended (0 = still running). */
	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Stats")
	double RunStartTime = 0.0;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Stats, Category = "Stats")
	double RunEndTime = 0.0;

private:
	UFUNCTION()
	void OnRep_Stats();

	bool CanWrite() const;
	void Changed();
};
