// BanditBaller

#include "GSTomahawkComponent.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSEnemyCharacter.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSThrownSubsystem.h"
#include "GSTomahawk.h"
#include "GSTomahawkSettings.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** At most this many owed throws in one frame (a runaway guard). */
	constexpr int32 MaxTomahawkThrowsPerFrame = 8;
}

UGSTomahawkComponent::UGSTomahawkComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // throw from where the character is after it moved this frame
}

AGSCharacter* UGSTomahawkComponent::GetCharacter() const
{
	return Cast<AGSCharacter>(GetOwner());
}

const UGSTomahawkSettings* UGSTomahawkComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetTomahawkSettings() : nullptr;
}

int32 UGSTomahawkComponent::GetLevel() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetTomahawkLevelAttribute()))) : 0;
}

float UGSTomahawkComponent::GetPower() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetTomahawkPowerAttribute())) : 0.f;
}

float UGSTomahawkComponent::GetDamageMultiplier() const
{
	const UGSTomahawkSettings* Settings = GetSettings();
	return Settings ? Settings->GetDamageMultiplier(GetPower()) : 1.f;
}

int32 UGSTomahawkComponent::GetTomahawksPerThrow() const
{
	const UGSTomahawkSettings* Settings = GetSettings();
	return Settings ? Settings->GetTomahawks(GetPower()) : 0;
}

float UGSTomahawkComponent::GetHitDamage() const
{
	const UGSTomahawkSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return Settings && ASC ? Settings->Damage * GetDamageMultiplier() * ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()) : 0.f;
}

double UGSTomahawkComponent::GetThrowInterval() const
{
	const UGSTomahawkSettings* Settings = GetSettings();
	return Settings ? FMath::Max(static_cast<double>(Settings->ThrowInterval), 0.05) : 0.0;
}

FVector UGSTomahawkComponent::GetThrowOrigin() const
{
	const AGSCharacter* Character = GetCharacter();
	const UGSTomahawkSettings* Settings = GetSettings();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const FVector Offset = Settings ? Settings->ThrowOffset : FVector::ZeroVector;
	return Character->GetActorLocation() + FRotator(0.f, Character->GetActorRotation().Yaw, 0.f).RotateVector(Offset);
}

bool UGSTomahawkComponent::CanThrow() const
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

bool UGSTomahawkComponent::HasClearLine(const AActor* Candidate) const
{
	// World geometry blocks the line; other enemies and the player don't (pawns aren't queried).
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSTomahawkLine), false, GetOwner());
	Params.AddIgnoredActor(Candidate);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByObjectType(Hit, GetThrowOrigin(), Candidate->GetActorLocation(), Objects, Params);
}

TArray<AGSEnemyCharacter*> UGSTomahawkComponent::FindTargets() const
{
	TArray<AGSEnemyCharacter*> Targets;
	const UGSTomahawkSettings* Settings = GetSettings();
	if (!Settings)
	{
		return Targets;
	}
	const FVector From = GetThrowOrigin();
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
			Targets.Add(Candidate.Value);
		}
	}
	return Targets;
}

TArray<AGSTomahawk*> UGSTomahawkComponent::GetActiveTomahawks() const
{
	TArray<AGSTomahawk*> Tomahawks;
	if (const UGSThrownSubsystem* Thrown = GetWorld() ? GetWorld()->GetSubsystem<UGSThrownSubsystem>() : nullptr)
	{
		for (AGSThrownActor* Actor : Thrown->GetActive())
		{
			if (AGSTomahawk* Tomahawk = Cast<AGSTomahawk>(Actor))
			{
				Tomahawks.Add(Tomahawk);
			}
		}
	}
	return Tomahawks;
}

void UGSTomahawkComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const UGSTomahawkSettings* Settings = GetSettings();
	const int32 Level = GetLevel();
	const float Power = Level > 0 ? GetPower() : 0.f;
	if (Settings && (Level != LoggedLevel || !FMath::IsNearlyEqual(Power, LoggedPower)))
	{
		if (Level > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[tomahawk] %s: Tomahawk %d pick(s), power %g -> x%.2f damage (%.1f per hit), %d tomahawk(s) a throw, every %.2f s"),
				*GetNameSafe(GetOwner()), Level, Power, Settings->GetDamageMultiplier(Power), Settings->Damage * Settings->GetDamageMultiplier(Power),
				Settings->GetTomahawks(Power), Settings->ThrowInterval);
		}
		else if (LoggedLevel > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[tomahawk] %s: Tomahawk removed"), *GetNameSafe(GetOwner()));
		}
		LoggedLevel = Level;
		LoggedPower = Power;
	}
	UpdateThrowing(DeltaTime);
}

void UGSTomahawkComponent::StopThrowing(double Now)
{
	bThrowing = false;
	bTargetInRange = false;
	// Nothing is banked while idle: the next target gets a throw at once, but never sooner than one interval after the
	// last throw.
	NextThrowTime = FMath::Max(NextThrowTime, Now);
}

void UGSTomahawkComponent::UpdateThrowing(float DeltaSeconds)
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
		bTargetInRange = FindTargets().Num() > 0;
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
	for (int32 Guard = 0; NextThrowTime <= Now && Guard < MaxTomahawkThrowsPerFrame; ++Guard)
	{
		const TArray<AGSEnemyCharacter*> Targets = FindTargets();
		if (Targets.Num() == 0)
		{
			StopThrowing(Now);
			return;
		}
		ThrowVolley(Targets, static_cast<float>(Now - NextThrowTime));
		NextThrowTime += Interval;
		++BurstShots;
	}
}

void UGSTomahawkComponent::ThrowVolley(const TArray<AGSEnemyCharacter*>& Targets, float Lateness)
{
	const UGSTomahawkSettings* Settings = GetSettings();
	AGSCharacter* Character = GetCharacter();
	UGSThrownSubsystem* Thrown = GetWorld()->GetSubsystem<UGSThrownSubsystem>();
	if (!Settings || !Character || !Thrown || Targets.Num() == 0)
	{
		return;
	}
	const FVector Origin = GetThrowOrigin();
	const int32 Count = GetTomahawksPerThrow();
	const float Damage = GetHitDamage(); // set at the throw
	LastTargets.Reset();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// The nearest first, then the next nearest; with fewer targets than tomahawks they go round the targets again.
		AGSEnemyCharacter* Target = Targets[Index % Targets.Num()];
		AGSTomahawk* Tomahawk = Thrown->Acquire<AGSTomahawk>();
		if (!Tomahawk)
		{
			continue;
		}
		FGSTomahawkLaunch Launch;
		Launch.Start = Origin;
		Launch.Direction = Target->GetActorLocation() - Origin; // flattened by the tomahawk: it flies level
		Launch.Distance = Settings->Distance;
		Launch.Speed = Settings->Speed;
		Launch.SweepRadius = Settings->SweepRadius;
		Launch.CatchDistance = Settings->CatchDistance;
		Launch.MaxFlightTime = Settings->MaxFlightTime;
		Launch.AlreadyFlown = FMath::Max(Lateness, 0.f);
		Launch.Damage = Damage;
		Launch.SourceASC = Character->GetAbilitySystemComponent();
		Launch.DamageEffect = Settings->DamageEffect;
		Launch.DamageSource = GSGameplayTags::Damage_Source_Tomahawk;
		Launch.ReturnTo = Character;
		Launch.Owner = this;
		Launch.Mesh = Settings->TomahawkMesh;
		Launch.Material = Settings->TomahawkMaterial;
		Launch.Scale = Settings->TomahawkScale;
		Launch.SpinRate = Settings->SpinRate;
		Launch.HitEffect = Settings->HitEffect;
		Launch.HitEffectScale = Settings->HitEffectScale;
		Tomahawk->Launch(Launch);
		LastTargets.Add(Target);
		++TomahawksThrown;
	}
	++Throws;

	// Placeholder sound (local: single-player). TODO(co-op): play it on every machine.
	if (Settings->ThrowSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->ThrowSound, Origin);
	}
	UE_LOG(LogBanditBaller, Verbose, TEXT("[tomahawk] throw %d: %d tomahawk(s), %.1f per hit, %.3f s late"), Throws, Count, Damage, Lateness);
}
