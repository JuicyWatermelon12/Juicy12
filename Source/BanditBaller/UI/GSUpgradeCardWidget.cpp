// BanditBaller

#include "GSUpgradeCardWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UGSUpgradeCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CardButton)
	{
		CardButton->OnClicked.AddUniqueDynamic(this, &UGSUpgradeCardWidget::HandleButtonClicked);
	}
}

void UGSUpgradeCardWidget::SetChoice(const UGSUpgradeData* InUpgrade, EGSUpgradeRarity InRarity, float PickValue, int32 OwnedPicks,
	float OwnedTotal, int32 Index)
{
	Upgrade = InUpgrade;
	Rarity = InRarity;
	CardIndex = Index;
	if (!InUpgrade)
	{
		return;
	}

	const FLinearColor Color = GetRarityColor(InRarity);
	if (CardBorder)
	{
		CardBorder->SetBrushColor(Color);
	}
	if (CategoryText)
	{
		CategoryText->SetText(FText::FromString(InUpgrade->GetCategoryName().ToUpper()));
	}
	if (NameText)
	{
		NameText->SetText(InUpgrade->DisplayName);
	}
	if (RarityText)
	{
		RarityText->SetText(FText::FromString(UGSUpgradeData::GetRarityName(InRarity).ToUpper()));
		RarityText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (PickText)
	{
		// A Common pick that only gives a weapon (or one of its guns) adds no value: the description says what it adds.
		const bool bHasValue = PickValue > 0.f;
		PickText->SetText(bHasValue ? InUpgrade->FormatValueText(PickValue) : FText::GetEmpty());
		PickText->SetVisibility(bHasValue ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(InUpgrade->GetFormattedDescription(OwnedPicks)); // the text for the next pick
	}
	if (NowText)
	{
		NowText->SetText(OwnedPicks > 0
			? FText::Format(NSLOCTEXT("GSUpgradeCard", "Now", "Now: {0}"), InUpgrade->FormatValueText(OwnedTotal))
			: NSLOCTEXT("GSUpgradeCard", "NowNone", "Now: none"));
	}
	if (KeyHintText)
	{
		KeyHintText->SetText(FText::Format(NSLOCTEXT("GSUpgradeCard", "KeyHint", "[{0}]"), FText::AsNumber(Index + 1)));
	}
	if (IconImage)
	{
		if (UTexture2D* Texture = InUpgrade->Icon.LoadSynchronous())
		{
			IconImage->SetBrushFromTexture(Texture);
		}
	}
	SetSelected(false);
}

void UGSUpgradeCardWidget::SetSelected(bool bSelected)
{
	SetRenderScale(FVector2D(bSelected ? SelectedScale : 1.f));
}

FLinearColor UGSUpgradeCardWidget::GetRarityColor(EGSUpgradeRarity InRarity) const
{
	const FLinearColor* Color = RarityColors.Find(InRarity);
	return Color ? *Color : FLinearColor::White;
}

FText UGSUpgradeCardWidget::GetCategoryText() const
{
	return CategoryText ? CategoryText->GetText() : FText::GetEmpty();
}

FText UGSUpgradeCardWidget::GetPickText() const
{
	return PickText ? PickText->GetText() : FText::GetEmpty();
}

FText UGSUpgradeCardWidget::GetNowText() const
{
	return NowText ? NowText->GetText() : FText::GetEmpty();
}

FText UGSUpgradeCardWidget::GetDescriptionText() const
{
	return DescriptionText ? DescriptionText->GetText() : FText::GetEmpty();
}

FText UGSUpgradeCardWidget::GetRarityText() const
{
	return RarityText ? RarityText->GetText() : FText::GetEmpty();
}

FLinearColor UGSUpgradeCardWidget::GetBorderColor() const
{
	return CardBorder ? CardBorder->GetBrushColor() : FLinearColor::Transparent;
}

void UGSUpgradeCardWidget::HandleButtonClicked()
{
	OnCardClicked.ExecuteIfBound(CardIndex);
}
