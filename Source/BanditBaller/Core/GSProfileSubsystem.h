// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSProfileSaveGame.h"
#include "GSProfileSubsystem.generated.h"

class AGSPlayerState;

/**
 * Permanent player state (today: cash), saved to disk and kept across runs, deaths and map travel (it lives on the
 * GameInstance). Every cash change goes through here; AGSPlayerState::Cash only mirrors the profile value for
 * replication and the HUD.
 *
 * Co-op: the host's save holds every player's cash, keyed by a stable player ID. Each machine generates its ID once
 * (UGSProfileSaveGame::LocalPlayerId); AGSPlayerController sends it to the host when it starts
 * (ServerSetProfileId), and the host binds that PlayerState to the entry (BindPlayer).
 *
 * Cash changes are server-only. Saves are coalesced (SaveDelay after a change) and flushed on shutdown / SaveNow.
 * Slot: "GSProfile"; "GSProfile_AutomationTest" while automation tests run; -GSProfileSlot=Name overrides (e.g. two
 * game instances on one machine).
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

	static UGSProfileSubsystem* Get(const UObject* WorldContextObject);

	// --- Identity ---

	/** This machine's player ID (distinct per PIE instance in the editor). */
	FGuid GetLocalPlayerId() const;

	static FString IdToKey(const FGuid& PlayerId) { return PlayerId.ToString(EGuidFormats::Digits); }

	/**
	 * Server: tie a PlayerState to a player ID. Creates the entry if new, mirrors its cash onto the PlayerState and
	 * adds any cash the player collected before the ID arrived.
	 */
	void BindPlayer(AGSPlayerState* PlayerState, const FGuid& PlayerId);

	// --- Cash (server only; the only way cash changes) ---

	/** Add Amount (> 0) to the player's permanent cash. Before binding, it is held on the PlayerState and added on bind. */
	bool AddCash(AGSPlayerState* PlayerState, int32 Amount, FName Reason);

	/** Spend Amount (>= 0) if the player can afford it. Fails for unbound players. */
	bool TrySpendCash(AGSPlayerState* PlayerState, int32 Amount, FName Reason);

	/** Debug (gs.Profile.Cash N): set the player's cash. */
	bool SetCash(AGSPlayerState* PlayerState, int32 NewCash, FName Reason);

	/** By player ID (also for players who are not connected). */
	bool AddCashById(const FString& PlayerKey, int32 Amount, FName Reason);
	int32 GetCashById(const FString& PlayerKey) const;

	/** Bound player's cash (pending pre-bind cash included); 0 if unknown. */
	int32 GetCash(const AGSPlayerState* PlayerState) const;

	/** Debug (gs.Profile.Reset): every player's cash back to 0 (entries and the local ID stay). */
	void ResetAllCash(FName Reason);

	// --- Save ---

	const UGSProfileSaveGame* GetSave() const { return Save; }
	const FString& GetSlotName() const { return SlotName; }

	/** Write the save now (otherwise it is written SaveDelay after the last change). */
	bool SaveNow();

	/** Seconds after a change before the save is written (coalesces bursts of cash pickups). */
	float SaveDelay = 1.f;

private:
	FGSProfilePlayerEntry* FindEntry(const FString& PlayerKey);
	const FGSProfilePlayerEntry* FindEntry(const FString& PlayerKey) const;
	bool ChangeCash(const FString& PlayerKey, int32 NewCash, FName Reason);
	void MirrorToBoundPlayers(const FString& PlayerKey) const;
	void RequestSave();
	void LoadOrCreate();

	UPROPERTY()
	TObjectPtr<UGSProfileSaveGame> Save;

	FString SlotName;
	bool bDirty = false;
	FTimerHandle SaveTimer;

	/** PlayerStates bound this session (server), for mirroring changes made by ID. */
	TArray<TWeakObjectPtr<AGSPlayerState>> BoundPlayers;
};
