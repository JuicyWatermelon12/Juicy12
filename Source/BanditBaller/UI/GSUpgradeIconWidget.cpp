// BanditBaller

#include "GSUpgradeIconWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UGSUpgradeIconWidget::SetUpgrade(const UGSUpgradeData* InUpgrade, int32 InStacks, EGSUpgradeRarity BestRarity)
{
	const bool bNewUpgrade = InUpgrade != Upgrade;
	Upgrade = InUpgrade;
	Stacks = InStacks;
	if (!Upgrade)
	{
		return;
	}

	if (IconBorder)
	{
		const FLinearColor* Color = RarityColors.Find(BestRarity);
		IconBorder->SetBrushColor(Color ? *Color : FLinearColor::White);
	}
	if (bNewUpgrade)
	{
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

void UGSUpgradeIconWidget::SetWeaponSlot(const UGSUpgradeData* Weapon)
{
	const bool bChanged = Weapon != Upgrade || !bWeaponSlot;
	bWeaponSlot = true;
	Upgrade = Weapon;
	Stacks = 0;
	// Icon only: no pick count, no name, no rarity color.
	if (StackText)
	{
		StackText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (NameText)
	{
		NameText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IconBorder)
	{
		IconBorder->SetBrushColor(Weapon ? WeaponSlotColor : EmptyWeaponSlotColor);
	}
	if (IconImage && bChanged)
	{
		UTexture2D* Texture = Weapon ? Weapon->Icon.LoadSynchronous() : nullptr;
		if (Texture)
		{
			IconImage->SetBrushFromTexture(Texture);
		}
		IconImage->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

bool UGSUpgradeIconWidget::IsIconShown() const
{
	return IconImage && IconImage->GetVisibility() != ESlateVisibility::Hidden && IconImage->GetVisibility() != ESlateVisibility::Collapsed;
}
