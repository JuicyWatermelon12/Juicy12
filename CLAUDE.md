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
