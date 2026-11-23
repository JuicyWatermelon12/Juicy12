// BanditBaller

#include "GSGameplayAbility_EnemyFire.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSEnemyAIController.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameplayTags.h"
#include "GSProjectile.h"
#include "GSProjectileSubsystem.h"
#include "GSRunRandom.h"
#include "GSWeaponData.h"
#include "GSWeaponDebug.h"
#include "BanditBaller.h"

UGSGameplayAbility_EnemyFire::UGSGameplayAbility_EnemyFire()
{
	SetAssetTags(FGameplayTagContainer(GSGameplayTags::Ability_Attack_Ranged));
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dead);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGSGameplayAbility_EnemyFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AGSEnemyCharacter* Enemy = ActorInfo ? Cast<AGSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UGSEnemyData* Data = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!Data || !Data->Weapon || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FaceTarget(*Enemy);
	Enemy->SetTelegraphing(true);
	ShotsRemaining = Data->BurstCount;

	UAbilityTask_WaitDelay* Aim = UAbilityTask_WaitDelay::WaitDelay(this, Data->AimTelegraphTime);
	Aim->OnFinish.AddDynamic(this, &UGSGameplayAbility_EnemyFire::OnAimFinished);
	Aim->ReadyForActivation();
}

void UGSGameplayAbility_EnemyFire::OnAimFinished()
{
	if (AGSEnemyCharacter* Enemy = Cast<AGSEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		Enemy->SetTelegraphing(false);
	}
	OnShotTimer();
}

void UGSGameplayAbility_EnemyFire::OnShotTimer()
{
	AGSEnemyCharacter* Enemy = Cast<AGSEnemyCharacter>(GetAvatarActorFromActorInfo());
	const UGSEnemyData* Data = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!Data || !Data->Weapon)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FireShot(*Enemy);
	--ShotsRemaining;

	if (ShotsRemaining > 0)
	{
		const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		const float FireRate = ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute()) : 1.f;
		UAbilityTask_WaitDelay* Next = UAbilityTask_WaitDelay::WaitDelay(this, Data->Weapon->GetRefireInterval(FireRate));
		Next->OnFinish.AddDynamic(this, &UGSGameplayAbility_EnemyFire::OnShotTimer);
		Next->ReadyForActivation();
	}
	else
	{
		UAbilityTask_WaitDelay* Recovery = UAbilityTask_WaitDelay::WaitDelay(this, Data->BurstRecovery);
		Recovery->OnFinish.AddDynamic(this, &UGSGameplayAbility_EnemyFire::OnRecoveryFinished);
		Recovery->ReadyForActivation();
	}
}

void UGSGameplayAbility_EnemyFire::OnRecoveryFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

AActor* UGSGameplayAbility_EnemyFire::FaceTarget(AGSEnemyCharacter& Enemy) const
{
	const AGSEnemyAIController* AIC = Cast<AGSEnemyAIController>(Enemy.GetController());
	AActor* Target = AIC ? AIC->GetTargetActor() : nullptr;
	if (Target)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Enemy.GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			Enemy.SetActorRotation(ToTarget.Rotation());
		}
	}
	return Target;
}

void UGSGameplayAbility_EnemyFire::FireShot(AGSEnemyCharacter& Enemy)
{
	const UGSEnemyData* Data = Enemy.GetEnemyData();
	const UGSWeaponData* Weapon = Data ? Data->Weapon.Get() : nullptr;
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = Enemy.GetWorld();
	if (!Weapon || !SourceASC || !World)
	{
		return;
	}

	// Track the target between shots, then fire into a cone around where its chest is now (no lead: a player who keeps
	// moving is where the shot was aimed no longer).
	const AActor* Target = FaceTarget(Enemy);
	const FVector Muzzle = Enemy.GetMuzzleLocation();
	const FVector AimPoint = Target ? Target->GetActorLocation() + FVector(0.f, 0.f, Data->AimHeightOffset)
		: Muzzle + Enemy.GetActorForwardVector() * Weapon->Range;
	const FVector AimDirection = (AimPoint - Muzzle).GetSafeNormal();
	const FVector ShotDirection = UGSRunRandom::VRandCone(&Enemy, AimDirection, FMath::DegreesToRadians(Weapon->HipSpread));

	// A visible, dodgeable projectile per shot (never a hit-scan). It hits the player or world geometry and nothing
	// else; the damage path (dash, shield, Health, Second Wind) decides what the hit does.
	// TODO(co-op): enemy shots are local (AGSProjectile isn't replicated); clients need to see them in M6.
	const float Speed = FMath::Max(Weapon->ProjectileSpeed, 100.f);
	FGSProjectileLaunch Launch;
	Launch.Start = Muzzle;
	Launch.Direction = ShotDirection;
	Launch.Speed = Speed;
	Launch.Lifetime = Weapon->Range / Speed;
	Launch.Radius = Weapon->ProjectileRadius;
	Launch.Damage = Weapon->Damage * SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
	Launch.SourceASC = SourceASC;
	Launch.DamageEffect = DamageEffectClass;
	Launch.bEnemyShot = true;
	Launch.Mesh = Weapon->ProjectileMesh;
	Launch.Material = Weapon->ProjectileMaterial;
	Launch.Scale = Weapon->ProjectileScale;
	Launch.ImpactEffect = Weapon->ProjectileImpactEffect;
	Launch.ImpactEffectScale = Weapon->ProjectileImpactEffectScale;
	Launch.ImpactSound = Weapon->ProjectileImpactSound;

	UGSProjectileSubsystem* Projectiles = World->GetSubsystem<UGSProjectileSubsystem>();
	const AGSProjectile* Shot = Projectiles ? Projectiles->FireProjectile(Launch) : nullptr;
	++ShotsFired;

	if (GSWeaponDebug::IsEnabled())
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[shot] %s fired at %s (%.0f m away, %.2f s to get there at %.0f cm/s) at t=%.2fs"), *Enemy.GetActorNameOrLabel(),
			*GetNameSafe(Target), Target ? FVector::Dist(Muzzle, AimPoint) / 100.f : 0.f, Target ? FVector::Dist(Muzzle, AimPoint) / Speed : 0.f, Speed,
			World->GetTimeSeconds());
	}
	if (!Shot)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[shot] %s: no projectile could be fired."), *Enemy.GetActorNameOrLabel());
	}

	// Muzzle flash and sound only; the projectile shows the shot and plays the impact.
	Enemy.MulticastShotFired(Muzzle, Muzzle + ShotDirection * Weapon->Range, false);
}

void UGSGameplayAbility_EnemyFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AGSEnemyCharacter* Enemy = ActorInfo ? Cast<AGSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Enemy->SetTelegraphing(false);
	}
	ShotsRemaining = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
