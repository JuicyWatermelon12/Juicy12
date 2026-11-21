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

/**
 * One level-up upgrade (DA_Upgrade_*). Picking it adds one stack of Effect (an infinite, stacking GE such as
 * GE_Upgrade_Damage) through UGSUpgradeComponent. The per-stack value lives only in the GE; the description shows it
 * through the {PerStack} placeholder.
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

	/** Placeholder art for now. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	EGSUpgradeRarity Rarity = EGSUpgradeRarity::Common;

	/** Infinite, stacking effect; one stack per pick (UGSUpgradeEffect subclasses). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TSubclassOf<UGameplayEffect> Effect;

	/** Most picks allowed per run; 0 = no cap. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0"))
	int32 MaxStacks = 0;

	/** True if one more stack is allowed on top of CurrentStacks. */
	bool CanAddStack(int32 CurrentStacks) const { return MaxStacks <= 0 || CurrentStacks < MaxStacks; }

	/**
	 * Per-stack value of the effect's first static modifier, as shown to the player: a Multiply (Additive) magnitude of
	 * 1.1 gives 10 (percent); an Additive magnitude of 10 gives 10. 0 if the effect has no static modifier.
	 */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	float GetPerStackValue() const;

	/** Description with {PerStack} filled in ("+10% Reload Speed"). */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	FText GetFormattedDescription() const;

	/** "Common" / "Rare". */
	static FString GetRarityName(EGSUpgradeRarity InRarity);
};
