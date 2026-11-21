// BanditBaller

#include "GSGameplayAbility_MeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSEnemyAIController.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameplayTags.h"
#include "GSWeaponDebug.h"

UGSGameplayAbility_MeleeAttack::UGSGameplayAbility_MeleeAttack()
{
	SetAssetTags(FGameplayTagContainer(GSGameplayTags::Ability_Attack_Melee));
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dead);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGSGameplayAbility_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AGSEnemyCharacter* Enemy = ActorInfo ? Cast<AGSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UGSEnemyData* Data = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!Data || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Snap to face the target, then telegraph.
	if (const AGSEnemyAIController* AIC = Cast<AGSEnemyAIController>(Enemy->GetController()))
	{
		if (const AActor* Target = AIC->GetTargetActor())
		{
			const FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
			if (!ToTarget.IsNearlyZero())
			{
				Enemy->SetActorRotation(ToTarget.Rotation());
			}
		}
	}
	Enemy->SetTelegraphing(true);

	UAbilityTask_WaitDelay* WindUp = UAbilityTask_WaitDelay::WaitDelay(this, Data->AttackWindUp);
	WindUp->OnFinish.AddDynamic(this, &UGSGameplayAbility_MeleeAttack::OnWindUpFinished);
	WindUp->ReadyForActivation();
}

void UGSGameplayAbility_MeleeAttack::OnWindUpFinished()
{
	AGSEnemyCharacter* Enemy = Cast<AGSEnemyCharacter>(GetAvatarActorFromActorInfo());
	if (Enemy)
	{
		Enemy->SetTelegraphing(false);
	}

	PerformSweep();

	const UGSEnemyData* Data = Enemy ? Enemy->GetEnemyData() : nullptr;
	UAbilityTask_WaitDelay* Recovery = UAbilityTask_WaitDelay::WaitDelay(this, Data ? Data->AttackRecovery : 0.5f);
	Recovery->OnFinish.AddDynamic(this, &UGSGameplayAbility_MeleeAttack::OnRecoveryFinished);
	Recovery->ReadyForActivation();
}

void UGSGameplayAbility_MeleeAttack::PerformSweep()
{
	AGSEnemyCharacter* Enemy = Cast<AGSEnemyCharacter>(GetAvatarActorFromActorInfo());
	const UGSEnemyData* Data = Enemy ? Enemy->GetEnemyData() : nullptr;
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = Enemy ? Enemy->GetWorld() : nullptr;
	if (!Data || !SourceASC || !World)
	{
		return;
	}

	// Sphere overlap centered in front of the enemy (a short forward "sweep").
	const FVector Center = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * Data->AttackSweepDistance;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSMeleeSweep), false, Enemy);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Data->AttackSweepRadius), Params);

	const float Damage = Data->AttackDamage * SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());

	TSet<AActor*> HitActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		APawn* HitPawn = Cast<APawn>(Overlap.GetActor());
		// Players only (no friendly fire between enemies).
		if (!HitPawn || !HitPawn->IsPlayerControlled() || HitActors.Contains(HitPawn))
		{
			continue;
		}
		HitActors.Add(HitPawn);

		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitPawn);
		if (!TargetASC || !DamageEffectClass)
		{
			continue;
		}

		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Damage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
		}
	}

	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugSphere(World, Center, Data->AttackSweepRadius, 12, HitActors.Num() > 0 ? FColor::Red : FColor::Orange, false, GSWeaponDebug::DrawDuration);
	}
}

void UGSGameplayAbility_MeleeAttack::OnRecoveryFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGSGameplayAbility_MeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AGSEnemyCharacter* Enemy = ActorInfo ? Cast<AGSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Enemy->SetTelegraphing(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
