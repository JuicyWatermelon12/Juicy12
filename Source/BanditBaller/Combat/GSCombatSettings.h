// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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

	// --- Powder Keg: enemies the player kills have a chance to explode ---

	/**
	 * Chance per PowderKeg stack that an enemy the player kills explodes (rolled on the run stream per kill), clamped to
	 * 100%. DA_Upgrade_PowderKeg's card text and MaxStacks (34 at 3%) are written to match it by hand.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", ClampMax = "1"))
	float ChancePerStack = 0.03f;

	/** Blast damage, before the player's Damage multiplier. The same at every stack count. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0"))
	float BlastDamage = 30.f;

	/** Blast radius. The same at every stack count. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", Units = "cm"))
	float BlastRadius = 300.f;

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

	/** Chance (0..1) that a kill explodes at Stacks PowderKeg stacks: Stacks x ChancePerStack, clamped. */
	float GetExplodeChance(int32 Stacks) const { return Stacks > 0 ? FMath::Clamp(Stacks * ChancePerStack, 0.f, 1.f) : 0.f; }
};
