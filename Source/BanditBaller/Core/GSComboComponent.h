// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSComboComponent.generated.h"

class UGSComboSettings;

/** Why a kill combo ended. */
UENUM(BlueprintType)
enum class EGSComboEndReason : uint8
{
	/** The combo window ran out without another kill. */
	TimedOut,
	/** The player lost health (UGSComboSettings::bEndComboOnDamage). */
	DamageTaken,
	/** A new run started, or code / tests ended it. */
	Reset
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnComboChangedSignature, int32, ComboCount, float, CashMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnComboEndedSignature, int32, FinalCount, EGSComboEndReason, Reason);
DECLARE_MULTICAST_DELEGATE_TwoParams(FGSOnComboEndedNative, int32, EGSComboEndReason);

/**
 * The player's kill combo, on AGSPlayerState (run state: AGSGameMode::StartRun ends it).
 *
 * - AGSGameMode::HandleEnemyKilled calls RegisterKill for the killer: +1, the cash multiplier for the new count
 *   (UGSComboSettings::CashMultiplierSteps) and a fresh ComboWindow. The GameMode multiplies that kill's cash drop
 *   right away, so a pickup keeps the multiplier of the moment its enemy died.
 * - The window is a world timer, so it stops while the game is paused (the upgrade choice screen).
 * - It ends when the window runs out, on damage taken (HandleDamageTaken, if bEndComboOnDamage) or on EndCombo.
 *
 * Authority only. TODO(co-op): not replicated (single-player first); clients' HUDs would need the count, multiplier
 * and window end time in M6.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSComboComponent();

	//~ UActorComponent
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Count an enemy kill: +1 to the combo, multiplier for the new count, window restarted. Returns the new count
	 * (0 without authority or without ComboSettings on the GameMode).
	 */
	int32 RegisterKill();

	/** The owner lost Amount health: ends the combo when the settings say so (bEndComboOnDamage). */
	void HandleDamageTaken(float Amount);

	/** End the combo now (nothing happens without one). Authority only. */
	void EndCombo(EGSComboEndReason Reason);

	/** Kills in the current combo (0 = no combo). */
	UFUNCTION(BlueprintPure, Category = "Combo")
	int32 GetComboCount() const { return ComboCount; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool IsComboActive() const { return ComboCount > 0; }

	/** Cash multiplier of the current combo (x1 without one). */
	UFUNCTION(BlueprintPure, Category = "Combo")
	float GetCashMultiplier() const { return CashMultiplier; }

	/** Game-time seconds left in the window (0 without a combo; frozen while the game is paused). */
	UFUNCTION(BlueprintPure, Category = "Combo")
	float GetTimeRemaining() const;

	/** GetTimeRemaining / the window length, 0..1 (the HUD's draining bar). */
	UFUNCTION(BlueprintPure, Category = "Combo")
	float GetTimeRemainingFraction() const;

	/** "TimedOut", "DamageTaken", "Reset". */
	static FString GetEndReasonName(EGSComboEndReason Reason);

	// --- Events (server; single-player = the local machine) ---

	/** After every kill (new count + multiplier) and when the combo ends (0, x1). */
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FGSOnComboChangedSignature OnComboChanged;

	/** The combo ended with FinalCount kills; fired before OnComboChanged(0, 1). */
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FGSOnComboEndedSignature OnComboEnded;

	/** Native mirror of OnComboEnded for C++ listeners (tests). */
	FGSOnComboEndedNative OnComboEndedNative;

private:
	const UGSComboSettings* GetSettings() const;
	void HandleWindowExpired();

	UPROPERTY(VisibleInstanceOnly, Category = "Combo")
	int32 ComboCount = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Combo")
	float CashMultiplier = 1.f;

	/** Length of the running window (from the settings when the last kill happened). */
	float ActiveWindow = 0.f;

	FTimerHandle WindowTimer;
	bool bWarnedNoSettings = false;
};
