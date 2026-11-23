// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "GSGameState.h"
#include "GSRunStats.h"
#include "GSHUDWidget.generated.h"

class AGSPlayerState;
class UGSProfileSubsystem;
class UAbilitySystemComponent;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UGSWeaponComponent;
class UGSProgressionComponent;
class UGSComboComponent;
class UGSSecondWindComponent;
class UGSUpgradeComponent;
class UGSUpgradeData;
class UGSUpgradeIconWidget;
class UGSRunStats;
class UWidget;
class USoundBase;
struct FOnAttributeChangeData;
enum class EGSUpgradeOfferChange : uint8;

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
 * - Round info (RoundInfoPanel, AGSGameState::OnPhaseChanged / OnRoundChanged / OnWaveCountsChanged): "ROUND 4", the
 *   phase, the countdown before round 1 and in the breather ("Round 11 in 10...", re-armed once per second by a world
 *   timer, so it stops while the game is paused) and "7 enemies left · 12 alive" in Combat (the newest round's enemies
 *   not yet killed, and every round's living enemies).
 * - Owned upgrades (UpgradeIconsBox): one UpgradeIconWidgetClass per upgrade with its stack count, from
 *   UGSUpgradeComponent::OnUpgradeStacksChanged.
 * - Round banners (RoundBanner): big "ROUND N" when a round starts, and the small round-clear pop-up
 *   (RoundClearPopup: "Round 4 clear  +$50" with its bonus lines, UGSRunStats::OnRoundClearPaid), queued and shown one
 *   at a time (play goes on). They wait while the "LEVEL UP!" banner shows or an upgrade offer is open (the choice cards), and a
 *   level-up (or an offer opening) puts a showing round banner back at the front of the queue. A queued banner that no
 *   longer fits the round / phase is dropped.
 * - Safe area (AGSGameState::CurrentArea == MarketDistrict: the hub, OnAreaChanged): the combat elements are collapsed
 *   (crosshair, weapon + ammo, health, level + XP, upgrade icons, combo, round info, round banners). The cash counter
 *   and the slide / dash icons stay.
 * Tuning values are editable on WBP_HUD's class defaults.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Spawn a floating damage number at a world location. CritLevel 1 (critical) is bigger, gold and ends in "!"; 2 (a
	 * triple hit, Loaded Dice past 100%) is bigger still, orange-red and ends in "!!".
	 */
	void AddDamageNumber(const FVector& WorldLocation, float Damage, uint8 CritLevel);

	// --- Damage number test hooks ---
	int32 GetDamageNumbersAdded() const { return DamageNumbersAdded; }
	int32 GetCriticalDamageNumbersAdded() const { return CriticalDamageNumbersAdded; }
	float GetLastDamageNumber() const { return LastDamageNumber; }
	bool WasLastDamageNumberCritical() const { return bLastDamageNumberCritical; }
	int32 GetTripleDamageNumbersAdded() const { return TripleDamageNumbersAdded; }
	uint8 GetLastDamageNumberCritLevel() const { return LastDamageNumberCritLevel; }
	int32 GetTripleCritFontSize() const { return TripleCritFontSize; }
	FLinearColor GetTripleCritColor() const { return TripleCritColor; }
	int32 GetCritFontSize() const { return CritFontSize; }
	int32 GetDamageNumberFontSize() const { return DamageNumberFontSize; }
	FLinearColor GetCritColor() const { return CritColor; }
	FLinearColor GetDamageNumberColor() const { return DamageNumberColor; }

	/** Flash the crosshair hit marker (red on a kill). */
	void ShowHitMarker(bool bKill);

	/** Floating "+$N" where cash was collected; pickups collected in quick succession merge into one popup. */
	void AddCashPopup(const FVector& WorldLocation, int32 Amount);

	/** Cash currently shown by CashText (from UGSProfileSubsystem::OnCashChanged). */
	int32 GetDisplayedCash() const { return DisplayedCash; }

	/** The ammo line as shown ("28 / 35"); empty without AmmoText or a weapon (tests). */
	FString GetAmmoText() const;

	/** Sum of the "+$N" popups currently on screen (tests). */
	int32 GetActiveCashPopupTotal() const;

	/** Health / MaxHealth currently shown by the health bar (attribute change delegates; tests). */
	float GetDisplayedHealth() const { return CurrentHealth; }
	float GetDisplayedMaxHealth() const { return CurrentMaxHealth; }

	/** Level / XP fill currently shown (from UGSProgressionComponent events; tests). */
	int32 GetDisplayedLevel() const { return DisplayedLevel; }
	float GetDisplayedXPPercent() const { return DisplayedXPPercent; }
	/** The level label ("LV 100") and XP text ("1,535 / 3,070 XP") as shown (tests). */
	FString GetLevelLabelText() const;
	FString GetXPLabelText() const;

	/** XP is about to be (or was just) gained from something at WorldLocation, e.g. a collected XP orb. */
	void NotifyXPSource(const FVector& WorldLocation);

	/** Sum of the "+N XP" popups currently on screen (tests). */
	int32 GetActiveXPPopupTotal() const;

	/** Green "+N" where a health orb healed N (nothing for 0, e.g. a vacuumed orb at full health). */
	void AddHealthPopup(const FVector& WorldLocation, int32 Amount);

	/** Sum of the "+N" health popups currently on screen (tests). */
	int32 GetActiveHealthPopupTotal() const;

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

	// --- Round info, banners and upgrade icons (tests) ---
	FText GetRoundText() const;
	FText GetPhaseText() const;
	/** "Round 4 in 3..." (empty outside PreRound). */
	FText GetCountdownText() const;
	/** "12 enemies left" (empty outside Combat). */
	FText GetEnemiesText() const;
	bool IsCountdownShowing() const;
	bool IsEnemiesShowing() const;
	/** A round or Second Wind banner shows (not the round-clear pop-up). */
	bool IsRoundBannerShowing() const { return IsAnyBannerShowing() && !ActiveRoundBanner.bClearPopup; }
	/** Anything in the banner queue shows: a round banner, the Second Wind banner or the round-clear pop-up. */
	bool IsAnyBannerShowing() const { return RoundBannerRemaining > 0.f; }
	/** The small round-clear pop-up ("Round 4 clear  +$50" and its bonus lines) shows. */
	bool IsRoundClearPopupShowing() const { return IsAnyBannerShowing() && ActiveRoundBanner.bClearPopup; }
	/** Its round (0 when none shows), title and bonus lines. */
	int32 GetRoundClearPopupRound() const { return IsRoundClearPopupShowing() ? ActiveRoundBanner.Payout.Round : 0; }
	FText GetRoundClearPopupTitle() const;
	FText GetRoundClearPopupLines() const;
	int32 GetRoundClearPopupsShown() const { return RoundClearPopupsShown; }
	/** Pop-ups waiting in the queue (tests). */
	int32 GetQueuedRoundClearPopupCount() const;
	/** "ROUND 4" / "ROUND CLEAR" for the showing round banner (empty when none shows). */
	FText GetRoundBannerTitle() const;
	int32 GetQueuedRoundBannerCount() const { return RoundBannerQueue.Num(); }
	/** Round banners shown so far (each show counts, a replay after a level-up too). */
	int32 GetRoundBannersShown() const { return RoundBannersShown; }

	// --- Second Wind feedback (tests) ---
	bool IsSecondWindFlashShowing() const { return SecondWindFlashRemaining > 0.f; }
	float GetSecondWindFlashAlpha() const;
	int32 GetSecondWindSoundsPlayed() const { return SecondWindSoundsPlayed; }
	int32 GetSecondWindBannersShown() const { return SecondWindBannersShown; }
	bool IsSecondWindBannerShowing() const { return IsAnyBannerShowing() && ActiveRoundBanner.bSecondWind; }
	FText GetRoundBannerSubtitle() const;
	int32 GetUpgradeIconCount() const { return UpgradeIcons.Num(); }
	/** Weapon slots shown (MaxWeapons) and the weapon in each (nullptr = empty), in pick order. */
	int32 GetWeaponSlotCount() const { return WeaponSlots.Num(); }
	const UGSUpgradeData* GetWeaponInSlot(int32 Index) const;
	const UGSUpgradeIconWidget* GetWeaponSlot(int32 Index) const { return WeaponSlots.IsValidIndex(Index) ? WeaponSlots[Index].Get() : nullptr; }
	const UGSUpgradeIconWidget* FindUpgradeIcon(const UGSUpgradeData* Upgrade) const;

	// --- Safe area (tests) ---
	/** The party is in the Market District (the hub): combat elements are hidden. */
	bool IsInSafeArea() const;
	/** Every combat element (crosshair, weapon, ammo, health, level, XP, upgrades, combo, round info, banners) is hidden. */
	bool AreCombatElementsHidden() const;
	/** The cash counter is on screen. */
	bool IsCashShown() const;

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

	/** XP towards the next level (levels are endless). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> XPBar;

	/** "135 / 156 XP". */
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

	/** Round info container (top center); collapsed before the first round. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RoundInfoPanel;

	/** "ROUND 4" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoundText;

	/** "PRE-ROUND" / "COMBAT" / "ROUND CLEAR" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhaseText;

	/** "Round 4 in 3..." (PreRound only). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;

	/** "12 enemies left" (Combat only). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EnemiesText;

	/** Big round banner container (same spot as the level-up banner); collapsed until one shows. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RoundBanner;

	/** "ROUND 4" / "ROUND CLEAR" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoundBannerTitle;

	/** "GET READY" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoundBannerSubtitle;

	/** The small round-clear pop-up (under the round info); collapsed until one shows. Play goes on meanwhile. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RoundClearPopup;

	/** "Round 4 clear  +$50" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoundClearPopupTitle;

	/** "Round clear +$20 · No damage +$15 · Fast clear missed" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoundClearPopupLines;

	/** Owned upgrade icons are created into this panel (a HorizontalBox). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> UpgradeIconsBox;

	/** The weapon slots (MaxWeapons of them, a HorizontalBox next to the upgrade icons): icon only, empty until a weapon is picked. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> WeaponSlotsBox;

	/** The crosshair (WBP_Crosshair); hidden in the safe area. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Crosshair;

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

	/** A triple hit's number (Loaded Dice past 100%). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers")
	int32 TripleCritFontSize = 36;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers")
	FLinearColor TripleCritColor = FLinearColor(1.f, 0.35f, 0.08f);

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

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health Orb", meta = (Units = "s"))
	float HealthPopupLifetime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health Orb")
	int32 HealthPopupFontSize = 22;

	/** Brighter, bluer green than the cash popup, so "+15" doesn't read as cash. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health Orb")
	FLinearColor HealthPopupColor = FLinearColor(0.15f, 1.f, 0.45f);

	/** Popups start this far above the collection point (cm); between the cash and XP popups. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health Orb")
	float HealthPopupHeight = 95.f;

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

	/** How long a round banner stays (including fades). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Round", meta = (Units = "s"))
	float RoundBannerDuration = 1.8f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Round", meta = (Units = "s"))
	float RoundBannerFadeIn = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Round", meta = (Units = "s"))
	float RoundBannerFadeOut = 0.4f;

	/** The banner starts at this scale and settles to 1 over the fade-in. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Round", meta = (ClampMin = "1"))
	float RoundBannerPopScale = 1.3f;

	/** Title color of "ROUND N". */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Round")
	FLinearColor RoundBannerColor = FLinearColor(1.f, 1.f, 1.f);

	/** Title color of the round-clear pop-up. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Round")
	FLinearColor RoundClearBannerColor = FLinearColor(0.45f, 1.f, 0.35f);

	/** How long the round-clear pop-up stays (including fades); it waits in the banner queue like a banner. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Round", meta = (Units = "s"))
	float RoundClearPopupDuration = 3.f;

	/** "SECOND WIND" banner color (shown in the round banner's spot, at once, ahead of any round banner). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Second Wind")
	FLinearColor SecondWindBannerColor = FLinearColor(1.f, 0.7f, 0.15f);

	/** Full-screen flash when a Second Wind saves the player: fades from SecondWindFlashMaxAlpha to 0. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Second Wind", meta = (Units = "s"))
	float SecondWindFlashDuration = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Second Wind", meta = (ClampMin = "0", ClampMax = "1"))
	float SecondWindFlashMaxAlpha = 0.85f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Second Wind")
	FLinearColor SecondWindFlashColor = FLinearColor(1.f, 0.85f, 0.4f);

	/** 2D sound when a Second Wind saves the player. Placeholder on WBP_HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Second Wind")
	TObjectPtr<USoundBase> SecondWindSound;

	/** One per owned upgrade in UpgradeIconsBox (WBP_UpgradeIcon). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Upgrades")
	TSubclassOf<UGSUpgradeIconWidget> UpgradeIconWidgetClass;

	/** Gap between upgrade icons (px). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Upgrades")
	float UpgradeIconSpacing = 6.f;

private:
	/** A floating damage number or "+$N" cash popup (shared pool). */
	struct FDamageNumber
	{
		FVector WorldLocation = FVector::ZeroVector;
		float Damage = 0.f;
		int32 Cash = 0;
		int32 XP = 0;
		int32 Heal = 0;
		float Age = 0.f;
		float Lifetime = 1.f;
		float JitterX = 0.f;
		/** 0 normal, 1 critical, 2 triple. */
		uint8 CritLevel = 0;
		bool bCash = false;
		bool bXP = false;
		bool bHeal = false;
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

	// Round info (AGSGameState).
	bool TryBindGameState();
	void UnbindGameState();
	void RefreshRoundInfo();
	/** Show the PreRound countdown and arm a world timer for the moment the shown second changes. */
	void UpdateCountdown();

	UFUNCTION()
	void HandlePhaseChanged(EGSRoundPhase OldPhase, EGSRoundPhase NewPhase);

	UFUNCTION()
	void HandleRoundChanged(int32 NewRound);

	UFUNCTION()
	void HandleWaveCountsChanged(int32 Total, int32 Remaining, int32 Alive);

	UFUNCTION()
	void HandleAreaChanged(EGSArea OldArea, EGSArea NewArea);

	/** Collapse the always-on combat elements in the safe area, restore them elsewhere. */
	void RefreshAreaVisibility();
	TArray<UWidget*> GetStaticCombatWidgets() const;

	// Owned upgrade icons + open offers (UGSUpgradeComponent on the PlayerState).
	bool TryBindUpgrades();
	void UnbindUpgrades();
	void RebuildUpgradeIcons();
	void SetUpgradeIcon(const UGSUpgradeData* Upgrade, int32 Stacks);
	void RefreshWeaponSlots();
	void HandleUpgradeOfferChanged(EGSUpgradeOfferChange Change);

	UFUNCTION()
	void HandleUpgradeStacksChanged(const UGSUpgradeData* Upgrade, int32 NewStacks);

	// Round banners.
	struct FRoundBannerRequest
	{
		int32 Round = 0;
		/** "SECOND WIND / N left" instead of a round banner (never queued: shown at once). */
		bool bSecondWind = false;
		int32 SecondWindsLeft = 0;
		/** The small round-clear pop-up for Payout.Round (in its own panel, RoundClearPopup), not the big banner. */
		bool bClearPopup = false;
		FGSRoundClearPayout Payout;
		bool operator==(const FRoundBannerRequest& Other) const
		{
			return Round == Other.Round && bSecondWind == Other.bSecondWind && bClearPopup == Other.bClearPopup
				&& Payout.Round == Other.Payout.Round;
		}
	};
	/** Show Request in the banner spot now (title, subtitle, color, fade-in). */
	void ShowBanner(const FRoundBannerRequest& Request);

	// Second Wind (UGSSecondWindComponent::OnSecondWindUsed).
	bool TryBindSecondWind();
	void UnbindSecondWind();
	void HandleSecondWindUsed(int32 SecondWindsLeft);
	void PaintSecondWindFlash(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	TWeakObjectPtr<UGSSecondWindComponent> BoundSecondWind;
	FDelegateHandle SecondWindUsedHandle;
	float SecondWindFlashRemaining = 0.f;
	int32 SecondWindSoundsPlayed = 0;
	int32 SecondWindBannersShown = 0;
	/** "ROUND N" when round N starts (dropped from the queue once a newer round has started). */
	void QueueRoundBanner(int32 Round);
	/** The round-clear pop-up (UGSRunStats::OnRoundClearPaid). */
	void HandleRoundClearPaid(const FGSRoundClearPayout& Payout);
	TWeakObjectPtr<UGSRunStats> BoundRunStats;
	FDelegateHandle RoundClearPaidHandle;
	int32 RoundClearPopupsShown = 0;
	/** Length of what the banner spot shows now (a banner or the pop-up). */
	float ActiveBannerDuration = 0.f;
	/** The widget showing it (RoundBanner or RoundClearPopup). */
	UWidget* GetActiveBannerWidget() const;
	bool CanShowRoundBanner() const;
	bool IsRoundBannerValid(const FRoundBannerRequest& Request) const;
	void TryShowNextRoundBanner();
	/** A level-up or an offer takes the banner spot: hide the showing round banner and queue it again first. */
	void InterruptRoundBanner();
	void TickRoundBanner(float DeltaTime);

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

	// Damage number test hooks.
	int32 DamageNumbersAdded = 0;
	int32 CriticalDamageNumbersAdded = 0;
	int32 TripleDamageNumbersAdded = 0;
	uint8 LastDamageNumberCritLevel = 0;
	float LastDamageNumber = 0.f;
	bool bLastDamageNumberCritical = false;

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

	TWeakObjectPtr<AGSGameState> BoundGameState;
	FTimerHandle CountdownTimer;
	/** Whole seconds shown by the countdown; < 0 outside PreRound. */
	int32 DisplayedCountdown = -1;

	TWeakObjectPtr<UGSUpgradeComponent> BoundUpgrades;
	FDelegateHandle OfferChangedHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGSUpgradeIconWidget>> UpgradeIcons;

	/** One per weapon slot in WeaponSlotsBox (WBP_UpgradeIcon in weapon-slot mode). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGSUpgradeIconWidget>> WeaponSlots;

	/** Designer visibility of the always-on combat elements (restored when leaving the safe area). */
	TMap<TWeakObjectPtr<UWidget>, ESlateVisibility> CombatWidgetVisibility;

	TArray<FRoundBannerRequest> RoundBannerQueue;
	FRoundBannerRequest ActiveRoundBanner;
	float RoundBannerRemaining = 0.f;
	int32 RoundBannersShown = 0;

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
