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
#include "GSRunRandom.h"
#include "GSSpawnLocationFinder.h"
#include "GSSpawnTelegraph.h"
#include "GSWaveSettings.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarWaveMaxAlive(
		TEXT("gs.Wave.MaxAlive"), 0,
		TEXT("Override the wave director's max-enemies-alive cap (0 = use DA_WaveSettings)."));

	constexpr float PendingTickInterval = 0.05f;

	FString EnemyShortName(const UGSEnemyData* Data)
	{
		FString Name = GetNameSafe(Data);
		Name.RemoveFromStart(TEXT("DA_Enemy_"));
		return Name;
	}

	/** "20 BeatCop, 10 PatrolOfficer" */
	FString DescribeComposition(const TArray<TObjectPtr<UGSEnemyData>>& Enemies)
	{
		TMap<const UGSEnemyData*, int32> Counts;
		for (const UGSEnemyData* Data : Enemies)
		{
			++Counts.FindOrAdd(Data);
		}
		FString Out;
		for (const TPair<const UGSEnemyData*, int32>& Pair : Counts)
		{
			Out += FString::Printf(TEXT("%s%d %s"), Out.IsEmpty() ? TEXT("") : TEXT(", "), Pair.Value, *EnemyShortName(Pair.Key));
		}
		return Out;
	}
}

UGSWaveDirector::UGSWaveDirector()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	int32 RangedBought = 0;
	OutEnemies.Reset();
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
		UGSEnemyData* Pick = Affordable[UGSRunRandom::RandRange(WorldContextObject, 0, Affordable.Num() - 1)];
		OutEnemies.Add(Pick);
		RangedBought += Pick->bRanged ? 1 : 0;
		Left -= Pick->SpawnCost;
	}

	if (OutBudgetLeft)
	{
		*OutBudgetLeft = FMath::Max(0.f, Left);
	}
}

void UGSWaveDirector::StartWave(int32 Round)
{
	StopWave(/*bDespawnLeftovers*/ true);

	WaveRound = Round;
	Spawned = Killed = Lost = 0;
	OnScreenSpawns = FallbackSpawns = 0;
	PeakAlive = 0;
	bLoggedCapThisWave = false;
	MinSpawnDistance = TNumericLimits<float>::Max();
	MaxSpawnDistance = 0.f;
	SpawnQueue.Reset();
	WaveEnemies.Reset();

	const UGSWaveSettings& Settings = GetSettings();
	float Interval = Settings.GeneratedSpawnInterval;
	FString Source;

	if (const FGSWaveRow* Row = FindWaveRow(Round))
	{
		for (const FGSWaveEntry& Entry : Row->Enemies)
		{
			for (int32 Index = 0; Entry.EnemyData && Index < Entry.Count; ++Index)
			{
				SpawnQueue.Add(Entry.EnemyData);
			}
		}
		Interval = Row->SpawnInterval;
		BatchSize = FMath::Max(1, Row->SpawnBatchSize);
		Source = FString::Printf(TEXT("DT_Waves Round%d"), Round);
	}
	else
	{
		// Past the table: buy the wave from the round's budget.
		const float Budget = Settings.GetBudget(Round);
		float Left = 0.f;
		TArray<UGSEnemyData*> Bought;
		PurchaseWave(this, Settings.EnemyPool, Budget, Round, Settings.MaxGeneratedEnemies, Bought, &Left, Settings.MaxRangedShare);
		SpawnQueue.Append(Bought);
		BatchSize = FMath::Max(1, Settings.GeneratedBatchSize);
		Source = FString::Printf(TEXT("generated, budget %.1f, %.1f left, %.0f%% ranged (cap %.0f%%)"), Budget, Left,
			GetRangedShare(Bought) * 100.f, Settings.MaxRangedShare * 100.f);
	}

	// Shuffle with the run stream (Fisher-Yates) so the order is seeded too.
	for (int32 Index = SpawnQueue.Num() - 1; Index > 0; --Index)
	{
		SpawnQueue.Swap(Index, UGSRunRandom::RandRange(this, 0, Index));
	}

	Total = SpawnQueue.Num();
	bWaveActive = Total > 0;
	WaveStartTime = GetWorld()->GetTimeSeconds();
	PushCounts();

	UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d (%s): %d enemies (%s), %d every %.1f s, max alive %d, telegraph %.2f s"),
		Round, *Source, Total, *DescribeComposition(SpawnQueue), BatchSize, Interval, GetMaxAlive(), Settings.SpawnTelegraphTime);

	if (!bWaveActive)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d has no enemies (empty row or pool / budget too small)."), Round);
		return;
	}
	SpawnNextBatch();
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &UGSWaveDirector::SpawnNextBatch, Interval, true);
}

void UGSWaveDirector::ClearPendingSpawns()
{
	for (const FGSPendingSpawn& Pending : PendingSpawns)
	{
		if (AGSSpawnTelegraph* Telegraph = Pending.Telegraph.Get())
		{
			Telegraph->Destroy();
		}
	}
	PendingSpawns.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingTimer);
	}
}

void UGSWaveDirector::StopWave(bool bDespawnLeftovers)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
	const int32 Cancelled = PendingSpawns.Num();
	ClearPendingSpawns();
	if (!bWaveActive)
	{
		return;
	}
	bWaveActive = false;

	int32 Removed = 0;
	if (bDespawnLeftovers)
	{
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Leftovers = WaveEnemies;
		WaveEnemies.Reset();
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
	}
	UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d stopped early after %.1f s: %d/%d spawned, %d killed, %d leftovers removed, %d telegraphs cancelled, %d never spawned."),
		WaveRound, GetWorld()->GetTimeSeconds() - WaveStartTime, Spawned, Total, Killed, Removed, Cancelled, SpawnQueue.Num());
	SpawnQueue.Reset();
	Total = Spawned = Killed = Lost = 0;
	PushCounts();
}

void UGSWaveDirector::SpawnNextBatch()
{
	UWorld* World = GetWorld();
	if (!bWaveActive || !World)
	{
		return;
	}
	// Keep ticking even when the queue is empty: a failed spawn is re-queued and retried here.
	// The timer stops when the wave clears or is stopped.
	if (SpawnQueue.IsEmpty())
	{
		return;
	}

	// Alive cap: telegraphing spawns count as alive.
	const int32 Room = GetMaxAlive() - GetAlive() - GetPending();
	if (Room <= 0)
	{
		if (!bLoggedCapThisWave)
		{
			bLoggedCapThisWave = true;
			UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d t=%.1f s: alive cap %d reached; %d spawns waiting for room."),
				WaveRound, World->GetTimeSeconds() - WaveStartTime, GetMaxAlive(), SpawnQueue.Num());
		}
		return;
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
		return; // try again next interval
	}
	APlayerController* Target = Living[UGSRunRandom::RandRange(this, 0, Living.Num() - 1)];
	const FVector PlayerLocation = Target->GetPawn()->GetActorLocation();

	const UGSWaveSettings& Settings = GetSettings();
	const int32 Count = FMath::Min3(BatchSize, SpawnQueue.Num(), Room);
	const TArray<FGSSpawnPoint> Points = UGSSpawnLocationFinder::FindSpawnLocations(Target, Count);
	const double Now = World->GetTimeSeconds();

	FString Batch;
	for (const FGSSpawnPoint& Point : Points)
	{
		FGSPendingSpawn& Pending = PendingSpawns.AddDefaulted_GetRef();
		Pending.EnemyData = SpawnQueue.Pop();
		Pending.Location = Point.Location;
		Pending.FaceTowards = PlayerLocation;
		Pending.SpawnAt = Now + Settings.SpawnTelegraphTime;

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

		OnScreenSpawns += Point.bOnScreen ? 1 : 0;
		FallbackSpawns += Point.bFallback ? 1 : 0;
		MinSpawnDistance = FMath::Min(MinSpawnDistance, Point.DistanceToPlayer);
		MaxSpawnDistance = FMath::Max(MaxSpawnDistance, Point.DistanceToPlayer);
		Batch += FString::Printf(TEXT("%s%s %.1f m%s%s"), Batch.IsEmpty() ? TEXT("") : TEXT(", "), *EnemyShortName(Pending.EnemyData),
			Point.DistanceToPlayer / 100.f, Point.bOnScreen ? TEXT(" ON-SCREEN") : TEXT(""), Point.bFallback ? TEXT(" FALLBACK") : TEXT(""));
	}

	if (!Points.IsEmpty())
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d t=%.1f s: telegraph +%d (%s) | spawned %d/%d, alive %d, pending %d, remaining %d"),
			WaveRound, Now - WaveStartTime, Points.Num(), *Batch, Spawned, Total, GetAlive(), GetPending(), GetRemaining());
	}

	ResolvePendingSpawns(); // spawns immediately when the telegraph time is 0
	if (!PendingSpawns.IsEmpty() && !World->GetTimerManager().IsTimerActive(PendingTimer))
	{
		World->GetTimerManager().SetTimer(PendingTimer, this, &UGSWaveDirector::ResolvePendingSpawns, PendingTickInterval, true);
	}
	PushCounts();
}

void UGSWaveDirector::ResolvePendingSpawns()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();

	for (int32 Index = PendingSpawns.Num() - 1; Index >= 0; --Index)
	{
		FGSPendingSpawn& Pending = PendingSpawns[Index];
		if (Pending.SpawnAt > Now)
		{
			continue;
		}
		if (AGSSpawnTelegraph* Telegraph = Pending.Telegraph.Get())
		{
			Telegraph->Destroy();
		}

		if (AGSEnemyCharacter* Enemy = AGSEnemyCharacter::SpawnEnemy(World, Pending.EnemyData, Pending.Location, Pending.FaceTowards))
		{
			Enemy->OnDestroyed.AddDynamic(this, &UGSWaveDirector::HandleEnemyDestroyed);
			WaveEnemies.Add(Enemy);
			++Spawned;
			PeakAlive = FMath::Max(PeakAlive, GetAlive());
		}
		else
		{
			SpawnQueue.Add(Pending.EnemyData); // retry next batch
		}
		PendingSpawns.RemoveAtSwap(Index);
	}

	if (PendingSpawns.IsEmpty())
	{
		World->GetTimerManager().ClearTimer(PendingTimer);
	}
	PushCounts();
}

bool UGSWaveDirector::RemoveWaveEnemy(const AActor* Actor)
{
	return WaveEnemies.RemoveAll([Actor](const TWeakObjectPtr<AGSEnemyCharacter>& Enemy) { return Enemy.Get() == Actor; }) > 0;
}

void UGSWaveDirector::HandleEnemyKilled(AActor* Enemy, AActor* Killer)
{
	if (bWaveActive && RemoveWaveEnemy(Enemy))
	{
		++Killed;
		PushCounts();
		CheckCleared();
	}
}

void UGSWaveDirector::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	// Still tracked means it left without dying (e.g. fell out of the world).
	if (bWaveActive && RemoveWaveEnemy(DestroyedActor))
	{
		++Lost;
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] %s was removed without dying; not counted as a kill."), *GetNameSafe(DestroyedActor));
		PushCounts();
		CheckCleared();
	}
}

void UGSWaveDirector::CheckCleared()
{
	if (!bWaveActive || !SpawnQueue.IsEmpty() || !PendingSpawns.IsEmpty() || Spawned < Total || !WaveEnemies.IsEmpty())
	{
		return;
	}
	bWaveActive = false;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
	UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d cleared in %.1f s: %d killed, %d lost, peak alive %d | spawns: closest %.1f m, on-screen %d, fallback %d"),
		WaveRound, GetWorld()->GetTimeSeconds() - WaveStartTime, Killed, Lost, PeakAlive, MinSpawnDistance / 100.f, OnScreenSpawns, FallbackSpawns);
	OnWaveCleared.Broadcast();
}

void UGSWaveDirector::PushCounts() const
{
	const UWorld* World = GetWorld();
	if (AGSGameState* GameState = World ? World->GetGameState<AGSGameState>() : nullptr)
	{
		GameState->SetWaveCounts(Total, GetRemaining(), GetAlive());
	}
}
