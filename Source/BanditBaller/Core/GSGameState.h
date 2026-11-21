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
	/** After combat: decide whether the Market District opens this round (set rounds or chance, M2). */
	MarketDecision,
	/** The party is in the Market District (shop + casino). */
	MarketVisit,
	GameOver
};

/** Where the party currently is. Combat and enemy spawns happen only in the Town. */
UENUM(BlueprintType)
enum class EGSArea : uint8
{
	Town,
	/** Safe, separate area reached by teleport (streamed sublevel L_MarketDistrict_Graybox). */
	MarketDistrict
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnAreaChangedSignature, EGSArea, OldArea, EGSArea, NewArea);

/**
 * Shared round state (round number, phase, phase timer, current area, market availability).
 * Written by AGSGameMode on the server; CurrentArea and bMarketOpenThisRound replicate to clients.
 */
UCLASS()
class BANDITBALLER_API AGSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Round")
	int32 GetRoundNumber() const { return RoundNumber; }

	UFUNCTION(BlueprintPure, Category = "Round")
	EGSRoundPhase GetRoundPhase() const { return RoundPhase; }

	UFUNCTION(BlueprintPure, Category = "Round")
	float GetPhaseTimeRemaining() const { return PhaseTimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "Area")
	EGSArea GetCurrentArea() const { return CurrentArea; }

	UFUNCTION(BlueprintPure, Category = "Area")
	bool IsMarketOpenThisRound() const { return bMarketOpenThisRound; }

	/** Server only (AGSGameMode). */
	void SetCurrentArea(EGSArea NewArea);
	void SetMarketOpenThisRound(bool bOpen) { bMarketOpenThisRound = bOpen; }

	/** Fires on server and clients when CurrentArea changes. */
	UPROPERTY(BlueprintAssignable, Category = "Area")
	FGSOnAreaChangedSignature OnAreaChanged;

protected:
	/** Current round, 0 before the first round starts. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	int32 RoundNumber = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	EGSRoundPhase RoundPhase = EGSRoundPhase::WaitingToStart;

	/** Seconds left in the current phase. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Round")
	float PhaseTimeRemaining = 0.f;

	/** Where the party is. Enemies only target players while this is Town. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentArea, Category = "Area")
	EGSArea CurrentArea = EGSArea::Town;

	/** Whether the Market District opens after this round's combat (decided in MarketDecision, M2). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Area")
	bool bMarketOpenThisRound = false;

private:
	UFUNCTION()
	void OnRep_CurrentArea(EGSArea OldArea);
};
