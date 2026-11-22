// BanditBaller
//
// Editor-only map tooling the MCP scene tools don't cover (they can place actors but not manage sublevels).
//
//   gs.Editor.RemoveSublevel <ShortName>   remove a streaming sublevel (e.g. L_MarketDistrict_Graybox) from the open
//                                          map; the map is left dirty, save it afterwards
//                                          (AssetTools.save_assets).
//
// (gs.Editor.MarketDistrictLevel, which created the old streamed Market District, is gone: the district is now the
// L_Hub_Graybox map.)

#if WITH_EDITOR

#include "BanditBaller.h"
#include "Editor.h"
#include "EditorLevelUtils.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"

namespace GSEditorLevels
{
	void RemoveSublevel(const TArray<FString>& Args)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || GEditor->PlayWorld)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Editor.RemoveSublevel: needs the editor world (stop PIE first)."));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Editor.RemoveSublevel <ShortName>, e.g. gs.Editor.RemoveSublevel L_MarketDistrict_Graybox"));
			return;
		}

		for (ULevelStreaming* Streaming : World->GetStreamingLevels())
		{
			if (!Streaming || FPackageName::GetShortName(Streaming->GetWorldAssetPackageName()) != Args[0])
			{
				continue;
			}
			const bool bRemoved = Streaming->GetLoadedLevel()
				? UEditorLevelUtils::RemoveLevelFromWorld(Streaming->GetLoadedLevel())
				: UEditorLevelUtils::RemoveInvalidLevelFromWorld(Streaming);
			World->MarkPackageDirty();
			UE_LOG(LogBanditBaller, Log, TEXT("gs.Editor.RemoveSublevel: %s %s from %s (save the map)."), *Args[0],
				bRemoved ? TEXT("removed") : TEXT("could NOT be removed"), *World->GetName());
			return;
		}
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.Editor.RemoveSublevel: %s is not a sublevel of %s."), *Args[0], *World->GetName());
	}

	static FAutoConsoleCommand RemoveSublevelCommand(
		TEXT("gs.Editor.RemoveSublevel"),
		TEXT("Editor: remove a streaming sublevel from the open map (save it afterwards). Usage: gs.Editor.RemoveSublevel <ShortName>"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&RemoveSublevel));
}

#endif // WITH_EDITOR
