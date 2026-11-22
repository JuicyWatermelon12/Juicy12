// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSBarbedWireComponent.generated.h"

class AGSCharacter;
class AGSEnemyCharacter;
class UGSBarbedWireSettings;
class UInstancedStaticMeshComponent;

/**
 * Barbed Wire (a weapon, M2c): on the player's character. While the player owns it (the BarbedWireLevel attribute, the
 * number of Barbed Wire picks, is 1 or more) a ring lies on the ground around the player and follows them. Every
 * TickInterval it damages every living enemy inside it: an enemy is inside when its center is within the radius of the
 * player's center, measured flat, and no more than MaxHeightDifference above or below.
 *
 * Follows the rules for every weapon:
 * - Damage = DA_BarbedWireSettings Damage x the power bonus x the player's Damage attribute; every enemy hit rolls its
 *   own critical hit (Loaded Dice). Piercing Rounds doesn't apply.
 * - Its kills are the player's (GE_Damage from the player's ASC).
 * - Exact schedule: ticks are due every TickInterval from the first one, a long frame runs every tick it owes, and
 *   nothing is banked while it can't act (dead or in a safe zone; a paused game doesn't tick it).
 * - No collision (the ring is a visual of instanced segments); a new run (BarbedWireLevel 0) removes it.
 * - Weapon power (BarbedWirePower): DamageBonusPerPower and RadiusBonusPerPower per point; the ring is redrawn at the
 *   new radius.
 *
 * Settings come from AGSGameMode (DA_BarbedWireSettings), so there is no ring in the hub.
 * TODO(co-op): server only, the ring and its hits aren't replicated (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSBarbedWireComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSBarbedWireComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded BarbedWireLevel attribute (Barbed Wire picks); 1 or more = owned. */
	int32 GetLevel() const;

	/** The BarbedWirePower attribute (weapon power; fractions count). */
	float GetPower() const;

	const UGSBarbedWireSettings* GetSettings() const;

	/** Radius and damage multiplier at the current power. */
	float GetRadius() const;
	float GetDamageMultiplier() const;

	/** Damage one tick deals to one enemy now, before a crit (Damage x power bonus x the player's Damage). */
	float GetTickDamage() const;

	/** The ring's center on the ground (under the player). */
	FVector GetRingCenter() const;

	/** Is this enemy inside the ring now? */
	bool IsInside(const AGSEnemyCharacter* Enemy) const;

	/** It would damage enemies now: owned, settings, alive, not in a safe zone, server. */
	bool CanAct() const;

	// --- State for tests / debug ---
	int32 GetTicks() const { return Ticks; }
	int32 GetHitsDealt() const { return HitsDealt; }
	int32 GetLastTickHits() const { return LastTickHits; }
	double GetFirstTickTime() const { return FirstTickTime; }
	int32 GetScheduledTicks() const { return ScheduledTicks; }
	bool IsRingShown() const;
	float GetShownRadius() const { return ShownRadius; }
	UInstancedStaticMeshComponent* GetRing() const { return Ring; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AGSCharacter* GetCharacter() const;
	void UpdateRing();
	void BuildRing(float Radius);
	void DamageTick();

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Ring;

	double NextTickTime = 0.0;
	bool bActive = false;
	double FirstTickTime = 0.0;
	/** Ticks since FirstTickTime on the current schedule (it restarts after a pause in acting). */
	int32 ScheduledTicks = 0;
	int32 Ticks = 0;
	int32 HitsDealt = 0;
	int32 LastTickHits = 0;
	float ShownRadius = 0.f;
	int32 LoggedLevel = 0;
	float LoggedPower = 0.f;
};
