// BanditBaller

#include "GSGameMode.h"
#include "BanditBaller.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSHealthComponent.h"
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

void AGSGameMode::HandleEnemyKilled(AActor* Enemy, AActor* Killer)
{
	UE_LOG(LogBanditBaller, Log, TEXT("OnEnemyKilled: %s killed by %s"), *GetNameSafe(Enemy), *GetNameSafe(Killer));
	OnEnemyKilled.Broadcast(Enemy, Killer);
}

void AGSGameMode::HandlePlayerDeath(APawn* Player, AActor* Killer)
{
	UE_LOG(LogBanditBaller, Log, TEXT("Player %s died (killer: %s)"), *GetNameSafe(Player), *GetNameSafe(Killer));
	OnPlayerDied.Broadcast(Player, Killer);

	if (AGSPlayerController* PC = Player ? Cast<AGSPlayerController>(Player->GetController()) : nullptr)
	{
		PC->ClientShowDeathScreen(DeathScreenWidgetClass);
	}
}

void AGSGameMode::RequestRestart(AController* Requester)
{
	// Only a dead player may restart. (Co-op will need "all players dead" instead.)
	const APawn* Pawn = Requester ? Requester->GetPawn() : nullptr;
	const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Pawn);
	if (!Health || !Health->IsDead())
	{
		return;
	}

	UE_LOG(LogBanditBaller, Log, TEXT("Restarting level (requested by %s)"), *GetNameSafe(Requester));
	// Reload the current map (AGameModeBase has no RestartGame()).
	GetWorld()->ServerTravel(TEXT("?Restart"));
}
