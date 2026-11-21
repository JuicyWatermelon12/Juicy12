// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GSUpgradeComponent.generated.h"

class UGSUpgradeData;

/** How many times one upgrade was picked this run. */
USTRUCT(BlueprintType)
struct FGSUpgradeStack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<const UGSUpgradeData> Upgrade;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	int32 Stacks = 0;

	/** The upgrade's stacking effect on the owner's ASC. */
	FActiveGameplayEffectHandle EffectHandle;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnUpgradeStacksChangedSignature, const UGSUpgradeData*, Upgrade, int32, NewStacks);

/**
 * Level-up upgrades picked this run, on AGSPlayerState (per run: cleared by AGSGameMode::StartRun).
 * GrantUpgrade adds one stack of the upgrade's infinite, stacking GE to the owner's ASC and counts it here, for the UI
 * and tests. Authority only.
 * TODO(co-op): the stack list is not replicated (single-player first); replicate it in M6 for clients' UI.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSUpgradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSUpgradeComponent();

	/**
	 * Add one stack of Upgrade (applies its Effect). Fails without authority, an ASC or an Effect, or when the
	 * upgrade's MaxStacks is reached. Source is logged ("Debug", "LevelUp").
	 */
	bool GrantUpgrade(const UGSUpgradeData* Upgrade, const TCHAR* Source = TEXT("Code"));

	/** Stacks of Upgrade picked this run (0 if never). */
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetStacks(const UGSUpgradeData* Upgrade) const;

	/** Every upgrade picked this run, in pick order of first pick. */
	const TArray<FGSUpgradeStack>& GetUpgradeStacks() const { return Stacks; }

	/** Total picks this run across all upgrades. */
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetTotalPicks() const;

	/** Remove every upgrade effect and forget all stacks (new run). Authority only. */
	void ResetUpgrades();

	/** After a stack was added (NewStacks) or on reset (NewStacks 0, once per upgrade that had stacks). */
	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FGSOnUpgradeStacksChangedSignature OnUpgradeStacksChanged;

private:
	UPROPERTY()
	TArray<FGSUpgradeStack> Stacks;
};
