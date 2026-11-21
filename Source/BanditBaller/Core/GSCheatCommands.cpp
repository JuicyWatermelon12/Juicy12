// BanditBaller
//
// Tuning console commands (non-Shipping). They act on the local player; on a client they are
// forwarded to the server (AGSPlayerController::ServerExecDebugCommand), so state stays server-authoritative.
//
//   gs.Spawn <EnemyDataName> [Count=1] [Radius=2000]   e.g. gs.Spawn MeleeBandit 10
//   gs.God [0|1]                                        toggle / set damage immunity
//   gs.KillAll                                          kill every living enemy (credited to you)
//   gs.GiveWeapon <WeaponDataName>                      e.g. gs.GiveWeapon Pistol
//   gs.SetAttr <Attribute> <Value>                      e.g. gs.SetAttr MoveSpeed 900
//   gs.Perf [Seconds=5]                                 log avg/max frame, game, render and GPU time
//   gs.Market.Teleport / gs.Market.Return               temporary area travel (AGSGameMode)

#include "BanditBaller.h"
#include "Containers/Ticker.h"
#include "DynamicRHI.h"
#include "EngineUtils.h"
#include "GSDebugLibrary.h"
#include "GSEnemyCharacter.h"
#include "GSHealthComponent.h"
#include "GSPlayerController.h"
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
		TEXT("Spawn enemies around the player. Usage: gs.Spawn <EnemyDataName> [Count=1] [Radius=2000], e.g. gs.Spawn MeleeBandit 10"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("Spawn"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs GodCommand(
		TEXT("gs.God"),
		TEXT("Toggle damage immunity (State.Invulnerable) for the local player. Usage: gs.God [0|1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("God"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs KillAllCommand(
		TEXT("gs.KillAll"),
		TEXT("Kill every living enemy through GE_Damage (credited to the local player)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("KillAll"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs GiveWeaponCommand(
		TEXT("gs.GiveWeapon"),
		TEXT("Equip a weapon by data asset name (replaces the equipped one if both slots are full). Usage: gs.GiveWeapon Pistol"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("GiveWeapon"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs SetAttrCommand(
		TEXT("gs.SetAttr"),
		TEXT("Set a player attribute's base value. Usage: gs.SetAttr <Health|MaxHealth|MoveSpeed|Damage|FireRate> <Value>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("SetAttr"), Args, World); }),
		ECVF_Cheat);

	// --- Temporary Market District travel (the round flow will drive this in M2) ---

	FAutoConsoleCommandWithWorldAndArgs MarketTeleportCommand(
		TEXT("gs.Market.Teleport"),
		TEXT("Temporary: stream in the Market District, fade, move the party to MarketArrival (CurrentArea = MarketDistrict)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("MarketTeleport"), Args, World); }),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs MarketReturnCommand(
		TEXT("gs.Market.Return"),
		TEXT("Temporary: fade, move the party to TownReturn, unload the Market District (CurrentArea = Town)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { Run(TEXT("MarketReturn"), Args, World); }),
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
