// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSMolotovComponent.generated.h"

class AGSCharacter;
class AGSEnemyCharacter;
class AGSFirePatch;
class UGSMolotovSettings;

/**
 * Molotov (a weapon, M2c): on the player's character. While the player owns it (the MolotovLevel attribute, the number
 * of Molotov picks, is 1 or more) it throws a bottle every ThrowInterval at the nearest living enemy within Range (no
 * clear line needed: it lobs over cover). The bottle (an AGSLobbedThrow) lands where that enemy stood when it was thrown,
 * after FlightTime, and leaves an AGSFirePatch there: for PatchDuration it burns every enemy standing in it every
 * BurnInterval (the first burn when it lands).
 *
 * Follows the rules for every weapon:
 * - Burn damage = DA_MolotovSettings BurnDamage x the power bonus x the player's Damage attribute (set at the throw, with
 *   the patch radius); every burn rolls its own crit per enemy (Loaded Dice). Piercing Rounds doesn't apply.
 * - Its kills are the player's (cash, XP orb, health orb chance, combo, Leech, Powder Keg, run-stats kills).
 * - Exact schedule (as GA_Fire's held fire, Prompt 31b), for the throws and for each patch's burns; nothing is banked
 *   while no enemy is in range.
 * - It doesn't throw, and patches don't burn, while the player is dead or in a safe zone; a paused game doesn't tick
 *   throws, bottles or patches; the fire never hurts the player; no collision; a new run (MolotovLevel 0,
 *   UGSThrownSubsystem::ReleaseAll) removes the weapon, the bottles in the air and the patches.
 * - Weapon power (MolotovPower): DamageBonusPerPower and RadiusBonusPerPower per point.
 *
 * Settings come from AGSGameMode (DA_MolotovSettings), so nothing is thrown in the hub.
 * TODO(co-op): server only, bottles and patches aren't replicated (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSMolotovComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSMolotovComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded MolotovLevel attribute (Molotov picks); 1 or more = owned. */
	int32 GetLevel() const;

	/** The MolotovPower attribute (weapon power; fractions count). */
	float GetPower() const;

	const UGSMolotovSettings* GetSettings() const;

	/** At the current power. */
	float GetDamageMultiplier() const;
	float GetPatchRadius() const;
	/** Damage per burn: BurnDamage x the power bonus x the player's Damage attribute (before crits). */
	float GetBurnDamage() const;
	double GetThrowInterval() const;

	/** Where bottles leave the player (capsule center + ThrowOffset in the player's facing frame). */
	FVector GetThrowOrigin() const;

	/** The nearest living enemy within Range of the player, or nullptr. No clear line needed. */
	AGSEnemyCharacter* FindTarget() const;

	/** It could throw now if an enemy were in range: owned, settings, alive, not in a safe zone, server. */
	bool CanThrow() const;

	/** Patches burn only while the player can act (alive, not in a safe zone). */
	bool CanBurn() const;

	/** Light a patch at a floor point now, at the current power (what a landing does; also the debug command). */
	AGSFirePatch* IgnitePatch(const FVector& FloorPoint);
	/** Target: the enemy the bottle was thrown at (optional); whether it stood in the fire at the first burn is counted. */
	AGSFirePatch* IgnitePatch(const FVector& FloorPoint, float Radius, float Damage, const AGSEnemyCharacter* Target = nullptr);

	/** Debug (gs.Molotov.Patches): Count patches around the player, 4 m out, at the current power. Returns how many. */
	int32 SpawnPatchesForDebug(int32 Count);

	/** Called by a patch after each burn. */
	void NotifyPatchBurned(int32 Hits);

	/** The patches burning now (from the thrown-actor pool). */
	TArray<AGSFirePatch*> GetActivePatches() const;

	// --- State for tests / debug ---
	int32 GetThrows() const { return Throws; }
	int32 GetPatchesLit() const { return PatchesLit; }
	int32 GetBurns() const { return Burns; }
	int32 GetHitsDealt() const { return HitsDealt; }
	FVector GetLastLanding() const { return LastLanding; }
	AGSEnemyCharacter* GetLastTarget() const { return LastTarget.Get(); }
	/** Whether the last bottle's landing was led ahead of its target (it was moving and LeadFactor > 0). */
	bool WasLastThrowLed() const { return bLastThrowLed; }
	/** Patches whose first burn caught the enemy their bottle was thrown at, and whether the last one did. */
	int32 GetFirstBurnTargetHits() const { return FirstBurnTargetHits; }
	bool DidLastPatchHitTarget() const { return bLastPatchHitTarget; }
	double GetBurstStartTime() const { return BurstStartTime; }
	int32 GetBurstShots() const { return BurstShots; }

private:
	AGSCharacter* GetCharacter() const;
	bool IsInRange(const AGSEnemyCharacter* Enemy) const;
	void UpdateThrowing(float DeltaSeconds);
	void StopThrowing(double Now);
	void ThrowAt(AGSEnemyCharacter* AtTarget, float Lateness);

	bool bTargetInRange = false;
	float TargetRefreshRemaining = 0.f;
	double NextThrowTime = 0.0;
	bool bThrowing = false;
	double BurstStartTime = 0.0;
	int32 BurstShots = 0;
	int32 Throws = 0;
	int32 PatchesLit = 0;
	int32 Burns = 0;
	int32 HitsDealt = 0;
	FVector LastLanding = FVector::ZeroVector;
	TWeakObjectPtr<AGSEnemyCharacter> LastTarget;
	bool bLastThrowLed = false;
	int32 FirstBurnTargetHits = 0;
	bool bLastPatchHitTarget = false;
	int32 LoggedLevel = 0;
	float LoggedPower = 0.f;
};
