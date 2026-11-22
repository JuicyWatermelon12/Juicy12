// BanditBaller

#include "GSProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GSCriticalHit.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSProjectileSubsystem.h"
#include "GSWeaponDebug.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"

AGSProjectile::AGSProjectile()
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

	SetActorHiddenInGame(true);
}

void AGSProjectile::Launch(const FGSProjectileLaunch& InLaunch)
{
	Params = InLaunch;
	Velocity = InLaunch.Direction.GetSafeNormal() * InLaunch.Speed;
	Age = 0.f;
	bActive = true;
	EndLocation = InLaunch.Start;

	if (InLaunch.Mesh)
	{
		Mesh->SetStaticMesh(InLaunch.Mesh);
	}
	if (InLaunch.Material)
	{
		Mesh->SetMaterial(0, InLaunch.Material);
	}
	SetActorScale3D(InLaunch.Scale);
	SetActorLocationAndRotation(InLaunch.Start, Velocity.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
}

void AGSProjectile::Deactivate()
{
	bActive = false;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	Params.SourceASC.Reset();
}

void AGSProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}

	// Don't fly past the end of the lifetime this frame.
	const float Step = FMath::Min(DeltaSeconds, FMath::Max(Params.Lifetime - Age, 0.f));
	Age += DeltaSeconds;
	const FVector From = GetActorLocation();
	const FVector To = From + Velocity * Step;

	// World geometry and pawns; the map boundary (its own object type), pickups (no collision) and ragdolls
	// (PhysicsBody) are not queried, so they are passed.
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GSProjectile), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, From, To, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Params.Radius), QueryParams);
	Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });
	for (const FHitResult& Hit : Hits)
	{
		if (HandleHit(Hit))
		{
			return;
		}
	}

	SetActorLocation(To);
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugLine(GetWorld(), From, To, FColor::Yellow, false, GSWeaponDebug::DrawDuration);
	}
	if (Age >= Params.Lifetime)
	{
		Finish(EGSProjectileEnd::Expired, To);
	}
}

bool AGSProjectile::HandleHit(const FHitResult& Hit)
{
	AActor* Actor = Hit.GetActor();
	const FVector Where = Hit.bStartPenetrating ? FVector(Hit.TraceStart) : FVector(Hit.Location);
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (Pawn->IsPlayerControlled())
		{
			return false; // never hits a player (the shooter included)
		}
	}

	UAbilitySystemComponent* TargetASC = Actor ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor) : nullptr;
	if (!TargetASC)
	{
		if (Actor && Actor->IsA<APawn>())
		{
			return false; // a pawn with nothing to damage: fly on
		}
		Finish(EGSProjectileEnd::HitWorld, Where); // walls, floors, cover
		return true;
	}
	const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Actor);
	if (Health && Health->IsDead())
	{
		return false; // corpses don't stop it
	}

	// A living target: its own critical roll (Loaded Dice), GE_Damage from the shooter (kills are the player's).
	UAbilitySystemComponent* SourceASC = Params.SourceASC.Get();
	if (SourceASC && Params.DamageEffect && Params.Damage > 0.f)
	{
		const FGSCritRoll Crit = GSCombat::RollCriticalHit(SourceASC);
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Params.DamageEffect, 1.f, SourceASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Params.Damage * Crit.DamageMultiplier);
			if (Crit.bCritical)
			{
				Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Critical, Crit.GetLevel()); // 1 crit, 2 triple
			}
			FHitResult ContextHit = Hit;
			ContextHit.ImpactPoint = Where;
			Spec.Data->GetContext().AddHitResult(ContextHit);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
		}
	}
	else if (!Params.DamageEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[gun] Projectile has no DamageEffect; the hit on %s did nothing."), *GetNameSafe(Actor));
	}
	Finish(EGSProjectileEnd::HitTarget, Where);
	return true;
}

void AGSProjectile::Finish(EGSProjectileEnd Reason, const FVector& Where)
{
	EndLocation = Where;
	if (Reason != EGSProjectileEnd::Expired && Params.ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Params.ImpactEffect, Where, FRotator::ZeroRotator, Params.ImpactEffectScale);
	}
	if (UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>())
	{
		Projectiles->ReleaseProjectile(this, Reason);
	}
	else
	{
		Deactivate();
	}
}
