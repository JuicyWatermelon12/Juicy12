// BanditBaller

#include "GSThrownActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GSThrownSubsystem.h"

AGSThrownActor::AGSThrownActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false; // TODO(co-op): replicate or spawn on clients in M6.

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);
	Mesh->SetMobility(EComponentMobility::Movable);

	SetActorHiddenInGame(true);
}

void AGSThrownActor::ActivateThrown()
{
	bActive = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
}

void AGSThrownActor::DeactivateThrown()
{
	bActive = false;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void AGSThrownActor::ReleaseToPool()
{
	if (UGSThrownSubsystem* Thrown = GetWorld() ? GetWorld()->GetSubsystem<UGSThrownSubsystem>() : nullptr)
	{
		Thrown->Release(this);
	}
	else
	{
		DeactivateThrown();
	}
}
