# BanditBaller

Read @docs/PROGRESS.md at the start of every session. After finishing any prompt, add a short entry to docs/PROGRESS.md (what was built, new assets/commands, known issues) and update 'Next up'.

Third-person co-op roguelike shooter in an off-grid bandit gambling town (single player + 2–4 player listen-server
co-op). Loop: fight a round of enemies in the town → collect cash → on some rounds the party teleports to the safe
**Market District** (shop + casino; streamed sublevel `L_MarketDistrict_Graybox`, not open every round) → back to
town → next round. Systems, commands, GAS reference, tuning and known issues live in `docs/PROGRESS.md`.

## Engine and build
- Unreal Engine **5.8.4** (launcher) at `C:\Program Files\Epic Games\UE_5.8`; `.uproject` `EngineAssociation` `5.8`.
- One runtime module `BanditBaller`; targets `BanditBaller` (game) and `BanditBallerEditor`.
- Close the editor before building (ask the user; Live Coding blocks the build and new UCLASSes need a full build):

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" BanditBallerEditor Win64 Development -Project="C:\Users\bbehm\OneDrive\Documents\Unreal Projects\BanditBaller\BanditBaller.uproject" -WaitMutex
```

- Relaunch with `Start-Process` on `Engine\Binaries\Win64\UnrealEditor.exe` + the `.uproject`. MCP is ready once
  127.0.0.1:8000 accepts connections.
- The machine has no Python. Use PowerShell or the Edit tool; for long text in Bash, write it to the scratchpad and
  append it (heredocs with quotes fail).

## Layout
```
Source/BanditBaller/
  Core/       GameMode (round loop, run seed, area travel), GameState, PlayerState, PlayerController,
              character, settings data classes, run random/stats, debug library + cheat commands
  Abilities/  GAS: ASC, AttributeSet, tags, abilities, effects, ability sets
  Combat/     weapons, health/death component, target dummy, hit camera shake
  AI/         enemies, AI controller, StateTree tasks/builder, spawn finder, wave director, spawn telegraph
  Economy/    cash pickups (actor, pool subsystem, settings)
  UI/         UMG C++ bases (HUD, death screen, ability icon, enemy health bar)
  Tests/      GSGameplayTests.cpp (editor-only, PIE-based)
  Market/, Casino/   planned (not created yet)
  Variant_*/  template sample code, reference only
Content/BanditBaller/
  Core (framework BPs), Abilities (GA_/GE_/ability sets), Data (DA_/DT_ tuning), Enemies, Weapons, Input,
  UI (WBP_), Materials, Maps, Characters
Content/Python/   init_unreal.py + banditballer_toolset/ (project MCP toolset; restart the editor after edits)
docs/PROGRESS.md  progress log and reference
```

## Rules
- **Gameplay logic in C++; tuning in data.** Numbers (damage, prices, odds, spawn waves) live in Data Assets /
  DataTables under `Content/BanditBaller/Data` (settings assets are set on `BP_GSGameMode`), never hard-coded.
- **Server-authoritative.** Everything that changes game state runs on the server (authority checks, Server RPCs
  from clients). Clients only predict or do cosmetics.
- **Replicated state lives in GameState / PlayerState** (round, timers, cash, team state), not in GameMode
  (server-only) or widgets.
- **All gameplay randomness uses the run stream** via `UGSRunRandom` (waves, spawn positions, drops, market access,
  casino, AI choices), never `FMath::Rand*` / `FRandRange` / `VRandCone`. Only exceptions: local cosmetics and the
  player's own predicted weapon spread/recoil.
- **Everything goes through GAS**: stats, buffs/debuffs, abilities and item effects are AttributeSets,
  GameplayEffects and GameplayAbilities with GameplayTags. Damage always goes through `GE_Damage`
  (SetByCaller `Data.Damage`).
- **Combat only in the town.** Enemies spawn, path and fight only in `L_Town_Graybox` (NavMesh covers the town
  only). The Market District is safe: no spawns, no NavMesh, enemies ignore players while
  `AGSGameState::CurrentArea != Town`. Shop and casino actors live only in the Market District.
- **Area travel is server-driven** (`AGSGameMode::TravelToMarketDistrict` / `ReturnToTown`). Whether the market
  opens is decided in the MarketDecision phase.
- **Tooling split**: Unreal MCP (`unreal-mcp`, http://127.0.0.1:8000/mcp, see `.mcp.json`) for editor and asset
  work; file edits for C++.
- **Make one MCP call at a time**; wait for each result before the next (batch with `ProgrammaticToolset` instead).
- **Build after every C++ change**; fix errors before moving on.
- **Run the automation tests** (`StartsWith:BanditBaller`) after gameplay changes; add a test for new core rules.
- **Commit after each working slice** (builds + works in editor). Never push without asking.

## Conventions
- C++ classes use the `GS` prefix (`AGSGameMode`, `UGSWaveDirector`); asset prefixes `BP_`, `DA_`, `DT_`, `GA_`,
  `GE_`, `ST_`, `WBP_`, `M_`, `MI_`, `IA_`, `IMC_`.
- Tuning lives in a `UPrimaryDataAsset` subclass (`UGS…Settings`, `Const`), with an asset in `Data/` assigned on
  `BP_GSGameMode` or the owning Blueprint.
- Native gameplay tags only, in `Abilities/GSGameplayTags.*`.
- Debug commands: `gs.` prefix, cheat-only, implemented in `UGSDebugLibrary` and registered in
  `GSCheatCommands.cpp`. Clients forward them through `ServerExecDebugCommand`. Debug toggles are `gs.Debug.*`
  cvars.
- Logs: `LogBanditBaller`, with bracketed prefixes for noisy systems (`[wave]`, `[cash]`, `[dmg]`, `[shot]`, `[ttk]`).
- Tests: PIE-based latent tests in `Tests/GSGameplayTests.cpp`, built from the `GSTests` helpers (`BeginPIE`,
  `WaitUntil`, `Step`, `EndPIE`, …). When appending, keep the closing `#endif` last.
- Commit messages: short imperative title, then a body listing what changed.

## Unreal MCP notes
- Tool-search mode: `list_toolsets`, `describe_toolset` (get schemas first), `call_tool` with the full
  `toolset_name` and the **bare** `tool_name`.
- Useful toolsets: `ObjectTools` (get/set properties; property names from `list_properties`), `AssetTools`
  (save_assets), `BlueprintTools`, `DataAssetTools`, `DataTableTools`, `MaterialInstanceTools`, `UMGToolSet`,
  `EditorToolset.EditorAppToolset` (StartPIE/StopPIE, CaptureEditorImage), `EditorToolset.LogsToolset`,
  `AutomationTestToolset`, `SlateInspectorToolset`, `ProgrammaticToolset`, project
  `banditballer_toolset.toolsets.BanditBallerTools`.
- After `ObjectTools.set_properties` on a Blueprint CDO (e.g. `BP_GSGameMode.Default__BP_GSGameMode_C`), run
  `BlueprintTools.compile_blueprint` before saving/PIE. Otherwise spawned instances keep the old value.
- Console commands: `SlateInspectorToolset` → `Observe(ref "")`, `Snapshot` to find the textbox right after the
  "Cmd" combobox (`tb2`/`tb3`, changes per session), then `Click` + `Type <ref> "<command>" submit=true`. Send
  several at once with one `ProgrammaticToolset.execute_tool_script`.
- PIE: `remove_from_scene` is refused (teleport below KillZ instead). Round 1 Combat starts 3 s in, so send
  `gs.God 1` / `gs.Round.Pause 1` right after starting PIE. With the editor in the background PIE runs at ~3 FPS:
  set `/Script/UnrealEd.Default__EditorPerformanceSettings` `bThrottleCPUWhenNotForeground` false in memory for
  timing work, then restore it.
- `CaptureViewport` shows the editor world, not PIE; use `CaptureEditorImage` (base64 PNG; decode with PowerShell).
- Tests: stop PIE, then `AutomationTestToolset` → `DiscoverTests`, `RunTestsByFilter("StartsWith:BanditBaller")`
  (or `Automation RunTests BanditBaller` in the editor console).

Enabled plugins of note: ModelContextProtocol + AllToolsets, GameplayAbilities, StateTree, GameplayStateTree,
EnhancedInput, CommonUI, ModelingToolsEditorMode, PCG. No online subsystems yet.
