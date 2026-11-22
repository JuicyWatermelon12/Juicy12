// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSRazorCardsComponent.generated.h"

class AGSCharacter;
class AGSEnemyCharacter;
class UGSRazorCardsSettings;

/**
 * Razor Cards (a weapon, M2c): on the player's character. While the player owns it (the RazorCardsLevel attribute, the
 * number of Razor Cards picks, is 1 or more) it throws playing cards from the player, one at a time, at the nearest
 * living enemy within Range that it has a clear line to (world geometry blocks it). Each card is a homing AGSProjectile
 * aimed at that enemy; if the enemy dies first the card flies on straight until it hits something or expires.
 *
 * Follows the rules for every weapon:
 * - Damage = DA_RazorCardsSettings Damage x the power bonus x the player's Damage attribute; each card rolls its own
 *   critical hit (Loaded Dice). Piercing Rounds doesn't apply (a card stops at the first target).
 * - Its kills are the player's (GE_Damage from the player's ASC): cash, XP orb, health orb chance, combo, Leech,
 *   Powder Keg, run-stats kills.
 * - Exact schedule like GA_Fire's held burst (Prompt 31b): each card is due one interval after the previous card's
 *   scheduled time; a long frame throws every card it owes (a late card starts that much further along); nothing is
 *   banked while there is no target. A fire rate change re-times from the last card.
 * - It doesn't throw while the player is dead or in a safe zone; a paused game doesn't tick it; no collision (the cards
 *   are pooled projectiles that never hit the player); a new run (RazorCardsLevel 0) stops it.
 * - Weapon power (RazorCardsPower): DamageBonusPerPower / FireRateBonusPerPower per point.
 *
 * Settings come from AGSGameMode (DA_RazorCardsSettings), so nothing is thrown in the hub.
 * TODO(co-op): server only, cards aren't replicated (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSRazorCardsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSRazorCardsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded RazorCardsLevel attribute (Razor Cards picks); 1 or more = owned. */
	int32 GetLevel() const;

	/** The RazorCardsPower attribute (weapon power; fractions count). */
	float GetPower() const;

	const UGSRazorCardsSettings* GetSettings() const;

	/** Seconds between cards and the damage multiplier at the current power. */
	double GetThrowInterval() const;
	float GetDamageMultiplier() const;

	/** Where cards leave the player (capsule center + ThrowOffset in the player's facing frame). */
	FVector GetThrowOrigin() const;

	/** The nearest living enemy within Range of the throw origin with a clear line, or nullptr. */
	AGSEnemyCharacter* FindTarget() const;

	/** It could throw now if it had a target: owned, settings, alive, not in a safe zone, server. */
	bool CanThrow() const;

	// --- State for tests / debug ---
	AGSEnemyCharacter* GetTarget() const { return Target.Get(); }
	int32 GetCardsThrown() const { return CardsThrown; }
	double GetBurstStartTime() const { return BurstStartTime; }
	int32 GetBurstShots() const { return BurstShots; }
	double GetBurstInterval() const { return BurstInterval; }

private:
	AGSCharacter* GetCharacter() const;
	bool HasClearLine(const AActor* Candidate) const;
	bool IsValidTarget(const AGSEnemyCharacter* Candidate) const;
	void UpdateThrowing(float DeltaSeconds);
	void StopThrowing(double Now);
	void ThrowAt(AGSEnemyCharacter* AtTarget, float Lateness);

	TWeakObjectPtr<AGSEnemyCharacter> Target;
	float TargetRefreshRemaining = 0.f;
	double NextThrowTime = 0.0;
	bool bThrowing = false;
	double BurstStartTime = 0.0;
	double BurstInterval = 0.0;
	double LastThrowTime = 0.0;
	int32 BurstShots = 0;
	int32 CardsThrown = 0;
	bool bPrewarmed = false;
	int32 LoggedLevel = 0;
	float LoggedPower = 0.f;
};
