// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSAttributeSet.generated.h"

/**
 * Core character attributes. Values are initialized and modified only through GameplayEffects
 * (starting values come from GE_InitPlayerStats, not C++).
 * Lives on the owner of the ASC (AGSPlayerState for players).
 */
UCLASS()
class BANDITBALLER_API UGSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSAttributeSet();

	//~ UAttributeSet
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Attributes|Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Damage, Category = "Attributes|Combat")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, Damage)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireRate, Category = "Attributes|Combat")
	FGameplayAttributeData FireRate;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, FireRate)

	/** Meta attribute: damage to apply this execution. Server-only, never replicated; converted to Health loss. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, IncomingDamage)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Damage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_FireRate(const FGameplayAttributeData& OldValue);
};
