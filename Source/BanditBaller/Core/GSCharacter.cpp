// BanditBaller

#include "GSCharacter.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GSAbilitySystemComponent.h"
#include "GSAttributeSet.h"
#include "GSGameplayTags.h"
#include "GSInputConfig.h"
#include "GSMovementAbilitySettings.h"
#include "GSMovementSettings.h"
#include "GSPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GSFloatingGunComponent.h"
#include "GSFloatingShieldsComponent.h"
#include "GSHealthComponent.h"
#include "GSWeaponComponent.h"
#include "GSProgressionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"

AGSCharacter::AGSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponComponent = CreateDefaultSubobject<UGSWeaponComponent>(TEXT("WeaponComponent"));
	HealthComponent = CreateDefaultSubobject<UGSHealthComponent>(TEXT("HealthComponent"));
	FloatingShields = CreateDefaultSubobject<UGSFloatingShieldsComponent>(TEXT("FloatingShields"));
	FloatingShields->SetupAttachment(GetRootComponent());
	FloatingGuns = CreateDefaultSubobject<UGSFloatingGunComponent>(TEXT("FloatingGuns"));

	// Slides use crouch for the low capsule.
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	// By default movement ticks before its owner (the engine adds that prerequisite). Slide/dash set
	// velocity in our Tick, so we need the opposite order; BeginPlay adds "movement after character".
	// Leaving this on would make that a cycle (LogTick warning) and the engine would drop our ordering.
	GetCharacterMovement()->bTickBeforeOwner = false;
}

UAbilitySystemComponent* AGSCharacter::GetAbilitySystemComponent() const
{
	return CachedAbilitySystem.Get();
}

void AGSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The owning client sets these locally before telling the server.
	DOREPLIFETIME_CONDITION(AGSCharacter, bFacingCamera, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AGSCharacter, bAimingReplicated, COND_SkipOwner);
}

void AGSCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyMovementSettings();
	ApplyFacingMode();

	// Slide/dash override velocity in our Tick; make sure movement runs after it every frame
	// (requires bTickBeforeOwner = false, set in the constructor).
	GetCharacterMovement()->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);

	HealthComponent->OnHealthChanged.AddDynamic(this, &AGSCharacter::OnHealthChanged);
	HealthComponent->OnDeathStarted.AddDynamic(this, &AGSCharacter::OnDeathStarted);
}

void AGSCharacter::OnHealthChanged(UGSHealthComponent* InHealthComponent, float OldHealth, float NewHealth, AActor* DamageInstigator)
{
	if (NewHealth < OldHealth && IsLocallyControlled() && HitCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->ClientStartCameraShake(HitCameraShake);
		}
	}
}

void AGSCharacter::OnDeathStarted(AActor* DeadActor, AActor* Killer)
{
	// Stop moving and acting.
	bAimHeld = false;
	bFireHeld = false;
	if (UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(CachedAbilitySystem.Get()))
	{
		ASC->ClearAbilityInput();
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->StopMovementImmediately();
	CMC->DisableMovement();
	CMC->SetComponentTickEnabled(false);

	// Ragdoll (runs on every machine; the capsule stays put so the camera keeps its pivot).
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	MeshComp->bBlendPhysics = true;
}

// ---------------------------------------------------------------------------
// Ability system
// ---------------------------------------------------------------------------

void AGSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server: bind avatar, then grant the startup set once per PlayerState.
	InitAbilitySystem();

	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (PS && PS->GrantStartupAbilitySet(StartupAbilitySet) && MovementSettings)
	{
		// Seed MoveSpeed from the movement settings; GAS owns it from here so buffs/debuffs apply.
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			ASC->SetNumericAttributeBase(UGSAttributeSet::GetMoveSpeedAttribute(), MovementSettings->MaxWalkSpeed);
		}
	}
}

void AGSCharacter::UnPossessed()
{
	if (UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(CachedAbilitySystem.Get()))
	{
		ASC->ClearAbilityInput();
	}

	ClearAbilitySystem();
	Super::UnPossessed();
}

void AGSCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client: bind avatar once the PlayerState has replicated (granting is server-only).
	InitAbilitySystem();
}

void AGSCharacter::InitAbilitySystem()
{
	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	ClearAbilitySystem();

	ASC->InitAbilityActorInfo(PS, this);
	CachedAbilitySystem = ASC;
	HealthComponent->InitializeWithAbilitySystem(ASC);

	MoveSpeedChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &AGSCharacter::OnMoveSpeedChanged);

	ExtraJumpsChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetExtraJumpsAttribute())
		.AddUObject(this, &AGSCharacter::OnExtraJumpsChanged);

	// Already initialized (e.g. respawn): sync immediately. A 0 value means stats aren't applied yet.
	ApplyMoveSpeed(ASC->GetNumericAttribute(UGSAttributeSet::GetMoveSpeedAttribute()));
	ApplyExtraJumps(ASC->GetNumericAttribute(UGSAttributeSet::GetExtraJumpsAttribute()));

	// Level-up glow on this pawn (server and every client see it; OnLevelUp also fires from replication).
	if (UGSProgressionComponent* Progression = PS->GetProgressionComponent())
	{
		Progression->OnLevelUp.AddUniqueDynamic(this, &AGSCharacter::HandleLevelUp);
		CachedProgression = Progression;
	}
}

void AGSCharacter::ClearAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = CachedAbilitySystem.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMoveSpeedAttribute()).Remove(MoveSpeedChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetExtraJumpsAttribute()).Remove(ExtraJumpsChangedHandle);
	}
	if (UGSProgressionComponent* Progression = CachedProgression.Get())
	{
		Progression->OnLevelUp.RemoveDynamic(this, &AGSCharacter::HandleLevelUp);
	}

	MoveSpeedChangedHandle.Reset();
	ExtraJumpsChangedHandle.Reset();
	CachedAbilitySystem.Reset();
	CachedProgression.Reset();
}

void AGSCharacter::HandleLevelUp(int32 NewLevel)
{
	// Cosmetic only; several level-ups in one frame just restart it.
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!LevelUpGlowMaterial || !MeshComp || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!LevelUpGlowMID)
	{
		LevelUpGlowMID = UMaterialInstanceDynamic::Create(LevelUpGlowMaterial, this);
	}
	LevelUpGlowMID->SetScalarParameterValue(LevelUpGlowParameter, 1.f);
	MeshComp->SetOverlayMaterial(LevelUpGlowMID);
	LevelUpGlowRemaining = LevelUpGlowDuration;
}

void AGSCharacter::TickLevelUpGlow(float DeltaSeconds)
{
	LevelUpGlowRemaining = FMath::Max(LevelUpGlowRemaining - DeltaSeconds, 0.f);
	const float Alpha = LevelUpGlowDuration > 0.f ? LevelUpGlowRemaining / LevelUpGlowDuration : 0.f;
	if (LevelUpGlowMID)
	{
		LevelUpGlowMID->SetScalarParameterValue(LevelUpGlowParameter, Alpha);
	}
	if (LevelUpGlowRemaining <= 0.f && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(nullptr);
	}
}

void AGSCharacter::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	ApplyMoveSpeed(Data.NewValue);
}

void AGSCharacter::ApplyMoveSpeed(float NewSpeed)
{
	if (NewSpeed > 0.f)
	{
		GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
	}
}

void AGSCharacter::OnExtraJumpsChanged(const FOnAttributeChangeData& Data)
{
	ApplyExtraJumps(Data.NewValue);
}

void AGSCharacter::ApplyExtraJumps(float NewExtraJumps)
{
	// Takes effect at once, also mid-air (a jump granted while falling can be used right away).
	JumpMaxCount = 1 + FMath::Max(0, FMath::RoundToInt(NewExtraJumps));
}

void AGSCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// JumpCurrentCount already includes this jump: 1 = the ground (or coyote) jump; 2+ = an air jump. Walking off a
	// ledge past the coyote window makes the first press count 2 as well (the engine skips the missed ground jump).
	if (JumpCurrentCount < 2 || !MovementSettings)
	{
		return;
	}

	// Same upward speed for every air jump, however fast we are falling (the engine alone only raises a slower Z).
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->Velocity.Z = FMath::Max(MovementSettings->AirJumpZVelocity, 0.f);
	LastAirJumpZVelocity = CMC->Velocity.Z;
	++AirJumpsSinceLanded;

	// Cosmetics once per real jump, not again when a client replays saved moves.
	if (!bClientUpdating)
	{
		PlayAirJumpFeedback();
	}
}

void AGSCharacter::PlayAirJumpFeedback()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// TODO(co-op): only the machines that simulate this jump (owner + server) see it; multicast it in M6.
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (AirJumpEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, AirJumpEffect, Feet, FRotator::ZeroRotator, AirJumpEffectScale);
	}
	if (AirJumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AirJumpSound, Feet);
	}
	++AirJumpFeedbackCount;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void AGSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Template binds Move / Look / MouseLook / Jump.
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		return;
	}

	if (AimAction)
	{
		EIC->BindAction(AimAction, ETriggerEvent::Started, this, &AGSCharacter::OnAimStarted);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &AGSCharacter::OnAimCompleted);
		EIC->BindAction(AimAction, ETriggerEvent::Canceled, this, &AGSCharacter::OnAimCompleted);
	}

	if (ShoulderSwapAction)
	{
		EIC->BindAction(ShoulderSwapAction, ETriggerEvent::Started, this, &AGSCharacter::OnShoulderSwap);
	}

	if (InputConfig)
	{
		for (const FGSInputAction& Entry : InputConfig->AbilityInputActions)
		{
			if (Entry.InputAction && Entry.InputTag.IsValid())
			{
				EIC->BindAction(Entry.InputAction, ETriggerEvent::Started, this, &AGSCharacter::OnAbilityInputPressed, Entry.InputTag);
				EIC->BindAction(Entry.InputAction, ETriggerEvent::Completed, this, &AGSCharacter::OnAbilityInputReleased, Entry.InputTag);
				EIC->BindAction(Entry.InputAction, ETriggerEvent::Canceled, this, &AGSCharacter::OnAbilityInputReleased, Entry.InputTag);
			}
		}
	}
	else
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no InputConfig; ability inputs are unbound."), *GetNameSafe(this));
	}
}

void AGSCharacter::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (InputTag == GSGameplayTags::Ability_Fire)
	{
		bFireHeld = true;
		UpdateFacingFromInput();
	}

	if (UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(CachedAbilitySystem.Get()))
	{
		ASC->AbilityInputTagPressed(InputTag);
	}
}

void AGSCharacter::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (InputTag == GSGameplayTags::Ability_Fire)
	{
		bFireHeld = false;
		UpdateFacingFromInput();
	}

	if (UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(CachedAbilitySystem.Get()))
	{
		ASC->AbilityInputTagReleased(InputTag);
	}
}

void AGSCharacter::OnAimStarted()
{
	bAimHeld = true;
	UpdateFacingFromInput();
}

void AGSCharacter::OnAimCompleted()
{
	bAimHeld = false;
	UpdateFacingFromInput();
}

void AGSCharacter::OnShoulderSwap()
{
	ShoulderSide = -ShoulderSide;
}

// ---------------------------------------------------------------------------
// Movement / facing
// ---------------------------------------------------------------------------

void AGSCharacter::ApplyMovementSettings()
{
	if (!MovementSettings)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no MovementSettings; using template defaults."), *GetNameSafe(this));
		return;
	}

	const UGSMovementSettings& S = *MovementSettings;
	UCharacterMovementComponent* CMC = GetCharacterMovement();

	// MaxWalkSpeed is owned by the MoveSpeed attribute; this is only the value until GAS syncs.
	CMC->MaxWalkSpeed = S.MaxWalkSpeed;
	CMC->MaxAcceleration = S.MaxAcceleration;
	CMC->BrakingDecelerationWalking = S.BrakingDecelerationWalking;
	CMC->GroundFriction = S.GroundFriction;
	CMC->AirControl = S.AirControl;
	CMC->AirControlBoostMultiplier = S.AirControlBoostMultiplier;
	CMC->BrakingDecelerationFalling = S.BrakingDecelerationFalling;
	CMC->GravityScale = S.GravityScale;
	CMC->JumpZVelocity = S.JumpZVelocity;

	USpringArmComponent* Boom = GetCameraBoom();
	Boom->TargetArmLength = S.ArmLength;
	Boom->SocketOffset = FVector(S.ShoulderOffset.X, S.ShoulderOffset.Y * ShoulderSide, S.ShoulderOffset.Z);
	Boom->bEnableCameraLag = S.bEnableCameraLag;
	Boom->CameraLagSpeed = S.CameraLagSpeed;

	GetFollowCamera()->SetFieldOfView(S.DefaultFOV);

	CurrentArmLength = S.ArmLength;
	CurrentShoulderY = S.ShoulderOffset.Y * ShoulderSide;
	CurrentFOV = S.DefaultFOV;
}

void AGSCharacter::UpdateFacingFromInput()
{
	const bool bNewFacingCamera = bAimHeld || bFireHeld;
	if (bNewFacingCamera == bFacingCamera && bAimHeld == bAimingReplicated)
	{
		return;
	}

	bFacingCamera = bNewFacingCamera;
	bAimingReplicated = bAimHeld;
	ApplyFacingMode();

	if (!HasAuthority())
	{
		ServerSetAimState(bAimHeld, bNewFacingCamera);
	}
}

void AGSCharacter::ServerSetAimState_Implementation(bool bNewAiming, bool bNewFacingCamera)
{
	bAimingReplicated = bNewAiming;
	bFacingCamera = bNewFacingCamera;
	ApplyFacingMode();
}

void AGSCharacter::OnRep_FacingCamera()
{
	ApplyFacingMode();
}

void AGSCharacter::ApplyFacingMode()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->bOrientRotationToMovement = !bFacingCamera;
	CMC->bUseControllerDesiredRotation = bFacingCamera;

	if (MovementSettings)
	{
		const float Rate = bFacingCamera ? MovementSettings->AimRotationRate : MovementSettings->MoveRotationRate;
		CMC->RotationRate = FRotator(0.f, Rate, 0.f);
	}
}

void AGSCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// Walked off a ledge (a jump would already have counted) -> start the coyote window.
	const bool bWasGrounded = PrevMovementMode == MOVE_Walking || PrevMovementMode == MOVE_NavWalking;
	if (bWasGrounded && GetCharacterMovement()->IsFalling() && JumpCurrentCount == 0)
	{
		LastGroundedTime = GetWorld()->GetTimeSeconds();
	}

	// Landed: the engine resets JumpCurrentCount (ResetJumpState); air jumps are available again.
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		AirJumpsSinceLanded = 0;
	}
}

bool AGSCharacter::CanJumpWhileFalling() const
{
	if (!MovementSettings || JumpCurrentCount != 0)
	{
		return Super::CanJumpWhileFalling();
	}

	return (GetWorld()->GetTimeSeconds() - LastGroundedTime) <= MovementSettings->CoyoteTime;
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

void AGSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Runs before CharacterMovement ticks this frame, so the velocity we set is what it moves with.
	if (IsSliding())
	{
		TickSlide(DeltaSeconds);
	}
	if (IsDashing())
	{
		TickDash(DeltaSeconds);
	}
	if (LevelUpGlowRemaining > 0.f)
	{
		TickLevelUpGlow(DeltaSeconds);
	}

	if (IsLocallyControlled())
	{
		UpdateCamera(DeltaSeconds);

		// Full-auto: keep re-activating abilities whose input is held.
		if (UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(CachedAbilitySystem.Get()))
		{
			ASC->ProcessHeldAbilityInput();
		}
	}
}

void AGSCharacter::UpdateCamera(float DeltaSeconds)
{
	if (!MovementSettings)
	{
		return;
	}

	const UGSMovementSettings& S = *MovementSettings;

	const float TargetArm = bAimHeld ? S.AimArmLength : S.ArmLength;
	const float TargetFOV = bAimHeld ? S.AimFOV : S.DefaultFOV;
	const float TargetShoulderY = S.ShoulderOffset.Y * ShoulderSide;

	CurrentArmLength = FMath::FInterpTo(CurrentArmLength, TargetArm, DeltaSeconds, S.AimInterpSpeed);
	CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, S.AimInterpSpeed);
	CurrentShoulderY = FMath::FInterpTo(CurrentShoulderY, TargetShoulderY, DeltaSeconds, S.ShoulderSwapInterpSpeed);

	// Slide: the shrunk capsule drops the boom; raise it back so the net dip is the slide's CameraDip.
	CurrentCameraHeightOffset = FMath::FInterpTo(CurrentCameraHeightOffset, bIsCrouched ? TargetCameraHeightOffset : 0.f, DeltaSeconds, 12.f);

	USpringArmComponent* Boom = GetCameraBoom();
	Boom->TargetArmLength = CurrentArmLength;
	Boom->SocketOffset = FVector(S.ShoulderOffset.X, CurrentShoulderY, S.ShoulderOffset.Z);
	Boom->TargetOffset = FVector(0.f, 0.f, CurrentCameraHeightOffset);

	GetFollowCamera()->SetFieldOfView(CurrentFOV);
}

// ---------------------------------------------------------------------------
// Slide / dash movement
// ---------------------------------------------------------------------------

FVector AGSCharacter::GetMovementInputDirection() const
{
	// Local input first; on the server for remote players, the replicated acceleration.
	FVector Input = GetLastMovementInputVector();
	if (Input.IsNearlyZero())
	{
		Input = GetCharacterMovement()->GetCurrentAcceleration();
	}
	Input.Z = 0.f;
	return Input.GetSafeNormal();
}

void AGSCharacter::SaveAndClearFriction()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (FrictionOverrideCount++ == 0)
	{
		SavedGroundFriction = CMC->GroundFriction;
		SavedBrakingDecelerationWalking = CMC->BrakingDecelerationWalking;
		SavedBrakingDecelerationFalling = CMC->BrakingDecelerationFalling;
	}

	// We drive the speed ourselves; stop CharacterMovement from braking it away.
	CMC->GroundFriction = 0.f;
	CMC->BrakingDecelerationWalking = 0.f;
	CMC->BrakingDecelerationFalling = 0.f;
}

void AGSCharacter::RestoreFriction()
{
	if (FrictionOverrideCount > 0 && --FrictionOverrideCount == 0)
	{
		UCharacterMovementComponent* CMC = GetCharacterMovement();
		CMC->GroundFriction = SavedGroundFriction;
		CMC->BrakingDecelerationWalking = SavedBrakingDecelerationWalking;
		CMC->BrakingDecelerationFalling = SavedBrakingDecelerationFalling;
	}
}

bool AGSCharacter::CanStartSlide(float MinSpeed) const
{
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	return !IsSliding() && CMC->IsMovingOnGround() && CMC->Velocity.Size2D() >= MinSpeed;
}

void AGSCharacter::StartSlide(const UGSSlideSettings& Settings)
{
	if (IsSliding())
	{
		return;
	}

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	const FVector FlatVelocity(CMC->Velocity.X, CMC->Velocity.Y, 0.f);
	SlideDirection = FlatVelocity.IsNearlyZero() ? GetActorForwardVector() : FlatVelocity.GetSafeNormal();
	SlideStartSpeed = FMath::Min(FlatVelocity.Size() + Settings.SpeedBoost, Settings.MaxSpeed);
	SlideElapsed = 0.f;
	ActiveSlide = &Settings;

	SaveAndClearFriction();

	// Low capsule via crouch; keep the camera dip to CameraDip.
	const float StandingHalfHeight = GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	CMC->SetCrouchedHalfHeight(Settings.CapsuleHalfHeight);
	TargetCameraHeightOffset = FMath::Max(StandingHalfHeight - Settings.CapsuleHalfHeight - Settings.CameraDip, 0.f);
	Crouch();

	CMC->Velocity = SlideDirection * SlideStartSpeed + FVector(0.f, 0.f, CMC->Velocity.Z);
}

void AGSCharacter::TickSlide(float DeltaSeconds)
{
	const UGSSlideSettings* Settings = ActiveSlide.Get();
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!Settings || !CMC->IsMovingOnGround())
	{
		// Left the ground (slid off a ledge or jumped): keep momentum, end the slide.
		StopSlide();
		return;
	}

	SlideElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(SlideElapsed / Settings->Duration, 0.f, 1.f);
	const float Speed = Settings->EndSpeed + (SlideStartSpeed - Settings->EndSpeed) * FMath::Pow(1.f - Alpha, Settings->DecayExponent);

	// Gentle steering toward the movement input.
	const FVector Input = GetMovementInputDirection();
	if (!Input.IsNearlyZero() && Settings->SteerRate > 0.f)
	{
		const float MaxRadians = FMath::DegreesToRadians(Settings->SteerRate * DeltaSeconds);
		const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(SlideDirection, Input), -1.f, 1.f));
		if (Angle > KINDA_SMALL_NUMBER)
		{
			const FVector Axis = FVector::CrossProduct(SlideDirection, Input).GetSafeNormal();
			if (!Axis.IsNearlyZero())
			{
				SlideDirection = SlideDirection.RotateAngleAxisRad(FMath::Min(Angle, MaxRadians), Axis).GetSafeNormal2D();
			}
		}
	}

	CMC->Velocity = SlideDirection * Speed + FVector(0.f, 0.f, CMC->Velocity.Z);

	if (Alpha >= 1.f)
	{
		StopSlide();
	}
}

void AGSCharacter::StopSlide()
{
	if (!IsSliding())
	{
		return;
	}

	ActiveSlide.Reset();
	UnCrouch();
	RestoreFriction();
	OnSlideFinished.Broadcast();
}

void AGSCharacter::Jump()
{
	// The dash drives velocity every tick (and zeroes Z), so a jump now would be eaten but still counted.
	if (IsDashing())
	{
		bJumpQueuedAfterDash = true;
		return;
	}

	if (const UGSSlideSettings* Settings = ActiveSlide.Get())
	{
		// Slide-hop: leave the slide and carry (optionally boosted) speed into the jump.
		UCharacterMovementComponent* CMC = GetCharacterMovement();
		CMC->Velocity.X *= Settings->SlideHopSpeedMultiplier;
		CMC->Velocity.Y *= Settings->SlideHopSpeedMultiplier;
		StopSlide();
	}

	Super::Jump();
}

bool AGSCharacter::CanJumpInternal_Implementation() const
{
	// Still crouched for a frame after leaving a slide (uncrouch is pending) - allow the jump anyway.
	if (bIsCrouched && !GetCharacterMovement()->bWantsToCrouch)
	{
		return JumpIsAllowedInternal();
	}
	return Super::CanJumpInternal_Implementation();
}

FVector AGSCharacter::GetDashDirection() const
{
	const FVector Input = GetMovementInputDirection();
	if (!Input.IsNearlyZero())
	{
		return Input;
	}

	const FRotator ControlYaw(0.f, GetControlRotation().Yaw, 0.f);
	return ControlYaw.Vector();
}

void AGSCharacter::StartDash(const FVector& Direction, const UGSDashSettings& Settings)
{
	if (IsDashing())
	{
		// Chaining dashes: keep a queued jump for the end of the new dash instead of firing it into it.
		const bool bKeepQueuedJump = bJumpQueuedAfterDash;
		bJumpQueuedAfterDash = false;
		StopDash();
		bJumpQueuedAfterDash = bKeepQueuedJump;
	}
	if (IsSliding())
	{
		StopSlide();
	}

	DashDirection = Direction.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector();
	}
	DashElapsed = 0.f;
	DashStartLocation = GetActorLocation();
	DashLastTraveled = 0.f;
	DashLastRequested = 0.f;
	ActiveDash = &Settings;

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	SaveAndClearFriction();
	SavedGravityScale = CMC->GravityScale;
	CMC->GravityScale = 0.f;

	// TickDash sets the (distance-capped) dash velocity before movement runs; hold still until then
	// so a full-speed frame can't slip through.
	CMC->Velocity = FVector::ZeroVector;
}

void AGSCharacter::TickDash(float DeltaSeconds)
{
	const UGSDashSettings* Settings = ActiveDash.Get();
	if (!Settings)
	{
		StopDash();
		return;
	}

	// Measure actual progress along the dash, so frame hitches and collisions can't skew the distance.
	const float Traveled = FVector::DotProduct(GetActorLocation() - DashStartLocation, DashDirection);
	const float Remaining = Settings->Distance - Traveled;

	// Blocked: last frame asked for real movement but we barely moved (wall / obstacle).
	const bool bBlocked = DashLastRequested > 5.f && (Traveled - DashLastTraveled) < DashLastRequested * 0.1f;

	// Done (CharacterMovement then moves with the exit speed this frame). The time cap is only a fallback.
	if (Remaining <= 1.f || bBlocked || DashElapsed >= Settings->Duration + 1.f)
	{
		StopDash();
		return;
	}

	// Frame-rate independent: never move further this frame than the distance left.
	const float Speed = Settings->GetDashSpeed();
	const float FrameSpeed = DeltaSeconds > 0.f ? FMath::Min(Speed, Remaining / DeltaSeconds) : Speed;

	GetCharacterMovement()->Velocity = DashDirection * FrameSpeed;
	DashLastTraveled = Traveled;
	DashLastRequested = FrameSpeed * DeltaSeconds;
	DashElapsed += DeltaSeconds;
}

void AGSCharacter::StopDash()
{
	const UGSDashSettings* Settings = ActiveDash.Get();
	if (!IsDashing())
	{
		return;
	}

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	const float ExitSpeed = Settings ? FMath::Min(Settings->ExitSpeed, Settings->GetDashSpeed()) : 0.f;
	CMC->Velocity = DashDirection * ExitSpeed;
	CMC->GravityScale = SavedGravityScale;

	ActiveDash.Reset();
	RestoreFriction();
	OnDashFinished.Broadcast();

	// A jump pressed during the dash happens now (ground jump, coyote jump or air jump as usual).
	if (bJumpQueuedAfterDash)
	{
		bJumpQueuedAfterDash = false;
		Jump();
	}
}
