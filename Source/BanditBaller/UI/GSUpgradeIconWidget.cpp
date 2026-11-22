// BanditBaller

#include "GSUpgradeIconWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UGSUpgradeIconWidget::SetUpgrade(const UGSUpgradeData* InUpgrade, int32 InStacks)
{
	const bool bNewUpgrade = InUpgrade != Upgrade;
	Upgrade = InUpgrade;
	Stacks = InStacks;
	if (!Upgrade)
	{
		return;
	}

	if (bNewUpgrade)
	{
		if (IconBorder)
		{
			const FLinearColor* Color = RarityColors.Find(Upgrade->Rarity);
			IconBorder->SetBrushColor(Color ? *Color : FLinearColor::White);
		}
		if (IconImage)
		{
			if (UTexture2D* Texture = Upgrade->Icon.LoadSynchronous())
			{
				IconImage->SetBrushFromTexture(Texture);
			}
		}
		if (NameText)
		{
			NameText->SetText(Upgrade->DisplayName);
		}
	}
	if (StackText)
	{
		StackText->SetText(GetStackText());
	}
}

FText UGSUpgradeIconWidget::GetStackText() const
{
	return FText::Format(NSLOCTEXT("GSUpgradeIcon", "Stacks", "x{0}"), FText::AsNumber(Stacks));
}
