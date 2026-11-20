// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GSAbilitySet.h"
#include "GSPlayerState.generated.h"

class UAbilitySystemComponent;
class UGSAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnCashChangedSignature, int32, NewCash, int32, Delta);

/**
 * Per-player persistent state. Owns the player's AbilitySystemComponent + attributes
 * (so they survive pawn changes) and the player's cash.
 * Cash is only modified with authority; cash replication will be added later.
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

	/** Grants the startup ability set once per PlayerState (survives respawns). Authority only. */
	void GrantStartupAbilitySet(const UGSAbilitySet* AbilitySet);

	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetCash() const { return Cash; }

	/** Adds a positive amount of cash. Ignored if Amount <= 0 or without authority. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Economy")
	void AddCash(int32 Amount);

	/** Spends cash if the player can afford it. Returns true on success. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Economy")
	bool TrySpendCash(int32 Amount);

	/** Broadcast whenever Cash changes. */
	UPROPERTY(BlueprintAssignable, Category = "Economy")
	FGSOnCashChangedSignature OnCashChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGSAttributeSet> AttributeSet;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Economy")
	int32 Cash = 0;

private:
	void SetCash(int32 NewCash);

	FGSAbilitySetGrantedHandles StartupAbilitySetHandles;
	bool bStartupAbilitySetGranted = false;
};
