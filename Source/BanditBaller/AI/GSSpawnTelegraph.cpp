// BanditBaller

#include "GSSpawnTelegraph.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Disc size: /Engine/BasicShapes/Cylinder is 100 cm wide and 100 cm tall. */
	constexpr float DiscMaxScale = 1.4f;
	constexpr float DiscMinScale = 0.4f;
	constexpr float DiscHeightScale = 0.02f;
}

AGSSpawnTelegraph::AGSSpawnTelegraph()
{
	bReplicates = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DiscMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetStaticMesh(DiscMesh.Object);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetGenerateOverlapEvents(false);
	Disc->SetCastShadow(false);
	Disc->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	Disc->SetRelativeScale3D(FVector(DiscMinScale, DiscMinScale, DiscHeightScale));
	RootComponent = Disc;
}

void AGSSpawnTelegraph::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSSpawnTelegraph, Material);
	DOREPLIFETIME(AGSSpawnTelegraph, Duration);
}

void AGSSpawnTelegraph::Initialize(UMaterialInterface* InMaterial, float InDuration)
{
	Material = InMaterial;
	Duration = FMath::Max(InDuration, 0.05f);
	SetLifeSpan(Duration + 0.5f); // the director destroys it on spawn; this only covers a lost director
	ApplyLook();
}

void AGSSpawnTelegraph::BeginPlay()
{
	Super::BeginPlay();
	LocalStartTime = GetWorld()->GetTimeSeconds();
	ApplyLook();
}

void AGSSpawnTelegraph::OnRep_Look()
{
	ApplyLook();
}

void AGSSpawnTelegraph::ApplyLook()
{
	if (Material)
	{
		Disc->SetMaterial(0, Material);
	}
}

void AGSSpawnTelegraph::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Grow from small to full size over the telegraph, so players read "something is about to appear here".
	const float Alpha = FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - LocalStartTime) / Duration, 0.f, 1.f);
	const float Scale = FMath::Lerp(DiscMinScale, DiscMaxScale, FMath::Sqrt(Alpha));
	Disc->SetRelativeScale3D(FVector(Scale, Scale, DiscHeightScale));
}
