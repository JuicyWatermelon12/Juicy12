// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSHUDWidget.generated.h"

class AGSPlayerState;
class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
class UGSWeaponComponent;
class UGSProgressionComponent;
struct FOnAttributeChangeData;

/**
 * In-game HUD base (WBP_HUD derives from this).
 * - Health bar/text bound to Health/MaxHealth attribute change delegates (no polling of values).
 * - Weapon name + ammo.
 * - Cash counter bound to AGSPlayerState::OnCashChanged.
 * - Level label + XP bar/text bound to UGSProgressionComponent::OnXPChanged / OnLevelUp (punch on level-up).
 * - Painted overlays (no extra widgets): floating damage numbers and "+$N" cash popups, crosshair hit marker,
 *   red screen-edge flash when hit and pulse at low health.
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

	/** Cash currently shown by CashText (from AGSPlayerState::OnCashChanged). */
	int32 GetDisplayedCash() const { return DisplayedCash; }

	/** Sum of the "+$N" popups currently on screen (tests). */
	int32 GetActiveCashPopupTotal() const;

	/** Health / MaxHealth currently shown by the health bar (attribute change delegates; tests). */
	float GetDisplayedHealth() const { return CurrentHealth; }
	float GetDisplayedMaxHealth() const { return CurrentMaxHealth; }

	/** Level / XP fill currently shown (from UGSProgressionComponent events; tests). */
	int32 GetDisplayedLevel() const { return DisplayedLevel; }
	float GetDisplayedXPPercent() const { return DisplayedXPPercent; }

protected:
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

	/** "$120" (bound to AGSPlayerState::OnCashChanged). */
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

private:
	/** A floating damage number or "+$N" cash popup (shared pool). */
	struct FDamageNumber
	{
		FVector WorldLocation = FVector::ZeroVector;
		float Damage = 0.f;
		int32 Cash = 0;
		float Age = 0.f;
		float Lifetime = 1.f;
		float JitterX = 0.f;
		bool bCritical = false;
		bool bCash = false;
		bool bActive = false;
	};

	FDamageNumber& AllocateFloatingNumber();

	// Cash counter (AGSPlayerState::OnCashChanged).
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

	TWeakObjectPtr<AGSPlayerState> BoundPlayerState;
	int32 DisplayedCash = 0;
	float CashPunchRemaining = 0.f;
	/** Pool index of the newest cash popup (merge target), or INDEX_NONE. */
	int32 LastCashPopup = INDEX_NONE;

	TWeakObjectPtr<UGSProgressionComponent> BoundProgression;
	int32 DisplayedLevel = 0;
	float DisplayedXPPercent = 0.f;
	float LevelPunchRemaining = 0.f;

	// Weapon text cache.
	TWeakObjectPtr<const UObject> LastWeaponData;
	int32 LastAmmo = -1;
	int32 LastMagazine = -1;
};
