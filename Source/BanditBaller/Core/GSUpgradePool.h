// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/Function.h"
#include "GSUpgradeData.h"
#include "GSUpgradePool.generated.h"

/**
 * The level-up upgrade pool (DA_UpgradePool, set as UpgradePool on BP_GSGameMode): which upgrades can be offered, how
 * likely each rarity is, how many choices an offer has and how many rerolls a run starts with.
 *
 * An offer picks ChoicesPerOffer different upgrades, one at a time: first a rarity by weight, then a random upgrade of
 * that rarity. A rarity with nothing available right now (no upgrades, all at MaxStacks, or all already in this offer)
 * is left out of the weighting, so empty tiers never waste a pick. If fewer upgrades are available than choices, the
 * offer has fewer choices.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSUpgradePool : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSUpgradePool();

	/** Upgrades that can be offered (duplicates and empty entries are ignored). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool")
	TArray<TObjectPtr<UGSUpgradeData>> Upgrades;

	/** Relative chance of each rarity per pick (placeholders: Common 100, Rare 20, Epic 6, Legendary 1). 0 = never. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool")
	TMap<EGSUpgradeRarity, float> RarityWeights;

	/** How much bigger a pick of each rarity is: its value = BaseValue x this (placeholders: 1 / 1.5 / 2 / 3). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pick values")
	TMap<EGSUpgradeRarity, float> RarityValueMultipliers;

	/**
	 * Whole-unit upgrades (bWholeUnits): a pick's value = BaseValue x this (placeholders: Common 1, Epic 2, Legendary
	 * 3). A rarity missing here (Rare) has no version of them.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pick values")
	TMap<EGSUpgradeRarity, float> WholeUnitValues;

	/** Different upgrades per offer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool", meta = (ClampMin = "1"))
	int32 ChoicesPerOffer = 3;

	/** Rerolls the player starts each run with. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool", meta = (ClampMin = "0"))
	int32 RerollsPerRun = 3;

	float GetRarityWeight(EGSUpgradeRarity Rarity) const;

	/** RarityValueMultipliers[Rarity] (0 if missing). */
	float GetRarityValueMultiplier(EGSUpgradeRarity Rarity) const;

	/** False if Upgrade can't come at Rarity: a whole-unit upgrade with no WholeUnitValues entry (Rare). */
	bool HasRarity(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity) const;

	/**
	 * The value of one pick of Upgrade at Rarity when the player already has PicksBefore picks of it:
	 * - Legendary ability: BaseValue (one charge), whatever the rarity;
	 * - whole units: BaseValue x WholeUnitValues[Rarity];
	 * - weapon: BaseValue x (multiplier - 1) for the first WeaponGivingPicks picks (they give the weapon), else
	 *   BaseValue x multiplier;
	 * - otherwise BaseValue x RarityValueMultipliers[Rarity].
	 * 0 if the upgrade has no such rarity (HasRarity).
	 */
	float GetPickValue(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity, int32 PicksBefore) const;

	/**
	 * Roll one offer from Stream. GetStacks returns how many stacks the player already has of an upgrade (for
	 * MaxStacks). Pure apart from Stream: the run stream for real offers, a private stream for simulations.
	 */
	TArray<UGSUpgradeData*> RollOffer(FRandomStream& Stream, TFunctionRef<int32(const UGSUpgradeData*)> GetStacks) const;

	/** Every rarity value, in order (Common, Rare, Epic, Legendary). */
	static TArray<EGSUpgradeRarity> GetAllRarities();
};
