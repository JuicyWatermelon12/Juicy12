// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GSDebugLibrary.generated.h"

class AGSEnemyCharacter;
class AGSPlayerController;
class UGSEnemyData;
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

	/** Names of all enemy / weapon data assets (for help text and errors). */
	static TArray<FString> GetEnemyDataNames();
	static TArray<FString> GetWeaponDataNames();

	// --- Server-side operations on a specific player ---

	/** Spawns Count enemies around the player at locations from UGSSpawnLocationFinder (DA_SpawnSettings). */
	static TArray<AGSEnemyCharacter*> SpawnEnemiesAround(AGSPlayerController* PC, UGSEnemyData* EnemyData, int32 Count);

	/** Kills every living enemy through GE_Damage (credited to PC's pawn). Returns how many. */
	static int32 KillAllEnemies(AGSPlayerController* PC);

	/** Turns damage immunity (State.Invulnerable) on/off for PC's player. */
	static bool SetGodMode(AGSPlayerController* PC, bool bEnabled);

	static bool GiveWeaponTo(AGSPlayerController* PC, UGSWeaponData* WeaponData);

	/** Sets the base value of a UGSAttributeSet attribute (Health, MaxHealth, MoveSpeed, Damage, FireRate). */
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
