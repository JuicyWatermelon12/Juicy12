// BanditBaller

#include "GSRunSummaryWidget.h"
#include "Components/Button.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/World.h"
#include "GSGameState.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSProfileSubsystem.h"
#include "GSRunStats.h"
#include "GSUpgradeComponent.h"
#include "GSUpgradeData.h"
#include "GSUpgradeIconWidget.h"

void UGSRunSummaryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (ReturnButton)
	{
		ReturnButton->OnClicked.AddUniqueDynamic(this, &UGSRunSummaryWidget::HandleReturnClicked);
	}
}

void UGSRunSummaryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InputUnlockTime = GetRealTime() + InputLockDuration;
	FadeInElapsed = 0.f;
	if (ScreenRoot)
	{
		ScreenRoot->SetRenderOpacity(FadeInTime > 0.f ? 0.f : 1.f);
	}
	TryBind();
	Refresh();
	SetKeyboardFocus();
}

void UGSRunSummaryWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UGSRunSummaryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The GameState / profile can arrive after the widget is created; once bound it's event-driven.
	if ((!BoundStats.IsValid() || !BoundProfile.IsValid()) && TryBind())
	{
		Refresh();
	}

	if (!ScreenRoot)
	{
		return;
	}
	if (bReturnRequested)
	{
		FadeOutElapsed += InDeltaTime;
		const float Alpha = FadeOutTime > 0.f ? FMath::Clamp(FadeOutElapsed / FadeOutTime, 0.f, 1.f) : 1.f;
		ScreenRoot->SetRenderOpacity(1.f - Alpha);
	}
	else if (FadeInElapsed < FadeInTime)
	{
		FadeInElapsed += InDeltaTime;
		ScreenRoot->SetRenderOpacity(FadeInTime > 0.f ? FMath::Clamp(FadeInElapsed / FadeInTime, 0.f, 1.f) : 1.f);
	}
}

double UGSRunSummaryWidget::GetRealTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : FPlatformTime::Seconds();
}

bool UGSRunSummaryWidget::IsInputLocked() const
{
	return GetRealTime() < InputUnlockTime;
}

float UGSRunSummaryWidget::GetScreenOpacity() const
{
	return ScreenRoot ? ScreenRoot->GetRenderOpacity() : GetRenderOpacity();
}

// ---------------------------------------------------------------------------
// Binding + refresh
// ---------------------------------------------------------------------------

bool UGSRunSummaryWidget::TryBind()
{
	const UWorld* World = GetWorld();
	AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
	UGSRunStats* Stats = GS ? GS->GetRunStats() : nullptr;
	UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(this);
	if (Stats && Stats != BoundStats.Get())
	{
		if (UGSRunStats* Old = BoundStats.Get())
		{
			Old->OnStatsChanged.RemoveDynamic(this, &UGSRunSummaryWidget::HandleStatsChanged);
		}
		BoundGameState = GS;
		BoundStats = Stats;
		Stats->OnStatsChanged.AddUniqueDynamic(this, &UGSRunSummaryWidget::HandleStatsChanged);
	}
	if (Profile && Profile != BoundProfile.Get())
	{
		if (UGSProfileSubsystem* Old = BoundProfile.Get())
		{
			Old->OnCashChanged.RemoveDynamic(this, &UGSRunSummaryWidget::HandleCashChanged);
		}
		BoundProfile = Profile;
		Profile->OnCashChanged.AddUniqueDynamic(this, &UGSRunSummaryWidget::HandleCashChanged);
	}
	return BoundStats.IsValid() && BoundProfile.IsValid();
}

void UGSRunSummaryWidget::Unbind()
{
	if (UGSRunStats* Stats = BoundStats.Get())
	{
		Stats->OnStatsChanged.RemoveDynamic(this, &UGSRunSummaryWidget::HandleStatsChanged);
	}
	if (UGSProfileSubsystem* Profile = BoundProfile.Get())
	{
		Profile->OnCashChanged.RemoveDynamic(this, &UGSRunSummaryWidget::HandleCashChanged);
	}
	BoundGameState.Reset();
	BoundStats.Reset();
	BoundProfile.Reset();
}

void UGSRunSummaryWidget::HandleStatsChanged()
{
	Refresh();
}

void UGSRunSummaryWidget::HandleCashChanged(int32 NewCash, int32 Delta)
{
	Refresh();
}

void UGSRunSummaryWidget::Refresh()
{
	auto SetText = [](UTextBlock* Block, const FText& Text)
	{
		if (Block)
		{
			Block->SetText(Text);
		}
	};
	SetText(RoundsClearedValue, GetRoundsClearedText());
	SetText(LevelReachedValue, GetLevelReachedText());
	SetText(KillsValue, GetKillsText());
	SetText(BestComboValue, GetBestComboText());
	SetText(CashEarnedValue, GetCashEarnedText());
	SetText(TotalCashValue, GetTotalCashText());
	SetText(RunTimeValue, GetRunTimeText());
	SetText(SeedValue, GetSeedText());
	RefreshUpgrades();
}

const UGSUpgradeComponent* UGSRunSummaryWidget::GetUpgrades() const
{
	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	return PS ? PS->GetUpgradeComponent() : nullptr;
}

void UGSRunSummaryWidget::RefreshUpgrades()
{
	const UGSUpgradeComponent* Upgrades = GetUpgrades();
	TArray<const FGSUpgradeStack*> Picked;
	if (Upgrades)
	{
		for (const FGSUpgradeStack& Stack : Upgrades->GetUpgradeStacks())
		{
			if (Stack.Upgrade && Stack.Stacks > 0)
			{
				Picked.Add(&Stack);
			}
		}
	}

	if (UpgradesBox && UpgradeIconWidgetClass)
	{
		// Reuse icons in pick order; create more if needed, drop the rest.
		while (UpgradeIcons.Num() < Picked.Num())
		{
			UGSUpgradeIconWidget* Icon = CreateWidget<UGSUpgradeIconWidget>(GetOwningPlayer(), UpgradeIconWidgetClass);
			if (!Icon)
			{
				break;
			}
			UPanelSlot* IconSlot = UpgradesBox->AddChild(Icon);
			if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(IconSlot))
			{
				WrapSlot->SetPadding(FMargin(0.f, 0.f, UpgradeIconSpacing, UpgradeIconSpacing));
			}
			else if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(IconSlot))
			{
				BoxSlot->SetPadding(FMargin(0.f, 0.f, UpgradeIconSpacing, 0.f));
			}
			UpgradeIcons.Add(Icon);
		}
		while (UpgradeIcons.Num() > Picked.Num())
		{
			UpgradeIcons.Pop()->RemoveFromParent();
		}
		for (int32 Index = 0; Index < UpgradeIcons.Num(); ++Index)
		{
			UpgradeIcons[Index]->SetUpgrade(Picked[Index]->Upgrade, Picked[Index]->Stacks);
		}
		UpgradesBox->SetVisibility(Picked.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (NoUpgradesText)
	{
		NoUpgradesText->SetVisibility(Picked.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------
// Texts
// ---------------------------------------------------------------------------

FText UGSRunSummaryWidget::GetRoundsClearedText() const
{
	const UGSRunStats* Stats = BoundStats.Get();
	return FText::AsNumber(Stats ? Stats->GetRoundsCleared() : 0);
}

FText UGSRunSummaryWidget::GetLevelReachedText() const
{
	const UGSRunStats* Stats = BoundStats.Get();
	return FText::AsNumber(Stats ? Stats->GetLevelReached() : 1);
}

FText UGSRunSummaryWidget::GetKillsText() const
{
	const UGSRunStats* Stats = BoundStats.Get();
	return FText::AsNumber(Stats ? Stats->GetTotalKills() : 0);
}

FText UGSRunSummaryWidget::GetBestComboText() const
{
	const UGSRunStats* Stats = BoundStats.Get();
	return FText::AsNumber(Stats ? Stats->GetBestCombo() : 0);
}

FText UGSRunSummaryWidget::GetCashEarnedText() const
{
	const UGSRunStats* Stats = BoundStats.Get();
	return FText::Format(NSLOCTEXT("GSRunSummary", "Cash", "${0}"), FText::AsNumber(Stats ? Stats->GetCashEarned() : 0));
}

FText UGSRunSummaryWidget::GetTotalCashText() const
{
	const UGSProfileSubsystem* Profile = BoundProfile.Get();
	return FText::Format(NSLOCTEXT("GSRunSummary", "Cash", "${0}"), FText::AsNumber(Profile ? Profile->GetCash() : 0));
}

FText UGSRunSummaryWidget::GetRunTimeText() const
{
	const UGSRunStats* Stats = BoundStats.Get();
	return FormatRunTime(Stats ? Stats->GetRunTime() : 0.f);
}

FText UGSRunSummaryWidget::GetSeedText() const
{
	const AGSGameState* GS = BoundGameState.Get();
	// No digit grouping: the seed is typed back in as gs.Run.Seed N.
	return FText::AsCultureInvariant(FString::FromInt(GS ? GS->GetRunSeed() : 0));
}

FText UGSRunSummaryWidget::GetUpgradesText() const
{
	TArray<FText> Parts;
	if (const UGSUpgradeComponent* Upgrades = GetUpgrades())
	{
		for (const FGSUpgradeStack& Stack : Upgrades->GetUpgradeStacks())
		{
			if (Stack.Upgrade && Stack.Stacks > 0)
			{
				Parts.Add(FText::Format(NSLOCTEXT("GSRunSummary", "UpgradeStack", "{0} x{1}"), Stack.Upgrade->DisplayName, FText::AsNumber(Stack.Stacks)));
			}
		}
	}
	return Parts.IsEmpty() ? NSLOCTEXT("GSRunSummary", "NoUpgrades", "None") : FText::Join(FText::FromString(TEXT(", ")), Parts);
}

FText UGSRunSummaryWidget::FormatRunTime(float Seconds)
{
	const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
	const int32 Hours = Total / 3600;
	const int32 Minutes = (Total / 60) % 60;
	const int32 Secs = Total % 60;
	return FText::AsCultureInvariant(Hours > 0 ? FString::Printf(TEXT("%d:%02d:%02d"), Hours, Minutes, Secs) : FString::Printf(TEXT("%d:%02d"), Minutes, Secs));
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

bool UGSRunSummaryWidget::TryReturnToHub()
{
	if (bReturnRequested || IsInputLocked())
	{
		return false;
	}
	AGSPlayerController* PC = Cast<AGSPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return false;
	}
	bReturnRequested = true;
	FadeOutElapsed = 0.f;
	if (ReturnButton)
	{
		ReturnButton->SetIsEnabled(false);
	}
	PC->ServerRequestReturnToHub(); // the GameMode fades the camera, then opens the hub
	return true;
}

void UGSRunSummaryWidget::HandleReturnClicked()
{
	TryReturnToHub();
}

bool UGSRunSummaryWidget::HandleReturnKey(const FKey& Key, bool bIsRepeat)
{
	if (!ReturnKeys.Contains(Key))
	{
		return false;
	}
	// A key held from gameplay (R is reload) only repeats; it never presses the button.
	if (!bIsRepeat)
	{
		TryReturnToHub();
	}
	return true;
}

FReply UGSRunSummaryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleReturnKey(InKeyEvent.GetKey(), InKeyEvent.IsRepeat()))
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
