// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSHUDWidget.generated.h"

class AGSPlayerState;
class UGSProfileSubsystem;
class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
class UGSWeaponComponent;
class UGSProgressionComponent;
class UGSComboComponent;
class USoundBase;
struct FOnAttributeChangeData;

/**
 * In-game HUD base (WBP_HUD derives from this).
 * - Health bar/text bound to Health/MaxHealth attribute change delegates (no polling of values).
 * - Weapon name + ammo.
 * - Cash counter bound to UGSProfileSubsystem::OnCashChanged (the permanent balance).
 * - Level label + XP bar/text bound to UGSProgressionComponent::OnXPChanged / OnLevelUp.
 * - Level-up feedback (OnLevelUp): label punch, XP bar pulse, "LEVEL UP!" banner (LevelUpBanner: new level + rewards
 *   from UGSLevelRewards::DescribeRewards; level-ups while it shows merge into it) and LevelUpSound.
 * - Painted overlays (no extra widgets): floating damage numbers, "+$N" cash popups and "+N XP" popups, crosshair
 *   hit marker, red screen-edge flash when hit and pulse at low health.
 * - "+N XP": the amount comes from OnXPChanged; it is placed where the XP orb was collected (NotifyXPSource) when the
 *   two arrive within XPPopupPairWindow of each other (either order), otherwise above the player (gs.XP.Add).
 * - Kill combo (ComboPanel): count + cash multiplier from UGSComboComponent::OnComboChanged, shown from
 *   ComboMinCountToShow kills; the timer bar drains from the combo's remaining window each tick (frozen while paused).
 * Tuning values are editable on WBP_HUD's class defaults.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Spawn a floating damage number at a world location (crits are stubbed for later). */
	void AddDamageNumber(const FVector& WorldLocation, float Damage, bool bCritical);

	/** Flash the crosshair hit marker (red on a kill). */
	void ShowHitMarker(bool bKill);

	/** Floating "+$N" where cash was collected; pickups collected in quick succession merge into one popup. */
	void AddCashPopup(const FVector& WorldLocation, int32 Amount);

	/** Cash currently shown by CashText (from UGSProfileSubsystem::OnCashChanged). */
	int32 GetDisplayedCash() const { return DisplayedCash; }

	/** Sum of the "+$N" popups currently on screen (tests). */
	int32 GetActiveCashPopupTotal() const;

	/** Health / MaxHealth currently shown by the health bar (attribute change delegates; tests). */
	float GetDisplayedHealth() const { return CurrentHealth; }
	float GetDisplayedMaxHealth() const { return CurrentMaxHealth; }

	/** Level / XP fill currently shown (from UGSProgressionComponent events; tests). */
	int32 GetDisplayedLevel() const { return DisplayedLevel; }
	float GetDisplayedXPPercent() const { return DisplayedXPPercent; }

	/** XP is about to be (or was just) gained from something at WorldLocation, e.g. a collected XP orb. */
	void NotifyXPSource(const FVector& WorldLocation);

	/** Sum of the "+N XP" popups currently on screen (tests). */
	int32 GetActiveXPPopupTotal() const;

	/** Where the last "+N XP" popup was anchored, before its height offset (tests). */
	FVector GetLastXPPopupAnchor() const { return LastXPPopupAnchor; }

	// --- Level-up feedback state (tests) ---
	bool IsLevelUpBannerShowing() const { return LevelUpBannerRemaining > 0.f; }
	int32 GetLevelUpBannerLevel() const { return BannerLastLevel; }
	FText GetLevelUpRewardText() const;
	bool IsXPBarPulsing() const { return XPBarPulseRemaining > 0.f; }
	int32 GetLevelUpSoundsPlayed() const { return LevelUpSoundsPlayed; }

	// --- Kill combo display (tests) ---
	bool IsComboShowing() const;
	int32 GetComboMinCountToShow() const { return FMath::Max(ComboMinCountToShow, 1); }
	int32 GetDisplayedComboCount() const { return DisplayedComboCount; }
	float GetDisplayedComboMultiplier() const { return DisplayedComboMultiplier; }
	float GetDisplayedComboTimerPercent() const { return DisplayedComboTimerPercent; }
	FText GetComboCountText() const;
	FText GetComboMultiplierText() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// --- Bound widgets (all optional) ---

	/** "12 / 30" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	/** "80 / 100" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	/** "$120": the permanent balance (UGSProfileSubsystem::OnCashChanged). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CashText;

	/** "LV 3" (UGSProgressionComponent::OnLevelUp / OnXPChanged). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	/** XP towards the next level (full at max level). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> XPBar;

	/** "135 / 156 XP", or "MAX" at max level. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> XPText;

	/** "LEVEL UP!" banner container; collapsed until a level-up, then fades in/out. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LevelUpBanner;

	/** "Level 3" inside the banner. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelUpLevelText;

	/** "+10 Max Health" inside the banner (UGSLevelRewards::DescribeRewards). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelUpRewardText;

	/** Kill combo container; collapsed while the combo is below ComboMinCountToShow. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ComboPanel;

	/** "5 COMBO" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ComboCountText;

	/** "x1.25 CASH" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ComboMultiplierText;

	/** Time left in the combo window (drains; frozen while the game is paused). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ComboTimerBar;

	// --- Tuning ---

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Hit Marker", meta = (Units = "s"))
	float HitMarkerDuration = 0.18f;

	/** Length of each hit marker stroke (px). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Hit Marker")
	float HitMarkerSize = 9.f;

	/** Gap from the screen center to the start of each stroke (px). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Hit Marker")
	float HitMarkerGap = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Flash", meta = (Units = "s"))
	float DamageFlashDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Flash", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageFlashMaxAlpha = 0.5f;

	/** Health fraction below which the screen edge pulses red. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Low Health", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHealthThreshold = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Low Health", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHealthMaxAlpha = 0.35f;

	/** Pulses per second at low health. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Low Health")
	float LowHealthPulseRate = 1.5f;

	/** Thickness of the red screen-edge gradient (px). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Flash")
	float EdgeThickness = 140.f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers", meta = (Units = "s"))
	float DamageNumberLifetime = 0.9f;

	/** How far numbers float up over their lifetime (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers")
	float DamageNumberRise = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers")
	int32 DamageNumberFontSize = 20;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers")
	int32 CritFontSize = 28;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers")
	FLinearColor DamageNumberColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers")
	FLinearColor CritColor = FLinearColor(1.f, 0.8f, 0.1f);

	/** Pool size (shared with cash popups); the oldest number is recycled when full. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers", meta = (ClampMin = "1"))
	int32 MaxDamageNumbers = 24;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Cash", meta = (Units = "s"))
	float CashPopupLifetime = 1.1f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Cash")
	int32 CashPopupFontSize = 24;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Cash")
	FLinearColor CashPopupColor = FLinearColor(0.45f, 1.f, 0.35f);

	/** Pickups collected within this time of the last popup add to it instead of stacking new ones. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Cash", meta = (Units = "s"))
	float CashPopupMergeWindow = 0.35f;

	/** Popups start this far above the collection point (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Cash")
	float CashPopupHeight = 70.f;

	/** CashText briefly scales up by this much when cash is gained. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Cash", meta = (ClampMin = "1"))
	float CashPunchScale = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Cash", meta = (Units = "s"))
	float CashPunchDuration = 0.2f;

	/** LevelText briefly scales up by this much on a level-up. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (ClampMin = "1"))
	float LevelUpPunchScale = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (Units = "s"))
	float LevelUpPunchDuration = 0.4f;

	/** How long the "LEVEL UP!" banner stays (including fades); restarted by further level-ups. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (Units = "s"))
	float LevelUpBannerDuration = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (Units = "s"))
	float LevelUpBannerFadeIn = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (Units = "s"))
	float LevelUpBannerFadeOut = 0.5f;

	/** The banner starts at this scale and settles to 1 over the fade-in. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (ClampMin = "1"))
	float LevelUpBannerPopScale = 1.3f;

	/** XP bar pulse on level-up: height scale at the peak and the fill color it flashes to. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (Units = "s"))
	float XPBarPulseDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level", meta = (ClampMin = "1"))
	float XPBarPulseScale = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level")
	FLinearColor XPBarPulseColor = FLinearColor(1.f, 0.95f, 0.6f);

	/** 2D sound on level-up (once per frame, even for several level-ups). Placeholder on WBP_HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Level")
	TObjectPtr<USoundBase> LevelUpSound;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|XP", meta = (Units = "s"))
	float XPPopupLifetime = 1.0f;

	/** Smaller than the cash popup. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|XP")
	int32 XPPopupFontSize = 17;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|XP")
	FLinearColor XPPopupColor = FLinearColor(0.45f, 0.75f, 1.f);

	/** Popups start this far above the kill point / player (cm); above the cash popup height. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|XP")
	float XPPopupHeight = 120.f;

	/** An XP gain and a kill notification this close in time belong together (either order). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|XP", meta = (Units = "s"))
	float XPPopupPairWindow = 0.25f;

	/** The combo panel appears from this many kills (a single kill isn't shown as a combo). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Combo", meta = (ClampMin = "1"))
	int32 ComboMinCountToShow = 2;

	/** ComboCountText briefly scales up by this much on every combo kill. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Combo", meta = (ClampMin = "1"))
	float ComboPunchScale = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Combo", meta = (Units = "s"))
	float ComboPunchDuration = 0.2f;

	/** ComboMultiplierText scales up by this much when the multiplier reaches a new step. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Combo", meta = (ClampMin = "1"))
	float ComboStepPunchScale = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Combo", meta = (Units = "s"))
	float ComboStepPunchDuration = 0.35f;

	/** Multiplier text color at x1, and once the combo multiplies cash. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Combo")
	FLinearColor ComboBaseMultiplierColor = FLinearColor(0.85f, 0.85f, 0.85f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Combo")
	FLinearColor ComboBonusMultiplierColor = FLinearColor(1.f, 0.8f, 0.15f);

private:
	/** A floating damage number or "+$N" cash popup (shared pool). */
	struct FDamageNumber
	{
		FVector WorldLocation = FVector::ZeroVector;
		float Damage = 0.f;
		int32 Cash = 0;
		int32 XP = 0;
		float Age = 0.f;
		float Lifetime = 1.f;
		float JitterX = 0.f;
		bool bCritical = false;
		bool bCash = false;
		bool bXP = false;
		bool bActive = false;
	};

	FDamageNumber& AllocateFloatingNumber();

	// Cash counter (UGSProfileSubsystem::OnCashChanged).
	bool TryBindCash();
	void UnbindCash();
	void RefreshCashDisplay();

	UFUNCTION()
	void HandleCashChanged(int32 NewCash, int32 Delta);

	// Level + XP (UGSProgressionComponent on the PlayerState).
	bool TryBindProgression();
	void UnbindProgression();
	void RefreshProgressionDisplay();

	UFUNCTION()
	void HandleXPChanged(int32 CurrentXP, int32 XPForNextLevel, int32 Delta);

	UFUNCTION()
	void HandleLevelUp(int32 NewLevel);

	// Kill combo (UGSComboComponent on the PlayerState).
	bool TryBindCombo();
	void UnbindCombo();
	void RefreshComboDisplay();
	void TickCombo(float DeltaTime);

	UFUNCTION()
	void HandleComboChanged(int32 ComboCount, float CashMultiplier);

	// Ability system binding (health).
	bool TryBindAbilitySystem();
	void UnbindAbilitySystem();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
	void RefreshHealthDisplay();

	void UpdateWeaponDisplay();
	UGSWeaponComponent* FindWeaponComponent() const;

	void PaintEdgeGlow(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, float Alpha) const;
	void PaintHitMarker(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	void PaintDamageNumbers(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	float CurrentHealth = 0.f;
	float CurrentMaxHealth = 0.f;

	float HitMarkerRemaining = 0.f;
	bool bHitMarkerKill = false;
	float DamageFlashRemaining = 0.f;
	float PulseTime = 0.f;

	/** Fixed-size pool, reused round-robin. */
	TArray<FDamageNumber> DamageNumbers;
	int32 NextDamageNumber = 0;

	TWeakObjectPtr<UGSProfileSubsystem> BoundProfile;
	int32 DisplayedCash = 0;
	float CashPunchRemaining = 0.f;
	/** Pool index of the newest cash popup (merge target), or INDEX_NONE. */
	int32 LastCashPopup = INDEX_NONE;

	TWeakObjectPtr<UGSProgressionComponent> BoundProgression;
	int32 DisplayedLevel = 0;
	float DisplayedXPPercent = 0.f;
	float LevelPunchRemaining = 0.f;

	TWeakObjectPtr<UGSComboComponent> BoundCombo;
	int32 DisplayedComboCount = 0;
	float DisplayedComboMultiplier = 1.f;
	float DisplayedComboTimerPercent = 0.f;
	float ComboPunchRemaining = 0.f;
	float ComboStepPunchRemaining = 0.f;

	// Level-up feedback.
	void ShowLevelUpBanner(int32 NewLevel);
	void TickLevelUpFeedback(float DeltaTime);
	float LevelUpBannerRemaining = 0.f;
	int32 BannerFirstLevel = 0;
	int32 BannerLastLevel = 0;
	float XPBarPulseRemaining = 0.f;
	FLinearColor XPBarBaseFillColor = FLinearColor::White;
	uint64 LastLevelUpSoundFrame = 0;
	int32 LevelUpSoundsPlayed = 0;

	// "+N XP" popups: pair XP gains (OnXPChanged) with where the XP came from (NotifyXPSource).
	void AddXPPopup(const FVector& WorldLocation, int32 Amount);
	void TickXPPopupPairing(float DeltaTime);
	struct FXPSource
	{
		FVector Location = FVector::ZeroVector;
		float Age = 0.f;
	};
	TArray<FXPSource> RecentXPSources;
	FVector LastXPPopupAnchor = FVector::ZeroVector;
	int32 PendingXP = 0;
	float PendingXPAge = 0.f;

	// Weapon text cache.
	TWeakObjectPtr<const UObject> LastWeaponData;
	int32 LastAmmo = -1;
	int32 LastMagazine = -1;
};
