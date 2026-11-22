// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GSThrownActor.h"
#include "GSTomahawk.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UGSTomahawkComponent;
class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;

/** Everything one tomahawk needs (AGSTomahawk::Launch). */
struct FGSTomahawkLaunch
{
	FVector Start = FVector::ZeroVector;
	/** Level (horizontal) unit direction out. */
	FVector Direction = FVector::ForwardVector;
	float Distance = 1800.f;
	float Speed = 2000.f;
	float SweepRadius = 30.f;
	float CatchDistance = 100.f;
	float MaxFlightTime = 4.f;
	/** Seconds already flown (a late throw starts that far along). */
	float AlreadyFlown = 0.f;
	/** Damage per hit, already x the power bonus and the player's Damage (a crit is rolled per hit). */
	float Damage = 12.f;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffect;
	FGameplayTag DamageSource;
	/** It comes back to this actor (the player). */
	TWeakObjectPtr<AActor> ReturnTo;
	/** The weapon that threw it (told about every hit). */
	TWeakObjectPtr<UGSTomahawkComponent> Owner;
	TObjectPtr<UStaticMesh> Mesh;
	TObjectPtr<UMaterialInterface> Material;
	FVector Scale = FVector(0.3f, 0.05f, 0.3f);
	float SpinRate = 900.f;
	TObjectPtr<UNiagaraSystem> HitEffect;
	FVector HitEffectScale = FVector(0.1f);
};

/**
 * A thrown tomahawk: flies level in a straight line for Distance, hurting every living enemy it passes through (once
 * each), then flies back to the player (homing on where the player is now) and hurts every enemy it passes on the way
 * back (once each again). World geometry on the way out turns it back early; on the way back it passes through walls so
 * it always comes home. It is caught within CatchDistance of the player, and gone after MaxFlightTime whatever happens.
 * Hits: GE_Damage from the player's ASC with its own crit roll and damage number. Players are never hit. No collision of
 * its own (it sweeps). Pauses with the game (world tick); a new run (UGSThrownSubsystem::ReleaseAll) removes it.
 * TODO(co-op): not replicated (single-player first).
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSTomahawk : public AGSThrownActor
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void DeactivateThrown() override;

	void Launch(const FGSTomahawkLaunch& InLaunch);

	bool IsReturning() const { return bReturning; }
	/** Where it turned back (its farthest point). */
	FVector GetTurnPoint() const { return TurnPoint; }
	bool TurnedAtWall() const { return bTurnedAtWall; }
	int32 GetHitsOut() const { return HitOut.Num(); }
	int32 GetHitsBack() const { return HitBack.Num(); }

private:
	/** Damage Actor if it is a living, non-player target not hit yet on this leg. Returns true if it hit. */
	bool TryHit(AActor* Actor, const FVector& Where, TArray<TWeakObjectPtr<AActor>>& HitThisLeg);
	void Finish();
	void StepOut(float Step);
	void StepBack(float Step);

	FGSTomahawkLaunch Params;
	bool bReturning = false;
	bool bTurnedAtWall = false;
	float Travelled = 0.f;
	float Age = 0.f;
	FVector TurnPoint = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AActor>> HitOut;
	TArray<TWeakObjectPtr<AActor>> HitBack;
};
