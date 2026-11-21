// BanditBaller

#include "GSLevelRewards.h"
#include "GameplayEffect.h"

FText UGSLevelRewards::DescribeRewards(int32 FirstLevel, int32 LastLevel) const
{
	// Sum static Additive modifiers per attribute, keeping first-seen order.
	TArray<FGameplayAttribute> Order;
	TMap<FGameplayAttribute, float> Totals;
	TArray<TSubclassOf<UGameplayEffect>> Effects;
	for (int32 Level = FirstLevel; Level <= LastLevel; ++Level)
	{
		Effects.Reset();
		GetEffectsForLevel(Level, Effects);
		for (const TSubclassOf<UGameplayEffect>& Effect : Effects)
		{
			const UGameplayEffect* Def = Effect ? Effect->GetDefaultObject<UGameplayEffect>() : nullptr;
			if (!Def || Def->DurationPolicy == EGameplayEffectDurationType::Instant)
			{
				continue; // instant effects (heals etc.) aren't lasting stat gains
			}
			for (const FGameplayModifierInfo& Modifier : Def->Modifiers)
			{
				float Magnitude = 0.f;
				if (Modifier.ModifierOp != EGameplayModOp::Additive || !Modifier.Attribute.IsValid()
					|| !Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(static_cast<float>(Level), Magnitude))
				{
					continue;
				}
				if (!Totals.Contains(Modifier.Attribute))
				{
					Order.Add(Modifier.Attribute);
				}
				Totals.FindOrAdd(Modifier.Attribute) += Magnitude;
			}
		}
	}

	TArray<FText> Lines;
	for (const FGameplayAttribute& Attribute : Order)
	{
		const float Total = Totals[Attribute];
		if (FMath::IsNearlyZero(Total))
		{
			continue;
		}
		FNumberFormattingOptions Format;
		Format.MaximumFractionalDigits = 1;
		Lines.Add(FText::Format(NSLOCTEXT("GSLevelRewards", "RewardLine", "{0}{1} {2}"),
			FText::FromString(Total > 0.f ? TEXT("+") : TEXT("")), FText::AsNumber(Total, &Format),
			FText::FromString(FName::NameToDisplayString(Attribute.GetName(), false))));
	}
	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}
