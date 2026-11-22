// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSUpgradeData.generated.h"

class UGameplayEffect;
class UTexture2D;

/** How rare a level-up upgrade is (offer weights per rarity live in UGSUpgradePool). Append new rarities at the end. */
UENUM(BlueprintType)
enum class EGSUpgradeRarity : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

/** The three kinds of level-up upgrade (M2c). */
UENUM(BlueprintType)
enum class EGSUpgradeCategory : uint8
{
	/** A bigger number on an existing stat (Damage, Fire Rate, ..., Loaded Dice). */
	StatBoost,
	/** Something new the player can do or that now happens (Extra Jump, ..., Powder Keg; Legendary abilities too). */
	Ability,
	/** An automatic attack (Floating Gun). A run holds at most MaxWeapons of them (Prompt 42). */
	Weapon
};

/**
 * One level-up upgrade (DA_Upgrade_*). A pick has a rarity; its value is BaseValue x the rarity's multiplier in
 * DA_UpgradePool (UGSUpgradePool::GetPickValue). UGSUpgradeComponent adds the values of every pick up and passes the
 * total to Effect (GE_Upgrade_*, a UGSUpgradeEffect) as SetByCaller values. The description shows BaseValue through the
 * {PerStack} placeholder.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSUpgradeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	/** Shown on the choice card. "{PerStack}" is replaced by the effect's per-stack value, e.g. "+{PerStack}% Damage" -> "+10% Damage". */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = true))
	FText Description;

	/**
	 * Optional card text for the next pick, by how many stacks the player already owns: entry 0 with none, entry 1 with
	 * one, ...; past the end, the last entry. Empty = Description for every pick. The same placeholders work.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = true))
	TArray<FText> DescriptionsByOwnedStacks;

	/** Placeholder art for now. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Stat Boost, Ability or Weapon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	EGSUpgradeCategory Category = EGSUpgradeCategory::StatBoost;

	/**
	 * A Legendary ability (Second Wind): no rarities, every pick adds BaseValue (one charge). From Prompt 42 a card has a
	 * small fixed chance to be one.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (EditCondition = "Category == EGSUpgradeCategory::Ability"))
	bool bLegendaryAbility = false;

	/** Relative chance to be the upgrade on a card within its category (used by the offer roll from Prompt 42). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0"))
	float AppearanceWeight = 1.f;

	/**
	 * The value of one Common pick, in the effect's units: a fraction of the base for percent upgrades (0.1 = +10%
	 * Damage), an amount otherwise (1 air jump, 0.05 crit chance, 1 point of weapon power).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0"))
	float BaseValue = 1.f;

	/**
	 * Comes in whole units (Extra Jump, Dash Charge, ...): a pick gives BaseValue x DA_UpgradePool WholeUnitValues
	 * (+1 / +2 / +3 at Common / Epic / Legendary) and there is no Rare version.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	bool bWholeUnits = false;

	/**
	 * Weapons: how many first picks give the weapon itself (the Floating Gun's first two picks give its two guns). Each
	 * of them adds (rarity multiplier - 1) x BaseValue power, so a Common one adds none; later picks add the multiplier.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "1", EditCondition = "Category == EGSUpgradeCategory::Weapon"))
	int32 WeaponGivingPicks = 1;

	/**
	 * What a value of this upgrade means, for the card's pick line and its "Now:" line. "{Value}" is the value as shown
	 * (x DisplayScale); plurals work: "+{Value} {Value}|plural(one=dash charge,other=dash charges)", "+{Value}% damage".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	FText ValueText;

	/**
	 * Shown value = value x this. 0 = automatic: 100 for percent upgrades (0.15 -> 15), 1 otherwise. Loaded Dice 100
	 * (0.05 crit chance -> 5%), Powder Keg 3 (1 stack -> DA_CombatSettings ChancePerStack 3%), Floating Gun 10 (1 power
	 * -> DA_FloatingGunSettings +10%); the tests check those two match their settings.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0"))
	float DisplayScale = 0.f;

	/** Infinite effect, applied once and updated with the total of every pick (UGSUpgradeEffect subclasses). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TSubclassOf<UGameplayEffect> Effect;

	/** Most picks allowed per run; 0 = no cap. (Removed in Prompt 44.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0"))
	int32 MaxStacks = 0;

	/** True if one more pick is allowed on top of CurrentStacks picks. */
	bool CanAddStack(int32 CurrentStacks) const { return MaxStacks <= 0 || CurrentStacks < MaxStacks; }

	/** True if Effect's values are fractions of the base (Multiply (Additive) on Data.UpgradeMultiplier): 0.1 = +10%. */
	bool IsPercentUpgrade() const;

	/** DisplayScale, or the automatic one (100 for percent upgrades, 1 otherwise). */
	float GetDisplayScale() const { return DisplayScale > 0.f ? DisplayScale : (IsPercentUpgrade() ? 100.f : 1.f); }

	/** Value as shown to the player (x GetDisplayScale, rounded to 0.1): 0.15 on Damage -> 15, 1.5 on Powder Keg -> 4.5. */
	float ToDisplayValue(float Value) const { return FMath::RoundToFloat(Value * GetDisplayScale() * 10.f) / 10.f; }

	/** ValueText with Value filled in ("+15% damage", "+2 dash charges"); the raw number if ValueText is empty. */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	FText FormatValueText(float Value) const;

	/** "+15%", "+2", "+0.075" for logs and the console. */
	FString FormatValue(float Value) const;

	/** One Common pick's value as shown to the player (BaseValue x the display scale; 0.1 on a percent upgrade -> 10). */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	float GetPerStackValue() const { return ToDisplayValue(BaseValue); }

	/**
	 * The card text for the next pick when the player owns OwnedStacks (DescriptionsByOwnedStacks, else Description),
	 * with {PerStack} filled in ("+10% Reload Speed").
	 */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	FText GetFormattedDescription(int32 OwnedStacks = 0) const;

	/** "Common" / "Rare". */
	static FString GetRarityName(EGSUpgradeRarity InRarity);

	/** "Stat Boost" / "Ability" / "Legendary ability" / "Weapon". */
	FString GetCategoryName() const;

	/** Parses "Common", "rare", "E", "L", ... False if Text names no rarity. */
	static bool ParseRarity(const FString& Text, EGSUpgradeRarity& OutRarity);
};
