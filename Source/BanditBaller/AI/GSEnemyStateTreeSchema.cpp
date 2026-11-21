// BanditBaller

#include "GSEnemyStateTreeSchema.h"
#include "GSEnemyAIController.h"
#include "GSEnemyCharacter.h"

UGSEnemyStateTreeSchema::UGSEnemyStateTreeSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Base AI schema has [0] = context actor, [1] = AI controller.
	ContextActorClass = AGSEnemyCharacter::StaticClass();
	AIControllerClass = AGSEnemyAIController::StaticClass();

	if (ContextDataDescs.Num() >= 2)
	{
		ContextDataDescs[0].Struct = ContextActorClass.Get();
		ContextDataDescs[1].Struct = AIControllerClass.Get();
	}
}
