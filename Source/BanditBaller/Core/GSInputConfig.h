// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GSInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FGSInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<const UInputAction> InputAction = nullptr;

	/** Matched against an ability's ActivationInputTag (spec dynamic source tags). */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (Categories = "Ability"))
	FGameplayTag InputTag;
};

/**
 * Maps Enhanced Input actions to ability input tags. Pressing an action activates whatever
 * granted ability carries the matching tag; the character never calls abilities directly.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (TitleProperty = "InputAction"))
	TArray<FGSInputAction> AbilityInputActions;
};
