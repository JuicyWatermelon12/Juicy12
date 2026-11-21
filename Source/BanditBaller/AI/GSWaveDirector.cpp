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
#include "TimerManager.h"

namespace
{
	FString EnemyShortName(const UGSEnemyData* Data)
	{
		FString Name = GetNameSafe(Data);
		Name.RemoveFromStart(TEXT("DA_Enemy_"));
		return Name;
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
		UE_LOG(LogBanditBaller, Warning, TEXT("UGSWaveDirector on %s has no WavesTable (DT_Waves); combat will only end on the safety timer."), *GetNameSafe(GetOwner()));
	}
}

const FGSWaveRow* UGSWaveDirector::FindWaveRow(int32 Round, FName* OutRowName) const
{
	if (!WavesTable || WavesTable->GetRowMap().Num() == 0)
	{
		return nullptr;
	}

	const FName RowName(*FString::Printf(TEXT("Round%d"), Round));
	if (const FGSWaveRow* Row = WavesTable->FindRow<FGSWaveRow>(RowName, TEXT("UGSWaveDirector"), /*bWarnIfRowMissing*/ false))
	{
		if (OutRowName) { *OutRowName = RowName; }
		return Row;
	}

	// Past the end of the table: keep using the last row.
	const TArray<FName> Names = WavesTable->GetRowNames();
	if (OutRowName) { *OutRowName = Names.Last(); }
	return WavesTable->FindRow<FGSWaveRow>(Names.Last(), TEXT("UGSWaveDirector"));
}

void UGSWaveDirector::StartWave(int32 Round)
{
	StopWave(/*bDespawnLeftovers*/ true);

	WaveRound = Round;
	Spawned = Killed = Lost = 0;
	OnScreenSpawns = FallbackSpawns = 0;
	MinSpawnDistance = TNumericLimits<float>::Max();
	SpawnQueue.Reset();
	WaveEnemies.Reset();

	FName RowName;
	const FGSWaveRow* Row = FindWaveRow(Round, &RowName);
	if (!Row)
	{
		Total = 0;
		PushCounts();
		UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d: no DT_Waves row; nothing to spawn."), Round);
		return;
	}

	for (const FGSWaveEntry& Entry : Row->Enemies)
	{
		for (int32 Index = 0; Entry.EnemyData && Index < Entry.Count; ++Index)
		{
			SpawnQueue.Add(Entry.EnemyData);
		}
	}

	// Shuffle with the run stream (Fisher-Yates) so the order is seeded too.
	for (int32 Index = SpawnQueue.Num() - 1; Index > 0; --Index)
	{
		SpawnQueue.Swap(Index, UGSRunRandom::RandRange(this, 0, Index));
	}

	Total = SpawnQueue.Num();
	BatchSize = FMath::Max(1, Row->SpawnBatchSize);
	bWaveActive = Total > 0;
	WaveStartTime = GetWorld()->GetTimeSeconds();
	PushCounts();

	FString Composition;
	for (const FGSWaveEntry& Entry : Row->Enemies)
	{
		Composition += FString::Printf(TEXT("%s%d %s"), Composition.IsEmpty() ? TEXT("") : TEXT(", "), Entry.Count, *EnemyShortName(Entry.EnemyData));
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d (%s): %d enemies (%s), %d every %.1f s"),
		Round, *RowName.ToString(), Total, *Composition, BatchSize, Row->SpawnInterval);

	if (!bWaveActive)
	{
		return;
	}
	SpawnNextBatch();
	if (!SpawnQueue.IsEmpty())
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &UGSWaveDirector::SpawnNextBatch, Row->SpawnInterval, true);
	}
}

void UGSWaveDirector::StopWave(bool bDespawnLeftovers)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
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
	UE_LOG(LogBanditBaller, Warning, TEXT("[wave] Round %d stopped early after %.1f s: %d/%d spawned, %d killed, %d leftovers removed, %d never spawned."),
		WaveRound, GetWorld()->GetTimeSeconds() - WaveStartTime, Spawned, Total, Killed, Removed, SpawnQueue.Num());
	SpawnQueue.Reset();
	Total = Spawned = Killed = Lost = 0;
	PushCounts();
}

void UGSWaveDirector::SpawnNextBatch()
{
	UWorld* World = GetWorld();
	if (!bWaveActive || !World || SpawnQueue.IsEmpty())
	{
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

	const TArray<FGSSpawnPoint> Points = UGSSpawnLocationFinder::FindSpawnLocations(Target, FMath::Min(BatchSize, SpawnQueue.Num()));
	FString Batch;
	for (const FGSSpawnPoint& Point : Points)
	{
		UGSEnemyData* Data = SpawnQueue.Pop();
		AGSEnemyCharacter* Enemy = AGSEnemyCharacter::SpawnEnemy(World, Data, Point.Location, PlayerLocation);
		if (!Enemy)
		{
			SpawnQueue.Add(Data); // retry next batch
			continue;
		}
		Enemy->OnDestroyed.AddDynamic(this, &UGSWaveDirector::HandleEnemyDestroyed);
		WaveEnemies.Add(Enemy);
		++Spawned;

		OnScreenSpawns += Point.bOnScreen ? 1 : 0;
		FallbackSpawns += Point.bFallback ? 1 : 0;
		MinSpawnDistance = FMath::Min(MinSpawnDistance, Point.DistanceToPlayer);
		Batch += FString::Printf(TEXT("%s%s %.1f m%s%s"), Batch.IsEmpty() ? TEXT("") : TEXT(", "), *EnemyShortName(Data),
			Point.DistanceToPlayer / 100.f, Point.bOnScreen ? TEXT(" ON-SCREEN") : TEXT(""), Point.bFallback ? TEXT(" FALLBACK") : TEXT(""));
	}

	UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d t=%.1f s: +%d (%s) | spawned %d/%d, alive %d, remaining %d"),
		WaveRound, World->GetTimeSeconds() - WaveStartTime, Points.Num(), *Batch, Spawned, Total, GetAlive(), GetRemaining());

	if (SpawnQueue.IsEmpty())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
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
	if (!bWaveActive || !SpawnQueue.IsEmpty() || Spawned < Total || !WaveEnemies.IsEmpty())
	{
		return;
	}
	bWaveActive = false;
	UE_LOG(LogBanditBaller, Log, TEXT("[wave] Round %d cleared in %.1f s: %d killed, %d lost | spawns: closest %.1f m, on-screen %d, fallback %d"),
		WaveRound, GetWorld()->GetTimeSeconds() - WaveStartTime, Killed, Lost, MinSpawnDistance / 100.f, OnScreenSpawns, FallbackSpawns);
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
