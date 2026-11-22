// BanditBaller

#include "GSUpgradePool.h"
#include "GameplayEffect.h"

UGSUpgradePool::UGSUpgradePool()
{
	RarityWeights.Add(EGSUpgradeRarity::Common, 100.f);
	RarityWeights.Add(EGSUpgradeRarity::Rare, 20.f);
	RarityWeights.Add(EGSUpgradeRarity::Epic, 6.f);
	RarityWeights.Add(EGSUpgradeRarity::Legendary, 1.f);

	RarityValueMultipliers.Add(EGSUpgradeRarity::Common, 1.f);
	RarityValueMultipliers.Add(EGSUpgradeRarity::Rare, 1.5f);
	RarityValueMultipliers.Add(EGSUpgradeRarity::Epic, 2.f);
	RarityValueMultipliers.Add(EGSUpgradeRarity::Legendary, 3.f);

	WholeUnitValues.Add(EGSUpgradeRarity::Common, 1.f);
	WholeUnitValues.Add(EGSUpgradeRarity::Epic, 2.f);
	WholeUnitValues.Add(EGSUpgradeRarity::Legendary, 3.f);
}

float UGSUpgradePool::GetRarityWeight(EGSUpgradeRarity Rarity) const
{
	const float* Weight = RarityWeights.Find(Rarity);
	return Weight ? FMath::Max(*Weight, 0.f) : 0.f;
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

TArray<UGSUpgradeData*> UGSUpgradePool::RollOffer(FRandomStream& Stream, TFunctionRef<int32(const UGSUpgradeData*)> GetStacks) const
{
	// Everything that could be offered right now.
	TArray<UGSUpgradeData*> Candidates;
	for (UGSUpgradeData* Upgrade : Upgrades)
	{
		if (Upgrade && Upgrade->Effect && !Candidates.Contains(Upgrade) && Upgrade->CanAddStack(GetStacks(Upgrade)))
		{
			Candidates.Add(Upgrade);
		}
	}

	const TArray<EGSUpgradeRarity> Rarities = GetAllRarities();
	TArray<UGSUpgradeData*> Offer;
	while (Offer.Num() < ChoicesPerOffer)
	{
		// Weight only the rarities that still have something to offer.
		TArray<TPair<EGSUpgradeRarity, float>> Tiers;
		float TotalWeight = 0.f;
		for (const EGSUpgradeRarity Rarity : Rarities)
		{
			const float Weight = GetRarityWeight(Rarity);
			if (Weight > 0.f && Candidates.ContainsByPredicate([Rarity](const UGSUpgradeData* U) { return U->Rarity == Rarity; }))
			{
				Tiers.Emplace(Rarity, Weight);
				TotalWeight += Weight;
			}
		}
		if (Tiers.Num() == 0)
		{
			break; // fewer upgrades available than choices
		}

		const float Roll = Stream.FRandRange(0.f, TotalWeight);
		EGSUpgradeRarity Picked = Tiers.Last().Key;
		float Cumulative = 0.f;
		for (const TPair<EGSUpgradeRarity, float>& Tier : Tiers)
		{
			Cumulative += Tier.Value;
			if (Roll < Cumulative)
			{
				Picked = Tier.Key;
				break;
			}
		}

		TArray<UGSUpgradeData*> InTier = Candidates.FilterByPredicate([Picked](const UGSUpgradeData* U) { return U->Rarity == Picked; });
		UGSUpgradeData* Choice = InTier[Stream.RandRange(0, InTier.Num() - 1)];
		Offer.Add(Choice);
		Candidates.Remove(Choice);
	}
	return Offer;
}
