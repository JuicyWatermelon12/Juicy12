// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GSProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;
class UStaticMeshComponent;

/** Why a projectile went back to the pool. */
enum class EGSProjectileEnd : uint8
{
	HitTarget,
	HitWorld,
	Expired,
	Released
};

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
	/** Damage.Source.* tag added to the damage spec (run stats by source). */
	FGameplayTag DamageSource;
	/** Look and impact effect. */
	TObjectPtr<UStaticMesh> Mesh;
	TObjectPtr<UMaterialInterface> Material;
	FVector Scale = FVector(0.12f);
	TObjectPtr<UNiagaraSystem> ImpactEffect;
	FVector ImpactEffectScale = FVector(0.25f);

	/**
	 * Homing (optional): the projectile turns toward this actor's center by at most HomingTurnRate degrees a second,
	 * keeping its speed. If the target dies or goes away first, it flies on straight. No target = straight from the start.
	 */
	TWeakObjectPtr<AActor> HomingTarget;
	float HomingTurnRate = 0.f;

	/** Cosmetic spin around the projectile's own up axis (degrees a second; a thrown card). */
	float SpinRate = 0.f;

	/** Living targets it passes through without hitting (the Lucky Coin: enemies a flip already hit). Optional. */
	TArray<TWeakObjectPtr<AActor>> IgnoredTargets;

	/**
	 * Optional: called once when the flight ends by hitting a target, hitting the world or expiring (not when a new run
	 * releases it), after the damage and after it went back to the pool. HitActor is the target hit (null otherwise).
	 * The Lucky Coin bounces from here.
	 */
	TFunction<void(EGSProjectileEnd Reason, AActor* HitActor, const FVector& Where)> OnFinished;
};

/**
 * A pooled projectile (the Floating Gun's bullets, Razor Cards' cards). Owned by UGSProjectileSubsystem; never placed.
 *
 * Flies straight at a fixed speed (or homes in on a target, turning at a limited rate, then straight once the target is
 * dead), sweeping a small sphere each tick against world geometry (WorldStatic /
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

	/** The actor it is homing in on (null when it flies straight: no homing, or the target died). */
	AActor* GetHomingTarget() const { return Params.HomingTarget.Get(); }
	/** The actor the current / last flight hit (null if it hit nothing living). */
	AActor* GetLastHitActor() const { return LastHitActor.Get(); }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	/** A hit worth stopping on: a living, non-player target with an ability system, or world geometry. */
	bool HandleHit(const FHitResult& Hit);
	void Finish(EGSProjectileEnd Reason, const FVector& Where);
	/** Turn the velocity toward the homing target (or drop the target if it is dead / gone). */
	void UpdateHoming(float DeltaSeconds);

	TWeakObjectPtr<AActor> LastHitActor;

	FGSProjectileLaunch Params;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	bool bActive = false;
	FVector EndLocation = FVector::ZeroVector;
};
