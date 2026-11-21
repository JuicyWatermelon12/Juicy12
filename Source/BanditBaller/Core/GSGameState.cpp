// BanditBaller

#include "GSGameState.h"
#include "Net/UnrealNetwork.h"

void AGSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSGameState, CurrentArea);
	DOREPLIFETIME(AGSGameState, bMarketOpenThisRound);
}

void AGSGameState::SetCurrentArea(EGSArea NewArea)
{
	if (!HasAuthority() || NewArea == CurrentArea)
	{
		return;
	}
	const EGSArea OldArea = CurrentArea;
	CurrentArea = NewArea;
	OnAreaChanged.Broadcast(OldArea, NewArea);
}

void AGSGameState::OnRep_CurrentArea(EGSArea OldArea)
{
	OnAreaChanged.Broadcast(OldArea, CurrentArea);
}
