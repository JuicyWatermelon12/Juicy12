// BanditBaller

#include "GSProjectileSubsystem.h"
#include "BanditBaller.h"
#include "Engine/World.h"

bool UGSProjectileSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGSProjectileSubsystem::Deinitialize()
{
	Pool.Reset();
	Active.Reset();
	Super::Deinitialize();
}

AGSProjectile* UGSProjectileSubsystem::CreateProjectile()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	AGSProjectile* Projectile = GetWorld()->SpawnActor<AGSProjectile>(AGSProjectile::StaticClass(), FTransform::Identity, SpawnParams);
	if (Projectile)
	{
		++NumCreated;
	}
	return Projectile;
}

void UGSProjectileSubsystem::Prewarm(int32 Count)
{
	if (bPrewarmed || !GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}
	bPrewarmed = true;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (AGSProjectile* Projectile = CreateProjectile())
		{
			Pool.Add(Projectile);
		}
	}
}

AGSProjectile* UGSProjectileSubsystem::FireProjectile(const FGSProjectileLaunch& Launch)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	AGSProjectile* Projectile = nullptr;
	while (Pool.Num() > 0 && !Projectile)
	{
		Projectile = Pool.Pop(EAllowShrinking::No);
		if (!IsValid(Projectile))
		{
			Projectile = nullptr;
		}
	}
	if (!Projectile)
	{
		Projectile = CreateProjectile(); // pool exhausted: grow
	}
	if (!Projectile)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("[gun] could not create a projectile."));
		return nullptr;
	}
	Active.Add(Projectile);
	++NumFired;
	Projectile->Launch(Launch);
	return Projectile;
}

void UGSProjectileSubsystem::ReleaseProjectile(AGSProjectile* Projectile, EGSProjectileEnd Reason)
{
	if (!Projectile || Active.RemoveSingleSwap(Projectile, EAllowShrinking::No) == 0)
	{
		return;
	}
	switch (Reason)
	{
	case EGSProjectileEnd::HitTarget: ++NumHitTargets; break;
	case EGSProjectileEnd::HitWorld:  ++NumHitWorld; break;
	case EGSProjectileEnd::Expired:   ++NumExpired; break;
	default: break;
	}
	Projectile->Deactivate();
	Pool.Add(Projectile);
}

void UGSProjectileSubsystem::ReleaseAll()
{
	const TArray<TObjectPtr<AGSProjectile>> InFlight = Active;
	for (AGSProjectile* Projectile : InFlight)
	{
		ReleaseProjectile(Projectile, EGSProjectileEnd::Released);
	}
}
