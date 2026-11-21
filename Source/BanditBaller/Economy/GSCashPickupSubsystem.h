// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSCashPickup.h"
#include "GSCashPickupSubsystem.generated.h"

class APlayerController;
class UGSCashSettings;

/**
 * Server-side owner of the enemy-drop pickup pool (one per game world): cash pieces and XP orbs.
 *
 * - SpawnCashDrop: AGSGameMode calls it when an enemy dies. The enemy's CashValue is split into a few pieces
 *   (SplitValue), scattered around the body with the run stream and tossed onto the ground.
 * - SpawnXPDrop: one XP orb worth the enemy's XPValue, tossed the same way. No XP is granted on the kill itself.
 * - Pickups fly to a player once inside that player's PickupRadius attribute (AGSCashPickup) and are collected:
 *   cash through AGSGameMode::AwardCash, XP through AGSGameMode::AwardXPPickup. Uncollected cash expires after
 *   Lifetime (gs.Cash.Lifetime overrides); XP orbs after XPOrbLifetime (0 = never).
 * - VacuumAll (RoundClear) sends every pickup on the ground to the nearest living player; ReleaseAll (new run)
 *   clears them without paying out.
 * Pickups share one pool: created once (PoolPrewarm), then hidden and reused for either kind.
 */
UCLASS()
class BANDITBALLER_API UGSCashPickupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Server: drop TotalValue worth of cash around Origin. Returns the number of pieces spawned. */
	int32 SpawnCashDrop(const UGSCashSettings* Settings, const FVector& Origin, int32 TotalValue);

	/** Server: drop one XP orb worth XPValue near Origin. Returns the orb (nullptr if nothing dropped). */
	AGSCashPickup* SpawnXPDrop(const UGSCashSettings* Settings, const FVector& Origin, int32 XPValue);

	/** How TotalValue is split into piece values (sum == TotalValue, each >= 1; largest first). */
	static void SplitValue(const UGSCashSettings& Settings, int32 TotalValue, TArray<int32>& OutPieces);

	/** Server: send every active piece to the nearest living player. Returns how many were sent. */
	int32 VacuumAll();

	/** Server: return every active piece to the pool without paying anyone (run restart). */
	void ReleaseAll();

	/** Called by AGSCashPickup (server): it reached Player; pays out and returns it to the pool. */
	void CollectPickup(AGSCashPickup* Pickup, APlayerController* Player);

	/** Called by AGSCashPickup (server): back to the pool, uncollected. */
	void ReleasePickup(AGSCashPickup* Pickup, bool bExpired);

	/**
	 * Seconds an uncollected pickup of Kind stays; 0 = forever. Cash: gs.Cash.Lifetime if > 0, else the settings'
	 * Lifetime. XP: the settings' XPOrbLifetime.
	 */
	float GetLifetime(EGSPickupKind Kind = EGSPickupKind::Cash) const;

	/** Every active pickup, both kinds. */
	const TArray<TObjectPtr<AGSCashPickup>>& GetActivePickups() const { return Active; }
	int32 GetNumActive() const { return Active.Num(); }
	int32 GetNumActiveOfKind(EGSPickupKind Kind) const;
	int32 GetNumPooled() const { return Pool.Num(); }

	// Counters since the world started (tests / logs). The cash counters count cash pieces only.
	int32 GetNumCreated() const { return NumCreated; }
	int32 GetNumCollected() const { return NumCollected; }
	int32 GetNumExpired() const { return NumExpired; }
	int32 GetValueCollected() const { return ValueCollected; }
	int32 GetValueExpired() const { return ValueExpired; }
	int32 GetNumXPOrbsDropped() const { return NumXPOrbsDropped; }
	int32 GetNumXPOrbsCollected() const { return NumXPOrbsCollected; }
	int32 GetNumXPOrbsExpired() const { return NumXPOrbsExpired; }

	/** Where the last XP orb was collected (tests: the "+N XP" popup appears there). */
	FVector GetLastXPCollectLocation() const { return LastXPCollectLocation; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	AGSCashPickup* AcquirePickup();
	AGSCashPickup* CreatePickup();
	void Prewarm(int32 Count);
	void PrewarmOnce(const UGSCashSettings& Settings);
	double GetExpireAt(EGSPickupKind Kind) const;

	/** Where a piece tossed from Origin toward Target lands (stops at walls, rests RestHeight above the floor). */
	FVector FindLanding(const UGSCashSettings& Settings, const FVector& Origin, const FVector& Target) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSCashPickup>> Pool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSCashPickup>> Active;

	/** Settings of the last drop (lifetime fallback for pieces that lose their magnet target). */
	UPROPERTY(Transient)
	TObjectPtr<const UGSCashSettings> LastSettings;

	bool bPrewarmed = false;
	int32 NumCreated = 0;
	int32 NumCollected = 0;
	int32 NumExpired = 0;
	int32 ValueCollected = 0;
	int32 ValueExpired = 0;
	int32 NumXPOrbsDropped = 0;
	int32 NumXPOrbsCollected = 0;
	int32 NumXPOrbsExpired = 0;
	FVector LastXPCollectLocation = FVector::ZeroVector;
};
