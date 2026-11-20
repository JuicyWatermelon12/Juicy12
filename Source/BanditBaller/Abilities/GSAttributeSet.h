// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSAttributeSet.generated.h"

#define GS_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Core character attributes. Values are initialized and modified only through GameplayEffects
 * (tuning lives in data, not here). No clamping/replication yet.
 */
UCLASS()
class BANDITBALLER_API UGSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	GS_ATTRIBUTE_ACCESSORS(UGSAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	GS_ATTRIBUTE_ACCESSORS(UGSAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MoveSpeed;
	GS_ATTRIBUTE_ACCESSORS(UGSAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Damage;
	GS_ATTRIBUTE_ACCESSORS(UGSAttributeSet, Damage)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData FireRate;
	GS_ATTRIBUTE_ACCESSORS(UGSAttributeSet, FireRate)
};
