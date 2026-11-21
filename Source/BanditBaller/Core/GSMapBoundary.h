// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSMapBoundary.generated.h"

class UBoxComponent;

/**
 * Invisible walls around a map's play area plus a ceiling (one per map, placed in the level; L_Town_Graybox has one).
 *
 * The actor's location is the center of the play area at ground level. PlayAreaSize (X / Y, cm) and CeilingHeight (cm
 * above that ground) set the inner box; the walls and ceiling are WallThickness thick and sit just outside it, so the
 * whole inside is playable. Change the properties on the placed actor when the map grows; the boxes follow.
 *
 * Blocks pawns only (object type GSCollision::MapBoundary; it ignores every channel but Pawn): weapon traces, enemy
 * shots, the camera and pickups pass through. Hitting the ceiling only stops upward movement (CharacterMovement's
 * normal blocking hit), which is what limits Extra Jump height. It never affects the NavMesh.
 * gs.Debug.Boundary 1 draws the play-area box.
 */
UCLASS()
class BANDITBALLER_API AGSMapBoundary : public AActor
{
	GENERATED_BODY()

public:
	AGSMapBoundary();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Inner play area width (X) and depth (Y), centered on the actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary", meta = (ClampMin = "100", Units = "cm"))
	FVector2D PlayAreaSize = FVector2D(15000.f, 15000.f);

	/** Height of the ceiling's underside above the actor (the ground). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary", meta = (ClampMin = "100", Units = "cm"))
	float CeilingHeight = 7500.f;

	/** How far the walls reach below the ground (so nothing slips under them on uneven floors). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary", meta = (ClampMin = "0", Units = "cm"))
	float WallDepthBelowGround = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary", meta = (ClampMin = "10", Units = "cm"))
	float WallThickness = 200.f;

	/** The inner play area: walls and ceiling on its sides and top (bottom = WallDepthBelowGround under the ground). */
	FBox GetPlayBounds() const;

	/** True if Location is inside the play area, at least Margin cm from every wall and the ceiling. */
	bool IsInside(const FVector& Location, float Margin = 0.f) const;

	float GetGroundZ() const { return GetActorLocation().Z; }
	float GetCeilingZ() const { return GetActorLocation().Z + CeilingHeight; }

	/** The first boundary in World (nullptr if the map has none). */
	static AGSMapBoundary* Find(const UWorld* World);

	/** The five boxes (4 walls + ceiling), for tests. */
	TArray<UBoxComponent*> GetBoxes() const;

	UBoxComponent* GetCeiling() const { return Ceiling; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	TObjectPtr<UBoxComponent> WallNorth;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	TObjectPtr<UBoxComponent> WallSouth;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	TObjectPtr<UBoxComponent> WallEast;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	TObjectPtr<UBoxComponent> WallWest;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	TObjectPtr<UBoxComponent> Ceiling;

private:
	UBoxComponent* CreateBox(FName Name);
	/** Size and place the five boxes from the properties (relative to the actor; the actor isn't rotated). */
	void LayoutBoxes();
};
