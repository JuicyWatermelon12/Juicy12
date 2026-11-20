// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameMode.generated.h"

/**
 * Server-only game rules. Will own the round phase flow (combat -> market -> next round).
 * Replicated round state lives in AGSGameState.
 */
UCLASS()
class BANDITBALLER_API AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();
};
