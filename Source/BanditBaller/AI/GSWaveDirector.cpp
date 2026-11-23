// BanditBaller

#include "GSWaveDirector.h"
#include "BanditBaller.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameMode.h"
#include "GSGameState.h"
#include "GSHealthComponent.h"
#include "GSRoundSettings.h"
#include "GSRunRandom.h"
#include "GSSpawnLocationFinder.h"
#include "GSSpawnTelegraph.h"
#include "GSWaveSettings.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarWaveMaxAlive(
		TEXT("gs.Wave.MaxAlive"), 0,
		TEXT("Override the wave director's max-enemies-alive cap (0 = use DA_WaveSettings)."));

	FString WaveEnemyShortName(const UGSEnemyData* Data)
	{
		FString Name = GetNameSafe(Data);
		Name.RemoveFromStart(TEXT("DA_Enemy_"));
		return Name;
	}

	/** "20 BeatCop, 10 PatrolOfficer" */
	FString DescribeWaveComposition(const TArray<TObjectPtr<UGSEnemyData>>& Enemies)
	{
		TMap<const UGSEnemyData*, int32> Counts;
		for (const UGSEnemyData* Data : Enemies)
		{
			++Counts.FindOrAdd(Data);
		}
		FString Out;
		for (const TPair<const UGSEnemyData*, int32>& Pair : Counts)
		{
			Out += FString::Printf(TEXT("%s%d %s"), Out.IsEmpty() ? TEXT("") : TEXT(", "), Pair.Value, *WaveEnemyShortName(Pair.Key));
		}
		return Out;
	}
}

UGSWaveDirector::UGSWaveDirector()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bTickEvenWhenPaused = false; // the game pause (upgrade choice) stops spawning
}

void UGSWaveDirector::BeginPlay()
{
	Super::BeginPlay();

	if (AGSGameMode* GameMode = Cast<AGSGameMode>(GetOwner()))
	{
		GameMode->OnEnemyKilled.AddDynamic(this, &UGSWaveDirector::HandleEnemyKilled);
	}
	if (!WavesTable)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("UGSWaveDirector on %s has no WavesTable (DT_Waves); every round will be generated."), *GetNameSafe(GetOwner()));
	}
	if (!WaveSettings)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("UGSWaveDirector on %s has no WaveSettings (DA_WaveSettings); generated waves have an empty pool."), *GetNameSafe(GetOwner()));
	}
}

const UGSWaveSettings& UGSWaveDirector::GetSettings() const
{
	return WaveSettings ? *WaveSettings : *GetDefault<UGSWaveSettings>();
}

const UGSRoundSettings& UGSWaveDirector::GetRoundSettings() const
{
	const AGSGameMode* GameMode = Cast<AGSGameMode>(GetOwner());
	return GameMode ? GameMode->GetRoundSettings() : *GetDefault<UGSRoundSettings>();
}

int32 UGSWaveDirector::GetMaxAlive() const
{
	const int32 Override = CVarWaveMaxAlive.GetValueOnGameThread();
	return Override > 0 ? Override : GetSettings().MaxAliveEnemies;
}

const FGSWaveRow* UGSWaveDirector::FindWaveRow(int32 Round) const
{
	if (!WavesTable)
	{
		return nullptr;
	}
	const FName RowName(*FString::Printf(TEXT("Round%d"), Round));
	return WavesTable->FindRow<FGSWaveRow>(RowName, TEXT("UGSWaveDirector"), /*bWarnIfRowMissing*/ false);
}

int32 UGSWaveDirector::GetLastTableRound() const
{
	int32 Last = 0;
	if (WavesTable)
	{
		for (const FName& Name : WavesTable->GetRowNames())
		{
			FString Text = Name.ToString();
			if (Text.RemoveFromStart(TEXT("Round")) && Text.IsNumeric())
			{
				Last = FMath::Max(Last, FCString::Atoi(*Text));
			}
		}
	}
	return Last;
}

float UGSWaveDirector::GetRangedShare(const TArray<UGSEnemyData*>& Enemies)
{
	int32 Ranged = 0;
	for (const UGSEnemyData* Data : Enemies)
	{
		Ranged += (Data && Data->bRanged) ? 1 : 0;
	}
	return Enemies.IsEmpty() ? 0.f : static_cast<float>(Ranged) / Enemies.Num();
}

void UGSWaveDirector::PurchaseWave(const UObject* WorldContextObject, const TArray<TObjectPtr<UGSEnemyData>>& Pool, float Budget, int32 Round,
	int32 MaxCount, TArray<UGSEnemyData*>& OutEnemies, float* OutBudgetLeft, float MaxRangedShare)
{
	PurchaseWaveFromStream(UGSRunRandom::GetStream(WorldContextObject), Pool, Budget, Round, MaxCount, OutEnemies, OutBudgetLeft, MaxRangedShare);
}

void UGSWaveDirector::PurchaseWaveFromStream(FRandomStream& Stream, const TArray<TObjectPtr<UGSEnemyData>>& Pool, float Budget, int32 Round,
	int32 MaxCount, TArray<UGSEnemyData*>& OutEnemies, float* OutBudgetLeft, float MaxRangedShare)
{
	OutEnemies.Reset();
	int32 RangedBought = 0;
	TArray<UGSEnemyData*> Unlocked;
	for (UGSEnemyData* Data : Pool)
	{
		if (Data && Data->MinRound <= Round && Data->SpawnCost > 0.f)
		{
			Unlocked.Add(Data);
		}
	}

	float Left = Budget;
	TArray<UGSEnemyData*> Affordable;
	while (OutEnemies.Num() < MaxCount)
	{
		// Ranged cap: one more ranged enemy must keep the wave at or under the share (the small margin keeps float noise
		// such as 0.4 x 5 = 2.0000001 from refusing an exact fit).
		const bool bRangedAllowed = MaxRangedShare >= 1.f
			|| static_cast<float>(RangedBought + 1) <= MaxRangedShare * static_cast<float>(OutEnemies.Num() + 1) + KINDA_SMALL_NUMBER;
		Affordable.Reset();
		for (UGSEnemyData* Data : Unlocked)
		{
			if (Data->SpawnCost <= Left + KINDA_SMALL_NUMBER && (bRangedAllowed || !Data->bRanged))
			{
				Affordable.Add(Data);
			}
		}
		if (Affordable.IsEmpty())
		{
			break;
		}
		UGSEnemyData* Pick = Affordable[Stream.RandRange(0, Affordable.Num() - 1)];
		OutEnemies.Add(Pick);
		RangedBought += Pick->bRanged ? 1 : 0;
		Left -= Pick->SpawnCost;
	}

	if (OutBudgetLeft)
	{
		*OutBudgetLeft = FMath::Max(0.f, Left);
	}
}

void UGSWaveDirector::BuildRoundWave(int32 Round, TArray<TObjectPtr<UGSEnemyData>>& OutQueue, FString* OutSource) const
{
	OutQueue.Reset();
	FRandomStream Stream = UGSRunRandom::MakeRoundStream(this, Round);
	const UGSWaveSettings& Settings = GetSettings();
	if (const FGSWaveRow* Row = FindWaveRow(Round))
	{
		for (const FGSWaveEntry& Entry : Row->Enemies)
		{
			for (int32 Index = 0; Entry.EnemyData && Index < Entry.Count; ++Index)
			{
				OutQueue.Add(Entry.EnemyData);
			}
		}
		if (OutSource)
		{
			*OutSource = FString::Printf(TEXT("DT_Waves Round%d"), Round);
		}
	}
	else
	{
		// Past the table: buy the wave from the round's budget.
		const float Budget = Settings.GetBudget(Round);
		float Left = 0.f;
		TArray<UGSEnemyData*> Bought;
		PurchaseWaveFromStream(Stream, Settings.EnemyPool, Budget, Round, Settings.MaxGeneratedEnemies, Bought, &Left, Settings.MaxRangedShare);
		OutQueue.Append(Bought);
		if (OutSource)
		{
			*OutSource = FString::Printf(TEXT("generated, budget %.1f, %.1f left, %.0f%% ranged (cap %.0f%%)"), Budget, Left,
				GetRangedShare(Bought) * 100.f, Settings.MaxRangedShare * 100.f);
		}
	}

	// Shuffle with the round's stream (Fisher-Yates) so the order is seeded too.
	for (int32 Index = OutQueue.Num() - 1; Index > 0; --Index)
	{
		OutQueue.Swap(Index, Stream.RandRange(0, Index));
	}
}

FGSRoundWave* UGSWaveDirector::FindRoundWaveMutable(int32 Round)
{
	return ActiveRounds.FindByPredicate([Round](const FGSRoundWave& Wave) { return Wave.Round == Round; });
}

const FGSRoundWave* UGSWaveDirector::FindRoundWave(int32 Round) const
{
	return ActiveRounds.FindByPredicate([Round](const FGSRoundWave& Wave) { return Wave.Round == Round; });
}

int32 UGSWaveDirector::GetNewestRound() const
{
	int32 Newest = 0;
	for (const FGSRoundWave& Wave : ActiveRounds)
	{
		Newest = FMath::Max(Newest, Wave.Round);
	}
	return Newest;
}

const FGSRoundWave* UGSWaveDirector::GetReportedRound() const
{
	const int32 Newest = GetNewestRound();
	return Newest > 0 ? FindRoundWave(Newest) : (LastFinished.Round > 0 ? &LastFinished : nullptr);
}

int32 UGSWaveDirector::GetTotal() const { const FGSRoundWave* W = GetReportedRound(); return W && IsWaveActive() ? W->Total : 0; }
int32 UGSWaveDirector::GetSpawned() const { const FGSRoundWave* W = GetReportedRound(); return W ? W->Spawned : 0; }
int32 UGSWaveDirector::GetKilled() const { const FGSRoundWave* W = GetReportedRound(); return W ? W->Killed : 0; }
int32 UGSWaveDirector::GetRemaining() const { const FGSRoundWave* W = IsWaveActive() ? GetReportedRound() : nullptr; return W ? W->GetRemaining() : 0; }
int32 UGSWaveDirector::GetFallbackSpawns() const { const FGSRoundWave* W = GetReportedRound(); return W ? W->FallbackSpawns : 0; }
int32 UGSWaveDirector::GetOnScreenSpawns() const { const FGSRoundWave* W = GetReportedRound(); return W ? W->OnScreenSpawns : 0; }
float UGSWaveDirector::GetClosestSpawnDistance() const { const FGSRoundWave* W = GetReportedRound(); return W ? W->GetClosestSpawnDistance() : 0.f; }
float UGSWaveDirector::GetFarthestSpawnDistance() const { const FGSRoundWave* W = GetReportedRound(); return W ? W->MaxSpawnDistance : 0.f; }

int32 UGSWaveDirector::GetAlive() const
{
	int32 Alive = 0;
	for (const FGSRoundWave& Wave : ActiveRounds)
	{
		Alive += Wave.GetAlive();
	}
	return Alive;
}

const TArray<TObjectPtr<UGSEnemyData>>& UGSWaveDirector::GetSpawnQueue() const
{
	static const TArray<TObjectPtr<UGSEnemyData>> Empty;
	const int32 Newest = GetNewestRound();
	const FGSRoundWave* Wave = Newest > 0 ? FindRoundWave(Newest) : nullptr;
	return Wave ? Wave->Queue : Empty;
}

bool UGSWaveDirector::IsRollOverReady(int32 Round) const
{
	const FGSRoundWave* Wave = FindRoundWave(Round);
	if (!Wave)
	{
		return false;
	}
	const bool bOneDied = Wave->Killed + Wave->Lost > 0 || Wave->Total == 0;
	return bOneDied && Wave->GetRemaining() <= FMath::Max(GetRoundSettings().RollOverRemaining, 0);
}

void UGSWaveDirector::StartRoundWave(int32 Round)
{
	if (FGSRoundWave* Existing = FindRoundWaveMutable(Round))
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d was already open; its wave starts over."), Round);
		ActiveRounds.RemoveAll([Round](const FGSRoundWave& Wave) { return Wave.Round == Round; });
	}

	FGSRoundWave& Wave = ActiveRounds.AddDefaulted_GetRef();
	Wave.Round = Round;
	FString Source;
	BuildRoundWave(Round, Wave.Queue, &Source);
	Wave.Total = Wave.Queue.Num();
	Wave.StartTime = GetWorld()->GetTimeSeconds();
	Wave.Gap = GetRoundSettings().GetSpawnGap(Wave.Total);
	Wave.NextSpawnAt = Wave.StartTime; // the first enemy is due at once
	bLoggedCap = false;

	UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d (%s): %d enemies (%s), one every %.2f s, max alive %d, telegraph %.2f s, %d round(s) open"),
		Round, *Source, Wave.Total, *DescribeWaveComposition(Wave.Queue), Wave.Gap, GetMaxAlive(), GetSettings().SpawnTelegraphTime, ActiveRounds.Num());
	if (Wave.Total == 0)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d has no enemies (empty row or pool / budget too small)."), Round);
	}

	SpawnDueEnemies();
	// An empty wave is finished at once; its clear is sent next tick (after the caller has recorded the round's start).
	CheckRound(*FindRoundWaveMutable(Round));
	PushCounts();
	UpdateTickEnabled();
}

void UGSWaveDirector::StopAll(bool bDespawnLeftovers)
{
	for (const FGSPendingSpawn& Pending : PendingSpawns)
	{
		if (AGSSpawnTelegraph* Telegraph = Pending.Telegraph.Get())
		{
			Telegraph->Destroy();
		}
	}
	const int32 Cancelled = PendingSpawns.Num();
	PendingSpawns.Reset();
	QueuedRollOvers.Reset();
	QueuedClears.Reset();

	for (FGSRoundWave& Wave : ActiveRounds)
	{
		int32 Removed = 0;
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Enemy : Wave.Enemies)
		{
			if (AGSEnemyCharacter* Alive = Enemy.Get())
			{
				Alive->OnDestroyed.RemoveDynamic(this, &UGSWaveDirector::HandleEnemyDestroyed);
				if (bDespawnLeftovers)
				{
					if (AController* Controller = Alive->GetController())
					{
						Controller->Destroy();
					}
					Alive->Destroy();
					++Removed;
				}
			}
		}
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d stopped after %.1f s: %d/%d spawned, %d killed, %d leftovers removed, %d never spawned."),
			Wave.Round, GetWorld()->GetTimeSeconds() - Wave.StartTime, Wave.Spawned, Wave.Total, Wave.Killed, Removed, Wave.Queue.Num() + Wave.Pending);
		LastFinished = Wave;
	}
	if (Cancelled > 0)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[wave] %d telegraphs cancelled."), Cancelled);
	}
	ActiveRounds.Reset();
	PushCounts();
	UpdateTickEnabled();
}

int32 UGSWaveDirector::ForceClearOpenRounds(const TCHAR* Why)
{
	if (ActiveRounds.IsEmpty())
	{
		return 0;
	}
	for (const FGSPendingSpawn& Pending : PendingSpawns)
	{
		if (AGSSpawnTelegraph* Telegraph = Pending.Telegraph.Get())
		{
			Telegraph->Destroy();
		}
	}
	PendingSpawns.Reset();

	const int32 Count = ActiveRounds.Num();
	for (FGSRoundWave& Wave : ActiveRounds)
	{
		int32 Removed = 0;
		const TArray<TWeakObjectPtr<AGSEnemyCharacter>> Leftovers = Wave.Enemies;
		Wave.Enemies.Reset();
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Enemy : Leftovers)
		{
			if (AGSEnemyCharacter* Alive = Enemy.Get())
			{
				Alive->OnDestroyed.RemoveDynamic(this, &UGSWaveDirector::HandleEnemyDestroyed);
				if (AController* Controller = Alive->GetController())
				{
					Controller->Destroy();
				}
				Alive->Destroy();
				++Removed;
			}
		}
		Wave.Lost += Removed + Wave.Queue.Num() + Wave.Pending;
		const int32 NeverSpawned = Wave.Queue.Num() + Wave.Pending;
		Wave.Queue.Reset();
		Wave.Pending = 0;
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d cleared by %s: %d leftovers removed, %d never spawned."), Wave.Round, Why, Removed, NeverSpawned);
		CheckRound(Wave);
	}
	FlushEvents();
	return Count;
}

void UGSWaveDirector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SpawnDueEnemies();
	ResolvePendingSpawns();
	FlushEvents();
}

void UGSWaveDirector::UpdateTickEnabled()
{
	SetComponentTickEnabled(!ActiveRounds.IsEmpty() || !PendingSpawns.IsEmpty());
}

void UGSWaveDirector::SpawnNextBatchNow()
{
	const int32 Newest = GetNewestRound();
	if (FGSRoundWave* Wave = Newest > 0 ? FindRoundWaveMutable(Newest) : nullptr)
	{
		if (!Wave->Queue.IsEmpty() && GetAlive() + GetPending() < GetMaxAlive())
		{
			const double Now = GetWorld()->GetTimeSeconds();
			SpawnOne(*Wave, Now);
			Wave->NextSpawnAt = Now + Wave->Gap;
		}
		ResolvePendingSpawns();
		FlushEvents();
	}
}

void UGSWaveDirector::SpawnDueEnemies()
{
	UWorld* World = GetWorld();
	if (!World || ActiveRounds.IsEmpty())
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	// Older rounds first (they were added in order).
	for (FGSRoundWave& Wave : ActiveRounds)
	{
		// A long frame can owe several spawns (each at its own due time); the guard stops a runaway.
		for (int32 Guard = 0; Guard < 64 && !Wave.Queue.IsEmpty() && Now >= Wave.NextSpawnAt; ++Guard)
		{
			if (GetAlive() + GetPending() >= GetMaxAlive())
			{
				if (!bLoggedCap)
				{
					bLoggedCap = true;
					UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d: alive cap %d reached; its spawns wait for room."), Wave.Round, GetMaxAlive());
				}
				break; // nothing banked: NextSpawnAt stays in the past, and the late rule below applies when room appears
			}
			const double Due = Wave.NextSpawnAt;
			if (!SpawnOne(Wave, Due))
			{
				break; // no living player to spawn around: try again next frame
			}
			// On schedule (or catching up within one gap): the next is one gap after this one's due time. Held back by
			// the cap for longer: the next is one gap after now.
			Wave.NextSpawnAt = (Now - Due <= Wave.Gap) ? Due + Wave.Gap : Now + Wave.Gap;
		}
	}
	ResolvePendingSpawns();
	UpdateTickEnabled();
}

bool UGSWaveDirector::SpawnOne(FGSRoundWave& Wave, double DueTime)
{
	UWorld* World = GetWorld();
	if (!World || Wave.Queue.IsEmpty())
	{
		return false;
	}
	// Spawn around a random living player.
	TArray<APlayerController*> Living;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
		const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Pawn);
		if (Pawn && (!Health || !Health->IsDead()))
		{
			Living.Add(It->Get());
		}
	}
	if (Living.IsEmpty())
	{
		return false;
	}
	APlayerController* Target = Living[UGSRunRandom::RandRange(this, 0, Living.Num() - 1)];
	const FVector PlayerLocation = Target->GetPawn()->GetActorLocation();
	const TArray<FGSSpawnPoint> Points = UGSSpawnLocationFinder::FindSpawnLocations(Target, 1);
	if (Points.IsEmpty())
	{
		return false;
	}
	const FGSSpawnPoint& Point = Points[0];
	const UGSWaveSettings& Settings = GetSettings();
	const double Now = World->GetTimeSeconds();

	FGSPendingSpawn& Pending = PendingSpawns.AddDefaulted_GetRef();
	Pending.EnemyData = Wave.Queue.Pop();
	Pending.Location = Point.Location;
	Pending.FaceTowards = PlayerLocation;
	Pending.SpawnAt = Now + Settings.SpawnTelegraphTime;
	Pending.Round = Wave.Round;
	++Wave.Pending;
	Wave.SpawnTimes.Add(Now);

	if (Settings.SpawnTelegraphTime > 0.f)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AGSSpawnTelegraph* Telegraph = World->SpawnActor<AGSSpawnTelegraph>(AGSSpawnTelegraph::StaticClass(), Point.Location, FRotator::ZeroRotator, Params))
		{
			Telegraph->Initialize(Settings.TelegraphMaterial, Settings.SpawnTelegraphTime);
			Pending.Telegraph = Telegraph;
		}
	}

	Wave.OnScreenSpawns += Point.bOnScreen ? 1 : 0;
	Wave.FallbackSpawns += Point.bFallback ? 1 : 0;
	Wave.MinSpawnDistance = FMath::Min(Wave.MinSpawnDistance, Point.DistanceToPlayer);
	Wave.MaxSpawnDistance = FMath::Max(Wave.MaxSpawnDistance, Point.DistanceToPlayer);
	UE_LOG(LogBanditBaller, Verbose, TEXT("[wave] Round %d t=%.2f s (due %.2f): telegraph %s %.1f m%s%s | %d/%d placed, alive %d (all rounds), pending %d"),
		Wave.Round, Now - Wave.StartTime, DueTime - Wave.StartTime, *WaveEnemyShortName(Pending.EnemyData), Point.DistanceToPlayer / 100.f,
		Point.bOnScreen ? TEXT(" ON-SCREEN") : TEXT(""), Point.bFallback ? TEXT(" FALLBACK") : TEXT(""), Wave.Total - Wave.Queue.Num(), Wave.Total,
		GetAlive(), GetPending());
	return true;
}

void UGSWaveDirector::ResolvePendingSpawns()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	for (int32 Index = 0; Index < PendingSpawns.Num();)
	{
		FGSPendingSpawn& Pending = PendingSpawns[Index];
		if (Pending.SpawnAt > Now)
		{
			++Index;
			continue;
		}
		if (AGSSpawnTelegraph* Telegraph = Pending.Telegraph.Get())
		{
			Telegraph->Destroy();
		}
		FGSRoundWave* Wave = FindRoundWaveMutable(Pending.Round);
		if (Wave)
		{
			--Wave->Pending;
			if (AGSEnemyCharacter* Enemy = AGSEnemyCharacter::SpawnEnemy(World, Pending.EnemyData, Pending.Location, Pending.FaceTowards, Wave->Round))
			{
				Enemy->OnDestroyed.AddDynamic(this, &UGSWaveDirector::HandleEnemyDestroyed);
				Wave->Enemies.Add(Enemy);
				++Wave->Spawned;
				Wave->PeakAlive = FMath::Max(Wave->PeakAlive, Wave->GetAlive());
			}
			else
			{
				Wave->Queue.Add(Pending.EnemyData); // retry when it is next due
			}
		}
		PendingSpawns.RemoveAt(Index);
	}
	PushCounts();
}

void UGSWaveDirector::CheckRound(FGSRoundWave& Wave)
{
	if (!Wave.bRollOverSent && IsRollOverReady(Wave.Round))
	{
		Wave.bRollOverSent = true;
		QueuedRollOvers.AddUnique(Wave.Round);
	}
	if (Wave.IsFinished())
	{
		QueuedClears.AddUnique(Wave.Round);
	}
}

void UGSWaveDirector::FlushEvents()
{
	if (bFlushing)
	{
		return; // a handler started a round mid-flush: the outer flush sends the rest
	}
	bFlushing = true;
	for (int32 Pass = 0; Pass < 16 && (QueuedClears.Num() > 0 || QueuedRollOvers.Num() > 0); ++Pass)
	{
		// Finished rounds leave first, so handlers see a consistent director (and may start the next round).
		const TArray<int32> Clears = MoveTemp(QueuedClears);
		const TArray<int32> RollOvers = MoveTemp(QueuedRollOvers);
		QueuedClears.Reset();
		QueuedRollOvers.Reset();
		for (const int32 Round : Clears)
		{
			if (const FGSRoundWave* Wave = FindRoundWave(Round))
			{
				LogRoundFinished(*Wave);
				LastFinished = *Wave;
				ActiveRounds.RemoveAll([Round](const FGSRoundWave& W) { return W.Round == Round; });
			}
		}
		PushCounts();
		UpdateTickEnabled();
		for (const int32 Round : Clears)
		{
			OnRoundCleared.Broadcast(Round);
		}
		for (const int32 Round : RollOvers)
		{
			OnRollOverReady.Broadcast(Round);
		}
	}
	bFlushing = false;
	PushCounts();
}

void UGSWaveDirector::LogRoundFinished(const FGSRoundWave& Wave) const
{
	UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d cleared in %.1f s: %d killed, %d lost, peak alive %d | spawns: one every %.2f s, closest %.1f m, on-screen %d, fallback %d"),
		Wave.Round, GetWorld()->GetTimeSeconds() - Wave.StartTime, Wave.Killed, Wave.Lost, Wave.PeakAlive, Wave.Gap,
		Wave.GetClosestSpawnDistance() / 100.f, Wave.OnScreenSpawns, Wave.FallbackSpawns);
}

void UGSWaveDirector::HandleEnemyKilled(AActor* Enemy, AActor* Killer)
{
	const AGSEnemyCharacter* EnemyCharacter = Cast<AGSEnemyCharacter>(Enemy);
	FGSRoundWave* Wave = EnemyCharacter ? FindRoundWaveMutable(EnemyCharacter->GetWaveRound()) : nullptr;
	if (Wave && Wave->Enemies.RemoveAll([Enemy](const TWeakObjectPtr<AGSEnemyCharacter>& E) { return E.Get() == Enemy; }) > 0)
	{
		++Wave->Killed;
		CheckRound(*Wave);
		FlushEvents();
	}
}

void UGSWaveDirector::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	// Still tracked means it left without dying (e.g. fell out of the world).
	const AGSEnemyCharacter* EnemyCharacter = Cast<AGSEnemyCharacter>(DestroyedActor);
	FGSRoundWave* Wave = EnemyCharacter ? FindRoundWaveMutable(EnemyCharacter->GetWaveRound()) : nullptr;
	if (Wave && Wave->Enemies.RemoveAll([DestroyedActor](const TWeakObjectPtr<AGSEnemyCharacter>& E) { return E.Get() == DestroyedActor; }) > 0)
	{
		++Wave->Lost;
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] %s (round %d) was removed without dying; not counted as a kill."), *GetNameSafe(DestroyedActor), Wave->Round);
		CheckRound(*Wave);
		FlushEvents();
	}
}

void UGSWaveDirector::PushCounts() const
{
	const UWorld* World = GetWorld();
	if (AGSGameState* GameState = World ? World->GetGameState<AGSGameState>() : nullptr)
	{
		GameState->SetWaveCounts(GetTotal(), GetRemaining(), GetAlive());
	}
}
