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
class UStaticMeshComponent;
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

	/** Set the enemy type before BeginPlay (e.g. on a deferred spawn). Ignored once play has begun. */
	void SetEnemyData(UGSEnemyData* InEnemyData);

	/**
	 * Server: spawn an enemy of EnemyData standing on NavLocation (a NavMesh point, e.g. from
	 * UGSSpawnLocationFinder), facing FaceTowards. Used by the wave director and gs.Spawn.
	 */
	static AGSEnemyCharacter* SpawnEnemy(UWorld* World, UGSEnemyData* EnemyData, const FVector& NavLocation, const FVector& FaceTowards, int32 WaveRound = 0);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UGSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/** Server: show/hide the attack wind-up telegraph (replicated to clients). */
	void SetTelegraphing(bool bInTelegraphing);

	/** What enemy shots collide with: world geometry and pawns (players ignore the Visibility channel). */
	static FCollisionObjectQueryParams GetShotObjectQueryParams();

	/** World-space muzzle of the ranged weapon (falls back to chest height in front of the enemy). */
	FVector GetMuzzleLocation() const;

	/** Server -> all: cosmetic shot (muzzle effect, sound, debug line; the impact belongs to the projectile). */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShotFired(FVector_NetQuantize Start, FVector_NetQuantize End, bool bHit);

	virtual void Tick(float DeltaSeconds) override;

	/** The props from EnemyData->MeshAttachments (cap, baton), one component each (tests). */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetMeshAttachmentComponents() const { return MeshAttachmentComponents; }

	/** The gun mesh of a ranged enemy (EnemyData->Weapon->Mesh); no mesh for melee enemies (tests). */
	UStaticMeshComponent* GetWeaponMeshComponent() const { return WeaponMeshComponent; }

	/** The aim laser beam (tests: shown while aiming). */
	UStaticMeshComponent* GetAimLaserComponent() const { return AimLaserComponent; }

	/** The dynamic body materials that carry TelegraphAmount, one per mesh slot (tests). */
	const TArray<TObjectPtr<UMaterialInstanceDynamic>>& GetBodyMaterials() const { return BodyMaterials; }

	bool IsTelegraphing() const { return bTelegraphing; }

	/** The round whose wave this enemy belongs to (0 = not from a wave, e.g. gs.Spawn). Set by UGSWaveDirector. */
	int32 GetWaveRound() const { return WaveRound; }
	void SetWaveRound(int32 Round) { WaveRound = Round; }

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

	/** Ranged enemies: gun mesh from EnemyData->Weapon, attached to its AttachSocket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ranged")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	/** Thin beam shown from the muzzle while aiming (telegraph). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ranged")
	TObjectPtr<UStaticMeshComponent> AimLaserComponent;

private:
	void UpdateAimLaser();
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

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> MeshAttachmentComponents;

	int32 WaveRound = 0;
};
