// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSCriticalHit.h"
#include "GSCombatSettings.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

/** Player combat tuning shared by every damage source (DA_CombatSettings, set as CombatSettings on BP_GSGameMode). */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSCombatSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Damage multiplier of a critical hit (CritChance, Loaded Dice). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Critical Hits", meta = (ClampMin = "1"))
	float CritMultiplier = 2.f;

	/** Damage multiplier of a triple hit: a crit chance past 100% is the chance that a crit uses this instead (130% = every hit crits, 30% of them triple). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Critical Hits", meta = (ClampMin = "1"))
	float TripleCritMultiplier = 3.f;

	// --- Powder Keg: enemies the player kills have a chance to explode ---

	/**
	 * Chance per PowderKeg stack that an enemy the player kills explodes (rolled on the run stream per kill). Past 100%
	 * every kill explodes and the rest is the chance of a double-size blast. DA_Upgrade_PowderKeg's DisplayScale (3) is
	 * set to match it by hand (Upgrades.PowderKeg checks it).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", ClampMax = "1"))
	float ChancePerStack = 0.03f;

	/** Blast damage, before the player's Damage multiplier. The same at every stack count. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0"))
	float BlastDamage = 30.f;

	/** Blast radius. The same at every stack count. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", Units = "cm"))
	float BlastRadius = 300.f;

	/** A double-size blast (Powder Keg past 100%) has BlastRadius x this; the same damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "1"))
	float DoubleBlastRadiusMultiplier = 2.f;

	/** An enemy killed by a blast rolls its own chance; if it explodes, it does this long after (so chains ripple). World time: pauses with the game. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", Units = "s"))
	float ChainDelay = 0.1f;

	/** The damage effect blasts apply (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg")
	TSubclassOf<UGameplayEffect> BlastDamageEffect;

	/** Placeholder explosion (its size at scale 1 matches BlastRadius). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg")
	TObjectPtr<UNiagaraSystem> BlastEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg")
	TObjectPtr<USoundBase> BlastSound;

	// --- Floating Shield: each shield blocks one hit, then recharges ---

	/** Seconds a shield is gone after it blocked a hit (each shield on its own; world time, so it pauses with the game). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Floating Shield", meta = (ClampMin = "0.1", Units = "s"))
	float ShieldCooldown = 10.f;

	// --- Second Wind: a killing blow uses one instead ---

	/** Share of MaxHealth the player comes back with (0.3 = 30%). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Second Wind", meta = (ClampMin = "0.01", ClampMax = "1"))
	float SecondWindHealthFraction = 0.3f;

	/** Seconds of invulnerability right after (State.Invulnerable: GE_Damage isn't applied, no shield is used). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Second Wind", meta = (ClampMin = "0", Units = "s"))
	float SecondWindInvulnerableTime = 2.f;

	/** The invulnerability (SetByCaller Data.Duration). Unset = the native UGSTimedInvulnerabilityEffect. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Second Wind")
	TSubclassOf<UGameplayEffect> SecondWindInvulnerableEffect;

	/** Stacks x ChancePerStack, not clamped (1.3 = 130%). */
	float GetRawExplodeChance(float Stacks) const { return Stacks > 0.f ? Stacks * ChancePerStack : 0.f; }

	/** Chance (0..1) that a kill explodes at Stacks PowderKeg stacks: Stacks x ChancePerStack, clamped (GSCombat::SnapChance). */
	float GetExplodeChance(float Stacks) const { return GSCombat::SnapChance(GetRawExplodeChance(Stacks)); }

	/** Chance (0..1) that a blast is double size: the explode chance past 100% (1.3 -> 0.3), clamped. */
	float GetDoubleBlastChance(float Stacks) const { return GSCombat::SnapChance(GetRawExplodeChance(Stacks) - 1.f); }
};
