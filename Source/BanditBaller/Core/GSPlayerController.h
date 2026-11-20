// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "BanditBallerPlayerController.h"
#include "GSPlayerController.generated.h"

/**
 * Game player controller. Inherits input-mapping setup from the template controller.
 */
UCLASS()
class BANDITBALLER_API AGSPlayerController : public ABanditBallerPlayerController
{
	GENERATED_BODY()
};
