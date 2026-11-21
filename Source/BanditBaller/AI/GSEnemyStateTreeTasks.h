// BanditBaller

#pragma once

#include "CoreMinimal.h"
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
