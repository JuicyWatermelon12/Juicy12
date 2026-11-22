// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSThrownActor.h"
#include "GSLobbedThrow.generated.h"

class UMaterialInterface;
class UStaticMesh;

/** Everything one lob needs (AGSLobbedThrow::Launch). */
struct FGSLobLaunch
{
	FVector Start = FVector::ZeroVector;
	/** Where it comes down, FlightTime after it was thrown. */
	FVector Landing = FVector::ZeroVector;
	float FlightTime = 0.6f;
	/** Height of the arc's apex above the straight line from Start to Landing. */
	float ArcHeight = 300.f;
	/** Seconds already flown (a late throw starts that far along). */
	float AlreadyFlown = 0.f;
	TObjectPtr<UStaticMesh> Mesh;
	TObjectPtr<UMaterialInterface> Material;
	FVector Scale = FVector(0.1f);
	/** Cosmetic tumble while it flies (degrees a second). */
	float SpinRate = 0.f;
	/** Called once when it lands, with the landing point, after it went back to the pool. */
	TFunction<void(const FVector&)> OnLanded;
};

/**
 * A lobbed throw (Dynamite's stick, the Molotov's bottle): flies a fixed arc from Start to Landing over FlightTime with no
 * collision (it goes over cover), then calls OnLanded and goes back to the pool. Pauses with the game (world tick). A new
 * run (UGSThrownSubsystem::ReleaseAll) drops it without landing.
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSLobbedThrow : public AGSThrownActor
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void DeactivateThrown() override;

	void Launch(FGSLobLaunch&& InLaunch);

	/** The point on the arc after Seconds of flight (0 = Start, FlightTime = Landing). */
	FVector GetArcLocation(float Seconds) const;

	FVector GetLanding() const { return Params.Landing; }
	float GetFlown() const { return Flown; }

private:
	FGSLobLaunch Params;
	float Flown = 0.f;
};
