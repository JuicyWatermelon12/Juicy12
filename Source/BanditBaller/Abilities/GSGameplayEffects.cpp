// BanditBaller

#include "GSGameplayEffects.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GSAttributeSet.h"
#include "GSGameplayTags.h"

UGSCooldownEffect::UGSCooldownEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = GSGameplayTags::Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	CooldownTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("CooldownTags"));
	GEComponents.Add(CooldownTagsComponent);
}

void UGSCooldownEffect::SetCooldownTag(const FGameplayTag& Tag)
{
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(Tag);
	CooldownTagsComponent->SetAndApplyTargetTagChanges(Tags);
}

UGSCooldownEffect_Slide::UGSCooldownEffect_Slide(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetCooldownTag(GSGameplayTags::Cooldown_Slide);
}

UGSCooldownEffect_Dash::UGSCooldownEffect_Dash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetCooldownTag(GSGameplayTags::Cooldown_Dash);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0; // no limit here; GA_Dash gates on its max charges (settings + ExtraDashCharges)
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}

UGSEnemyRoundScalingEffect::UGSEnemyRoundScalingEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	auto AddMultiplier = [this](const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;

		FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::MultiplyCompound;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	};
	AddMultiplier(UGSAttributeSet::GetMaxHealthAttribute(), GSGameplayTags::Data_HealthMultiplier);
	AddMultiplier(UGSAttributeSet::GetDamageAttribute(), GSGameplayTags::Data_DamageMultiplier);
}

UGSLevelUpMaxHealthEffect::UGSLevelUpMaxHealthEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0; // no limit: one stack per level-up

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UGSAttributeSet::GetMaxHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.f));
}

UGSUpgradeEffect::UGSUpgradeEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Applied once per upgrade (no stacking); UGSUpgradeComponent updates its SetByCaller values on every pick.
	DurationPolicy = EGameplayEffectDurationType::Infinite;
}

namespace
{
	FGameplayEffectModifierMagnitude MakeSetByCaller(const FGameplayTag& Tag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = Tag;
		return FGameplayEffectModifierMagnitude(SetByCaller);
	}
}

bool UGSUpgradeEffect::IsPercentUpgrade() const
{
	for (const FGameplayModifierInfo& Modifier : Modifiers)
	{
		if (Modifier.ModifierMagnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::SetByCaller
			&& Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag == GSGameplayTags::Data_UpgradeMultiplier)
		{
			return true;
		}
	}
	return false;
}

void UGSUpgradeEffect::AddMultiplierModifier(const FGameplayAttribute& Attribute)
{
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::MultiplyAdditive;
	Modifier.ModifierMagnitude = MakeSetByCaller(GSGameplayTags::Data_UpgradeMultiplier);
}

void UGSUpgradeEffect::AddAdditiveModifier(const FGameplayAttribute& Attribute)
{
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = MakeSetByCaller(GSGameplayTags::Data_UpgradeValue);
}

void UGSUpgradeEffect::AddPicksModifier(const FGameplayAttribute& Attribute)
{
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = MakeSetByCaller(GSGameplayTags::Data_UpgradePicks);
}

UGSUpgradeEffect_ExtraJump::UGSUpgradeEffect_ExtraJump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetExtraJumpsAttribute());
}

UGSUpgradeEffect_DashCharge::UGSUpgradeEffect_DashCharge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetExtraDashChargesAttribute());
}

UGSUpgradeEffect_ReloadSpeed::UGSUpgradeEffect_ReloadSpeed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetReloadSpeedAttribute());
}

UGSUpgradeEffect_Damage::UGSUpgradeEffect_Damage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetDamageAttribute());
}

UGSUpgradeEffect_MoveSpeed::UGSUpgradeEffect_MoveSpeed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetMoveSpeedAttribute());
}

UGSUpgradeEffect_FireRate::UGSUpgradeEffect_FireRate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetFireRateAttribute());
}

UGSUpgradeEffect_Magnet::UGSUpgradeEffect_Magnet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetPickupRadiusAttribute());
}

UGSUpgradeEffect_BiggerMags::UGSUpgradeEffect_BiggerMags(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetMagazineSizeMultiplierAttribute());
}

UGSUpgradeEffect_Leech::UGSUpgradeEffect_Leech(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetHealthOnKillAttribute());
}

UGSUpgradeEffect_PowderKeg::UGSUpgradeEffect_PowderKeg(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetPowderKegAttribute());
}

UGSUpgradeEffect_FloatingShield::UGSUpgradeEffect_FloatingShield(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetShieldsAttribute());
}

UGSUpgradeEffect_FloatingGun::UGSUpgradeEffect_FloatingGun(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddPicksModifier(UGSAttributeSet::GetFloatingGunLevelAttribute()); // guns
	AddAdditiveModifier(UGSAttributeSet::GetFloatingGunPowerAttribute()); // +damage / fire rate steps
}

UGSUpgradeEffect_RazorCards::UGSUpgradeEffect_RazorCards(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddPicksModifier(UGSAttributeSet::GetRazorCardsLevelAttribute()); // owned
	AddAdditiveModifier(UGSAttributeSet::GetRazorCardsPowerAttribute()); // +damage / fire rate steps
}

UGSUpgradeEffect_SecondWind::UGSUpgradeEffect_SecondWind(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetSecondWindsAttribute());
}

UGSTimedInvulnerabilityEffect::UGSTimedInvulnerabilityEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat Duration;
	Duration.DataTag = GSGameplayTags::Data_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);

	UTargetTagsGameplayEffectComponent* TagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("InvulnerableTags"));
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(GSGameplayTags::State_Invulnerable);
	TagsComponent->SetAndApplyTargetTagChanges(Tags);
	GEComponents.Add(TagsComponent);
}

UGSUpgradeEffect_PiercingRounds::UGSUpgradeEffect_PiercingRounds(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetPierceAttribute());
}

UGSUpgradeEffect_LoadedDice::UGSUpgradeEffect_LoadedDice(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddAdditiveModifier(UGSAttributeSet::GetCritChanceAttribute());
}

UGSHealthOnKillEffect::UGSHealthOnKillEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FAttributeBasedFloat HealthOnKill;
	HealthOnKill.Coefficient = FScalableFloat(1.f);
	HealthOnKill.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UGSAttributeSet::GetHealthOnKillAttribute(), EGameplayEffectAttributeCaptureSource::Target, /*bSnapshot*/ false);

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UGSAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthOnKill);
}

UGSHealEffect::UGSHealEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat Heal;
	Heal.DataTag = GSGameplayTags::Data_Heal;

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UGSAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Heal);
}

UGSFullHealEffect::UGSFullHealEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Health = the target's MaxHealth at application time (after any reward that raised it).
	FAttributeBasedFloat MaxHealth;
	MaxHealth.Coefficient = FScalableFloat(1.f);
	MaxHealth.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UGSAttributeSet::GetMaxHealthAttribute(), EGameplayEffectAttributeCaptureSource::Target, /*bSnapshot*/ false);

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UGSAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Override;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxHealth);
}

UGSDamageEffect::UGSDamageEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UTargetTagRequirementsGameplayEffectComponent* Requirements =
		CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("DamageTargetRequirements"));
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GSGameplayTags::State_Invulnerable);
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GSGameplayTags::State_Dead);
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GSGameplayTags::State_SafeZone);
	GEComponents.Add(Requirements);
}

UGSSafeZoneEffect::UGSSafeZoneEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* TagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("SafeZoneTags"));
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(GSGameplayTags::State_SafeZone);
	TagsComponent->SetAndApplyTargetTagChanges(Tags);
	GEComponents.Add(TagsComponent);
}
