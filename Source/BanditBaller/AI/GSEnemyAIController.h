// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GSEnemyAIController.generated.h"

class UAISenseConfig_Sight;
class UStateTreeAIComponent;

/**
 * Enemy brain: runs the enemy's StateTree (from its UGSEnemyData) and tracks the target.
 * Sight perception picks the player it can see; otherwise it falls back to the nearest
 * living player, since this is a horde game and enemies always know where you are.
 */
UCLASS()
class BANDITBALLER_API AGSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGSEnemyAIController();

	/** Current target: the perceived player if alive, else the nearest living player pawn. */
	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetTargetActor() const;

	UStateTreeAIComponent* GetStateTreeComponent() const { return StateTreeComponent; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	static bool IsValidTarget(const AActor* Actor);

	TWeakObjectPtr<AActor> PerceivedTarget;
};
