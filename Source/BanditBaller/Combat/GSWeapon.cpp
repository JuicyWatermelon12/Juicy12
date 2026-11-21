// BanditBaller

#include "GSWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "GSWeaponData.h"
#include "Net/UnrealNetwork.h"

const FName AGSWeapon::MuzzleSocketName(TEXT("Muzzle"));

AGSWeapon::AGSWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(true);
	RootComponent = Mesh;
}

void AGSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSWeapon, WeaponData);
}

void AGSWeapon::InitializeWeapon(const UGSWeaponData* InWeaponData)
{
	WeaponData = InWeaponData;
	ApplyWeaponData();
}

void AGSWeapon::OnRep_WeaponData()
{
	ApplyWeaponData();
}

void AGSWeapon::ApplyWeaponData()
{
	if (WeaponData)
	{
		Mesh->SetStaticMesh(WeaponData->Mesh);
	}
}

FVector AGSWeapon::GetMuzzleLocation() const
{
	if (Mesh->DoesSocketExist(MuzzleSocketName))
	{
		return Mesh->GetSocketLocation(MuzzleSocketName);
	}

	const FVector LocalOffset = WeaponData ? WeaponData->MuzzleOffset : FVector::ZeroVector;
	return Mesh->GetComponentTransform().TransformPosition(LocalOffset);
}
