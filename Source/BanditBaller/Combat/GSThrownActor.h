// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSThrownActor.generated.h"

class UStaticMeshComponent;

/**
 * Base of the pooled weapon actors that aren't AGSProjectiles (UGSThrownSubsystem): Dynamite's and the Molotov's lobbed
 * throws, and later fire patches and tomahawks. A mesh with no collision, hidden and not ticking while pooled. Ticks with
 * the world, so everything pauses with the game. Server only; never placed.
 * TODO(co-op): not replicated (single-player first).
 */
UCLASS(Abstract, NotPlaceable)
class BANDITBALLER_API AGSThrownActor : public AActor
{
	GENERATED_BODY()

public:
	AGSThrownActor();

	/** Out of the pool: shown and ticking. */
	void ActivateThrown();

	/** Back to the pool (hidden, no tick). Called by UGSThrownSubsystem. */
	virtual void DeactivateThrown();

	bool IsActiveThrown() const { return bActive; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

protected:
	/** Hand this actor back to UGSThrownSubsystem (or just deactivate it if there is none). */
	void ReleaseToPool();

	UPROPERTY(VisibleAnywhere, Category = "Thrown")
	TObjectPtr<UStaticMeshComponent> Mesh;

	bool bActive = false;
};
