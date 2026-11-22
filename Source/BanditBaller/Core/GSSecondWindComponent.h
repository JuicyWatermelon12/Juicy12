// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GSSecondWindComponent.generated.h"

class UAbilitySystemComponent;
class UGSCombatSettings;

DECLARE_MULTICAST_DELEGATE_OneParam(FGSOnSecondWindUsedNative, int32 /*SecondWindsLeft*/);

/**
 * The player's Second Winds, on AGSPlayerState (run state).
 *
 * The count is the SecondWinds attribute (Second Wind upgrade). When a hit would take Health to 0, UGSAttributeSet asks
 * CanSave, sets Health to GetRestoredHealth (DA_CombatSettings SecondWindHealthFraction of MaxHealth) instead of 0 and
 * calls UseSecondWind, which:
 * - makes the player invulnerable for SecondWindInvulnerableTime (State.Invulnerable through a timed GE);
 * - removes one stack of the Second Wind upgrade (UGSUpgradeComponent::RemoveStackByEffect), so the attribute, the HUD
 *   icon, the card's "Owned" count and gs.Upgrade.List show how many are left;
 * - fires OnSecondWindUsed for the HUD (flash, sound, banner).
 * Order in the damage path: dash invulnerability -> Floating Shield -> Health -> Second Wind (killing blows only).
 *
 * Authority only. TODO(co-op): not replicated (single-player first); a client's HUD needs the event in M6.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSSecondWindComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSSecondWindComponent();

	/** Second Winds left (the rounded SecondWinds attribute). */
	int32 GetSecondWindsLeft() const;

	/** Server: a Second Wind can save the player from the hit being applied now. */
	bool CanSave() const;

	/** Health the player comes back with: SecondWindHealthFraction x MaxHealth, rounded to whole points (at least 1). */
	float GetRestoredHealth(float MaxHealth) const;

	/** Server: a Second Wind saved the player from a killing blow (Health already set by the attribute set). */
	void UseSecondWind(float HealthBefore, const AActor* Killer);

	/** A new run: no uses counted, no invulnerability left over. */
	void ResetSecondWind();

	/** Second Winds used this run (tests / logs). */
	int32 GetUsesThisRun() const { return UsesThisRun; }

	/** Fires after a Second Wind was used, with how many are left. */
	FGSOnSecondWindUsedNative OnSecondWindUsed;

private:
	UAbilitySystemComponent* GetASC() const;
	const UGSCombatSettings* GetSettings() const;

	FActiveGameplayEffectHandle InvulnerabilityHandle;
	int32 UsesThisRun = 0;
};
