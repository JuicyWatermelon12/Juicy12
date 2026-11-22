# BanditBaller

Read @docs/PROGRESS.md at the start of every session. After finishing any prompt, add a short entry to docs/PROGRESS.md (what was built, new assets/commands, known issues) and update 'Next up'.

Third-person roguelike shooter in an off-grid bandit gambling town. **Single-player first**; 2–4 player
listen-server co-op is planned for **M6**, after single-player is finished or nearly finished.
- **Hub:** the game starts in the safe **Market District** (shop + casino), its own map `L_Hub_Graybox`
  (`AGSHubGameMode`; the startup map). The "Start Run" portal (`AGSStartRunPortal`) opens the town map
  (`L_Town_Graybox`), and the run starts when it loads.
- **Run:** PreRound → Combat → RoundClear → PreRound … in the town until the player dies. There are no market visits
  during a run. Enemies drop cash and XP as pickups: a **flat amount per enemy type, the same in every round**.
  Income grows because later rounds spawn more enemies and the player kills faster.
- **After death:** a run summary, then back to the hub to spend or gamble cash.
- **Playthrough progression:** cash, owned items and the gear loadout, saved to a player profile and kept across
  runs and deaths. Everything the player owns is active or usable from the start of the next run; how depends on the
  item type (M3):
  - **Usable items** (consumables) can be bought again and again to restock. Using one in a run uses it up.
  - **Gear** is bought once. In the hub the player puts it in a loadout slot, swapping out the gear in that slot. The
    loadout applies at the start of the next run.
  - Other types (abilities, powerups) are decided in M3.
  - The **shop is static**: a fixed catalogue the player works through. Its stock is never random or rerolled.
  - Casino prizes are owned items like shop purchases. Casino bets risk **cash only**, never owned items.
- **Prestige** (later milestone): the player can choose to prestige. It wipes cash and everything bought or won,
  and gives bonuses for the next playthrough based on the total cash earned in the playthrough. Only prestige
  rewards survive it.
- **Per-run progression** (reset on death): XP, level, and level-up upgrades. XP comes from pickups enemies drop,
  not instantly on the kill.
  - Each level-up fully heals, adds MaxHealth, and offers 3 random upgrades in four rarity tiers (Common, Rare, Epic,
    Legendary).
  - The player must take one (no skip), but can reroll the 3 choices a limited number of times per run (3 to start).
  - The game pauses while the player chooses (the co-op party pause is M6).

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
  Core/       GameMode (round loop, run seed, map travel), hub GameMode, GameState, PlayerState, PlayerController,
              character, settings data classes, run random/stats, progression (XP/levels/rewards, upgrades),
              kill combo,
              debug library + cheat commands, profile subsystem + save (permanent cash),
              map boundary (AGSMapBoundary) + collision channels
  Abilities/  GAS: ASC, AttributeSet, tags, abilities, effects, ability sets
  Combat/     weapons, health/death component, target dummy, hit camera shake
  AI/         enemies, AI controller, StateTree tasks/builder, spawn finder, wave director, spawn telegraph
  Economy/    enemy-drop pickups: cash pieces and XP orbs (actor, pool subsystem, settings)
  UI/         UMG C++ bases (HUD, run summary, ability icon, enemy health bar, upgrade choice screen + card,
              upgrade icon, RoundClear summary)
  Tests/      GSGameplayTests.cpp (editor-only, PIE-based)
  Market/     hub actors (Start Run portal; shop later)
  Casino/     planned (not created yet)
  Variant_*/  template sample code, reference only
Content/BanditBaller/
  Core (framework BPs), Abilities (GA_/GE_/ability sets), Data (DA_/DT_ tuning), Enemies, Weapons, Input,
  UI (WBP_), Materials, Maps (L_Hub_Graybox, L_Town_Graybox), Characters, Hub (hub actors: BP_GSStartRunPortal)
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
  cash.
  - In M3 the save layout changes to two layers: **playthrough data** (cash, owned items, loadout, total cash earned
    this playthrough), which prestige wipes, and **prestige data**, which survives it. Don't build prestige before
    its milestone.
  - Owned items become active or usable at the start of the next run, by item type (usable items are used up, gear
    applies through the loadout).
  - **All cash changes go through it** (`AddCash` / `TrySpendCash`): earning in a run, spending in the shop, gambling.
    Nothing else stores cash (PlayerState has none); the HUD listens to its `OnCashChanged`.
  - Casino bets only ever take cash; owned items are never at stake.
  - It saves at each RoundClear (when it ends), at run end (death) and on quit, not on every change.
  - Automation tests use the `GSProfile_AutomationTest` slot, never the real profile.
- **All in-run randomness uses the run stream** via `UGSRunRandom` (waves, spawn positions, drops, upgrade choices,
  AI choices), never `FMath::Rand*` / `FRandRange` / `VRandCone`. Only exceptions: local cosmetics and the player's
  own predicted weapon spread/recoil. Hub randomness gets its own seeded stream, not the run stream; only the casino
  uses it (the shop is static).
- **Cash and XP per enemy are flat**: the same in every round, with no round scaling (the round clear bonus is a flat
  amount too). Income grows only through more enemies and faster kills.
- **Everything goes through GAS**: stats, buffs/debuffs, abilities and item effects are AttributeSets,
  GameplayEffects and GameplayAbilities with GameplayTags. Damage always goes through `GE_Damage`
  (SetByCaller `Data.Damage`).
- **Combat only in the town.** Enemies spawn, path and fight only in `L_Town_Graybox` (NavMesh covers the town
  only). The Market District hub map is safe: no spawns, no NavMesh, no round loop, and players there have
  `State.SafeZone` (GE_SafeZone: no firing, no damage). Shop and casino actors live only in the hub.
- **Every map has a boundary**: invisible walls around the play area and a ceiling (`AGSMapBoundary`, object type
  `GSCollision::MapBoundary`). It blocks pawns only (players and enemies); shots, traces, pickups and the camera pass
  through. The ceiling is what limits Extra Jump height (Extra Jump has no cap). Spawns stay inside it.
- **Hub and run are separate maps with separate GameModes**: `AGSHubGameMode` (`BP_GSHubGameMode`, set in
  `L_Hub_Graybox`'s World Settings) for the shop, casino and portal; `AGSGameMode` (`BP_GSGameMode`, the global default)
  for the town's round loop.
  - Single-player travels between them with `OpenLevel`. Seamless / server travel and "the whole party travels
    together" are M6.
  - Each run loads a fresh town, so run state resets by being rebuilt, not by hand-written cleanup.
  - Only what is chosen explicitly carries over a map change. Cash survives because the profile subsystem lives on
    the GameInstance.
- **Area travel** (single-player: `OpenLevel`; co-op server travel is M6).
  - The game starts in the hub.
  - The "Start Run" portal starts a run and travels the player to the town (`AGSHubGameMode::StartRun`). The only
    thing passed along is the seed chosen with `gs.Run.Seed` in the hub (`?GSSeed=N`).
  - Death → run summary → travel back to the hub.
  - There is no travel to the hub during a run. `gs.Market.Teleport` (open the hub) / `gs.Market.Return` (start a
    run) are debug shortcuts.
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
- Logs: `LogBanditBaller`, with bracketed prefixes for noisy systems (`[wave]`, `[cash]`, `[dmg]`, `[shot]`, `[ttk]`, `[xp]`, `[profile]`, `[upgrade]`, `[combo]`, `[round]` one line per finished round).
- Tests: PIE-based latent tests in `Tests/GSGameplayTests.cpp`, built from the `GSTests` helpers (`BeginPIE`,
  `WaitUntil`, `Step`, `EndPIE`, …). `BeginPIE` opens `L_Town_Graybox` first (pass `HubMap` for the hub), since the
  editor starts in the hub. `EndPIE` only requests the end: before a second `BeginPIE` in the same test, wait for
  `GetPIEWorld() == nullptr`. When appending, keep the closing `#endif` last.
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
- Playing in PIE through MCP: a console command takes 1–3 s to arrive, too slow to fight. Read live round state with
  `ObjectTools.get_properties` on `...UEDPIE_0_L_Town_Graybox...:PersistentLevel.BP_GSGameState_C_0` (`RoundPhase`,
  `WaveEnemiesTotal/Remaining/Alive`). Clear waves with `gs.God 1` + `gs.KillAll`, and read results from the `[round]`
  / `[upgrade]` log lines (the log file flushes a few seconds late). Click run-summary buttons with
  `SlateInspectorToolset.Click` on the "Return to Market" button ref.

Enabled plugins of note: ModelContextProtocol + AllToolsets, GameplayAbilities, StateTree, GameplayStateTree,
EnhancedInput, CommonUI, ModelingToolsEditorMode, PCG. No online subsystems yet.
