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
class UCameraShakeBase;
class UGSHealthComponent;
class UGSFloatingShieldsComponent;
class UGSFloatingGunComponent;
class UGSRazorCardsComponent;
class UGSBarbedWireComponent;
class UGSDynamiteComponent;
class UGSLuckyCoinComponent;
class UGSDashSettings;
class UGSMovementSettings;
class UGSSlideSettings;
class UGSWeaponComponent;
class UInputAction;
class UGSProgressionComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
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

	/** Aiming down sights. Local input on the owning client, replicated value elsewhere (server uses it for spread). */
	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsAiming() const { return IsLocallyControlled() ? bAimHeld : bAimingReplicated; }

	UFUNCTION(BlueprintPure, Category = "Weapons")
	UGSWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	UFUNCTION(BlueprintPure, Category = "Health")
	UGSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/** The Floating Shields you see (visual only; the shields' state is the PlayerState's UGSShieldComponent). */
	UGSFloatingShieldsComponent* GetFloatingShields() const { return FloatingShields; }

	/** The Floating Guns (spawned from the FloatingGunLevel attribute). */
	UGSFloatingGunComponent* GetFloatingGuns() const { return FloatingGuns; }

	/** Razor Cards (throws while the RazorCardsLevel attribute is 1 or more). */
	UGSRazorCardsComponent* GetRazorCards() const { return RazorCards; }

	/** Barbed Wire (a damaging ring while the BarbedWireLevel attribute is 1 or more). */
	UGSBarbedWireComponent* GetBarbedWire() const { return BarbedWire; }

	/** Dynamite (lobs sticks while the DynamiteLevel attribute is 1 or more). */
	UGSDynamiteComponent* GetDynamite() const { return Dynamite; }

	/** Lucky Coin (flips bouncing coins while the LuckyCoinLevel attribute is 1 or more). */
	UGSLuckyCoinComponent* GetLuckyCoin() const { return LuckyCoin; }

	// --- Camera debug / tests (local player): the same as the aim and shoulder-swap inputs ---
	void SetAimingForDebug(bool bAim) { bAim ? OnAimStarted() : OnAimCompleted(); }
	void SwapShoulderForDebug() { OnShoulderSwap(); }
	/** +1 camera over the right shoulder, -1 over the left. */
	float GetShoulderSide() const { return ShoulderSide; }

	/** True while the pawn turns to face the camera (aiming or firing). */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsFacingCamera() const { return bFacingCamera; }

	// --- Movement abilities (driven by GA_Slide / GA_Dash; runs on owner and server) ---

	bool CanStartSlide(float MinSpeed) const;
	void StartSlide(const UGSSlideSettings& Settings);
	void StopSlide();
	bool IsSliding() const { return ActiveSlide.IsValid(); }

	/** Movement input direction (flattened), else camera forward. */
	FVector GetDashDirection() const;
	void StartDash(const FVector& Direction, const UGSDashSettings& Settings);
	void StopDash();
	bool IsDashing() const { return ActiveDash.IsValid(); }

	/**
	 * Speed ratio for the slide and dash: the MoveSpeed attribute / DA_PlayerMovementSettings MaxWalkSpeed, never
	 * below 1 (1 without an ASC or settings). Read live; a slide or dash keeps the value it started with.
	 */
	float GetMoveSpeedRatio() const;

	/** The ratio the current (or last) slide / dash started with (tests / debug). */
	float GetSlideSpeedRatio() const { return SlideSpeedRatio; }
	float GetDashSpeedRatio() const { return DashSpeedRatio; }
	/** Speed the current (or last) slide started at, after its MaxSpeed cap. */
	float GetSlideStartSpeed() const { return SlideStartSpeed; }

	/** Fired when the slide/dash ends for any reason. */
	FSimpleMulticastDelegate OnSlideFinished;
	FSimpleMulticastDelegate OnDashFinished;

	//~ ACharacter
	/**
	 * Jumping during a slide ends it and keeps the momentum (slide-hop). Jumping during a dash is queued and happens
	 * when the dash ends (the dash would otherwise overwrite the jump's upward speed and waste the jump).
	 */
	virtual void Jump() override;

	// --- Extra jumps (ExtraJumps attribute -> JumpMaxCount; tests / debug) ---

	/** Air jumps taken since the player last stood on the ground. */
	int32 GetAirJumpsSinceLanded() const { return AirJumpsSinceLanded; }

	/** Upward speed the most recent air jump set. */
	float GetLastAirJumpZVelocity() const { return LastAirJumpZVelocity; }

	/** Air-jump puffs / sounds played so far (cosmetic). */
	int32 GetAirJumpFeedbackCount() const { return AirJumpFeedbackCount; }

	/** A jump was pressed during the dash and will happen when it ends. */
	bool IsJumpQueuedAfterDash() const { return bJumpQueuedAfterDash; }

	const UGSMovementSettings* GetMovementSettings() const { return MovementSettings; }

protected:
	virtual bool CanJumpInternal_Implementation() const override;
	/** Air jumps (JumpCurrentCount >= 2) set the same upward speed every time and play the air-jump feedback. */
	virtual void OnJumped_Implementation() override;
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

	/** Loadout: up to 2 weapons with per-weapon ammo. Starting weapons are set on BP_GSCharacter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TObjectPtr<UGSWeaponComponent> WeaponComponent;

	/** Shared health/death logic (listens to the PlayerState ASC's Health). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UGSHealthComponent> HealthComponent;

	/** Floating Shield visuals: mesh, material, orbit and block / return feedback are set on BP_GSCharacter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UGSFloatingShieldsComponent> FloatingShields;

	/** Floating Gun: spawns the guns for the player's FloatingGunLevel (tuning in DA_FloatingGunSettings). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UGSFloatingGunComponent> FloatingGuns;

	/** Razor Cards: throws homing cards for the player's RazorCardsLevel (tuning in DA_RazorCardsSettings). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UGSRazorCardsComponent> RazorCards;

	/** Barbed Wire: the damaging ring for the player's BarbedWireLevel (tuning in DA_BarbedWireSettings). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UGSBarbedWireComponent> BarbedWire;

	/** Dynamite: lobs sticks at the biggest group for the player's DynamiteLevel (tuning in DA_DynamiteSettings). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UGSDynamiteComponent> Dynamite;

	/** Lucky Coin: flips bouncing coins for the player's LuckyCoinLevel (tuning in DA_LuckyCoinSettings). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UGSLuckyCoinComponent> LuckyCoin;

	/** Played on the local player's camera when they take damage (CS_PlayerHit). */
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

private:
	// --- Ability system ---
	void InitAbilitySystem();
	void ClearAbilitySystem();
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void ApplyMoveSpeed(float NewSpeed);
	void OnExtraJumpsChanged(const FOnAttributeChangeData& Data);
	/** JumpMaxCount = 1 + ExtraJumps (rounded, never below 1). */
	void ApplyExtraJumps(float NewExtraJumps);

	/** Placeholder puff + sound at the feet (local cosmetic; skipped during client move replays). */
	void PlayAirJumpFeedback();

	// --- Input ---
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void OnAimStarted();
	void OnAimCompleted();
	void OnShoulderSwap();

	// --- Health / death ---
	UFUNCTION()
	void OnHealthChanged(UGSHealthComponent* InHealthComponent, float OldHealth, float NewHealth, AActor* DamageInstigator);

	UFUNCTION()
	void OnDeathStarted(AActor* DeadActor, AActor* Killer);

	// --- Movement / camera ---
	void ApplyMovementSettings();
	void UpdateFacingFromInput();
	void ApplyFacingMode();
	void UpdateCamera(float DeltaSeconds);

	UFUNCTION(Server, Reliable)
	void ServerSetAimState(bool bNewAiming, bool bNewFacingCamera);

	UFUNCTION()
	void OnRep_FacingCamera();

	/** Replicated so the server and other clients simulate the same rotation mode. */
	UPROPERTY(ReplicatedUsing = OnRep_FacingCamera)
	bool bFacingCamera = false;

	/** Replicated aim state for non-owning machines (server spread, other clients' animation later). */
	UPROPERTY(Replicated)
	bool bAimingReplicated = false;

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

	// --- Slide / dash state ---
	void TickSlide(float DeltaSeconds);
	void TickDash(float DeltaSeconds);
	void SaveAndClearFriction();
	void RestoreFriction();
	FVector GetMovementInputDirection() const;

	TWeakObjectPtr<const UGSSlideSettings> ActiveSlide;
	FVector SlideDirection = FVector::ForwardVector;
	float SlideStartSpeed = 0.f;
	float SlideElapsed = 0.f;
	/** GetMoveSpeedRatio() when the slide started; scales SpeedBoost, MaxSpeed and EndSpeed for the whole slide. */
	float SlideSpeedRatio = 1.f;
	/** GetMoveSpeedRatio() when the dash started; scales Distance and ExitSpeed for the whole dash. */
	float DashSpeedRatio = 1.f;

	TWeakObjectPtr<const UGSDashSettings> ActiveDash;
	FVector DashDirection = FVector::ForwardVector;
	float DashElapsed = 0.f;
	/** Where the dash started; distance is measured from here so hitches and walls can't skew it. */
	FVector DashStartLocation = FVector::ZeroVector;
	/** Distance moved along the dash direction as of the previous tick (to detect being blocked). */
	float DashLastTraveled = 0.f;
	/** Distance requested for the previous frame. */
	float DashLastRequested = 0.f;
	float SavedGravityScale = 1.f;

	int32 FrictionOverrideCount = 0;
	float SavedGroundFriction = 0.f;
	float SavedBrakingDecelerationWalking = 0.f;
	float SavedBrakingDecelerationFalling = 0.f;

	/** Camera height compensation while the capsule is shrunk (local only). */
	float CurrentCameraHeightOffset = 0.f;
	float TargetCameraHeightOffset = 0.f;

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystem;
	FDelegateHandle MoveSpeedChangedHandle;
	FDelegateHandle ExtraJumpsChangedHandle;

	// --- Extra jump state ---
	int32 AirJumpsSinceLanded = 0;
	float LastAirJumpZVelocity = 0.f;
	int32 AirJumpFeedbackCount = 0;
	bool bJumpQueuedAfterDash = false;

	// --- Level-up glow (cosmetic, every machine; bound to this player's UGSProgressionComponent::OnLevelUp) ---
	UFUNCTION()
	void HandleLevelUp(int32 NewLevel);
	void TickLevelUpGlow(float DeltaSeconds);

	TWeakObjectPtr<UGSProgressionComponent> CachedProgression;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LevelUpGlowMID;

	float LevelUpGlowRemaining = 0.f;

public:
	/** Level-up glow playing on the mesh (tests). */
	bool IsLevelUpGlowActive() const { return LevelUpGlowRemaining > 0.f; }

protected:
	/** Placeholder level-up flash: overlay material on the mesh (M_LevelUpGlow, scalar "Glow" faded 1 -> 0). */
	UPROPERTY(EditDefaultsOnly, Category = "Level Up")
	TObjectPtr<UMaterialInterface> LevelUpGlowMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Level Up", meta = (Units = "s"))
	float LevelUpGlowDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Level Up")
	FName LevelUpGlowParameter = TEXT("Glow");

	/** Placeholder puff spawned at the feet on each air jump (optional). */
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	TObjectPtr<UNiagaraSystem> AirJumpEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	FVector AirJumpEffectScale = FVector(0.4f);

	/** Placeholder sound played at the feet on each air jump (optional). */
	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	TObjectPtr<USoundBase> AirJumpSound;
};
