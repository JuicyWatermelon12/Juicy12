// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "GSEnemyStateTreeSchema.generated.h"

/**
 * StateTree schema for enemies: context actor = AGSEnemyCharacter, controller = AGSEnemyAIController.
 */
UCLASS(meta = (DisplayName = "BanditBaller Enemy AI"))
class BANDITBALLER_API UGSEnemyStateTreeSchema : public UStateTreeAIComponentSchema
{
	GENERATED_BODY()

public:
	UGSEnemyStateTreeSchema(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
