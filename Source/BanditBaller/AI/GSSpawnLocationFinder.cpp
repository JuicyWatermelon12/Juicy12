// BanditBaller

#include "GSSpawnLocationFinder.h"
#include "BanditBaller.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GSGameMode.h"
#include "GSGameState.h"
#include "GSRunRandom.h"
#include "GSSpawnSettings.h"
#include "HAL/IConsoleManager.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

const FName UGSSpawnLocationFinder::EnemySpawnTag(TEXT("EnemySpawn"));

namespace
{
	TAutoConsoleVariable<bool> CVarDebugSpawns(
		TEXT("gs.Debug.Spawns"), false,
		TEXT("Draw the spawn ring (min/max radius) around the player and a marker per chosen spawn point (green = ring, yellow = fallback)."));

	constexpr float DebugDrawTime = 8.f;
	/** How far a ring point may snap vertically/horizontally when projected onto the NavMesh. */
	const FVector ProjectExtent(150.f, 150.f, 600.f);
	/** Body-height offset used for the visibility test. */
	constexpr float BodyHeight = 90.f;
	/** Assumed aspect ratio for the vertical half-FOV of the visibility cone. */
	constexpr float AssumedAspect = 16.f / 9.f;

	bool ProjectToNav(const UNavigationSystemV1& NavSys, const FVector& Point, FVector& OutLocation, const FVector& Extent = ProjectExtent)
	{
		FNavLocation NavLocation;
		if (NavSys.ProjectPointToNavigation(Point, NavLocation, Extent))
		{
			OutLocation = NavLocation.Location;
			return true;
		}
		return false;
	}

	bool IsReachable(UWorld& World, const FVector& From, const FVector& PlayerNavLocation)
	{
		const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(&World, From, PlayerNavLocation);
		return Path && Path->IsValid() && !Path->IsPartial();
	}

	bool IsFarEnough(const FVector& Location, const TArray<FGSSpawnPoint>& Chosen, float MinSpacing)
	{
		for (const FGSSpawnPoint& Other : Chosen)
		{
			if (FVector::DistSquared2D(Location, Other.Location) < FMath::Square(MinSpacing))
			{
				return false;
			}
		}
		return true;
	}
}

FString FGSSpawnSearchStats::ToString() const
{
	return FString::Printf(TEXT("%d requested: %d ring, %d fallback, %d failed | %d attempts (%.1f per point) | rejected: nav %d, ring %d, unreachable %d, spacing %d, on-screen %d"),
		Requested, Ring, Fallback, Failed, Attempts, Requested > 0 ? static_cast<float>(Attempts) / Requested : 0.f,
		RejectedNoNavMesh, RejectedOutsideRing, RejectedUnreachable, RejectedSpacing, RejectedOnScreen);
}

const UGSSpawnSettings& UGSSpawnLocationFinder::GetSettings(const UWorld* World)
{
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	const UGSSpawnSettings* Settings = GameMode ? GameMode->GetSpawnSettings() : nullptr;
	return Settings ? *Settings : *GetDefault<UGSSpawnSettings>();
}

bool UGSSpawnLocationFinder::IsVisibleToAnyPlayer(const UWorld& World, const FVector& Location)
{
	const FVector Target = Location + FVector(0.f, 0.f, BodyHeight);
	for (FConstPlayerControllerIterator It = World.GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const APlayerCameraManager* Camera = PC ? PC->PlayerCameraManager.Get() : nullptr;
		if (!Camera)
		{
			continue;
		}

		const FVector CameraLocation = Camera->GetCameraLocation();
		const FRotationMatrix View(Camera->GetCameraRotation());
		const FVector ToTarget = Target - CameraLocation;
		const float Forward = FVector::DotProduct(ToTarget, View.GetUnitAxis(EAxis::X));
		if (Forward <= 0.f)
		{
			continue; // behind the camera
		}

		// Inside the view frustum (horizontal FOV from the camera, vertical from an assumed 16:9 aspect)?
		const float HalfHorizontal = FMath::DegreesToRadians(Camera->GetFOVAngle() * 0.5f);
		const float HalfVertical = FMath::Atan(FMath::Tan(HalfHorizontal) / AssumedAspect);
		const float Right = FVector::DotProduct(ToTarget, View.GetUnitAxis(EAxis::Y));
		const float Up = FVector::DotProduct(ToTarget, View.GetUnitAxis(EAxis::Z));
		if (FMath::Abs(FMath::Atan2(Right, Forward)) > HalfHorizontal || FMath::Abs(FMath::Atan2(Up, Forward)) > HalfVertical)
		{
			continue;
		}

		// In the frustum: visible unless something solid is in the way (behind a wall counts as off-screen).
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSSpawnVisibility), false, PC->GetPawn());
		if (!World.LineTraceTestByChannel(CameraLocation, Target, ECC_Visibility, Params))
		{
			return true;
		}
	}
	return false;
}

TArray<FGSSpawnPoint> UGSSpawnLocationFinder::FindSpawnLocations(APlayerController* PC, int32 Count, FGSSpawnSearchStats* OutStats)
{
	TArray<FGSSpawnPoint> Chosen;
	FGSSpawnSearchStats Stats;
	Stats.Requested = Count;

	UWorld* World = PC ? PC->GetWorld() : nullptr;
	const APawn* Player = PC ? PC->GetPawn() : nullptr;
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!World || !Player || !NavSys || Count <= 0)
	{
		Stats.Failed = FMath::Max(Count, 0);
		if (OutStats) { *OutStats = Stats; }
		return Chosen;
	}

	// The Market District is safe: never spawn while the party is there.
	if (const AGSGameState* GameState = World->GetGameState<AGSGameState>(); GameState && GameState->GetCurrentArea() == EGSArea::MarketDistrict)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("SpawnLocationFinder: refused, the party is in the Market District."));
		Stats.Failed = Count;
		if (OutStats) { *OutStats = Stats; }
		return Chosen;
	}

	const UGSSpawnSettings& Settings = GetSettings(World);
	const float MinRadius = FMath::Min(Settings.MinRadius, Settings.MaxRadius);
	const float MaxRadius = FMath::Max(Settings.MinRadius, Settings.MaxRadius);
	const FVector Origin = Player->GetActorLocation();

	// Reachability is tested against the player's position on the NavMesh.
	FVector PlayerNav;
	const bool bPlayerOnNav = ProjectToNav(*NavSys, Origin, PlayerNav, FVector(300.f, 300.f, 1000.f));
	if (!bPlayerOnNav)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("SpawnLocationFinder: player at %s is off the NavMesh; skipping the reachability check."), *Origin.ToCompactString());
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FGSSpawnPoint Point;
		bool bFound = false;

		for (int32 Attempt = 0; Attempt < Settings.MaxAttempts && !bFound; ++Attempt)
		{
			++Point.Attempts;
			++Stats.Attempts;

			// Uniform over the ring's area (sqrt of a uniform r^2), run stream only.
			const float Angle = UGSRunRandom::FRandRange(PC, 0.f, 2.f * PI);
			const float Radius = FMath::Sqrt(UGSRunRandom::FRandRange(PC, FMath::Square(MinRadius), FMath::Square(MaxRadius)));
			const FVector Candidate = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;

			FVector OnNav;
			if (!ProjectToNav(*NavSys, Candidate, OnNav))
			{
				++Stats.RejectedNoNavMesh;
				continue;
			}
			const float Distance = FVector::Dist2D(OnNav, Origin);
			if (Distance < MinRadius || Distance > MaxRadius)
			{
				++Stats.RejectedOutsideRing;
				continue;
			}
			if (!IsFarEnough(OnNav, Chosen, Settings.MinSpacing))
			{
				++Stats.RejectedSpacing;
				continue;
			}
			const bool bRequireOffScreen = Settings.bPreferOffScreen && Attempt < Settings.MaxAttempts / 2;
			if (bRequireOffScreen && IsVisibleToAnyPlayer(*World, OnNav))
			{
				++Stats.RejectedOnScreen;
				continue;
			}
			if (bPlayerOnNav && !IsReachable(*World, OnNav, PlayerNav))
			{
				++Stats.RejectedUnreachable;
				continue;
			}

			Point.Location = OnNav;
			bFound = true;
		}

		if (!bFound)
		{
			// Fallback: the farthest EnemySpawn point on the NavMesh, preferring ones that keep the spacing.
			const FVector* Best = nullptr;
			FVector BestLocation;
			bool bBestSpaced = false;
			float BestDistance = -1.f;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				FVector OnNav;
				if (!It->ActorHasTag(EnemySpawnTag) || !ProjectToNav(*NavSys, It->GetActorLocation(), OnNav))
				{
					continue;
				}
				const bool bSpaced = IsFarEnough(OnNav, Chosen, Settings.MinSpacing);
				const float Distance = FVector::Dist2D(OnNav, Origin);
				if ((bSpaced && !bBestSpaced) || (bSpaced == bBestSpaced && Distance > BestDistance))
				{
					BestLocation = OnNav;
					Best = &BestLocation;
					bBestSpaced = bSpaced;
					BestDistance = Distance;
				}
			}

			if (Best)
			{
				// Several fallbacks can share a Target Point: nudge onto free NavMesh nearby.
				if (!bBestSpaced)
				{
					for (int32 Nudge = 0; Nudge < 8; ++Nudge)
					{
						const float Angle = UGSRunRandom::FRandRange(PC, 0.f, 2.f * PI);
						const FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Settings.MinSpacing * (1.f + Nudge * 0.5f);
						FVector OnNav;
						if (ProjectToNav(*NavSys, BestLocation + Offset, OnNav) && IsFarEnough(OnNav, Chosen, Settings.MinSpacing))
						{
							BestLocation = OnNav;
							break;
						}
					}
				}
				Point.Location = BestLocation;
				Point.bFallback = true;
				bFound = true;
				UE_LOG(LogBanditBaller, Warning, TEXT("SpawnLocationFinder: no valid ring point after %d attempts around %s; using EnemySpawn fallback at %s (%.0f cm away)."),
					Point.Attempts, *Origin.ToCompactString(), *BestLocation.ToCompactString(), FVector::Dist2D(BestLocation, Origin));
			}
		}

		if (bFound)
		{
			Chosen.Add(Point);
			(Point.bFallback ? Stats.Fallback : Stats.Ring)++;
		}
		else
		{
			++Stats.Failed;
			UE_LOG(LogBanditBaller, Warning, TEXT("SpawnLocationFinder: no ring point and no EnemySpawn fallback on the NavMesh."));
		}
	}

#if ENABLE_DRAW_DEBUG
	if (CVarDebugSpawns.GetValueOnGameThread())
	{
		const FVector Center = Origin - FVector(0.f, 0.f, Player->GetSimpleCollisionHalfHeight() - 5.f);
		DrawDebugCircle(World, Center, MinRadius, 64, FColor::Cyan, false, DebugDrawTime, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		DrawDebugCircle(World, Center, MaxRadius, 96, FColor::Blue, false, DebugDrawTime, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		for (const FGSSpawnPoint& Point : Chosen)
		{
			const FColor Color = Point.bFallback ? FColor::Yellow : FColor::Green;
			DrawDebugSphere(World, Point.Location + FVector(0.f, 0.f, 50.f), 50.f, 12, Color, false, DebugDrawTime, 0, 3.f);
			DrawDebugLine(World, Point.Location, Point.Location + FVector(0.f, 0.f, 400.f), Color, false, DebugDrawTime, 0, 6.f);
		}
	}
#endif

	UE_LOG(LogBanditBaller, Log, TEXT("SpawnLocationFinder: %s"), *Stats.ToString());
	if (OutStats) { *OutStats = Stats; }
	return Chosen;
}
