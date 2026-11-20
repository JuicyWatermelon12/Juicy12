// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "GSAbilitySet.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UGSGameplayAbility;

USTRUCT(BlueprintType)
struct FGSAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TSubclassOf<UGSGameplayAbility> Ability;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;
};

USTRUCT(BlueprintType)
struct FGSAbilitySet_GameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float EffectLevel = 1.f;
};

/** Handles of everything an ability set granted, so it can be removed later. */
USTRUCT(BlueprintType)
struct FGSAbilitySetGrantedHandles
{
	GENERATED_BODY()

	void Reset() { AbilitySpecHandles.Reset(); EffectHandles.Reset(); }

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> EffectHandles;

	/** Removes everything this set granted from the ASC (authority only). */
	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);
};

/**
 * Data asset listing abilities and effects to grant together (e.g. the player's startup kit).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Grants the set to the ASC. Authority only. OutHandles may be null. */
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC, FGSAbilitySetGrantedHandles* OutHandles, UObject* SourceObject = nullptr) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Abilities", meta = (TitleProperty = "Ability"))
	TArray<FGSAbilitySet_GameplayAbility> GrantedAbilities;

	/** Applied in order; use for startup stats (e.g. GE_InitPlayerStats). */
	UPROPERTY(EditDefaultsOnly, Category = "Effects", meta = (TitleProperty = "GameplayEffect"))
	TArray<FGSAbilitySet_GameplayEffect> GrantedEffects;
};
