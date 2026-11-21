// BanditBaller

#include "GSGameMode.h"
#include "BanditBaller.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSHealthComponent.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSRoundSettings.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

const FName AGSGameMode::TownReturnTag(TEXT("TownReturn"));
const FName AGSGameMode::MarketArrivalTag(TEXT("MarketArrival"));

namespace
{
	constexpr float TravelTickInterval = 0.05f;
}

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

// ---------------------------------------------------------------------------
// Area travel: Town <-> Market District
// ---------------------------------------------------------------------------

ULevelStreaming* AGSGameMode::GetMarketDistrictStreamingLevel() const
{
	const FString Wanted = MarketDistrictLevelName.ToString();
	for (ULevelStreaming* Streaming : GetWorld()->GetStreamingLevels())
	{
		// PIE adds a UEDPIE_N_ prefix to the package name, so compare the de-prefixed short name.
		if (Streaming && FPackageName::GetShortName(UWorld::RemovePIEPrefix(Streaming->GetWorldAssetPackageName())) == Wanted)
		{
			return Streaming;
		}
	}
	return nullptr;
}

AActor* AGSGameMode::FindTaggedActor(const UWorld* World, FName Tag)
{
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(Tag))
		{
			return *It;
		}
	}
	return nullptr;
}

void AGSGameMode::FadeAllPlayers(bool bFadeOut) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->ClientSetCameraFade(true, FColor::Black, bFadeOut ? FVector2D(0.f, 1.f) : FVector2D(1.f, 0.f),
				AreaFadeTime, /*bFadeAudio*/ false, /*bHoldWhenFinished*/ bFadeOut);
		}
	}
}

void AGSGameMode::MovePartyTo(const AActor& Destination) const
{
	const FVector Center = Destination.GetActorLocation();
	const FRotator Facing(0.f, Destination.GetActorRotation().Yaw, 0.f);

	TArray<APlayerController*> Players;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get() && It->Get()->GetPawn())
		{
			Players.Add(It->Get());
		}
	}

	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		APlayerController* PC = Players[Index];
		APawn* Pawn = PC->GetPawn();

		// First player on the point, the rest in a ring around it.
		FVector Location = Center;
		if (Index > 0)
		{
			const float Angle = 2.f * PI * (Index - 1) / FMath::Max(Players.Num() - 1, 1);
			Location += FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * PartySpawnSpacing;
		}

		if (UCharacterMovementComponent* CMC = Pawn->FindComponentByClass<UCharacterMovementComponent>())
		{
			CMC->StopMovementImmediately();
		}
		Pawn->TeleportTo(Location, Facing);
		PC->ClientSetRotation(Facing);
	}
}

bool AGSGameMode::TravelToMarketDistrict()
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	ULevelStreaming* Streaming = GetMarketDistrictStreamingLevel();
	if (!GS || IsAreaTravelInProgress() || GS->GetCurrentArea() == EGSArea::MarketDistrict)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("TravelToMarketDistrict: ignored (already there or travelling)."));
		return false;
	}
	if (!Streaming)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("TravelToMarketDistrict: this map has no '%s' streaming level."), *MarketDistrictLevelName.ToString());
		return false;
	}

	UE_LOG(LogBanditBaller, Log, TEXT("Market District: streaming in and teleporting the party..."));
	FadeAllPlayers(true);
	// Server-side streaming changes are pushed to clients (APlayerController::LevelStreamingStatusChanged).
	Streaming->SetShouldBeLoaded(true);
	Streaming->SetShouldBeVisible(true);

	TravelStartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(TravelTimer, this, &AGSGameMode::TickTravelToMarket, TravelTickInterval, true);
	return true;
}

void AGSGameMode::TickTravelToMarket()
{
	const double Elapsed = GetWorld()->GetTimeSeconds() - TravelStartTime;
	ULevelStreaming* Streaming = GetMarketDistrictStreamingLevel();

	// Wait for the fade to cover the screen and for the district to be visible on the server.
	// TODO(co-op): also wait for remote clients to report the level visible before moving them.
	if (Elapsed < AreaFadeTime || !Streaming || !Streaming->IsLevelVisible())
	{
		if (Elapsed > AreaStreamTimeout)
		{
			UE_LOG(LogBanditBaller, Error, TEXT("Market District: streaming timed out after %.1fs; staying in town."), Elapsed);
			FinishTravel();
		}
		return;
	}

	const AActor* Arrival = FindTaggedActor(GetWorld(), MarketArrivalTag);
	if (!Arrival)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("Market District: no actor tagged '%s'; staying in town."), *MarketArrivalTag.ToString());
		FinishTravel();
		return;
	}

	MovePartyTo(*Arrival);
	GetGameState<AGSGameState>()->SetCurrentArea(EGSArea::MarketDistrict);
	UE_LOG(LogBanditBaller, Log, TEXT("Market District: party arrived at %s (%.2fs)."), *Arrival->GetActorNameOrLabel(), Elapsed);
	FinishTravel();
}

bool AGSGameMode::ReturnToTown()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS || IsAreaTravelInProgress() || GS->GetCurrentArea() == EGSArea::Town)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("ReturnToTown: ignored (already in town or travelling)."));
		return false;
	}

	UE_LOG(LogBanditBaller, Log, TEXT("Market District: returning the party to town..."));
	FadeAllPlayers(true);
	TravelStartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(TravelTimer, this, &AGSGameMode::TickReturnToTown, TravelTickInterval, true);
	return true;
}

void AGSGameMode::TickReturnToTown()
{
	if (GetWorld()->GetTimeSeconds() - TravelStartTime < AreaFadeTime)
	{
		return;
	}

	if (const AActor* TownReturn = FindTaggedActor(GetWorld(), TownReturnTag))
	{
		MovePartyTo(*TownReturn);
	}
	else
	{
		UE_LOG(LogBanditBaller, Error, TEXT("ReturnToTown: no actor tagged '%s'; players stay where they are."), *TownReturnTag.ToString());
	}

	GetGameState<AGSGameState>()->SetCurrentArea(EGSArea::Town);
	if (ULevelStreaming* Streaming = GetMarketDistrictStreamingLevel())
	{
		Streaming->SetShouldBeVisible(false);
		Streaming->SetShouldBeLoaded(false);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("Market District: party back in town; district unloading."));
	FinishTravel();
}

void AGSGameMode::FinishTravel()
{
	GetWorldTimerManager().ClearTimer(TravelTimer);
	FadeAllPlayers(false);
}

// ---------------------------------------------------------------------------
// Round loop
// ---------------------------------------------------------------------------

namespace
{
	FString PhaseName(EGSRoundPhase Phase)
	{
		return StaticEnum<EGSRoundPhase>()->GetNameStringByValue(static_cast<int64>(Phase));
	}
}

void AGSGameMode::StartPlay()
{
	Super::StartPlay();

	if (!RoundSettings)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no RoundSettings; using UGSRoundSettings defaults."), *GetName());
	}
	if (bAutoStartRounds)
	{
		StartNextRound();
	}
}

const UGSRoundSettings& AGSGameMode::GetRoundSettings() const
{
	return RoundSettings ? *RoundSettings : *GetDefault<UGSRoundSettings>();
}

float AGSGameMode::GetPhaseDuration(EGSRoundPhase Phase) const
{
	const UGSRoundSettings& Settings = GetRoundSettings();
	switch (Phase)
	{
	case EGSRoundPhase::PreRound:   return Settings.PreRoundDuration;
	case EGSRoundPhase::Combat:     return Settings.CombatFallbackDuration;
	case EGSRoundPhase::RoundClear: return Settings.RoundClearDuration;
	default:                        return 0.f; // WaitingToStart, MarketDecision (instant), MarketVisit, GameOver
	}
}

void AGSGameMode::SetPhase(EGSRoundPhase NewPhase)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("SetPhase(%s): no AGSGameState."), *PhaseName(NewPhase));
		return;
	}

	GetWorldTimerManager().ClearTimer(PhaseTimer);

	const EGSRoundPhase OldPhase = GS->GetRoundPhase();
	const float Duration = GetPhaseDuration(NewPhase);
	const double EndTime = Duration > 0.f ? GS->GetServerWorldTimeSeconds() + Duration : 0.0;

	UE_LOG(LogBanditBaller, Log, TEXT("Round %d: %s -> %s (%s) at t=%.2fs"), GS->GetRoundNumber(), *PhaseName(OldPhase), *PhaseName(NewPhase),
		Duration > 0.f ? *FString::Printf(TEXT("%.1f s"), Duration) : TEXT("untimed"), GetWorld()->GetTimeSeconds());

	GS->SetRoundPhase(NewPhase, EndTime);
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &AGSGameMode::AdvancePhase, Duration, false);
	}

	// Phases that resolve immediately on entry.
	if (NewPhase == EGSRoundPhase::MarketDecision)
	{
		RunMarketDecision();
	}
}

void AGSGameMode::AdvancePhase()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS)
	{
		return;
	}

	switch (GS->GetRoundPhase())
	{
	case EGSRoundPhase::WaitingToStart: StartNextRound(); break;
	case EGSRoundPhase::PreRound:       SetPhase(EGSRoundPhase::Combat); break;
	case EGSRoundPhase::Combat:         SetPhase(EGSRoundPhase::RoundClear); break;
	case EGSRoundPhase::RoundClear:     SetPhase(EGSRoundPhase::MarketDecision); break;
	case EGSRoundPhase::MarketDecision: RunMarketDecision(); break;
	case EGSRoundPhase::MarketVisit:
		if (GS->GetCurrentArea() == EGSArea::MarketDistrict)
		{
			ReturnToTown();
		}
		StartNextRound();
		break;
	case EGSRoundPhase::GameOver:
		UE_LOG(LogBanditBaller, Log, TEXT("AdvancePhase: GameOver is final."));
		break;
	}
}

void AGSGameMode::RunMarketDecision()
{
	// For now the market never opens; set rounds / chance arrive in a later milestone.
	AGSGameState* GS = GetGameState<AGSGameState>();
	const bool bOpenMarket = false;
	GS->SetMarketOpenThisRound(bOpenMarket);
	UE_LOG(LogBanditBaller, Log, TEXT("Round %d: market decision -> %s"), GS->GetRoundNumber(), bOpenMarket ? TEXT("open") : TEXT("closed, next round"));

	if (bOpenMarket)
	{
		SetPhase(EGSRoundPhase::MarketVisit);
	}
	else
	{
		StartNextRound();
	}
}

void AGSGameMode::StartNextRound()
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS)
	{
		return;
	}
	GS->SetMarketOpenThisRound(false);
	GS->SetRoundNumber(GS->GetRoundNumber() + 1);
	SetPhase(EGSRoundPhase::PreRound);
}

void AGSGameMode::SetRound(int32 NewRound)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS || NewRound < 1)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.Round.Set: round must be >= 1."));
		return;
	}
	if (GS->GetCurrentArea() == EGSArea::MarketDistrict)
	{
		ReturnToTown();
	}
	GS->SetMarketOpenThisRound(false);
	GS->SetRoundNumber(NewRound);
	SetPhase(EGSRoundPhase::PreRound);
}
