// BanditBaller

#include "GSProfileSubsystem.h"
#include "BanditBaller.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GSProfileSaveGame.h"
#include "Kismet/GameplayStatics.h"

const TCHAR* UGSProfileSubsystem::DefaultSlotName = TEXT("GSProfile");
const TCHAR* UGSProfileSubsystem::AutomationSlotName = TEXT("GSProfile_AutomationTest");

void UGSProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Automation tests get their own slot so they never change the real profile.
	SlotName = GIsAutomationTesting ? AutomationSlotName : DefaultSlotName;
	LoadOrCreate();
}

void UGSProfileSubsystem::Deinitialize()
{
	// Quit (or end of PIE): keep whatever was earned since the last RoundClear / death save.
	Save(TEXT("Quit"));
	Super::Deinitialize();
}

UGSProfileSubsystem* UGSProfileSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UGSProfileSubsystem>() : nullptr;
}

void UGSProfileSubsystem::LoadOrCreate()
{
	SaveData = Cast<UGSProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!SaveData)
	{
		SaveData = Cast<UGSProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UGSProfileSaveGame::StaticClass()));
		UE_LOG(LogBanditBaller, Log, TEXT("[profile] No save in slot %s; new profile (cash 0)."), *SlotName);
		return;
	}

	// Versioning: older saves are upgraded here when the layout changes; newer ones load what we understand.
	if (SaveData->SaveVersion > UGSProfileSaveGame::CurrentVersion)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[profile] Slot %s has save version %d, newer than this build (%d)."),
			*SlotName, SaveData->SaveVersion, UGSProfileSaveGame::CurrentVersion);
	}
	else if (SaveData->SaveVersion < UGSProfileSaveGame::CurrentVersion)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[profile] Upgrading slot %s from version %d to %d."), *SlotName, SaveData->SaveVersion, UGSProfileSaveGame::CurrentVersion);
		SaveData->SaveVersion = UGSProfileSaveGame::CurrentVersion;
	}
	SaveData->Cash = FMath::Max(0, SaveData->Cash);
	UE_LOG(LogBanditBaller, Log, TEXT("[profile] Loaded slot %s (version %d): cash %d."), *SlotName, SaveData->SaveVersion, SaveData->Cash);
}

int32 UGSProfileSubsystem::GetCash() const
{
	return SaveData ? SaveData->Cash : 0;
}

void UGSProfileSubsystem::SetCash(int32 NewCash, FName Reason)
{
	if (!SaveData)
	{
		return;
	}
	const int32 OldCash = SaveData->Cash;
	SaveData->Cash = FMath::Max(0, NewCash);
	const int32 Delta = SaveData->Cash - OldCash;
	if (Delta != 0)
	{
		UE_LOG(LogBanditBaller, Verbose, TEXT("[profile] Cash %d -> %d (%+d, %s)"), OldCash, SaveData->Cash, Delta, *Reason.ToString());
		OnCashChanged.Broadcast(SaveData->Cash, Delta);
	}
}

bool UGSProfileSubsystem::AddCash(int32 Amount, FName Reason)
{
	if (Amount <= 0 || !SaveData)
	{
		return false;
	}
	// Saturate instead of overflowing.
	SetCash(static_cast<int32>(FMath::Min<int64>(static_cast<int64>(SaveData->Cash) + Amount, MAX_int32)), Reason);
	return true;
}

bool UGSProfileSubsystem::TrySpendCash(int32 Amount, FName Reason)
{
	if (!SaveData || Amount < 0 || Amount > SaveData->Cash)
	{
		return false;
	}
	SetCash(SaveData->Cash - Amount, Reason);
	return true;
}

bool UGSProfileSubsystem::Save(FName Reason)
{
	if (!SaveData)
	{
		return false;
	}
	SaveData->SaveVersion = UGSProfileSaveGame::CurrentVersion;
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0);
	if (bSaved)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[profile] Saved slot %s (%s): cash %d."), *SlotName, *Reason.ToString(), SaveData->Cash);
	}
	else
	{
		UE_LOG(LogBanditBaller, Error, TEXT("[profile] Saving slot %s failed (%s)."), *SlotName, *Reason.ToString());
	}
	return bSaved;
}

void UGSProfileSubsystem::ResetProfile()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	}
	const int32 OldCash = GetCash();
	SaveData = Cast<UGSProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UGSProfileSaveGame::StaticClass()));
	UE_LOG(LogBanditBaller, Log, TEXT("[profile] Reset: deleted slot %s, new profile (cash 0)."), *SlotName);
	if (OldCash != 0)
	{
		OnCashChanged.Broadcast(0, -OldCash);
	}
}

FString UGSProfileSubsystem::ToDebugString() const
{
	return FString::Printf(TEXT("Slot:          %s%s\nSave version:  %d\nCash:          %d\nOwned items:   %d"),
		*SlotName, UGameplayStatics::DoesSaveGameExist(SlotName, 0) ? TEXT("") : TEXT(" (not saved yet)"),
		SaveData ? SaveData->SaveVersion : 0, GetCash(), SaveData ? SaveData->OwnedItems.Num() : 0);
}
