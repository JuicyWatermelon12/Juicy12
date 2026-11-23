// BanditBaller

#include "GSLobbedThrow.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GSWeaponDebug.h"

FGSLobAim AGSLobbedThrow::AimAt(const ACharacter* Target, float FlightTime, float LeadFactor)
{
	FGSLobAim Aim;
	if (!Target)
	{
		return Aim;
	}
	// Today's spot: the body center, and the feet under it (the capsule's bottom).
	const float HalfHeight = Target->GetCapsuleComponent() ? Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	Aim.BodyCenter = Target->GetActorLocation();
	Aim.Landing = Aim.BodyCenter - FVector(0.f, 0.f, HalfHeight);

	FVector Lead = Target->GetVelocity() * (FMath::Max(FlightTime, 0.f) * FMath::Max(LeadFactor, 0.f));
	Lead.Z = 0.f; // flat: the ground below decides the height
	UWorld* World = Target->GetWorld();
	if (Lead.SizeSquared() < 1.f || !World)
	{
		return Aim; // standing still (or no lead): exactly the old landing point
	}

	// The ground under the point ahead: a trace down from body height through the floor, world geometry only (pawns,
	// pickups and the map boundary are not ground).
	const FVector Ahead = Aim.BodyCenter + Lead;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(GSLobAim), false, Target);
	FHitResult Ground;
	if (!World->LineTraceSingleByObjectType(Ground, Ahead, Ahead - FVector(0.f, 0.f, HalfHeight + 500.f), Objects, Query))
	{
		return Aim; // no ground there (off the map): where it stands now
	}
	Aim.Landing = Ground.ImpactPoint;
	Aim.BodyCenter = Aim.Landing + FVector(0.f, 0.f, HalfHeight);
	Aim.bLed = true;
	return Aim;
}

void AGSLobbedThrow::Launch(FGSLobLaunch&& InLaunch)
{
	Params = MoveTemp(InLaunch);
	Params.FlightTime = FMath::Max(Params.FlightTime, 0.01f);
	Flown = FMath::Clamp(Params.AlreadyFlown, 0.f, Params.FlightTime);
	if (Params.Mesh)
	{
		Mesh->SetStaticMesh(Params.Mesh);
	}
	if (Params.Material)
	{
		Mesh->SetMaterial(0, Params.Material);
	}
	SetActorScale3D(Params.Scale);
	SetActorLocation(GetArcLocation(Flown), false, nullptr, ETeleportType::TeleportPhysics);
}

FVector AGSLobbedThrow::GetArcLocation(float Seconds) const
{
	const float T = FMath::Clamp(Seconds / Params.FlightTime, 0.f, 1.f);
	return FMath::Lerp(Params.Start, Params.Landing, T) + FVector(0.f, 0.f, 4.f * Params.ArcHeight * T * (1.f - T));
}

void AGSLobbedThrow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}
	const FVector From = GetActorLocation();
	Flown += DeltaSeconds;
	if (Flown >= Params.FlightTime)
	{
		// Down: back to the pool first, so the landing (a blast, a fire patch) can use the pool freely.
		const FVector Landing = Params.Landing;
		TFunction<void(const FVector&)> OnLanded = MoveTemp(Params.OnLanded);
		Params.OnLanded = nullptr;
		SetActorLocation(Landing);
		ReleaseToPool();
		if (OnLanded)
		{
			OnLanded(Landing);
		}
		return;
	}
	const FVector To = GetArcLocation(Flown);
	const FRotator Tumble(FMath::Fmod(Flown * Params.SpinRate, 360.f), (To - From).Rotation().Yaw, 0.f);
	SetActorLocationAndRotation(To, Tumble);
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugLine(GetWorld(), From, To, FColor::Red, false, GSWeaponDebug::DrawDuration);
	}
}

void AGSLobbedThrow::DeactivateThrown()
{
	Params.OnLanded = nullptr; // a new run drops it without landing
	Super::DeactivateThrown();
}
