// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GSGameState.generated.h"

/** High-level phase of the current round. */
UENUM(BlueprintType)
enum class EGSRoundPhase : uint8
{
	WaitingToStart,
	Combat,
	Market,
	GameOver
};

/**
 * Shared round state (round number, phase, phase timer). Written by AGSGameMode on the server.
 * Replication will be added later.
 */
UCLASS()
class BANDITBALLER_API AGSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Round")
	int32 GetRoundNumber() const { return RoundNumber; }

	UFUNCTION(BlueprintPure, Category = "Round")
	EGSRoundPhase GetRoundPhase() const { return RoundPhase; }

	UFUNCTION(BlueprintPure, Category = "Round")
	float GetPhaseTimeRemaining() const { return PhaseTimeRemaining; }

protected:
	/** Current round, 0 before the first round starts. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	int32 RoundNumber = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	EGSRoundPhase RoundPhase = EGSRoundPhase::WaitingToStart;

	/** Seconds left in the current phase. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	float PhaseTimeRemaining = 0.f;
};
