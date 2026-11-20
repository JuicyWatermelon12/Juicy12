// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GSAbilitySystemComponent.generated.h"

/**
 * Project ASC. Routes input by gameplay tag: abilities whose spec carries the input tag
 * (from UGSGameplayAbility::ActivationInputTag) are activated on press and notified on release.
 */
UCLASS()
class BANDITBALLER_API UGSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

protected:
	//~ UAbilitySystemComponent
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
};
