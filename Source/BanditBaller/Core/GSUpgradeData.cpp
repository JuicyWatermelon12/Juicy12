// BanditBaller

#include "GSUpgradeData.h"
#include "GameplayEffect.h"

const FPrimaryAssetType UGSUpgradeData::PrimaryAssetType(TEXT("Upgrade"));

FPrimaryAssetId UGSUpgradeData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

float UGSUpgradeData::GetPerStackValue() const
{
	const UGameplayEffect* GE = Effect ? Effect->GetDefaultObject<UGameplayEffect>() : nullptr;
	if (!GE)
	{
		return 0.f;
	}
	for (const FGameplayModifierInfo& Modifier : GE->Modifiers)
	{
		float Magnitude = 0.f;
		if (!Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Magnitude))
		{
			continue;
		}
		switch (Modifier.ModifierOp)
		{
		case EGameplayModOp::MultiplyAdditive:
		case EGameplayModOp::MultiplyCompound:
			return (Magnitude - 1.f) * 100.f;
		default:
			return Magnitude;
		}
	}
	return 0.f;
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
