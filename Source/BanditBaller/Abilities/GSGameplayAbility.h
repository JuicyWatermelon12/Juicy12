// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GSGameplayAbility.generated.h"

/**
 * Base class for all BanditBaller gameplay abilities.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGSGameplayAbility();

	const FGameplayTag& GetActivationInputTag() const { return ActivationInputTag; }

protected:
	/** Input tag that activates this ability. Added to the spec's dynamic source tags when granted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FGameplayTag ActivationInputTag;
};
