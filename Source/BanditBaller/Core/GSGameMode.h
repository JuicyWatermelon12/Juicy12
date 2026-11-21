// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameMode.generated.h"

class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnEnemyKilledSignature, AActor*, Enemy, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnPlayerDiedSignature, APawn*, Player, AActor*, Killer);

/**
 * Server-only game rules. Will own the round phase flow (combat -> market -> next round).
 * Replicated round state lives in AGSGameState.
 * Receives deaths from UGSHealthComponent: broadcasts OnEnemyKilled (cash/rounds listen here)
 * and shows the death screen + restart flow for players.
 */
UCLASS()
class BANDITBALLER_API AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	/** Called by UGSHealthComponent when a non-player actor dies. */
	void HandleEnemyKilled(AActor* Enemy, AActor* Killer);

	/** Called by UGSHealthComponent when a player-controlled pawn dies. */
	void HandlePlayerDeath(APawn* Player, AActor* Killer);

	/** A dead player asked to restart (death screen). Restarts the level. */
	void RequestRestart(AController* Requester);

	/** Server-only: an enemy (anything with a health component that isn't a player) died. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGSOnEnemyKilledSignature OnEnemyKilled;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGSOnPlayerDiedSignature OnPlayerDied;

protected:
	/** Shown to a player when they die (WBP_DeathScreen). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DeathScreenWidgetClass;
};
