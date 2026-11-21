// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSSpawnTelegraph.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;

/**
 * Ground marker shown at a spawn point shortly before an enemy appears (UGSWaveDirector). Server spawns it;
 * it replicates so every player sees it. Purely cosmetic: a flat disc that grows over its duration.
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSSpawnTelegraph : public AActor
{
	GENERATED_BODY()

public:
	AGSSpawnTelegraph();

	/** Server: set the look and duration (also sets a slightly longer life span as a safety net). */
	void Initialize(UMaterialInterface* InMaterial, float InDuration);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Telegraph")
	TObjectPtr<UStaticMeshComponent> Disc;

private:
	UFUNCTION()
	void OnRep_Look();

	void ApplyLook();

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	float Duration = 0.75f;

	double LocalStartTime = 0.0;
};
