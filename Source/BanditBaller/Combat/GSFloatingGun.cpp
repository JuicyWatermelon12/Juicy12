// BanditBaller

#include "GSFloatingGun.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSEnemyCharacter.h"
#include "GSFloatingGunSettings.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSProjectileSubsystem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Snap instead of gliding when the gun is this far from where it should be (respawn, teleport). */
	constexpr float SnapDistance = 500.f;
	/** At most this many owed shots are fired in one frame (a runaway guard, like GA_Fire's). */
	constexpr int32 MaxShotsPerFrame = 16;
}

AGSFloatingGun::AGSFloatingGun()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics; // after the character moved this frame
	bReplicates = false; // TODO(co-op): replicate the gun (or spawn it on clients) in M6.

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	// Visual only: it never blocks or overlaps anything.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);
}

void AGSFloatingGun::InitGun(AGSCharacter* InOwner, const UGSFloatingGunSettings* InSettings, float InSide)
{
	OwnerCharacter = InOwner;
	Settings = InSettings;
	Side = InSide >= 0.f ? 1.f : -1.f;
	if (Settings)
	{
		if (Settings->GunMesh)
		{
			Mesh->SetStaticMesh(Settings->GunMesh);
		}
		if (Settings->GunMaterial)
		{
			Mesh->SetMaterial(0, Settings->GunMaterial);
		}
		SetActorScale3D(Settings->GunScale);
		if (UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>())
		{
			Projectiles->Prewarm(Settings->ProjectilePoolPrewarm);
		}
	}
	NextShotTime = GetWorld()->GetTimeSeconds();
	UpdatePlacement(0.f, /*bSnap*/ true);
}

FVector AGSFloatingGun::GetMuzzleLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * (Settings ? Settings->MuzzleOffset : 0.f);
}

void AGSFloatingGun::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!OwnerCharacter.IsValid() || !Settings)
	{
		return;
	}
	UpdateCrosshairClearance(DeltaSeconds);
	UpdatePlacement(DeltaSeconds, /*bSnap*/ false);
	UpdateShooting(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Placement and the crosshair
// ---------------------------------------------------------------------------

void AGSFloatingGun::UpdatePlacement(float DeltaSeconds, bool bSnap)
{
	const AGSCharacter* Character = OwnerCharacter.Get();
	if (!Character || !Settings)
	{
		return;
	}
	// The camera's yaw frame: over the right shoulder as the player sees it, however the character is turned.
	const AController* Controller = Character->GetController();
	const float Yaw = Controller ? Controller->GetControlRotation().Yaw : Character->GetActorRotation().Yaw;
	const FRotator YawFrame(0.f, Yaw, 0.f);
	const FVector Offset(Settings->ShoulderOffset.X, Settings->ShoulderOffset.Y * Side, Settings->ShoulderOffset.Z);
	RestLocation = Character->GetActorLocation() + YawFrame.RotateVector(Offset);

	const FVector Desired = RestLocation + AvoidOffset;
	const FVector Current = GetActorLocation();
	const FVector NewLocation = (bSnap || FVector::DistSquared(Current, Desired) > FMath::Square(SnapDistance))
		? Desired : FMath::VInterpTo(Current, Desired, DeltaSeconds, Settings->FollowSpeed);

	// Face the target, else where the camera looks (level).
	const AGSEnemyCharacter* AimTarget = Target.Get();
	const FRotator DesiredRotation = AimTarget ? (AimTarget->GetActorLocation() - NewLocation).Rotation() : YawFrame;
	const FRotator NewRotation = bSnap ? DesiredRotation : FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, Settings->TurnSpeed);
	SetActorLocationAndRotation(NewLocation, NewRotation);

	// Where it shows on the local player's screen (tests / debug).
	LastCrosshairDistance = -1.f;
	const APlayerController* PC = Cast<APlayerController>(Controller);
	int32 SizeX = 0, SizeY = 0;
	FVector2D Screen;
	if (PC && PC->IsLocalController() && (PC->GetViewportSize(SizeX, SizeY), SizeY > 0)
		&& UGameplayStatics::ProjectWorldToScreen(PC, NewLocation, Screen, /*bPlayerViewportRelative*/ true))
	{
		LastCrosshairDistance = static_cast<float>((Screen - FVector2D(SizeX, SizeY) * 0.5).Size() / SizeY);
	}
}

void AGSFloatingGun::UpdateCrosshairClearance(float DeltaSeconds)
{
	const AGSCharacter* Character = OwnerCharacter.Get();
	const APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	int32 SizeX = 0, SizeY = 0;
	if (PC && PC->IsLocalController())
	{
		PC->GetViewportSize(SizeX, SizeY);
	}
	const float Radius = Settings->CrosshairClearRadius;
	if (!PC || !PC->IsLocalController() || !PC->PlayerCameraManager || SizeY <= 0 || Radius <= 0.f)
	{
		AvoidOffset = FMath::VInterpConstantTo(AvoidOffset, FVector::ZeroVector, DeltaSeconds, Settings->CrosshairAvoidSpeed);
		return;
	}

	// Where it would show if it stood where it wants to be (rest + the current push).
	FVector2D Screen;
	const FVector Wanted = RestLocation + AvoidOffset;
	if (!UGameplayStatics::ProjectWorldToScreen(PC, Wanted, Screen, /*bPlayerViewportRelative*/ true))
	{
		AvoidOffset = FMath::VInterpConstantTo(AvoidOffset, FVector::ZeroVector, DeltaSeconds, Settings->CrosshairAvoidSpeed * 0.5f);
		return; // behind the camera: nowhere near the crosshair
	}
	const FVector2D Center = FVector2D(SizeX, SizeY) * 0.5;
	const FVector2D FromCenter = Screen - Center;
	const float Distance = static_cast<float>(FromCenter.Size() / SizeY);
	if (Distance < Radius)
	{
		// Slide away from the center in screen space (straight up if it sits right on it).
		const FVector2D Away = FromCenter.IsNearlyZero() ? FVector2D(0.f, -1.f) : FromCenter.GetSafeNormal();
		const FRotator CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
		const FVector Push = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y) * Away.X - FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Z) * Away.Y;
		AvoidOffset = (AvoidOffset + Push * Settings->CrosshairAvoidSpeed * DeltaSeconds).GetClampedToMaxSize(Settings->CrosshairMaxAvoid);
	}
	else if (Distance > Radius * 1.5f)
	{
		// Well clear: ease back to the shoulder (the gap between 1x and 1.5x the radius stops it from jittering).
		AvoidOffset = FMath::VInterpConstantTo(AvoidOffset, FVector::ZeroVector, DeltaSeconds, Settings->CrosshairAvoidSpeed * 0.5f);
	}
}

// ---------------------------------------------------------------------------
// Targeting and shooting (server)
// ---------------------------------------------------------------------------

bool AGSFloatingGun::CanShoot() const
{
	const AGSCharacter* Character = OwnerCharacter.Get();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !Settings)
	{
		return false;
	}
	const UGSHealthComponent* Health = Character->GetHealthComponent();
	return !(Health && Health->IsDead())
		&& !ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead)
		&& !ASC->HasMatchingGameplayTag(GSGameplayTags::State_SafeZone);
}

bool AGSFloatingGun::HasClearLine(const AActor* Candidate) const
{
	// World geometry blocks the line; other enemies and the player don't (pawns aren't queried).
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFloatingGunLine), false, this);
	Params.AddIgnoredActor(OwnerCharacter.Get());
	Params.AddIgnoredActor(Candidate);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByObjectType(Hit, GetActorLocation(), Candidate->GetActorLocation(), Objects, Params);
}

AGSEnemyCharacter* AGSFloatingGun::FindTarget() const
{
	if (!Settings)
	{
		return nullptr;
	}
	const FVector From = GetActorLocation();
	const float RangeSq = FMath::Square(Settings->Range);
	TArray<TPair<float, AGSEnemyCharacter*>> Candidates;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const UGSHealthComponent* Health = It->GetHealthComponent();
		const float DistanceSq = FVector::DistSquared(It->GetActorLocation(), From);
		if (Health && !Health->IsDead() && DistanceSq <= RangeSq)
		{
			Candidates.Emplace(DistanceSq, *It);
		}
	}
	Candidates.Sort([](const TPair<float, AGSEnemyCharacter*>& A, const TPair<float, AGSEnemyCharacter*>& B) { return A.Key < B.Key; });
	for (const TPair<float, AGSEnemyCharacter*>& Candidate : Candidates)
	{
		if (HasClearLine(Candidate.Value))
		{
			return Candidate.Value;
		}
	}
	return nullptr;
}

void AGSFloatingGun::UpdateShooting(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const double Interval = Settings->GetShotInterval();

	auto StopShooting = [this, Now]()
	{
		Target.Reset();
		bShooting = false;
		// Nothing is banked while idle: the next target is shot at once, but never sooner than one interval after the
		// last shot.
		NextShotTime = FMath::Max(NextShotTime, Now);
	};
	if (!CanShoot())
	{
		StopShooting();
		return;
	}

	auto IsValidTarget = [this](const AGSEnemyCharacter* Candidate)
	{
		const UGSHealthComponent* Health = Candidate ? Candidate->GetHealthComponent() : nullptr;
		return Health && !Health->IsDead()
			&& FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation()) <= FMath::Square(Settings->Range)
			&& HasClearLine(Candidate);
	};
	TargetRefreshRemaining -= DeltaSeconds;
	if (TargetRefreshRemaining <= 0.f || !IsValidTarget(Target.Get()))
	{
		Target = FindTarget(); // the nearest one
		TargetRefreshRemaining = Settings->TargetRefreshInterval;
	}
	if (!Target.IsValid())
	{
		StopShooting();
		return;
	}

	if (!bShooting)
	{
		bShooting = true;
		NextShotTime = FMath::Max(NextShotTime, Now);
		BurstStartTime = NextShotTime;
		BurstShots = 0;
	}
	// Every shot due by now, each on its own scheduled time (a long frame fires all it owes).
	for (int32 Guard = 0; NextShotTime <= Now && Guard < MaxShotsPerFrame; ++Guard)
	{
		if (!IsValidTarget(Target.Get()))
		{
			Target = FindTarget(); // the last shot killed it
			if (!Target.IsValid())
			{
				StopShooting();
				return;
			}
		}
		FireAt(Target.Get(), static_cast<float>(Now - NextShotTime));
		NextShotTime += Interval;
		++BurstShots;
	}
}

void AGSFloatingGun::FireAt(AGSEnemyCharacter* AtTarget, float Lateness)
{
	AGSCharacter* Character = OwnerCharacter.Get();
	UAbilitySystemComponent* SourceASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>();
	if (!AtTarget || !SourceASC || !Projectiles)
	{
		return;
	}

	const FVector Muzzle = GetMuzzleLocation();
	FVector AimPoint = AtTarget->GetActorLocation();
	if (Settings->bLeadTargets)
	{
		// One step of lead: where the target will be when a shot fired now gets there.
		const float FlightTime = FVector::Dist(Muzzle, AimPoint) / Settings->ProjectileSpeed;
		AimPoint += AtTarget->GetVelocity() * FlightTime;
	}

	FGSProjectileLaunch Launch;
	Launch.Direction = (AimPoint - Muzzle).GetSafeNormal();
	// A late shot (fired in a frame after its scheduled time) starts as far along as it would be by now.
	const float Late = FMath::Clamp(Lateness, 0.f, Settings->ProjectileLifetime);
	Launch.Start = Muzzle + Launch.Direction * Settings->ProjectileSpeed * Late;
	Launch.Speed = Settings->ProjectileSpeed;
	Launch.Lifetime = FMath::Max(Settings->ProjectileLifetime - Late, 0.01f);
	Launch.Radius = Settings->ProjectileRadius;
	Launch.Damage = Settings->Damage * SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
	Launch.SourceASC = SourceASC;
	Launch.DamageEffect = Settings->DamageEffect;
	Launch.Mesh = Settings->ProjectileMesh;
	Launch.Material = Settings->ProjectileMaterial;
	Launch.Scale = Settings->ProjectileScale;
	Launch.ImpactEffect = Settings->ImpactEffect;
	Launch.ImpactEffectScale = Settings->ImpactEffectScale;
	Projectiles->FireProjectile(Launch);
	++ShotsFired;

	// Placeholder sound (local: single-player). TODO(co-op): play it on every machine.
	if (Settings->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->FireSound, Muzzle);
	}
	UE_LOG(LogBanditBaller, Verbose, TEXT("[gun] %s shot %d at %s (%.0f damage, %.3f s late)"), *GetNameSafe(this), ShotsFired,
		*GetNameSafe(AtTarget), Launch.Damage, Lateness);
}
