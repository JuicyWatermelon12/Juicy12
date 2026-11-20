# BanditBaller

Third-person co-op roguelike shooter set in an off-grid bandit gambling town.
Core loop: fight rounds of enemies → earn cash → visit the market (shop + casino) → next round.
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
  Variant_*/   template sample code (Combat / Platforming / SideScrolling) — reference only

Content/BanditBaller/
  Maps, Characters, Weapons, Enemies, Items, Casino, UI, Materials, VFX, Audio, Data
```

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

## Rules
- **Gameplay logic in C++; tuning in data.** Numbers (damage, prices, odds, spawn waves) live in
  Data Assets / DataTables under `Content/BanditBaller/Data`, never hard-coded.
- **Server-authoritative.** Everything that changes game state runs on the server
  (authority checks, Server RPCs from clients). Clients only predict/cosmetic.
- **Replicated state lives in GameState / PlayerState** (round, timers, cash, team state),
  not in GameMode (server-only) or widgets.
- **Everything goes through GAS**: stats, buffs/debuffs, abilities, and item effects are
  AttributeSets, GameplayEffects, and GameplayAbilities with GameplayTags.
- **Tooling split**: use Unreal MCP (`unreal-mcp`, http://127.0.0.1:8000/mcp, see `.mcp.json`)
  for editor and asset work; use file edits for C++.
- **Make one MCP call at a time** — wait for each result before the next.
- **Build after every C++ change** with the command above; fix errors before moving on.
- **Commit after each working slice** (builds + works in editor). Never push without asking.
