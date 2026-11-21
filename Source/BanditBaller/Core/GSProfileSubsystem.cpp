// BanditBaller

#include "GSProfileSubsystem.h"
#include "BanditBaller.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GSPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

const TCHAR* UGSProfileSubsystem::DefaultSlotName = TEXT("GSProfile");
const TCHAR* UGSProfileSubsystem::AutomationSlotName = TEXT("GSProfile_AutomationTest");

void UGSProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Tests must never touch the real profile; -GSProfileSlot= lets two game instances on one machine differ.
	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("GSProfileSlot="), Override) && !Override.IsEmpty())
	{
		SlotName = Override;
	}
	else
	{
		SlotName = GIsAutomationTesting ? AutomationSlotName : DefaultSlotName;
	}
	LoadOrCreate();
}

void UGSProfileSubsystem::Deinitialize()
{
	if (bDirty)
	{
		SaveNow();
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetTimerManager().ClearTimer(SaveTimer);
	}
	Super::Deinitialize();
}

UGSProfileSubsystem* UGSProfileSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UGSProfileSubsystem>() : nullptr;
}

void UGSProfileSubsystem::LoadOrCreate()
{
	Save = Cast<UGSProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	const bool bLoaded = Save != nullptr;
	if (!Save)
	{
		Save = Cast<UGSProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UGSProfileSaveGame::StaticClass()));
	}
	if (Save->Version > UGSProfileSaveGame::CurrentVersion)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[profile] Slot %s has version %d (newer than %d); loading what we understand."),
			*SlotName, Save->Version, UGSProfileSaveGame::CurrentVersion);
	}
	Save->Version = UGSProfileSaveGame::CurrentVersion;

	if (!Save->LocalPlayerId.IsValid())
	{
		Save->LocalPlayerId = FGuid::NewGuid();
		SaveNow();
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[profile] %s slot %s: local player %s, %d player entr%s."), bLoaded ? TEXT("Loaded") : TEXT("Created"),
		*SlotName, *IdToKey(GetLocalPlayerId()), Save->Players.Num(), Save->Players.Num() == 1 ? TEXT("y") : TEXT("ies"));
}

FGuid UGSProfileSubsystem::GetLocalPlayerId() const
{
	FGuid Id = Save ? Save->LocalPlayerId : FGuid();
#if WITH_EDITOR
	// PIE instances share one save file; give each instance its own stable ID so co-op PIE has distinct players.
	const UGameInstance* GI = GetGameInstance();
	const FWorldContext* Context = GI ? GI->GetWorldContext() : nullptr;
	if (Context && Context->WorldType == EWorldType::PIE && Context->PIEInstance > 0)
	{
		Id.D ^= static_cast<uint32>(Context->PIEInstance);
	}
#endif
	return Id;
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UGSProfileSubsystem::BindPlayer(AGSPlayerState* PlayerState, const FGuid& PlayerId)
{
	if (!PlayerState || !PlayerState->HasAuthority() || !PlayerId.IsValid() || !Save)
	{
		return;
	}

	const FString Key = IdToKey(PlayerId);
	FGSProfilePlayerEntry& Entry = Save->Players.FindOrAdd(Key);
	Entry.DisplayName = PlayerState->GetPlayerName();
	Entry.LastSeen = FDateTime::UtcNow();

	PlayerState->ProfileKey = Key;
	BoundPlayers.RemoveAll([PlayerState](const TWeakObjectPtr<AGSPlayerState>& Bound) { return !Bound.IsValid() || Bound.Get() == PlayerState; });
	BoundPlayers.Add(PlayerState);

	UE_LOG(LogBanditBaller, Log, TEXT("[profile] Bound %s to player %s (cash %d)."), *PlayerState->GetPlayerName(), *Key, Entry.Cash);

	// Cash collected before the ID arrived goes into the profile now.
	const int32 Pending = PlayerState->PendingProfileCash;
	PlayerState->PendingProfileCash = 0;
	if (Pending > 0)
	{
		AddCashById(Key, Pending, TEXT("BeforeBind"));
	}
	MirrorToBoundPlayers(Key);
	RequestSave();
}

// ---------------------------------------------------------------------------
// Cash
// ---------------------------------------------------------------------------

FGSProfilePlayerEntry* UGSProfileSubsystem::FindEntry(const FString& PlayerKey)
{
	return Save ? Save->Players.Find(PlayerKey) : nullptr;
}

const FGSProfilePlayerEntry* UGSProfileSubsystem::FindEntry(const FString& PlayerKey) const
{
	return Save ? Save->Players.Find(PlayerKey) : nullptr;
}

int32 UGSProfileSubsystem::GetCashById(const FString& PlayerKey) const
{
	const FGSProfilePlayerEntry* Entry = FindEntry(PlayerKey);
	return Entry ? Entry->Cash : 0;
}

int32 UGSProfileSubsystem::GetCash(const AGSPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return 0;
	}
	return PlayerState->ProfileKey.IsEmpty() ? PlayerState->PendingProfileCash : GetCashById(PlayerState->ProfileKey);
}

bool UGSProfileSubsystem::ChangeCash(const FString& PlayerKey, int32 NewCash, FName Reason)
{
	FGSProfilePlayerEntry* Entry = Save ? &Save->Players.FindOrAdd(PlayerKey) : nullptr;
	if (!Entry)
	{
		return false;
	}
	const int32 OldCash = Entry->Cash;
	Entry->Cash = FMath::Max(0, NewCash);
	if (Entry->Cash != OldCash)
	{
		UE_LOG(LogBanditBaller, Verbose, TEXT("[profile] %s (%s) cash %d -> %d (%s)"), *Entry->DisplayName, *PlayerKey.Left(8), OldCash, Entry->Cash, *Reason.ToString());
		MirrorToBoundPlayers(PlayerKey);
		RequestSave();
	}
	return true;
}

bool UGSProfileSubsystem::AddCashById(const FString& PlayerKey, int32 Amount, FName Reason)
{
	if (Amount <= 0 || PlayerKey.IsEmpty())
	{
		return false;
	}
	const int64 NewCash = FMath::Min<int64>(static_cast<int64>(GetCashById(PlayerKey)) + Amount, MAX_int32);
	return ChangeCash(PlayerKey, static_cast<int32>(NewCash), Reason);
}

bool UGSProfileSubsystem::AddCash(AGSPlayerState* PlayerState, int32 Amount, FName Reason)
{
	if (!PlayerState || !PlayerState->HasAuthority() || Amount <= 0)
	{
		return false;
	}
	if (PlayerState->ProfileKey.IsEmpty())
	{
		// The ID hasn't arrived yet (client just joined): hold it and show it; BindPlayer adds it to the profile.
		PlayerState->PendingProfileCash = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(PlayerState->PendingProfileCash) + Amount, MAX_int32));
		PlayerState->SetCashFromProfile(PlayerState->PendingProfileCash);
		return true;
	}
	return AddCashById(PlayerState->ProfileKey, Amount, Reason);
}

bool UGSProfileSubsystem::TrySpendCash(AGSPlayerState* PlayerState, int32 Amount, FName Reason)
{
	if (!PlayerState || !PlayerState->HasAuthority() || PlayerState->ProfileKey.IsEmpty() || Amount < 0)
	{
		return false;
	}
	const int32 Cash = GetCashById(PlayerState->ProfileKey);
	if (Amount > Cash)
	{
		return false;
	}
	return Amount == 0 || ChangeCash(PlayerState->ProfileKey, Cash - Amount, Reason);
}

bool UGSProfileSubsystem::SetCash(AGSPlayerState* PlayerState, int32 NewCash, FName Reason)
{
	if (!PlayerState || !PlayerState->HasAuthority() || PlayerState->ProfileKey.IsEmpty())
	{
		return false;
	}
	return ChangeCash(PlayerState->ProfileKey, NewCash, Reason);
}

void UGSProfileSubsystem::ResetAllCash(FName Reason)
{
	if (!Save)
	{
		return;
	}
	TArray<FString> Keys;
	Save->Players.GetKeys(Keys);
	for (const FString& Key : Keys)
	{
		ChangeCash(Key, 0, Reason);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[profile] Cash reset to 0 for %d player(s) (%s)."), Keys.Num(), *Reason.ToString());
}

void UGSProfileSubsystem::MirrorToBoundPlayers(const FString& PlayerKey) const
{
	const int32 Cash = GetCashById(PlayerKey);
	for (const TWeakObjectPtr<AGSPlayerState>& Bound : BoundPlayers)
	{
		if (AGSPlayerState* PS = Bound.Get(); PS && PS->ProfileKey == PlayerKey)
		{
			PS->SetCashFromProfile(Cash);
		}
	}
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------

void UGSProfileSubsystem::RequestSave()
{
	bDirty = true;
	UGameInstance* GI = GetGameInstance();
	if (!GI || SaveDelay <= 0.f)
	{
		SaveNow();
		return;
	}
	FTimerManager& Timers = GI->GetTimerManager();
	if (!Timers.IsTimerActive(SaveTimer))
	{
		Timers.SetTimer(SaveTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { SaveNow(); }), SaveDelay, false);
	}
}

bool UGSProfileSubsystem::SaveNow()
{
	if (!Save)
	{
		return false;
	}
	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	bDirty = !bSaved;
	if (!bSaved)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("[profile] Saving slot %s failed."), *SlotName);
	}
	return bSaved;
}
