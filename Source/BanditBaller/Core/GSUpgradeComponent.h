// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GSUpgradeData.h"
#include "GSUpgradeComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UGSUpgradePool;

/** One pick of an upgrade: its rarity and the value it added. */
USTRUCT(BlueprintType)
struct FGSUpgradePick
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	EGSUpgradeRarity Rarity = EGSUpgradeRarity::Common;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	float Value = 0.f;
};

/** One upgrade picked this run: how many times (Stacks = picks) and the total value of those picks. */
USTRUCT(BlueprintType)
struct FGSUpgradeStack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<const UGSUpgradeData> Upgrade;

	/** Picks of the upgrade (the HUD icon's "xN", the card's "Owned: N"). */
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	int32 Stacks = 0;

	/** Sum of every pick's value: what the attribute gets (Data.UpgradeValue / Data.UpgradeMultiplier = 1 + it). */
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	float TotalValue = 0.f;

	/** Every pick in order. */
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades")
	TArray<FGSUpgradePick> Picks;

	/** The upgrade's effect on the owner's ASC (applied once, updated on every pick). */
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
 * - Picks: GrantUpgrade adds one pick of an upgrade at a rarity. The pick's value comes from UGSUpgradePool::GetPickValue;
 *   the values of every pick add up, and the upgrade's one infinite GE gets the total (and the pick count) as SetByCaller
 *   values: applied on the first pick, updated in place after that.
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
	 * Add one pick of Upgrade at Rarity: its value (UGSUpgradePool::GetPickValue) joins the total that the upgrade's
	 * Effect applies. Fails without authority, an ASC or an Effect, when the upgrade has no such rarity (a whole-unit
	 * upgrade has no Rare) or when its MaxStacks is reached. Source is logged ("Debug", "LevelUp").
	 */
	bool GrantUpgrade(const UGSUpgradeData* Upgrade, EGSUpgradeRarity Rarity, const TCHAR* Source = TEXT("Code"));

	/** One Common pick (what every offered pick is until the per-card rarity roll, Prompt 42). */
	bool GrantUpgrade(const UGSUpgradeData* Upgrade, const TCHAR* Source = TEXT("Code"))
	{
		return GrantUpgrade(Upgrade, EGSUpgradeRarity::Common, Source);
	}

	/**
	 * Take back the last pick of the picked upgrade whose Effect is (a child of) EffectClass: its value comes off the
	 * total and the pick count drops (the entry and its GE go at 0 picks, so the HUD icon and the card's "Owned"
	 * follow). Used when an upgrade is used up (a Second Wind charge). Returns the upgrade, or nullptr if no picked
	 * upgrade has that effect. Authority only.
	 */
	const UGSUpgradeData* RemoveStackByEffect(TSubclassOf<UGameplayEffect> EffectClass, const TCHAR* Reason = TEXT("Code"));

	/** Picks of Upgrade this run (0 if never). */
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetStacks(const UGSUpgradeData* Upgrade) const;

	/** Same as GetStacks. */
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetPicks(const UGSUpgradeData* Upgrade) const { return GetStacks(Upgrade); }

	/** Total value of Upgrade's picks this run (0 if never), in the effect's units (0.25 = +25% on a percent upgrade). */
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	float GetTotalValue(const UGSUpgradeData* Upgrade) const;

	/** Upgrade's entry (picks, total, every pick), or nullptr if it wasn't picked this run. */
	const FGSUpgradeStack* FindEntry(const UGSUpgradeData* Upgrade) const;

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

	/** After a stack was added or used up (NewStacks) or on reset (NewStacks 0, once per upgrade that had stacks). */
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

	/** GetPool(), else the class defaults (pick values work without a GameMode, e.g. in tests). */
	const UGSUpgradePool* GetPoolOrDefault() const;

	/** Apply Entry's effect with its totals, or update the active one in place. False if applying failed. */
	bool ApplyEntryEffect(UAbilitySystemComponent* ASC, FGSUpgradeStack& Entry) const;
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
