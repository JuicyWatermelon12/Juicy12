// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSProfileSubsystem.generated.h"

class UGSProfileSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSOnProfileCashChangedSignature, int32, NewCash, int32, Delta);

/**
 * Owns the permanent cash balance (and later other permanent progression). Lives on the GameInstance, so it survives
 * runs, deaths and map travel; the balance is saved in UGSProfileSaveGame.
 *
 * Every cash change goes through AddCash / TrySpendCash. Nothing else stores cash (AGSPlayerState has none); the HUD
 * listens to OnCashChanged. Cash earned in the current run is tracked separately in UGSRunStats.
 *
 * Load: on game start (Initialize). Save: at each RoundClear and at run end (player death), both from AGSGameMode, on
 * quit (Deinitialize), and on demand (Save, gs.Profile.Save). Not on every change.
 *
 * Slot "GSProfile"; while automation tests run, "GSProfile_AutomationTest" so tests never touch the real profile.
 *
 * Co-op: this is the local machine's balance. On a listen server only the host's own pickups are credited here;
 * remote players' pickups are not credited yet (TODO(co-op), see AGSGameMode::AwardCash).
 */
UCLASS()
class BANDITBALLER_API UGSProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* DefaultSlotName;
	static const TCHAR* AutomationSlotName;

	//~ USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** The profile subsystem of the world's GameInstance (nullptr without one). */
	static UGSProfileSubsystem* Get(const UObject* WorldContextObject);

	/** Adds Amount (> 0) to the balance. Reason is logged. Returns false if ignored. */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	bool AddCash(int32 Amount, FName Reason = NAME_None);

	/** Spends Amount (>= 0) if the balance covers it. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	bool TrySpendCash(int32 Amount, FName Reason = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Profile")
	int32 GetCash() const;

	/** Fires whenever the balance changes (Delta < 0 for spending / reset). */
	UPROPERTY(BlueprintAssignable, Category = "Profile")
	FGSOnProfileCashChangedSignature OnCashChanged;

	/** Write the profile to its slot. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	bool Save(FName Reason = NAME_None);

	/** Debug (gs.Profile.Reset): delete the save file and start a fresh profile (cash 0, not saved until the next save). */
	void ResetProfile();

	const FString& GetSlotName() const { return SlotName; }
	const UGSProfileSaveGame* GetSaveData() const { return SaveData; }

	/** Multi-line summary for gs.Profile.Print. */
	FString ToDebugString() const;

private:
	void LoadOrCreate();
	void SetCash(int32 NewCash, FName Reason);

	UPROPERTY()
	TObjectPtr<UGSProfileSaveGame> SaveData;

	FString SlotName;
};
