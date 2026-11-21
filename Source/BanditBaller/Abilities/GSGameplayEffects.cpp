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
	StackLimitCount = 0; // no limit here; GA_Dash gates on MaxCharges from its settings
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
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0; // no limit here; UGSUpgradeData::MaxStacks caps picks
}

void UGSUpgradeEffect::AddMultiplierModifier(const FGameplayAttribute& Attribute, float PerStackBonus)
{
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::MultiplyAdditive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f + PerStackBonus));
}

UGSUpgradeEffect_ReloadSpeed::UGSUpgradeEffect_ReloadSpeed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetReloadSpeedAttribute(), 0.10f);
}

UGSUpgradeEffect_Damage::UGSUpgradeEffect_Damage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetDamageAttribute(), 0.10f);
}

UGSUpgradeEffect_MoveSpeed::UGSUpgradeEffect_MoveSpeed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AddMultiplierModifier(UGSAttributeSet::GetMoveSpeedAttribute(), 0.08f);
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
	GEComponents.Add(Requirements);
}
