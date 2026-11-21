// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSRunStats.generated.h"

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSOnRunStatsChanged);

/**
 * Party-wide stats for the current run, on AGSGameState (replicated). Written only by AGSGameMode on the
 * server: rounds cleared, kills (total + per enemy type), cash earned/spent, damage dealt/taken, run time.
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
	void AddCashEarned(int32 Amount);
	void AddCashSpent(int32 Amount);
	void AddDamageDealt(float Amount);
	void AddDamageTaken(float Amount);
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

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetDamageTaken() const { return DamageTaken; }

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
