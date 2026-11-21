// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Shakes/DefaultCameraShakeBase.h"
#include "GSHitCameraShake.generated.h"

/**
 * Short Perlin-noise shake played on the local player's camera when they take damage.
 * Defaults set here; tune in a Blueprint subclass (CS_PlayerHit).
 */
UCLASS()
class BANDITBALLER_API UGSHitCameraShake : public UDefaultCameraShakeBase
{
	GENERATED_BODY()

public:
	UGSHitCameraShake(const FObjectInitializer& ObjectInitializer);
};
