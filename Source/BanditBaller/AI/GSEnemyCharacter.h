// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GSEnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UGSAttributeSet;
class UGSEnemyData;
class UGSHealthComponent;
class UMaterialInstanceDynamic;
class UWidgetComponent;
struct FOnAttributeChangeData;

/**
 * Base enemy. Owns its ASC + UGSAttributeSet, initialized from UGSEnemyData on spawn
 * (stats, mesh, abilities, StateTree). Dies through UGSHealthComponent (OnEnemyKilled).
 */
UCLASS()
class BANDITBALLER_API AGSEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSEnemyCharacter();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	const UGSEnemyData* GetEnemyData() const { return EnemyData; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UGSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/** Server: show/hide the attack wind-up telegraph (replicated to clients). */
	void SetTelegraphing(bool bInTelegraphing);

protected:
	virtual void BeginPlay() override;

	/** Enemy type definition (DA_Enemy_*). Set on the Blueprint or per placed instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UGSEnemyData> EnemyData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGSAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UGSHealthComponent> HealthComponent;

	/** Screen-space health bar; hidden until the enemy takes damage. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UWidgetComponent> HealthBarComponent;

private:
	void ApplyEnemyDataVisuals();
	void InitializeStats();
	void UpdateHealthBar();
	void ApplyTelegraphVisual();

	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void OnHealthChanged(UGSHealthComponent* InHealthComponent, float OldHealth, float NewHealth, AActor* DamageInstigator);

	UFUNCTION()
	void OnDeathStarted(AActor* DeadActor, AActor* Killer);

	UFUNCTION()
	void OnRep_Telegraphing();

	UPROPERTY(ReplicatedUsing = OnRep_Telegraphing)
	bool bTelegraphing = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BodyMaterials;
};
