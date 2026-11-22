// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSProjectile.h"
#include "GSLuckyCoinComponent.generated.h"

class AGSCharacter;
class AGSEnemyCharacter;
class UGSLuckyCoinSettings;

/**
 * Lucky Coin (a weapon, M2c): on the player's character. While the player owns it (the LuckyCoinLevel attribute, the
 * number of Lucky Coin picks, is 1 or more) it flips a coin every FlipInterval at the nearest living enemy within Range
 * that it has a clear line to. After a hit the coin bounces to the nearest other living enemy within BounceRange of the
 * enemy it hit (with a clear line), while bounces are left; one flip never hits the same enemy twice. Each leg is a
 * homing AGSProjectile that passes through the enemies this flip already hit; the next leg starts where the last one hit.
 *
 * Follows the rules for every weapon:
 * - Damage = DA_LuckyCoinSettings Damage x the power bonus x the player's Damage attribute (set at the flip); every hit
 *   rolls its own crit (Loaded Dice). Piercing Rounds doesn't apply.
 * - Its kills are the player's (cash, XP orb, health orb chance, combo, Leech, Powder Keg, run-stats kills).
 * - Exact schedule (as GA_Fire's held fire, Prompt 31b): each flip is due one interval after the previous flip's
 *   scheduled time; a long frame flips all it owes (a late coin starts that far along); nothing is banked while there is
 *   no target.
 * - It doesn't flip while the player is dead or in a safe zone; a paused game doesn't tick it or the coins; no collision
 *   (pooled projectiles that never hit the player); a new run (LuckyCoinLevel 0, the projectile pool released) removes it
 *   and the coins in the air.
 * - Weapon power (LuckyCoinPower): DamageBonusPerPower per point, and one bounce per PowerPerBounce full points.
 *
 * Settings come from AGSGameMode (DA_LuckyCoinSettings), so nothing is flipped in the hub.
 * TODO(co-op): server only, coins aren't replicated (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSLuckyCoinComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSLuckyCoinComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded LuckyCoinLevel attribute (Lucky Coin picks); 1 or more = owned. */
	int32 GetLevel() const;

	/** The LuckyCoinPower attribute (weapon power; fractions count). */
	float GetPower() const;

	const UGSLuckyCoinSettings* GetSettings() const;

	/** At the current power. */
	float GetDamageMultiplier() const;
	int32 GetBounces() const;
	/** Damage per hit: Damage x the power bonus x the player's Damage attribute (before crits). */
	float GetHitDamage() const;
	double GetFlipInterval() const;

	/** Where coins leave the player (capsule center + ThrowOffset in the player's facing frame). */
	FVector GetThrowOrigin() const;

	/** The nearest living enemy within Range of the throw origin with a clear line, or nullptr. */
	AGSEnemyCharacter* FindTarget() const;

	/** The nearest living enemy within BounceRange of From with a clear line from From, not in Exclude; or nullptr. */
	AGSEnemyCharacter* FindBounceTarget(const FVector& From, const TArray<TWeakObjectPtr<AActor>>& Exclude) const;

	/** It could flip now if it had a target: owned, settings, alive, not in a safe zone, server. */
	bool CanFlip() const;

	// --- State for tests / debug ---
	AGSEnemyCharacter* GetTarget() const { return Target.Get(); }
	int32 GetFlips() const { return Flips; }
	/** Flips whose coin has finished (no bounce left, nobody in reach, or a miss). */
	int32 GetFlipsDone() const { return FlipsDone; }
	int32 GetFlipsInAir() const { return Flips - FlipsDone; }
	int32 GetHitsDealt() const { return HitsDealt; }
	/** The enemies the last finished flip hit, in order. */
	const TArray<TWeakObjectPtr<AActor>>& GetLastFlipHits() const { return LastFlipHits; }
	float GetLastFlipDamage() const { return LastFlipDamage; }
	double GetBurstStartTime() const { return BurstStartTime; }
	int32 GetBurstShots() const { return BurstShots; }

private:
	/** One flip in the air: who it hit so far, how many bounces are left, its damage. */
	struct FFlip
	{
		TArray<TWeakObjectPtr<AActor>> Hit;
		int32 BouncesLeft = 0;
		float Damage = 0.f;
	};

	AGSCharacter* GetCharacter() const;
	bool HasClearLine(const FVector& From, const AActor* Candidate) const;
	bool IsValidTarget(const AGSEnemyCharacter* Candidate) const;
	void UpdateFlipping(float DeltaSeconds);
	void StopFlipping(double Now);
	void FlipAt(AGSEnemyCharacter* AtTarget, float Lateness);
	/** Launch one leg of a flip from Start at Target (the first throw or a bounce). */
	void LaunchCoin(const TSharedRef<FFlip>& Flip, const FVector& Start, AGSEnemyCharacter* AtTarget, float Lateness);
	/** A leg ended: bounce on or end the flip. */
	void HandleCoinFinished(const TSharedRef<FFlip>& Flip, EGSProjectileEnd Reason, AActor* HitActor, const FVector& Where);
	void EndFlip(const TSharedRef<FFlip>& Flip);

	TWeakObjectPtr<AGSEnemyCharacter> Target;
	float TargetRefreshRemaining = 0.f;
	double NextFlipTime = 0.0;
	bool bFlipping = false;
	double BurstStartTime = 0.0;
	int32 BurstShots = 0;
	int32 Flips = 0;
	int32 FlipsDone = 0;
	int32 HitsDealt = 0;
	TArray<TWeakObjectPtr<AActor>> LastFlipHits;
	float LastFlipDamage = 0.f;
	bool bPrewarmed = false;
	int32 LoggedLevel = 0;
	float LoggedPower = 0.f;
};
