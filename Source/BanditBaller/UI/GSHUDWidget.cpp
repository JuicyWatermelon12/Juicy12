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
#include "GSPlayerState.h"
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

void UGSHUDWidget::NativeDestruct()
{
	UnbindAbilitySystem();
	Super::NativeDestruct();
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

void UGSHUDWidget::AddDamageNumber(const FVector& WorldLocation, float Damage, bool bCritical)
{
	const int32 PoolSize = FMath::Max(MaxDamageNumbers, 1);
	if (DamageNumbers.Num() != PoolSize)
	{
		DamageNumbers.SetNum(PoolSize);
		NextDamageNumber = 0;
	}

	FDamageNumber& Entry = DamageNumbers[NextDamageNumber];
	NextDamageNumber = (NextDamageNumber + 1) % PoolSize;

	Entry.WorldLocation = WorldLocation;
	Entry.Damage = Damage;
	Entry.Age = 0.f;
	Entry.JitterX = FMath::FRandRange(-12.f, 12.f); // cosmetic, local only (not a gameplay roll)
	Entry.bCritical = bCritical;
	Entry.bActive = true;
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

	UpdateWeaponDisplay();

	HitMarkerRemaining = FMath::Max(HitMarkerRemaining - InDeltaTime, 0.f);
	DamageFlashRemaining = FMath::Max(DamageFlashRemaining - InDeltaTime, 0.f);
	PulseTime += InDeltaTime;

	for (FDamageNumber& Entry : DamageNumbers)
	{
		if (Entry.bActive)
		{
			Entry.Age += InDeltaTime;
			Entry.bActive = Entry.Age < DamageNumberLifetime;
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
	if (!PC || DamageNumberLifetime <= 0.f)
	{
		return;
	}

	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	for (const FDamageNumber& Entry : DamageNumbers)
	{
		if (!Entry.bActive)
		{
			continue;
		}

		const float T = Entry.Age / DamageNumberLifetime;
		const FVector World = Entry.WorldLocation + FVector(0.f, 0.f, DamageNumberRise * T);

		FVector2D ScreenPos;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, World, ScreenPos, true))
		{
			continue;
		}

		const FString Text = Entry.bCritical
			? FString::Printf(TEXT("%.0f!"), Entry.Damage)
			: FString::Printf(TEXT("%.0f"), Entry.Damage);
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Entry.bCritical ? CritFontSize : DamageNumberFontSize);
		const FVector2f TextSize = FVector2f(FontMeasure->Measure(Text, Font));
		const FVector2f Pos = FVector2f(ScreenPos) - TextSize * 0.5f + FVector2f(Entry.JitterX, 0.f);

		const float Alpha = 1.f - FMath::Square(T);
		FLinearColor Color = Entry.bCritical ? CritColor : DamageNumberColor;
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
