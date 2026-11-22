// BanditBaller

#include "GSHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GSAttributeSet.h"
#include "GSComboComponent.h"
#include "GSGameState.h"
#include "GSSecondWindComponent.h"
#include "GSUpgradeComponent.h"
#include "GSUpgradeIconWidget.h"
#include "GSUpgradePool.h"
#include "GSGameMode.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "TimerManager.h"
#include "GSLevelRewards.h"
#include "GSPlayerState.h"
#include "GSProfileSubsystem.h"
#include "GSProgressionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GSWeaponComponent.h"
#include "GSWeaponData.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

// ---------------------------------------------------------------------------
// Health (attribute delegates)
// ---------------------------------------------------------------------------

bool UGSHUDWidget::TryBindAbilitySystem()
{
	if (BoundAbilitySystem.IsValid())
	{
		return true;
	}

	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	BoundAbilitySystem = ASC;
	HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UGSHUDWidget::HandleHealthChanged);
	MaxHealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UGSHUDWidget::HandleMaxHealthChanged);

	CurrentHealth = ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute());
	CurrentMaxHealth = ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute());
	RefreshHealthDisplay();
	return true;
}

void UGSHUDWidget::UnbindAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = BoundAbilitySystem.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
	}
	BoundAbilitySystem.Reset();
}

void UGSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LevelUpBanner)
	{
		LevelUpBanner->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (XPBar)
	{
		XPBarBaseFillColor = XPBar->GetFillColorAndOpacity();
	}
	if (ComboPanel)
	{
		ComboPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UWidget* Widget : TArray<UWidget*>{ RoundBanner.Get(), CountdownText.Get(), EnemiesText.Get(), RoundInfoPanel.Get() })
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	for (UWidget* Widget : GetStaticCombatWidgets())
	{
		CombatWidgetVisibility.Add(Widget, Widget->GetVisibility());
	}
}

// ---------------------------------------------------------------------------
// Safe area (the hub): combat elements hidden
// ---------------------------------------------------------------------------

TArray<UWidget*> UGSHUDWidget::GetStaticCombatWidgets() const
{
	TArray<UWidget*> Widgets;
	for (UWidget* Widget : { static_cast<UWidget*>(Crosshair.Get()), static_cast<UWidget*>(WeaponNameText.Get()), static_cast<UWidget*>(AmmoText.Get()),
		static_cast<UWidget*>(HealthBar.Get()), static_cast<UWidget*>(HealthText.Get()), static_cast<UWidget*>(LevelText.Get()),
		static_cast<UWidget*>(XPBar.Get()), static_cast<UWidget*>(XPText.Get()), static_cast<UWidget*>(UpgradeIconsBox.Get()), static_cast<UWidget*>(WeaponSlotsBox.Get()) })
	{
		if (Widget)
		{
			Widgets.Add(Widget);
		}
	}
	return Widgets;
}

bool UGSHUDWidget::IsInSafeArea() const
{
	const AGSGameState* GS = BoundGameState.Get();
	return GS && GS->GetCurrentArea() == EGSArea::MarketDistrict;
}

void UGSHUDWidget::RefreshAreaVisibility()
{
	const bool bSafe = IsInSafeArea();
	for (UWidget* Widget : GetStaticCombatWidgets())
	{
		const ESlateVisibility* Designed = CombatWidgetVisibility.Find(Widget);
		Widget->SetVisibility(bSafe ? ESlateVisibility::Collapsed : (Designed ? *Designed : ESlateVisibility::HitTestInvisible));
	}
}

void UGSHUDWidget::HandleAreaChanged(EGSArea OldArea, EGSArea NewArea)
{
	RefreshAreaVisibility();
	RefreshRoundInfo();
	RefreshComboDisplay();
	if (IsInSafeArea())
	{
		InterruptRoundBanner(); // waits in the queue; dropped if its round / phase is over by then
	}
	else
	{
		TryShowNextRoundBanner();
	}
}

bool UGSHUDWidget::AreCombatElementsHidden() const
{
	for (const UWidget* Widget : GetStaticCombatWidgets())
	{
		if (Widget->IsVisible())
		{
			return false;
		}
	}
	for (const UWidget* Widget : { static_cast<const UWidget*>(ComboPanel.Get()), static_cast<const UWidget*>(RoundInfoPanel.Get()), static_cast<const UWidget*>(RoundBanner.Get()) })
	{
		if (Widget && Widget->IsVisible())
		{
			return false;
		}
	}
	return true;
}

bool UGSHUDWidget::IsCashShown() const
{
	return CashText && CashText->IsVisible();
}

void UGSHUDWidget::NativeDestruct()
{
	UnbindAbilitySystem();
	UnbindCash();
	UnbindProgression();
	UnbindCombo();
	UnbindSecondWind();
	UnbindGameState();
	UnbindUpgrades();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Round info (AGSGameState)
// ---------------------------------------------------------------------------

bool UGSHUDWidget::TryBindGameState()
{
	const UWorld* World = GetWorld();
	AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
	if (GS == BoundGameState.Get())
	{
		return GS != nullptr;
	}

	UnbindGameState();
	if (!GS)
	{
		return false;
	}
	BoundGameState = GS;
	GS->OnPhaseChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandlePhaseChanged);
	GS->OnRoundChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandleRoundChanged);
	GS->OnWaveCountsChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandleWaveCountsChanged);
	GS->OnAreaChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandleAreaChanged);
	RefreshAreaVisibility();
	RefreshRoundInfo();
	RefreshComboDisplay();
	UpdateCountdown();
	// The first PreRound starts before the HUD has bound.
	if (GS->GetRoundPhase() == EGSRoundPhase::PreRound)
	{
		QueueRoundBanner(false, GS->GetRoundNumber());
	}
	return true;
}

void UGSHUDWidget::UnbindGameState()
{
	if (AGSGameState* GS = BoundGameState.Get())
	{
		GS->OnPhaseChanged.RemoveDynamic(this, &UGSHUDWidget::HandlePhaseChanged);
		GS->OnRoundChanged.RemoveDynamic(this, &UGSHUDWidget::HandleRoundChanged);
		GS->OnWaveCountsChanged.RemoveDynamic(this, &UGSHUDWidget::HandleWaveCountsChanged);
		GS->OnAreaChanged.RemoveDynamic(this, &UGSHUDWidget::HandleAreaChanged);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimer);
	}
	BoundGameState.Reset();
}

void UGSHUDWidget::HandlePhaseChanged(EGSRoundPhase OldPhase, EGSRoundPhase NewPhase)
{
	RefreshRoundInfo();
	UpdateCountdown();
	const AGSGameState* GS = BoundGameState.Get();
	if (!GS)
	{
		return;
	}
	if (NewPhase == EGSRoundPhase::PreRound)
	{
		QueueRoundBanner(false, GS->GetRoundNumber());
	}
	else if (NewPhase == EGSRoundPhase::RoundClear && OldPhase == EGSRoundPhase::Combat)
	{
		QueueRoundBanner(true, GS->GetRoundNumber());
	}
}

void UGSHUDWidget::HandleRoundChanged(int32 NewRound)
{
	RefreshRoundInfo();
}

void UGSHUDWidget::HandleWaveCountsChanged(int32 Total, int32 Remaining, int32 Alive)
{
	RefreshRoundInfo();
}

FText UGSHUDWidget::GetRoundText() const
{
	const AGSGameState* GS = BoundGameState.Get();
	return FText::Format(NSLOCTEXT("GSHUD", "RoundFormat", "ROUND {0}"), FText::AsNumber(GS ? GS->GetRoundNumber() : 0));
}

FText UGSHUDWidget::GetPhaseText() const
{
	const AGSGameState* GS = BoundGameState.Get();
	switch (GS ? GS->GetRoundPhase() : EGSRoundPhase::WaitingToStart)
	{
	case EGSRoundPhase::PreRound:   return NSLOCTEXT("GSHUD", "PhasePreRound", "PRE-ROUND");
	case EGSRoundPhase::Combat:     return NSLOCTEXT("GSHUD", "PhaseCombat", "COMBAT");
	case EGSRoundPhase::RoundClear: return NSLOCTEXT("GSHUD", "PhaseRoundClear", "ROUND CLEAR");
	case EGSRoundPhase::GameOver:   return NSLOCTEXT("GSHUD", "PhaseGameOver", "GAME OVER");
	default:                        return FText::GetEmpty();
	}
}

bool UGSHUDWidget::IsCountdownShowing() const
{
	return DisplayedCountdown >= 0;
}

FText UGSHUDWidget::GetCountdownText() const
{
	const AGSGameState* GS = BoundGameState.Get();
	return (GS && IsCountdownShowing())
		? FText::Format(NSLOCTEXT("GSHUD", "Countdown", "Round {0} in {1}…"), FText::AsNumber(GS->GetRoundNumber()), FText::AsNumber(DisplayedCountdown))
		: FText::GetEmpty();
}

bool UGSHUDWidget::IsEnemiesShowing() const
{
	const AGSGameState* GS = BoundGameState.Get();
	return GS && GS->GetRoundPhase() == EGSRoundPhase::Combat;
}

FText UGSHUDWidget::GetEnemiesText() const
{
	const AGSGameState* GS = BoundGameState.Get();
	return IsEnemiesShowing()
		? FText::Format(NSLOCTEXT("GSHUD", "EnemiesLeft", "{0} {0}|plural(one=enemy,other=enemies) left"), GS->GetWaveEnemiesRemaining())
		: FText::GetEmpty();
}

void UGSHUDWidget::RefreshRoundInfo()
{
	const AGSGameState* GS = BoundGameState.Get();
	if (!GS)
	{
		return;
	}
	if (RoundInfoPanel)
	{
		RoundInfoPanel->SetVisibility(GS->GetRoundNumber() > 0 && !IsInSafeArea() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (RoundText)
	{
		RoundText->SetText(GetRoundText());
	}
	if (PhaseText)
	{
		PhaseText->SetText(GetPhaseText());
	}
	if (EnemiesText)
	{
		EnemiesText->SetText(GetEnemiesText());
		EnemiesText->SetVisibility(IsEnemiesShowing() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGSHUDWidget::UpdateCountdown()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(CountdownTimer);
	}
	const AGSGameState* GS = BoundGameState.Get();
	DisplayedCountdown = -1;
	if (GS && GS->GetRoundPhase() == EGSRoundPhase::PreRound && GS->GetPhaseEndTime() > 0.0)
	{
		const float Remaining = GS->GetPhaseTimeRemaining();
		DisplayedCountdown = FMath::Max(FMath::CeilToInt(Remaining - KINDA_SMALL_NUMBER), 0);
		// Next update when the shown number drops by one. A world timer, like the phase timer: both stop while paused.
		if (World && DisplayedCountdown > 0)
		{
			const float UntilNext = Remaining - static_cast<float>(DisplayedCountdown - 1);
			World->GetTimerManager().SetTimer(CountdownTimer, this, &UGSHUDWidget::UpdateCountdown, FMath::Max(UntilNext, 0.05f), false);
		}
	}
	if (CountdownText)
	{
		CountdownText->SetText(GetCountdownText());
		CountdownText->SetVisibility(IsCountdownShowing() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------
// Owned upgrade icons (UGSUpgradeComponent)
// ---------------------------------------------------------------------------

bool UGSHUDWidget::TryBindUpgrades()
{
	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
	if (Upgrades == BoundUpgrades.Get())
	{
		return Upgrades != nullptr;
	}

	UnbindUpgrades();
	if (!Upgrades)
	{
		return false;
	}
	BoundUpgrades = Upgrades;
	Upgrades->OnUpgradeStacksChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandleUpgradeStacksChanged);
	OfferChangedHandle = Upgrades->OnUpgradeOfferChangedNative.AddUObject(this, &UGSHUDWidget::HandleUpgradeOfferChanged);
	RebuildUpgradeIcons();
	return true;
}

void UGSHUDWidget::UnbindUpgrades()
{
	if (UGSUpgradeComponent* Upgrades = BoundUpgrades.Get())
	{
		Upgrades->OnUpgradeStacksChanged.RemoveDynamic(this, &UGSHUDWidget::HandleUpgradeStacksChanged);
		Upgrades->OnUpgradeOfferChangedNative.Remove(OfferChangedHandle);
	}
	OfferChangedHandle.Reset();
	BoundUpgrades.Reset();
}

void UGSHUDWidget::RebuildUpgradeIcons()
{
	for (UGSUpgradeIconWidget* Icon : UpgradeIcons)
	{
		if (Icon)
		{
			Icon->RemoveFromParent();
		}
	}
	UpgradeIcons.Reset();
	if (const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get())
	{
		for (const FGSUpgradeStack& Entry : Upgrades->GetUpgradeStacks())
		{
			SetUpgradeIcon(Entry.Upgrade, Entry.Stacks);
		}
	}
	RefreshWeaponSlots();
}

void UGSHUDWidget::HandleUpgradeStacksChanged(const UGSUpgradeData* Upgrade, int32 NewStacks)
{
	if (Upgrade && Upgrade->Category == EGSUpgradeCategory::Weapon)
	{
		RefreshWeaponSlots(); // weapons live in the weapon slots, not the upgrade icon row
		return;
	}
	SetUpgradeIcon(Upgrade, NewStacks);
}

void UGSHUDWidget::RefreshWeaponSlots()
{
	if (!WeaponSlotsBox || !UpgradeIconWidgetClass)
	{
		return;
	}
	// One slot per MaxWeapons. TODO(co-op): the pool comes from the server GameMode (single-player: this machine).
	const UWorld* World = GetWorld();
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	const UGSUpgradePool* Pool = GameMode && GameMode->GetUpgradePool() ? GameMode->GetUpgradePool() : GetDefault<UGSUpgradePool>();
	const int32 SlotCount = FMath::Max(Pool->MaxWeapons, 0);
	while (WeaponSlots.Num() > SlotCount)
	{
		WeaponSlots.Pop()->RemoveFromParent();
	}
	while (WeaponSlots.Num() < SlotCount)
	{
		UGSUpgradeIconWidget* WeaponSlot = CreateWidget<UGSUpgradeIconWidget>(GetOwningPlayer(), UpgradeIconWidgetClass);
		if (!WeaponSlot)
		{
			break;
		}
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(WeaponSlotsBox->AddChild(WeaponSlot)))
		{
			BoxSlot->SetPadding(FMargin(0.f, 0.f, UpgradeIconSpacing, 0.f));
		}
		WeaponSlots.Add(WeaponSlot);
	}
	// The weapons in the order they were first picked; the rest of the slots stay empty.
	const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get();
	const TArray<const UGSUpgradeData*> Weapons = Upgrades ? Upgrades->GetOwnedWeapons() : TArray<const UGSUpgradeData*>();
	for (int32 Index = 0; Index < WeaponSlots.Num(); ++Index)
	{
		WeaponSlots[Index]->SetWeaponSlot(Weapons.IsValidIndex(Index) ? Weapons[Index] : nullptr);
	}
}

const UGSUpgradeData* UGSHUDWidget::GetWeaponInSlot(int32 Index) const
{
	return WeaponSlots.IsValidIndex(Index) && WeaponSlots[Index] ? WeaponSlots[Index]->GetUpgrade() : nullptr;
}

void UGSHUDWidget::SetUpgradeIcon(const UGSUpgradeData* Upgrade, int32 Stacks)
{
	if (!Upgrade || Upgrade->Category == EGSUpgradeCategory::Weapon) // weapons: RefreshWeaponSlots
	{
		return;
	}
	const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get();
	const FGSUpgradeStack* Entry = Upgrades ? Upgrades->FindEntry(Upgrade) : nullptr;
	const EGSUpgradeRarity BestRarity = Entry ? Entry->GetBestRarity() : EGSUpgradeRarity::Common;
	const int32 Index = UpgradeIcons.IndexOfByPredicate([Upgrade](const UGSUpgradeIconWidget* Icon) { return Icon && Icon->GetUpgrade() == Upgrade; });
	if (Stacks <= 0)
	{
		if (Index != INDEX_NONE)
		{
			UpgradeIcons[Index]->RemoveFromParent();
			UpgradeIcons.RemoveAt(Index);
		}
		return;
	}
	if (Index != INDEX_NONE)
	{
		UpgradeIcons[Index]->SetUpgrade(Upgrade, Stacks, BestRarity);
		return;
	}
	if (!UpgradeIconsBox || !UpgradeIconWidgetClass)
	{
		return;
	}
	UGSUpgradeIconWidget* Icon = CreateWidget<UGSUpgradeIconWidget>(GetOwningPlayer(), UpgradeIconWidgetClass);
	if (!Icon)
	{
		return;
	}
	Icon->SetUpgrade(Upgrade, Stacks, BestRarity);
	if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(UpgradeIconsBox->AddChild(Icon)))
	{
		IconSlot->SetPadding(FMargin(0.f, 0.f, UpgradeIconSpacing, 0.f));
	}
	UpgradeIcons.Add(Icon);
}

const UGSUpgradeIconWidget* UGSHUDWidget::FindUpgradeIcon(const UGSUpgradeData* Upgrade) const
{
	const TObjectPtr<UGSUpgradeIconWidget>* Found = UpgradeIcons.FindByPredicate([Upgrade](const UGSUpgradeIconWidget* Icon) { return Icon && Icon->GetUpgrade() == Upgrade; });
	return Found ? Found->Get() : nullptr;
}

void UGSHUDWidget::HandleUpgradeOfferChanged(EGSUpgradeOfferChange Change)
{
	// The choice cards take over the screen while an offer is open; round banners wait for them.
	if (Change == EGSUpgradeOfferChange::Closed)
	{
		TryShowNextRoundBanner();
	}
	else
	{
		InterruptRoundBanner();
	}
}

// ---------------------------------------------------------------------------
// Round banners ("ROUND N", "ROUND CLEAR")
// ---------------------------------------------------------------------------

void UGSHUDWidget::QueueRoundBanner(bool bRoundClear, int32 Round)
{
	if (Round <= 0)
	{
		return;
	}
	const FRoundBannerRequest Request{ bRoundClear, Round };
	if ((IsRoundBannerShowing() && ActiveRoundBanner == Request) || RoundBannerQueue.Contains(Request))
	{
		return;
	}
	RoundBannerQueue.Add(Request);
	TryShowNextRoundBanner();
}

bool UGSHUDWidget::CanShowRoundBanner() const
{
	const UGSUpgradeComponent* Upgrades = BoundUpgrades.Get();
	return LevelUpBannerRemaining <= 0.f && !(Upgrades && Upgrades->HasOpenOffer()) && !IsInSafeArea();
}

bool UGSHUDWidget::IsRoundBannerValid(const FRoundBannerRequest& Request) const
{
	if (Request.bSecondWind)
	{
		return true; // interrupted by another banner: still worth showing
	}
	const AGSGameState* GS = BoundGameState.Get();
	if (!GS || GS->GetRoundNumber() != Request.Round)
	{
		return false;
	}
	const EGSRoundPhase Phase = GS->GetRoundPhase();
	return Request.bRoundClear ? Phase == EGSRoundPhase::RoundClear : (Phase == EGSRoundPhase::PreRound || Phase == EGSRoundPhase::Combat);
}

void UGSHUDWidget::TryShowNextRoundBanner()
{
	if (IsRoundBannerShowing())
	{
		return;
	}
	while (RoundBannerQueue.Num() > 0 && CanShowRoundBanner())
	{
		const FRoundBannerRequest Next = RoundBannerQueue[0];
		RoundBannerQueue.RemoveAt(0);
		if (!IsRoundBannerValid(Next))
		{
			continue; // its round or phase is over
		}
		ShowBanner(Next);
		return;
	}
}

void UGSHUDWidget::ShowBanner(const FRoundBannerRequest& Request)
{
	ActiveRoundBanner = Request;
	RoundBannerRemaining = RoundBannerDuration;
	++RoundBannersShown;
	if (RoundBannerTitle)
	{
		RoundBannerTitle->SetText(GetRoundBannerTitle());
		RoundBannerTitle->SetColorAndOpacity(FSlateColor(Request.bSecondWind ? SecondWindBannerColor
			: (Request.bRoundClear ? RoundClearBannerColor : RoundBannerColor)));
	}
	if (RoundBannerSubtitle)
	{
		RoundBannerSubtitle->SetText(GetRoundBannerSubtitle());
	}
	if (RoundBanner)
	{
		RoundBanner->SetRenderOpacity(RoundBannerFadeIn > 0.f ? 0.f : 1.f);
		RoundBanner->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

FText UGSHUDWidget::GetRoundBannerTitle() const
{
	if (!IsRoundBannerShowing())
	{
		return FText::GetEmpty();
	}
	if (ActiveRoundBanner.bSecondWind)
	{
		return NSLOCTEXT("GSHUD", "SecondWindBanner", "SECOND WIND");
	}
	return ActiveRoundBanner.bRoundClear ? NSLOCTEXT("GSHUD", "RoundBannerClear", "ROUND CLEAR")
		: FText::Format(NSLOCTEXT("GSHUD", "RoundBanner", "ROUND {0}"), FText::AsNumber(ActiveRoundBanner.Round));
}

FText UGSHUDWidget::GetRoundBannerSubtitle() const
{
	if (!IsRoundBannerShowing())
	{
		return FText::GetEmpty();
	}
	if (ActiveRoundBanner.bSecondWind)
	{
		return ActiveRoundBanner.SecondWindsLeft > 0
			? FText::Format(NSLOCTEXT("GSHUD", "SecondWindBannerLeft", "{0} left"), FText::AsNumber(ActiveRoundBanner.SecondWindsLeft))
			: NSLOCTEXT("GSHUD", "SecondWindBannerLast", "THAT WAS YOUR LAST");
	}
	return ActiveRoundBanner.bRoundClear
		? FText::Format(NSLOCTEXT("GSHUD", "RoundBannerClearSub", "Round {0}"), FText::AsNumber(ActiveRoundBanner.Round))
		: NSLOCTEXT("GSHUD", "RoundBannerSub", "GET READY");
}

// ---------------------------------------------------------------------------
// Second Wind (UGSSecondWindComponent): flash, sound, banner
// ---------------------------------------------------------------------------

bool UGSHUDWidget::TryBindSecondWind()
{
	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	UGSSecondWindComponent* SecondWind = PS ? PS->GetSecondWindComponent() : nullptr;
	if (SecondWind == BoundSecondWind.Get())
	{
		return SecondWind != nullptr;
	}
	UnbindSecondWind();
	if (!SecondWind)
	{
		return false;
	}
	BoundSecondWind = SecondWind;
	SecondWindUsedHandle = SecondWind->OnSecondWindUsed.AddUObject(this, &UGSHUDWidget::HandleSecondWindUsed);
	return true;
}

void UGSHUDWidget::UnbindSecondWind()
{
	if (UGSSecondWindComponent* SecondWind = BoundSecondWind.Get())
	{
		SecondWind->OnSecondWindUsed.Remove(SecondWindUsedHandle);
	}
	BoundSecondWind.Reset();
}

void UGSHUDWidget::HandleSecondWindUsed(int32 SecondWindsLeft)
{
	SecondWindFlashRemaining = SecondWindFlashDuration;
	if (SecondWindSound)
	{
		UGameplayStatics::PlaySound2D(this, SecondWindSound);
	}
	++SecondWindSoundsPlayed;

	// The banner takes the spot at once; a round banner that was showing goes back to the front of the queue.
	InterruptRoundBanner();
	FRoundBannerRequest Request;
	Request.bSecondWind = true;
	Request.SecondWindsLeft = SecondWindsLeft;
	ShowBanner(Request);
	++SecondWindBannersShown;
}

float UGSHUDWidget::GetSecondWindFlashAlpha() const
{
	return SecondWindFlashDuration > 0.f ? SecondWindFlashMaxAlpha * (SecondWindFlashRemaining / SecondWindFlashDuration) : 0.f;
}

void UGSHUDWidget::PaintSecondWindFlash(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));
	FLinearColor Color = SecondWindFlashColor;
	Color.A = GetSecondWindFlashAlpha();
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), White, ESlateDrawEffect::None, Color);
}

void UGSHUDWidget::InterruptRoundBanner()
{
	if (!IsRoundBannerShowing())
	{
		return;
	}
	RoundBannerQueue.Insert(ActiveRoundBanner, 0);
	RoundBannerRemaining = 0.f;
	if (RoundBanner)
	{
		RoundBanner->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGSHUDWidget::TickRoundBanner(float DeltaTime)
{
	if (!IsRoundBannerShowing())
	{
		return;
	}
	RoundBannerRemaining = FMath::Max(RoundBannerRemaining - DeltaTime, 0.f);
	if (RoundBanner)
	{
		const float Elapsed = RoundBannerDuration - RoundBannerRemaining;
		float Alpha = 1.f;
		float Scale = 1.f;
		if (RoundBannerFadeIn > 0.f && Elapsed < RoundBannerFadeIn)
		{
			const float T = Elapsed / RoundBannerFadeIn;
			Alpha = T;
			Scale = FMath::Lerp(RoundBannerPopScale, 1.f, T);
		}
		else if (RoundBannerFadeOut > 0.f && RoundBannerRemaining < RoundBannerFadeOut)
		{
			Alpha = RoundBannerRemaining / RoundBannerFadeOut;
		}
		RoundBanner->SetRenderOpacity(Alpha);
		RoundBanner->SetRenderScale(FVector2D(Scale, Scale));
	}
	if (RoundBannerRemaining <= 0.f)
	{
		if (RoundBanner)
		{
			RoundBanner->SetVisibility(ESlateVisibility::Collapsed);
		}
		TryShowNextRoundBanner();
	}
}

// ---------------------------------------------------------------------------
// Kill combo (UGSComboComponent)
// ---------------------------------------------------------------------------

bool UGSHUDWidget::TryBindCombo()
{
	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	UGSComboComponent* Combo = PS ? PS->GetComboComponent() : nullptr;
	if (Combo == BoundCombo.Get())
	{
		return Combo != nullptr;
	}

	UnbindCombo();
	if (!Combo)
	{
		return false;
	}
	BoundCombo = Combo;
	Combo->OnComboChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandleComboChanged);
	DisplayedComboCount = Combo->GetComboCount();
	DisplayedComboMultiplier = Combo->GetCashMultiplier();
	RefreshComboDisplay();
	return true;
}

void UGSHUDWidget::UnbindCombo()
{
	if (UGSComboComponent* Combo = BoundCombo.Get())
	{
		Combo->OnComboChanged.RemoveDynamic(this, &UGSHUDWidget::HandleComboChanged);
	}
	BoundCombo.Reset();
}

void UGSHUDWidget::HandleComboChanged(int32 ComboCount, float CashMultiplier)
{
	if (ComboCount > DisplayedComboCount)
	{
		ComboPunchRemaining = ComboPunchDuration;
	}
	if (ComboCount > 0 && CashMultiplier > DisplayedComboMultiplier + KINDA_SMALL_NUMBER)
	{
		ComboStepPunchRemaining = ComboStepPunchDuration;
	}
	DisplayedComboCount = ComboCount;
	DisplayedComboMultiplier = CashMultiplier;
	RefreshComboDisplay();
}

bool UGSHUDWidget::IsComboShowing() const
{
	return DisplayedComboCount >= GetComboMinCountToShow();
}

FText UGSHUDWidget::GetComboCountText() const
{
	return FText::Format(NSLOCTEXT("GSHUD", "ComboCount", "{0} COMBO"), FText::AsNumber(DisplayedComboCount));
}

FText UGSHUDWidget::GetComboMultiplierText() const
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = 2;
	return FText::Format(NSLOCTEXT("GSHUD", "ComboMultiplier", "x{0} CASH"), FText::AsNumber(DisplayedComboMultiplier, &Options));
}

void UGSHUDWidget::RefreshComboDisplay()
{
	const bool bShow = IsComboShowing() && !IsInSafeArea();
	if (ComboPanel)
	{
		ComboPanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShow)
	{
		return;
	}
	if (ComboCountText)
	{
		ComboCountText->SetText(GetComboCountText());
	}
	if (ComboMultiplierText)
	{
		ComboMultiplierText->SetText(GetComboMultiplierText());
		ComboMultiplierText->SetColorAndOpacity(FSlateColor(DisplayedComboMultiplier > 1.f + KINDA_SMALL_NUMBER ? ComboBonusMultiplierColor : ComboBaseMultiplierColor));
	}
}

void UGSHUDWidget::TickCombo(float DeltaTime)
{
	// The window drains continuously, so the bar reads it every tick (game time: it stays put while paused).
	const UGSComboComponent* Combo = BoundCombo.Get();
	DisplayedComboTimerPercent = Combo ? Combo->GetTimeRemainingFraction() : 0.f;
	if (ComboTimerBar && IsComboShowing())
	{
		ComboTimerBar->SetPercent(DisplayedComboTimerPercent);
	}

	auto Punch = [DeltaTime](UWidget* Widget, float& Remaining, float Duration, float PeakScale)
	{
		if (Widget && (Remaining > 0.f || !Widget->GetRenderTransform().Scale.Equals(FVector2D::UnitVector)))
		{
			Remaining = FMath::Max(Remaining - DeltaTime, 0.f);
			const float T = Duration > 0.f ? Remaining / Duration : 0.f;
			const float Scale = FMath::Lerp(1.f, PeakScale, FMath::Sin(T * PI));
			Widget->SetRenderScale(FVector2D(Scale, Scale));
		}
	};
	Punch(ComboCountText.Get(), ComboPunchRemaining, ComboPunchDuration, ComboPunchScale);
	Punch(ComboMultiplierText.Get(), ComboStepPunchRemaining, ComboStepPunchDuration, ComboStepPunchScale);
}

// ---------------------------------------------------------------------------
// Cash counter (UGSProfileSubsystem::OnCashChanged: the permanent balance)
// ---------------------------------------------------------------------------

bool UGSHUDWidget::TryBindCash()
{
	UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(this);
	if (Profile == BoundProfile.Get())
	{
		return Profile != nullptr;
	}

	UnbindCash();
	if (!Profile)
	{
		return false;
	}
	BoundProfile = Profile;
	Profile->OnCashChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandleCashChanged);
	DisplayedCash = Profile->GetCash();
	RefreshCashDisplay();
	return true;
}

void UGSHUDWidget::UnbindCash()
{
	if (UGSProfileSubsystem* Profile = BoundProfile.Get())
	{
		Profile->OnCashChanged.RemoveDynamic(this, &UGSHUDWidget::HandleCashChanged);
	}
	BoundProfile.Reset();
}

void UGSHUDWidget::HandleCashChanged(int32 NewCash, int32 Delta)
{
	DisplayedCash = NewCash;
	if (Delta > 0)
	{
		CashPunchRemaining = CashPunchDuration;
	}
	RefreshCashDisplay();
}

void UGSHUDWidget::RefreshCashDisplay()
{
	if (CashText)
	{
		CashText->SetText(FText::Format(NSLOCTEXT("GSHUD", "CashFormat", "${0}"), FText::AsNumber(DisplayedCash)));
	}
}

// ---------------------------------------------------------------------------
// Level + XP (UGSProgressionComponent)
// ---------------------------------------------------------------------------

bool UGSHUDWidget::TryBindProgression()
{
	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	UGSProgressionComponent* Progression = PS ? PS->GetProgressionComponent() : nullptr;
	if (Progression == BoundProgression.Get())
	{
		return Progression != nullptr;
	}

	UnbindProgression();
	if (!Progression)
	{
		return false;
	}
	BoundProgression = Progression;
	Progression->OnXPChanged.AddUniqueDynamic(this, &UGSHUDWidget::HandleXPChanged);
	Progression->OnLevelUp.AddUniqueDynamic(this, &UGSHUDWidget::HandleLevelUp);
	RefreshProgressionDisplay();
	return true;
}

void UGSHUDWidget::UnbindProgression()
{
	if (UGSProgressionComponent* Progression = BoundProgression.Get())
	{
		Progression->OnXPChanged.RemoveDynamic(this, &UGSHUDWidget::HandleXPChanged);
		Progression->OnLevelUp.RemoveDynamic(this, &UGSHUDWidget::HandleLevelUp);
	}
	BoundProgression.Reset();
}

void UGSHUDWidget::HandleXPChanged(int32 CurrentXP, int32 XPForNextLevel, int32 Delta)
{
	RefreshProgressionDisplay();

	if (Delta <= 0)
	{
		return;
	}
	// Pair with an XP source (collected orb) that already arrived; otherwise wait briefly for it.
	if (RecentXPSources.Num() > 0)
	{
		AddXPPopup(RecentXPSources[0].Location, Delta);
		RecentXPSources.RemoveAt(0);
		return;
	}
	if (PendingXP <= 0)
	{
		PendingXPAge = 0.f;
	}
	PendingXP += Delta;
}

void UGSHUDWidget::HandleLevelUp(int32 NewLevel)
{
	// Fires once per level gained (several in one frame for a big XP gain); the state is already final.
	LevelPunchRemaining = LevelUpPunchDuration;
	XPBarPulseRemaining = XPBarPulseDuration;
	RefreshProgressionDisplay();
	ShowLevelUpBanner(NewLevel);

	if (LevelUpSound && LastLevelUpSoundFrame != GFrameCounter)
	{
		LastLevelUpSoundFrame = GFrameCounter;
		++LevelUpSoundsPlayed;
		UGameplayStatics::PlaySound2D(this, LevelUpSound);
	}
}

// ---------------------------------------------------------------------------
// Level-up banner + XP bar pulse
// ---------------------------------------------------------------------------

void UGSHUDWidget::ShowLevelUpBanner(int32 NewLevel)
{
	// "LEVEL UP!" takes the banner spot at once (the choice cards follow it); a round banner waits behind it.
	InterruptRoundBanner();

	// Level-ups while the banner is up merge into it ("Level 4", "+20 Max Health").
	if (LevelUpBannerRemaining <= 0.f)
	{
		BannerFirstLevel = NewLevel;
	}
	BannerLastLevel = NewLevel;
	LevelUpBannerRemaining = LevelUpBannerDuration;

	if (LevelUpLevelText)
	{
		LevelUpLevelText->SetText(FText::Format(NSLOCTEXT("GSHUD", "LevelUpLevel", "Level {0}"), FText::AsNumber(BannerLastLevel)));
	}
	if (LevelUpRewardText)
	{
		const FText Rewards = GetLevelUpRewardText();
		LevelUpRewardText->SetText(Rewards);
		LevelUpRewardText->SetVisibility(Rewards.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (LevelUpBanner)
	{
		LevelUpBanner->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

FText UGSHUDWidget::GetLevelUpRewardText() const
{
	const UWorld* World = GetWorld();
	const AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
	const UGSLevelRewards* Rewards = GS ? GS->GetLevelRewards() : nullptr;
	return (Rewards && BannerFirstLevel > 0) ? Rewards->DescribeRewards(BannerFirstLevel, BannerLastLevel) : FText::GetEmpty();
}

void UGSHUDWidget::TickLevelUpFeedback(float DeltaTime)
{
	if (LevelUpBanner && LevelUpBannerRemaining > 0.f)
	{
		LevelUpBannerRemaining = FMath::Max(LevelUpBannerRemaining - DeltaTime, 0.f);
		const float Elapsed = LevelUpBannerDuration - LevelUpBannerRemaining;
		float Alpha = 1.f;
		float Scale = 1.f;
		if (LevelUpBannerFadeIn > 0.f && Elapsed < LevelUpBannerFadeIn)
		{
			const float T = Elapsed / LevelUpBannerFadeIn;
			Alpha = T;
			Scale = FMath::Lerp(LevelUpBannerPopScale, 1.f, T);
		}
		else if (LevelUpBannerFadeOut > 0.f && LevelUpBannerRemaining < LevelUpBannerFadeOut)
		{
			Alpha = LevelUpBannerRemaining / LevelUpBannerFadeOut;
		}
		LevelUpBanner->SetRenderOpacity(Alpha);
		LevelUpBanner->SetRenderScale(FVector2D(Scale, Scale));
		if (LevelUpBannerRemaining <= 0.f)
		{
			LevelUpBanner->SetVisibility(ESlateVisibility::Collapsed);
			TryShowNextRoundBanner(); // waits on: no open offer either
		}
	}

	if (XPBar && (XPBarPulseRemaining > 0.f || !XPBar->GetRenderTransform().Scale.Equals(FVector2D::UnitVector)))
	{
		XPBarPulseRemaining = FMath::Max(XPBarPulseRemaining - DeltaTime, 0.f);
		const float T = XPBarPulseDuration > 0.f ? XPBarPulseRemaining / XPBarPulseDuration : 0.f;
		const float Wave = FMath::Sin(T * PI);
		XPBar->SetRenderScale(FVector2D(1.f, FMath::Lerp(1.f, XPBarPulseScale, Wave)));
		XPBar->SetFillColorAndOpacity(FMath::Lerp(XPBarBaseFillColor, XPBarPulseColor, Wave));
	}
}

// ---------------------------------------------------------------------------
// "+N XP" popups
// ---------------------------------------------------------------------------

void UGSHUDWidget::NotifyXPSource(const FVector& WorldLocation)
{
	if (PendingXP > 0)
	{
		AddXPPopup(WorldLocation, PendingXP);
		PendingXP = 0;
		return;
	}
	if (RecentXPSources.Num() >= 16)
	{
		RecentXPSources.RemoveAt(0);
	}
	RecentXPSources.Add({ WorldLocation, 0.f });
}

void UGSHUDWidget::TickXPPopupPairing(float DeltaTime)
{
	for (int32 Index = RecentXPSources.Num() - 1; Index >= 0; --Index)
	{
		RecentXPSources[Index].Age += DeltaTime;
		if (RecentXPSources[Index].Age > XPPopupPairWindow)
		{
			RecentXPSources.RemoveAt(Index); // a source that gave no XP
		}
	}

	// XP without a source (gs.XP.Add): show it above the player.
	if (PendingXP > 0)
	{
		PendingXPAge += DeltaTime;
		if (PendingXPAge > XPPopupPairWindow)
		{
			if (const APawn* Pawn = GetOwningPlayerPawn())
			{
				AddXPPopup(Pawn->GetActorLocation(), PendingXP);
			}
			PendingXP = 0;
		}
	}
}

void UGSHUDWidget::AddXPPopup(const FVector& WorldLocation, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	LastXPPopupAnchor = WorldLocation;
	FDamageNumber& Entry = AllocateFloatingNumber();
	Entry.WorldLocation = WorldLocation + FVector(0.f, 0.f, XPPopupHeight);
	Entry.XP = Amount;
	Entry.Lifetime = XPPopupLifetime;
	Entry.JitterX = 0.f;
	Entry.bXP = true;
}

int32 UGSHUDWidget::GetActiveXPPopupTotal() const
{
	int32 Total = 0;
	for (const FDamageNumber& Entry : DamageNumbers)
	{
		Total += (Entry.bActive && Entry.bXP) ? Entry.XP : 0;
	}
	return Total;
}

void UGSHUDWidget::AddHealthPopup(const FVector& WorldLocation, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	FDamageNumber& Entry = AllocateFloatingNumber();
	Entry.WorldLocation = WorldLocation + FVector(0.f, 0.f, HealthPopupHeight);
	Entry.Heal = Amount;
	Entry.Lifetime = HealthPopupLifetime;
	Entry.JitterX = 0.f;
	Entry.bHeal = true;
}

int32 UGSHUDWidget::GetActiveHealthPopupTotal() const
{
	int32 Total = 0;
	for (const FDamageNumber& Entry : DamageNumbers)
	{
		Total += (Entry.bActive && Entry.bHeal) ? Entry.Heal : 0;
	}
	return Total;
}

FString UGSHUDWidget::GetLevelLabelText() const
{
	return LevelText ? LevelText->GetText().ToString() : FString();
}

FString UGSHUDWidget::GetXPLabelText() const
{
	return XPText ? XPText->GetText().ToString() : FString();
}

void UGSHUDWidget::RefreshProgressionDisplay()
{
	const UGSProgressionComponent* Progression = BoundProgression.Get();
	if (!Progression)
	{
		return;
	}
	DisplayedLevel = Progression->GetLevel();
	DisplayedXPPercent = Progression->GetLevelProgress();

	if (LevelText)
	{
		LevelText->SetText(FText::Format(NSLOCTEXT("GSHUD", "LevelFormat", "LV {0}"), FText::AsNumber(DisplayedLevel)));
	}
	if (XPBar)
	{
		XPBar->SetPercent(DisplayedXPPercent);
	}
	if (XPText)
	{
		// Levels are endless, so there is always a next level to show.
		XPText->SetText(FText::Format(NSLOCTEXT("GSHUD", "XPFormat", "{0} / {1} XP"),
			FText::AsNumber(Progression->GetCurrentXP()), FText::AsNumber(Progression->GetXPForNextLevel())));
	}
}

void UGSHUDWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		DamageFlashRemaining = DamageFlashDuration;
	}

	CurrentHealth = Data.NewValue;
	RefreshHealthDisplay();
}

void UGSHUDWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentMaxHealth = Data.NewValue;
	RefreshHealthDisplay();
}

void UGSHUDWidget::RefreshHealthDisplay()
{
	const float Fraction = CurrentMaxHealth > 0.f ? FMath::Clamp(CurrentHealth / CurrentMaxHealth, 0.f, 1.f) : 0.f;

	if (HealthBar)
	{
		HealthBar->SetPercent(Fraction);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(NSLOCTEXT("GSHUD", "HealthFormat", "{0} / {1}"),
			FText::AsNumber(FMath::CeilToInt(CurrentHealth)), FText::AsNumber(FMath::CeilToInt(CurrentMaxHealth))));
	}
}

// ---------------------------------------------------------------------------
// Feedback entry points
// ---------------------------------------------------------------------------

UGSHUDWidget::FDamageNumber& UGSHUDWidget::AllocateFloatingNumber()
{
	const int32 PoolSize = FMath::Max(MaxDamageNumbers, 1);
	if (DamageNumbers.Num() != PoolSize)
	{
		DamageNumbers.SetNum(PoolSize);
		NextDamageNumber = 0;
		LastCashPopup = INDEX_NONE;
	}

	const int32 Index = NextDamageNumber;
	NextDamageNumber = (NextDamageNumber + 1) % PoolSize;
	if (Index == LastCashPopup)
	{
		LastCashPopup = INDEX_NONE; // being recycled
	}

	FDamageNumber& Entry = DamageNumbers[Index];
	Entry = FDamageNumber();
	Entry.JitterX = FMath::FRandRange(-12.f, 12.f); // cosmetic, local only (not a gameplay roll)
	Entry.bActive = true;
	return Entry;
}

void UGSHUDWidget::AddDamageNumber(const FVector& WorldLocation, float Damage, uint8 CritLevel)
{
	FDamageNumber& Entry = AllocateFloatingNumber();
	Entry.WorldLocation = WorldLocation;
	Entry.Damage = Damage;
	Entry.Lifetime = DamageNumberLifetime;
	Entry.CritLevel = CritLevel;

	++DamageNumbersAdded;
	CriticalDamageNumbersAdded += CritLevel > 0 ? 1 : 0; // triple hits are critical too
	TripleDamageNumbersAdded += CritLevel >= 2 ? 1 : 0;
	LastDamageNumber = Damage;
	bLastDamageNumberCritical = CritLevel > 0;
	LastDamageNumberCritLevel = CritLevel;
}

void UGSHUDWidget::AddCashPopup(const FVector& WorldLocation, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	// Several pieces landing together read as one "+$N" that keeps counting up.
	if (DamageNumbers.IsValidIndex(LastCashPopup))
	{
		FDamageNumber& Last = DamageNumbers[LastCashPopup];
		if (Last.bActive && Last.bCash && Last.Age < CashPopupMergeWindow)
		{
			Last.Cash += Amount;
			Last.WorldLocation = WorldLocation + FVector(0.f, 0.f, CashPopupHeight);
			Last.Age = 0.f;
			return;
		}
	}

	FDamageNumber& Entry = AllocateFloatingNumber();
	Entry.WorldLocation = WorldLocation + FVector(0.f, 0.f, CashPopupHeight);
	Entry.Cash = Amount;
	Entry.Lifetime = CashPopupLifetime;
	Entry.JitterX = 0.f;
	Entry.bCash = true;
	LastCashPopup = static_cast<int32>(&Entry - DamageNumbers.GetData());
}

int32 UGSHUDWidget::GetActiveCashPopupTotal() const
{
	int32 Total = 0;
	for (const FDamageNumber& Entry : DamageNumbers)
	{
		Total += (Entry.bActive && Entry.bCash) ? Entry.Cash : 0;
	}
	return Total;
}

void UGSHUDWidget::ShowHitMarker(bool bKill)
{
	HitMarkerRemaining = HitMarkerDuration;
	bHitMarkerKill = bKill;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void UGSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The PlayerState may replicate after the HUD is created; bind once it exists (then it's delegate-driven).
	TryBindAbilitySystem();
	TryBindCash();
	TryBindProgression();
	TryBindCombo();
	TryBindSecondWind();
	TryBindUpgrades();
	TryBindGameState();

	UpdateWeaponDisplay();
	TickLevelUpFeedback(InDeltaTime);
	TickXPPopupPairing(InDeltaTime);
	TickCombo(InDeltaTime);
	TickRoundBanner(InDeltaTime);

	// Level label punch on level-up.
	if (LevelText && (LevelPunchRemaining > 0.f || !LevelText->GetRenderTransform().Scale.Equals(FVector2D::UnitVector)))
	{
		LevelPunchRemaining = FMath::Max(LevelPunchRemaining - InDeltaTime, 0.f);
		const float T = LevelUpPunchDuration > 0.f ? LevelPunchRemaining / LevelUpPunchDuration : 0.f;
		const float Scale = FMath::Lerp(1.f, LevelUpPunchScale, FMath::Sin(T * PI));
		LevelText->SetRenderScale(FVector2D(Scale, Scale));
	}

	HitMarkerRemaining = FMath::Max(HitMarkerRemaining - InDeltaTime, 0.f);
	DamageFlashRemaining = FMath::Max(DamageFlashRemaining - InDeltaTime, 0.f);
	SecondWindFlashRemaining = FMath::Max(SecondWindFlashRemaining - InDeltaTime, 0.f);
	PulseTime += InDeltaTime;

	// Cash counter punch when cash comes in.
	if (CashText && (CashPunchRemaining > 0.f || !CashText->GetRenderTransform().Scale.Equals(FVector2D::UnitVector)))
	{
		CashPunchRemaining = FMath::Max(CashPunchRemaining - InDeltaTime, 0.f);
		const float T = CashPunchDuration > 0.f ? CashPunchRemaining / CashPunchDuration : 0.f;
		const float Scale = FMath::Lerp(1.f, CashPunchScale, FMath::Sin(T * PI));
		CashText->SetRenderScale(FVector2D(Scale, Scale));
	}

	for (FDamageNumber& Entry : DamageNumbers)
	{
		if (Entry.bActive)
		{
			Entry.Age += InDeltaTime;
			Entry.bActive = Entry.Age < Entry.Lifetime;
		}
	}
}

UGSWeaponComponent* UGSHUDWidget::FindWeaponComponent() const
{
	const APawn* Pawn = GetOwningPlayerPawn();
	return Pawn ? Pawn->FindComponentByClass<UGSWeaponComponent>() : nullptr;
}

FString UGSHUDWidget::GetAmmoText() const
{
	return AmmoText ? AmmoText->GetText().ToString() : FString();
}

void UGSHUDWidget::UpdateWeaponDisplay()
{
	const UGSWeaponComponent* Weapons = FindWeaponComponent();
	const UGSWeaponData* Data = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	const int32 Ammo = Weapons ? Weapons->GetCurrentAmmo() : 0;
	const int32 Magazine = Weapons ? Weapons->GetCurrentMagazineSize() : 0; // read every frame: follows Bigger Mags at once

	if (Data == LastWeaponData.Get() && Ammo == LastAmmo && Magazine == LastMagazine)
	{
		return;
	}

	LastWeaponData = Data;
	LastAmmo = Ammo;
	LastMagazine = Magazine;

	if (WeaponNameText)
	{
		WeaponNameText->SetText(Data ? Data->DisplayName : FText::GetEmpty());
	}

	if (AmmoText)
	{
		AmmoText->SetText(Data
			? FText::Format(NSLOCTEXT("GSHUD", "AmmoFormat", "{0} / {1}"), FText::AsNumber(Ammo), FText::AsNumber(Magazine))
			: FText::GetEmpty());
	}
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------

int32 UGSHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// Red edge: max of the hit flash and the low-health pulse.
	const float FlashAlpha = DamageFlashDuration > 0.f ? DamageFlashMaxAlpha * (DamageFlashRemaining / DamageFlashDuration) : 0.f;
	const float HealthFraction = CurrentMaxHealth > 0.f ? CurrentHealth / CurrentMaxHealth : 1.f;
	float LowHealthAlpha = 0.f;
	if (CurrentHealth > 0.f && HealthFraction < LowHealthThreshold)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(PulseTime * LowHealthPulseRate * UE_TWO_PI);
		LowHealthAlpha = LowHealthMaxAlpha * FMath::Lerp(0.4f, 1.f, Pulse);
	}
	const float EdgeAlpha = FMath::Max(FlashAlpha, LowHealthAlpha);

	if (EdgeAlpha > KINDA_SMALL_NUMBER)
	{
		PaintEdgeGlow(AllottedGeometry, OutDrawElements, ++MaxLayer, EdgeAlpha);
	}

	if (SecondWindFlashRemaining > 0.f)
	{
		PaintSecondWindFlash(AllottedGeometry, OutDrawElements, ++MaxLayer);
	}

	PaintDamageNumbers(AllottedGeometry, OutDrawElements, ++MaxLayer);

	if (HitMarkerRemaining > 0.f)
	{
		PaintHitMarker(AllottedGeometry, OutDrawElements, ++MaxLayer);
	}

	return MaxLayer;
}

void UGSHUDWidget::PaintEdgeGlow(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, float Alpha) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));
	const FVector2f Size = FVector2f(Geometry.GetLocalSize());
	const float Thickness = FMath::Min(EdgeThickness, FMath::Min(Size.X, Size.Y) * 0.5f);

	// Stacked bands fading toward the center = cheap gradient.
	constexpr int32 Bands = 10;
	const float BandSize = Thickness / Bands;

	for (int32 Band = 0; Band < Bands; ++Band)
	{
		const float BandAlpha = Alpha * FMath::Square(1.f - static_cast<float>(Band) / Bands) / Bands * 2.5f;
		const FLinearColor Color(0.8f, 0.f, 0.f, BandAlpha);
		const float Offset = Band * BandSize;

		auto DrawRect = [&](const FVector2f& Pos, const FVector2f& RectSize)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(RectSize, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, Color);
		};

		DrawRect(FVector2f(0.f, Offset), FVector2f(Size.X, BandSize));                        // top
		DrawRect(FVector2f(0.f, Size.Y - Offset - BandSize), FVector2f(Size.X, BandSize));    // bottom
		DrawRect(FVector2f(Offset, 0.f), FVector2f(BandSize, Size.Y));                        // left
		DrawRect(FVector2f(Size.X - Offset - BandSize, 0.f), FVector2f(BandSize, Size.Y));    // right
	}
}

void UGSHUDWidget::PaintHitMarker(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	const FVector2f Center = FVector2f(Geometry.GetLocalSize()) * 0.5f;
	const float Alpha = HitMarkerDuration > 0.f ? HitMarkerRemaining / HitMarkerDuration : 0.f;
	const FLinearColor Color = bHitMarkerKill ? FLinearColor(1.f, 0.15f, 0.15f, Alpha) : FLinearColor(1.f, 1.f, 1.f, Alpha);

	const float Inner = HitMarkerGap;
	const float Outer = HitMarkerGap + HitMarkerSize;
	const FVector2f Diagonals[4] = { FVector2f(1, 1), FVector2f(-1, 1), FVector2f(1, -1), FVector2f(-1, -1) };

	for (const FVector2f& Dir : Diagonals)
	{
		TArray<FVector2f> Points;
		Points.Add(Center + Dir * Inner);
		Points.Add(Center + Dir * Outer);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), MoveTemp(Points),
			ESlateDrawEffect::None, Color, true, 2.f);
	}
}

void UGSHUDWidget::PaintDamageNumbers(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	const APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	for (const FDamageNumber& Entry : DamageNumbers)
	{
		if (!Entry.bActive || Entry.Lifetime <= 0.f)
		{
			continue;
		}

		const float T = Entry.Age / Entry.Lifetime;
		const FVector World = Entry.WorldLocation + FVector(0.f, 0.f, DamageNumberRise * T);

		FVector2D ScreenPos;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, World, ScreenPos, true))
		{
			continue;
		}

		FString Text;
		int32 FontSize = DamageNumberFontSize;
		FLinearColor Color = DamageNumberColor;
		if (Entry.bCash)
		{
			Text = FString::Printf(TEXT("+$%d"), Entry.Cash);
			FontSize = CashPopupFontSize;
			Color = CashPopupColor;
		}
		else if (Entry.bXP)
		{
			Text = FString::Printf(TEXT("+%d XP"), Entry.XP);
			FontSize = XPPopupFontSize;
			Color = XPPopupColor;
		}
		else if (Entry.bHeal)
		{
			Text = FString::Printf(TEXT("+%d"), Entry.Heal);
			FontSize = HealthPopupFontSize;
			Color = HealthPopupColor;
		}
		else if (Entry.CritLevel >= 2)
		{
			Text = FString::Printf(TEXT("%.0f!!"), Entry.Damage);
			FontSize = TripleCritFontSize;
			Color = TripleCritColor;
		}
		else if (Entry.CritLevel == 1)
		{
			Text = FString::Printf(TEXT("%.0f!"), Entry.Damage);
			FontSize = CritFontSize;
			Color = CritColor;
		}
		else
		{
			Text = FString::Printf(TEXT("%.0f"), Entry.Damage);
		}
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FontSize);
		const FVector2f TextSize = FVector2f(FontMeasure->Measure(Text, Font));
		const FVector2f Pos = FVector2f(ScreenPos) - TextSize * 0.5f + FVector2f(Entry.JitterX, 0.f);

		const float Alpha = 1.f - FMath::Square(T);
		Color.A *= Alpha;

		// Drop shadow, then text.
		FSlateDrawElement::MakeText(OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Pos + FVector2f(2.f, 2.f))), Text, Font,
			ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.75f * Alpha));
		FSlateDrawElement::MakeText(OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Pos)), Text, Font,
			ESlateDrawEffect::None, Color);
	}
}
