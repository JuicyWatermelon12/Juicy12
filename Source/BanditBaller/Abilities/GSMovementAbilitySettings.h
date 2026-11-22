// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSMovementAbilitySettings.generated.h"

/** Tuning for GA_Slide (DA_SlideSettings). */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSSlideSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Minimum ground speed (cm/s) needed to start a slide. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0", Units = "cm/s"))
	float MinStartSpeed = 300.f;

	/** Speed added on top of the current speed when the slide starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0", Units = "cm/s"))
	float SpeedBoost = 600.f;

	/** Cap on the slide's starting speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0", Units = "cm/s"))
	float MaxSpeed = 1600.f;

	/** Speed the slide decays to by the end of Duration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0", Units = "cm/s"))
	float EndSpeed = 600.f;

	/** How long the boost takes to decay; the slide ends after this. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0.05", Units = "s"))
	float Duration = 0.8f;

	/** Decay curve exponent: 1 = linear, 2 = fast early falloff. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0.1"))
	float DecayExponent = 2.f;

	/** How fast the slide can be steered toward the movement input (deg/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0"))
	float SteerRate = 90.f;

	/** Horizontal speed multiplier applied when jumping out of a slide. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0"))
	float SlideHopSpeedMultiplier = 1.1f;

	/** Capsule half-height while sliding (standing is 96). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "20", Units = "cm"))
	float CapsuleHalfHeight = 48.f;

	/** How far the camera dips during the slide (the capsule drop is compensated). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0", Units = "cm"))
	float CameraDip = 20.f;

	/** Cooldown from slide start (GE_Cooldown_Slide). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 1.f;
};

/** Tuning for GA_Dash (DA_DashSettings). */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSDashSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	float GetDashSpeed() const { return Distance / FMath::Max(Duration, 0.01f); }

	/** Distance covered by one dash. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0", Units = "cm"))
	float Distance = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0.02", Units = "s"))
	float Duration = 0.15f;

	/** Horizontal speed kept when the dash ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0", Units = "cm/s"))
	float ExitSpeed = 900.f;

	/** Damage is ignored (State.Invulnerable) for this long after the dash starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0", Units = "s"))
	float InvulnerabilityDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	bool bAllowInAir = true;

	/** Charges the player starts a run with. The Dash Charge upgrade adds the ExtraDashCharges attribute on top. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charges", meta = (ClampMin = "1"))
	int32 MaxCharges = 2;

	/** Seconds to recharge one charge (charges recharge one after another). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charges", meta = (ClampMin = "0", Units = "s"))
	float RechargeTime = 3.f;
};
