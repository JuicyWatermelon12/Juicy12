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

void UGSUpgradeCardWidget::SetUpgrade(const UGSUpgradeData* InUpgrade, int32 OwnedStacks, int32 Index)
{
	Upgrade = InUpgrade;
	CardIndex = Index;
	if (!InUpgrade)
	{
		return;
	}

	const FLinearColor Color = GetRarityColor(InUpgrade->Rarity);
	if (CardBorder)
	{
		CardBorder->SetBrushColor(Color);
	}
	if (NameText)
	{
		NameText->SetText(InUpgrade->DisplayName);
	}
	if (RarityText)
	{
		RarityText->SetText(FText::FromString(UGSUpgradeData::GetRarityName(InUpgrade->Rarity).ToUpper()));
		RarityText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(InUpgrade->GetFormattedDescription(OwnedStacks)); // the text for the next pick
	}
	if (OwnedText)
	{
		OwnedText->SetText(FText::Format(NSLOCTEXT("GSUpgradeCard", "Owned", "Owned: {0}"), FText::AsNumber(OwnedStacks)));
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

FLinearColor UGSUpgradeCardWidget::GetRarityColor(EGSUpgradeRarity Rarity) const
{
	const FLinearColor* Color = RarityColors.Find(Rarity);
	return Color ? *Color : FLinearColor::White;
}

FText UGSUpgradeCardWidget::GetOwnedText() const
{
	return OwnedText ? OwnedText->GetText() : FText::GetEmpty();
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
