// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSTomahawkComponent.generated.h"

class AGSCharacter;
class AGSEnemyCharacter;
class AGSTomahawk;
class UGSTomahawkSettings;

/**
 * Tomahawk (a weapon, M2c): on the player's character. While the player owns it (the TomahawkLevel attribute, the number
 * of Tomahawk picks, is 1 or more) it throws every ThrowInterval: one tomahawk (more with power) at the nearest living
 * enemy within Range that it has a clear line to, each extra one at the next nearest. An AGSTomahawk flies level toward
 * its target for Distance, hurting every enemy it passes, then comes back to the player and hurts them again; a wall
 * turns it back early.
 *
 * Follows the rules for every weapon:
 * - Damage = DA_TomahawkSettings Damage x the power bonus x the player's Damage attribute (set at the throw); every hit
 *   rolls its own crit (Loaded Dice). Piercing Rounds doesn't apply (it passes through everyone by itself).
 * - Its kills are the player's (cash, XP orb, health orb chance, combo, Leech, Powder Keg, run-stats kills).
 * - Exact schedule (as GA_Fire's held fire, Prompt 31b): each throw is due one interval after the previous throw's
 *   scheduled time; a long frame throws all it owes (a late tomahawk starts that far along); nothing is banked while
 *   there is no target.
 * - It doesn't throw while the player is dead or in a safe zone; a paused game doesn't tick it or the tomahawks; no
 *   collision (they sweep and never hit the player); a new run (TomahawkLevel 0, UGSThrownSubsystem::ReleaseAll) removes
 *   it and the tomahawks in the air.
 * - Weapon power (TomahawkPower): DamageBonusPerPower per point, and one more tomahawk per PowerPerTomahawk full points.
 *
 * Settings come from AGSGameMode (DA_TomahawkSettings), so nothing is thrown in the hub.
 * TODO(co-op): server only, tomahawks aren't replicated (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSTomahawkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSTomahawkComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded TomahawkLevel attribute (Tomahawk picks); 1 or more = owned. */
	int32 GetLevel() const;

	/** The TomahawkPower attribute (weapon power; fractions count). */
	float GetPower() const;

	const UGSTomahawkSettings* GetSettings() const;

	/** At the current power. */
	float GetDamageMultiplier() const;
	int32 GetTomahawksPerThrow() const;
	/** Damage per hit: Damage x the power bonus x the player's Damage attribute (before crits). */
	float GetHitDamage() const;
	double GetThrowInterval() const;

	/** Where tomahawks leave the player (capsule center + ThrowOffset in the player's facing frame). */
	FVector GetThrowOrigin() const;

	/** Living enemies within Range of the throw origin with a clear line, nearest first. */
	TArray<AGSEnemyCharacter*> FindTargets() const;

	/** It could throw now if it had a target: owned, settings, alive, not in a safe zone, server. */
	bool CanThrow() const;

	/** Called by a tomahawk for every enemy it hits. */
	void NotifyTomahawkHit() { ++HitsDealt; }

	/** This weapon's tomahawks in the air now (from the thrown-actor pool). */
	TArray<AGSTomahawk*> GetActiveTomahawks() const;

	// --- State for tests / debug ---
	int32 GetThrows() const { return Throws; }
	int32 GetTomahawksThrown() const { return TomahawksThrown; }
	int32 GetHitsDealt() const { return HitsDealt; }
	/** The targets of the last throw, one per tomahawk, in throw order. */
	const TArray<TWeakObjectPtr<AGSEnemyCharacter>>& GetLastTargets() const { return LastTargets; }
	double GetBurstStartTime() const { return BurstStartTime; }
	int32 GetBurstShots() const { return BurstShots; }

private:
	AGSCharacter* GetCharacter() const;
	bool HasClearLine(const AActor* Candidate) const;
	void UpdateThrowing(float DeltaSeconds);
	void StopThrowing(double Now);
	void ThrowVolley(const TArray<AGSEnemyCharacter*>& Targets, float Lateness);

	bool bTargetInRange = false;
	float TargetRefreshRemaining = 0.f;
	double NextThrowTime = 0.0;
	bool bThrowing = false;
	double BurstStartTime = 0.0;
	int32 BurstShots = 0;
	int32 Throws = 0;
	int32 TomahawksThrown = 0;
	int32 HitsDealt = 0;
	TArray<TWeakObjectPtr<AGSEnemyCharacter>> LastTargets;
	int32 LoggedLevel = 0;
	float LoggedPower = 0.f;
};
