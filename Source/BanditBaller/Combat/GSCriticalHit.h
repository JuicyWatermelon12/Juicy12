// BanditBaller

#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;

/** The outcome of one critical-hit roll. */
struct FGSCritRoll
{
	bool bCritical = false;

	/** Multiply the hit's damage by this (CritMultiplier on a crit, otherwise 1). */
	float DamageMultiplier = 1.f;
};

namespace GSCombat
{
	/**
	 * Rolls whether one hit from Source (a player's ASC) is critical: chance = Source's CritChance attribute, on the run
	 * stream (UGSRunRandom). A crit multiplies damage by CritMultiplier (DA_CombatSettings on the GameMode, 2.0 by
	 * default). Server only. Call it once per hit that lands on a target; GA_Fire does, and so will the Floating Gun.
	 * A chance of 0 or 1 needs no dice, so it doesn't touch the run stream: runs without Loaded Dice replay as before.
	 * The caller marks a critical damage spec with SetByCaller Data.Critical = 1 (damage numbers show it).
	 */
	BANDITBALLER_API FGSCritRoll RollCriticalHit(const UAbilitySystemComponent* Source);
}
