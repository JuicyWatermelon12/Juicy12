// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GSGameplayEffects.generated.h"

class UTargetTagsGameplayEffectComponent;

/**
 * Base for cooldown effects: HasDuration with the duration from SetByCaller Data.Cooldown
 * (the ability sets it from its settings asset) and a granted cooldown tag.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSCooldownEffect(const FObjectInitializer& ObjectInitializer);

protected:
	void SetCooldownTag(const FGameplayTag& Tag);

	UPROPERTY()
	TObjectPtr<UTargetTagsGameplayEffectComponent> CooldownTagsComponent;
};

/** Grants Cooldown.Slide. */
UCLASS()
class BANDITBALLER_API UGSCooldownEffect_Slide : public UGSCooldownEffect
{
	GENERATED_BODY()

public:
	UGSCooldownEffect_Slide(const FObjectInitializer& ObjectInitializer);
};

/**
 * Grants Cooldown.Dash. Each dash adds a stack (one spent charge); stacks expire one at a time,
 * each after the recharge time. GA_Dash allows activation while stacks < MaxCharges.
 */
UCLASS()
class BANDITBALLER_API UGSCooldownEffect_Dash : public UGSCooldownEffect
{
	GENERATED_BODY()

public:
	UGSCooldownEffect_Dash(const FObjectInitializer& ObjectInitializer);
};

/**
 * Enemy difficulty scaling (GE_EnemyRoundScaling derives from this). Infinite; multiplies MaxHealth by SetByCaller
 * Data.HealthMultiplier and Damage by Data.DamageMultiplier (MultiplyCompound). AGSGameMode applies it when an
 * enemy spawns with magnitudes from UGSDifficultyCurve for the current round.
 */
UCLASS()
class BANDITBALLER_API UGSEnemyRoundScalingEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSEnemyRoundScalingEffect(const FObjectInitializer& ObjectInitializer);
};

/**
 * Level-up reward (GE_LevelUp_MaxHealth derives from this). Infinite, stacks by target without limit; each stack adds
 * the modifier magnitude (default +10, edit it on the asset) to MaxHealth. UGSProgressionComponent adds one stack per
 * level-up through UGSLevelRewards and removes all stacks when progression resets.
 */
UCLASS()
class BANDITBALLER_API UGSLevelUpMaxHealthEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSLevelUpMaxHealthEffect(const FObjectInitializer& ObjectInitializer);
};

/**
 * Base for level-up upgrade effects (GE_Upgrade_* derive from the subclasses below). Infinite, stacks by target with
 * no limit on the effect itself (UGSUpgradeData::MaxStacks caps picks); UGSUpgradeComponent adds one stack per pick
 * and removes them all when a new run starts.
 * Percent upgrades use one Multiply (Additive) modifier, magnitude 1 + per-stack bonus (edit it on the asset). GAS
 * scales it by the stack count around the bias of 1, so N stacks give base x (1 + bonus x N): +10% per stack stays
 * +10% of the base value, not compounding. Flat upgrades (Extra Jump) use one Additive modifier: N stacks add N x it.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSUpgradeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect(const FObjectInitializer& ObjectInitializer);

protected:
	/** Adds the effect's Multiply (Additive) modifier on Attribute with magnitude 1 + PerStackBonus. */
	void AddMultiplierModifier(const FGameplayAttribute& Attribute, float PerStackBonus);

	/** Adds the effect's Additive modifier on Attribute with magnitude PerStackAmount. */
	void AddAdditiveModifier(const FGameplayAttribute& Attribute, float PerStackAmount);
};

/** +1 ExtraJumps per stack (GE_Upgrade_ExtraJump); AGSCharacter keeps JumpMaxCount = 1 + ExtraJumps. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_ExtraJump : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_ExtraJump(const FObjectInitializer& ObjectInitializer);
};

/** +10% ReloadSpeed per stack (GE_Upgrade_ReloadSpeed). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_ReloadSpeed : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_ReloadSpeed(const FObjectInitializer& ObjectInitializer);
};

/** +10% Damage per stack (GE_Upgrade_Damage). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_Damage : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_Damage(const FObjectInitializer& ObjectInitializer);
};

/** +8% MoveSpeed per stack (GE_Upgrade_MoveSpeed); AGSCharacter syncs MoveSpeed to MaxWalkSpeed. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_MoveSpeed : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_MoveSpeed(const FObjectInitializer& ObjectInitializer);
};

/** Instant: overrides Health with the target's current MaxHealth (GE_FullHeal; used after level-up rewards). */
UCLASS()
class BANDITBALLER_API UGSFullHealEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSFullHealEffect(const FObjectInitializer& ObjectInitializer);
};

/** Base for damage effects: cannot be applied to targets with State.Invulnerable or State.Dead. */
UCLASS()
class BANDITBALLER_API UGSDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSDamageEffect(const FObjectInitializer& ObjectInitializer);
};
