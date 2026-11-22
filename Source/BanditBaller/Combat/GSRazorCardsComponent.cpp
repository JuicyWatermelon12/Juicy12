// BanditBaller

#include "GSRazorCardsComponent.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSEnemyCharacter.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSProjectileSubsystem.h"
#include "GSRazorCardsSettings.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** At most this many owed cards are thrown in one frame (a runaway guard, like the Floating Gun's). */
	constexpr int32 MaxCardsPerFrame = 16;
}

UGSRazorCardsComponent::UGSRazorCardsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // throw from where the character is after it moved this frame
}

AGSCharacter* UGSRazorCardsComponent::GetCharacter() const
{
	return Cast<AGSCharacter>(GetOwner());
}

const UGSRazorCardsSettings* UGSRazorCardsComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetRazorCardsSettings() : nullptr;
}

int32 UGSRazorCardsComponent::GetLevel() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetRazorCardsLevelAttribute()))) : 0;
}

float UGSRazorCardsComponent::GetPower() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetRazorCardsPowerAttribute())) : 0.f;
}

double UGSRazorCardsComponent::GetThrowInterval() const
{
	const UGSRazorCardsSettings* Settings = GetSettings();
	return Settings ? Settings->GetThrowInterval(GetPower()) : 0.0;
}

float UGSRazorCardsComponent::GetDamageMultiplier() const
{
	const UGSRazorCardsSettings* Settings = GetSettings();
	return Settings ? Settings->GetDamageMultiplier(GetPower()) : 1.f;
}

FVector UGSRazorCardsComponent::GetThrowOrigin() const
{
	const AGSCharacter* Character = GetCharacter();
	const UGSRazorCardsSettings* Settings = GetSettings();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const FVector Offset = Settings ? Settings->ThrowOffset : FVector::ZeroVector;
	return Character->GetActorLocation() + FRotator(0.f, Character->GetActorRotation().Yaw, 0.f).RotateVector(Offset);
}

bool UGSRazorCardsComponent::CanThrow() const
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

bool UGSRazorCardsComponent::HasClearLine(const AActor* Candidate) const
{
	// World geometry blocks the line; other enemies and the player don't (pawns aren't queried).
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSRazorCardsLine), false, GetOwner());
	Params.AddIgnoredActor(Candidate);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByObjectType(Hit, GetThrowOrigin(), Candidate->GetActorLocation(), Objects, Params);
}

bool UGSRazorCardsComponent::IsValidTarget(const AGSEnemyCharacter* Candidate) const
{
	const UGSRazorCardsSettings* Settings = GetSettings();
	const UGSHealthComponent* Health = Candidate ? Candidate->GetHealthComponent() : nullptr;
	return Settings && Health && !Health->IsDead()
		&& FVector::DistSquared(Candidate->GetActorLocation(), GetThrowOrigin()) <= FMath::Square(Settings->Range)
		&& HasClearLine(Candidate);
}

AGSEnemyCharacter* UGSRazorCardsComponent::FindTarget() const
{
	const UGSRazorCardsSettings* Settings = GetSettings();
	if (!Settings)
	{
		return nullptr;
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
			return Candidate.Value;
		}
	}
	return nullptr;
}

void UGSRazorCardsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const UGSRazorCardsSettings* Settings = GetSettings();
	const int32 Level = GetLevel();
	const float Power = Level > 0 ? GetPower() : 0.f;
	if (Settings && (Level != LoggedLevel || !FMath::IsNearlyEqual(Power, LoggedPower)))
	{
		if (Level > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[cards] %s: Razor Cards %d pick(s), power %g -> x%.2f damage (%.1f per card), %.2f cards/s"),
				*GetNameSafe(GetOwner()), Level, Power, Settings->GetDamageMultiplier(Power), Settings->Damage * Settings->GetDamageMultiplier(Power),
				1.0 / Settings->GetThrowInterval(Power));
		}
		else if (LoggedLevel > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[cards] %s: Razor Cards removed"), *GetNameSafe(GetOwner()));
		}
		LoggedLevel = Level;
		LoggedPower = Power;
	}
	if (Level > 0 && Settings && !bPrewarmed)
	{
		if (UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>())
		{
			Projectiles->Prewarm(Settings->PoolPrewarm);
			bPrewarmed = true;
		}
	}
	UpdateThrowing(DeltaTime);
}

void UGSRazorCardsComponent::StopThrowing(double Now)
{
	Target.Reset();
	bThrowing = false;
	// Nothing is banked while idle: the next target gets a card at once, but never sooner than one interval after the
	// last card.
	NextThrowTime = FMath::Max(NextThrowTime, Now);
}

void UGSRazorCardsComponent::UpdateThrowing(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanThrow())
	{
		StopThrowing(Now);
		return;
	}
	const UGSRazorCardsSettings* Settings = GetSettings();
	const double Interval = Settings->GetThrowInterval(GetPower()); // the power's fire rate bonus included

	TargetRefreshRemaining -= DeltaSeconds;
	if (TargetRefreshRemaining <= 0.f || !IsValidTarget(Target.Get()))
	{
		Target = FindTarget();
		TargetRefreshRemaining = Settings->TargetRefreshInterval;
	}
	if (!Target.IsValid())
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
		BurstInterval = Interval;
	}
	else if (BurstShots > 0 && !FMath::IsNearlyEqual(Interval, BurstInterval))
	{
		// The fire rate changed mid-burst (a pick): the next card is due one new interval after the last card's scheduled
		// time, and the schedule starts over from that card.
		NextThrowTime = LastThrowTime + Interval;
		BurstStartTime = LastThrowTime;
		BurstShots = 1;
		BurstInterval = Interval;
	}
	// Every card due by now, each on its own scheduled time (a long frame throws all it owes).
	for (int32 Guard = 0; NextThrowTime <= Now && Guard < MaxCardsPerFrame; ++Guard)
	{
		if (!IsValidTarget(Target.Get()))
		{
			Target = FindTarget();
			if (!Target.IsValid())
			{
				StopThrowing(Now);
				return;
			}
		}
		ThrowAt(Target.Get(), static_cast<float>(Now - NextThrowTime));
		LastThrowTime = NextThrowTime;
		NextThrowTime += Interval;
		++BurstShots;
	}
}

void UGSRazorCardsComponent::ThrowAt(AGSEnemyCharacter* AtTarget, float Lateness)
{
	AGSCharacter* Character = GetCharacter();
	UAbilitySystemComponent* SourceASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>();
	const UGSRazorCardsSettings* Settings = GetSettings();
	if (!AtTarget || !SourceASC || !Projectiles || !Settings)
	{
		return;
	}

	const FVector Origin = GetThrowOrigin();
	FGSProjectileLaunch Launch;
	Launch.Direction = (AtTarget->GetActorLocation() - Origin).GetSafeNormal(); // homing does the rest
	// A late card (thrown in a frame after its scheduled time) starts as far along as it would be by now.
	const float Late = FMath::Clamp(Lateness, 0.f, Settings->CardLifetime);
	Launch.Start = Origin + Launch.Direction * Settings->CardSpeed * Late;
	Launch.Speed = Settings->CardSpeed;
	Launch.Lifetime = FMath::Max(Settings->CardLifetime - Late, 0.01f);
	Launch.Radius = Settings->CardRadius;
	Launch.Damage = Settings->Damage * Settings->GetDamageMultiplier(GetPower()) * SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
	Launch.SourceASC = SourceASC;
	Launch.DamageEffect = Settings->DamageEffect;
	Launch.DamageSource = GSGameplayTags::Damage_Source_RazorCards;
	Launch.Mesh = Settings->CardMesh;
	Launch.Material = Settings->CardMaterial;
	Launch.Scale = Settings->CardScale;
	Launch.ImpactEffect = Settings->ImpactEffect;
	Launch.ImpactEffectScale = Settings->ImpactEffectScale;
	Launch.HomingTarget = AtTarget;
	Launch.HomingTurnRate = Settings->HomingTurnRate;
	Launch.SpinRate = Settings->CardSpinRate;
	Projectiles->FireProjectile(Launch);
	++CardsThrown;

	// Placeholder sound (local: single-player). TODO(co-op): play it on every machine.
	if (Settings->ThrowSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->ThrowSound, Origin);
	}
	UE_LOG(LogBanditBaller, Verbose, TEXT("[cards] card %d at %s (%.1f damage, %.3f s late)"), CardsThrown, *GetNameSafe(AtTarget), Launch.Damage, Lateness);
}
