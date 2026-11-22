// BanditBaller

#include "GSTomahawk.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"
#include "GSCriticalHit.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSTomahawkComponent.h"
#include "GSWeaponDebug.h"
#include "NiagaraFunctionLibrary.h"

void AGSTomahawk::Launch(const FGSTomahawkLaunch& InLaunch)
{
	Params = InLaunch;
	Params.Direction = FVector(InLaunch.Direction.X, InLaunch.Direction.Y, 0.f).GetSafeNormal();
	if (Params.Direction.IsNearlyZero())
	{
		Params.Direction = FVector::ForwardVector;
	}
	bReturning = false;
	bTurnedAtWall = false;
	Travelled = 0.f;
	Age = 0.f;
	TurnPoint = Params.Start;
	HitOut.Reset();
	HitBack.Reset();
	if (Params.Mesh)
	{
		Mesh->SetStaticMesh(Params.Mesh);
	}
	if (Params.Material)
	{
		Mesh->SetMaterial(0, Params.Material);
	}
	SetActorScale3D(Params.Scale);
	SetActorLocationAndRotation(Params.Start, Params.Direction.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (Params.AlreadyFlown > 0.f)
	{
		Age = Params.AlreadyFlown;
		StepOut(Params.Speed * Params.AlreadyFlown); // a late throw starts that far along
	}
}

void AGSTomahawk::DeactivateThrown()
{
	Params.SourceASC.Reset();
	Params.ReturnTo.Reset();
	Super::DeactivateThrown();
}

void AGSTomahawk::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}
	Age += DeltaSeconds;
	const float Step = Params.Speed * DeltaSeconds;
	if (!bReturning)
	{
		StepOut(Step);
	}
	else
	{
		StepBack(Step);
	}
	if (bActive && Age >= Params.MaxFlightTime)
	{
		Finish(); // safety: never stays out longer than this
		return;
	}
	if (bActive)
	{
		const float Yaw = (bReturning ? -Params.Direction : Params.Direction).Rotation().Yaw;
		SetActorRotation(FRotator(FMath::Fmod(Age * Params.SpinRate, 360.f), Yaw, 0.f)); // end over end
	}
}

void AGSTomahawk::StepOut(float Step)
{
	const float Left = FMath::Max(Params.Distance - Travelled, 0.f);
	const float Move = FMath::Min(Step, Left);
	const FVector From = GetActorLocation();
	const FVector To = From + Params.Direction * Move;

	// Enemies and world geometry: every enemy it passes is hit once; the first wall turns it back.
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(GSTomahawkOut), false, this);
	if (AActor* Player = Params.ReturnTo.Get())
	{
		Query.AddIgnoredActor(Player);
	}
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, From, To, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Params.SweepRadius), Query);
	Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });
	for (const FHitResult& Hit : Hits)
	{
		AActor* Actor = Hit.GetActor();
		const FVector Where = Hit.bStartPenetrating ? FVector(Hit.TraceStart) : FVector(Hit.Location);
		const APawn* Pawn = Cast<APawn>(Actor);
		if (Pawn && Pawn->IsPlayerControlled())
		{
			continue; // never hits a player
		}
		if (!Actor || !UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
		{
			if (Pawn)
			{
				continue; // a pawn with nothing to damage
			}
			// A wall: turn back here, and use the rest of this step on the way back.
			bTurnedAtWall = true;
			bReturning = true;
			TurnPoint = Where;
			Travelled += FVector::Dist(From, Where);
			SetActorLocation(Where);
			if (GSWeaponDebug::IsEnabled())
			{
				DrawDebugLine(GetWorld(), From, Where, FColor::Silver, false, GSWeaponDebug::DrawDuration);
			}
			StepBack(FMath::Max(Step - FVector::Dist(From, Where), 0.f));
			return;
		}
		TryHit(Actor, Where, HitOut);
	}
	SetActorLocation(To);
	Travelled += Move;
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugLine(GetWorld(), From, To, FColor::Silver, false, GSWeaponDebug::DrawDuration);
	}
	if (Travelled >= Params.Distance - KINDA_SMALL_NUMBER)
	{
		bReturning = true;
		TurnPoint = To;
		StepBack(FMath::Max(Step - Move, 0.f));
	}
}

void AGSTomahawk::StepBack(float Step)
{
	const AActor* Player = Params.ReturnTo.Get();
	if (!Player)
	{
		Finish();
		return;
	}
	const FVector From = GetActorLocation();
	const FVector Home = Player->GetActorLocation();
	const float Distance = FVector::Dist(From, Home);
	const bool bCaught = Distance <= Params.CatchDistance + Step;
	const FVector To = bCaught ? Home : From + (Home - From).GetSafeNormal() * Step;

	// Only enemies on the way back: it passes through walls so it always comes home.
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(GSTomahawkBack), false, this);
	Query.AddIgnoredActor(Player);
	TArray<FHitResult> Hits;
	if (Step > 0.f)
	{
		GetWorld()->SweepMultiByObjectType(Hits, From, To, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Params.SweepRadius), Query);
		Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });
	}
	for (const FHitResult& Hit : Hits)
	{
		AActor* Actor = Hit.GetActor();
		const APawn* Pawn = Cast<APawn>(Actor);
		if (!Actor || (Pawn && Pawn->IsPlayerControlled()))
		{
			continue;
		}
		TryHit(Actor, Hit.bStartPenetrating ? FVector(Hit.TraceStart) : FVector(Hit.Location), HitBack);
	}
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugLine(GetWorld(), From, To, FColor::White, false, GSWeaponDebug::DrawDuration);
	}
	if (bCaught)
	{
		Finish();
		return;
	}
	SetActorLocation(To);
}

bool AGSTomahawk::TryHit(AActor* Actor, const FVector& Where, TArray<TWeakObjectPtr<AActor>>& HitThisLeg)
{
	if (HitThisLeg.Contains(Actor))
	{
		return false; // at most once per leg
	}
	const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Actor);
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	if (!TargetASC || (Health && Health->IsDead()))
	{
		return false; // corpses don't count
	}
	HitThisLeg.Add(Actor);
	UAbilitySystemComponent* SourceASC = Params.SourceASC.Get();
	if (!SourceASC || !Params.DamageEffect || Params.Damage <= 0.f)
	{
		return false;
	}
	// Its own critical roll per hit (Loaded Dice); GE_Damage from the player (kills are the player's).
	const FGSCritRoll Crit = GSCombat::RollCriticalHit(SourceASC);
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Params.DamageEffect, 1.f, SourceASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return false;
	}
	Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Params.Damage * Crit.DamageMultiplier);
	if (Params.DamageSource.IsValid())
	{
		Spec.Data->AddDynamicAssetTag(Params.DamageSource); // run stats by source
	}
	if (Crit.bCritical)
	{
		Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Critical, Crit.GetLevel());
	}
	FHitResult Hit(Actor, nullptr, Where, FVector::UpVector);
	Hit.ImpactPoint = Where;
	Spec.Data->GetContext().AddHitResult(Hit);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	if (UGSTomahawkComponent* Thrower = Params.Owner.Get())
	{
		Thrower->NotifyTomahawkHit();
	}
	// Placeholder hit effect (local: single-player). TODO(co-op): play it on every machine.
	if (Params.HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Params.HitEffect, Where, FRotator::ZeroRotator, Params.HitEffectScale);
	}
	return true;
}

void AGSTomahawk::Finish()
{
	ReleaseToPool();
}
