// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "BanditBallerCharacter.h"
#include "GameplayTagContainer.h"
#include "GSCharacter.generated.h"

class UAbilitySystemComponent;
class UGSAbilitySet;
class UGSInputConfig;
class UGSMovementSettings;
class UInputAction;
struct FOnAttributeChangeData;

/**
 * Player character. Third-person camera/movement/input come from the template character.
 * The ASC lives on AGSPlayerState; this pawn is its avatar.
 * Adds an over-the-shoulder camera with ADS and shoulder swap, aim/fire facing, coyote time,
 * and routes ability inputs to the ASC by gameplay tag. All tuning is in UGSMovementSettings.
 */
UCLASS()
class BANDITBALLER_API AGSCharacter : public ABanditBallerCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSCharacter();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//~ AActor
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsAiming() const { return bAimHeld; }

	/** True while the pawn turns to face the camera (aiming or firing). */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsFacingCamera() const { return bFacingCamera; }

protected:
	//~ AActor / APawn / ACharacter
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual bool CanJumpWhileFalling() const override;

	/** Abilities/effects granted on first spawn (e.g. DA_PlayerAbilitySet with GE_InitPlayerStats). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UGSAbilitySet> StartupAbilitySet;

	/** Movement + camera tuning (DA_PlayerMovementSettings). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UGSMovementSettings> MovementSettings;

	/** Input actions that activate abilities by tag (DA_PlayerInputConfig). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UGSInputConfig> InputConfig;

	/** Hold to aim down sights (local camera + facing; not an ability). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	/** Press to swap the camera to the other shoulder. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ShoulderSwapAction;

private:
	// --- Ability system ---
	void InitAbilitySystem();
	void ClearAbilitySystem();
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void ApplyMoveSpeed(float NewSpeed);

	// --- Input ---
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void OnAimStarted();
	void OnAimCompleted();
	void OnShoulderSwap();

	// --- Movement / camera ---
	void ApplyMovementSettings();
	void UpdateFacingFromInput();
	void ApplyFacingMode();
	void UpdateCamera(float DeltaSeconds);

	UFUNCTION(Server, Reliable)
	void ServerSetFacingCamera(bool bNewFacingCamera);

	UFUNCTION()
	void OnRep_FacingCamera();

	/** Replicated so the server and other clients simulate the same rotation mode. */
	UPROPERTY(ReplicatedUsing = OnRep_FacingCamera)
	bool bFacingCamera = false;

	// Local-only input state.
	bool bAimHeld = false;
	bool bFireHeld = false;

	// Local-only camera state. ShoulderSide: +1 right, -1 left.
	float ShoulderSide = 1.f;
	float CurrentShoulderY = 0.f;
	float CurrentArmLength = 0.f;
	float CurrentFOV = 90.f;

	/** World time when the pawn last left the ground without jumping (for coyote time). */
	double LastGroundedTime = -1000.0;

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystem;
	FDelegateHandle MoveSpeedChangedHandle;
};
