// BanditBaller
//
// Tuning console commands (non-Shipping). They act on the local player; on a client they are
// forwarded to the server (AGSPlayerController::ServerExecDebugCommand), so state stays server-authoritative.
//
//   gs.Spawn <EnemyDataName> [Count=1]                  e.g. gs.Spawn MeleeBandit 10 (UGSSpawnLocationFinder)
//   gs.Debug.Spawns 1                                   draw the spawn ring and chosen points
//   gs.God [0|1]                                        toggle / set damage immunity
//   gs.KillAll                                          kill every living enemy (credited to you; drops cash + XP orbs)
//   gs.GiveWeapon <WeaponDataName>                      e.g. gs.GiveWeapon Pistol
//   gs.SetAttr <Attribute> <Value>                      e.g. gs.SetAttr MoveSpeed 900
//   gs.Perf [Seconds=5]                                 log avg/max frame, game, render and GPU time
//   gs.Market.Teleport / gs.Market.Return               open the hub map (from a run) / start a run (from the hub)
//   gs.Round.Skip / gs.Round.Set <N>                    round loop control (AGSGameMode)
//   gs.Debug.Round 1                                    on-screen round / phase / time left (AGSGameState)
//   gs.Run.Seed [N]                                     town: print seed / restart the run with seed N;
//                                                       hub: print / set the next run's seed
//   gs.Run.Stats                                        print run stats (local replicated copy)
//   gs.XP.Add <N> / gs.XP.SetLevel <N>                  grant raw XP / jump to a level (UGSProgressionComponent)
//   gs.Upgrade.Grant <Name> [N] / gs.Upgrade.List       add upgrade stacks / list upgrades (UGSUpgradeComponent)
//   gs.Debug.Boundary 1                                 draw the map boundary (AGSMapBoundary)
//   gs.Upgrade.Offer / Choose <0-2> / Reroll            queue, pick from and reroll level-up offers (AGSGameMode)
//   gs.Profile.AddCash <N> / Reset / Save / Print       permanent cash (UGSProfileSubsystem; local, not forwarded)

#include "BanditBaller.h"
#include "Containers/Ticker.h"
#include "DynamicRHI.h"
#include "EngineUtils.h"
#include "GSDebugLibrary.h"
#include "Engine/Engine.h"
#include "GSEnemyCharacter.h"
#include "GSGameState.h"
#include "GSRunStats.h"
#include "GSHealthComponent.h"
#include "GSPlayerController.h"
#include "GSProfileSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "RenderTimer.h"

#if !UE_BUILD_SHIPPING
namespace GSCheats
{
	void Run(const TCHAR* Command, const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = UGSDebugLibrary::FindGameWorld(World);
		AGSPlayerController* PC = GameWorld ? Cast<AGSPlayerController>(GameWorld->GetFirstPlayerController()) : nullptr;
		if (!PC)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.%s: no local AGSPlayerController (is the game running?)"), Command);
			return;
		}

		if (PC->HasAuthority())
		{
			UGSDebugLibrary::ExecuteDebugCommand(PC, Command, Args);
		}
		else
		{
			PC->ServerExecDebugCommand(Command, Args);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnCommand(
		TEXT("gs.Spawn"),
		TEXT("Spawn enemies around the player. Usage: gs.Spawn <EnemyDataName> [Count=1], e.g. gs.Spawn MeleeBandit 10. Locations follow DA_SpawnSettings (ring, NavMesh, off-screen, fallback)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("Spawn"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs GodCommand(
		TEXT("gs.God"),
		TEXT("Toggle damage immunity (State.Invulnerable) for the local player. Usage: gs.God [0|1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("God"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs KillAllCommand(
		TEXT("gs.KillAll"),
		TEXT("Kill every living enemy through GE_Damage (credited to the local player). They drop cash and XP orbs; no XP until the orbs are collected."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("KillAll"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs GiveWeaponCommand(
		TEXT("gs.GiveWeapon"),
		TEXT("Equip a weapon by data asset name (replaces the equipped one if both slots are full). Usage: gs.GiveWeapon Pistol"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("GiveWeapon"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs SetAttrCommand(
		TEXT("gs.SetAttr"),
		TEXT("Set a player attribute's base value. Usage: gs.SetAttr <Health|MaxHealth|MoveSpeed|Damage|FireRate|PickupRadius|XPGain|ReloadSpeed> <Value>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("SetAttr"), Args, World); }),
		ECVF_Cheat);

	// --- Hub <-> run map travel (debug shortcuts; the portal is the real way in) ---

	FAutoConsoleCommandWithWorldAndArgs MarketTeleportCommand(
		TEXT("gs.Market.Teleport"),
		TEXT("Leave the run: open the hub map (L_Hub_Graybox, AGSGameMode::OpenHubMap). Only in a run."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("MarketTeleport"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs MarketReturnCommand(
		TEXT("gs.Market.Return"),
		TEXT("Start a run from the hub, like the Start Run portal (AGSHubGameMode::StartRun). Only in the hub."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("MarketReturn"), Args, World); }),
		ECVF_Cheat);

	// --- Round loop ---

	FAutoConsoleCommandWithWorldAndArgs RoundSkipCommand(
		TEXT("gs.Round.Skip"),
		TEXT("Advance the round loop to the next phase (PreRound -> Combat -> RoundClear -> next round's PreRound)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("RoundSkip"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs RoundSetCommand(
		TEXT("gs.Round.Set"),
		TEXT("Jump to round N and restart it from PreRound. Usage: gs.Round.Set <N>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("RoundSet"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs RoundPauseCommand(
		TEXT("gs.Round.Pause"),
		TEXT("Freeze / resume the round phase timer (the current phase won't time out). Usage: gs.Round.Pause [0|1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("RoundPause"), Args, World); }),
		ECVF_Cheat);

	// --- Run ---

	FAutoConsoleCommandWithWorldAndArgs RunSeedCommand(
		TEXT("gs.Run.Seed"),
		TEXT("Town: print the run seed, or restart the run (stream + stats, round 1) with seed N. Hub: print / set the next run's seed (0 = random). Usage: gs.Run.Seed [N]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("RunSeed"), Args, World); }),
		ECVF_Cheat);

	// --- XP / levels ---

	FAutoConsoleCommandWithWorldAndArgs XPAddCommand(
		TEXT("gs.XP.Add"),
		TEXT("Grant raw XP to the local player (XPGain not applied; may level up several times). Usage: gs.XP.Add <N>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("XPAdd"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs XPSetLevelCommand(
		TEXT("gs.XP.SetLevel"),
		TEXT("Reset the local player to level 1 and add exactly the XP for level N (0 XP into it). Usage: gs.XP.SetLevel <N>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("XPSetLevel"), Args, World); }),
		ECVF_Cheat);

	// --- Level-up upgrades (per run) ---

	FAutoConsoleCommandWithWorldAndArgs UpgradeGrantCommand(
		TEXT("gs.Upgrade.Grant"),
		TEXT("Add stacks of an upgrade to the local player (stops at MaxStacks). Usage: gs.Upgrade.Grant <UpgradeName> [Count=1], e.g. gs.Upgrade.Grant Damage, gs.Upgrade.Grant ExtraJump 45"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("UpgradeGrant"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs UpgradeListCommand(
		TEXT("gs.Upgrade.List"),
		TEXT("List every upgrade with rarity, this run's stacks / cap and description, plus the stats they change."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("UpgradeList"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs UpgradeOfferCommand(
		TEXT("gs.Upgrade.Offer"),
		TEXT("Queue a level-up upgrade offer without the level (opens at once if none is open), then print the open offer."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("UpgradeOffer"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs UpgradeChooseCommand(
		TEXT("gs.Upgrade.Choose"),
		TEXT("Take a choice of the open upgrade offer; the next queued offer opens. Usage: gs.Upgrade.Choose <0-2>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("UpgradeChoose"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs UpgradeRerollCommand(
		TEXT("gs.Upgrade.Reroll"),
		TEXT("Reroll the open upgrade offer for one of this run's rerolls."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("UpgradeReroll"), Args, World); }),
		ECVF_Cheat);

	/** Reads the local (replicated) GameState, so it also works on clients. */
	void RunStatsCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = UGSDebugLibrary::FindGameWorld(World);
		const AGSGameState* GS = GameWorld ? GameWorld->GetGameState<AGSGameState>() : nullptr;
		const UGSRunStats* Stats = GS ? GS->GetRunStats() : nullptr;
		if (!Stats)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Run.Stats: no AGSGameState / run stats (is the game running?)"));
			return;
		}

		const FString Header = FString::Printf(TEXT("Run stats [%s] | seed %d | %s"),
			GS->HasAuthority() ? TEXT("server") : TEXT("client"), GS->GetRunSeed(), *GS->GetRoundDebugString());
		TArray<FString> Lines;
		Stats->ToDebugString().ParseIntoArrayLines(Lines);

		UE_LOG(LogBanditBaller, Log, TEXT("%s"), *Header);
		for (const FString& Line : Lines)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("  %s"), *Line);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Header + TEXT("\n") + Stats->ToDebugString());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs RunStatsConsoleCommand(
		TEXT("gs.Run.Stats"),
		TEXT("Print run stats (rounds cleared, kills by type, cash, damage dealt/taken, XP, level reached, run time) to the log and screen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunStatsCommand));

	// --- Profile (permanent cash): this machine's UGSProfileSubsystem, so these run locally (not forwarded) ---

	UGSProfileSubsystem* FindProfile(UWorld* World, const TCHAR* Command)
	{
		UWorld* GameWorld = UGSDebugLibrary::FindGameWorld(World);
		UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(GameWorld);
		if (!Profile)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("%s: no profile subsystem (is the game running?)"), Command);
		}
		return Profile;
	}

	void ProfilePrint(UGSProfileSubsystem& Profile)
	{
		TArray<FString> Lines;
		Profile.ToDebugString().ParseIntoArrayLines(Lines);
		UE_LOG(LogBanditBaller, Log, TEXT("Profile:"));
		for (const FString& Line : Lines)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("  %s"), *Line);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Green, TEXT("Profile\n") + Profile.ToDebugString());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ProfileAddCashCommand(
		TEXT("gs.Profile.AddCash"),
		TEXT("Add N to the permanent cash balance (not saved until the next save). Usage: gs.Profile.AddCash <N>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGSProfileSubsystem* Profile = FindProfile(World, TEXT("gs.Profile.AddCash"));
			const int32 Amount = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			if (Profile && !Profile->AddCash(Amount, TEXT("Debug")))
			{
				UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Profile.AddCash <N> (N > 0)"));
			}
			else if (Profile)
			{
				UE_LOG(LogBanditBaller, Log, TEXT("gs.Profile.AddCash: +%d -> %d"), Amount, Profile->GetCash());
			}
		}),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ProfileResetCommand(
		TEXT("gs.Profile.Reset"),
		TEXT("Wipe the profile: delete the save file and start over with cash 0."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGSProfileSubsystem* Profile = FindProfile(World, TEXT("gs.Profile.Reset")))
			{
				Profile->ResetProfile();
			}
		}),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ProfileSaveCommand(
		TEXT("gs.Profile.Save"),
		TEXT("Save the profile now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGSProfileSubsystem* Profile = FindProfile(World, TEXT("gs.Profile.Save")))
			{
				Profile->Save(TEXT("Debug"));
			}
		}),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ProfilePrintCommand(
		TEXT("gs.Profile.Print"),
		TEXT("Print the profile (slot, save version, cash, owned items) to the log and screen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGSProfileSubsystem* Profile = FindProfile(World, TEXT("gs.Profile.Print")))
			{
				ProfilePrint(*Profile);
			}
		}),
		ECVF_Cheat);

	// --- gs.Perf: sample frame timing over a window and log one summary line ---

	struct FPerfSample
	{
		double EndTime = 0.0;
		int32 Frames = 0;
		double FrameSum = 0.0, FrameMax = 0.0;
		double GameSum = 0.0, GameMax = 0.0;
		double RenderSum = 0.0, RenderMax = 0.0;
		double GPUSum = 0.0, GPUMax = 0.0;
		TArray<double> FrameTimes;
	};

	void PerfCommand(const TArray<FString>& Args, UWorld* World)
	{
		const float Seconds = Args.Num() > 0 ? FMath::Max(FCString::Atof(*Args[0]), 0.5f) : 5.f;
		TSharedRef<FPerfSample> Sample = MakeShared<FPerfSample>();
		Sample->EndTime = FPlatformTime::Seconds() + Seconds;
		UE_LOG(LogBanditBaller, Log, TEXT("gs.Perf: sampling for %.1fs..."), Seconds);

		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Sample](float) -> bool
		{
			const double FrameMs = FApp::GetDeltaTime() * 1000.0;
			const double GameMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
			const double RenderMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
			const double GPUMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));

			++Sample->Frames;
			Sample->FrameTimes.Add(FrameMs);
			Sample->FrameSum += FrameMs;   Sample->FrameMax = FMath::Max(Sample->FrameMax, FrameMs);
			Sample->GameSum += GameMs;     Sample->GameMax = FMath::Max(Sample->GameMax, GameMs);
			Sample->RenderSum += RenderMs; Sample->RenderMax = FMath::Max(Sample->RenderMax, RenderMs);
			Sample->GPUSum += GPUMs;       Sample->GPUMax = FMath::Max(Sample->GPUMax, GPUMs);

			if (FPlatformTime::Seconds() < Sample->EndTime)
			{
				return true;
			}

			int32 Enemies = 0;
			if (UWorld* GameWorld = UGSDebugLibrary::FindGameWorld(nullptr))
			{
				for (TActorIterator<AGSEnemyCharacter> It(GameWorld); It; ++It)
				{
					const UGSHealthComponent* Health = It->GetHealthComponent();
					Enemies += (!Health || !Health->IsDead()) ? 1 : 0;
				}
			}

			Sample->FrameTimes.Sort();
			const double P95 = Sample->FrameTimes[FMath::Clamp(FMath::FloorToInt(Sample->FrameTimes.Num() * 0.95), 0, Sample->FrameTimes.Num() - 1)];
			const double N = FMath::Max(Sample->Frames, 1);
			UE_LOG(LogBanditBaller, Log,
				TEXT("gs.Perf: %d frames, %d enemies alive | frame avg %.2f ms (%.0f fps) p95 %.2f max %.2f | game avg %.2f max %.2f | render avg %.2f max %.2f | gpu avg %.2f max %.2f"),
				Sample->Frames, Enemies, Sample->FrameSum / N, 1000.0 / FMath::Max(Sample->FrameSum / N, 0.001), P95, Sample->FrameMax,
				Sample->GameSum / N, Sample->GameMax, Sample->RenderSum / N, Sample->RenderMax, Sample->GPUSum / N, Sample->GPUMax);
			return false;
		}));
	}

	FAutoConsoleCommandWithWorldAndArgs PerfConsoleCommand(
		TEXT("gs.Perf"),
		TEXT("Log average/p95/max frame time plus game/render/GPU time over a window. Usage: gs.Perf [Seconds=5]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PerfCommand));
}
#endif
