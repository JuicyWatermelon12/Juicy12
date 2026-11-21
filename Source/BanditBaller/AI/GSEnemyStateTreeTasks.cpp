// BanditBaller

#include "GSEnemyStateTreeTasks.h"
#include "AbilitySystemComponent.h"
#include "GSEnemyAIController.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameplayTags.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

namespace GSEnemyTasks
{
	AGSEnemyAIController* GetController(const FStateTreeExecutionContext& Context)
	{
		UObject* Owner = Context.GetOwner();
		if (AGSEnemyAIController* AIC = Cast<AGSEnemyAIController>(Owner))
		{
			return AIC;
		}
		if (const APawn* Pawn = Cast<APawn>(Owner))
		{
			return Cast<AGSEnemyAIController>(Pawn->GetController());
		}
		return nullptr;
	}

	AGSEnemyCharacter* GetEnemy(const AGSEnemyAIController* AIC)
	{
		return AIC ? Cast<AGSEnemyCharacter>(AIC->GetPawn()) : nullptr;
	}

	bool IsInAttackRange(const AGSEnemyCharacter& Enemy, const AActor& Target)
	{
		const UGSEnemyData* Data = Enemy.GetEnemyData();
		const float Range = Data ? Data->AttackRange : 150.f;
		return FVector::DistSquared2D(Enemy.GetActorLocation(), Target.GetActorLocation()) <= FMath::Square(Range);
	}

	FGameplayAbilitySpec* FindMeleeSpec(UAbilitySystemComponent& ASC)
	{
		for (FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(GSGameplayTags::Ability_Attack_Melee))
			{
				return &Spec;
			}
		}
		return nullptr;
	}
}

// ---------------------------------------------------------------------------
// Wait For Target
// ---------------------------------------------------------------------------

EStateTreeRunStatus FGSStateTreeTask_WaitForTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return Tick(Context, 0.f);
}

EStateTreeRunStatus FGSStateTreeTask_WaitForTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	return AIC && AIC->GetTargetActor() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

// ---------------------------------------------------------------------------
// Chase
// ---------------------------------------------------------------------------

EStateTreeRunStatus FGSStateTreeTask_ChaseTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.RepathTimer = 0.f;
	return Tick(Context, 0.f);
}

EStateTreeRunStatus FGSStateTreeTask_ChaseTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	AActor* Target = AIC ? AIC->GetTargetActor() : nullptr;
	if (!Enemy || !Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (GSEnemyTasks::IsInAttackRange(*Enemy, *Target))
	{
		AIC->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	Data.RepathTimer -= DeltaTime;
	if (Data.RepathTimer <= 0.f || AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		Data.RepathTimer = RepathInterval;

		const UGSEnemyData* EnemyData = Enemy->GetEnemyData();
		const float Acceptance = EnemyData ? EnemyData->AttackRange * 0.5f : 60.f;

		const EPathFollowingRequestResult::Type Result = AIC->MoveToActor(Target, Acceptance, true, /*bUsePathfinding*/ true, /*bCanStrafe*/ false);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			// No navmesh path (e.g. nav not built): walk straight at the target.
			AIC->MoveToActor(Target, Acceptance, true, /*bUsePathfinding*/ false, false);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FGSStateTreeTask_ChaseTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context))
	{
		AIC->StopMovement();
	}
}

// ---------------------------------------------------------------------------
// Melee attack
// ---------------------------------------------------------------------------

EStateTreeRunStatus FGSStateTreeTask_MeleeAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bStarted = false;

	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	AIC->StopMovement();

	FGameplayAbilitySpec* Spec = GSEnemyTasks::FindMeleeSpec(*ASC);
	if (!Spec || !ASC->TryActivateAbility(Spec->Handle))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.bStarted = true;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FGSStateTreeTask_MeleeAttack::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(GSEnemyTasks::GetController(Context));
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !Data.bStarted)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FGameplayAbilitySpec* Spec = GSEnemyTasks::FindMeleeSpec(*ASC);
	return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}
