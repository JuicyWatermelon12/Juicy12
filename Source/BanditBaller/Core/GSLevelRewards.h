// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSLevelRewards.generated.h"

class UGameplayEffect;

/** Extra Gameplay Effects granted when a player reaches one specific level. */
USTRUCT(BlueprintType)
struct FGSLevelReward
{
	GENERATED_BODY()

	/** The level being reached (2 = first level-up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards", meta = (ClampMin = "2"))
	int32 Level = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	TArray<TSubclassOf<UGameplayEffect>> Effects;
};

/**
 * What a player gets on level-up (DA_LevelRewards, set as LevelRewards on BP_GSGameMode). Applied on the server by
 * UGSProgressionComponent for every level gained: EveryLevelEffects, then the LevelSpecificRewards entries for that
 * level; after all levels of one XP gain, FullHealEffect once. Duration/infinite rewards are removed when progression
 * resets (new run).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSLevelRewards : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** EveryLevelEffects followed by LevelSpecificRewards for Level. */
	void GetEffectsForLevel(int32 Level, TArray<TSubclassOf<UGameplayEffect>>& OutEffects) const
	{
		for (const TSubclassOf<UGameplayEffect>& Effect : EveryLevelEffects)
		{
			if (Effect)
			{
				OutEffects.Add(Effect);
			}
		}
		for (const FGSLevelReward& Reward : LevelSpecificRewards)
		{
			if (Reward.Level == Level)
			{
				for (const TSubclassOf<UGameplayEffect>& Effect : Reward.Effects)
				{
					if (Effect)
					{
						OutEffects.Add(Effect);
					}
				}
			}
		}
	}

	/**
	 * Player-facing summary of what levels FirstLevel..LastLevel grant, built from the effects' static Additive
	 * modifiers and summed per attribute, e.g. "+20 Max Health" for two levels of GE_LevelUp_MaxHealth.
	 * One line per attribute; empty if nothing is describable. Used by the level-up banner (HUD).
	 */
	FText DescribeRewards(int32 FirstLevel, int32 LastLevel) const;

	/** Applied on every level-up (e.g. GE_LevelUp_MaxHealth: one +MaxHealth stack per level). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	TArray<TSubclassOf<UGameplayEffect>> EveryLevelEffects;

	/** Extra rewards for reaching specific levels (applied after EveryLevelEffects). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	TArray<FGSLevelReward> LevelSpecificRewards;

	/** Applied once after the rewards of a level-up (GE_FullHeal: Health = new MaxHealth). Skipped for dead players. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	TSubclassOf<UGameplayEffect> FullHealEffect;
};
