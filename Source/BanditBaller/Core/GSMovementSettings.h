// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSMovementSettings.generated.h"

/**
 * All player movement and camera tuning. Assigned on BP_GSCharacter (DA_PlayerMovementSettings).
 * MaxWalkSpeed seeds the GAS MoveSpeed attribute, which then drives CharacterMovement so buffs still apply.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSMovementSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Ground ---

	/** Starting value of the MoveSpeed attribute (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground", meta = (ClampMin = "0", Units = "cm/s"))
	float MaxWalkSpeed = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground", meta = (ClampMin = "0"))
	float MaxAcceleration = 6000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground", meta = (ClampMin = "0"))
	float BrakingDecelerationWalking = 4000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground", meta = (ClampMin = "0"))
	float GroundFriction = 10.f;

	// --- Air ---

	/** 0..1, how much lateral control the player has while falling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0", ClampMax = "1"))
	float AirControl = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0"))
	float AirControlBoostMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0"))
	float BrakingDecelerationFalling = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0"))
	float GravityScale = 1.75f;

	// --- Jump ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0"))
	float JumpZVelocity = 800.f;

	/** Seconds after walking off a ledge during which a jump is still allowed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0", Units = "s"))
	float CoyoteTime = 0.12f;

	// --- Rotation ---

	/** Yaw rate (deg/s) when orienting to movement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0"))
	float MoveRotationRate = 900.f;

	/** Yaw rate (deg/s) when turning to face the camera while aiming/firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0"))
	float AimRotationRate = 1440.f;

	// --- Camera ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0"))
	float ArmLength = 350.f;

	/** Spring arm socket offset (X fwd, Y right, Z up). Y is mirrored by shoulder swap. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FVector ShoulderOffset = FVector(0.f, 60.f, 60.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float DefaultFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	bool bEnableCameraLag = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0"))
	float CameraLagSpeed = 15.f;

	/** Interp speed when swapping shoulders. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0"))
	float ShoulderSwapInterpSpeed = 10.f;

	// --- Aim down sights ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim", meta = (ClampMin = "0"))
	float AimArmLength = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim", meta = (ClampMin = "5", ClampMax = "170"))
	float AimFOV = 65.f;

	/** Interp speed for FOV / arm length blend in and out of aim. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim", meta = (ClampMin = "0"))
	float AimInterpSpeed = 12.f;
};
