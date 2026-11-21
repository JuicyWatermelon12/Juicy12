// BanditBaller

#include "GSGameState.h"
#include "Engine/Engine.h"
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
	DOREPLIFETIME(AGSGameState, bMarketOpenThisRound);
	DOREPLIFETIME(AGSGameState, RunSeed);
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
	return FString::Printf(TEXT("Round %d | %s | %s"), RoundNumber, *Phase, *Time);
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
			FString::Printf(TEXT("[%s] %s | Area: %s | Market open: %s"), Who, *GetRoundDebugString(), *Area,
				bMarketOpenThisRound ? TEXT("yes") : TEXT("no")));
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
