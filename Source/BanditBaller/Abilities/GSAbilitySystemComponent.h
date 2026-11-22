// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GSAbilitySystemComponent.generated.h"

/**
 * Project ASC. Routes input by gameplay tag: abilities whose spec carries the input tag
 * (from UGSGameplayAbility::ActivationInputTag) are activated on press and notified on release.
 * Abilities that opt in (ShouldRetriggerWhileInputHeld) are re-activated while the input is held.
 */
UCLASS()
class BANDITBALLER_API UGSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	/** Call every frame on the owning client: re-activates held, repeatable abilities (e.g. full-auto fire). */
	void ProcessHeldAbilityInput();

	/** Forget all held inputs (e.g. on unpossess). */
	void ClearAbilityInput();

	/** True while InputTag's input is held (pressed and not released or cleared). GA_Fire checks it before each shot. */
	bool IsAbilityInputHeld(const FGameplayTag& InputTag) const { return HeldInputTags.HasTagExact(InputTag); }

protected:
	//~ UAbilitySystemComponent
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

private:
	FGameplayTagContainer HeldInputTags;
};
