// BanditBaller
//
// Editor-only: builds/rebuilds enemy StateTree assets from code, so the AI layout is reproducible
// (the MCP StateTree tools can only inspect trees). The result is a normal StateTree asset that can
// be opened and edited in the StateTree editor afterwards.
//
// Console:  gs.Editor.BuildEnemyStateTree [/Game/Path/ST_Name]   (default /Game/BanditBaller/Enemies/ST_MeleeBandit)

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "BanditBaller.h"
#include "GSEnemyStateTreeSchema.h"
#include "GSEnemyStateTreeTasks.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorSchema.h"
#include "StateTreeState.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GSStateTreeBuilder
{
	static void BuildMeleeEnemyTree(UStateTreeEditorData& EditorData)
	{
		UStateTreeState& Root = EditorData.AddRootState();

		UStateTreeState& Idle = Root.AddChildState(TEXT("Idle"));
		Idle.AddTask<FGSStateTreeTask_WaitForTarget>();

		UStateTreeState& Chase = Root.AddChildState(TEXT("Chase"));
		Chase.AddTask<FGSStateTreeTask_ChaseTarget>();

		UStateTreeState& Attack = Root.AddChildState(TEXT("Attack"));
		Attack.AddTask<FGSStateTreeTask_MeleeAttack>();

		Idle.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Chase);
		Idle.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Idle);

		Chase.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Attack);
		Chase.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Idle);

		Attack.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Chase);
	}

	static void BuildCommand(const TArray<FString>& Args)
	{
		const FString PackagePath = Args.Num() > 0 ? Args[0] : TEXT("/Game/BanditBaller/Enemies/ST_MeleeBandit");
		if (!FPackageName::IsValidLongPackageName(PackagePath))
		{
			UE_LOG(LogBanditBaller, Error, TEXT("gs.Editor.BuildEnemyStateTree: invalid package path '%s'"), *PackagePath);
			return;
		}
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);

		UPackage* Package = CreatePackage(*PackagePath);
		Package->FullyLoad();

		UStateTree* Tree = FindObject<UStateTree>(Package, *AssetName);
		const bool bIsNew = Tree == nullptr;
		if (bIsNew)
		{
			Tree = NewObject<UStateTree>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		}

		// Fresh editor data every time: the layout lives in code.
		UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(Tree, NAME_None, RF_Transactional);
		EditorData->Schema = NewObject<UGSEnemyStateTreeSchema>(EditorData, NAME_None, RF_Transactional);
		EditorData->EditorSchema = NewObject<UStateTreeEditorSchema>(EditorData, NAME_None, RF_Transactional);
		Tree->EditorData = EditorData;

		BuildMeleeEnemyTree(*EditorData);

		UStateTreeEditingSubsystem::ValidateStateTree(Tree);

		FStateTreeCompilerLog Log;
		const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(Tree, Log);
		Log.DumpToLog(LogBanditBaller);
		if (!bCompiled)
		{
			UE_LOG(LogBanditBaller, Error, TEXT("gs.Editor.BuildEnemyStateTree: compile FAILED for %s"), *PackagePath);
			return;
		}

		if (bIsNew)
		{
			FAssetRegistryModule::AssetCreated(Tree);
		}
		Package->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		const bool bSaved = UPackage::SavePackage(Package, Tree, *Filename, SaveArgs);

		UE_LOG(LogBanditBaller, Log, TEXT("gs.Editor.BuildEnemyStateTree: %s %s (compiled, %s)"),
			bIsNew ? TEXT("created") : TEXT("rebuilt"), *PackagePath, bSaved ? TEXT("saved") : TEXT("SAVE FAILED"));
	}

	static FAutoConsoleCommand BuildEnemyStateTreeCommand(
		TEXT("gs.Editor.BuildEnemyStateTree"),
		TEXT("Build/rebuild the melee enemy StateTree asset from code. Usage: gs.Editor.BuildEnemyStateTree [/Game/Path/ST_Name]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&BuildCommand));
}

#endif // WITH_EDITOR
