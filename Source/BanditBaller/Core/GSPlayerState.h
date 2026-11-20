// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GSPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnCashChangedSignature, int32, NewCash, int32, Delta);

/**
 * Per-player persistent state. Owns the player's cash.
 * Cash is only modified with authority; replication will be added later.
 */
UCLASS()
class BANDITBALLER_API AGSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
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
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Economy")
	int32 Cash = 0;

private:
	void SetCash(int32 NewCash);
};
