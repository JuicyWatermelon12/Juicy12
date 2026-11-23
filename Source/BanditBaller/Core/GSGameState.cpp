// BanditBaller

#include "GSGameState.h"
#include "Engine/Engine.h"
#include "GSGameMode.h"
#include "GSRunStats.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDebugRound(
		TEXT("gs.Debug.Round"), false,
		TEXT("Show the round number, phase and time left on screen (read from each machine's replicated AGSGameState)."));
}

AGSGameState::AGSGameState()
{
	// Tick only drives the gs.Debug.Round display.
	PrimaryActorTick.bCanEverTick = !UE_BUILD_SHIPPING;
	PrimaryActorTick.TickInterval = 0.f;

	RunStats = CreateDefaultSubobject<UGSRunStats>(TEXT("RunStats"));
}

void AGSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSGameState, RoundNumber);
	DOREPLIFETIME(AGSGameState, RoundPhase);
	DOREPLIFETIME(AGSGameState, PhaseEndTime);
	DOREPLIFETIME(AGSGameState, CurrentArea);
	DOREPLIFETIME(AGSGameState, RunSeed);
	DOREPLIFETIME(AGSGameState, LevelRewards);
	DOREPLIFETIME(AGSGameState, WaveEnemiesTotal);
	DOREPLIFETIME(AGSGameState, WaveEnemiesRemaining);
	DOREPLIFETIME(AGSGameState, WaveEnemiesAlive);
	DOREPLIFETIME(AGSGameState, EnemyHealthMultiplier);
	DOREPLIFETIME(AGSGameState, EnemyDamageMultiplier);
}

void AGSGameState::SetEnemyRoundMultipliers(float InHealth, float InDamage)
{
	if (HasAuthority())
	{
		EnemyHealthMultiplier = InHealth;
		EnemyDamageMultiplier = InDamage;
	}
}

void AGSGameState::SetWaveCounts(int32 InTotal, int32 InRemaining, int32 InAlive)
{
	if (HasAuthority() && (InTotal != WaveEnemiesTotal || InRemaining != WaveEnemiesRemaining || InAlive != WaveEnemiesAlive))
	{
		WaveEnemiesTotal = InTotal;
		WaveEnemiesRemaining = InRemaining;
		WaveEnemiesAlive = InAlive;
		OnWaveCountsChanged.Broadcast(WaveEnemiesTotal, WaveEnemiesRemaining, WaveEnemiesAlive);
	}
}

// ---------------------------------------------------------------------------
// Round
// ---------------------------------------------------------------------------

float AGSGameState::GetPhaseTimeRemaining() const
{
	if (PhaseEndTime <= 0.0)
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(PhaseEndTime - GetServerWorldTimeSeconds()));
}

void AGSGameState::SetRoundPhase(EGSRoundPhase NewPhase, double NewPhaseEndTime)
{
	if (!HasAuthority())
	{
		return;
	}
	const EGSRoundPhase OldPhase = RoundPhase;
	RoundPhase = NewPhase;
	PhaseEndTime = NewPhaseEndTime;
	// Broadcast even for a same-phase restart (e.g. gs.Round.Set restarting PreRound).
	OnPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void AGSGameState::SetRoundNumber(int32 NewRound)
{
	if (!HasAuthority() || NewRound == RoundNumber)
	{
		return;
	}
	RoundNumber = NewRound;
	OnRoundChanged.Broadcast(RoundNumber);
}

void AGSGameState::OnRep_RoundNumber()
{
	OnRoundChanged.Broadcast(RoundNumber);
}

void AGSGameState::OnRep_RoundPhase(EGSRoundPhase OldPhase)
{
	OnPhaseChanged.Broadcast(OldPhase, RoundPhase);
}

FString AGSGameState::GetRoundDebugString() const
{
	const FString Phase = StaticEnum<EGSRoundPhase>()->GetNameStringByValue(static_cast<int64>(RoundPhase));
	const FString Time = PhaseEndTime > 0.0 ? FString::Printf(TEXT("%.1f s"), GetPhaseTimeRemaining()) : TEXT("--");
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	const FString Loop = GameMode ? FString(TEXT(" | ")) + GameMode->DescribeRoundLoop() : FString();
	return FString::Printf(TEXT("Round %d | %s | %s | Enemies alive %d, newest round remaining %d/%d | Enemy HP x%.2f DMG x%.2f%s"), RoundNumber, *Phase, *Time,
		WaveEnemiesAlive, WaveEnemiesRemaining, WaveEnemiesTotal, EnemyHealthMultiplier, EnemyDamageMultiplier, *Loop);
}

void AGSGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	if (CVarDebugRound.GetValueOnGameThread() && GEngine)
	{
		const TCHAR* Who = HasAuthority() ? TEXT("Server") : TEXT("Client");
		const FString Area = StaticEnum<EGSArea>()->GetNameStringByValue(static_cast<int64>(CurrentArea));
		// Stable key per GameState so the line updates in place; one line per PIE instance.
		const int32 Key = 0x6A5000 + static_cast<int32>(GetUniqueID() & 0xFFF);
		GEngine->AddOnScreenDebugMessage(Key, 0.5f, FColor::Yellow,
			FString::Printf(TEXT("[%s] %s | Area: %s"), Who, *GetRoundDebugString(), *Area));
	}
#endif
}

// ---------------------------------------------------------------------------
// Area
// ---------------------------------------------------------------------------

void AGSGameState::SetCurrentArea(EGSArea NewArea)
{
	if (!HasAuthority() || NewArea == CurrentArea)
	{
		return;
	}
	const EGSArea OldArea = CurrentArea;
	CurrentArea = NewArea;
	OnAreaChanged.Broadcast(OldArea, NewArea);
}

void AGSGameState::OnRep_CurrentArea(EGSArea OldArea)
{
	OnAreaChanged.Broadcast(OldArea, CurrentArea);
}
