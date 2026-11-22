// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSProjectile.h"
#include "GSProjectileSubsystem.generated.h"

/**
 * The projectile pool (one per game world): created once (Prewarm), then hidden and reused. FireProjectile launches
 * one; projectiles call ReleaseProjectile when they hit or expire; ReleaseAll (a new run) clears them in flight.
 * Server only (single-player).
 */
UCLASS()
class BANDITBALLER_API UGSProjectileSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Server: create Count projectiles up front (once). */
	void Prewarm(int32 Count);

	/** Server: launch a projectile. Returns it (nullptr if none could be created). */
	AGSProjectile* FireProjectile(const FGSProjectileLaunch& Launch);

	/** Called by AGSProjectile: back to the pool. */
	void ReleaseProjectile(AGSProjectile* Projectile, EGSProjectileEnd Reason);

	/** Every projectile in flight back to the pool (a new run). */
	void ReleaseAll();

	const TArray<TObjectPtr<AGSProjectile>>& GetActiveProjectiles() const { return Active; }
	int32 GetNumActive() const { return Active.Num(); }
	int32 GetNumPooled() const { return Pool.Num(); }

	// Counters since the world started (tests / logs).
	int32 GetNumCreated() const { return NumCreated; }
	int32 GetNumFired() const { return NumFired; }
	int32 GetNumHitTargets() const { return NumHitTargets; }
	int32 GetNumHitWorld() const { return NumHitWorld; }
	int32 GetNumExpired() const { return NumExpired; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	AGSProjectile* CreateProjectile();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSProjectile>> Pool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSProjectile>> Active;

	bool bPrewarmed = false;
	int32 NumCreated = 0;
	int32 NumFired = 0;
	int32 NumHitTargets = 0;
	int32 NumHitWorld = 0;
	int32 NumExpired = 0;
};
