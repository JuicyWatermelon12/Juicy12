// BanditBaller

#include "GSHubGameMode.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "GameplayEffect.h"
#include "GSCharacter.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
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

bool AGSHubGameMode::StartRun(APlayerController* Requester)
{
	if (bStartingRun)
	{
		return false;
	}
	if (RunMap.IsNull())
	{
		UE_LOG(LogBanditBaller, Error, TEXT("%s has no RunMap; can't start a run."), *GetName());
		return false;
	}
	bStartingRun = true;
	UE_LOG(LogBanditBaller, Log, TEXT("Hub: %s starts a run (%s, seed %s)."), *GetNameSafe(Requester), *RunMap.ToSoftObjectPath().GetLongPackageName(),
		NextRunSeed > 0 ? *FString::FromInt(NextRunSeed) : TEXT("random"));

	// Fade everyone out and stop them moving; the new map rebuilds the players.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->ClientSetCameraFade(true, FColor::Black, FVector2D(0.f, 1.f), StartRunFadeTime, /*bFadeAudio*/ true, /*bHoldWhenFinished*/ true);
			if (APawn* Pawn = PC->GetPawn())
			{
				Pawn->DisableInput(PC);
			}
		}
	}
	if (StartRunFadeTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(StartRunTimer, this, &AGSHubGameMode::OpenRunMap, StartRunFadeTime, false);
	}
	else
	{
		OpenRunMap();
	}
	return true;
}

void AGSHubGameMode::OpenRunMap()
{
	// Only what is chosen explicitly carries over: the seed as a URL option (the profile lives on the GameInstance).
	// TODO(co-op): ServerTravel for the whole party (M6).
	const FString Options = NextRunSeed > 0 ? FString::Printf(TEXT("%s=%d"), AGSGameMode::RunSeedOption, NextRunSeed) : FString();
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, RunMap, /*bAbsolute*/ true, Options);
}

void AGSHubGameMode::SetNextRunSeed(int32 Seed)
{
	NextRunSeed = FMath::Max(Seed, 0);
	UE_LOG(LogBanditBaller, Log, TEXT("Hub: the next run uses %s."), NextRunSeed > 0 ? *FString::Printf(TEXT("seed %d"), NextRunSeed) : TEXT("a random seed"));
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
