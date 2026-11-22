// BanditBaller

#include "GSThrownSubsystem.h"
#include "BanditBaller.h"
#include "Engine/World.h"
#include "GSThrownActor.h"

bool UGSThrownSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGSThrownSubsystem::Deinitialize()
{
	Pool.Reset();
	Active.Reset();
	Super::Deinitialize();
}

AGSThrownActor* UGSThrownSubsystem::Acquire(TSubclassOf<AGSThrownActor> Class)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || !Class)
	{
		return nullptr;
	}
	AGSThrownActor* Actor = nullptr;
	for (int32 Index = Pool.Num() - 1; Index >= 0; --Index)
	{
		AGSThrownActor* Candidate = Pool[Index];
		if (!IsValid(Candidate))
		{
			Pool.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Candidate->GetClass() == Class.Get())
		{
			Actor = Candidate;
			Pool.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			break;
		}
	}
	if (!Actor)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<AGSThrownActor>(Class, FTransform::Identity, SpawnParams);
		if (!Actor)
		{
			UE_LOG(LogBanditBaller, Error, TEXT("[thrown] could not spawn a %s."), *GetNameSafe(Class.Get()));
			return nullptr;
		}
		++NumCreated;
	}
	Active.Add(Actor);
	Actor->ActivateThrown();
	return Actor;
}

void UGSThrownSubsystem::Release(AGSThrownActor* Actor)
{
	if (!Actor || Active.RemoveSingleSwap(Actor, EAllowShrinking::No) == 0)
	{
		return;
	}
	Actor->DeactivateThrown();
	Pool.Add(Actor);
}

void UGSThrownSubsystem::ReleaseAll()
{
	const TArray<TObjectPtr<AGSThrownActor>> InPlay = Active;
	for (AGSThrownActor* Actor : InPlay)
	{
		Release(Actor);
	}
}

int32 UGSThrownSubsystem::GetNumActive(TSubclassOf<AGSThrownActor> Class) const
{
	int32 Count = 0;
	for (const AGSThrownActor* Actor : Active)
	{
		Count += (Actor && (!Class || Actor->IsA(Class))) ? 1 : 0;
	}
	return Count;
}
