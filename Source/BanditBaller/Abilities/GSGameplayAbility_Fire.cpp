// BanditBaller

#include "GSGameplayAbility_Fire.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "GSAbilitySystemComponent.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSCriticalHit.h"
#include "GSGameplayTags.h"
#include "GSWeapon.h"
#include "GSWeaponComponent.h"
#include "GSWeaponData.h"
#include "GSWeaponDebug.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

namespace
{
	/** A shot due within this much of now counts as due (float rounding between the schedule and world time). */
	constexpr double ShotTimeTolerance = 1e-4;

	/** Runaway guard: most shots one timer update fires before carrying on next tick (only an extreme FireRate). */
	constexpr int32 MaxShotsPerUpdate = 256;
}

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
	LastShotTime = GetWorld()->GetTimeSeconds();

	// A FireRate change re-times the pending shot, so it applies from the next shot.
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		FireRateChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetFireRateAttribute())
			.AddUObject(this, &UGSGameplayAbility_Fire::OnFireRateChanged);
	}

	ScheduleNextShot();
}

float UGSGameplayAbility_Fire::GetCurrentRefireInterval(const UGSWeaponData& WeaponData) const
{
	const UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	const float FireRateMultiplier = ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute()) : 1.f;
	return WeaponData.GetRefireInterval(FireRateMultiplier);
}

UGSGameplayAbility_Fire::ENextShot UGSGameplayAbility_Fire::CheckNextShot() const
{
	// Everything a new activation would check: alive, not reloading, no safe zone, a weapon equipped.
	if (!CurrentActorInfo || !CanActivateAbility(CurrentSpecHandle, CurrentActorInfo, nullptr, nullptr, nullptr))
	{
		return ENextShot::Blocked;
	}

	const UGSWeaponComponent* Weapons = GetWeaponComponent(CurrentActorInfo);
	const UGSWeaponData* WeaponData = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	if (!WeaponData)
	{
		return ENextShot::Blocked;
	}
	if (WeaponData->FireMode != EGSFireMode::FullAuto)
	{
		return ENextShot::Done; // semi-auto: one shot per press
	}

	const UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
	if (!ASC || !ASC->IsAbilityInputHeld(ActivationInputTag))
	{
		return ENextShot::Done; // released
	}
	if (Weapons->GetCurrentAmmo() <= 0)
	{
		return ENextShot::Done; // empty: EndAbility starts the auto-reload
	}
	return ENextShot::Fire;
}

void UGSGameplayAbility_Fire::ScheduleNextShot()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const UGSWeaponComponent* Weapons = GetWeaponComponent(CurrentActorInfo);
	const UGSWeaponData* WeaponData = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	const double Due = LastShotTime + (WeaponData ? GetCurrentRefireInterval(*WeaponData) : 0.0);
	const double Delay = Due - World->GetTimeSeconds();

	FTimerManager& Timers = World->GetTimerManager();
	Timers.ClearTimer(ShotTimerHandle);
	if (Delay > ShotTimeTolerance)
	{
		Timers.SetTimer(ShotTimerHandle, this, &UGSGameplayAbility_Fire::OnShotDue, static_cast<float>(Delay), false);
	}
	else
	{
		ShotTimerHandle = Timers.SetTimerForNextTick(this, &UGSGameplayAbility_Fire::OnShotDue);
	}
}

void UGSGameplayAbility_Fire::OnShotDue()
{
	ShotTimerHandle.Invalidate();
	const UWorld* World = GetWorld();
	if (!World || !IsActive())
	{
		return;
	}

	// Fire every shot that is due by now, each one refire interval after the previous shot's scheduled time. A slow
	// frame that spans several intervals fires every shot it owes, so the average rate stays exact.
	const double Now = World->GetTimeSeconds();
	for (int32 Shot = 0; Shot < MaxShotsPerUpdate; ++Shot)
	{
		const UGSWeaponComponent* Weapons = GetWeaponComponent(CurrentActorInfo);
		const UGSWeaponData* WeaponData = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
		const double Due = LastShotTime + (WeaponData ? GetCurrentRefireInterval(*WeaponData) : 0.0);
		if (Due > Now + ShotTimeTolerance)
		{
			// Not due yet: the timer was armed before this frame's timer update, or FireRate went down. Wait the rest.
			ScheduleNextShot();
			return;
		}

		const ENextShot Next = CheckNextShot();
		if (Next != ENextShot::Fire)
		{
			// Ends when the next shot would have been due: State.Firing lasts one refire interval past the last shot.
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, Next == ENextShot::Blocked);
			return;
		}

		FireShot(CurrentActorInfo, *WeaponData);
		LastShotTime = Due;
	}
	ScheduleNextShot(); // only after MaxShotsPerUpdate (an extreme FireRate): carry on next tick
}

void UGSGameplayAbility_Fire::OnFireRateChanged(const FOnAttributeChangeData& Data)
{
	if (IsActive() && ShotTimerHandle.IsValid())
	{
		ScheduleNextShot();
	}
}

void UGSGameplayAbility_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShotTimerHandle);
	}
	if (FireRateChangedHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetFireRateAttribute()).Remove(FireRateChangedHandle);
		}
		FireRateChangedHandle.Reset();
	}

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

FGSShotTrace UGSGameplayAbility_Fire::TraceShot(const UWorld* World, const FVector& Start, const FVector& End, int32 Pierce,
	const FCollisionQueryParams& Params)
{
	FGSShotTrace Result;
	Result.End = End;
	if (!World)
	{
		return Result;
	}

	FCollisionQueryParams Query = Params;
	const int32 MaxTargets = 1 + FMath::Max(Pierce, 0);
	for (int32 Guard = 0; Guard < 256; ++Guard)
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query))
		{
			return Result; // nothing more on the line: the shot runs to End
		}

		AActor* HitActor = Hit.GetActor();
		const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
		if (!TargetASC)
		{
			Result.BlockingHit = Hit; // a wall or other world geometry stops the shot
			Result.End = Hit.ImpactPoint;
			return Result;
		}

		// A target: hit once, then ignored so the trace continues behind it. Dead ones don't use up the pierce.
		Query.AddIgnoredActor(HitActor);
		if (TargetASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead))
		{
			continue;
		}
		Result.TargetHits.Add(Hit);
		Result.End = Hit.ImpactPoint;
		if (Result.TargetHits.Num() >= MaxTargets)
		{
			return Result; // out of pierce: the shot stops in this target
		}
	}
	return Result;
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

	// 2) Muzzle -> aim point. Without Pierce the trace ends just past the aim point (as it always has); with Pierce it
	//    runs on to the weapon's range, through up to Pierce targets, and stops at walls.
	const FVector Muzzle = Weapon->GetMuzzleLocation();
	const FVector ShotDirection = (AimPoint - Muzzle).GetSafeNormal();
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	const int32 Pierce = SourceASC ? FMath::Max(0, FMath::RoundToInt(SourceASC->GetNumericAttribute(UGSAttributeSet::GetPierceAttribute()))) : 0;
	const FVector MuzzleTraceEnd = Pierce > 0 ? Muzzle + ShotDirection * WeaponData.Range : AimPoint + ShotDirection * 10.f;

	const FGSShotTrace Shot = TraceShot(World, Muzzle, MuzzleTraceEnd, Pierce, Params);

	// 3) Authority: ammo once per shot, then full damage to every target hit, each with its own crit roll (run stream;
	//    Loaded Dice) and its own damage spec (so its own damage number at its own impact point).
	TArray<float> AppliedDamage;
	AppliedDamage.Init(0.f, Shot.TargetHits.Num());
	if (bAuthority)
	{
		Weapons->ConsumeAmmo();

		for (int32 Index = 0; Index < Shot.TargetHits.Num(); ++Index)
		{
			const FHitResult& Hit = Shot.TargetHits[Index];
			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Hit.GetActor());
			if (!SourceASC || !TargetASC || !DamageEffectClass)
			{
				if (TargetASC && !DamageEffectClass)
				{
					UE_LOG(LogBanditBaller, Warning, TEXT("%s has no DamageEffectClass; hit on %s did nothing."), *GetNameSafe(this), *GetNameSafe(Hit.GetActor()));
				}
				continue;
			}

			const FGSCritRoll Crit = GSCombat::RollCriticalHit(SourceASC);
			const float DamageMultiplier = SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
			AppliedDamage[Index] = WeaponData.Damage * DamageMultiplier * Crit.DamageMultiplier;

			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, AppliedDamage[Index]);
				SpecHandle.Data->AddDynamicAssetTag(GSGameplayTags::Damage_Source_Gun); // run stats by source
				if (Crit.bCritical)
				{
					SpecHandle.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Critical, Crit.GetLevel()); // 1 crit, 2 triple
				}
				SpecHandle.Data->GetContext().AddHitResult(Hit);
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
			}
		}
	}

	// 4) Cosmetics (placeholders; null assets are skipped): an impact at every target hit and at the wall that stopped
	//    the shot. The shot's line (debug draw; a tracer later) runs to the last thing hit.
	if (WeaponData.MuzzleEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, WeaponData.MuzzleEffect, Muzzle, ShotDirection.Rotation());
	}
	if (WeaponData.FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, WeaponData.FireSound, Muzzle);
	}
	if (WeaponData.ImpactEffect)
	{
		for (const FHitResult& Hit : Shot.TargetHits)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, WeaponData.ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		}
		if (Shot.BlockingHit.bBlockingHit)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, WeaponData.ImpactEffect, Shot.BlockingHit.ImpactPoint, Shot.BlockingHit.ImpactNormal.Rotation());
		}
	}

	if (bDebug)
	{
		const float Duration = GSWeaponDebug::DrawDuration;
		const bool bHitSomething = Shot.TargetHits.Num() > 0 || Shot.BlockingHit.bBlockingHit;
		DrawDebugLine(World, CameraTraceStart, AimPoint, FColor::Blue, false, Duration, 0, 0.5f);
		DrawDebugLine(World, Muzzle, Shot.End, bHitSomething ? FColor::Red : FColor::Green, false, Duration, 0, 1.f);
		DrawDebugSphere(World, Muzzle, 4.f, 8, FColor::Yellow, false, Duration);
		for (int32 Index = 0; Index < Shot.TargetHits.Num(); ++Index)
		{
			DrawDebugPoint(World, Shot.TargetHits[Index].ImpactPoint, 10.f, FColor::Red, false, Duration);
			if (AppliedDamage[Index] > 0.f)
			{
				DrawDebugString(World, Shot.TargetHits[Index].ImpactPoint, FString::Printf(TEXT("%.0f"), AppliedDamage[Index]), nullptr, FColor::White, Duration);
			}
		}
		if (Shot.BlockingHit.bBlockingHit)
		{
			DrawDebugPoint(World, Shot.BlockingHit.ImpactPoint, 10.f, FColor::Red, false, Duration);
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
