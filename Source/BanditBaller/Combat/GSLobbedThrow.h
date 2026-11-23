// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSThrownActor.h"
#include "GSLobbedThrow.generated.h"

class ACharacter;
class UMaterialInterface;
class UStaticMesh;

/** Where a lob thrown at a target comes down (AGSLobbedThrow::AimAt). */
struct FGSLobAim
{
	/** The landing point: on the ground, under where the target will be. */
	FVector Landing = FVector::ZeroVector;
	/** The target's body center over the landing point (where Dynamite's blast is centered). */
	FVector BodyCenter = FVector::ZeroVector;
	/** True when the landing was moved ahead of the target (it moves, LeadFactor > 0 and there is ground there). */
	bool bLed = false;
};

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

	/**
	 * Aim a lob at Target that lands FlightTime from now: its position plus its flat velocity x FlightTime x LeadFactor,
	 * on the ground under that point (a WorldStatic trace down). LeadFactor 0, a target standing still, or no ground there
	 * (off the map): where it stands now, at its feet, exactly as before leading existed.
	 */
	static FGSLobAim AimAt(const ACharacter* Target, float FlightTime, float LeadFactor);

	/** The point on the arc after Seconds of flight (0 = Start, FlightTime = Landing). */
	FVector GetArcLocation(float Seconds) const;

	FVector GetLanding() const { return Params.Landing; }
	float GetFlown() const { return Flown; }

private:
	FGSLobLaunch Params;
	float Flown = 0.f;
};
