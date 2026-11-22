// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSDynamiteComponent.generated.h"

class AGSCharacter;
class AGSEnemyCharacter;
class UGSDynamiteSettings;

/**
 * Dynamite (a weapon, M2c): on the player's character. While the player owns it (the DynamiteLevel attribute, the number
 * of Dynamite picks, is 1 or more) it lobs a stick of dynamite every ThrowInterval at the living enemy within Range that
 * has the most other living enemies within the blast radius of it (the nearest one on a tie). No clear line is needed.
 * The stick (an AGSLobbedThrow) lands where that enemy stood when it was thrown, after FlightTime, and explodes there.
 *
 * The blast: every living enemy within the blast radius of the target's position at the throw (its body center; 3D
 * distance, no line of sight, like Powder Keg) takes GE_Damage from the player's ASC with its own critical-hit roll and
 * damage number. Players are never hit.
 *
 * Follows the rules for every weapon:
 * - Damage = DA_DynamiteSettings Damage x the power bonus x the player's Damage attribute (set at the throw); crits per
 *   enemy hit (Loaded Dice). Piercing Rounds doesn't apply.
 * - Its kills are the player's (cash, XP orb, health orb chance, combo, Leech, Powder Keg, run-stats kills).
 * - Exact schedule (as GA_Fire's held fire, Prompt 31b): each throw is due one interval after the previous throw's
 *   scheduled time; a long frame throws all it owes (a late stick starts that far along its arc); nothing is banked while
 *   no enemy is in range.
 * - It doesn't throw while the player is dead or in a safe zone; a paused game doesn't tick it or the sticks; no
 *   collision; a new run (DynamiteLevel 0, UGSThrownSubsystem::ReleaseAll) removes it and any stick in the air.
 * - Weapon power (DynamitePower): DamageBonusPerPower and RadiusBonusPerPower per point.
 *
 * Settings come from AGSGameMode (DA_DynamiteSettings), so nothing is thrown in the hub.
 * TODO(co-op): server only, sticks and blasts aren't replicated (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSDynamiteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSDynamiteComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded DynamiteLevel attribute (Dynamite picks); 1 or more = owned. */
	int32 GetLevel() const;

	/** The DynamitePower attribute (weapon power; fractions count). */
	float GetPower() const;

	const UGSDynamiteSettings* GetSettings() const;

	/** At the current power. */
	float GetDamageMultiplier() const;
	float GetBlastRadius() const;
	/** Damage to each enemy in a blast: Damage x the power bonus x the player's Damage attribute (before crits). */
	float GetBlastDamage() const;

	/** Where sticks leave the player (capsule center + ThrowOffset in the player's facing frame). */
	FVector GetThrowOrigin() const;

	/**
	 * The living enemy within Range of the player with the most other living enemies within the current blast radius of
	 * it; the nearest one on a tie. nullptr if none is in range.
	 */
	AGSEnemyCharacter* FindTarget() const;

	/** Other living enemies within the current blast radius of Enemy (the number FindTarget ranks by). */
	int32 CountNeighbours(const AGSEnemyCharacter* Enemy) const;

	/** It could throw now if an enemy were in range: owned, settings, alive, not in a safe zone, server. */
	bool CanThrow() const;

	/** Blow up a stick at Center now (what a landing does): returns the number of enemies hit. */
	int32 Explode(const FVector& Center, float Radius, float Damage);

	// --- State for tests / debug ---
	int32 GetThrows() const { return Throws; }
	int32 GetBlasts() const { return Blasts; }
	int32 GetLastBlastHits() const { return LastBlastHits; }
	int32 GetHitsDealt() const { return HitsDealt; }
	FVector GetLastBlastCenter() const { return LastBlastCenter; }
	float GetLastBlastRadius() const { return LastBlastRadius; }
	FVector GetLastLanding() const { return LastLanding; }
	AGSEnemyCharacter* GetLastTarget() const { return LastTarget.Get(); }
	double GetBurstStartTime() const { return BurstStartTime; }
	int32 GetBurstShots() const { return BurstShots; }
	double GetThrowInterval() const;

private:
	AGSCharacter* GetCharacter() const;
	bool IsLivingEnemy(const AGSEnemyCharacter* Enemy) const;
	bool IsInRange(const AGSEnemyCharacter* Enemy) const;
	void UpdateThrowing(float DeltaSeconds);
	void StopThrowing(double Now);
	void ThrowAt(AGSEnemyCharacter* AtTarget, float Lateness);

	bool bTargetInRange = false;
	float TargetRefreshRemaining = 0.f;
	double NextThrowTime = 0.0;
	bool bThrowing = false;
	double BurstStartTime = 0.0;
	int32 BurstShots = 0;
	int32 Throws = 0;
	int32 Blasts = 0;
	int32 LastBlastHits = 0;
	int32 HitsDealt = 0;
	FVector LastBlastCenter = FVector::ZeroVector;
	float LastBlastRadius = 0.f;
	FVector LastLanding = FVector::ZeroVector;
	TWeakObjectPtr<AGSEnemyCharacter> LastTarget;
	int32 LoggedLevel = 0;
	float LoggedPower = 0.f;
};
