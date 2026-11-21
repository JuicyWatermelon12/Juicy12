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
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
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

	/** Cash pickups inside this radius (cm) fly to the player. Starting value from GE_InitPlayerStats. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PickupRadius, Category = "Attributes|Economy")
	FGameplayAttributeData PickupRadius;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, PickupRadius)

	/** XP multiplier: XP granted = enemy XPValue x XPGain (items can raise it). Default 1. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_XPGain, Category = "Attributes|Progression")
	FGameplayAttributeData XPGain;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, XPGain)

	/** Meta attribute: damage to apply this execution. Server-only, never replicated; converted to Health loss. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, IncomingDamage)

	/** Server: who dealt the most recent damage (effect causer, else instigator). Used to report killers. */
	AActor* GetLastDamageInstigator() const { return LastDamageInstigator.Get(); }

protected:
	TWeakObjectPtr<AActor> LastDamageInstigator;

	/** Server: tell the attacking player's client about damage they dealt (damage numbers, hit marker). */
	void NotifyDamageDealt(const struct FGameplayEffectModCallbackData& Data, float DamageAmount, bool bCritical, bool bKilled) const;

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

	UFUNCTION()
	void OnRep_PickupRadius(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_XPGain(const FGameplayAttributeData& OldValue);
};
