// BanditBaller

#include "GSCashPickup.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GSAttributeSet.h"
#include "GSCashPickupSubsystem.h"
#include "GSCashSettings.h"
#include "GSHealthComponent.h"
#include "GSPlayerState.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float SpinDegreesPerSecond = 220.f;
	constexpr float BobHeight = 8.f;
	constexpr float BobSpeed = 3.f;
	constexpr float MaxMagnetDistance = 10000.f;
}

AGSCashPickup::AGSCashPickup()
{
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(30.f);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);

	SetActorHiddenInGame(true);
}

void AGSCashPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSCashPickup, bActive);
	DOREPLIFETIME(AGSCashPickup, Value);
	DOREPLIFETIME(AGSCashPickup, bBill);
	DOREPLIFETIME(AGSCashPickup, Settings);
	DOREPLIFETIME(AGSCashPickup, ExpireAtServerTime);
}

// ---------------------------------------------------------------------------
// Pool lifecycle (server)
// ---------------------------------------------------------------------------

void AGSCashPickup::ActivatePickup(EGSPickupKind InKind, int32 InValue, bool bInBill, const UGSCashSettings* InSettings, const FVector& From, const FVector& Landing, double InExpireAtServerTime)
{
	Kind = InKind;
	Value = InValue;
	bBill = bInBill;
	Settings = InSettings;
	ExpireAtServerTime = InExpireAtServerTime;

	State = EState::Tossing;
	TossFrom = From;
	TossTo = Landing;
	TossElapsed = 0.f;
	MagnetTarget.Reset();
	MagnetSpeed = 0.f;
	bForcedMagnet = false;
	CosmeticTime = 0.f;

	SetActorLocation(From, false, nullptr, ETeleportType::TeleportPhysics);
	bActive = true;
	ApplyLook();
	ApplyActive();
	ForceNetUpdate();
}

void AGSCashPickup::StartMagnet(APawn* Target, bool bForced)
{
	if (!bActive || !Target)
	{
		return;
	}
	State = EState::Magnet;
	MagnetTarget = Target;
	MagnetSpeed = Settings ? Settings->MagnetStartSpeed : 400.f;
	bForcedMagnet = bForced;
	ExpireAtServerTime = 0.0; // no expiry / blink while flying
}

void AGSCashPickup::ReturnToGround(UGSCashPickupSubsystem& Subsystem)
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	State = EState::Idle;
	MagnetTarget.Reset();
	bForcedMagnet = false;
	const float Lifetime = Subsystem.GetLifetime(Kind);
	ExpireAtServerTime = (GameState && Lifetime > 0.f) ? GameState->GetServerWorldTimeSeconds() + Lifetime : 0.0;
}

bool AGSCashPickup::IsAtFullHealth(const APawn* Pawn)
{
	const AGSPlayerState* PS = Pawn ? Pawn->GetPlayerState<AGSPlayerState>() : nullptr;
	const UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return true;
	}
	const float Health = ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute());
	return Health >= MaxHealth - KINDA_SMALL_NUMBER;
}

void AGSCashPickup::DeactivatePickup()
{
	bActive = false;
	bForcedMagnet = false;
	MagnetTarget.Reset();
	ApplyActive();
	ForceNetUpdate();
}

void AGSCashPickup::OnRep_Active()
{
	ApplyActive();
}

void AGSCashPickup::OnRep_Look()
{
	ApplyLook();
}

void AGSCashPickup::ApplyActive()
{
	SetActorHiddenInGame(!bActive);
	SetActorTickEnabled(bActive);
	Mesh->SetVisibility(true);
}

const FGSCashPieceLook* AGSCashPickup::GetLook() const
{
	if (!Settings)
	{
		return nullptr;
	}
	if (Kind == EGSPickupKind::XP)
	{
		return &Settings->XPOrbLook;
	}
	if (Kind == EGSPickupKind::Health)
	{
		return &Settings->HealthOrbLook;
	}
	return bBill ? &Settings->BillLook : &Settings->CoinLook;
}

void AGSCashPickup::ApplyLook()
{
	const FGSCashPieceLook* Look = GetLook();
	if (!Look)
	{
		return;
	}
	if (Look->Mesh)
	{
		Mesh->SetStaticMesh(Look->Mesh);
	}
	if (Look->Material)
	{
		Mesh->SetMaterial(0, Look->Material);
	}
	Mesh->SetRelativeScale3D(Look->Scale);
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AGSCashPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}
	if (HasAuthority())
	{
		TickServer(DeltaSeconds);
	}
	if (bActive) // may have been collected / expired above
	{
		TickCosmetic(DeltaSeconds);
	}
}

bool AGSCashPickup::IsLivingPlayer(const APawn* Pawn)
{
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return false;
	}
	const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Pawn);
	return !Health || !Health->IsDead();
}

APawn* AGSCashPickup::FindPlayerInRange() const
{
	APawn* Best = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		const UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
		if (!IsLivingPlayer(Pawn) || !ASC)
		{
			continue;
		}
		if (Kind == EGSPickupKind::Health && IsAtFullHealth(Pawn))
		{
			continue; // nothing to heal: the orb waits until this player is hurt
		}
		const float Radius = ASC->GetNumericAttribute(UGSAttributeSet::GetPickupRadiusAttribute());
		const float DistanceSq = FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation());
		if (DistanceSq <= FMath::Square(Radius) && DistanceSq < BestDistanceSq)
		{
			Best = Pawn;
			BestDistanceSq = DistanceSq;
		}
	}
	return Best;
}

void AGSCashPickup::TickServer(float DeltaSeconds)
{
	UGSCashPickupSubsystem* Subsystem = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>();
	const UGSCashSettings* S = Settings.Get();
	if (!Subsystem || !S)
	{
		return;
	}

	switch (State)
	{
	case EState::Tossing:
	{
		TossElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(TossElapsed / S->TossTime, 0.f, 1.f);
		FVector Location = FMath::Lerp(TossFrom, TossTo, Alpha);
		Location.Z += S->TossHeight * 4.f * Alpha * (1.f - Alpha); // parabola
		SetActorLocation(Location);
		if (Alpha >= 1.f)
		{
			State = EState::Idle;
		}
		break;
	}

	case EState::Idle:
	{
		const AGameStateBase* GameState = GetWorld()->GetGameState();
		if (ExpireAtServerTime > 0.0 && GameState && GameState->GetServerWorldTimeSeconds() >= ExpireAtServerTime)
		{
			Subsystem->ReleasePickup(this, /*bExpired*/ true);
			return;
		}
		if (APawn* Player = FindPlayerInRange())
		{
			StartMagnet(Player);
		}
		break;
	}

	case EState::Magnet:
	{
		APawn* Target = MagnetTarget.Get();
		if (!IsLivingPlayer(Target) || (Kind == EGSPickupKind::Health && !bForcedMagnet && IsAtFullHealth(Target)))
		{
			// Target died or left, or a health orb's target got to full health on the way: wait on the ground again with
			// a fresh lifetime (none for orbs by default).
			ReturnToGround(*Subsystem);
			break;
		}

		MagnetSpeed = FMath::Min(MagnetSpeed + S->MagnetAcceleration * DeltaSeconds, S->MagnetMaxSpeed);
		const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		const float Distance = ToTarget.Size();
		const float Step = MagnetSpeed * DeltaSeconds;
		// Very far = the player teleported (area travel): pay out now instead of flying across the map.
		if (Distance <= S->CollectDistance || Step >= Distance || Distance > MaxMagnetDistance)
		{
			Subsystem->CollectPickup(this, Cast<APlayerController>(Target->GetController()));
			return;
		}
		SetActorLocation(GetActorLocation() + ToTarget / Distance * Step);
		break;
	}
	}
}

void AGSCashPickup::TickCosmetic(float DeltaSeconds)
{
	CosmeticTime += DeltaSeconds;

	// Spin (cash) or pulse (orbs) + bob, on the mesh only (the replicated root stays where the server puts it).
	const FGSCashPieceLook* Look = GetLook();
	const FRotator BaseRotation = Look ? Look->Rotation : FRotator::ZeroRotator;
	if (Kind != EGSPickupKind::Cash)
	{
		const bool bHealth = Kind == EGSPickupKind::Health;
		const float Amount = Settings ? (bHealth ? Settings->HealthOrbPulseAmount : Settings->XPOrbPulseAmount) : 0.f;
		const float Speed = Settings ? (bHealth ? Settings->HealthOrbPulseSpeed : Settings->XPOrbPulseSpeed) : 0.f;
		const float Pulse = 1.f + Amount * FMath::Sin(CosmeticTime * Speed * UE_TWO_PI);
		Mesh->SetRelativeScale3D((Look ? Look->Scale : FVector::OneVector) * Pulse);
		Mesh->SetRelativeRotation(BaseRotation);
	}
	else
	{
		Mesh->SetRelativeRotation(FRotator(0.f, CosmeticTime * SpinDegreesPerSecond, 0.f).Quaternion() * BaseRotation.Quaternion());
	}
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, BobHeight * FMath::Sin(CosmeticTime * BobSpeed)));

	// Blink during the last BlinkTime seconds, faster as it runs out.
	bool bVisible = true;
	if (IsBlinking())
	{
		const AGameStateBase* GameState = GetWorld()->GetGameState();
		const float Remaining = static_cast<float>(ExpireAtServerTime - GameState->GetServerWorldTimeSeconds());
		const float Urgency = 1.f - FMath::Clamp(Remaining / Settings->BlinkTime, 0.f, 1.f);
		const float Frequency = FMath::Lerp(3.f, 10.f, Urgency);
		bVisible = FMath::Frac(CosmeticTime * Frequency) < 0.55f;
	}
	Mesh->SetVisibility(bVisible);
}

bool AGSCashPickup::IsBlinking() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!bActive || ExpireAtServerTime <= 0.0 || !GameState || !Settings || Settings->BlinkTime <= 0.f)
	{
		return false;
	}
	return ExpireAtServerTime - GameState->GetServerWorldTimeSeconds() < Settings->BlinkTime;
}
