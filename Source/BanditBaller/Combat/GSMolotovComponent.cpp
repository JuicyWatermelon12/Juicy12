// BanditBaller

#include "GSMolotovComponent.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSEnemyCharacter.h"
#include "GSFirePatch.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSLobbedThrow.h"
#include "GSMolotovSettings.h"
#include "GSThrownSubsystem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** At most this many owed throws in one frame (a runaway guard). */
	constexpr int32 MaxMolotovThrowsPerFrame = 8;
}

UGSMolotovComponent::UGSMolotovComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // throw from where the character is after it moved this frame
}

AGSCharacter* UGSMolotovComponent::GetCharacter() const
{
	return Cast<AGSCharacter>(GetOwner());
}

const UGSMolotovSettings* UGSMolotovComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetMolotovSettings() : nullptr;
}

int32 UGSMolotovComponent::GetLevel() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetMolotovLevelAttribute()))) : 0;
}

float UGSMolotovComponent::GetPower() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetMolotovPowerAttribute())) : 0.f;
}

float UGSMolotovComponent::GetDamageMultiplier() const
{
	const UGSMolotovSettings* Settings = GetSettings();
	return Settings ? Settings->GetDamageMultiplier(GetPower()) : 1.f;
}

float UGSMolotovComponent::GetPatchRadius() const
{
	const UGSMolotovSettings* Settings = GetSettings();
	return Settings ? Settings->GetPatchRadius(GetPower()) : 0.f;
}

float UGSMolotovComponent::GetBurnDamage() const
{
	const UGSMolotovSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return Settings && ASC ? Settings->BurnDamage * GetDamageMultiplier() * ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()) : 0.f;
}

double UGSMolotovComponent::GetThrowInterval() const
{
	const UGSMolotovSettings* Settings = GetSettings();
	return Settings ? FMath::Max(static_cast<double>(Settings->ThrowInterval), 0.05) : 0.0;
}

FVector UGSMolotovComponent::GetThrowOrigin() const
{
	const AGSCharacter* Character = GetCharacter();
	const UGSMolotovSettings* Settings = GetSettings();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const FVector Offset = Settings ? Settings->ThrowOffset : FVector::ZeroVector;
	return Character->GetActorLocation() + FRotator(0.f, Character->GetActorRotation().Yaw, 0.f).RotateVector(Offset);
}

bool UGSMolotovComponent::CanBurn() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !Character->HasAuthority())
	{
		return false;
	}
	const UGSHealthComponent* Health = Character->GetHealthComponent();
	return !(Health && Health->IsDead())
		&& !ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead)
		&& !ASC->HasMatchingGameplayTag(GSGameplayTags::State_SafeZone);
}

bool UGSMolotovComponent::CanThrow() const
{
	return GetSettings() && GetLevel() > 0 && CanBurn();
}

bool UGSMolotovComponent::IsInRange(const AGSEnemyCharacter* Enemy) const
{
	const UGSMolotovSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UGSHealthComponent* Health = IsValid(Enemy) ? Enemy->GetHealthComponent() : nullptr;
	return Settings && Character && Health && !Health->IsDead()
		&& FVector::DistSquared(Enemy->GetActorLocation(), Character->GetActorLocation()) <= FMath::Square(Settings->Range);
}

AGSEnemyCharacter* UGSMolotovComponent::FindTarget() const
{
	const AGSCharacter* Character = GetCharacter();
	if (!Character || !GetSettings())
	{
		return nullptr;
	}
	AGSEnemyCharacter* Best = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const float DistanceSq = FVector::DistSquared(It->GetActorLocation(), Character->GetActorLocation());
		if (IsInRange(*It) && DistanceSq < BestDistanceSq)
		{
			Best = *It;
			BestDistanceSq = DistanceSq;
		}
	}
	return Best;
}

TArray<AGSFirePatch*> UGSMolotovComponent::GetActivePatches() const
{
	TArray<AGSFirePatch*> Patches;
	if (const UGSThrownSubsystem* Thrown = GetWorld() ? GetWorld()->GetSubsystem<UGSThrownSubsystem>() : nullptr)
	{
		for (AGSThrownActor* Actor : Thrown->GetActive())
		{
			AGSFirePatch* Patch = Cast<AGSFirePatch>(Actor);
			if (Patch && Patch->GetParams().Owner.Get() == this)
			{
				Patches.Add(Patch);
			}
		}
	}
	return Patches;
}

void UGSMolotovComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const UGSMolotovSettings* Settings = GetSettings();
	const int32 Level = GetLevel();
	const float Power = Level > 0 ? GetPower() : 0.f;
	if (Settings && (Level != LoggedLevel || !FMath::IsNearlyEqual(Power, LoggedPower)))
	{
		if (Level > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[molotov] %s: Molotov %d pick(s), power %g -> x%.2f damage (%.2f per burn), patch radius %.0f cm (x%.2f), a burn every %.2f s for %.1f s, a throw every %.2f s"),
				*GetNameSafe(GetOwner()), Level, Power, Settings->GetDamageMultiplier(Power), Settings->BurnDamage * Settings->GetDamageMultiplier(Power),
				Settings->GetPatchRadius(Power), Settings->GetRadiusMultiplier(Power), Settings->BurnInterval, Settings->PatchDuration, Settings->ThrowInterval);
		}
		else if (LoggedLevel > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[molotov] %s: Molotov removed"), *GetNameSafe(GetOwner()));
		}
		LoggedLevel = Level;
		LoggedPower = Power;
	}
	UpdateThrowing(DeltaTime);
}

void UGSMolotovComponent::StopThrowing(double Now)
{
	bThrowing = false;
	bTargetInRange = false;
	// Nothing is banked while idle: the next throw comes at once when an enemy is in range, but never sooner than one
	// interval after the last throw.
	NextThrowTime = FMath::Max(NextThrowTime, Now);
}

void UGSMolotovComponent::UpdateThrowing(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanThrow())
	{
		StopThrowing(Now);
		return;
	}
	const double Interval = GetThrowInterval();

	TargetRefreshRemaining -= DeltaSeconds;
	if (TargetRefreshRemaining <= 0.f || !bTargetInRange)
	{
		bTargetInRange = FindTarget() != nullptr;
		TargetRefreshRemaining = GetSettings()->TargetRefreshInterval;
	}
	if (!bTargetInRange)
	{
		StopThrowing(Now);
		return;
	}

	if (!bThrowing)
	{
		bThrowing = true;
		NextThrowTime = FMath::Max(NextThrowTime, Now);
		BurstStartTime = NextThrowTime;
		BurstShots = 0;
	}
	// Every throw due by now, each on its own scheduled time (a long frame throws all it owes).
	for (int32 Guard = 0; NextThrowTime <= Now && Guard < MaxMolotovThrowsPerFrame; ++Guard)
	{
		AGSEnemyCharacter* Target = FindTarget();
		if (!Target)
		{
			StopThrowing(Now);
			return;
		}
		ThrowAt(Target, static_cast<float>(Now - NextThrowTime));
		NextThrowTime += Interval;
		++BurstShots;
	}
}

void UGSMolotovComponent::ThrowAt(AGSEnemyCharacter* AtTarget, float Lateness)
{
	const UGSMolotovSettings* Settings = GetSettings();
	UGSThrownSubsystem* Thrown = GetWorld()->GetSubsystem<UGSThrownSubsystem>();
	if (!AtTarget || !Settings || !Thrown)
	{
		return;
	}
	AGSLobbedThrow* Bottle = Thrown->Acquire<AGSLobbedThrow>();
	if (!Bottle)
	{
		return;
	}

	// It lands at the target's feet where it will be when it comes down (its flat velocity x the flight time left x
	// LeadFactor, on the ground there); damage and radius are set at the throw.
	const float AlreadyFlown = FMath::Clamp(Lateness, 0.f, Settings->FlightTime); // a late throw starts that far along its arc
	const FGSLobAim Aim = AGSLobbedThrow::AimAt(AtTarget, Settings->FlightTime - AlreadyFlown, Settings->LeadFactor);
	const FVector Landing = Aim.Landing;
	const float Radius = GetPatchRadius();
	const float Damage = GetBurnDamage();
	LastTarget = AtTarget;
	LastLanding = Landing;
	bLastThrowLed = Aim.bLed;

	FGSLobLaunch Lob;
	Lob.Start = GetThrowOrigin();
	Lob.Landing = Landing;
	Lob.FlightTime = Settings->FlightTime;
	Lob.ArcHeight = Settings->ArcHeight;
	Lob.AlreadyFlown = AlreadyFlown;
	Lob.Mesh = Settings->BottleMesh;
	Lob.Material = Settings->BottleMaterial;
	Lob.Scale = Settings->BottleScale;
	Lob.SpinRate = Settings->BottleSpinRate;
	TWeakObjectPtr<UGSMolotovComponent> WeakThis(this);
	TWeakObjectPtr<AGSEnemyCharacter> WeakTarget(AtTarget);
	Lob.OnLanded = [WeakThis, WeakTarget, Radius, Damage](const FVector& Where)
	{
		if (UGSMolotovComponent* Self = WeakThis.Get())
		{
			Self->IgnitePatch(Where, Radius, Damage, WeakTarget.Get());
		}
	};
	Bottle->Launch(MoveTemp(Lob));
	++Throws;

	// Placeholder sound (local: single-player). TODO(co-op): play it on every machine.
	if (Settings->ThrowSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->ThrowSound, GetThrowOrigin());
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[molotov] bottle %d at %s (moving %.0f cm/s), %.2f per burn in %.0f cm, %.3f s late, %s"), Throws, *GetNameSafe(AtTarget),
		AtTarget->GetVelocity().Size2D(), Damage, Radius, Lateness,
		Aim.bLed ? *FString::Printf(TEXT("led %.0f cm ahead"), FVector::Dist2D(Aim.Landing, AtTarget->GetActorLocation())) : TEXT("not led"));
}

AGSFirePatch* UGSMolotovComponent::IgnitePatch(const FVector& FloorPoint)
{
	return IgnitePatch(FloorPoint, GetPatchRadius(), GetBurnDamage());
}

AGSFirePatch* UGSMolotovComponent::IgnitePatch(const FVector& FloorPoint, float Radius, float Damage, const AGSEnemyCharacter* Target)
{
	const UGSMolotovSettings* Settings = GetSettings();
	UGSThrownSubsystem* Thrown = GetWorld() ? GetWorld()->GetSubsystem<UGSThrownSubsystem>() : nullptr;
	const AGSCharacter* Character = GetCharacter();
	if (!Settings || !Thrown || !Character)
	{
		return nullptr;
	}
	AGSFirePatch* Patch = Thrown->Acquire<AGSFirePatch>();
	if (!Patch)
	{
		return nullptr;
	}
	FGSFirePatchParams Params;
	Params.Center = FloorPoint;
	Params.Radius = Radius;
	Params.MaxHeightDifference = Settings->MaxHeightDifference;
	Params.Damage = Damage;
	Params.BurnInterval = Settings->BurnInterval;
	Params.Duration = Settings->PatchDuration;
	Params.SourceASC = Character->GetAbilitySystemComponent();
	Params.DamageEffect = Settings->DamageEffect;
	Params.DamageSource = GSGameplayTags::Damage_Source_Molotov;
	Params.Owner = this;
	Params.Mesh = Settings->PatchMesh;
	Params.Material = Settings->PatchMaterial;
	Params.Thickness = Settings->PatchThickness;
	Params.HeightAboveFloor = Settings->PatchHeightAboveFloor;
	// Whether the enemy it was thrown at stands in the fire at the first burn (Ignite burns at once; checked first, in
	// case that burn kills it). A target that died before the landing counts as missed.
	const UGSHealthComponent* TargetHealth = IsValid(Target) ? Target->GetHealthComponent() : nullptr;
	const FVector ToTarget = TargetHealth ? Target->GetActorLocation() - FloorPoint : FVector::ZeroVector;
	bLastPatchHitTarget = TargetHealth && !TargetHealth->IsDead() && ToTarget.Size2D() <= Radius && FMath::Abs(ToTarget.Z) <= Params.MaxHeightDifference;
	FirstBurnTargetHits += bLastPatchHitTarget ? 1 : 0;
	++PatchesLit;
	Patch->Ignite(Params); // the first burn happens here

	// Placeholder sound (local: single-player). TODO(co-op): play it on every machine.
	if (Settings->LandSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->LandSound, FloorPoint);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[molotov] patch %d at %s: radius %.0f, %.2f per burn every %.2f s for %.1f s, %d enemies in it%s"),
		PatchesLit, *FloorPoint.ToCompactString(), Radius, Damage, Params.BurnInterval, Params.Duration, Patch->GetLastBurnHits(),
		Target ? (bLastPatchHitTarget ? TEXT(", target in it") : TEXT(", target missed")) : TEXT(""));
	return Patch;
}

int32 UGSMolotovComponent::SpawnPatchesForDebug(int32 Count)
{
	const AGSCharacter* Character = GetCharacter();
	if (!Character || !GetSettings())
	{
		return 0;
	}
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
	int32 Lit = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = 2.f * PI * Index / FMath::Max(Count, 1);
		Lit += IgnitePatch(Feet + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 400.f) ? 1 : 0;
	}
	return Lit;
}

void UGSMolotovComponent::NotifyPatchBurned(int32 Hits)
{
	++Burns;
	HitsDealt += Hits;
}
