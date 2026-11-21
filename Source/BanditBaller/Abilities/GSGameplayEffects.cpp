// BanditBaller

#include "GSGameplayEffects.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
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

UGSDamageEffect::UGSDamageEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UTargetTagRequirementsGameplayEffectComponent* Requirements =
		CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("DamageTargetRequirements"));
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GSGameplayTags::State_Invulnerable);
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GSGameplayTags::State_Dead);
	GEComponents.Add(Requirements);
}
