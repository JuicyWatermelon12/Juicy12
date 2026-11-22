// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GSUpgradeComponent.generated.h"

class UGSUpgradeData;
class UGSUpgradePool;

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

/** What happened to the current upgrade offer. */
UENUM(BlueprintType)
enum class EGSUpgradeOfferChange : uint8
{
	/** A new offer is open: after a level-up, or the next queued one replacing a picked offer. */
	Opened,
	/** The open offer was replaced by a reroll. */
	Rerolled,
	/** No offer is open any more (the last one was picked, or the run reset). */
	Closed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnUpgradeStacksChangedSignature, const UGSUpgradeData*, Upgrade, int32, NewStacks);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSOnUpgradeOfferChangedSignature, EGSUpgradeOfferChange, Change);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSOnRerollsLeftChangedSignature, int32, RerollsLeft);
DECLARE_MULTICAST_DELEGATE_OneParam(FGSOnUpgradeOfferChangedNative, EGSUpgradeOfferChange);
DECLARE_MULTICAST_DELEGATE_OneParam(FGSOnRerollsLeftChangedNative, int32);

/**
 * Level-up upgrades this run, on AGSPlayerState (run state: AGSGameMode::StartRun calls ResetUpgrades).
 *
 * - Stacks: GrantUpgrade adds one stack of an upgrade's infinite, stacking GE and counts it.
 * - Offers: every level-up (the sibling UGSProgressionComponent's OnLevelUp, which fires after the level-up rewards
 *   and heal) queues one offer. The front offer is rolled from UGSUpgradePool with the run stream when it opens, so a
 *   queued offer already respects what was picked in the offer before it. ChooseUpgrade takes one choice (there is no
 *   skip) and opens the next queued offer; RerollOffer re-rolls the open offer for one of this run's rerolls.
 *
 * Authority only. TODO(co-op): offers, rerolls and stacks are not replicated (single-player first); replicate them
 * in M6 for clients' UI, with picks / rerolls through Server RPCs.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSUpgradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSUpgradeComponent();

	//~ UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- Stacks ---

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

	// --- Offers ---

	/**
	 * Queue one offer (a level-up). Opens it at once if no offer is open. Source is logged. Refused once the run is
	 * over (GameOver): a dead player has nothing to pick for, and an offer would pause the game under the run summary.
	 */
	void QueueOffer(const TCHAR* Source);

	/**
	 * Close the open offer and forget the queued ones (the run ended). Upgrades and rerolls are kept (the run summary
	 * shows them). Authority only.
	 */
	void DropOffers(const TCHAR* Reason);

	/** Take choice Index of the open offer (grants it), then open the next queued offer. False if refused. */
	bool ChooseUpgrade(int32 Index);

	/** Replace the open offer with a fresh roll for one reroll. Refused with no open offer or no rerolls left. */
	bool RerollOffer();

	UFUNCTION(BlueprintPure, Category = "Upgrades")
	bool HasOpenOffer() const { return CurrentChoices.Num() > 0; }

	/** The open offer's choices (empty when none is open). */
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	TArray<UGSUpgradeData*> GetCurrentChoices() const;

	/** Offers waiting behind the open one. */
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetQueuedOffers() const { return QueuedOffers; }

	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetRerollsLeft() const { return RerollsLeft; }

	/** New run: remove every upgrade effect, forget all stacks, drop all offers and refill the rerolls. Authority only. */
	void ResetUpgrades();

	/** "Offer: [0] DA_Upgrade_Damage (Common) ... | 1 queued | 3 rerolls" for logs and the console. */
	FString DescribeOffer() const;

	// --- Events (server; single-player = the local machine) ---

	/** After a stack was added (NewStacks) or on reset (NewStacks 0, once per upgrade that had stacks). */
	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FGSOnUpgradeStacksChangedSignature OnUpgradeStacksChanged;

	/** The open offer opened, was rerolled or closed. Read GetCurrentChoices() for the new choices. */
	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FGSOnUpgradeOfferChangedSignature OnUpgradeOfferChanged;

	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FGSOnRerollsLeftChangedSignature OnRerollsLeftChanged;

	/** Native versions of the two events above, for C++ listeners (UI bases, tests); fired at the same time. */
	FGSOnUpgradeOfferChangedNative OnUpgradeOfferChangedNative;
	FGSOnRerollsLeftChangedNative OnRerollsLeftChangedNative;

private:
	UFUNCTION()
	void HandleLevelUp(int32 NewLevel);

	const UGSUpgradePool* GetPool() const;
	TArray<UGSUpgradeData*> RollChoices() const;

	/** Roll queued offers until one has choices (an offer with nothing available is dropped). */
	void OpenNextOffer();

	void SetRerollsLeft(int32 NewRerollsLeft);
	void BroadcastOfferChanged(EGSUpgradeOfferChange Change);

	UPROPERTY()
	TArray<FGSUpgradeStack> Stacks;

	/** The open offer (empty = none). */
	UPROPERTY()
	TArray<TObjectPtr<UGSUpgradeData>> CurrentChoices;

	int32 QueuedOffers = 0;
	int32 RerollsLeft = 0;
};
