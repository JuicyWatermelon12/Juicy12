// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GSGameplayEffects.generated.h"

class UTargetTagsGameplayEffectComponent;

/**
 * Base for cooldown effects: HasDuration with the duration from SetByCaller Data.Cooldown
 * (the ability sets it from its settings asset) and a granted cooldown tag.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSCooldownEffect(const FObjectInitializer& ObjectInitializer);

protected:
	void SetCooldownTag(const FGameplayTag& Tag);

	UPROPERTY()
	TObjectPtr<UTargetTagsGameplayEffectComponent> CooldownTagsComponent;
};

/** Grants Cooldown.Slide. */
UCLASS()
class BANDITBALLER_API UGSCooldownEffect_Slide : public UGSCooldownEffect
{
	GENERATED_BODY()

public:
	UGSCooldownEffect_Slide(const FObjectInitializer& ObjectInitializer);
};

/**
 * Grants Cooldown.Dash. Each dash adds a stack (one spent charge); stacks expire one at a time,
 * each after the recharge time. GA_Dash allows activation while stacks < MaxCharges.
 */
UCLASS()
class BANDITBALLER_API UGSCooldownEffect_Dash : public UGSCooldownEffect
{
	GENERATED_BODY()

public:
	UGSCooldownEffect_Dash(const FObjectInitializer& ObjectInitializer);
};

/**
 * Enemy difficulty scaling (GE_EnemyRoundScaling derives from this). Infinite; multiplies MaxHealth by SetByCaller
 * Data.HealthMultiplier and Damage by Data.DamageMultiplier (MultiplyCompound). AGSGameMode applies it when an
 * enemy spawns with magnitudes from UGSDifficultyCurve for the current round.
 */
UCLASS()
class BANDITBALLER_API UGSEnemyRoundScalingEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSEnemyRoundScalingEffect(const FObjectInitializer& ObjectInitializer);
};

/** Base for damage effects: cannot be applied to targets with State.Invulnerable or State.Dead. */
UCLASS()
class BANDITBALLER_API UGSDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGSDamageEffect(const FObjectInitializer& ObjectInitializer);
};
