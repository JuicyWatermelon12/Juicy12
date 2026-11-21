// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "GSEnemyStateTreeTasks.generated.h"

/*
 * Enemy StateTree tasks. They need no property bindings: each finds the AGSEnemyAIController
 * (the StateTree owner) and its pawn itself, so trees stay trivial to author.
 *
 *   Idle   (Wait For Target) --succeeded--> Chase
 *   Chase  (Chase Target)    --succeeded (in range)--> Attack, --failed--> Idle
 *   Attack (Melee Attack)    --completed--> Chase
 */

USTRUCT()
struct FGSStateTreeEmptyInstanceData
{
	GENERATED_BODY()
};

/** Running until the controller has a target, then Succeeded. */
USTRUCT(meta = (DisplayName = "GS Wait For Target", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeTask_WaitForTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeEmptyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FGSStateTreeChaseInstanceData
{
	GENERATED_BODY()

	/** Seconds until the move request is refreshed (target keeps moving). */
	UPROPERTY()
	float RepathTimer = 0.f;
};

/**
 * Moves to the target until within the enemy's AttackRange (Succeeded). Fails if the target is lost.
 * Falls back to direct movement if there is no navmesh path.
 */
USTRUCT(meta = (DisplayName = "GS Chase Target", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeTask_ChaseTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeChaseInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** How often the move target is refreshed while chasing. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float RepathInterval = 0.4f;
};

USTRUCT()
struct FGSStateTreeAttackInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bStarted = false;
};

/** Activates the enemy's Ability.Attack.Melee and stays Running until it ends (Succeeded). */
USTRUCT(meta = (DisplayName = "GS Melee Attack", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeTask_MeleeAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ---------------------------------------------------------------------------
// Ranged enemy
//
//   Idle  --succeeded--> Chase (Move To Firing Range) --succeeded--> Attack (Ranged Attack)
//   Attack --completed--> Reposition --completed--> Chase
//   Chase / Reposition --tick, Target Too Close--> Back Off --completed--> Attack
// ---------------------------------------------------------------------------

/**
 * Moves toward the target until within PreferredRangeMax with line of sight (Succeeded).
 * Without line of sight it keeps closing in. Fails if the target is lost.
 */
USTRUCT(meta = (DisplayName = "GS Move To Firing Range", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeTask_MoveToFiringRange : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeChaseInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float RepathInterval = 0.4f;
};

/** Activates the enemy's Ability.Attack.Ranged (GA_EnemyFire) and stays Running until it ends. */
USTRUCT(meta = (DisplayName = "GS Ranged Attack", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeTask_RangedAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FGSStateTreeMoveTimerInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	float Elapsed = 0.f;
};

/**
 * Strafes to a random reachable point around the target (RepositionAngleMin..Max around it, at
 * PreferredRangeMin..Max), facing the target. Succeeded on arrival or after RepositionMaxTime.
 */
USTRUCT(meta = (DisplayName = "GS Reposition Around Target", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeTask_Reposition : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeMoveTimerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/**
 * Backpedals away from the target (facing it) toward PreferredRangeMin.
 * Succeeded once far enough, on arrival, or after RetreatMaxTime.
 */
USTRUCT(meta = (DisplayName = "GS Back Off From Target", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeTask_BackOff : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeMoveTimerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/** True when the target is closer than the enemy's RetreatRange. */
USTRUCT(meta = (DisplayName = "GS Target Too Close", Category = "BanditBaller|Enemy"))
struct BANDITBALLER_API FGSStateTreeCondition_TargetTooClose : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeEmptyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
