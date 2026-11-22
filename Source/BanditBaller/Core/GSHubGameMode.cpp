// BanditBaller

#include "GSHubGameMode.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "GameplayEffect.h"
#include "GSCharacter.h"
#include "GSGameplayTags.h"
#include "GSGameState.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"

AGSHubGameMode::AGSHubGameMode()
{
	// Native defaults; BP_GSHubGameMode uses the same Blueprint subclasses as BP_GSGameMode.
	GameStateClass = AGSGameState::StaticClass();
	PlayerStateClass = AGSPlayerState::StaticClass();
	PlayerControllerClass = AGSPlayerController::StaticClass();
	DefaultPawnClass = AGSCharacter::StaticClass();
}

void AGSHubGameMode::StartPlay()
{
	Super::StartPlay();

	// The hub is the safe Market District: no round loop runs here.
	if (AGSGameState* GS = GetGameState<AGSGameState>())
	{
		GS->SetCurrentArea(EGSArea::MarketDistrict);
	}
	if (!SafeZoneEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no SafeZoneEffect; players can shoot and take damage in the hub."), *GetName());
	}
	UE_LOG(LogBanditBaller, Log, TEXT("Hub started (no round loop; area MarketDistrict)."));
}

void AGSHubGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer); // spawns the pawn (abilities + stats on possess)
	ApplySafeZone(NewPlayer);
}

void AGSHubGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	ApplySafeZone(NewPlayer);
}

void AGSHubGameMode::ApplySafeZone(AController* Player) const
{
	const AGSPlayerState* PS = Player ? Player->GetPlayerState<AGSPlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !SafeZoneEffect || ASC->HasMatchingGameplayTag(GSGameplayTags::State_SafeZone))
	{
		return;
	}
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SafeZoneEffect, 1.f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		UE_LOG(LogBanditBaller, Log, TEXT("Hub: %s is in the safe zone (no firing, no damage)."), *GetNameSafe(Player));
	}
}
