# BanditBaller

Read @docs/PROGRESS.md at the start of every session. After finishing any prompt, add a short entry to docs/PROGRESS.md (what was built, new assets/commands, known issues) and update 'Next up'.

Third-person roguelike shooter in an off-grid bandit gambling town. **Single-player first**; 2–4 player
listen-server co-op is planned for **M6**, after single-player is finished or nearly finished.
- **Hub:** the game starts in the safe **Market District** (shop + casino). It is a separate hub map (planned
  `L_Hub_Graybox`; today it is still the streamed sublevel `L_MarketDistrict_Graybox` in the town map). A "Start
  Run" portal sends the player to the town.
- **Run:** PreRound → Combat → RoundClear → PreRound … in the town until the player dies. There are no market visits
  during a run.
- **After death:** a run summary, then back to the hub to spend or gamble cash.
- **Permanent progression:** cash, saved to a player profile and kept across runs and deaths.
- **Per-run progression** (reset on death): XP, level, and level-up upgrades. Each level-up fully heals, adds
  MaxHealth, and offers 3 random upgrades with rarities. The game pauses while the player chooses (the co-op party
  pause is M6).

Systems, commands, GAS reference, tuning and known issues live in `docs/PROGRESS.md`.

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
              character, settings data classes, run random/stats, progression (XP/levels/rewards, upgrades),
              debug library + cheat commands, profile subsystem + save (permanent cash)
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
- **Only do what the current prompt asks.** Don't start work from later steps, even if the plan mentions it — list it
  as a suggestion instead.
- **Gameplay logic in C++; tuning in data.** Numbers (damage, prices, odds, spawn waves) live in Data Assets /
  DataTables under `Content/BanditBaller/Data` (settings assets are set on `BP_GSGameMode`), never hard-coded.
- **Co-op-ready, single-player first.** Co-op comes in M6.
  - Build and test new work for single-player only. Don't add new RPCs, replication or network tests.
  - Don't remove existing replication code (replicated properties, RPCs, authority checks).
  - Keep game state in GameMode / GameState / PlayerState and permanent state in the profile subsystem.
  - Change state only through gameplay functions (e.g. `AddXP`, `AwardCash`, `SetPhase`), never by poking fields
    from widgets or other systems.
  - Pass the PlayerState / Controller explicitly instead of grabbing player 0 (`GetPlayerController(0)`,
    `GetFirstPlayerController`) deep in gameplay code.
  - Mark co-op gaps with `TODO(co-op)`.
- **Run state lives in GameState / PlayerState** and resets when the run ends: round, timers, run stats, team state,
  XP, level, level-up upgrades. It never lives in GameMode (server-only) or in widgets.
- **Permanent state lives in the profile subsystem** (`UGSProfileSubsystem`, a GameInstance subsystem saved to
  `Saved/SaveGames/GSProfile.sav` as `UGSProfileSaveGame`). It survives runs, deaths and map travel. Today that is
  cash; later, owned items.
  - **All cash changes go through it** (`AddCash` / `TrySpendCash`): earning in a run, spending in the shop, gambling.
    Nothing else stores cash (PlayerState has none); the HUD listens to its `OnCashChanged`.
  - It saves at each RoundClear (when it ends), at run end (death) and on quit, not on every change.
  - Automation tests use the `GSProfile_AutomationTest` slot, never the real profile.
- **All in-run randomness uses the run stream** via `UGSRunRandom` (waves, spawn positions, drops, upgrade choices,
  AI choices), never `FMath::Rand*` / `FRandRange` / `VRandCone`. Only exceptions: local cosmetics and the player's
  own predicted weapon spread/recoil. Hub randomness (casino) will get its own seeded stream, not the run stream.
- **Everything goes through GAS**: stats, buffs/debuffs, abilities and item effects are AttributeSets,
  GameplayEffects and GameplayAbilities with GameplayTags. Damage always goes through `GE_Damage`
  (SetByCaller `Data.Damage`).
- **Combat only in the town.** Enemies spawn, path and fight only in `L_Town_Graybox` (NavMesh covers the town
  only). The Market District hub map is safe: no spawns, no NavMesh, no round loop. Shop and casino actors live only
  in the hub.
- **Hub and run are separate maps with separate GameModes** (planned: a hub GameMode for shop, casino and portal; a
  run GameMode for the round loop).
  - Single-player travels between them with `OpenLevel`. Seamless / server travel and "the whole party travels
    together" are M6.
  - Each run loads a fresh town, so run state resets by being rebuilt, not by hand-written cleanup.
  - Only what is chosen explicitly carries over a map change. Cash survives because the profile subsystem lives on
    the GameInstance.
- **Area travel** (single-player: `OpenLevel`; co-op server travel is M6).
  - The game starts in the hub.
  - The "Start Run" portal starts a run and travels the player to the town.
  - Death → run summary → travel back to the hub.
  - There is no travel to the market during a run (`gs.Market.Teleport` / `Return` are debug tools only until the
    hub map replaces them).
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
- Logs: `LogBanditBaller`, with bracketed prefixes for noisy systems (`[wave]`, `[cash]`, `[dmg]`, `[shot]`, `[ttk]`, `[xp]`, `[profile]`, `[upgrade]`).
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
