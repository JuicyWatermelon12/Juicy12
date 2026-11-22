// BanditBaller

#include "GSWeaponComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "GSAttributeSet.h"
#include "GameFramework/Character.h"
#include "GSWeapon.h"
#include "GSWeaponData.h"
#include "Net/UnrealNetwork.h"

UGSWeaponComponent::UGSWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	WeaponActorClass = AGSWeapon::StaticClass();
}

void UGSWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGSWeaponComponent, Slots);
	DOREPLIFETIME(UGSWeaponComponent, CurrentIndex);
}

void UGSWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		for (const UGSWeaponData* WeaponData : StartingWeapons)
		{
			AddWeapon(WeaponData);
		}

		if (Slots.Num() > 0)
		{
			EquipIndex(0);
		}
	}
}

void UGSWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (FGSWeaponSlot& Slot : Slots)
		{
			if (Slot.Weapon)
			{
				Slot.Weapon->Destroy();
			}
		}
		Slots.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

const UGSWeaponData* UGSWeaponComponent::GetCurrentWeaponData() const
{
	return Slots.IsValidIndex(CurrentIndex) ? Slots[CurrentIndex].Data.Get() : nullptr;
}

AGSWeapon* UGSWeaponComponent::GetCurrentWeapon() const
{
	return Slots.IsValidIndex(CurrentIndex) ? Slots[CurrentIndex].Weapon.Get() : nullptr;
}

int32 UGSWeaponComponent::GetCurrentAmmo() const
{
	return Slots.IsValidIndex(CurrentIndex) ? Slots[CurrentIndex].Ammo : 0;
}

int32 UGSWeaponComponent::GetMagazineSize(const UGSWeaponData* WeaponData) const
{
	if (!WeaponData)
	{
		return 0;
	}
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	const FGameplayAttribute Attribute = UGSAttributeSet::GetMagazineSizeMultiplierAttribute();
	const float Multiplier = ASC && ASC->HasAttributeSetForAttribute(Attribute) ? ASC->GetNumericAttribute(Attribute) : 1.f;
	// Rounded up; the small margin keeps float noise on an exact product (30 x 1.6 = 48) from adding a round.
	return FMath::Max(1, FMath::CeilToInt(WeaponData->MagazineSize * Multiplier - 1e-3f));
}

int32 UGSWeaponComponent::GetCurrentMagazineSize() const
{
	return GetMagazineSize(GetCurrentWeaponData());
}

bool UGSWeaponComponent::IsCurrentMagazineFull() const
{
	const UGSWeaponData* Data = GetCurrentWeaponData();
	return !Data || GetCurrentAmmo() >= GetMagazineSize(Data);
}

bool UGSWeaponComponent::AddWeapon(const UGSWeaponData* WeaponData)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !WeaponData || Slots.Num() >= MaxWeapons)
	{
		return false;
	}

	FGSWeaponSlot NewSlot;
	NewSlot.Data = WeaponData;
	NewSlot.Ammo = GetMagazineSize(WeaponData);

	if (WeaponActorClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.Instigator = Cast<APawn>(Owner);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AGSWeapon* Weapon = GetWorld()->SpawnActor<AGSWeapon>(WeaponActorClass, Owner->GetActorTransform(), Params))
		{
			Weapon->InitializeWeapon(WeaponData);

			if (const ACharacter* Character = Cast<ACharacter>(Owner))
			{
				Weapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->AttachSocket);
				Weapon->SetActorRelativeTransform(WeaponData->MeshTransform);
			}

			NewSlot.Weapon = Weapon;
		}
	}

	Slots.Add(NewSlot);
	UpdateWeaponVisibility();
	NotifyChanged();
	return true;
}

bool UGSWeaponComponent::GiveWeapon(const UGSWeaponData* WeaponData)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !WeaponData)
	{
		return false;
	}

	// Already owned: equip and refill.
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index].Data == WeaponData)
		{
			EquipIndex(Index);
			RefillCurrentMagazine();
			return true;
		}
	}

	// Full: drop the equipped weapon to make room.
	if (Slots.Num() >= MaxWeapons && Slots.IsValidIndex(CurrentIndex))
	{
		if (Slots[CurrentIndex].Weapon)
		{
			Slots[CurrentIndex].Weapon->Destroy();
		}
		Slots.RemoveAt(CurrentIndex);
		CurrentIndex = INDEX_NONE;
	}

	if (!AddWeapon(WeaponData))
	{
		return false;
	}
	EquipIndex(Slots.Num() - 1);
	return true;
}

void UGSWeaponComponent::EquipIndex(int32 NewIndex)
{
	if (!GetOwner()->HasAuthority() || !Slots.IsValidIndex(NewIndex))
	{
		return;
	}

	CurrentIndex = NewIndex;
	UpdateWeaponVisibility();
	NotifyChanged();
}

void UGSWeaponComponent::CycleWeapon()
{
	if (Slots.Num() > 1)
	{
		EquipIndex((CurrentIndex + 1) % Slots.Num());
	}
}

bool UGSWeaponComponent::ConsumeAmmo()
{
	if (!GetOwner()->HasAuthority() || !Slots.IsValidIndex(CurrentIndex) || Slots[CurrentIndex].Ammo <= 0)
	{
		return false;
	}

	--Slots[CurrentIndex].Ammo;
	NotifyChanged();
	return true;
}

void UGSWeaponComponent::RefillCurrentMagazine()
{
	const UGSWeaponData* Data = GetCurrentWeaponData();
	if (!GetOwner()->HasAuthority() || !Data)
	{
		return;
	}

	Slots[CurrentIndex].Ammo = GetMagazineSize(Data);
	NotifyChanged();
}

void UGSWeaponComponent::OnRep_Slots()
{
	UpdateWeaponVisibility();
	NotifyChanged();
}

void UGSWeaponComponent::OnRep_CurrentIndex()
{
	UpdateWeaponVisibility();
	NotifyChanged();
}

void UGSWeaponComponent::UpdateWeaponVisibility()
{
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (AGSWeapon* Weapon = Slots[Index].Weapon)
		{
			Weapon->SetActorHiddenInGame(Index != CurrentIndex);
		}
	}
}

void UGSWeaponComponent::NotifyChanged()
{
	OnWeaponStateChanged.Broadcast();
}
