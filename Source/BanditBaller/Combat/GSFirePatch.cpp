// BanditBaller

#include "GSFirePatch.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "GSCriticalHit.h"
#include "GSEnemyCharacter.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSMolotovComponent.h"
#include "GSWeaponDebug.h"

namespace
{
	/** At most this many owed burns in one frame (a runaway guard). */
	constexpr int32 MaxBurnsPerFrame = 16;
}

void AGSFirePatch::Ignite(const FGSFirePatchParams& InParams)
{
	Params = InParams;
	Params.BurnInterval = FMath::Max(Params.BurnInterval, 0.05f);
	Age = 0.f;
	Burns = 0;
	LastBurnHits = 0;
	if (Params.Mesh)
	{
		Mesh->SetStaticMesh(Params.Mesh);
	}
	if (Params.Material)
	{
		Mesh->SetMaterial(0, Params.Material);
	}
	// The engine cylinder is 100 cm wide and 100 cm tall, its pivot in the middle.
	SetActorScale3D(FVector(Params.Radius / 50.f, Params.Radius / 50.f, FMath::Max(Params.Thickness, 0.1f) / 100.f));
	SetActorLocationAndRotation(Params.Center + FVector(0.f, 0.f, Params.HeightAboveFloor + Params.Thickness * 0.5f), FRotator::ZeroRotator);
	Burn(); // the first burn when it lands
}

bool AGSFirePatch::IsInside(const AGSEnemyCharacter* Enemy) const
{
	const UGSHealthComponent* Health = IsValid(Enemy) ? Enemy->GetHealthComponent() : nullptr;
	if (!Health || Health->IsDead())
	{
		return false;
	}
	const FVector Delta = Enemy->GetActorLocation() - Params.Center;
	return Delta.Size2D() <= Params.Radius && FMath::Abs(Delta.Z) <= Params.MaxHeightDifference;
}

void AGSFirePatch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}
	Age += DeltaSeconds;
	// Every burn due by now on its own scheduled time (a long frame runs all it owes); the last one is before Duration.
	for (int32 Guard = 0; Guard < MaxBurnsPerFrame; ++Guard)
	{
		const float NextBurn = Burns * Params.BurnInterval;
		if (NextBurn > Age + KINDA_SMALL_NUMBER || NextBurn >= Params.Duration - KINDA_SMALL_NUMBER)
		{
			break;
		}
		Burn();
	}
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugCircle(GetWorld(), Params.Center, Params.Radius, 32, FColor::Orange, false, -1.f, 0, 2.f, FVector::ForwardVector, FVector::RightVector, false);
	}
	if (Age >= Params.Duration)
	{
		ReleaseToPool();
	}
}

void AGSFirePatch::Burn()
{
	++Burns; // a burn that is skipped (the player can't act) still uses its slot: nothing is banked
	LastBurnHits = 0;
	UGSMolotovComponent* Molotov = Params.Owner.Get();
	UAbilitySystemComponent* SourceASC = Params.SourceASC.Get();
	if (!Molotov || !Molotov->CanBurn() || !SourceASC || !Params.DamageEffect || Params.Damage <= 0.f)
	{
		return;
	}

	// Everyone inside first, then the damage (a kill can spawn pickups or set off Powder Keg mid-loop).
	TArray<AGSEnemyCharacter*> Inside;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (IsInside(*It))
		{
			Inside.Add(*It);
		}
	}
	for (AGSEnemyCharacter* Enemy : Inside)
	{
		UAbilitySystemComponent* TargetASC = IsInside(Enemy) ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Enemy) : nullptr;
		if (!TargetASC)
		{
			continue; // died this burn (a Powder Keg blast)
		}
		// Its own critical roll per enemy (Loaded Dice); GE_Damage from the player (kills are the player's).
		const FGSCritRoll Crit = GSCombat::RollCriticalHit(SourceASC);
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Params.DamageEffect, 1.f, SourceASC->MakeEffectContext());
		if (!Spec.IsValid())
		{
			continue;
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
		const FVector Where = Enemy->GetActorLocation();
		FHitResult Hit(Enemy, nullptr, Where, FVector::UpVector);
		Hit.ImpactPoint = Where;
		Spec.Data->GetContext().AddHitResult(Hit);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
		++LastBurnHits;
	}
	Molotov->NotifyPatchBurned(LastBurnHits);
}
