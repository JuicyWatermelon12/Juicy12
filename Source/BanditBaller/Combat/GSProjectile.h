// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;
class UStaticMeshComponent;

/** Everything one shot needs (UGSProjectileSubsystem::FireProjectile). */
struct FGSProjectileLaunch
{
	FVector Start = FVector::ZeroVector;
	/** Unit direction of travel. */
	FVector Direction = FVector::ForwardVector;
	float Speed = 6000.f;
	float Lifetime = 0.75f;
	float Radius = 8.f;
	/** Damage per hit, already x the shooter's Damage attribute (a critical hit is rolled when it lands). */
	float Damage = 0.f;
	/** The player whose damage this is (crit chance, effect context: kills are theirs). */
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffect;
	/** Look and impact effect. */
	TObjectPtr<UStaticMesh> Mesh;
	TObjectPtr<UMaterialInterface> Material;
	FVector Scale = FVector(0.12f);
	TObjectPtr<UNiagaraSystem> ImpactEffect;
	FVector ImpactEffectScale = FVector(0.25f);
};

/** Why a projectile went back to the pool. */
enum class EGSProjectileEnd : uint8
{
	HitTarget,
	HitWorld,
	Expired,
	Released
};

/**
 * A pooled projectile (the Floating Gun's; the game's first). Owned by UGSProjectileSubsystem; never placed.
 *
 * Flies straight at a fixed speed, sweeping a small sphere each tick against world geometry (WorldStatic /
 * WorldDynamic) and pawns. It passes through player pawns, dead targets, pickups (no collision) and the map boundary
 * (its own object type isn't queried). The first living target with an ability system takes the damage (GE_Damage from
 * the shooter's ASC, with its own critical-hit roll), then the projectile goes back to the pool; world geometry also
 * stops it, and it expires after its lifetime. Pauses with the game (world tick).
 *
 * Server only. TODO(co-op): not replicated (single-player first); clients need to see projectiles in M6.
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGSProjectile();

	virtual void Tick(float DeltaSeconds) override;

	/** Take it out of the pool and fly. */
	void Launch(const FGSProjectileLaunch& InLaunch);

	/** Back to the pool (hidden, no tick). */
	void Deactivate();

	bool IsActiveProjectile() const { return bActive; }
	FVector GetVelocity() const { return Velocity; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Where the last flight ended (tests). */
	FVector GetEndLocation() const { return EndLocation; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	/** A hit worth stopping on: a living, non-player target with an ability system, or world geometry. */
	bool HandleHit(const FHitResult& Hit);
	void Finish(EGSProjectileEnd Reason, const FVector& Where);

	FGSProjectileLaunch Params;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	bool bActive = false;
	FVector EndLocation = FVector::ZeroVector;
};
