// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GSTargetDummy.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UGSAttributeSet;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

/**
 * Shootable practice target with its own ASC + UGSAttributeSet.
 * Logs damage taken, goes State.Dead at 0 health, then resets after ResetDelay.
 */
UCLASS()
class BANDITBALLER_API AGSTargetDummy : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSTargetDummy();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dummy")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGSAttributeSet> AttributeSet;

	/** Sets starting Health/MaxHealth (e.g. GE_InitPlayerStats). */
	UPROPERTY(EditDefaultsOnly, Category = "Dummy")
	TSubclassOf<UGameplayEffect> InitEffect;

	/** Seconds after "dying" before health is restored. */
	UPROPERTY(EditDefaultsOnly, Category = "Dummy", meta = (ClampMin = "0", Units = "s"))
	float ResetDelay = 3.f;

private:
	void ApplyInitEffect();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void ResetDummy();

	FTimerHandle ResetTimer;
};
