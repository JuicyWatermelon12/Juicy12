// BanditBaller

#include "GSRoundSummaryWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UGSRoundSummaryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	TryBind();
	Refresh();
}

void UGSRoundSummaryWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UGSRoundSummaryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The GameState can arrive after the widget is created; once bound it's event-driven.
	if (!BoundGameState.IsValid() && TryBind())
	{
		Refresh();
	}

	if (bShowing && SummaryPanel && FadeElapsed < FadeInTime)
	{
		FadeElapsed += InDeltaTime;
		SummaryPanel->SetRenderOpacity(FadeInTime > 0.f ? FMath::Clamp(FadeElapsed / FadeInTime, 0.f, 1.f) : 1.f);
	}
}

bool UGSRoundSummaryWidget::TryBind()
{
	const UWorld* World = GetWorld();
	AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
	UGSRunStats* Stats = GS ? GS->GetRunStats() : nullptr;
	if (!Stats)
	{
		return false;
	}
	if (GS == BoundGameState.Get() && Stats == BoundStats.Get())
	{
		return true;
	}
	Unbind();
	BoundGameState = GS;
	BoundStats = Stats;
	GS->OnPhaseChanged.AddUniqueDynamic(this, &UGSRoundSummaryWidget::HandlePhaseChanged);
	Stats->OnStatsChanged.AddUniqueDynamic(this, &UGSRoundSummaryWidget::HandleStatsChanged);
	return true;
}

void UGSRoundSummaryWidget::Unbind()
{
	if (AGSGameState* GS = BoundGameState.Get())
	{
		GS->OnPhaseChanged.RemoveDynamic(this, &UGSRoundSummaryWidget::HandlePhaseChanged);
	}
	if (UGSRunStats* Stats = BoundStats.Get())
	{
		Stats->OnStatsChanged.RemoveDynamic(this, &UGSRoundSummaryWidget::HandleStatsChanged);
	}
	BoundGameState.Reset();
	BoundStats.Reset();
}

void UGSRoundSummaryWidget::HandlePhaseChanged(EGSRoundPhase OldPhase, EGSRoundPhase NewPhase)
{
	Refresh();
}

void UGSRoundSummaryWidget::HandleStatsChanged()
{
	Refresh();
}

void UGSRoundSummaryWidget::Refresh()
{
	const AGSGameState* GS = BoundGameState.Get();
	const UGSRunStats* Stats = BoundStats.Get();
	const FGSRoundClearPayout* Payout = Stats ? &Stats->GetLastRoundClearPayout() : nullptr;

	// Only the RoundClear that followed this round's Combat has a payout for the current round.
	const bool bShow = GS && Payout && GS->GetRoundPhase() == EGSRoundPhase::RoundClear && Payout->Round > 0
		&& Payout->Round == GS->GetRoundNumber() && Stats->GetCurrentRound().Round == Payout->Round;
	if (bShow && !bShowing)
	{
		FadeElapsed = 0.f;
		if (SummaryPanel)
		{
			SummaryPanel->SetRenderOpacity(FadeInTime > 0.f ? 0.f : 1.f);
		}
	}
	bShowing = bShow;
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShow)
	{
		return;
	}

	ShownPayout = *Payout;
	ShownRound = Stats->GetCurrentRound();

	auto SetText = [](UTextBlock* Block, const FText& Text)
	{
		if (Block)
		{
			Block->SetText(Text);
		}
	};
	SetText(TitleText, GetTitleText());
	SetText(KillsValue, GetKillsText());
	SetText(CashCollectedValue, GetCashCollectedText());
	SetText(XPEarnedValue, GetXPEarnedText());
	SetText(BaseBonusValue, GetBaseBonusText());
	SetText(NoDamageValue, GetNoDamageBonusText());
	SetText(FastClearLabel, GetFastClearLabelText());
	SetText(FastClearValue, GetFastClearBonusText());
	SetText(TotalValue, GetTotalText());
	if (NoDamageRow)
	{
		NoDamageRow->SetRenderOpacity(ShownPayout.bNoDamage ? 1.f : MissedBonusOpacity);
	}
	if (FastClearRow)
	{
		FastClearRow->SetRenderOpacity(ShownPayout.bFastClear ? 1.f : MissedBonusOpacity);
	}
}

FText UGSRoundSummaryWidget::FormatBonus(int32 Amount, bool bEarned)
{
	return bEarned ? FText::Format(NSLOCTEXT("GSRoundSummary", "Bonus", "+${0}"), FText::AsNumber(Amount))
		: NSLOCTEXT("GSRoundSummary", "Missed", "missed");
}

FText UGSRoundSummaryWidget::GetTitleText() const
{
	return FText::Format(NSLOCTEXT("GSRoundSummary", "Title", "ROUND {0} CLEAR"), FText::AsNumber(ShownPayout.Round));
}

FText UGSRoundSummaryWidget::GetKillsText() const
{
	return FText::AsNumber(ShownRound.Kills);
}

FText UGSRoundSummaryWidget::GetCashCollectedText() const
{
	return FText::Format(NSLOCTEXT("GSRoundSummary", "Cash", "${0}"), FText::AsNumber(ShownRound.CashCollected));
}

FText UGSRoundSummaryWidget::GetXPEarnedText() const
{
	return FText::Format(NSLOCTEXT("GSRoundSummary", "XP", "{0} XP"), FText::AsNumber(ShownRound.XPEarned));
}

FText UGSRoundSummaryWidget::GetBaseBonusText() const
{
	return FormatBonus(ShownPayout.BaseBonus, true);
}

FText UGSRoundSummaryWidget::GetNoDamageBonusText() const
{
	return FormatBonus(ShownPayout.NoDamageBonus, ShownPayout.bNoDamage);
}

FText UGSRoundSummaryWidget::GetFastClearLabelText() const
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = 1;
	return FText::Format(NSLOCTEXT("GSRoundSummary", "FastClear", "Fast clear ({0} s / {1} s)"),
		FText::AsNumber(ShownPayout.ClearTime, &Options), FText::AsNumber(ShownPayout.FastClearTime, &Options));
}

FText UGSRoundSummaryWidget::GetFastClearBonusText() const
{
	return FormatBonus(ShownPayout.FastClearBonus, ShownPayout.bFastClear);
}

FText UGSRoundSummaryWidget::GetTotalText() const
{
	return FText::Format(NSLOCTEXT("GSRoundSummary", "Total", "+${0}"), FText::AsNumber(ShownPayout.GetTotal()));
}
