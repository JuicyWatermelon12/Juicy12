// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSUpgradeComponent.h"
#include "GSUpgradeChoiceWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UGSUpgradeCardWidget;

/**
 * Level-up choice screen (WBP_UpgradeChoice derives from this; AGSPlayerController adds it to the screen at start).
 *
 * Driven only by the owning player's UGSUpgradeComponent events (OnUpgradeOfferChanged / OnRerollsLeftChanged):
 * - Opened (fresh offer): the game is already paused by AGSGameMode; wait RevealDelay (real time) so the HUD's
 *   "LEVEL UP!" banner plays first, then show one card per choice (CardWidgetClass in CardsBox) with UI input.
 * - Opened while showing (next queued offer after a pick) and Rerolled: swap the cards at once.
 * - Closed: hide and give input back to the game (AGSGameMode unpauses).
 * Every time cards appear or change, input is ignored for InputLockDuration (and the cards fade in), so a held
 * fire button or mashed key can't pick by accident.
 *
 * Input: click a card, keys 1-3, R / Reroll button; gamepad D-pad or left stick to select, A to pick, Y to reroll.
 * TODO(co-op): picks and rerolls call AGSGameMode directly (single-player / listen-server host); clients need a
 * Server RPC in M6.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSUpgradeChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Pick card Index (mouse / key path). False while hidden, locked or refused. */
	bool TryChoose(int32 Index);

	/** Spend a reroll (button / key path). False while hidden, locked, at 0 rerolls or refused. */
	bool TryReroll();

	/** Keyboard / gamepad handling (NativeOnKeyDown; public for tests). True if the key was consumed. */
	bool HandleChoiceKey(const FKey& Key);

	// --- Shown state (tests) ---
	bool AreCardsShowing() const { return bCardsShowing; }
	bool IsRevealPending() const { return RevealRemaining >= 0.f; }
	bool IsInputLocked() const;
	int32 GetCardCount() const { return Cards.Num(); }
	UGSUpgradeCardWidget* GetCard(int32 Index) const { return Cards.IsValidIndex(Index) ? Cards[Index] : nullptr; }
	int32 GetSelectedIndex() const { return SelectedIndex; }
	FText GetRerollLabel() const;
	bool IsRerollEnabled() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Everything that shows with the cards (dim, title, cards, reroll); collapsed while no offer is shown. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ChoicePanel;

	/** Cards are created into this panel (a HorizontalBox). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CardsBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RerollButton;

	/** "Reroll (2 left)" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollText;

	/** "+2 more upgrades after this" (hidden when nothing is queued). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QueuedText;

	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice")
	TSubclassOf<UGSUpgradeCardWidget> CardWidgetClass;

	/** Real seconds between a fresh offer opening (game paused) and the cards appearing; the banner plays meanwhile. */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice", meta = (ClampMin = "0", Units = "s"))
	float RevealDelay = 1.f;

	/** Real seconds input is ignored after cards appear or change (the cards fade in over this time). */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice", meta = (ClampMin = "0", Units = "s"))
	float InputLockDuration = 0.3f;

	/** Keys picking card 1, 2, 3, ... by position. */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice|Input")
	TArray<FKey> CardKeys = { EKeys::One, EKeys::Two, EKeys::Three };

	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice|Input")
	TArray<FKey> CardKeysAlt = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree };

	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice|Input")
	TArray<FKey> RerollKeys = { EKeys::R, EKeys::Gamepad_FaceButton_Top };

	/** Picks the selected card (gamepad). With nothing selected yet it selects the first card. */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice|Input")
	TArray<FKey> ConfirmKeys = { EKeys::Gamepad_FaceButton_Bottom };

	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice|Input")
	TArray<FKey> SelectLeftKeys = { EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left, EKeys::Left };

	UPROPERTY(EditDefaultsOnly, Category = "Upgrade Choice|Input")
	TArray<FKey> SelectRightKeys = { EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right, EKeys::Right };

private:
	UFUNCTION()
	void HandleOfferChanged(EGSUpgradeOfferChange Change);

	UFUNCTION()
	void HandleRerollsLeftChanged(int32 RerollsLeft);

	UFUNCTION()
	void HandleRerollClicked();

	void HandleCardClicked(int32 Index);

	bool TryBindUpgrades();
	void UnbindUpgrades();

	void ShowCards();
	void HideCards();
	void RebuildCards();
	void LockInput();
	void UpdateRerollButton();
	void UpdateQueuedText();
	void SetSelectedIndex(int32 NewIndex);
	double GetRealTime() const;

	TWeakObjectPtr<UGSUpgradeComponent> BoundUpgrades;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGSUpgradeCardWidget>> Cards;

	bool bCardsShowing = false;
	/** Seconds until a pending fresh offer is revealed; < 0 when none is pending. */
	float RevealRemaining = -1.f;
	double InputUnlockTime = 0.0;
	int32 SelectedIndex = INDEX_NONE;
};
