// BanditBaller

#include "GSUpgradeChoiceWidget.h"
#include "BanditBaller.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GSGameMode.h"
#include "GSPlayerState.h"
#include "GSUpgradeCardWidget.h"

void UGSUpgradeChoiceWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (RerollButton)
	{
		RerollButton->OnClicked.AddUniqueDynamic(this, &UGSUpgradeChoiceWidget::HandleRerollClicked);
	}
}

void UGSUpgradeChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideCards();
	TryBindUpgrades();
}

void UGSUpgradeChoiceWidget::NativeDestruct()
{
	UnbindUpgrades();
	Super::NativeDestruct();
}

bool UGSUpgradeChoiceWidget::TryBindUpgrades()
{
	if (BoundUpgrades.IsValid())
	{
		return true;
	}
	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
	if (!Upgrades)
	{
		return false;
	}
	BoundUpgrades = Upgrades;
	Upgrades->OnUpgradeOfferChanged.AddUniqueDynamic(this, &UGSUpgradeChoiceWidget::HandleOfferChanged);
	Upgrades->OnRerollsLeftChanged.AddUniqueDynamic(this, &UGSUpgradeChoiceWidget::HandleRerollsLeftChanged);

	// Bound late with an offer already open (e.g. the PlayerState replicated after this widget was made).
	if (Upgrades->HasOpenOffer())
	{
		HandleOfferChanged(EGSUpgradeOfferChange::Opened);
	}
	return true;
}

void UGSUpgradeChoiceWidget::UnbindUpgrades()
{
	if (UGSUpgradeComponent* Upgrades = BoundUpgrades.Get())
	{
		Upgrades->OnUpgradeOfferChanged.RemoveDynamic(this, &UGSUpgradeChoiceWidget::HandleOfferChanged);
		Upgrades->OnRerollsLeftChanged.RemoveDynamic(this, &UGSUpgradeChoiceWidget::HandleRerollsLeftChanged);
	}
	BoundUpgrades.Reset();
}

void UGSUpgradeChoiceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Only binding is retried here; offer state arrives through events. The widget keeps ticking while the game is
	// paused, so the timers below run in real time.
	TryBindUpgrades();

	if (RevealRemaining >= 0.f)
	{
		RevealRemaining -= InDeltaTime;
		if (RevealRemaining < 0.f)
		{
			ShowCards();
		}
	}

	// Cards fade in over the input lock.
	if (bCardsShowing && CardsBox)
	{
		const double Left = InputUnlockTime - GetRealTime();
		const float Alpha = InputLockDuration > 0.f ? 1.f - FMath::Clamp(static_cast<float>(Left) / InputLockDuration, 0.f, 1.f) : 1.f;
		CardsBox->SetRenderOpacity(FMath::Lerp(0.25f, 1.f, Alpha));
	}
}

double UGSUpgradeChoiceWidget::GetRealTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : FPlatformTime::Seconds();
}

bool UGSUpgradeChoiceWidget::IsInputLocked() const
{
	return GetRealTime() < InputUnlockTime;
}

void UGSUpgradeChoiceWidget::LockInput()
{
	InputUnlockTime = GetRealTime() + InputLockDuration;
}

// ---------------------------------------------------------------------------
// Offer events
// ---------------------------------------------------------------------------

void UGSUpgradeChoiceWidget::HandleOfferChanged(EGSUpgradeOfferChange Change)
{
	APlayerController* PC = GetOwningPlayer();
	switch (Change)
	{
	case EGSUpgradeOfferChange::Opened:
		if (bCardsShowing)
		{
			// The next queued offer after a pick: swap the cards, no banner wait.
			RebuildCards();
			LockInput();
		}
		else if (RevealRemaining < 0.f)
		{
			// Fresh offer: the game is paused now. Release held buttons (fire, aim) so nothing sticks afterwards,
			// let the "LEVEL UP!" banner play, then show the cards.
			if (PC)
			{
				PC->FlushPressedKeys();
			}
			RevealRemaining = RevealDelay;
			if (RevealRemaining <= 0.f)
			{
				RevealRemaining = -1.f;
				ShowCards();
			}
		}
		break;

	case EGSUpgradeOfferChange::Rerolled:
		if (bCardsShowing)
		{
			RebuildCards();
			LockInput();
		}
		break;

	case EGSUpgradeOfferChange::Closed:
		RevealRemaining = -1.f;
		HideCards();
		break;
	}
}

void UGSUpgradeChoiceWidget::HandleRerollsLeftChanged(int32 RerollsLeft)
{
	UpdateRerollButton();
}

// ---------------------------------------------------------------------------
// Show / hide
// ---------------------------------------------------------------------------

void UGSUpgradeChoiceWidget::ShowCards()
{
	const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get();
	if (!Upgrades || !Upgrades->HasOpenOffer())
	{
		return;
	}
	bCardsShowing = true;
	RebuildCards();
	LockInput();
	if (ChoicePanel)
	{
		ChoicePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
	SetKeyboardFocus();
}

void UGSUpgradeChoiceWidget::HideCards()
{
	const bool bWasShowing = bCardsShowing;
	bCardsShowing = false;
	SetSelectedIndex(INDEX_NONE);
	if (ChoicePanel)
	{
		ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	// Stay on screen (so the widget keeps ticking) but let input through to the game.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (bWasShowing)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
		}
	}
}

void UGSUpgradeChoiceWidget::RebuildCards()
{
	const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get();
	if (!CardsBox || !Upgrades)
	{
		return;
	}
	const TArray<UGSUpgradeData*> Choices = Upgrades->GetCurrentChoices();

	// Reuse card widgets; create more if the offer is bigger than before.
	while (Cards.Num() < Choices.Num() && CardWidgetClass)
	{
		UGSUpgradeCardWidget* Card = CreateWidget<UGSUpgradeCardWidget>(this, CardWidgetClass);
		if (!Card)
		{
			break;
		}
		Card->OnCardClicked.BindUObject(this, &UGSUpgradeChoiceWidget::HandleCardClicked);
		CardsBox->AddChild(Card);
		Cards.Add(Card);
	}
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const bool bUsed = Choices.IsValidIndex(Index);
		Cards[Index]->SetVisibility(bUsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bUsed)
		{
			Cards[Index]->SetUpgrade(Choices[Index], Upgrades->GetStacks(Choices[Index]), Index);
		}
	}
	// Trim unused cards from the count tests and keys see.
	while (Cards.Num() > Choices.Num())
	{
		Cards.Pop()->RemoveFromParent();
	}

	SetSelectedIndex(INDEX_NONE);
	UpdateRerollButton();
	UpdateQueuedText();
}

void UGSUpgradeChoiceWidget::UpdateRerollButton()
{
	const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get();
	const int32 Left = Upgrades ? Upgrades->GetRerollsLeft() : 0;
	if (RerollText)
	{
		RerollText->SetText(FText::Format(NSLOCTEXT("GSUpgradeChoice", "Reroll", "Reroll ({0} left)"), FText::AsNumber(Left)));
	}
	if (RerollButton)
	{
		RerollButton->SetIsEnabled(Left > 0);
	}
}

void UGSUpgradeChoiceWidget::UpdateQueuedText()
{
	if (!QueuedText)
	{
		return;
	}
	const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get();
	const int32 Queued = Upgrades ? Upgrades->GetQueuedOffers() : 0;
	QueuedText->SetVisibility(Queued > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	QueuedText->SetText(FText::Format(NSLOCTEXT("GSUpgradeChoice", "Queued", "+{0} more {0}|plural(one=upgrade,other=upgrades) after this"), FText::AsNumber(Queued)));
}

void UGSUpgradeChoiceWidget::SetSelectedIndex(int32 NewIndex)
{
	SelectedIndex = Cards.IsValidIndex(NewIndex) ? NewIndex : INDEX_NONE;
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		Cards[Index]->SetSelected(Index == SelectedIndex);
	}
}

FText UGSUpgradeChoiceWidget::GetRerollLabel() const
{
	return RerollText ? RerollText->GetText() : FText::GetEmpty();
}

bool UGSUpgradeChoiceWidget::IsRerollEnabled() const
{
	return RerollButton && RerollButton->GetIsEnabled();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

bool UGSUpgradeChoiceWidget::TryChoose(int32 Index)
{
	if (!bCardsShowing || IsInputLocked() || !Cards.IsValidIndex(Index))
	{
		return false;
	}
	UWorld* World = GetWorld();
	AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	// The component's events (Opened for the next offer, or Closed) update this widget.
	return GameMode && PS && GameMode->ChooseUpgrade(PS, Index);
}

bool UGSUpgradeChoiceWidget::TryReroll()
{
	if (!bCardsShowing || IsInputLocked())
	{
		return false;
	}
	UWorld* World = GetWorld();
	AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	return GameMode && PS && GameMode->RerollUpgradeOffer(PS);
}

void UGSUpgradeChoiceWidget::HandleCardClicked(int32 Index)
{
	TryChoose(Index);
}

void UGSUpgradeChoiceWidget::HandleRerollClicked()
{
	TryReroll();
}

bool UGSUpgradeChoiceWidget::HandleChoiceKey(const FKey& Key)
{
	if (!bCardsShowing)
	{
		return false;
	}

	int32 CardIndex = CardKeys.IndexOfByKey(Key);
	if (CardIndex == INDEX_NONE)
	{
		CardIndex = CardKeysAlt.IndexOfByKey(Key);
	}
	if (CardIndex != INDEX_NONE)
	{
		TryChoose(CardIndex);
		return true;
	}
	if (RerollKeys.Contains(Key))
	{
		TryReroll();
		return true;
	}
	if (SelectLeftKeys.Contains(Key) || SelectRightKeys.Contains(Key))
	{
		const int32 Step = SelectRightKeys.Contains(Key) ? 1 : -1;
		const int32 Start = SelectedIndex == INDEX_NONE ? (Step > 0 ? -1 : Cards.Num()) : SelectedIndex;
		SetSelectedIndex(FMath::Clamp(Start + Step, 0, Cards.Num() - 1));
		return true;
	}
	if (ConfirmKeys.Contains(Key))
	{
		if (SelectedIndex == INDEX_NONE)
		{
			SetSelectedIndex(0); // first press only shows the selection
		}
		else
		{
			TryChoose(SelectedIndex);
		}
		return true;
	}
	return false;
}

FReply UGSUpgradeChoiceWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleChoiceKey(InKeyEvent.GetKey()))
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
