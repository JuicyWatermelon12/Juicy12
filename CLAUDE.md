# BanditBaller

Third-person co-op roguelike shooter set in an off-grid bandit gambling town.
Core loop: fight a round of enemies in the town → earn cash → on some rounds (set rounds or by chance) the
party teleports to the safe **Market District** (shop + casino) → teleport back → next round.
The Market District is a separate area (streamed sublevel `L_MarketDistrict_Graybox`, ~2 km from the town),
not part of the town plaza, and is **not open every round**.
Modes: single-player and 2–4 player listen-server co-op.

## Engine
- Unreal Engine **5.8.4** (launcher install) at `C:\Program Files\Epic Games\UE_5.8`
- `.uproject` `EngineAssociation`: `5.8`

## Build
Close the editor first (or Live Coding will block the build), then:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" BanditBallerEditor Win64 Development -Project="C:\Users\bbehm\OneDrive\Documents\Unreal Projects\BanditBaller\BanditBaller.uproject" -WaitMutex
```

## Layout
Single runtime game module: `BanditBaller` (`Source/BanditBaller/BanditBaller.Build.cs`).
Targets: `BanditBaller` (game), `BanditBallerEditor` (editor).

```
Source/BanditBaller/
  Core/        GameMode, GameState, PlayerState, PlayerController, character base, round flow
  Combat/      weapons, projectiles, hit/damage plumbing
  Abilities/   GAS: AbilitySystemComponent, AttributeSets, GameplayAbilities, GameplayEffects
  Economy/     cash, rewards, payouts
  Market/      shop, inventory offers, purchasing
  Casino/      casino games and odds
  AI/          enemy AI controllers, StateTree tasks/conditions, spawners
  UI/          CommonUI / UMG widgets (C++ bases)
  Tests/       automation tests (editor-only, PIE-based)
  Variant_*/   template sample code (Combat / Platforming / SideScrolling) — reference only

Content/BanditBaller/
  Maps, Characters, Weapons, Enemies, Items, Casino, UI, Materials, VFX, Audio, Data
Content/Python/
  init_unreal.py + banditballer_toolset/   project MCP toolset (registered at editor startup)
```

## Game systems (so far)
- **Player**: `AGSCharacter` (fast movement, slide, dash) with its ASC on `AGSPlayerState`
  (`UGSAbilitySystemComponent`, input-tag activation). Stats in `UGSAttributeSet`: Health, MaxHealth, MoveSpeed,
  Damage, FireRate (+ IncomingDamage meta attribute). Death/restart via `UGSHealthComponent` + `AGSGameMode`.
- **Weapons**: `UGSWeaponData` assets (`Content/BanditBaller/Weapons`: DA_Rifle, DA_Pistol, DA_BanditCarbine),
  `UGSWeaponComponent` (2 slots, ammo), GA_Fire (hitscan on Visibility) / GA_Reload / GA_SwapWeapon.
- **Damage**: always GE_Damage (`UGSDamageEffect`) with SetByCaller `Data.Damage`; blocked by `State.Invulnerable`
  (dash i-frames, gs.God) and `State.Dead`.
- **Enemies**: `AGSEnemyCharacter` (own ASC + UGSAttributeSet) configured by `UGSEnemyData`
  (`Content/BanditBaller/Enemies`: DA_Enemy_MeleeBandit, DA_Enemy_RangedBandit). `AGSEnemyAIController` runs the
  data's StateTree (ST_MeleeBandit, ST_RangedBandit), generated from code with
  `gs.Editor.BuildEnemyStateTree [melee|ranged|all]`. Abilities GA_MeleeAttack / GA_EnemyFire.
- **Areas**: `L_Town_Graybox` (persistent; combat, NavMesh, `EnemySpawn` points, plaza Target Points `TownReturn`
  and `MarketPortalSpot`; the old plaza casino/stall boxes are untagged `Cover_*`) and the Market District sublevel
  `L_MarketDistrict_Graybox` (Blueprint-streamed `ULevelStreamingDynamic`, unloaded by default, centred at
  X=+200000): walled 50x50 m square, `Casino` building, 4 `MarketStall`s, Target Points `MarketArrival` /
  `MarketExitPortalSpot`, warm post-process tint, no NavMesh, no spawns. `AGSGameState::CurrentArea`
  (`EGSArea`), `bMarketOpenThisRound`; round phases include `MarketDecision` and `MarketVisit`.
  To edit the district with MCP: `gs.Editor.MarketDistrictLevel edit`, place actors, then `... persistent` and save.
- **Round loop**: `AGSGameMode` state machine over `EGSRoundPhase`:
  PreRound → Combat → RoundClear → MarketDecision → (MarketVisit) → PreRound with round + 1. Starts round 1 on
  `StartPlay`. Every change goes through `SetPhase()` (logs `Round N: Old -> New`); `AdvancePhase()` holds the
  transition table. Timings in `UGSRoundSettings` (`Content/BanditBaller/Data/DA_RoundSettings`, set on BP_GSGameMode):
  PreRound 3 s, Combat fallback 60 s, RoundClear 4 s. For now Combat ends on its timer and MarketDecision always
  closes the market and starts the next round. `AGSGameState` replicates RoundNumber, RoundPhase, PhaseEndTime
  (server clock; `GetPhaseTimeRemaining()`), and fires `OnPhaseChanged` / `OnRoundChanged` on server and clients.
- **Collision gotcha**: player capsule + mesh ignore `ECC_Visibility`; enemy capsules block it (player hitscan).
  Enemy shots/LOS use `AGSEnemyCharacter::GetShotObjectQueryParams()` (WorldStatic/Dynamic/Pawn).

## Debug & tuning tools
Console commands (non-Shipping, cheat; run on the server — clients forward via `ServerExecDebugCommand`).
Implementation is shared in `UGSDebugLibrary` (`Core/GSDebugLibrary.*`, `Core/GSCheatCommands.cpp`).

| Command | Effect |
|---|---|
| `gs.Spawn <EnemyDataName> [Count=1] [Radius=2000]` | spawn enemies on the navmesh around the player (`MeleeBandit`, `RangedBandit`, or full `DA_Enemy_*` name) |
| `gs.God [0\|1]` | toggle/set damage immunity (adds one `State.Invulnerable` count) |
| `gs.KillAll` | kill all living enemies through GE_Damage (credited to the player) |
| `gs.GiveWeapon <WeaponDataName>` | equip a weapon with a full mag (`Pistol`, `Rifle`, `BanditCarbine`); replaces the equipped one if both slots are full |
| `gs.SetAttr <Attribute> <Value>` | set a player attribute base value (Health, MaxHealth, MoveSpeed, Damage, FireRate) |
| `gs.Perf [Seconds=5]` | log avg/p95/max frame time, game thread and GPU time, enemies alive |
| `gs.Round.Skip` | advance to the next round phase |
| `gs.Round.Set <N>` | jump to round N and restart it from PreRound |
| `gs.Debug.Round 1` | on-screen `[Server/Client] Round N \| Phase \| time left \| Area \| Market open` (from each machine's replicated GameState) |
| `gs.Market.Teleport` / `gs.Market.Return` | temporary: travel the party to the Market District and back (`AGSGameMode`); M2's round flow will call these |
| `gs.Editor.MarketDistrictLevel create\|edit\|persistent` | editor only: create the district sublevel / make it (or the persistent level) current so MCP places actors there |
| `gs.Damage.Self <n>` | damage the local player through GAS |
| `gs.Debug.DamageLog 1` | log every health loss: `[dmg] victim took N from X (health) at t=` |
| `gs.Debug.Weapons 1` | debug-draw shots; log enemy shots (`[shot]`) |
| `GSFire`, `GSPress <InputTag>`, `GSAimAt X Y Z` | drive player input from the console (exec on the controller) |

Typing console commands via MCP: `SlateInspectorToolset` → `Observe(ref "")`, `Click tb3`
(the Output Log "Cmd" box), `Type tb3 "<command>" submit=true`. Refs reset after an editor restart.

**Project MCP toolset** `banditballer_toolset.toolsets.BanditBallerTools` (needs PIE running):
- `spawn_enemies(type, count=1, radius=2000)` → spawned actor names
- `get_player_attributes()` → attribute values + Ammo, God, Invulnerable, Dead
- `list_enemies_alive()` → name, type, health, location, distance to player, attacking (nearest first)

Source: `Content/Python/banditballer_toolset/` (Python `ToolsetDefinition` wrapping `UGSDebugLibrary`);
registered by `Content/Python/init_unreal.py`, so restart the editor after editing it.

PIE notes: `SceneTools.remove_from_scene` is refused during PIE (teleport below KillZ instead). With the editor
in the background, `EditorPerformanceSettings.bThrottleCPUWhenNotForeground` drops PIE to ~3 FPS, which skews
timings; turn it off temporarily (in memory) for perf checks.

## Automation tests
`Source/BanditBaller/Tests/GSGameplayTests.cpp` (editor-only). Each test starts PIE in the current editor map
(L_Town_Graybox), waits for the player, removes enemies, runs, and ends PIE. **Stop PIE before running.**

- `BanditBaller.GAS.DamageReducesHealth`
- `BanditBaller.GAS.HealthClampsToZeroAndMax`
- `BanditBaller.GAS.DeathAddsStateDead`
- `BanditBaller.Weapons.FireConsumesAmmoReloadRefills`
- `BanditBaller.Abilities.DashInvulnerabilityBlocksDamage`
- `BanditBaller.Map.TownAndMarketDistrict` (town tags/navmesh, district sublevel contents + no navmesh,
  teleport there and back, spawning refused in the district)
- `BanditBaller.Round.PhaseLoop` (round 1 PreRound on start, PreRound timer → Combat, skips through
  RoundClear/MarketDecision into round 2, gs.Round.Set)

Run via MCP: `AutomationTestToolset` → `DiscoverTests`, then `RunTestsByFilter("StartsWith:BanditBaller")`.
Or in the editor console: `Automation RunTests BanditBaller`.

Enabled plugins of note: ModelContextProtocol + AllToolsets (Unreal MCP), GameplayAbilities,
StateTree, GameplayStateTree, EnhancedInput, CommonUI, ModelingToolsEditorMode, PCG.
No online subsystems yet.

## Available MCP toolsets
The server runs in tool-search mode: it exposes only `list_toolsets`, `describe_toolset` and `call_tool`.
Call `describe_toolset` first to get the schemas, then call `call_tool` with `toolset_name` = the full toolset name
and `tool_name` = the **bare** tool name (e.g. `get_current_level`, not the prefixed form).
The editor must be open; the server listens on 127.0.0.1:8000 once startup finishes.

- **Level / actors**: `editor_toolset…SceneTools` (current level, load level, spawn/find/remove actors, outliner folders),
  `ActorTools` (transforms, components, attachment), `PrimitiveTools`
- **Assets & objects**: `AssetTools`, `ObjectTools` (list/get/set properties, class discovery),
  `StaticMeshTools`, `SkeletalMeshTools`, `TextureTools`, `PhysicsToolsets.PhysicsAssetToolset`
- **Blueprints**: `BlueprintTools`
- **Data**: `DataAssetTools`, `DataTableTools`, `CurveTableTools`, `StringTableTools`, `DataRegistryToolset`,
  `GameplayTagsToolset`
- **GAS**: `GASToolsets.AttributeSetToolset`, `GameplayCueToolset`, `AbilitySystemInspectorToolset` (runtime ASC state)
- **AI**: `state_tree_toolset…StateTreeTools`, `aimodule_toolset…BehaviorTreeTools`, `WorldConditionsToolset`
  (these are inspect-only)
- **UI**: `UMGToolSet` (widget trees; use ObjectTools list_properties before set_properties),
  `SlateInspectorToolset` (drive the editor UI)
- **Materials**: `MaterialTools`, `MaterialInstanceTools`
- **PCG**: `PCGToolset.PCGToolset` (graphs), `PCGSpatialToolset`
- **Niagara / VFX**: `NiagaraToolsets.*` (System, Component, Blueprint, Assets, Info)
- **Animation / Sequencer**: `animation_toolset…` (ControlRig, Sequencer, Keyframing, ControlRig-in-Sequencer,
  Outliner, Conditions, CustomBindings, ImportExport), `DataflowAgent`
- **Editor / automation**: `EditorToolset.EditorAppToolset` (cvars, selection, viewport, PIE control),
  `EditorToolset.LogsToolset` (read the output log), `AutomationTestToolset` (run tests),
  `ConfigSettingsToolset` (ini settings), `PluginToolset`, `GameFeaturesToolset`
- **Misc**: `SemanticSearchToolset` (asset search), `ProgrammaticToolset` (batch tool calls via sandboxed Python),
  `ToolsetRegistry.AgentSkillToolset`, `conversation_toolset…ConversationTools`
- **Project**: `banditballer_toolset.toolsets.BanditBallerTools` (spawn enemies, player attributes, live enemies;
  see Debug & tuning tools)

## Rules
- **Gameplay logic in C++; tuning in data.** Numbers (damage, prices, odds, spawn waves) live in
  Data Assets / DataTables under `Content/BanditBaller/Data`, never hard-coded.
- **Server-authoritative.** Everything that changes game state runs on the server
  (authority checks, Server RPCs from clients). Clients only predict/cosmetic.
- **Replicated state lives in GameState / PlayerState** (round, timers, cash, team state),
  not in GameMode (server-only) or widgets.
- **Everything goes through GAS**: stats, buffs/debuffs, abilities, and item effects are
  AttributeSets, GameplayEffects, and GameplayAbilities with GameplayTags.
- **Combat only in the town.** Enemies spawn, path and fight only in L_Town_Graybox (NavMesh covers the town only).
  The Market District is safe: no enemy spawns, no NavMesh, enemies ignore players while
  `AGSGameState::CurrentArea != Town`. Shop and casino actors live only in the Market District.
- **Area travel is server-driven** (`AGSGameMode::TravelToMarketDistrict` / `ReturnToTown`): the server
  streams the Market District sublevel in/out, moves the whole party, and sets the replicated `CurrentArea`.
  Whether the market opens this round (`bMarketOpenThisRound`) is decided in the MarketDecision phase (M2).
- **Tooling split**: use Unreal MCP (`unreal-mcp`, http://127.0.0.1:8000/mcp, see `.mcp.json`)
  for editor and asset work; use file edits for C++.
- **Make one MCP call at a time** — wait for each result before the next.
- **Build after every C++ change** with the command above; fix errors before moving on.
- **Run the automation tests** (`StartsWith:BanditBaller`) after gameplay changes; add a test for new core rules.
- **Commit after each working slice** (builds + works in editor). Never push without asking.
