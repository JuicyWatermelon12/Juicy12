// BanditBaller

#include "GSUpgradeData.h"
#include "GameplayEffect.h"
#include "GSGameplayEffects.h"

const FPrimaryAssetType UGSUpgradeData::PrimaryAssetType(TEXT("Upgrade"));

FPrimaryAssetId UGSUpgradeData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

bool UGSUpgradeData::IsPercentUpgrade() const
{
	const UGSUpgradeEffect* GE = Effect ? Cast<UGSUpgradeEffect>(Effect->GetDefaultObject()) : nullptr;
	return GE && GE->IsPercentUpgrade();
}

FString UGSUpgradeData::FormatValue(float Value) const
{
	const float Shown = ToDisplayValue(Value);
	FString Number = FString::SanitizeFloat(FMath::RoundToDouble(Shown * 1000.0) / 1000.0, 0);
	return FString::Printf(TEXT("%s%s%s"), Shown >= 0.f ? TEXT("+") : TEXT(""), *Number, IsPercentUpgrade() ? TEXT("%") : TEXT(""));
}

FText UGSUpgradeData::GetFormattedDescription(int32 OwnedStacks) const
{
	const FText& Text = DescriptionsByOwnedStacks.IsEmpty() ? Description
		: DescriptionsByOwnedStacks[FMath::Clamp(OwnedStacks, 0, DescriptionsByOwnedStacks.Num() - 1)];
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 1;
	FFormatNamedArguments Args;
	Args.Add(TEXT("PerStack"), FText::AsNumber(GetPerStackValue(), &Options));
	// For additive fractions such as a chance: 0.05 -> "5".
	Args.Add(TEXT("PerStackPercent"), FText::AsNumber(GetPerStackValue() * 100.f, &Options));
	return FText::Format(Text, Args);
}

FString UGSUpgradeData::GetRarityName(EGSUpgradeRarity InRarity)
{
	return StaticEnum<EGSUpgradeRarity>()->GetNameStringByValue(static_cast<int64>(InRarity));
}

FString UGSUpgradeData::GetCategoryName() const
{
	switch (Category)
	{
	case EGSUpgradeCategory::StatBoost: return TEXT("Stat Boost");
	case EGSUpgradeCategory::Ability: return bLegendaryAbility ? TEXT("Legendary ability") : TEXT("Ability");
	case EGSUpgradeCategory::Weapon: return TEXT("Weapon");
	}
	return TEXT("?");
}

bool UGSUpgradeData::ParseRarity(const FString& Text, EGSUpgradeRarity& OutRarity)
{
	const UEnum* Enum = StaticEnum<EGSUpgradeRarity>();
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index) // last entry is the generated _MAX
	{
		const FString Name = Enum->GetNameStringByIndex(Index);
		if (Name.Equals(Text, ESearchCase::IgnoreCase) || (Text.Len() == 1 && Name.StartsWith(Text, ESearchCase::IgnoreCase)))
		{
			OutRarity = static_cast<EGSUpgradeRarity>(Enum->GetValueByIndex(Index));
			return true;
		}
	}
	return false;
}
