// BanditBaller

#include "GSStartRunPortal.h"
#include "BanditBaller.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GSHubGameMode.h"

AGSStartRunPortal::AGSStartRunPortal()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Trigger = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->InitCapsuleSize(110.f, 130.f);
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(Root);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetGenerateOverlapEvents(false);
	RingMesh->SetCastShadow(false);

	PromptTextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptText"));
	PromptTextComponent->SetupAttachment(Root);
	PromptTextComponent->SetRelativeLocation(FVector(0.f, 0.f, 280.f));
	PromptTextComponent->SetHorizontalAlignment(EHTA_Center);
	PromptTextComponent->SetVerticalAlignment(EVRTA_TextCenter);
	PromptTextComponent->SetWorldSize(70.f);
	PromptTextComponent->SetTextRenderColor(FColor(90, 255, 160));
	PromptTextComponent->SetText(Prompt);
	PromptTextComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGSStartRunPortal::BeginPlay()
{
	Super::BeginPlay();
	PromptTextComponent->SetText(Prompt);
	RingBaseScale = RingMesh->GetRelativeScale3D();
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &AGSStartRunPortal::HandleTriggerOverlap);
	}
}

void AGSStartRunPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Cosmetic, local only: a slow pulse of the ring.
	PulseTime += DeltaSeconds;
	const float Pulse = 1.f + RingPulseAmount * FMath::Sin(PulseTime * RingPulseSpeed * UE_TWO_PI);
	RingMesh->SetRelativeScale3D(FVector(RingBaseScale.X * Pulse, RingBaseScale.Y * Pulse, RingBaseScale.Z));
}

void AGSStartRunPortal::HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryStartRun(Cast<APawn>(OtherActor));
}

bool AGSStartRunPortal::TryStartRun(APawn* Pawn)
{
	if (!HasAuthority() || bStartedRun || !Pawn || !Pawn->IsPlayerControlled())
	{
		return false;
	}
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	AGSHubGameMode* Hub = GetWorld()->GetAuthGameMode<AGSHubGameMode>();
	if (!PC || !Hub)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s: no player controller or not in the hub (AGSHubGameMode); no run started."), *GetName());
		return false;
	}
	bStartedRun = Hub->StartRun(PC);
	return bStartedRun;
}
