// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GSUpgradeData.h"
#include "GSDebugLibrary.generated.h"

class AGSEnemyCharacter;
class AGSGameMode;
class USkeletalMesh;
class AGSPlayerController;
class UGSUpgradeComponent;
struct FGSUpgradeChoice;
class UGSEnemyData;
class UGSUpgradeData;
class UGSWeaponData;

/** One living enemy, for list_enemies_alive / tuning dashboards. */
USTRUCT(BlueprintType)
struct FGSEnemyDebugInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString Name;

	/** Enemy data asset name, e.g. DA_Enemy_MeleeBandit. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString EnemyType;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FVector Location = FVector::ZeroVector;

	/** Distance to the first player's pawn (cm), -1 if there is none. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float DistanceToPlayer = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	bool bAttacking = false;
};

/** Result of UGSDebugLibrary::SimulateUpgradeOffers: how often upgrades and rarities came up. */
USTRUCT(BlueprintType)
struct FGSUpgradeOfferSimulation
{
	GENERATED_BODY()

	/** The pool asset that was rolled (empty if none was found). */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString Pool;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 Seed = 0;

	/** Upgrades counted as owned (one pick each) while rolling, e.g. three weapons for "the slots are full". */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FString> Owned;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 Offers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 ChoicesPerOffer = 0;

	/** All choices rolled (Offers x ChoicesPerOffer unless some offers came up short). */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 TotalChoices = 0;

	/** Offers with fewer than ChoicesPerOffer choices (not enough upgrades available). */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 ShortOffers = 0;

	/** Upgrade asset name -> offers it appeared in (at most once per offer). */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TMap<FString, int32> UpgradeOffers;

	/** Rarity name -> choices of that rarity. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TMap<FString, int32> RarityChoices;

	/** Rarity name -> offers with at least one choice of that rarity. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TMap<FString, int32> RarityOffers;

	/** Upgrade asset name -> cards it was on (the same as UpgradeOffers: an offer never repeats an upgrade). */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TMap<FString, int32> UpgradeChoices;

	/** Category ("Stat Boost", "Ability", "Legendary ability", "Weapon") -> cards of that category. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TMap<FString, int32> CategoryChoices;

	/** Category -> offers with at least one card of that category. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TMap<FString, int32> CategoryOffers;

	/** "Ability / Epic" -> cards of that category at that rarity. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TMap<FString, int32> CategoryRarityChoices;
};

/**
 * Tuning/debug helpers shared by the gs.* console commands, the BanditBaller Python MCP toolset and
 * automation tests. Every mutation runs on the server (authority) world. Disabled in Shipping.
 *
 * The world-context versions act on the first local player controller of the game (PIE) world.
 */
UCLASS()
class BANDITBALLER_API UGSDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// --- Lookup ---

	/** The given world if it is a game world, else the first PIE/game world (nullptr if none). */
	static UWorld* FindGameWorld(UWorld* World);

	/** Finds a UGSEnemyData asset by name: "DA_Enemy_MeleeBandit", "MeleeBandit" or "meleebandit". */
	static UGSEnemyData* FindEnemyData(const FString& Name);

	/** Finds a UGSWeaponData asset by name: "DA_Pistol", "Pistol", "BanditCarbine". */
	static UGSWeaponData* FindWeaponData(const FString& Name);

	/** Finds a UGSUpgradeData asset by name: "DA_Upgrade_Damage", "Damage" or "damage". */
	static UGSUpgradeData* FindUpgradeData(const FString& Name);

	/** Names of all upgrade data assets. */
	static TArray<FString> GetUpgradeDataNames();

	/** gs.Upgrade.Grant: add Count picks of the named upgrade at Rarity to PC's player. Returns how many. */
	static int32 GrantUpgradeTo(AGSPlayerController* PC, const FString& UpgradeName, int32 Count = 1, EGSUpgradeRarity Rarity = EGSUpgradeRarity::Common);

	/** gs.Upgrade.List: log + screen every upgrade by category with this run's picks and total value, and the stats they change. */
	static void ListUpgrades(AGSPlayerController* PC);

	/** Log + screen PC's open offer, queued offers and rerolls left (gs.Upgrade.Offer / Choose / Reroll). */
	static void PrintUpgradeOffer(AGSPlayerController* PC);

	/**
	 * Roll Count offers from the project's upgrade pool (DA_UpgradePool) with a private stream seeded by Seed, as for a
	 * fresh run with no picks, and count how often each upgrade and rarity comes up. Never touches the run stream and
	 * needs no PIE. Used by the MCP tool simulate_upgrade_offers.
	 */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug")
	static FGSUpgradeOfferSimulation SimulateUpgradeOffers(int32 Count, int32 Seed);

	/**
	 * SimulateUpgradeOffers with the named upgrades ("FloatingGun", "DA_Upgrade_Dynamite", …) counted as owned, one pick
	 * each, for every offer: e.g. three weapons to see the offers once the weapon slots are full. Unknown names are
	 * skipped with a warning (and left out of Result.Owned).
	 */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug")
	static FGSUpgradeOfferSimulation SimulateUpgradeOffersOwning(int32 Count, int32 Seed, const TArray<FString>& OwnedUpgrades);

	/**
	 * Placeholder pipeline check (Prompt 56): one line per bone of the skeletal mesh's own reference pose, local
	 * (parent-space) translation, rotation and scale, plus the component-space location; MaxBones 0 = all. Compare a
	 * Blender export against the mesh it replaces (e.g. SKM_Quinn_Simple) to catch scale or axis errors on the bones.
	 * No PIE needed (MCP: BanditBallerTools.describe_ref_pose).
	 */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug")
	static FString DescribeRefPose(USkeletalMesh* Mesh, const TArray<FName>& Bones, int32 MaxBones = 0);

	/**
	 * Debug (cvar gs.Debug.AutoPickUpgrades 1): a frame later, take every open upgrade offer by a fixed policy, for
	 * playtests driven from the console: an unowned weapon while a slot is free, else an owned weapon, else the rarest
	 * card (the first one on a tie). An offer with no weapon and only Common cards is rerolled first while rerolls last.
	 * Called by AGSGameMode whenever an offer is open; does nothing while the cvar is 0.
	 */
	static void ScheduleAutoPickUpgrades(AGSGameMode* GameMode);

	/** The auto-pick policy for one offer: the card index to take, or INDEX_NONE to reroll (bCanReroll). */
	static int32 ChooseUpgradeForAutoPick(const TArray<FGSUpgradeChoice>& Offer, const UGSUpgradeComponent* Upgrades, int32 MaxWeapons, bool bCanReroll);

	/** Names of all enemy / weapon data assets (for help text and errors). */
	static TArray<FString> GetEnemyDataNames();
	static TArray<FString> GetWeaponDataNames();

	// --- Server-side operations on a specific player ---

	/** Spawns Count enemies around the player at locations from UGSSpawnLocationFinder (DA_SpawnSettings). */
	static TArray<AGSEnemyCharacter*> SpawnEnemiesAround(AGSPlayerController* PC, UGSEnemyData* EnemyData, int32 Count);

	/** Kills every living enemy through GE_Damage (credited to PC's pawn; they drop cash and XP orbs). Returns how many. */
	static int32 KillAllEnemies(AGSPlayerController* PC);

	/** Turns damage immunity (State.Invulnerable) on/off for PC's player. */
	static bool SetGodMode(AGSPlayerController* PC, bool bEnabled);

	static bool GiveWeaponTo(AGSPlayerController* PC, UGSWeaponData* WeaponData);

	/** Sets the base value of a UGSAttributeSet attribute (Health, MaxHealth, MoveSpeed, Damage, FireRate, PickupRadius, XPGain). */
	static bool SetAttributeOn(AGSPlayerController* PC, const FString& AttributeName, float Value);

	static TMap<FString, float> GetAttributesOf(const AGSPlayerController* PC);

	/** Server entry point for the console commands: Command is "Spawn", "God", "KillAll", "GiveWeapon" or "SetAttr". */
	static void ExecuteDebugCommand(AGSPlayerController* PC, const FString& Command, const TArray<FString>& Args);

	// --- World-context versions (first local player); used by Python / Blueprint ---

	/** Spawn Count enemies of EnemyType around the first player. Returns the spawned actors. */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug", meta = (WorldContext = "WorldContextObject"))
	static TArray<AGSEnemyCharacter*> SpawnEnemies(const UObject* WorldContextObject, const FString& EnemyType, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug", meta = (WorldContext = "WorldContextObject"))
	static int32 KillAll(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug", meta = (WorldContext = "WorldContextObject"))
	static bool SetGod(const UObject* WorldContextObject, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug", meta = (WorldContext = "WorldContextObject"))
	static bool GiveWeapon(const UObject* WorldContextObject, const FString& WeaponName);

	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug", meta = (WorldContext = "WorldContextObject"))
	static bool SetPlayerAttribute(const UObject* WorldContextObject, const FString& AttributeName, float Value);

	/** Current values of the first player's attributes, plus "Ammo" and "God" (0/1). */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug", meta = (WorldContext = "WorldContextObject"))
	static TMap<FString, float> GetPlayerAttributes(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Debug", meta = (WorldContext = "WorldContextObject"))
	static TArray<FGSEnemyDebugInfo> ListEnemiesAlive(const UObject* WorldContextObject);

	/** First player controller of the world as AGSPlayerController. */
	static AGSPlayerController* GetFirstPlayer(const UObject* WorldContextObject);
};
