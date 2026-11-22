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
 * Base for level-up upgrade effects (GE_Upgrade_* derive from the subclasses below). Infinite and applied once per
 * upgrade: UGSUpgradeComponent applies it on the first pick and, on every later pick (or a Second Wind used up),
 * updates its SetByCaller values in place, so the attribute never dips between picks. A new run removes it.
 * The per-pick values live in the upgrade data (UGSUpgradeData::BaseValue x the pick's rarity), not in the effect:
 * - Percent upgrades use one Multiply (Additive) modifier with SetByCaller Data.UpgradeMultiplier = 1 + the total of
 *   every pick, so the attribute is base x (1 + total): a +10% and a +15% pick give +25% of the base, not compounding.
 * - Flat upgrades (Extra Jump) use one Additive modifier with SetByCaller Data.UpgradeValue = the total.
 * - Data.UpgradePicks = how many times the upgrade was picked (the Floating Gun's gun count).
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSUpgradeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect(const FObjectInitializer& ObjectInitializer);

	/** True if a modifier reads Data.UpgradeMultiplier: the upgrade's values are fractions of the base (0.1 = +10%). */
	bool IsPercentUpgrade() const;

protected:
	/** Adds a Multiply (Additive) modifier on Attribute from SetByCaller Data.UpgradeMultiplier (1 + total value). */
	void AddMultiplierModifier(const FGameplayAttribute& Attribute);

	/** Adds an Additive modifier on Attribute from SetByCaller Data.UpgradeValue (the total value). */
	void AddAdditiveModifier(const FGameplayAttribute& Attribute);

	/** Adds an Additive modifier on Attribute from SetByCaller Data.UpgradePicks (the number of picks). */
	void AddPicksModifier(const FGameplayAttribute& Attribute);
};

/** +1 ExtraJumps per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_ExtraJump); AGSCharacter keeps JumpMaxCount = 1 + ExtraJumps. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_ExtraJump : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_ExtraJump(const FObjectInitializer& ObjectInitializer);
};

/** +1 ExtraDashCharges per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_DashCharge); GA_Dash's max charges = DA_DashSettings MaxCharges + it. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_DashCharge : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_DashCharge(const FObjectInitializer& ObjectInitializer);
};

/** +10% ReloadSpeed per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_ReloadSpeed). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_ReloadSpeed : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_ReloadSpeed(const FObjectInitializer& ObjectInitializer);
};

/** +10% Damage per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_Damage). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_Damage : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_Damage(const FObjectInitializer& ObjectInitializer);
};

/** +8% MoveSpeed per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_MoveSpeed); AGSCharacter syncs MoveSpeed to MaxWalkSpeed. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_MoveSpeed : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_MoveSpeed(const FObjectInitializer& ObjectInitializer);
};

/** +8% FireRate per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_FireRate); GA_Fire's refire interval divides by FireRate. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_FireRate : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_FireRate(const FObjectInitializer& ObjectInitializer);
};

/** +15% PickupRadius per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_Magnet); cash pieces and XP orbs share the magnet radius. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_Magnet : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_Magnet(const FObjectInitializer& ObjectInitializer);
};

/** +15% MagazineSizeMultiplier per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_BiggerMags); magazines are ceil(MagazineSize x it). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_BiggerMags : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_BiggerMags(const FObjectInitializer& ObjectInitializer);
};

/** +1 HealthOnKill per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_Leech). */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_Leech : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_Leech(const FObjectInitializer& ObjectInitializer);
};

/** +1 PowderKeg per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_PowderKeg): each stack adds DA_CombatSettings ChancePerStack that a kill explodes. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_PowderKeg : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_PowderKeg(const FObjectInitializer& ObjectInitializer);
};

/** +1 Shields per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_FloatingShield): one more floating shield that blocks a hit, then recharges. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_FloatingShield : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_FloatingShield(const FObjectInitializer& ObjectInitializer);
};

/**
 * GE_Upgrade_FloatingGun: FloatingGunLevel = the number of picks (1 = a gun over the right shoulder, 2+ = and one over
 * the left) and FloatingGunPower = the total value (the guns' +damage / +fire rate steps).
 */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_FloatingGun : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_FloatingGun(const FObjectInitializer& ObjectInitializer);
};

/** +1 SecondWinds per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_SecondWind): survive one more killing blow. Using one removes one pick. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_SecondWind : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_SecondWind(const FObjectInitializer& ObjectInitializer);
};

/**
 * Timed invulnerability: grants State.Invulnerable for SetByCaller Data.Duration seconds (Second Wind; DA_CombatSettings
 * SecondWindInvulnerableEffect can replace it). GE_Damage isn't applied while the tag is on.
 */
UCLASS()
class BANDITBALLER_API UGSTimedInvulnerabilityEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSTimedInvulnerabilityEffect(const FObjectInitializer& ObjectInitializer);
};

/** +1 Pierce per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_PiercingRounds): each shot passes through one more target. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_PiercingRounds : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_PiercingRounds(const FObjectInitializer& ObjectInitializer);
};

/** +0.05 CritChance per Common pick (BaseValue on the DA_Upgrade_*; GE_Upgrade_LoadedDice); 20 Common picks = every hit critical. */
UCLASS()
class BANDITBALLER_API UGSUpgradeEffect_LoadedDice : public UGSUpgradeEffect
{
	GENERATED_BODY()

public:
	UGSUpgradeEffect_LoadedDice(const FObjectInitializer& ObjectInitializer);
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

/**
 * Instant: adds SetByCaller Data.Heal to the target's Health (health orbs; DA_CashSettings HealthOrbHealEffect). Health's
 * clamp keeps it at or below MaxHealth, so anything over is lost.
 */
UCLASS()
class BANDITBALLER_API UGSHealEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSHealEffect(const FObjectInitializer& ObjectInitializer);
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
