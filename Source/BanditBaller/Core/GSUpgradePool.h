// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/Function.h"
#include "GSUpgradeData.h"
#include "GSUpgradePool.generated.h"

/** One card of an offer: the upgrade and the rarity rolled for this card. */
USTRUCT(BlueprintType)
struct FGSUpgradeChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UGSUpgradeData> Upgrade;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	EGSUpgradeRarity Rarity = EGSUpgradeRarity::Common;

	bool operator==(const FGSUpgradeChoice& Other) const { return Upgrade == Other.Upgrade && Rarity == Other.Rarity; }
};

/**
 * The level-up upgrade pool (DA_UpgradePool, set as UpgradePool on BP_GSGameMode): which upgrades can be offered, the
 * rarity weights per category, how much bigger a rarer pick is, how many choices an offer has and how many rerolls a
 * run starts with.
 *
 * An offer rolls ChoicesPerOffer cards, one at a time, each with a different upgrade (M2c):
 * 1. Roll LegendaryAbilityChance. On a hit the card is a Legendary ability (by AppearanceWeight) not yet in the offer.
 * 2. Otherwise pick a stat boost, ability or weapon not yet in the offer, by AppearanceWeight, then roll its rarity with
 *    its category's weights. A whole-unit upgrade has no Rare: a Rare roll counts as Common.
 * Upgrades at MaxStacks are never offered, and a weapon the player doesn't own only while they own fewer than MaxWeapons
 * weapons. A hit with no Legendary ability left falls back to step 2 (and step 2 with nothing left falls back to a
 * Legendary ability). With nothing available at all the offer is shorter. The first offer of a run has a weapon card.
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

	/** Chance per card (0..1) to be a Legendary ability instead (placeholder 0.008, about the Legendary rate 1 / 127). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0", ClampMax = "1"))
	float LegendaryAbilityChance = 0.008f;

	/** Rarity weights of a Stat Boost card (placeholders: Common 100, Rare 20, Epic 6, Legendary 1). 0 = never. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll")
	TMap<EGSUpgradeRarity, float> StatBoostRarityWeights;

	/** Rarity weights of an Ability card (placeholders: 100 / 10 / 3 / 0.5: strong ability picks are rarer). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll")
	TMap<EGSUpgradeRarity, float> AbilityRarityWeights;

	/** Rarity weights of a Weapon card (placeholders: 100 / 20 / 6 / 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll")
	TMap<EGSUpgradeRarity, float> WeaponRarityWeights;

	/** How much bigger a pick of each rarity is: its value = BaseValue x this (placeholders: 1 / 1.5 / 2 / 3). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pick values")
	TMap<EGSUpgradeRarity, float> RarityValueMultipliers;

	/**
	 * Whole-unit upgrades (bWholeUnits): a pick's value = BaseValue x this (placeholders: Common 1, Epic 2, Legendary
	 * 3). A rarity missing here (Rare) has no version of them.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pick values")
	TMap<EGSUpgradeRarity, float> WholeUnitValues;

	/** Cards per offer (each a different upgrade). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool", meta = (ClampMin = "1"))
	int32 ChoicesPerOffer = 3;

	/** Rerolls the player starts each run with. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool", meta = (ClampMin = "0"))
	int32 RerollsPerRun = 3;

	/**
	 * Most different weapons a run holds (the HUD shows this many weapon slots). A weapon the player doesn't own is only
	 * offered while they own fewer; owned weapons can always be offered, as upgrades. The rifle and pistol don't count.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool", meta = (ClampMin = "0"))
	int32 MaxWeapons = 3;

	/** True if Weapon may be offered to a player with these picks (GetStacks): owned, or a free weapon slot left. */
	bool CanOfferWeapon(const UGSUpgradeData* Weapon, TFunctionRef<int32(const UGSUpgradeData*)> GetStacks) const;

	/** Different weapons in the pool the player owns (GetStacks > 0). */
	int32 CountOwnedWeapons(TFunctionRef<int32(const UGSUpgradeData*)> GetStacks) const;

	/** The rarity weights of Category's cards. */
	const TMap<EGSUpgradeRarity, float>& GetRarityWeights(EGSUpgradeCategory Category) const;

	/** One rarity's weight for Category (0 if missing or negative). */
	float GetRarityWeight(EGSUpgradeCategory Category, EGSUpgradeRarity Rarity) const;

	/** RarityValueMultipliers[Rarity] (0 if missing). */
	float GetRarityValueMultiplier(EGSUpgradeRarity Rarity) const;

	/** False if Upgrade can't come at Rarity: a whole-unit upgrade with no WholeUnitValues entry (Rare). */
	bool HasRarity(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity) const;

	/** The rarity a card of Upgrade gets for a rolled Rarity: a rarity it has no version of counts as Common. */
	EGSUpgradeRarity ResolveRarity(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity) const;

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
	 * Roll one offer from Stream. GetStacks returns how many picks the player already has of an upgrade (for
	 * MaxStacks and MaxWeapons). With bWeaponCard (the first offer of a run) one card is a weapon (by AppearanceWeight,
	 * rarity rolled as usual) at a random position, if any weapon may be offered. Pure apart from Stream: the run
	 * stream for real offers, a private stream for simulations.
	 */
	TArray<FGSUpgradeChoice> RollOffer(FRandomStream& Stream, TFunctionRef<int32(const UGSUpgradeData*)> GetStacks, bool bWeaponCard = false) const;

	/** Roll a rarity for Category's weights (Common if every weight is 0). */
	EGSUpgradeRarity RollRarity(FRandomStream& Stream, EGSUpgradeCategory Category) const;

	/** Every rarity value, in order (Common, Rare, Epic, Legendary). */
	static TArray<EGSUpgradeRarity> GetAllRarities();

	/** Every category, in order (Stat Boost, Ability, Weapon). */
	static TArray<EGSUpgradeCategory> GetAllCategories();
};
