// BanditBaller

#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;

/** The outcome of one critical-hit roll. */
struct FGSCritRoll
{
	bool bCritical = false;

	/** A triple hit: a crit chance past 100% (Loaded Dice) made this crit deal TripleCritMultiplier. */
	bool bTriple = false;

	/** Multiply the hit's damage by this (CritMultiplier on a crit, TripleCritMultiplier on a triple, otherwise 1). */
	float DamageMultiplier = 1.f;

	/** 0 = a normal hit, 1 = critical, 2 = triple: the SetByCaller Data.Critical value and the damage number's look. */
	uint8 GetLevel() const { return bTriple ? 2 : (bCritical ? 1 : 0); }
};

namespace GSCombat
{
	/**
	 * Rolls whether one hit from Source (a player's ASC) is critical: chance = Source's CritChance attribute, on the run
	 * stream (UGSRunRandom). A crit multiplies damage by CritMultiplier (DA_CombatSettings on the GameMode, 2.0 by
	 * default). Past 100% every hit crits, and CritChance - 1 is the chance that it is a triple hit instead
	 * (TripleCritMultiplier, 3.0): 130% = every hit crits, 30% of them triple. Server only. Call it once per hit that
	 * lands on a target (GA_Fire, the Floating Gun's projectiles).
	 * A chance of exactly 0 or 1 (and a triple chance of 0 or 1) needs no dice, so it doesn't touch the run stream: runs
	 * without Loaded Dice replay as before. The caller sets SetByCaller Data.Critical = GetLevel() on a crit.
	 */
	BANDITBALLER_API FGSCritRoll RollCriticalHit(const UAbilitySystemComponent* Source);

	/** The chance (0..1) that a crit is triple at CritChance (130% -> 0.3). */
	BANDITBALLER_API float GetTripleCritChance(float CritChance);

	/**
	 * Clamp a chance to 0..1 and snap values within 0.0001 of 0 or 1 to them. Chances built from summed pick values
	 * (20 x 0.05) carry float noise (1.0000001); snapping keeps "exactly 100%" a sure thing that needs no dice.
	 */
	BANDITBALLER_API float SnapChance(float Chance);
}
