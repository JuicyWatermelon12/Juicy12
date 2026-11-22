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
 * each after the recharge time. GA_Dash allows activation while stacks < its max charges.
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

/** +1 ExtraDashCharges per stack (GE_Upgrade_DashCharge); GA_Dash's max charges = DA_DashSettings MaxCharges + it. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_DashCharge : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_DashCharge(const FObjectInitializer& ObjectInitializer);
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

/** +8% FireRate per stack (GE_Upgrade_FireRate); GA_Fire's refire interval divides by FireRate. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_FireRate : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_FireRate(const FObjectInitializer& ObjectInitializer);
};

/** +15% PickupRadius per stack (GE_Upgrade_Magnet); cash pieces and XP orbs share the magnet radius. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_Magnet : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_Magnet(const FObjectInitializer& ObjectInitializer);
};

/** +15% MagazineSizeMultiplier per stack (GE_Upgrade_BiggerMags); magazines are ceil(MagazineSize x it). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_BiggerMags : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_BiggerMags(const FObjectInitializer& ObjectInitializer);
};

/** +1 HealthOnKill per stack (GE_Upgrade_Leech). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_Leech : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_Leech(const FObjectInitializer& ObjectInitializer);
};

/**
 * Instant: adds the target's own HealthOnKill to its Health (captured live, not snapshotted). AGSGameMode applies it to
 * the killer for each enemy kill (Leech). Health's clamp keeps it at or below MaxHealth.
 */
UCLASS()
class BANDITBALLER_API UGSHealthOnKillEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSHealthOnKillEffect(const FObjectInitializer& ObjectInitializer);
};

/** Instant: overrides Health with the target's current MaxHealth (GE_FullHeal; used after level-up rewards). */
UCLASS()
class BANDITBALLER_API UGSFullHealEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSFullHealEffect(const FObjectInitializer& ObjectInitializer);
};

/** Base for damage effects: cannot be applied to targets with State.Invulnerable, State.Dead or State.SafeZone. */
UCLASS()
class BANDITBALLER_API UGSDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSDamageEffect(const FObjectInitializer& ObjectInitializer);
};

/**
 * Infinite: grants State.SafeZone (GE_SafeZone; AGSHubGameMode applies it to every player in the hub). GA_Fire is
 * blocked by the tag and damage effects ignore targets that have it.
 */
UCLASS()
class BANDITBALLER_API UGSSafeZoneEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSSafeZoneEffect(const FObjectInitializer& ObjectInitializer);
};
