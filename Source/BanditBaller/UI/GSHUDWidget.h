// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSHUDWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
class UGSWeaponComponent;
struct FOnAttributeChangeData;

/**
 * In-game HUD base (WBP_HUD derives from this).
 * - Health bar/text bound to Health/MaxHealth attribute change delegates (no polling of values).
 * - Weapon name + ammo.
 * - Painted overlays (no extra widgets): floating damage numbers, crosshair hit marker,
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

	/** Pool size; the oldest number is recycled when full. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage Numbers", meta = (ClampMin = "1"))
	int32 MaxDamageNumbers = 24;

private:
	struct FDamageNumber
	{
		FVector WorldLocation = FVector::ZeroVector;
		float Damage = 0.f;
		float Age = 0.f;
		float JitterX = 0.f;
		bool bCritical = false;
		bool bActive = false;
	};

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

	// Weapon text cache.
	TWeakObjectPtr<const UObject> LastWeaponData;
	int32 LastAmmo = -1;
	int32 LastMagazine = -1;
};
