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
#include "GSGameState.h"
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
}

void UGSHUDWidget::NativeDestruct()
{
	UnbindAbilitySystem();
	UnbindCash();
	UnbindProgression();
	Super::NativeDestruct();
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
		XPText->SetText(Progression->IsMaxLevel()
			? NSLOCTEXT("GSHUD", "XPMax", "MAX")
			: FText::Format(NSLOCTEXT("GSHUD", "XPFormat", "{0} / {1} XP"),
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

void UGSHUDWidget::AddDamageNumber(const FVector& WorldLocation, float Damage, bool bCritical)
{
	FDamageNumber& Entry = AllocateFloatingNumber();
	Entry.WorldLocation = WorldLocation;
	Entry.Damage = Damage;
	Entry.Lifetime = DamageNumberLifetime;
	Entry.bCritical = bCritical;
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

	UpdateWeaponDisplay();
	TickLevelUpFeedback(InDeltaTime);
	TickXPPopupPairing(InDeltaTime);

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

void UGSHUDWidget::UpdateWeaponDisplay()
{
	const UGSWeaponComponent* Weapons = FindWeaponComponent();
	const UGSWeaponData* Data = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	const int32 Ammo = Weapons ? Weapons->GetCurrentAmmo() : 0;
	const int32 Magazine = Data ? Data->MagazineSize : 0;

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
		else if (Entry.bCritical)
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
