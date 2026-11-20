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
#include "GSMovementSettings.h"
#include "GSPlayerState.h"
#include "Net/UnrealNetwork.h"

AGSCharacter::AGSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

UAbilitySystemComponent* AGSCharacter::GetAbilitySystemComponent() const
{
	return CachedAbilitySystem.Get();
}

void AGSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The owning client sets this locally before telling the server.
	DOREPLIFETIME_CONDITION(AGSCharacter, bFacingCamera, COND_SkipOwner);
}

void AGSCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyMovementSettings();
	ApplyFacingMode();
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

	MoveSpeedChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &AGSCharacter::OnMoveSpeedChanged);

	// Already initialized (e.g. respawn): sync immediately. A 0 value means stats aren't applied yet.
	ApplyMoveSpeed(ASC->GetNumericAttribute(UGSAttributeSet::GetMoveSpeedAttribute()));
}

void AGSCharacter::ClearAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = CachedAbilitySystem.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMoveSpeedAttribute()).Remove(MoveSpeedChangedHandle);
	}

	MoveSpeedChangedHandle.Reset();
	CachedAbilitySystem.Reset();
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
	if (bNewFacingCamera == bFacingCamera)
	{
		return;
	}

	bFacingCamera = bNewFacingCamera;
	ApplyFacingMode();

	if (!HasAuthority())
	{
		ServerSetFacingCamera(bNewFacingCamera);
	}
}

void AGSCharacter::ServerSetFacingCamera_Implementation(bool bNewFacingCamera)
{
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

	if (IsLocallyControlled())
	{
		UpdateCamera(DeltaSeconds);
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

	USpringArmComponent* Boom = GetCameraBoom();
	Boom->TargetArmLength = CurrentArmLength;
	Boom->SocketOffset = FVector(S.ShoulderOffset.X, CurrentShoulderY, S.ShoulderOffset.Z);

	GetFollowCamera()->SetFieldOfView(CurrentFOV);
}
