// BanditBaller

#include "GSLobbedThrow.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GSWeaponDebug.h"

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
