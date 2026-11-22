// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GSThrownActor.h"
#include "GSFirePatch.generated.h"

class AGSEnemyCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;
class UGSMolotovComponent;
class UMaterialInterface;
class UStaticMesh;

/** Everything one patch needs (AGSFirePatch::Ignite). Damage and radius are set when the bottle is thrown. */
struct FGSFirePatchParams
{
	/** The floor point it burns around. */
	FVector Center = FVector::ZeroVector;
	float Radius = 250.f;
	float MaxHeightDifference = 200.f;
	/** Damage per burn to each enemy inside, already x the power bonus and the player's Damage (crits are rolled per burn). */
	float Damage = 4.f;
	float BurnInterval = 0.5f;
	float Duration = 4.f;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffect;
	FGameplayTag DamageSource;
	/** The weapon that made it: asked before every burn whether the player can act, and told about every burn. */
	TWeakObjectPtr<UGSMolotovComponent> Owner;
	TObjectPtr<UStaticMesh> Mesh;
	TObjectPtr<UMaterialInterface> Material;
	float Thickness = 2.f;
	float HeightAboveFloor = 2.f;
};

/**
 * A patch of fire on the ground (the Molotov). It burns on an exact schedule of its own game time from the moment it is lit:
 * the first burn at once, then one every BurnInterval while its age is below Duration (4 s at 0.5 s = 8 burns), then it
 * goes back to the pool. Each burn hits every living enemy whose center is within Radius of the center (flat) and within
 * MaxHeightDifference up or down: GE_Damage from the player's ASC with its own crit roll and damage number. Players are
 * never hit. It ages and burns with the world tick, so it pauses with the game. A burn is skipped while the player can't
 * act (dead, safe zone). No collision; a new run (UGSThrownSubsystem::ReleaseAll) removes it.
 * TODO(co-op): not replicated (single-player first).
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSFirePatch : public AGSThrownActor
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

	/** Light it: placed at the center, sized to the radius, first burn now. */
	void Ignite(const FGSFirePatchParams& InParams);

	/** Whether Enemy stands in the fire. */
	bool IsInside(const AGSEnemyCharacter* Enemy) const;

	const FGSFirePatchParams& GetParams() const { return Params; }
	float GetAge() const { return Age; }
	int32 GetBurns() const { return Burns; }
	int32 GetLastBurnHits() const { return LastBurnHits; }

private:
	void Burn();

	FGSFirePatchParams Params;
	float Age = 0.f;
	int32 Burns = 0;
	int32 LastBurnHits = 0;
};
