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

	// --- Powder Keg: enemies the player kills explode ---

	/** Blast damage at 1 stack, before the player's Damage multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0"))
	float BlastDamage = 30.f;

	/** Extra blast damage per stack after the first. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0"))
	float BlastDamagePerExtraStack = 10.f;

	/** Blast radius at 1 stack. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", Units = "cm"))
	float BlastRadius = 300.f;

	/** Extra blast radius per stack after the first. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", Units = "cm"))
	float BlastRadiusPerExtraStack = 50.f;

	/** An enemy killed by a blast explodes this long after (so chains ripple). World time: pauses with the game. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg", meta = (ClampMin = "0", Units = "s"))
	float ChainDelay = 0.1f;

	/** The damage effect blasts apply (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg")
	TSubclassOf<UGameplayEffect> BlastDamageEffect;

	/** Placeholder explosion, scaled with the radius (its size at 1 = BlastRadius). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg")
	TObjectPtr<UNiagaraSystem> BlastEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powder Keg")
	TObjectPtr<USoundBase> BlastSound;

	float GetBlastDamage(int32 Stacks) const { return Stacks > 0 ? BlastDamage + BlastDamagePerExtraStack * (Stacks - 1) : 0.f; }
	float GetBlastRadius(int32 Stacks) const { return Stacks > 0 ? BlastRadius + BlastRadiusPerExtraStack * (Stacks - 1) : 0.f; }
};
