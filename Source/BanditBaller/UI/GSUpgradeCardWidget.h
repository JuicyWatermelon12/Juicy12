// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSUpgradeData.h"
#include "GSUpgradeCardWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FGSOnUpgradeCardClicked, int32 /*CardIndex*/);

/**
 * One upgrade card in the level-up choice screen (WBP_UpgradeCard derives from this): icon, name, rarity,
 * description, "Owned: N" and a key hint, framed by CardBorder in the rarity's color. Clicking CardButton reports the
 * card's index to the choice screen, which decides whether the pick is allowed (input lock).
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSUpgradeCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fill the card. Index is its position in the offer (key hint Index + 1). */
	void SetUpgrade(const UGSUpgradeData* InUpgrade, int32 OwnedStacks, int32 Index);

	/** Gamepad selection highlight. */
	void SetSelected(bool bSelected);

	FLinearColor GetRarityColor(EGSUpgradeRarity Rarity) const;

	/** Set by the choice screen. */
	FGSOnUpgradeCardClicked OnCardClicked;

	// --- Shown state (tests) ---
	const UGSUpgradeData* GetUpgrade() const { return Upgrade.Get(); }
	FText GetOwnedText() const;
	FText GetDescriptionText() const;
	FText GetRarityText() const;
	FLinearColor GetBorderColor() const;

protected:
	virtual void NativeOnInitialized() override;

	/** Frame: its brush color is the rarity color. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> CardBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** "RARE", tinted with the rarity color. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RarityText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	/** "Owned: 2" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OwnedText;

	/** "[1]" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyHintText;

	/** Frame color per rarity. */
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TMap<EGSUpgradeRarity, FLinearColor> RarityColors = {
		{ EGSUpgradeRarity::Common, FLinearColor(0.55f, 0.55f, 0.58f) },
		{ EGSUpgradeRarity::Rare, FLinearColor(0.15f, 0.45f, 1.f) },
		{ EGSUpgradeRarity::Epic, FLinearColor(0.62f, 0.25f, 0.95f) },
		{ EGSUpgradeRarity::Legendary, FLinearColor(1.f, 0.72f, 0.1f) } };

	/** Render scale of the gamepad-selected card. */
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	float SelectedScale = 1.07f;

private:
	UFUNCTION()
	void HandleButtonClicked();

	TWeakObjectPtr<const UGSUpgradeData> Upgrade;
	int32 CardIndex = INDEX_NONE;
};
