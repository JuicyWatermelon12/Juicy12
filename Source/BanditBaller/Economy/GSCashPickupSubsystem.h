// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSCashPickupSubsystem.generated.h"

class AGSCashPickup;
class APlayerController;
class UGSCashSettings;

/**
 * Server-side owner of the cash pickup pool (one per game world).
 *
 * - SpawnCashDrop: AGSGameMode calls it when an enemy dies. The enemy's CashValue is split into a few pieces
 *   (SplitValue), scattered around the body with the run stream and tossed onto the ground.
 * - Pieces fly to a player once inside that player's PickupRadius attribute (AGSCashPickup) and are collected
 *   through AGSGameMode::AwardCash; uncollected ones expire after Lifetime (gs.Cash.Lifetime overrides).
 * - VacuumAll (RoundClear) sends every piece on the ground to the nearest living player.
 * Pickups are pooled: created once (PoolPrewarm), then hidden and reused.
 */
UCLASS()
class BANDITBALLER_API UGSCashPickupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Server: drop TotalValue worth of cash around Origin. Returns the number of pieces spawned. */
	int32 SpawnCashDrop(const UGSCashSettings* Settings, const FVector& Origin, int32 TotalValue);

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

	/** Seconds an uncollected piece stays (gs.Cash.Lifetime if > 0, else the settings' Lifetime). */
	float GetLifetime() const;

	const TArray<TObjectPtr<AGSCashPickup>>& GetActivePickups() const { return Active; }
	int32 GetNumActive() const { return Active.Num(); }
	int32 GetNumPooled() const { return Pool.Num(); }

	// Counters since the world started (tests / logs).
	int32 GetNumCreated() const { return NumCreated; }
	int32 GetNumCollected() const { return NumCollected; }
	int32 GetNumExpired() const { return NumExpired; }
	int32 GetValueCollected() const { return ValueCollected; }
	int32 GetValueExpired() const { return ValueExpired; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	AGSCashPickup* AcquirePickup();
	AGSCashPickup* CreatePickup();
	void Prewarm(int32 Count);

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
};
