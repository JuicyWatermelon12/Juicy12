// BanditBaller
//
// Editor-only: manage the Market District streaming sublevel of the open map, since the MCP scene
// tools can place actors but cannot create sublevels or change the current level.
//
//   gs.Editor.MarketDistrictLevel create       create L_MarketDistrict_Graybox as a Blueprint-streamed
//                                              (ULevelStreamingDynamic, unloaded at runtime) sublevel
//   gs.Editor.MarketDistrictLevel edit         make it the current level (new actors go into it)
//   gs.Editor.MarketDistrictLevel persistent   make the persistent level current again
//
// Save afterwards (AssetTools.save_assets with an empty list saves all dirty packages).

#if WITH_EDITOR

#include "BanditBaller.h"
#include "Editor.h"
#include "EditorLevelUtils.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"

namespace GSEditorLevels
{
	const TCHAR* MarketDistrictPackage = TEXT("/Game/BanditBaller/Maps/L_MarketDistrict_Graybox");

	ULevelStreaming* FindStreamingLevel(UWorld& World, const FString& PackageName)
	{
		for (ULevelStreaming* Streaming : World.GetStreamingLevels())
		{
			if (Streaming && Streaming->GetWorldAssetPackageName() == PackageName)
			{
				return Streaming;
			}
		}
		return nullptr;
	}

	void LogCurrentLevel(UWorld& World)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("gs.Editor.MarketDistrictLevel: current level is now %s"),
			World.GetCurrentLevel() ? *World.GetCurrentLevel()->GetOutermost()->GetName() : TEXT("<none>"));
	}

	void Command(const TArray<FString>& Args)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || GEditor->PlayWorld)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Editor.MarketDistrictLevel: needs the editor world (stop PIE first)."));
			return;
		}

		const FString Action = Args.Num() > 0 ? Args[0].ToLower() : TEXT("edit");
		ULevelStreaming* Streaming = FindStreamingLevel(*World, MarketDistrictPackage);

		if (Action == TEXT("create"))
		{
			if (Streaming)
			{
				UE_LOG(LogBanditBaller, Log, TEXT("gs.Editor.MarketDistrictLevel: %s already exists in %s."), MarketDistrictPackage, *World->GetName());
				return;
			}
			const FString Filename = FPackageName::LongPackageNameToFilename(MarketDistrictPackage, FPackageName::GetMapPackageExtension());
			Streaming = UEditorLevelUtils::CreateNewStreamingLevelForWorld(*World, ULevelStreamingDynamic::StaticClass(), Filename,
				/*bMoveSelectedActorsIntoNewLevel*/ false, /*InTemplateWorld*/ nullptr, /*bInUseSaveAs*/ false);
			if (!Streaming)
			{
				UE_LOG(LogBanditBaller, Error, TEXT("gs.Editor.MarketDistrictLevel: failed to create %s."), MarketDistrictPackage);
				return;
			}
			// Runtime: stays unloaded until AGSGameMode streams it in (still visible in the editor for authoring).
			if (ULevelStreamingDynamic* Dynamic = Cast<ULevelStreamingDynamic>(Streaming))
			{
				Dynamic->bInitiallyLoaded = false;
				Dynamic->bInitiallyVisible = false;
			}
			World->MarkPackageDirty();
			UEditorLevelUtils::MakeLevelCurrent(World->PersistentLevel);
			UE_LOG(LogBanditBaller, Log, TEXT("gs.Editor.MarketDistrictLevel: created %s (%s)."), MarketDistrictPackage, *Streaming->GetClass()->GetName());
		}
		else if (Action == TEXT("edit"))
		{
			if (!Streaming || !Streaming->GetLoadedLevel())
			{
				UE_LOG(LogBanditBaller, Warning, TEXT("gs.Editor.MarketDistrictLevel: %s is not in this map or not loaded."), MarketDistrictPackage);
				return;
			}
			UEditorLevelUtils::MakeLevelCurrent(Streaming);
		}
		else if (Action == TEXT("persistent"))
		{
			UEditorLevelUtils::MakeLevelCurrent(World->PersistentLevel);
		}
		else
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Editor.MarketDistrictLevel create|edit|persistent"));
			return;
		}
		LogCurrentLevel(*World);
	}

	static FAutoConsoleCommand MarketDistrictLevelCommand(
		TEXT("gs.Editor.MarketDistrictLevel"),
		TEXT("Editor: manage the Market District sublevel. Usage: gs.Editor.MarketDistrictLevel create|edit|persistent"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Command));
}

#endif // WITH_EDITOR
