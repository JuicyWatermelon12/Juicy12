// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GSGameplayAbility.generated.h"

class AGSCharacter;
class UGSWeaponComponent;

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

	/**
	 * If true, the ASC re-activates this ability every frame its input is held (once the previous
	 * activation has ended). Used for full-auto fire. Called on the ability CDO.
	 */
	virtual bool ShouldRetriggerWhileInputHeld(const FGameplayAbilityActorInfo* ActorInfo) const { return false; }

protected:
	/** Input tag that activates this ability. Added to the spec's dynamic source tags when granted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FGameplayTag ActivationInputTag;

	static AGSCharacter* GetGSCharacter(const FGameplayAbilityActorInfo* ActorInfo);
	static UGSWeaponComponent* GetWeaponComponent(const FGameplayAbilityActorInfo* ActorInfo);
};
