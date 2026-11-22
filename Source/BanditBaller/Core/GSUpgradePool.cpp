// BanditBaller

#include "GSUpgradePool.h"
#include "GameplayEffect.h"

UGSUpgradePool::UGSUpgradePool()
{
	StatBoostRarityWeights.Add(EGSUpgradeRarity::Common, 100.f);
	StatBoostRarityWeights.Add(EGSUpgradeRarity::Rare, 20.f);
	StatBoostRarityWeights.Add(EGSUpgradeRarity::Epic, 6.f);
	StatBoostRarityWeights.Add(EGSUpgradeRarity::Legendary, 1.f);
	WeaponRarityWeights = StatBoostRarityWeights;

	AbilityRarityWeights.Add(EGSUpgradeRarity::Common, 100.f);
	AbilityRarityWeights.Add(EGSUpgradeRarity::Rare, 10.f);
	AbilityRarityWeights.Add(EGSUpgradeRarity::Epic, 3.f);
	AbilityRarityWeights.Add(EGSUpgradeRarity::Legendary, 0.5f);

	RarityValueMultipliers.Add(EGSUpgradeRarity::Common, 1.f);
	RarityValueMultipliers.Add(EGSUpgradeRarity::Rare, 1.5f);
	RarityValueMultipliers.Add(EGSUpgradeRarity::Epic, 2.f);
	RarityValueMultipliers.Add(EGSUpgradeRarity::Legendary, 3.f);

	WholeUnitValues.Add(EGSUpgradeRarity::Common, 1.f);
	WholeUnitValues.Add(EGSUpgradeRarity::Epic, 2.f);
	WholeUnitValues.Add(EGSUpgradeRarity::Legendary, 3.f);
}

const TMap<EGSUpgradeRarity, float>& UGSUpgradePool::GetRarityWeights(EGSUpgradeCategory Category) const
{
	switch (Category)
	{
	case EGSUpgradeCategory::Ability: return AbilityRarityWeights;
	case EGSUpgradeCategory::Weapon: return WeaponRarityWeights;
	default: return StatBoostRarityWeights;
	}
}

float UGSUpgradePool::GetRarityWeight(EGSUpgradeCategory Category, EGSUpgradeRarity Rarity) const
{
	const float* Weight = GetRarityWeights(Category).Find(Rarity);
	return Weight ? FMath::Max(*Weight, 0.f) : 0.f;
}

EGSUpgradeRarity UGSUpgradePool::ResolveRarity(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity) const
{
	return HasRarity(Upgrade, Rarity) ? Rarity : EGSUpgradeRarity::Common;
}

EGSUpgradeRarity UGSUpgradePool::RollRarity(FRandomStream& Stream, EGSUpgradeCategory Category) const
{
	const TArray<EGSUpgradeRarity> Rarities = GetAllRarities();
	float Total = 0.f;
	for (const EGSUpgradeRarity Rarity : Rarities)
	{
		Total += GetRarityWeight(Category, Rarity);
	}
	if (Total <= 0.f)
	{
		return EGSUpgradeRarity::Common;
	}
	const float Roll = Stream.FRandRange(0.f, Total);
	float Cumulative = 0.f;
	EGSUpgradeRarity Last = EGSUpgradeRarity::Common;
	for (const EGSUpgradeRarity Rarity : Rarities)
	{
		const float Weight = GetRarityWeight(Category, Rarity);
		if (Weight <= 0.f)
		{
			continue;
		}
		Last = Rarity;
		Cumulative += Weight;
		if (Roll < Cumulative)
		{
			return Rarity;
		}
	}
	return Last;
}

TArray<EGSUpgradeCategory> UGSUpgradePool::GetAllCategories()
{
	TArray<EGSUpgradeCategory> Result;
	const UEnum* Enum = StaticEnum<EGSUpgradeCategory>();
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index) // last entry is the generated _MAX
	{
		Result.Add(static_cast<EGSUpgradeCategory>(Enum->GetValueByIndex(Index)));
	}
	return Result;
}

float UGSUpgradePool::GetRarityValueMultiplier(EGSUpgradeRarity Rarity) const
{
	const float* Multiplier = RarityValueMultipliers.Find(Rarity);
	return Multiplier ? FMath::Max(*Multiplier, 0.f) : 0.f;
}

bool UGSUpgradePool::HasRarity(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity) const
{
	if (!Upgrade)
	{
		return false;
	}
	if (Upgrade->bLegendaryAbility)
	{
		return true; // no rarities: any pick is one charge
	}
	return Upgrade->bWholeUnits ? WholeUnitValues.Contains(Rarity) : RarityValueMultipliers.Contains(Rarity);
}

float UGSUpgradePool::GetPickValue(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity, int32 PicksBefore) const
{
	if (!HasRarity(Upgrade, Rarity))
	{
		return 0.f;
	}
	if (Upgrade->bLegendaryAbility)
	{
		return Upgrade->BaseValue;
	}
	if (Upgrade->bWholeUnits)
	{
		return Upgrade->BaseValue * FMath::Max(WholeUnitValues.FindChecked(Rarity), 0.f);
	}
	const float Multiplier = GetRarityValueMultiplier(Rarity);
	if (Upgrade->Category == EGSUpgradeCategory::Weapon && PicksBefore < FMath::Max(Upgrade->WeaponGivingPicks, 1))
	{
		return Upgrade->BaseValue * FMath::Max(Multiplier - 1.f, 0.f); // this pick gives the weapon (or a gun of it)
	}
	return Upgrade->BaseValue * Multiplier;
}

TArray<EGSUpgradeRarity> UGSUpgradePool::GetAllRarities()
{
	TArray<EGSUpgradeRarity> Result;
	const UEnum* Enum = StaticEnum<EGSUpgradeRarity>();
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index) // last entry is the generated _MAX
	{
		Result.Add(static_cast<EGSUpgradeRarity>(Enum->GetValueByIndex(Index)));
	}
	return Result;
}

namespace
{
	/** One of Candidates by AppearanceWeight (nullptr if none has a weight above 0). */
	UGSUpgradeData* PickByWeight(FRandomStream& Stream, const TArray<UGSUpgradeData*>& Candidates)
	{
		float Total = 0.f;
		for (const UGSUpgradeData* Upgrade : Candidates)
		{
			Total += FMath::Max(Upgrade->AppearanceWeight, 0.f);
		}
		if (Total <= 0.f)
		{
			return nullptr;
		}
		const float Roll = Stream.FRandRange(0.f, Total);
		float Cumulative = 0.f;
		UGSUpgradeData* Last = nullptr;
		for (UGSUpgradeData* Upgrade : Candidates)
		{
			const float Weight = FMath::Max(Upgrade->AppearanceWeight, 0.f);
			if (Weight <= 0.f)
			{
				continue;
			}
			Last = Upgrade;
			Cumulative += Weight;
			if (Roll < Cumulative)
			{
				return Upgrade;
			}
		}
		return Last;
	}
}

TArray<FGSUpgradeChoice> UGSUpgradePool::RollOffer(FRandomStream& Stream, TFunctionRef<int32(const UGSUpgradeData*)> GetStacks) const
{
	// Everything that could be offered right now, split into Legendary abilities and the rest.
	TArray<UGSUpgradeData*> Legendary;
	TArray<UGSUpgradeData*> Normal;
	for (UGSUpgradeData* Upgrade : Upgrades)
	{
		if (Upgrade && Upgrade->Effect && !Legendary.Contains(Upgrade) && !Normal.Contains(Upgrade) && Upgrade->CanAddStack(GetStacks(Upgrade)))
		{
			(Upgrade->bLegendaryAbility ? Legendary : Normal).Add(Upgrade);
		}
	}

	TArray<FGSUpgradeChoice> Offer;
	while (Offer.Num() < ChoicesPerOffer)
	{
		// 1. A Legendary ability (no rarity roll: it always comes as itself).
		const bool bLegendaryCard = LegendaryAbilityChance > 0.f && Stream.FRand() < LegendaryAbilityChance;
		UGSUpgradeData* Upgrade = bLegendaryCard ? PickByWeight(Stream, Legendary) : nullptr;
		EGSUpgradeRarity Rarity = EGSUpgradeRarity::Legendary;

		// 2. Otherwise a stat boost, ability or weapon by AppearanceWeight, then its rarity from its category's weights.
		if (!Upgrade)
		{
			Upgrade = PickByWeight(Stream, Normal);
			if (Upgrade)
			{
				Rarity = ResolveRarity(Upgrade, RollRarity(Stream, Upgrade->Category)); // whole units: Rare -> Common
			}
		}
		// Nothing but Legendary abilities left: one of those.
		if (!Upgrade)
		{
			Upgrade = PickByWeight(Stream, Legendary);
			Rarity = EGSUpgradeRarity::Legendary;
		}
		if (!Upgrade)
		{
			break; // fewer upgrades available than choices
		}
		Offer.Add(FGSUpgradeChoice{ Upgrade, Rarity });
		Legendary.Remove(Upgrade);
		Normal.Remove(Upgrade);
	}
	return Offer;
}
