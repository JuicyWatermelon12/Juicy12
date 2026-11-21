// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GSAbilitySet.h"
#include "GSAbilitySystemComponent.h"
#include "GSPlayerState.generated.h"

class UGSAttributeSet;
class UGSProgressionComponent;
class UGSUpgradeComponent;

/**
 * Per-player run state. Owns the player's AbilitySystemComponent + attributes (so they survive pawn changes) and
 * progression (XP / level).
 * Cash is not stored here: the permanent balance lives in UGSProfileSubsystem (saved, kept across runs).
 */
UCLASS()
class BANDITBALLER_API AGSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSPlayerState();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UGSAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Grants the startup ability set once per PlayerState (survives respawns). Authority only. Returns true if it granted now. */
	bool GrantStartupAbilitySet(const UGSAbilitySet* AbilitySet);

	/** The ASC as the project subclass (input-tag routing). */
	UGSAbilitySystemComponent* GetGSAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Level + XP (server-authoritative; UGSProgressionComponent::AddXP). */
	UFUNCTION(BlueprintPure, Category = "Progression")
	UGSProgressionComponent* GetProgressionComponent() const { return Progression; }

	/** Level-up upgrades picked this run (UGSUpgradeComponent::GrantUpgrade). */
	UFUNCTION(BlueprintPure, Category = "Progression")
	UGSUpgradeComponent* GetUpgradeComponent() const { return Upgrades; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UGSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGSAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UGSProgressionComponent> Progression;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UGSUpgradeComponent> Upgrades;

private:
	FGSAbilitySetGrantedHandles StartupAbilitySetHandles;
	bool bStartupAbilitySetGranted = false;
};
