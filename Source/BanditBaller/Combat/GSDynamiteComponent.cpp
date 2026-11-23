// BanditBaller

#include "GSDynamiteComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSCriticalHit.h"
#include "GSDynamiteSettings.h"
#include "GSEnemyCharacter.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSHealthComponent.h"
#include "GSLobbedThrow.h"
#include "GSThrownSubsystem.h"
#include "GSWeaponDebug.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

namespace
{
	/** At most this many owed throws in one frame (a runaway guard). */
	constexpr int32 MaxDynamiteThrowsPerFrame = 8;
}

UGSDynamiteComponent::UGSDynamiteComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // throw from where the character is after it moved this frame
}

AGSCharacter* UGSDynamiteComponent::GetCharacter() const
{
	return Cast<AGSCharacter>(GetOwner());
}

const UGSDynamiteSettings* UGSDynamiteComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetDynamiteSettings() : nullptr;
}

int32 UGSDynamiteComponent::GetLevel() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetDynamiteLevelAttribute()))) : 0;
}

float UGSDynamiteComponent::GetPower() const
{
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetDynamitePowerAttribute())) : 0.f;
}

float UGSDynamiteComponent::GetDamageMultiplier() const
{
	const UGSDynamiteSettings* Settings = GetSettings();
	return Settings ? Settings->GetDamageMultiplier(GetPower()) : 1.f;
}

float UGSDynamiteComponent::GetBlastRadius() const
{
	const UGSDynamiteSettings* Settings = GetSettings();
	return Settings ? Settings->GetBlastRadius(GetPower()) : 0.f;
}

float UGSDynamiteComponent::GetBlastDamage() const
{
	const UGSDynamiteSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return Settings && ASC ? Settings->Damage * GetDamageMultiplier() * ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()) : 0.f;
}

double UGSDynamiteComponent::GetThrowInterval() const
{
	const UGSDynamiteSettings* Settings = GetSettings();
	return Settings ? FMath::Max(static_cast<double>(Settings->ThrowInterval), 0.05) : 0.0;
}

FVector UGSDynamiteComponent::GetThrowOrigin() const
{
	const AGSCharacter* Character = GetCharacter();
	const UGSDynamiteSettings* Settings = GetSettings();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const FVector Offset = Settings ? Settings->ThrowOffset : FVector::ZeroVector;
	return Character->GetActorLocation() + FRotator(0.f, Character->GetActorRotation().Yaw, 0.f).RotateVector(Offset);
}

bool UGSDynamiteComponent::CanThrow() const
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

bool UGSDynamiteComponent::IsLivingEnemy(const AGSEnemyCharacter* Enemy) const
{
	const UGSHealthComponent* Health = IsValid(Enemy) ? Enemy->GetHealthComponent() : nullptr;
	return Health && !Health->IsDead();
}

bool UGSDynamiteComponent::IsInRange(const AGSEnemyCharacter* Enemy) const
{
	const UGSDynamiteSettings* Settings = GetSettings();
	const AGSCharacter* Character = GetCharacter();
	return Settings && Character && IsLivingEnemy(Enemy)
		&& FVector::DistSquared(Enemy->GetActorLocation(), Character->GetActorLocation()) <= FMath::Square(Settings->Range);
}

int32 UGSDynamiteComponent::CountNeighbours(const AGSEnemyCharacter* Enemy) const
{
	if (!IsLivingEnemy(Enemy))
	{
		return 0;
	}
	const float RadiusSq = FMath::Square(GetBlastRadius());
	int32 Count = 0;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != Enemy && IsLivingEnemy(*It) && FVector::DistSquared(It->GetActorLocation(), Enemy->GetActorLocation()) <= RadiusSq)
		{
			++Count;
		}
	}
	return Count;
}

AGSEnemyCharacter* UGSDynamiteComponent::FindTarget() const
{
	const AGSCharacter* Character = GetCharacter();
	if (!Character || !GetSettings())
	{
		return nullptr;
	}
	AGSEnemyCharacter* Best = nullptr;
	int32 BestCount = -1;
	float BestDistanceSq = 0.f;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (!IsInRange(*It))
		{
			continue;
		}
		const int32 Count = CountNeighbours(*It);
		const float DistanceSq = FVector::DistSquared(It->GetActorLocation(), Character->GetActorLocation());
		if (Count > BestCount || (Count == BestCount && DistanceSq < BestDistanceSq))
		{
			Best = *It;
			BestCount = Count;
			BestDistanceSq = DistanceSq;
		}
	}
	return Best;
}

void UGSDynamiteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const UGSDynamiteSettings* Settings = GetSettings();
	const int32 Level = GetLevel();
	const float Power = Level > 0 ? GetPower() : 0.f;
	if (Settings && (Level != LoggedLevel || !FMath::IsNearlyEqual(Power, LoggedPower)))
	{
		if (Level > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[dynamite] %s: Dynamite %d pick(s), power %g -> x%.2f damage (%.1f per blast), blast radius %.0f cm (x%.2f), every %.2f s"),
				*GetNameSafe(GetOwner()), Level, Power, Settings->GetDamageMultiplier(Power), Settings->Damage * Settings->GetDamageMultiplier(Power),
				Settings->GetBlastRadius(Power), Settings->GetRadiusMultiplier(Power), Settings->ThrowInterval);
		}
		else if (LoggedLevel > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[dynamite] %s: Dynamite removed"), *GetNameSafe(GetOwner()));
		}
		LoggedLevel = Level;
		LoggedPower = Power;
	}
	UpdateThrowing(DeltaTime);
}

void UGSDynamiteComponent::StopThrowing(double Now)
{
	bThrowing = false;
	bTargetInRange = false;
	// Nothing is banked while idle: the next throw comes at once when an enemy is in range, but never sooner than one
	// interval after the last throw.
	NextThrowTime = FMath::Max(NextThrowTime, Now);
}

void UGSDynamiteComponent::UpdateThrowing(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanThrow())
	{
		StopThrowing(Now);
		return;
	}
	const double Interval = GetThrowInterval();

	// Idle check: is any enemy in range (refreshed every TargetRefreshInterval, the target is picked at the throw).
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
	for (int32 Guard = 0; NextThrowTime <= Now && Guard < MaxDynamiteThrowsPerFrame; ++Guard)
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

void UGSDynamiteComponent::ThrowAt(AGSEnemyCharacter* AtTarget, float Lateness)
{
	const UGSDynamiteSettings* Settings = GetSettings();
	UGSThrownSubsystem* Thrown = GetWorld()->GetSubsystem<UGSThrownSubsystem>();
	if (!AtTarget || !Settings || !Thrown)
	{
		return;
	}
	AGSLobbedThrow* Stick = Thrown->Acquire<AGSLobbedThrow>();
	if (!Stick)
	{
		return;
	}

	// It lands where the target will be when it comes down (its flat velocity x the flight time left x LeadFactor, on the
	// ground there), at its feet; the blast is centered on the body over that point (where Powder Keg measures).
	const float AlreadyFlown = FMath::Clamp(Lateness, 0.f, Settings->FlightTime); // a late throw starts that far along its arc
	const FGSLobAim Aim = AGSLobbedThrow::AimAt(AtTarget, Settings->FlightTime - AlreadyFlown, Settings->LeadFactor);
	const FVector Center = Aim.BodyCenter;
	const float Radius = GetBlastRadius();
	const float Damage = GetBlastDamage(); // set at the throw, like the projectiles
	LastTarget = AtTarget;
	LastLanding = Aim.Landing;
	bLastThrowLed = Aim.bLed;

	FGSLobLaunch Lob;
	Lob.Start = GetThrowOrigin();
	Lob.Landing = LastLanding;
	Lob.FlightTime = Settings->FlightTime;
	Lob.ArcHeight = Settings->ArcHeight;
	Lob.AlreadyFlown = AlreadyFlown;
	Lob.Mesh = Settings->StickMesh;
	Lob.Material = Settings->StickMaterial;
	Lob.Scale = Settings->StickScale;
	Lob.SpinRate = Settings->StickSpinRate;
	TWeakObjectPtr<UGSDynamiteComponent> WeakThis(this);
	TWeakObjectPtr<AGSEnemyCharacter> WeakTarget(AtTarget);
	Lob.OnLanded = [WeakThis, WeakTarget, Center, Radius, Damage](const FVector&)
	{
		if (UGSDynamiteComponent* Self = WeakThis.Get())
		{
			Self->Explode(Center, Radius, Damage, WeakTarget.Get());
		}
	};
	Stick->Launch(MoveTemp(Lob));
	++Throws;

	// Placeholder sound (local: single-player). TODO(co-op): play it on every machine.
	if (Settings->ThrowSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->ThrowSound, GetThrowOrigin());
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[dynamite] stick %d at %s (%d neighbour(s), moving %.0f cm/s), %.1f damage in %.0f cm, %.3f s late, %s"),
		Throws, *GetNameSafe(AtTarget), CountNeighbours(AtTarget), AtTarget->GetVelocity().Size2D(), Damage, Radius, Lateness,
		Aim.bLed ? *FString::Printf(TEXT("led %.0f cm ahead"), FVector::Dist2D(Aim.Landing, AtTarget->GetActorLocation())) : TEXT("not led"));
}

int32 UGSDynamiteComponent::Explode(const FVector& Center, float Radius, float Damage, const AGSEnemyCharacter* Target)
{
	AGSCharacter* Character = GetCharacter();
	UAbilitySystemComponent* SourceASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	const UGSDynamiteSettings* Settings = GetSettings();
	if (!SourceASC || !Settings)
	{
		return 0;
	}
	++Blasts;
	LastBlastCenter = Center;
	LastBlastRadius = Radius;

	// Everyone in the blast first, then the damage (a kill can spawn pickups or set off Powder Keg mid-loop). Only
	// enemies: the player is never hit.
	TArray<AGSEnemyCharacter*> Targets;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (IsLivingEnemy(*It) && FVector::DistSquared(It->GetActorLocation(), Center) <= FMath::Square(Radius))
		{
			Targets.Add(*It);
		}
	}
	bLastBlastHitTarget = Target && Targets.ContainsByPredicate([Target](const AGSEnemyCharacter* Enemy) { return Enemy == Target; });
	TargetHits += bLastBlastHitTarget ? 1 : 0;
	LastBlastHits = 0;
	if (Settings->DamageEffect && Damage > 0.f)
	{
		for (AGSEnemyCharacter* Enemy : Targets)
		{
			UAbilitySystemComponent* TargetASC = IsLivingEnemy(Enemy) ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Enemy) : nullptr;
			if (!TargetASC)
			{
				continue; // died in this blast already (a Powder Keg blast)
			}
			// Its own critical roll per enemy (Loaded Dice); GE_Damage from the player (kills are the player's).
			const FGSCritRoll Crit = GSCombat::RollCriticalHit(SourceASC);
			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Settings->DamageEffect, 1.f, SourceASC->MakeEffectContext());
			if (!Spec.IsValid())
			{
				continue;
			}
			Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Damage * Crit.DamageMultiplier);
			Spec.Data->AddDynamicAssetTag(GSGameplayTags::Damage_Source_Dynamite); // run stats by source
			if (Crit.bCritical)
			{
				Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Critical, Crit.GetLevel());
			}
			const FVector Where = Enemy->GetActorLocation();
			FHitResult Hit(Enemy, nullptr, Where, (Where - Center).GetSafeNormal());
			Hit.ImpactPoint = Where;
			Spec.Data->GetContext().AddHitResult(Hit);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			++LastBlastHits;
			++HitsDealt;
		}
	}
	else if (!Settings->DamageEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[dynamite] DA_DynamiteSettings has no DamageEffect; the blast did no damage."));
	}

	// Placeholder explosion and sound (local: single-player). TODO(co-op): multicast them.
	if (Settings->BlastEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Settings->BlastEffect, Center, FRotator::ZeroRotator,
			FVector(Radius / FMath::Max(Settings->BlastEffectRadius, 1.f)));
	}
	if (Settings->BlastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Settings->BlastSound, Center);
	}
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugSphere(GetWorld(), Center, Radius, 16, FColor::Red, false, GSWeaponDebug::DrawDuration);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[dynamite] blast %d at %s: radius %.0f, damage %.1f, %d enemies hit%s"),
		Blasts, *Center.ToCompactString(), Radius, Damage, LastBlastHits,
		Target ? (bLastBlastHitTarget ? TEXT(", target hit") : TEXT(", target missed")) : TEXT(""));
	return LastBlastHits;
}
