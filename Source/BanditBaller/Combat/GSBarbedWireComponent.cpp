// BanditBaller

#include "GSBarbedWireComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSBarbedWireSettings.h"
#include "GSCharacter.h"
#include "GSCriticalHit.h"
#include "GSEnemyCharacter.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "NiagaraFunctionLibrary.h"

namespace
{
	/** At most this many owed ticks are run in one frame (a runaway guard). */
	constexpr int32 MaxTicksPerFrame = 16;
}

UGSBarbedWireComponent::UGSBarbedWireComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // the ring follows the character after it moved this frame
}

AGSCharacter* UGSBarbedWireComponent::GetCharacter() const
{
	return Cast<AGSCharacter>(GetOwner());
}

const UGSBarbedWireSettings* UGSBarbedWireComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetBarbedWireSettings() : nullptr;
}

int32 UGSBarbedWireComponent::GetLevel() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetBarbedWireLevelAttribute()))) : 0;
}

float UGSBarbedWireComponent::GetPower() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetBarbedWirePowerAttribute())) : 0.f;
}

float UGSBarbedWireComponent::GetRadius() const
{
	const UGSBarbedWireSettings* Settings = GetSettings();
	return Settings ? Settings->GetRadius(GetPower()) : 0.f;
}

float UGSBarbedWireComponent::GetDamageMultiplier() const
{
	const UGSBarbedWireSettings* Settings = GetSettings();
	return Settings ? Settings->GetDamageMultiplier(GetPower()) : 1.f;
}

float UGSBarbedWireComponent::GetTickDamage() const
{
	const UGSBarbedWireSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return Settings && ASC ? Settings->Damage * GetDamageMultiplier() * ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()) : 0.f;
}

FVector UGSBarbedWireComponent::GetRingCenter() const
{
	const AGSCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const UGSBarbedWireSettings* Settings = GetSettings();
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return Character->GetActorLocation() - FVector(0.f, 0.f, HalfHeight - (Settings ? Settings->HeightAboveFloor : 0.f));
}

bool UGSBarbedWireComponent::CanAct() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !GetSettings() || GetLevel() <= 0 || !Character->HasAuthority())
	{
		return false;
	}
	const UGSHealthComponent* Health = Character->GetHealthComponent();
	return !(Health && Health->IsDead())
		&& !ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead)
		&& !ASC->HasMatchingGameplayTag(GSGameplayTags::State_SafeZone);
}

bool UGSBarbedWireComponent::IsInside(const AGSEnemyCharacter* Enemy) const
{
	const UGSBarbedWireSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UGSHealthComponent* Health = Enemy ? Enemy->GetHealthComponent() : nullptr;
	if (!Settings || !Character || !Health || Health->IsDead())
	{
		return false;
	}
	const FVector Delta = Enemy->GetActorLocation() - Character->GetActorLocation();
	return Delta.Size2D() <= GetRadius() && FMath::Abs(Delta.Z) <= Settings->MaxHeightDifference;
}

bool UGSBarbedWireComponent::IsRingShown() const
{
	return Ring && Ring->IsVisible() && Ring->GetInstanceCount() > 0;
}

void UGSBarbedWireComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const UGSBarbedWireSettings* Settings = GetSettings();
	const int32 Level = GetLevel();
	const float Power = Level > 0 ? GetPower() : 0.f;
	if (Settings && (Level != LoggedLevel || !FMath::IsNearlyEqual(Power, LoggedPower)))
	{
		if (Level > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[wire] %s: Barbed Wire %d pick(s), power %g -> x%.2f damage (%.2f per tick), radius %.0f cm (x%.2f), every %.2f s"),
				*GetNameSafe(GetOwner()), Level, Power, Settings->GetDamageMultiplier(Power), Settings->Damage * Settings->GetDamageMultiplier(Power),
				Settings->GetRadius(Power), Settings->GetRadiusMultiplier(Power), Settings->TickInterval);
		}
		else if (LoggedLevel > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[wire] %s: Barbed Wire removed"), *GetNameSafe(GetOwner()));
		}
		LoggedLevel = Level;
		LoggedPower = Power;
	}

	UpdateRing();

	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanAct())
	{
		// Nothing is banked while it can't act: the next tick is due at once when it can again.
		bActive = false;
		NextTickTime = FMath::Max(NextTickTime, Now);
		return;
	}
	if (!bActive)
	{
		bActive = true;
		NextTickTime = FMath::Max(NextTickTime, Now);
		FirstTickTime = NextTickTime;
		ScheduledTicks = 0;
	}
	// Every tick due by now, each on its own scheduled time (a long frame runs all it owes).
	for (int32 Guard = 0; NextTickTime <= Now && Guard < MaxTicksPerFrame; ++Guard)
	{
		DamageTick();
		++ScheduledTicks;
		NextTickTime += Settings->TickInterval;
	}
}

void UGSBarbedWireComponent::UpdateRing()
{
	const UGSBarbedWireSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UGSHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
	const bool bWanted = Settings && Character && GetLevel() > 0 && !(Health && Health->IsDead());
	if (!bWanted)
	{
		if (Ring && Ring->IsVisible())
		{
			Ring->SetVisibility(false);
		}
		return;
	}

	if (!Ring)
	{
		// Visual only: no collision, no overlaps, no navigation; placed in world space each frame (it doesn't turn
		// with the character).
		Ring = NewObject<UInstancedStaticMeshComponent>(GetOwner(), TEXT("BarbedWireRing"));
		Ring->SetUsingAbsoluteLocation(true);
		Ring->SetUsingAbsoluteRotation(true);
		Ring->SetUsingAbsoluteScale(true);
		Ring->SetMobility(EComponentMobility::Movable);
		Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ring->SetGenerateOverlapEvents(false);
		Ring->SetCanEverAffectNavigation(false);
		Ring->SetCastShadow(false);
		Ring->SetupAttachment(GetOwner()->GetRootComponent());
		Ring->RegisterComponent();
		ShownRadius = 0.f;
	}
	if (Settings->SegmentMesh && Ring->GetStaticMesh() != Settings->SegmentMesh)
	{
		Ring->SetStaticMesh(Settings->SegmentMesh);
	}
	if (Settings->SegmentMaterial && Ring->GetMaterial(0) != Settings->SegmentMaterial)
	{
		Ring->SetMaterial(0, Settings->SegmentMaterial);
	}
	const float Radius = GetRadius();
	if (!FMath::IsNearlyEqual(Radius, ShownRadius, 0.1f) || Ring->GetInstanceCount() == 0)
	{
		BuildRing(Radius); // it grows with the radius
	}
	Ring->SetWorldLocationAndRotation(GetRingCenter(), FRotator::ZeroRotator);
	if (!Ring->IsVisible())
	{
		Ring->SetVisibility(true);
	}
}

void UGSBarbedWireComponent::BuildRing(float Radius)
{
	const UGSBarbedWireSettings* Settings = GetSettings();
	if (!Ring || !Settings)
	{
		return;
	}
	Ring->ClearInstances();
	const int32 Segments = FMath::Clamp(Settings->RingSegments, 8, 256);
	// The engine cube is 100 cm; each segment is a little longer than its share of the circle so they meet.
	const float SegmentLength = 2.f * PI * Radius / Segments * 1.08f;
	const FVector Scale(SegmentLength / 100.f, Settings->WireThickness / 100.f, Settings->WireThickness / 100.f);
	TArray<FTransform> Transforms;
	Transforms.Reserve(Segments);
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float Angle = 2.f * PI * Index / Segments;
		const FVector Location(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Settings->WireThickness * 0.5f);
		const FRotator Rotation(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f); // along the circle
		Transforms.Emplace(Rotation, Location, Scale);
	}
	Ring->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);
	ShownRadius = Radius;
}

void UGSBarbedWireComponent::DamageTick()
{
	AGSCharacter* Character = GetCharacter();
	UAbilitySystemComponent* SourceASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	const UGSBarbedWireSettings* Settings = GetSettings();
	if (!SourceASC || !Settings)
	{
		return;
	}
	++Ticks;

	// Everyone inside first, then the damage (a kill can spawn pickups or set off Powder Keg mid-loop).
	TArray<AGSEnemyCharacter*> Inside;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (IsInside(*It))
		{
			Inside.Add(*It);
		}
	}
	LastTickHits = 0;
	const float Damage = GetTickDamage();
	if (!Settings->DamageEffect || Damage <= 0.f)
	{
		return;
	}
	for (AGSEnemyCharacter* Enemy : Inside)
	{
		UAbilitySystemComponent* TargetASC = IsValid(Enemy) ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Enemy) : nullptr;
		if (!TargetASC || !IsInside(Enemy))
		{
			continue; // died this tick (a blast)
		}
		// Its own critical roll per enemy (Loaded Dice); GE_Damage from the player (kills are the player's).
		const FGSCritRoll Crit = GSCombat::RollCriticalHit(SourceASC);
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Settings->DamageEffect, 1.f, SourceASC->MakeEffectContext());
		if (!Spec.IsValid())
		{
			continue;
		}
		Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Damage * Crit.DamageMultiplier);
		if (Crit.bCritical)
		{
			Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Critical, Crit.GetLevel());
		}
		const FVector Where = Enemy->GetActorLocation();
		FHitResult Hit(Enemy, nullptr, Where, FVector::UpVector);
		Hit.ImpactPoint = Where;
		Spec.Data->GetContext().AddHitResult(Hit);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
		++LastTickHits;
		++HitsDealt;

		// Placeholder hit effect (local: single-player). TODO(co-op): play it on every machine.
		if (Settings->HitEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Settings->HitEffect, Where, FRotator::ZeroRotator, Settings->HitEffectScale);
		}
	}
	UE_LOG(LogBanditBaller, Verbose, TEXT("[wire] tick %d: %d enemies inside, %.1f damage each"), Ticks, LastTickHits, Damage);
}

void UGSBarbedWireComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Ring)
	{
		Ring->DestroyComponent();
		Ring = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
