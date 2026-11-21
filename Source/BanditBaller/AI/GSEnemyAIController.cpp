// BanditBaller

#include "GSEnemyAIController.h"
#include "BanditBaller.h"
#include "Components/StateTreeAIComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameState.h"
#include "GSHealthComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AGSEnemyAIController::AGSEnemyAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	// The StateTree comes from the possessed enemy's data asset, so start it manually.
	StateTreeComponent->SetStartLogicAutomatically(false);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2500.f;
	SightConfig->LoseSightRadius = 3000.f;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	Perception->ConfigureSense(*SightConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*Perception);

	bWantsPlayerState = false;
}

void AGSEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	GetPerceptionComponent()->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AGSEnemyAIController::OnTargetPerceptionUpdated);

	const AGSEnemyCharacter* Enemy = Cast<AGSEnemyCharacter>(InPawn);
	const UGSEnemyData* Data = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!Data)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s possessed %s without enemy data; AI not started."), *GetName(), *GetNameSafe(InPawn));
		return;
	}

	// Sight from data.
	SightConfig->SightRadius = Data->SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(Data->LoseSightRadius, Data->SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = Data->PeripheralVisionAngle;
	GetPerceptionComponent()->ConfigureSense(*SightConfig);

	if (Data->StateTree)
	{
		StateTreeComponent->SetStateTree(Data->StateTree);
		StateTreeComponent->StartLogic();
	}
	else
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s: %s has no StateTree."), *GetName(), *GetNameSafe(Data));
	}
}

void AGSEnemyAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("Unpossessed"));
	Super::OnUnPossess();
}

void AGSEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	const APawn* SeenPawn = Cast<APawn>(Actor);
	if (!SeenPawn || !SeenPawn->IsPlayerControlled())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		PerceivedTarget = Actor;
	}
	else if (PerceivedTarget.Get() == Actor)
	{
		PerceivedTarget.Reset();
	}
}

bool AGSEnemyAIController::IsValidTarget(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}
	// Combat only happens in the town: while the party is in the Market District, nobody is a target.
	const AGSGameState* GameState = Actor->GetWorld() ? Actor->GetWorld()->GetGameState<AGSGameState>() : nullptr;
	if (GameState && GameState->GetCurrentArea() != EGSArea::Town)
	{
		return false;
	}
	const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Actor);
	return !Health || !Health->IsDead();
}

AActor* AGSEnemyAIController::GetTargetActor() const
{
	if (IsValidTarget(PerceivedTarget.Get()))
	{
		return PerceivedTarget.Get();
	}

	// Horde fallback: always know where the nearest living player is.
	const APawn* Self = GetPawn();
	if (!Self)
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* PlayerPawn = It->Get() ? It->Get()->GetPawn() : nullptr;
		if (!IsValidTarget(PlayerPawn))
		{
			continue;
		}

		const double DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), Self->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = PlayerPawn;
		}
	}
	return Best;
}
