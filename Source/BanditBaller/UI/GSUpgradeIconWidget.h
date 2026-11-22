// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSUpgradeData.h"
#include "GSUpgradeIconWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

/**
 * One owned level-up upgrade on the HUD (WBP_UpgradeIcon derives from this): icon in a rarity-colored frame, short name
 * and stack count. UGSHUDWidget creates one per upgrade in its UpgradeIconsBox and calls SetUpgrade when the stacks
 * change.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSUpgradeIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** BestRarity tints the frame (the rarest pick of the upgrade so far). */
	void SetUpgrade(const UGSUpgradeData* InUpgrade, int32 InStacks, EGSUpgradeRarity BestRarity = EGSUpgradeRarity::Common);

	/**
	 * Use this widget as a HUD weapon slot: icon only (no count, no name, a neutral frame). Weapon nullptr = an empty
	 * slot (no icon).
	 */
	void SetWeaponSlot(const UGSUpgradeData* Weapon);

	bool IsWeaponSlot() const { return bWeaponSlot; }
	bool IsIconShown() const;

	const UGSUpgradeData* GetUpgrade() const { return Upgrade; }
	int32 GetStacks() const { return Stacks; }

	/** "x3" */
	FText GetStackText() const;

protected:
	/** Frame tinted with the rarity color. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> IconBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	/** Stack count ("x3"). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StackText;

	/** The upgrade's display name, small (the placeholder icons all look alike). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** Frame color per rarity (same defaults as the upgrade cards). */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Icon")
	TMap<EGSUpgradeRarity, FLinearColor> RarityColors = {
		{ EGSUpgradeRarity::Common, FLinearColor(0.55f, 0.55f, 0.58f) },
		{ EGSUpgradeRarity::Rare, FLinearColor(0.15f, 0.45f, 1.f) },
		{ EGSUpgradeRarity::Epic, FLinearColor(0.62f, 0.25f, 0.95f) },
		{ EGSUpgradeRarity::Legendary, FLinearColor(1.f, 0.72f, 0.1f) } };

	/** Frame color of a weapon slot holding a weapon (no rarity colors on weapon slots). */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Icon")
	FLinearColor WeaponSlotColor = FLinearColor(0.75f, 0.75f, 0.78f);

	/** Frame color of an empty weapon slot. */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Icon")
	FLinearColor EmptyWeaponSlotColor = FLinearColor(0.25f, 0.25f, 0.27f, 0.8f);

private:
	UPROPERTY(Transient)
	TObjectPtr<const UGSUpgradeData> Upgrade;

	int32 Stacks = 0;
	bool bWeaponSlot = false;
};
