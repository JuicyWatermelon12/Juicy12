// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSStartRunPortal.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The hub's "Start Run" portal (BP_GSStartRunPortal, placed at MarketExitPortalSpot in L_Hub_Graybox): a placeholder
 * glowing ring on the floor with a floating "START RUN" prompt. A player walking into the trigger asks
 * AGSHubGameMode::StartRun, which fades out and opens the run map; the run starts when the town loads.
 * Server only (single-player: the local player). The ring mesh / material are set on the Blueprint.
 */
UCLASS()
class BANDITBALLER_API AGSStartRunPortal : public AActor
{
	GENERATED_BODY()

public:
	AGSStartRunPortal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** What walking in does: start the run for Pawn's player. False if Pawn isn't a player or the hub refused. */
	bool TryStartRun(APawn* Pawn);

	bool HasStartedRun() const { return bStartedRun; }
	FText GetPromptText() const { return Prompt; }
	UCapsuleComponent* GetTrigger() const { return Trigger; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<USceneComponent> Root;

	/** Overlaps pawns; entering it starts the run. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UCapsuleComponent> Trigger;

	/** Placeholder glowing ring on the floor (mesh + emissive material on BP_GSStartRunPortal). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> RingMesh;

	/** "START RUN", floating above the ring (faces the actor's forward). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UTextRenderComponent> PromptTextComponent;

	UPROPERTY(EditAnywhere, Category = "Portal")
	FText Prompt = NSLOCTEXT("GSPortal", "StartRun", "START RUN");

	/** Cosmetic: the ring's scale pulses by this much, this many times per second (local only). */
	UPROPERTY(EditAnywhere, Category = "Portal", meta = (ClampMin = "0", ClampMax = "1"))
	float RingPulseAmount = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Portal", meta = (ClampMin = "0"))
	float RingPulseSpeed = 0.8f;

private:
	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bStartedRun = false;
	FVector RingBaseScale = FVector::OneVector;
	float PulseTime = 0.f;
};
