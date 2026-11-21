// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSWeaponComponent.generated.h"

class AGSWeapon;
class UGSWeaponData;

USTRUCT(BlueprintType)
struct FGSWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<const UGSWeaponData> Data = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AGSWeapon> Weapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 Ammo = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSOnWeaponStateChanged);

/**
 * Character loadout: up to MaxWeapons weapons, per-weapon magazine ammo, and the equipped index.
 * State changes are server-only and replicate to clients.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxWeapons = 2;

	UGSWeaponComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Queries ---

	UFUNCTION(BlueprintPure, Category = "Weapon")
	const UGSWeaponData* GetCurrentWeaponData() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AGSWeapon* GetCurrentWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentIndex() const { return CurrentIndex; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetNumWeapons() const { return Slots.Num(); }

	bool IsCurrentMagazineFull() const;

	// --- Server-only mutations ---

	/** Adds a weapon (spawns and attaches its actor). Returns false if full or invalid. */
	bool AddWeapon(const UGSWeaponData* WeaponData);

	/**
	 * Debug/pickup: equip WeaponData with a full magazine. Re-equips it if already owned, adds it if
	 * there is a free slot, otherwise replaces the equipped weapon. Returns false if invalid.
	 */
	bool GiveWeapon(const UGSWeaponData* WeaponData);

	void EquipIndex(int32 NewIndex);
	void CycleWeapon();

	/** Removes one round from the current magazine. Returns false if empty. */
	bool ConsumeAmmo();

	void RefillCurrentMagazine();

	/** Fired on every machine when weapons, the equipped index, or ammo change. */
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FGSOnWeaponStateChanged OnWeaponStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Granted on spawn, in slot order (max MaxWeapons). */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TObjectPtr<UGSWeaponData>> StartingWeapons;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AGSWeapon> WeaponActorClass;

	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<FGSWeaponSlot> Slots;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentIndex)
	int32 CurrentIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_CurrentIndex();

private:
	void UpdateWeaponVisibility();
	void NotifyChanged();
};
