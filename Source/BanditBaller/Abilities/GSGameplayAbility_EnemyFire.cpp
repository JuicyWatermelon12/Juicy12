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

	// Track the target between shots, then fire into a cone around it.
	const AActor* Target = FaceTarget(Enemy);
	const FVector Muzzle = Enemy.GetMuzzleLocation();
	const FVector AimPoint = Target ? Target->GetActorLocation() : Muzzle + Enemy.GetActorForwardVector() * Weapon->Range;
	const FVector AimDirection = (AimPoint - Muzzle).GetSafeNormal();
	const FVector ShotDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(Weapon->HipSpread));
	const FVector TraceEnd = Muzzle + ShotDirection * Weapon->Range;

	// Object query, not the Visibility channel: player capsules/meshes ignore Visibility.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSEnemyFire), false, &Enemy);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByObjectType(Hit, Muzzle, TraceEnd, AGSEnemyCharacter::GetShotObjectQueryParams(), Params);

	if (GSWeaponDebug::IsEnabled())
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[shot] %s -> %s at t=%.2fs"), *Enemy.GetActorNameOrLabel(),
			bHit ? *GetNameSafe(Hit.GetActor()) : TEXT("miss"), World->GetTimeSeconds());
	}

	// Players only (enemies in the way soak the shot without friendly fire).
	APawn* HitPawn = bHit ? Cast<APawn>(Hit.GetActor()) : nullptr;
	if (HitPawn && HitPawn->IsPlayerControlled() && DamageEffectClass)
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitPawn))
		{
			const float Damage = Weapon->Damage * SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
			FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
			if (Spec.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Damage);
				Spec.Data->GetContext().AddHitResult(Hit);
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			}
		}
	}

	Enemy.MulticastShotFired(Muzzle, bHit ? FVector(Hit.ImpactPoint) : TraceEnd, bHit);
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
