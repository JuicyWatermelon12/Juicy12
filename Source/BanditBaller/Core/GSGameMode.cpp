// BanditBaller

#include "GSGameMode.h"
#include "BanditBaller.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSHealthComponent.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSRoundSettings.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSRunStats.h"
#include "GSWaveDirector.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSCashPickupSubsystem.h"
#include "GSCashSettings.h"
#include "GSDifficultyCurve.h"
#include "GSGameplayTags.h"
#include "GSProgressionComponent.h"
#include "GSXPCurve.h"
#include "GSLevelRewards.h"

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

	WaveDirector = CreateDefaultSubobject<UGSWaveDirector>(TEXT("WaveDirector"));
}

namespace
{
	/** A player pawn or player state (damage instigators can be either). */
	bool IsPlayerActor(const AActor* Actor)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->IsPlayerControlled();
		}
		return Actor && Actor->IsA<APlayerState>();
	}

	/** The player state behind a killer (player pawn or player state), else nullptr. */
	AGSPlayerState* GetPlayerStateOf(AActor* Actor)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->IsPlayerControlled() ? Pawn->GetPlayerState<AGSPlayerState>() : nullptr;
		}
		return Cast<AGSPlayerState>(Actor);
	}

	/** "MeleeBandit" from DA_Enemy_MeleeBandit; class name for anything without enemy data. */
	FName GetEnemyTypeName(const AActor* Enemy)
	{
		if (const AGSEnemyCharacter* EnemyCharacter = Cast<AGSEnemyCharacter>(Enemy))
		{
			if (const UGSEnemyData* Data = EnemyCharacter->GetEnemyData())
			{
				FString Name = Data->GetName();
				Name.RemoveFromStart(TEXT("DA_Enemy_"));
				return FName(*Name);
			}
		}
		return Enemy ? Enemy->GetClass()->GetFName() : NAME_None;
	}
}

void AGSGameMode::HandleEnemyKilled(AActor* Enemy, AActor* Killer)
{
	UE_LOG(LogBanditBaller, Log, TEXT("OnEnemyKilled: %s killed by %s"), *GetNameSafe(Enemy), *GetNameSafe(Killer));

	// Run stats count enemies the party killed.
	if (UGSRunStats* Stats = GetRunStats(); Stats && IsPlayerActor(Killer))
	{
		Stats->RecordKill(GetEnemyTypeName(Enemy));
	}

	// Cash drop before the broadcast: if this kill clears the wave, RoundClear's vacuum includes it.
	const AGSEnemyCharacter* EnemyCharacter = Cast<AGSEnemyCharacter>(Enemy);
	const UGSEnemyData* EnemyData = EnemyCharacter ? EnemyCharacter->GetEnemyData() : nullptr;
	if (EnemyData && EnemyData->CashValue > 0 && CashSettings)
	{
		if (UGSCashPickupSubsystem* Cash = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>())
		{
			Cash->SpawnCashDrop(CashSettings, Enemy->GetActorLocation(), EnemyData->CashValue);
		}
	}

	// XP goes straight to the killing player (no pickup).
	if (EnemyData && EnemyData->XPValue > 0)
	{
		if (AGSPlayerState* KillerPS = GetPlayerStateOf(Killer))
		{
			AwardXP(KillerPS, static_cast<float>(EnemyData->XPValue));
		}
	}

	OnEnemyKilled.Broadcast(Enemy, Killer);
}

int32 AGSGameMode::AwardXP(AGSPlayerState* Player, float BaseXP, bool bApplyXPGain)
{
	UGSProgressionComponent* Progression = Player ? Player->GetProgressionComponent() : nullptr;
	if (!Progression || BaseXP <= 0.f)
	{
		return 0;
	}
	float Gain = 1.f;
	if (bApplyXPGain)
	{
		const UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
		Gain = ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetXPGainAttribute())) : 1.f;
	}
	const int32 Amount = FMath::RoundToInt(BaseXP * Gain);
	if (Amount <= 0)
	{
		return 0;
	}
	Progression->AddXP(Amount);
	if (UGSRunStats* Stats = GetRunStats())
	{
		Stats->AddXPEarned(Amount);
		Stats->RecordLevelReached(Progression->GetLevel());
	}
	return Amount;
}

void AGSGameMode::SetPlayerLevelForDebug(AGSPlayerState* Player, int32 Level)
{
	UGSProgressionComponent* Progression = Player ? Player->GetProgressionComponent() : nullptr;
	if (!Progression || !XPCurve)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.XP.SetLevel: no progression component or no XPCurve on the GameMode."));
		return;
	}
	const int32 Target = FMath::Clamp(Level, 1, XPCurve->GetMaxLevel());
	Progression->ResetProgression();
	const int64 Total = XPCurve->GetTotalXPToReachLevel(Target);
	if (Total > 0)
	{
		Progression->AddXP(static_cast<int32>(FMath::Min<int64>(Total, MAX_int32)));
	}
	if (UGSRunStats* Stats = GetRunStats())
	{
		Stats->RecordLevelReached(Progression->GetLevel());
	}
}

void AGSGameMode::AwardCash(APlayerController* Player, int32 Amount, const FVector& Location)
{
	AGSPlayerState* PS = Player ? Player->GetPlayerState<AGSPlayerState>() : nullptr;
	if (!PS || Amount <= 0)
	{
		return;
	}
	PS->AddCash(Amount);
	if (UGSRunStats* Stats = GetRunStats())
	{
		Stats->AddCashEarned(Amount);
	}
	if (AGSPlayerController* PC = Cast<AGSPlayerController>(Player))
	{
		PC->ClientNotifyCashCollected(Location, Amount);
	}
}

void AGSGameMode::HandleDamageApplied(AActor* Victim, AActor* DamageInstigator, float Amount)
{
	UGSRunStats* Stats = GetRunStats();
	if (!Stats || Amount <= 0.f)
	{
		return;
	}
	if (IsPlayerActor(Victim))
	{
		Stats->AddDamageTaken(Amount);
	}
	else if (IsPlayerActor(DamageInstigator))
	{
		Stats->AddDamageDealt(Amount);
	}
}

UGSRunStats* AGSGameMode::GetRunStats() const
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	return GS ? GS->GetRunStats() : nullptr;
}

// ---------------------------------------------------------------------------
// Run seed
// ---------------------------------------------------------------------------

int32 AGSGameMode::MakeRandomSeed()
{
	const uint64 Mixed = FPlatformTime::Cycles64() ^ static_cast<uint64>(FDateTime::Now().GetTicks());
	const int32 Seed = static_cast<int32>(GetTypeHash(Mixed) & 0x7FFFFFFF);
	return Seed != 0 ? Seed : 1;
}

void AGSGameMode::StartRun(int32 Seed, const TCHAR* Source)
{
	RunSeed = Seed;
	RunStream.Initialize(Seed);

	// A fresh run starts with no cash on the ground.
	if (UGSCashPickupSubsystem* Cash = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>())
	{
		Cash->ReleaseAll();
	}

	if (AGSGameState* GS = GetGameState<AGSGameState>())
	{
		GS->SetRunSeed(Seed);
		if (UGSRunStats* Stats = GS->GetRunStats())
		{
			Stats->ResetStats(GS->GetServerWorldTimeSeconds());
		}
		// Levels are per run.
		for (APlayerState* PS : GS->PlayerArray)
		{
			const AGSPlayerState* GSPS = Cast<AGSPlayerState>(PS);
			if (UGSProgressionComponent* Progression = GSPS ? GSPS->GetProgressionComponent() : nullptr)
			{
				Progression->ResetProgression();
			}
		}
	}
	UE_LOG(LogBanditBaller, Log, TEXT("Run started: seed %d (%s). Replay with -GSSeed=%d or gs.Run.Seed %d"), Seed, Source, Seed, Seed);
}

void AGSGameMode::RestartRunWithSeed(int32 Seed)
{
	StartRun(Seed, TEXT("gs.Run.Seed"));
	SetRound(1);
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

	// Run seed: -GSSeed=N on the command line, otherwise random. Logged so any run can be replayed.
	int32 Seed = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("GSSeed="), Seed))
	{
		StartRun(Seed, TEXT("-GSSeed"));
	}
	else
	{
		StartRun(MakeRandomSeed(), TEXT("random"));
	}

	WaveDirector->OnWaveCleared.AddUObject(this, &AGSGameMode::HandleWaveCleared);

	// Clients describe level-up rewards from this (HUD banner).
	if (AGSGameState* GS = GetGameState<AGSGameState>())
	{
		GS->SetLevelRewards(LevelRewards);
	}

	if (bAutoStartRounds)
	{
		StartNextRound();
	}
}

void AGSGameMode::HandleWaveCleared()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (GS && GS->GetRoundPhase() == EGSRoundPhase::Combat)
	{
		SetPhase(EGSRoundPhase::RoundClear);
	}
}

void AGSGameMode::HandlePhaseTimerExpired()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (GS && GS->GetRoundPhase() == EGSRoundPhase::Combat && WaveDirector->IsWaveActive())
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("Round %d: Combat safety timer (%.0f s) expired with %d enemies remaining (%d alive); ending combat anyway."),
			GS->GetRoundNumber(), GetPhaseDuration(EGSRoundPhase::Combat), WaveDirector->GetRemaining(), WaveDirector->GetAlive());
	}
	AdvancePhase();
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
	default:                        return 0.f; // WaitingToStart, GameOver
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

	// Publish this round's enemy multipliers (the round number may have changed).
	GS->SetEnemyRoundMultipliers(GetEnemyHealthMultiplier(GS->GetRoundNumber()), GetEnemyDamageMultiplier(GS->GetRoundNumber()));

	// Combat finished (timer, skip or, later, all enemies dead): the round counts as cleared.
	if (OldPhase == EGSRoundPhase::Combat && NewPhase == EGSRoundPhase::RoundClear)
	{
		if (UGSRunStats* Stats = GetRunStats())
		{
			Stats->RecordRoundCleared();
		}
	}
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &AGSGameMode::HandlePhaseTimerExpired, Duration, false);
		if (bRoundTimerPaused)
		{
			GetWorldTimerManager().PauseTimer(PhaseTimer);
		}
	}

	// Waves: Combat runs the round's wave; leaving Combat early (skip, safety timer) stops it and removes leftovers.
	if (OldPhase == EGSRoundPhase::Combat && NewPhase != EGSRoundPhase::Combat)
	{
		WaveDirector->StopWave(/*bDespawnLeftovers*/ true);
	}
	if (NewPhase == EGSRoundPhase::Combat)
	{
		WaveDirector->StartWave(GS->GetRoundNumber());
	}

	// Round won: every cash piece still on the ground flies to the party.
	if (NewPhase == EGSRoundPhase::RoundClear)
	{
		if (UGSCashPickupSubsystem* Cash = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>())
		{
			Cash->VacuumAll();
		}
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
	case EGSRoundPhase::RoundClear:     StartNextRound(); break;
	case EGSRoundPhase::GameOver:
		UE_LOG(LogBanditBaller, Log, TEXT("AdvancePhase: GameOver is final."));
		break;
	}
}

void AGSGameMode::StartNextRound()
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS)
	{
		return;
	}
	// Combat only happens in the town; bring the party back if a debug teleport (gs.Market.Teleport) left it away.
	if (GS->GetCurrentArea() == EGSArea::MarketDistrict)
	{
		ReturnToTown();
	}
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
	GS->SetRoundNumber(NewRound);
	SetPhase(EGSRoundPhase::PreRound);
}

// ---------------------------------------------------------------------------
// Enemy difficulty scaling
// ---------------------------------------------------------------------------

float AGSGameMode::GetEnemyHealthMultiplier(int32 Round) const
{
	return DifficultyCurve ? DifficultyCurve->GetHealthMultiplier(Round) : 1.f;
}

float AGSGameMode::GetEnemyDamageMultiplier(int32 Round) const
{
	return DifficultyCurve ? DifficultyCurve->GetDamageMultiplier(Round) : 1.f;
}

void AGSGameMode::ApplyEnemyRoundScaling(AGSEnemyCharacter& Enemy) const
{
	UAbilitySystemComponent* ASC = Enemy.GetAbilitySystemComponent();
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (!ASC || !GS)
	{
		return;
	}
	if (!EnemyRoundScalingEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no EnemyRoundScalingEffect; enemies are not scaled by round."), *GetName());
		return;
	}

	const int32 Round = FMath::Max(GS->GetRoundNumber(), 1);
	const float HealthMultiplier = GetEnemyHealthMultiplier(Round);
	const float DamageMultiplier = GetEnemyDamageMultiplier(Round);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EnemyRoundScalingEffect, static_cast<float>(Round), ASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_HealthMultiplier, HealthMultiplier);
	Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_DamageMultiplier, DamageMultiplier);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);

	// Spawn at full (scaled) health.
	ASC->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute()));
}

void AGSGameMode::SetRoundTimerPaused(bool bPaused)
{
	bRoundTimerPaused = bPaused;
	if (bPaused)
	{
		GetWorldTimerManager().PauseTimer(PhaseTimer);
	}
	else
	{
		GetWorldTimerManager().UnPauseTimer(PhaseTimer);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("gs.Round.Pause: phase timer %s"), bPaused ? TEXT("PAUSED") : TEXT("running"));
}
