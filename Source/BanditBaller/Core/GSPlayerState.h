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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnCashChangedSignature, int32, NewCash, int32, Delta);

/**
 * Per-player run state. Owns the player's AbilitySystemComponent + attributes (so they survive pawn changes) and
 * progression, and mirrors the player's permanent cash.
 * Cash is NOT owned here: UGSProfileSubsystem holds it (saved, kept across runs) and is the only writer
 * (SetCashFromProfile). The mirror replicates; OnCashChanged fires on the server and on clients.
 */
UCLASS()
class BANDITBALLER_API AGSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UGSAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Grants the startup ability set once per PlayerState (survives respawns). Authority only. Returns true if it granted now. */
	bool GrantStartupAbilitySet(const UGSAbilitySet* AbilitySet);

	/** The ASC as the project subclass (input-tag routing). */
	UGSAbilitySystemComponent* GetGSAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** The player's permanent cash, mirrored from UGSProfileSubsystem (change it through the subsystem). */
	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetCash() const { return Cash; }

	/** Server: the profile entry this player is bound to (empty until the client's player ID arrives). */
	const FString& GetProfileKey() const { return ProfileKey; }

	/** Level + XP (server-authoritative; UGSProgressionComponent::AddXP). */
	UFUNCTION(BlueprintPure, Category = "Progression")
	UGSProgressionComponent* GetProgressionComponent() const { return Progression; }

	/** Broadcast whenever Cash changes. */
	UPROPERTY(BlueprintAssignable, Category = "Economy")
	FGSOnCashChangedSignature OnCashChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UGSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGSAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UGSProgressionComponent> Progression;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Cash, Category = "Economy")
	int32 Cash = 0;

private:
	friend class UGSProfileSubsystem;

	/** UGSProfileSubsystem only: update the mirror (server; replicates). */
	void SetCashFromProfile(int32 NewCash);

	UFUNCTION()
	void OnRep_Cash(int32 OldCash);

	/** Server: UGSProfileSubsystem entry key (player ID). */
	UPROPERTY(VisibleInstanceOnly, Category = "Economy")
	FString ProfileKey;

	/** Server: cash collected before the player ID arrived; added to the profile on bind. */
	int32 PendingProfileCash = 0;

	FGSAbilitySetGrantedHandles StartupAbilitySetHandles;
	bool bStartupAbilitySetGranted = false;
};
