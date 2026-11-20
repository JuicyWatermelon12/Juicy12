// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "BanditBallerCharacter.h"
#include "GSCharacter.generated.h"

class UAbilitySystemComponent;
class UGSAttributeSet;

/**
 * Player character. Third-person camera/movement/input come from the template character;
 * stats, buffs and abilities go through the AbilitySystemComponent.
 */
UCLASS()
class BANDITBALLER_API AGSCharacter : public ABanditBallerCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSCharacter();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UGSAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGSAttributeSet> AttributeSet;
};
