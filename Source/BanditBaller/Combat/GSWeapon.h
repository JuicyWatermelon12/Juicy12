// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSWeapon.generated.h"

class UGSWeaponData;
class UStaticMeshComponent;

/**
 * Visual weapon actor attached to the owning character's hand socket.
 * Holds no gameplay state; ammo lives in UGSWeaponComponent.
 */
UCLASS()
class BANDITBALLER_API AGSWeapon : public AActor
{
	GENERATED_BODY()

public:
	AGSWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: assign data and apply the mesh. */
	void InitializeWeapon(const UGSWeaponData* InWeaponData);

	const UGSWeaponData* GetWeaponData() const { return WeaponData; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** World-space muzzle location ("Muzzle" socket if present, else WeaponData->MuzzleOffset). */
	FVector GetMuzzleLocation() const;

	static const FName MuzzleSocketName;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponData)
	TObjectPtr<const UGSWeaponData> WeaponData;

	UFUNCTION()
	void OnRep_WeaponData();

private:
	void ApplyWeaponData();
};
