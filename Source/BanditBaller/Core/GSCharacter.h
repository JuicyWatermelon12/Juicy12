// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "BanditBallerCharacter.h"
#include "GSCharacter.generated.h"

class UAbilitySystemComponent;
class UGSAbilitySet;
struct FOnAttributeChangeData;

/**
 * Player character. Third-person camera/movement/input come from the template character.
 * The ASC lives on AGSPlayerState; this pawn is its avatar.
 */
UCLASS()
class BANDITBALLER_API AGSCharacter : public ABanditBallerCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSCharacter();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	//~ APawn
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_PlayerState() override;

	/** Abilities/effects granted on first spawn (e.g. DA_PlayerAbilitySet with GE_InitPlayerStats). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UGSAbilitySet> StartupAbilitySet;

private:
	/** Binds this pawn as the avatar of the PlayerState's ASC. */
	void InitAbilitySystem();
	void ClearAbilitySystem();

	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void ApplyMoveSpeed(float NewSpeed);

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystem;
	FDelegateHandle MoveSpeedChangedHandle;
};
