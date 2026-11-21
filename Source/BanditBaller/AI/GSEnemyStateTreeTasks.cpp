// BanditBaller

#include "GSEnemyStateTreeTasks.h"
#include "AbilitySystemComponent.h"
#include "GSEnemyAIController.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
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

	FGameplayAbilitySpec* FindAttackSpec(UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag)
	{
		for (FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(AbilityTag))
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

	FGameplayAbilitySpec* Spec = GSEnemyTasks::FindAttackSpec(*ASC, GSGameplayTags::Ability_Attack_Melee);
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

	const FGameplayAbilitySpec* Spec = GSEnemyTasks::FindAttackSpec(*ASC, GSGameplayTags::Ability_Attack_Melee);
	return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

// ---------------------------------------------------------------------------
// Ranged helpers
// ---------------------------------------------------------------------------

namespace GSEnemyTasks
{
	/** Eye-to-target trace on Visibility. Other enemies in the way block line of sight. */
	bool HasLineOfSight(const AGSEnemyCharacter& Enemy, const AActor& Target)
	{
		const UWorld* World = Enemy.GetWorld();
		if (!World)
		{
			return false;
		}
		const FVector Eye = Enemy.GetActorLocation() + FVector(0.f, 0.f, 60.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSEnemyLOS), false, &Enemy);
		FHitResult Hit;
		if (!World->LineTraceSingleByObjectType(Hit, Eye, Target.GetActorLocation(), AGSEnemyCharacter::GetShotObjectQueryParams(), Params))
		{
			return true;
		}
		return Hit.GetActor() == &Target;
	}

	/** Face the target while moving (strafe / backpedal) instead of turning toward the move direction. */
	void SetStrafing(AGSEnemyAIController& AIC, AGSEnemyCharacter& Enemy, AActor* FocusTarget)
	{
		UCharacterMovementComponent* CMC = Enemy.GetCharacterMovement();
		if (FocusTarget)
		{
			AIC.SetFocus(FocusTarget);
			CMC->bOrientRotationToMovement = false;
			CMC->bUseControllerDesiredRotation = true;
		}
		else
		{
			AIC.ClearFocus(EAIFocusPriority::Gameplay);
			CMC->bOrientRotationToMovement = true;
			CMC->bUseControllerDesiredRotation = false;
		}
	}

	/** Point on the navmesh near Desired, or false if none. */
	bool ProjectToNav(const UWorld& World, const FVector& Desired, FVector& OutLocation)
	{
		const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
		FNavLocation NavLocation;
		if (NavSys && NavSys->ProjectPointToNavigation(Desired, NavLocation, FVector(300.f, 300.f, 500.f)))
		{
			OutLocation = NavLocation.Location;
			return true;
		}
		return false;
	}

	/** Point at Radius from Target's location, at YawDegrees around it. */
	FVector PointAroundTarget(const AActor& Target, float YawDegrees, float Radius)
	{
		const float Rad = FMath::DegreesToRadians(YawDegrees);
		return Target.GetActorLocation() + FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f) * Radius;
	}

	bool MoveTo(AGSEnemyAIController& AIC, const FVector& Location)
	{
		EPathFollowingRequestResult::Type Result = AIC.MoveToLocation(Location, 50.f, false, /*bUsePathfinding*/ true, false, /*bCanStrafe*/ true);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			Result = AIC.MoveToLocation(Location, 50.f, false, /*bUsePathfinding*/ false, false, true);
		}
		return Result != EPathFollowingRequestResult::Failed;
	}

	EStateTreeRunStatus StartAttackAbility(FStateTreeExecutionContext& Context, bool& bOutStarted, const FGameplayTag& AbilityTag)
	{
		bOutStarted = false;
		AGSEnemyAIController* AIC = GetController(Context);
		AGSEnemyCharacter* Enemy = GetEnemy(AIC);
		UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (!ASC)
		{
			return EStateTreeRunStatus::Failed;
		}

		AIC->StopMovement();
		FGameplayAbilitySpec* Spec = FindAttackSpec(*ASC, AbilityTag);
		if (!Spec || !ASC->TryActivateAbility(Spec->Handle))
		{
			return EStateTreeRunStatus::Failed;
		}
		bOutStarted = true;
		return EStateTreeRunStatus::Running;
	}

	EStateTreeRunStatus TickAttackAbility(FStateTreeExecutionContext& Context, bool bStarted, const FGameplayTag& AbilityTag)
	{
		AGSEnemyCharacter* Enemy = GetEnemy(GetController(Context));
		UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (!ASC || !bStarted)
		{
			return EStateTreeRunStatus::Failed;
		}
		const FGameplayAbilitySpec* Spec = FindAttackSpec(*ASC, AbilityTag);
		return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
	}
}

// ---------------------------------------------------------------------------
// Move To Firing Range
// ---------------------------------------------------------------------------

EStateTreeRunStatus FGSStateTreeTask_MoveToFiringRange::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetInstanceData(*this).RepathTimer = 0.f;
	return Tick(Context, 0.f);
}

EStateTreeRunStatus FGSStateTreeTask_MoveToFiringRange::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	AActor* Target = AIC ? AIC->GetTargetActor() : nullptr;
	const UGSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!Enemy || !Target || !EnemyData)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float Dist = FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation());
	const bool bLineOfSight = GSEnemyTasks::HasLineOfSight(*Enemy, *Target);
	if (Dist <= EnemyData->PreferredRangeMax && bLineOfSight)
	{
		AIC->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	Data.RepathTimer -= DeltaTime;
	if (Data.RepathTimer <= 0.f || AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		Data.RepathTimer = RepathInterval;

		// With sight, stop inside the preferred band; without it, keep closing in.
		const float Acceptance = bLineOfSight ? EnemyData->PreferredRangeMax * 0.8f : 200.f;
		if (AIC->MoveToActor(Target, Acceptance, false, true, false) == EPathFollowingRequestResult::Failed)
		{
			AIC->MoveToActor(Target, Acceptance, false, false, false);
		}
	}
	return EStateTreeRunStatus::Running;
}

void FGSStateTreeTask_MoveToFiringRange::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context))
	{
		AIC->StopMovement();
	}
}

// ---------------------------------------------------------------------------
// Ranged attack
// ---------------------------------------------------------------------------

EStateTreeRunStatus FGSStateTreeTask_RangedAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	return GSEnemyTasks::StartAttackAbility(Context, Data.bStarted, GSGameplayTags::Ability_Attack_Ranged);
}

EStateTreeRunStatus FGSStateTreeTask_RangedAttack::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	return GSEnemyTasks::TickAttackAbility(Context, Data.bStarted, GSGameplayTags::Ability_Attack_Ranged);
}

void FGSStateTreeTask_RangedAttack::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Leaving early (e.g. tree stopped): cancel the burst.
	AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(GSEnemyTasks::GetController(Context));
	if (UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr)
	{
		const FGameplayTagContainer Tags(GSGameplayTags::Ability_Attack_Ranged);
		ASC->CancelAbilities(&Tags);
	}
}

// ---------------------------------------------------------------------------
// Reposition
// ---------------------------------------------------------------------------

EStateTreeRunStatus FGSStateTreeTask_Reposition::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.f;

	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	AActor* Target = AIC ? AIC->GetTargetActor() : nullptr;
	const UGSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;
	const UWorld* World = Enemy ? Enemy->GetWorld() : nullptr;
	if (!Enemy || !Target || !EnemyData || !World)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Current bearing from the target, then step around it by a random angle (random side first).
	const FVector FromTarget = Enemy->GetActorLocation() - Target->GetActorLocation();
	const float CurrentYaw = FMath::RadiansToDegrees(FMath::Atan2(FromTarget.Y, FromTarget.X));
	const float Step = FMath::FRandRange(EnemyData->RepositionAngleMin, EnemyData->RepositionAngleMax);
	const float Radius = FMath::FRandRange(EnemyData->PreferredRangeMin, FMath::Max(EnemyData->PreferredRangeMin, EnemyData->PreferredRangeMax * 0.9f));
	const float Side = FMath::RandBool() ? 1.f : -1.f;

	for (const float Sign : { Side, -Side })
	{
		FVector Goal;
		if (GSEnemyTasks::ProjectToNav(*World, GSEnemyTasks::PointAroundTarget(*Target, CurrentYaw + Sign * Step, Radius), Goal)
			&& GSEnemyTasks::MoveTo(*AIC, Goal))
		{
			GSEnemyTasks::SetStrafing(*AIC, *Enemy, Target);
			return EStateTreeRunStatus::Running;
		}
	}

	// Nowhere to go: just shoot again.
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FGSStateTreeTask_Reposition::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	const AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	const UGSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!EnemyData || !AIC->GetTargetActor())
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed += DeltaTime;
	if (Data.Elapsed >= EnemyData->RepositionMaxTime || AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

void FGSStateTreeTask_Reposition::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	if (AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC))
	{
		AIC->StopMovement();
		GSEnemyTasks::SetStrafing(*AIC, *Enemy, nullptr);
	}
}

// ---------------------------------------------------------------------------
// Back off
// ---------------------------------------------------------------------------

EStateTreeRunStatus FGSStateTreeTask_BackOff::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.f;

	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	AActor* Target = AIC ? AIC->GetTargetActor() : nullptr;
	const UGSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;
	const UWorld* World = Enemy ? Enemy->GetWorld() : nullptr;
	if (!Enemy || !Target || !EnemyData || !World)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Straight away from the target first, then angled escapes if that is blocked.
	const FVector FromTarget = Enemy->GetActorLocation() - Target->GetActorLocation();
	const float AwayYaw = FMath::RadiansToDegrees(FMath::Atan2(FromTarget.Y, FromTarget.X));
	for (const float Offset : { 0.f, 45.f, -45.f, 90.f, -90.f })
	{
		FVector Goal;
		if (GSEnemyTasks::ProjectToNav(*World, GSEnemyTasks::PointAroundTarget(*Target, AwayYaw + Offset, EnemyData->PreferredRangeMin), Goal)
			&& GSEnemyTasks::MoveTo(*AIC, Goal))
		{
			GSEnemyTasks::SetStrafing(*AIC, *Enemy, Target);
			return EStateTreeRunStatus::Running;
		}
	}
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FGSStateTreeTask_BackOff::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	const AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	const AActor* Target = AIC ? AIC->GetTargetActor() : nullptr;
	const UGSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!EnemyData || !Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed += DeltaTime;
	const float Dist = FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation());
	if (Dist >= EnemyData->PreferredRangeMin * 0.9f || Data.Elapsed >= EnemyData->RetreatMaxTime
		|| AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

void FGSStateTreeTask_BackOff::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	if (AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC))
	{
		AIC->StopMovement();
		GSEnemyTasks::SetStrafing(*AIC, *Enemy, nullptr);
	}
}

// ---------------------------------------------------------------------------
// Target too close
// ---------------------------------------------------------------------------

bool FGSStateTreeCondition_TargetTooClose::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AGSEnemyAIController* AIC = GSEnemyTasks::GetController(Context);
	const AGSEnemyCharacter* Enemy = GSEnemyTasks::GetEnemy(AIC);
	const AActor* Target = AIC ? AIC->GetTargetActor() : nullptr;
	const UGSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!EnemyData || !Target)
	{
		return false;
	}
	return FVector::DistSquared2D(Enemy->GetActorLocation(), Target->GetActorLocation()) < FMath::Square(EnemyData->RetreatRange);
}
