// BanditBaller

#include "GSGameMode.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"

AGSGameMode::AGSGameMode()
{
	// Native defaults; BP_GSGameMode overrides these with the Blueprint subclasses.
	GameStateClass = AGSGameState::StaticClass();
	PlayerStateClass = AGSPlayerState::StaticClass();
	PlayerControllerClass = AGSPlayerController::StaticClass();
	DefaultPawnClass = AGSCharacter::StaticClass();
}
