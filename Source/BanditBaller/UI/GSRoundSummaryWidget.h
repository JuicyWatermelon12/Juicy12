// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSGameState.h"
#include "GSRunStats.h"
#include "GSRoundSummaryWidget.generated.h"

class UTextBlock;

/**
 * RoundClear summary (WBP_RoundSummary derives from this; AGSPlayerController adds it to the screen at start).
 *
 * Shown while the round is in RoundClear after its Combat, i.e. once AGSGameMode has paid the round-clear bonus
 * (UGSRunStats::GetLastRoundClearPayout is for the current round). Lines: kills, cash collected and XP earned this round
 * (FGSRoundStats; they keep counting while RoundClear's vacuum pays out), the base / no-damage / fast-clear bonus
 * (missed bonuses dimmed) and the bonus total that was paid into the profile.
 * Driven by AGSGameState::OnPhaseChanged and UGSRunStats::OnStatsChanged; it only reads them. Its tick only retries
 * binding and fades the panel in.
 * TODO(co-op): reads server-side run stats (round stats and payout aren't replicated yet).
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSRoundSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Shown state (tests) ---
	bool IsSummaryShowing() const { return bShowing; }
	int32 GetShownRound() const { return bShowing ? ShownPayout.Round : 0; }
	FText GetTitleText() const;
	FText GetKillsText() const;
	FText GetCashCollectedText() const;
	FText GetXPEarnedText() const;
	FText GetBaseBonusText() const;
	FText GetNoDamageBonusText() const;
	FText GetFastClearLabelText() const;
	FText GetFastClearBonusText() const;
	FText GetTotalText() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Everything that shows; collapsed outside RoundClear. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SummaryPanel;

	/** "ROUND 3 CLEAR" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillsValue;

	/** "$140" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CashCollectedValue;

	/** "150 XP" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> XPEarnedValue;

	/** "+$20" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BaseBonusValue;

	/** The no-damage line (dimmed when missed). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NoDamageRow;

	/** "+$15" or "missed" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoDamageValue;

	/** The fast-clear line (dimmed when missed). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> FastClearRow;

	/** "Fast clear (9.4 s / 14 s)" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FastClearLabel;

	/** "+$15" or "missed" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FastClearValue;

	/** "+$50": the bonus paid into the profile. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalValue;

	/** Real seconds the panel takes to fade in. */
	UPROPERTY(EditDefaultsOnly, Category = "Round Summary", meta = (ClampMin = "0", Units = "s"))
	float FadeInTime = 0.25f;

	/** Opacity of a bonus line that wasn't earned. */
	UPROPERTY(EditDefaultsOnly, Category = "Round Summary", meta = (ClampMin = "0", ClampMax = "1"))
	float MissedBonusOpacity = 0.4f;

private:
	bool TryBind();
	void Unbind();
	void Refresh();

	UFUNCTION()
	void HandlePhaseChanged(EGSRoundPhase OldPhase, EGSRoundPhase NewPhase);

	UFUNCTION()
	void HandleStatsChanged();

	static FText FormatBonus(int32 Amount, bool bEarned);

	TWeakObjectPtr<AGSGameState> BoundGameState;
	TWeakObjectPtr<UGSRunStats> BoundStats;

	bool bShowing = false;
	float FadeElapsed = 0.f;
	FGSRoundStats ShownRound;
	FGSRoundClearPayout ShownPayout;
};
