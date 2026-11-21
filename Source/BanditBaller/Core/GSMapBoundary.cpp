// BanditBaller

#include "GSMapBoundary.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GSCollisionChannels.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarDebugBoundary(
		TEXT("gs.Debug.Boundary"), 0,
		TEXT("1 = draw the map boundary's play area (walls + ceiling) as a box."),
		ECVF_Cheat);
}

AGSMapBoundary::AGSMapBoundary()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	WallNorth = CreateBox(TEXT("WallNorth"));
	WallSouth = CreateBox(TEXT("WallSouth"));
	WallEast = CreateBox(TEXT("WallEast"));
	WallWest = CreateBox(TEXT("WallWest"));
	Ceiling = CreateBox(TEXT("Ceiling"));
	LayoutBoxes();
}

UBoxComponent* AGSMapBoundary::CreateBox(FName Name)
{
	UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Name);
	Box->SetupAttachment(Root);
	// Pawns only: its own object type, ignore every channel except Pawn.
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(GSCollision::MapBoundary);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Box->SetGenerateOverlapEvents(false);
	Box->SetCanEverAffectNavigation(false);
	Box->SetHiddenInGame(true);
	Box->ShapeColor = FColor(255, 80, 40);
	return Box;
}

void AGSMapBoundary::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutBoxes();
}

void AGSMapBoundary::BeginPlay()
{
	Super::BeginPlay();
	LayoutBoxes();
}

void AGSMapBoundary::LayoutBoxes()
{
	const float HalfX = PlayAreaSize.X * 0.5f;
	const float HalfY = PlayAreaSize.Y * 0.5f;
	const float HalfT = WallThickness * 0.5f;
	// Walls run from below the ground to the top of the ceiling, and overlap at the corners.
	const float WallBottom = -WallDepthBelowGround;
	const float WallTop = CeilingHeight + WallThickness;
	const float WallHalfZ = (WallTop - WallBottom) * 0.5f;
	const float WallCenterZ = (WallTop + WallBottom) * 0.5f;

	auto Place = [](UBoxComponent* Box, const FVector& Center, const FVector& Extent)
	{
		if (Box)
		{
			Box->SetRelativeLocation(Center);
			Box->SetBoxExtent(Extent, /*bUpdateOverlaps*/ false);
		}
	};
	Place(WallNorth, FVector(HalfX + HalfT, 0.f, WallCenterZ), FVector(HalfT, HalfY + WallThickness, WallHalfZ));
	Place(WallSouth, FVector(-HalfX - HalfT, 0.f, WallCenterZ), FVector(HalfT, HalfY + WallThickness, WallHalfZ));
	Place(WallEast, FVector(0.f, HalfY + HalfT, WallCenterZ), FVector(HalfX + WallThickness, HalfT, WallHalfZ));
	Place(WallWest, FVector(0.f, -HalfY - HalfT, WallCenterZ), FVector(HalfX + WallThickness, HalfT, WallHalfZ));
	Place(Ceiling, FVector(0.f, 0.f, CeilingHeight + HalfT), FVector(HalfX + WallThickness, HalfY + WallThickness, HalfT));
}

FBox AGSMapBoundary::GetPlayBounds() const
{
	const FVector Center = GetActorLocation();
	return FBox(Center + FVector(-PlayAreaSize.X * 0.5f, -PlayAreaSize.Y * 0.5f, -WallDepthBelowGround),
		Center + FVector(PlayAreaSize.X * 0.5f, PlayAreaSize.Y * 0.5f, CeilingHeight));
}

bool AGSMapBoundary::IsInside(const FVector& Location, float Margin) const
{
	const FBox Bounds = GetPlayBounds();
	return Location.X >= Bounds.Min.X + Margin && Location.X <= Bounds.Max.X - Margin
		&& Location.Y >= Bounds.Min.Y + Margin && Location.Y <= Bounds.Max.Y - Margin
		&& Location.Z <= Bounds.Max.Z - Margin;
}

AGSMapBoundary* AGSMapBoundary::Find(const UWorld* World)
{
	if (World)
	{
		for (TActorIterator<AGSMapBoundary> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

TArray<UBoxComponent*> AGSMapBoundary::GetBoxes() const
{
	return { WallNorth, WallSouth, WallEast, WallWest, Ceiling };
}

void AGSMapBoundary::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if ENABLE_DRAW_DEBUG
	if (CVarDebugBoundary.GetValueOnGameThread())
	{
		const FBox Bounds = GetPlayBounds();
		DrawDebugBox(GetWorld(), Bounds.GetCenter(), Bounds.GetExtent(), FColor(255, 80, 40), false, -1.f, 0, 12.f);
		// The ceiling plane, so it is easy to see from the ground.
		const FVector C = GetActorLocation() + FVector(0.f, 0.f, CeilingHeight);
		DrawDebugLine(GetWorld(), C + FVector(-PlayAreaSize.X * 0.5f, 0.f, 0.f), C + FVector(PlayAreaSize.X * 0.5f, 0.f, 0.f), FColor::Red, false, -1.f, 0, 12.f);
		DrawDebugLine(GetWorld(), C + FVector(0.f, -PlayAreaSize.Y * 0.5f, 0.f), C + FVector(0.f, PlayAreaSize.Y * 0.5f, 0.f), FColor::Red, false, -1.f, 0, 12.f);
	}
#endif
}
