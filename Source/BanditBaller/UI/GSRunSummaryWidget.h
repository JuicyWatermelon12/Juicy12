// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSRunSummaryWidget.generated.h"

class AGSGameState;
class UButton;
class UGSProfileSubsystem;
class UGSRunStats;
class UGSUpgradeComponent;
class UGSUpgradeIconWidget;
class UPanelWidget;
class UTextBlock;

/**
 * Run summary after death (WBP_RunSummary derives from this; AGSPlayerController::ClientShowRunSummary adds it when the
 * player dies and the run is in GameOver). Replaces the old "YOU DIED" screen.
 *
 * Shows the run's stats (UGSRunStats: rounds cleared, level reached, kills, best combo, cash earned, run time), the
 * upgrades picked this run with their stacks (the owning player's UGSUpgradeComponent, one UpgradeIconWidgetClass
 * icon each), the total cash (UGSProfileSubsystem) and the run seed (AGSGameState). It only reads them, refreshing on
 * UGSRunStats::OnStatsChanged and the profile's OnCashChanged.
 *
 * One action, "Return to Market": the ReturnButton, or ReturnKeys (R, gamepad Start / Y). It is ignored for
 * InputLockDuration after the screen appears (so a held reload key can't skip it) and for key repeats. It fades the
 * screen out and asks the server to go back to the hub (AGSPlayerController::ServerRequestReturnToHub ->
 * AGSGameMode::RequestReturnToHub, which fades the camera and opens the hub map).
 * TODO(co-op): reads server-side upgrades / best combo (not replicated yet) and this machine's profile.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSRunSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** "Return to Market" (button / key path). False while locked or once requested. */
	bool TryReturnToHub();

	/** Keyboard / gamepad handling (NativeOnKeyDown; public for tests). True if the key was consumed. */
	bool HandleReturnKey(const FKey& Key, bool bIsRepeat = false);

	// --- Shown state (tests) ---
	bool IsInputLocked() const;
	bool IsReturnRequested() const { return bReturnRequested; }
	/** Current opacity of the whole screen (fades in, then out after Return to Market). */
	float GetScreenOpacity() const;
	FText GetRoundsClearedText() const;
	FText GetLevelReachedText() const;
	FText GetKillsText() const;
	FText GetBestComboText() const;
	FText GetCashEarnedText() const;
	FText GetTotalCashText() const;
	FText GetRunTimeText() const;
	FText GetSeedText() const;
	/** "Damage x2, Extra Jump x1" in pick order, or "None" (weapons are listed apart). */
	FText GetUpgradesText() const;
	/** "Floating Gun, Razor Cards" in the order they were picked, or "None". */
	FText GetWeaponsText() const;
	int32 GetUpgradeIconCount() const { return UpgradeIcons.Num(); }
	UGSUpgradeIconWidget* GetUpgradeIcon(int32 Index) const { return UpgradeIcons.IsValidIndex(Index) ? UpgradeIcons[Index] : nullptr; }

	/** "4:07" (or "1:02:03" from an hour). */
	static FText FormatRunTime(float Seconds);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Everything that shows (dim + panel); faded in and out as a whole. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ScreenRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoundsClearedValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelReachedValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillsValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BestComboValue;

	/** "$540": cash earned this run (pickups + round-clear bonuses). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CashEarnedValue;

	/** "$1,880": the permanent balance. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalCashValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RunTimeValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SeedValue;

	/** Upgrade icons are created into this panel (a WrapBox or HorizontalBox). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> UpgradesBox;

	/** "None", shown instead of the icons when no upgrade was picked. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoUpgradesText;

	/** The weapons held ("Floating Gun", or "None"). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponsValue;

	/** "Return to Market" (non-focusable; keys are handled here). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReturnButton;

	/** One icon per picked upgrade (WBP_UpgradeIcon). */
	UPROPERTY(EditDefaultsOnly, Category = "Run Summary")
	TSubclassOf<UGSUpgradeIconWidget> UpgradeIconWidgetClass;

	/** Space after each upgrade icon in UpgradesBox. */
	UPROPERTY(EditDefaultsOnly, Category = "Run Summary", meta = (ClampMin = "0"))
	float UpgradeIconSpacing = 8.f;

	/** Keys that press "Return to Market" (the old death screen's restart keys). */
	UPROPERTY(EditDefaultsOnly, Category = "Run Summary|Input")
	TArray<FKey> ReturnKeys = { EKeys::R, EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Top };

	/** Real seconds after the screen appears before the button and keys work (R is also reload). */
	UPROPERTY(EditDefaultsOnly, Category = "Run Summary|Input", meta = (ClampMin = "0", Units = "s"))
	float InputLockDuration = 1.f;

	/** Real seconds the screen takes to fade in. */
	UPROPERTY(EditDefaultsOnly, Category = "Run Summary", meta = (ClampMin = "0", Units = "s"))
	float FadeInTime = 0.4f;

	/** Real seconds the screen takes to fade out after Return to Market (match AGSGameMode::ReturnToHubFadeTime). */
	UPROPERTY(EditDefaultsOnly, Category = "Run Summary", meta = (ClampMin = "0", Units = "s"))
	float FadeOutTime = 0.5f;

private:
	bool TryBind();
	void Unbind();
	void Refresh();
	void RefreshUpgrades();
	double GetRealTime() const;

	UFUNCTION()
	void HandleStatsChanged();

	UFUNCTION()
	void HandleCashChanged(int32 NewCash, int32 Delta);

	UFUNCTION()
	void HandleReturnClicked();

	const UGSUpgradeComponent* GetUpgrades() const;

	TWeakObjectPtr<AGSGameState> BoundGameState;
	TWeakObjectPtr<UGSRunStats> BoundStats;
	TWeakObjectPtr<UGSProfileSubsystem> BoundProfile;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGSUpgradeIconWidget>> UpgradeIcons;

	double InputUnlockTime = 0.0;
	float FadeInElapsed = 0.f;
	float FadeOutElapsed = 0.f;
	bool bReturnRequested = false;
};
