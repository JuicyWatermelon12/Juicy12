// BanditBaller

#include "GSGameplayAbility_Fire.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "BanditBaller.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSGameplayTags.h"
#include "GSWeapon.h"
#include "GSWeaponComponent.h"
#include "GSWeaponData.h"
#include "GSWeaponDebug.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

UGSGameplayAbility_Fire::UGSGameplayAbility_Fire()
{
	ActivationInputTag = GSGameplayTags::Ability_Fire;
	SetAssetTags(FGameplayTagContainer(GSGameplayTags::Ability_Fire));
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Firing);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Reloading);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_SafeZone); // no shooting in the hub
}

bool UGSGameplayAbility_Fire::ShouldRetriggerWhileInputHeld(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	const UGSWeaponData* Data = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	return Data && Data->FireMode == EGSFireMode::FullAuto;
}

bool UGSGameplayAbility_Fire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	return Weapons && Weapons->GetCurrentWeaponData() && Weapons->GetCurrentWeapon();
}

void UGSGameplayAbility_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	const UGSWeaponData* WeaponData = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	if (!WeaponData || Weapons->GetCurrentAmmo() <= 0)
	{
		// Empty: EndAbility kicks off an auto-reload.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FireShot(ActorInfo, *WeaponData);

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const float FireRateMultiplier = ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute()) : 1.f;

	UAbilityTask_WaitDelay* WaitRefire = UAbilityTask_WaitDelay::WaitDelay(this, WeaponData->GetRefireInterval(FireRateMultiplier));
	WaitRefire->OnFinish.AddDynamic(this, &UGSGameplayAbility_Fire::OnRefireReady);
	WaitRefire->ReadyForActivation();
}

void UGSGameplayAbility_Fire::OnRefireReady()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGSGameplayAbility_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// Auto-reload on empty (the owning player drives it so it predicts like a button press).
	const UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	if (!bWasCancelled && ActorInfo && ActorInfo->IsLocallyControlled() && Weapons && Weapons->GetCurrentWeaponData() && Weapons->GetCurrentAmmo() <= 0)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(GSGameplayTags::Ability_Reload));
		}
	}
}

void UGSGameplayAbility_Fire::FireShot(const FGameplayAbilityActorInfo* ActorInfo, const UGSWeaponData& WeaponData)
{
	AGSCharacter* Character = GetGSCharacter(ActorInfo);
	UGSWeaponComponent* Weapons = GetWeaponComponent(ActorInfo);
	AGSWeapon* Weapon = Weapons ? Weapons->GetCurrentWeapon() : nullptr;
	AController* Controller = Character ? Character->GetController() : nullptr;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!Character || !Weapon || !Controller || !World)
	{
		return;
	}

	const bool bAuthority = ActorInfo->IsNetAuthority();
	const bool bDebug = GSWeaponDebug::IsEnabled();

	// 1) Camera ray with spread -> aim point.
	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const float SpreadDegrees = Character->IsAiming() ? WeaponData.ADSSpread : WeaponData.HipSpread;
	// Not on the run stream (UGSRunRandom): spread is rolled wherever the shot is predicted, including clients.
	const FVector AimDirection = FMath::VRandCone(ViewRotation.Vector(), FMath::DegreesToRadians(SpreadDegrees));

	// Start the camera trace level with the pawn so objects between camera and pawn are ignored.
	const float PawnDistanceAlongRay = FVector::DotProduct(Character->GetActorLocation() - ViewLocation, AimDirection);
	const FVector CameraTraceStart = ViewLocation + AimDirection * FMath::Max(PawnDistanceAlongRay, 0.f);
	const FVector CameraTraceEnd = CameraTraceStart + AimDirection * WeaponData.Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSWeaponFire), false);
	Params.AddIgnoredActor(Character);
	Params.AddIgnoredActor(Weapon);
	Params.bReturnPhysicalMaterial = true;

	FHitResult CameraHit;
	World->LineTraceSingleByChannel(CameraHit, CameraTraceStart, CameraTraceEnd, ECC_Visibility, Params);
	const FVector AimPoint = CameraHit.bBlockingHit ? CameraHit.ImpactPoint : CameraTraceEnd;

	// 2) Muzzle -> aim point (slightly past it so we register the surface).
	const FVector Muzzle = Weapon->GetMuzzleLocation();
	const FVector ShotDirection = (AimPoint - Muzzle).GetSafeNormal();
	const FVector MuzzleTraceEnd = AimPoint + ShotDirection * 10.f;

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Muzzle, MuzzleTraceEnd, ECC_Visibility, Params);
	const FVector ShotEnd = bHit ? Hit.ImpactPoint : MuzzleTraceEnd;

	// 3) Authority: ammo and damage.
	float AppliedDamage = 0.f;
	if (bAuthority)
	{
		Weapons->ConsumeAmmo();

		UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
		UAbilitySystemComponent* TargetASC = bHit ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Hit.GetActor()) : nullptr;

		if (SourceASC && TargetASC && DamageEffectClass)
		{
			const float DamageMultiplier = SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
			AppliedDamage = WeaponData.Damage * DamageMultiplier;

			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, AppliedDamage);
				SpecHandle.Data->GetContext().AddHitResult(Hit);
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
			}
		}
		else if (TargetASC && !DamageEffectClass)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("%s has no DamageEffectClass; hit on %s did nothing."), *GetNameSafe(this), *GetNameSafe(Hit.GetActor()));
		}
	}

	// 4) Cosmetics (placeholders; null assets are skipped).
	if (WeaponData.MuzzleEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, WeaponData.MuzzleEffect, Muzzle, ShotDirection.Rotation());
	}
	if (WeaponData.FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, WeaponData.FireSound, Muzzle);
	}
	if (bHit && WeaponData.ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, WeaponData.ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	if (bDebug)
	{
		const float Duration = GSWeaponDebug::DrawDuration;
		DrawDebugLine(World, CameraTraceStart, AimPoint, FColor::Blue, false, Duration, 0, 0.5f);
		DrawDebugLine(World, Muzzle, ShotEnd, bHit ? FColor::Red : FColor::Green, false, Duration, 0, 1.f);
		DrawDebugSphere(World, Muzzle, 4.f, 8, FColor::Yellow, false, Duration);
		if (bHit)
		{
			DrawDebugPoint(World, Hit.ImpactPoint, 10.f, FColor::Red, false, Duration);
			if (AppliedDamage > 0.f)
			{
				DrawDebugString(World, Hit.ImpactPoint, FString::Printf(TEXT("%.0f"), AppliedDamage), nullptr, FColor::White, Duration);
			}
		}
	}

	// 5) Recoil for the player who fired.
	if (ActorInfo->IsLocallyControlled())
	{
		ApplyRecoil(ActorInfo, WeaponData);
	}
}

void UGSGameplayAbility_Fire::ApplyRecoil(const FGameplayAbilityActorInfo* ActorInfo, const UGSWeaponData& WeaponData) const
{
	AController* Controller = ActorInfo->PlayerController.Get();
	if (!Controller)
	{
		return;
	}

	FRotator Rotation = Controller->GetControlRotation();
	Rotation.Pitch += WeaponData.RecoilKick;
	Rotation.Yaw += FMath::FRandRange(-WeaponData.RecoilYaw, WeaponData.RecoilYaw);
	Controller->SetControlRotation(Rotation);
}
