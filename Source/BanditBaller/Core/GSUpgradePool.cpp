// BanditBaller

#include "GSUpgradePool.h"
#include "GameplayEffect.h"

UGSUpgradePool::UGSUpgradePool()
{
	RarityWeights.Add(EGSUpgradeRarity::Common, 100.f);
	RarityWeights.Add(EGSUpgradeRarity::Rare, 20.f);
	RarityWeights.Add(EGSUpgradeRarity::Epic, 6.f);
	RarityWeights.Add(EGSUpgradeRarity::Legendary, 1.f);
}

float UGSUpgradePool::GetRarityWeight(EGSUpgradeRarity Rarity) const
{
	const float* Weight = RarityWeights.Find(Rarity);
	return Weight ? FMath::Max(*Weight, 0.f) : 0.f;
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
