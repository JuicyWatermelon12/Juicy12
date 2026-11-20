// BanditBaller

#include "GSAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void UGSAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> ToActivate;
	{
		ABILITYLIST_SCOPE_LOCK();
		for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
		{
			if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				AbilitySpecInputPressed(Spec);
				if (!Spec.IsActive())
				{
					ToActivate.Add(Spec.Handle);
				}
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : ToActivate)
	{
		TryActivateAbility(Handle);
	}
}

void UGSAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpecInputReleased(Spec);
		}
	}
}

void UGSAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// Let WaitInputPress tasks on active abilities (and the server) hear about the press.
	if (Spec.IsActive())
	{
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		const FPredictionKey PredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : FPredictionKey();
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, PredictionKey);
	}
}

void UGSAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (Spec.IsActive())
	{
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		const FPredictionKey PredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : FPredictionKey();
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, PredictionKey);
	}
}
