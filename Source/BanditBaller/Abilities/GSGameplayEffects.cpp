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

UGSDamageEffect::UGSDamageEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UTargetTagRequirementsGameplayEffectComponent* Requirements =
		CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("DamageTargetRequirements"));
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GSGameplayTags::State_Invulnerable);
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GSGameplayTags::State_Dead);
	GEComponents.Add(Requirements);
}
