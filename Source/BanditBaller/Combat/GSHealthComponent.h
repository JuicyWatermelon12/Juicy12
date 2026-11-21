// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSHealthComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

class UGSHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FGSHealthChangedSignature, UGSHealthComponent*, HealthComponent, float, OldHealth, float, NewHealth, AActor*, DamageInstigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSDeathSignature, AActor*, DeadActor, AActor*, Killer);

/**
 * Shared health/death logic for any actor with our ASC + UGSAttributeSet (players, dummies, enemies).
 * Listens to the Health attribute; when it reaches 0 on the server it adds State.Dead, cancels abilities,
 * and reports to AGSGameMode (player death, or OnEnemyKilled for everything else).
 * OnDeathStarted fires on every machine; the owner reacts (ragdoll, reset, ...).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Health")
	static UGSHealthComponent* FindHealthComponent(const AActor* Actor);

	/** Start listening to the given ASC (call after InitAbilityActorInfo). */
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystem);
	void UninitializeFromAbilitySystem();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	/** Server: clear the dead state (e.g. a dummy resetting). Health itself is restored by a GE. */
	void ResetDeath();

	/** Fires on every machine whenever Health changes. Instigator is only known on the server. */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FGSHealthChangedSignature OnHealthChanged;

	/** Fires once per death on every machine. */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FGSDeathSignature OnDeathStarted;

protected:
	/** Non-player owners report OnEnemyKilled to the game mode when they die. */
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	bool bReportEnemyKill = true;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> Killer;

	UFUNCTION()
	void OnRep_IsDead();

private:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void StartDeath(AActor* InKiller);
	void BroadcastDeath();

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	FDelegateHandle HealthChangedHandle;
	bool bDeathBroadcast = false;
};
