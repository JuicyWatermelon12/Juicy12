// BanditBaller

#include "GSLuckyCoinComponent.h"
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
#include "GSLuckyCoinSettings.h"
#include "GSProjectileSubsystem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** At most this many owed flips in one frame (a runaway guard). */
	constexpr int32 MaxFlipsPerFrame = 16;
}

UGSLuckyCoinComponent::UGSLuckyCoinComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // flip from where the character is after it moved this frame
}

AGSCharacter* UGSLuckyCoinComponent::GetCharacter() const
{
	return Cast<AGSCharacter>(GetOwner());
}

const UGSLuckyCoinSettings* UGSLuckyCoinComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetLuckyCoinSettings() : nullptr;
}

int32 UGSLuckyCoinComponent::GetLevel() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetLuckyCoinLevelAttribute()))) : 0;
}

float UGSLuckyCoinComponent::GetPower() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetLuckyCoinPowerAttribute())) : 0.f;
}

float UGSLuckyCoinComponent::GetDamageMultiplier() const
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
	return Settings ? Settings->GetDamageMultiplier(GetPower()) : 1.f;
}

int32 UGSLuckyCoinComponent::GetBounces() const
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
	return Settings ? Settings->GetBounces(GetPower()) : 0;
}

float UGSLuckyCoinComponent::GetHitDamage() const
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return Settings && ASC ? Settings->Damage * GetDamageMultiplier() * ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()) : 0.f;
}

double UGSLuckyCoinComponent::GetFlipInterval() const
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
	return Settings ? FMath::Max(static_cast<double>(Settings->FlipInterval), 0.05) : 0.0;
}

FVector UGSLuckyCoinComponent::GetThrowOrigin() const
{
	const AGSCharacter* Character = GetCharacter();
	const UGSLuckyCoinSettings* Settings = GetSettings();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const FVector Offset = Settings ? Settings->ThrowOffset : FVector::ZeroVector;
	return Character->GetActorLocation() + FRotator(0.f, Character->GetActorRotation().Yaw, 0.f).RotateVector(Offset);
}

bool UGSLuckyCoinComponent::CanFlip() const
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

bool UGSLuckyCoinComponent::HasClearLine(const FVector& From, const AActor* Candidate) const
{
	// World geometry blocks the line; other enemies and the player don't (pawns aren't queried).
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSLuckyCoinLine), false, GetOwner());
	Params.AddIgnoredActor(Candidate);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByObjectType(Hit, From, Candidate->GetActorLocation(), Objects, Params);
}

bool UGSLuckyCoinComponent::IsValidTarget(const AGSEnemyCharacter* Candidate) const
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
	const UGSHealthComponent* Health = IsValid(Candidate) ? Candidate->GetHealthComponent() : nullptr;
	return Settings && Health && !Health->IsDead()
		&& FVector::DistSquared(Candidate->GetActorLocation(), GetThrowOrigin()) <= FMath::Square(Settings->Range)
		&& HasClearLine(GetThrowOrigin(), Candidate);
}

AGSEnemyCharacter* UGSLuckyCoinComponent::FindTarget() const
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
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
		if (HasClearLine(From, Candidate.Value))
		{
			return Candidate.Value;
		}
	}
	return nullptr;
}

AGSEnemyCharacter* UGSLuckyCoinComponent::FindBounceTarget(const FVector& From, const TArray<TWeakObjectPtr<AActor>>& Exclude) const
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
	if (!Settings)
	{
		return nullptr;
	}
	const float RangeSq = FMath::Square(Settings->BounceRange);
	TArray<TPair<float, AGSEnemyCharacter*>> Candidates;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const UGSHealthComponent* Health = It->GetHealthComponent();
		const float DistanceSq = FVector::DistSquared(It->GetActorLocation(), From);
		if (Health && !Health->IsDead() && DistanceSq <= RangeSq && !Exclude.Contains(*It))
		{
			Candidates.Emplace(DistanceSq, *It);
		}
	}
	Candidates.Sort([](const TPair<float, AGSEnemyCharacter*>& A, const TPair<float, AGSEnemyCharacter*>& B) { return A.Key < B.Key; });
	for (const TPair<float, AGSEnemyCharacter*>& Candidate : Candidates)
	{
		if (HasClearLine(From, Candidate.Value))
		{
			return Candidate.Value;
		}
	}
	return nullptr;
}

void UGSLuckyCoinComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const UGSLuckyCoinSettings* Settings = GetSettings();
	const int32 Level = GetLevel();
	const float Power = Level > 0 ? GetPower() : 0.f;
	if (Settings && (Level != LoggedLevel || !FMath::IsNearlyEqual(Power, LoggedPower)))
	{
		if (Level > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[coin] %s: Lucky Coin %d pick(s), power %g -> x%.2f damage (%.1f per hit), %d bounce(s) (up to %d hits a flip), every %.2f s"),
				*GetNameSafe(GetOwner()), Level, Power, Settings->GetDamageMultiplier(Power), Settings->Damage * Settings->GetDamageMultiplier(Power),
				Settings->GetBounces(Power), 1 + Settings->GetBounces(Power), Settings->FlipInterval);
		}
		else if (LoggedLevel > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[coin] %s: Lucky Coin removed"), *GetNameSafe(GetOwner()));
		}
		LoggedLevel = Level;
		LoggedPower = Power;
	}
	if (Level <= 0)
	{
		FlipsDone = Flips; // a new run released the coins in the air without a callback
	}
	if (Level > 0 && Settings && !bPrewarmed)
	{
		if (UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>())
		{
			Projectiles->Prewarm(Settings->PoolPrewarm);
			bPrewarmed = true;
		}
	}
	UpdateFlipping(DeltaTime);
}

void UGSLuckyCoinComponent::StopFlipping(double Now)
{
	Target.Reset();
	bFlipping = false;
	// Nothing is banked while idle: the next target gets a coin at once, but never sooner than one interval after the
	// last flip.
	NextFlipTime = FMath::Max(NextFlipTime, Now);
}

void UGSLuckyCoinComponent::UpdateFlipping(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanFlip())
	{
		StopFlipping(Now);
		return;
	}
	const double Interval = GetFlipInterval();

	TargetRefreshRemaining -= DeltaSeconds;
	if (TargetRefreshRemaining <= 0.f || !IsValidTarget(Target.Get()))
	{
		Target = FindTarget();
		TargetRefreshRemaining = GetSettings()->TargetRefreshInterval;
	}
	if (!Target.IsValid())
	{
		StopFlipping(Now);
		return;
	}

	if (!bFlipping)
	{
		bFlipping = true;
		NextFlipTime = FMath::Max(NextFlipTime, Now);
		BurstStartTime = NextFlipTime;
		BurstShots = 0;
	}
	// Every flip due by now, each on its own scheduled time (a long frame flips all it owes).
	for (int32 Guard = 0; NextFlipTime <= Now && Guard < MaxFlipsPerFrame; ++Guard)
	{
		if (!IsValidTarget(Target.Get()))
		{
			Target = FindTarget();
			if (!Target.IsValid())
			{
				StopFlipping(Now);
				return;
			}
		}
		FlipAt(Target.Get(), static_cast<float>(Now - NextFlipTime));
		NextFlipTime += Interval;
		++BurstShots;
	}
}

void UGSLuckyCoinComponent::FlipAt(AGSEnemyCharacter* AtTarget, float Lateness)
{
	const UGSLuckyCoinSettings* Settings = GetSettings();
	if (!AtTarget || !Settings)
	{
		return;
	}
	TSharedRef<FFlip> Flip = MakeShared<FFlip>();
	Flip->BouncesLeft = GetBounces();
	Flip->Damage = GetHitDamage(); // set at the flip, like the projectiles
	++Flips;
	LaunchCoin(Flip, GetThrowOrigin(), AtTarget, Lateness);

	// Placeholder sound (local: single-player). TODO(co-op): play it on every machine.
	if (Settings->FlipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->FlipSound, GetThrowOrigin());
	}
}

void UGSLuckyCoinComponent::LaunchCoin(const TSharedRef<FFlip>& Flip, const FVector& Start, AGSEnemyCharacter* AtTarget, float Lateness)
{
	AGSCharacter* Character = GetCharacter();
	UAbilitySystemComponent* SourceASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>();
	const UGSLuckyCoinSettings* Settings = GetSettings();
	if (!AtTarget || !SourceASC || !Projectiles || !Settings)
	{
		EndFlip(Flip);
		return;
	}

	FGSProjectileLaunch Launch;
	Launch.Direction = (AtTarget->GetActorLocation() - Start).GetSafeNormal(); // homing does the rest
	// A late coin (flipped in a frame after its scheduled time) starts as far along as it would be by now.
	const float Late = FMath::Clamp(Lateness, 0.f, Settings->CoinLifetime);
	Launch.Start = Start + Launch.Direction * Settings->CoinSpeed * Late;
	Launch.Speed = Settings->CoinSpeed;
	Launch.Lifetime = FMath::Max(Settings->CoinLifetime - Late, 0.01f);
	Launch.Radius = Settings->CoinRadius;
	Launch.Damage = Flip->Damage;
	Launch.SourceASC = SourceASC;
	Launch.DamageEffect = Settings->DamageEffect;
	Launch.DamageSource = GSGameplayTags::Damage_Source_LuckyCoin;
	Launch.Mesh = Settings->CoinMesh;
	Launch.Material = Settings->CoinMaterial;
	Launch.Scale = Settings->CoinScale;
	Launch.ImpactEffect = Settings->ImpactEffect;
	Launch.ImpactEffectScale = Settings->ImpactEffectScale;
	Launch.HomingTarget = AtTarget;
	Launch.HomingTurnRate = Settings->HomingTurnRate;
	Launch.SpinRate = Settings->CoinSpinRate;
	Launch.IgnoredTargets = Flip->Hit; // never the same enemy twice in one flip
	TWeakObjectPtr<UGSLuckyCoinComponent> WeakThis(this);
	Launch.OnFinished = [WeakThis, Flip](EGSProjectileEnd Reason, AActor* HitActor, const FVector& Where)
	{
		if (UGSLuckyCoinComponent* Self = WeakThis.Get())
		{
			Self->HandleCoinFinished(Flip, Reason, HitActor, Where);
		}
	};
	if (!Projectiles->FireProjectile(Launch))
	{
		EndFlip(Flip);
	}
}

void UGSLuckyCoinComponent::HandleCoinFinished(const TSharedRef<FFlip>& Flip, EGSProjectileEnd Reason, AActor* HitActor, const FVector& Where)
{
	if (Reason != EGSProjectileEnd::HitTarget || !HitActor)
	{
		EndFlip(Flip); // a miss: the wall or the end of its flight
		return;
	}
	Flip->Hit.Add(HitActor);
	++HitsDealt;
	if (Flip->BouncesLeft <= 0 || !CanFlip())
	{
		EndFlip(Flip);
		return;
	}
	// Bounce: the nearest other living enemy within reach of the one just hit, not hit yet this flip.
	AGSEnemyCharacter* Next = FindBounceTarget(HitActor->GetActorLocation(), Flip->Hit);
	if (!Next)
	{
		EndFlip(Flip); // nobody else in bounce range
		return;
	}
	--Flip->BouncesLeft;
	LaunchCoin(Flip, Where, Next, 0.f);
}

void UGSLuckyCoinComponent::EndFlip(const TSharedRef<FFlip>& Flip)
{
	++FlipsDone;
	LastFlipHits = Flip->Hit;
	LastFlipDamage = Flip->Damage;
	UE_LOG(LogBanditBaller, Verbose, TEXT("[coin] flip done: %d hit(s), %.1f damage each"), Flip->Hit.Num(), Flip->Damage);
}
