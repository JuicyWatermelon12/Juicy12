# BanditBaller

Read @docs/PROGRESS.md at the start of every session. After finishing any prompt, add a short entry to docs/PROGRESS.md (what was built, new assets/commands, known issues) and update 'Next up'.

Third-person roguelike shooter, **cops and robbers**: the player is the robber, out to get all the cash, and the
enemies are law enforcement (since M2d, Prompt 54; this replaced "bandits in a bandit town"). The project and code
names (BanditBaller, `GS`) stay. **Single-player first**; 2–4 player listen-server co-op is planned for **M6**, after
single-player is finished or nearly finished.
- **Hub:** the game starts in the safe **Market District**, the black market and casino (shop + casino), its own
  map `L_Hub_Graybox` (`AGSHubGameMode`; the startup map). The "Start Run" portal (`AGSStartRunPortal`) opens the run
  map, and the run starts when it loads.
- **Run map:** today the 300 × 300 m town `L_Town_Graybox` (75 m ceiling). In M2d (Prompt 67) a **city graybox of
  500 × 500 m with a 100 m ceiling** replaces it.
- **Run:** PreRound (round 1's 3 s countdown) → Combat, where rounds roll over (Breather after every 10th) … until the
  player dies (GameOver). There is no RoundClear phase since Prompt 59. There are no market visits during a
  run. Enemies drop cash and XP as pickups: a **flat amount per enemy type, the same in every round**. Income grows
  because later rounds spawn more enemies and the player kills faster.
  - **Roll-over rounds** (M2d, Prompt 59, built): play stays split into rounds but feels continuous. The next round's
    enemies start arriving when only a few (3) of the current round are left, with no countdown; enemies arrive in a
    steady stream, not a few batches; the round-clear bonus is a small pop-up while play goes on. Every 10 rounds
    there is a 10-second breather (placeholders; a boss goes there later). Level-ups still pause for the card pick.
- **Enemies** (law enforcement): eight types that unlock as the rounds go on (no wanted level, no agency tiers).
  Beat Cop (baton, melee) and Patrol Officer (carbine) are built; Rusher, Shield, Grenadier, Heavy, Sniper and Drone
  come in M2d. Ranged enemies fire visible, dodgeable projectiles (no hit-scan), and at most 40% of a generated wave
  is ranged. The alive cap goes to 100 in M2d, at 60 fps. Code names: `BeatCop` / `PatrolOfficer`
  (`DA_Enemy_BeatCop`, `gs.Spawn BeatCop`, …; renamed from MeleeBandit / RangedBandit in Prompt 57).
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
  not instantly on the kill. Levels are endless (M2c): there is no level cap, and the XP needed per level grows by
  a fixed amount.
  - Each level-up fully heals, adds MaxHealth, and offers 3 random upgrade cards.
  - Every upgrade is in one of three categories:
    - **Stat Boosts:** Damage, Fire Rate, Reload Speed, Move Speed, Magnet, Bigger Mags, Loaded Dice.
    - **Abilities:** Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech, Powder Keg. **Legendary
      abilities** are a sub-group: Second Wind today, more later.
    - **Weapons:** automatic attacks: Floating Gun, Razor Cards, Barbed Wire, Dynamite, Lucky Coin, Molotov and
      Tomahawk (all seven built; Prompts 38–39, 46–47, 49–52). A run holds at most 3 weapons; the rifle and pistol
      aren't part of this.
  - Each card rolls its own rarity (Common, Rare, Epic, Legendary), and a higher rarity gives a bigger pick. Nothing
    has a pick cap.
  - The 15 upgrades were built in **M2b** (after M2, before the market, M3). **M2c** reworked them (17 upgrades
    with Razor Cards and Barbed Wire); the M2b + M2c checkpoint is Prompt 48. Prompts 49–52 added the last four
    weapons (21 upgrades); the weapons checkpoint (Prompt 53) is done. The design and its rules are in
    `docs/PROGRESS.md` (**Design update — upgrade rework (M2c)**; the original list is under **Design update — M2b
    upgrades**).
- **Current block: M2d** (Prompts 54–69, after the weapons checkpoint and before the market, M3): the cops theme,
  Blender placeholders, dodgeable shots, roll-over rounds, 100 enemies, six new enemy types and the city map. Its
  checkpoint (Prompt 69) gives the income numbers that set the M3 prices. The design is in `docs/PROGRESS.md`
  (**Design update — cops, city and round flow (M2d)**).
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
              kill combo, floating shields (state on the PlayerState, visuals on the character), second wind,
              debug library + cheat commands, profile subsystem + save (permanent cash),
              map boundary (AGSMapBoundary) + collision channels, worn props (FGSMeshAttachment)
  Abilities/  GAS: ASC, AttributeSet, tags, abilities, effects, ability sets
  Combat/     weapons, health/death component, target dummy, hit camera shake, critical hits, projectile + pool
              (optional homing, ignore list, finish callback), floating gun (gun actor + character component +
              settings), razor cards, barbed wire, dynamite, lucky coin, molotov and tomahawk (character component +
              settings each), thrown weapon actors + pool (lobbed throw, fire patch, tomahawk)
  AI/         enemies, AI controller, StateTree tasks/builder, spawn finder, wave director, spawn telegraph
  Economy/    enemy-drop pickups: cash pieces, XP orbs and health orbs (actor, pool subsystem, settings)
  UI/         UMG C++ bases (HUD, run summary, ability icon, enemy health bar, upgrade choice screen + card,
              upgrade icon; the round-clear pop-up is part of the HUD since Prompt 59)
  Tests/      GSGameplayTests.cpp (editor-only, PIE-based)
  Market/     hub actors (Start Run portal; shop later)
  Casino/     planned (not created yet)
  Variant_*/  template sample code, reference only
Content/BanditBaller/
  Core (framework BPs), Abilities (GA_/GE_/ability sets), Data (DA_/DT_ tuning), Enemies, Weapons, Input,
  UI (WBP_), Materials, Maps (L_Hub_Graybox, L_Town_Graybox), Characters, Hub (hub actors: BP_GSStartRunPortal)
Content/Art/Placeholders/  Blender placeholders: Characters, Props, Enemies, Weapons, City, Materials (M_LowPoly + MI_)
Content/Python/   init_unreal.py + banditballer_toolset/ (project MCP toolset; restart the editor after edits)
Art/Blender/      Blender sources (.blend); Art/Export/<Kind>/ the FBX exports (both in git LFS)
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
  - It saves at each round's clear (once its vacuum has landed), at run end (death) and on quit, not on every change.
  - Automation tests use the `GSProfile_AutomationTest` slot, never the real profile.
- **All in-run randomness uses the run stream** via `UGSRunRandom` (waves, spawn positions, drops, upgrade choices,
  AI choices), never `FMath::Rand*` / `FRandRange` / `VRandCone`. Only exceptions: local cosmetics and the player's
  own predicted weapon spread/recoil. Hub randomness gets its own seeded stream, not the run stream; only the casino
  uses it (the shop is static).
- **Cash and XP per enemy are flat**: the same in every round, with no round scaling (the round clear bonus is a flat
  amount too). Income grows only through more enemies and faster kills.
- **Level-up upgrades** (M2c; full design in `docs/PROGRESS.md`, **Design update — upgrade rework (M2c)**):
  - Every upgrade is a Stat Boost, an Ability (Legendary abilities are a sub-group) or a Weapon.
  - Rarity is rolled for each card on the run stream, not fixed per upgrade. A pick's size is the upgrade's base
    value × the rarity multiplier: ×1 / ×1.5 / ×2 / ×3 for Common / Rare / Epic / Legendary. Whole-unit upgrades
    (Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech) give +1 / +2 / +3 and have no Rare version (a Rare roll counts as Common).
  - Rarity weights per card: Stat Boosts and Weapons 100 / 20 / 6 / 1, Abilities 100 / 10 / 3 / 0.5. Weights,
    multipliers and per-pick values live in data, never in code.
  - Legendary abilities have no rarity: each card has a fixed chance (0.8%) to be one, and a pick adds one charge.
    Second Wind charges are used one at a time.
  - **No pick caps.** A stat that reaches a natural limit gets a secondary effect instead (Loaded Dice past 100%: a
    chance of a triple-damage hit; Powder Keg past 100%: a chance of a double-size blast).
  - **Weapons:** at most `MaxWeapons` (3, in data) different weapons per run; after that only the owned ones are
    offered, as upgrades. The first level-up of a run always includes one weapon card. The first pick gives the
    weapon; every pick adds weapon power equal to its rarity multiplier (the first pick adds the multiplier − 1). Each
    point of power is one upgrade step, defined per weapon. The rifle and pistol are not weapons in this sense.
  - **Every weapon:** attacks on its own; its damage is × the player's Damage attribute and can crit (Loaded Dice);
    Piercing Rounds doesn't apply; its kills are the player's (cash, XP orb, combo, Leech, Powder Keg); it doesn't act
    while the player is dead, in a safe zone or while the game is paused; its timing follows an exact schedule (as
    GA_Fire's held fire, Prompt 31b); it has no collision; a new run removes it.
- **Everything goes through GAS**: stats, buffs/debuffs, abilities and item effects are AttributeSets,
  GameplayEffects and GameplayAbilities with GameplayTags. Damage always goes through `GE_Damage`
  (SetByCaller `Data.Damage`).
- **Art and placeholders** (M2d):
  - **The target look is low poly** (faceted, flat shaded, clean blocks of colour, no texture detail) with
    high-quality shaders and lighting: coloured rim light, glowing emissives with bloom, coloured shadows, soft
    gradient skies. The reference is the game RevenBlade. The art pass is M5; until then placeholders follow this
    look.
  - **Placeholder models are made in Blender** (from Prompts 55–56 on), not from engine cubes, cones and cylinders.
    That includes the player (the robber) and every enemy: very basic low poly that gives the idea of the theme and
    the final look, with props such as a cap, a baton, a carbine and a shield. Older engine-shape placeholders are
    remade in Blender in Prompt 68.
  - No new animations yet: the models use the skeleton and the animations the game has today.
- **Combat only in the run map.** Enemies spawn, path and fight only in `L_Town_Graybox` (NavMesh covers the town
  only; the city map replaces it in Prompt 67). The Market District hub map is safe: no spawns, no NavMesh, no round loop, and players there have
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
  work; Blender MCP (`blender`, see **Blender** below) for placeholder models; file edits for C++.
- **Make one MCP call at a time**; wait for each result before the next (batch with `ProgrammaticToolset` instead).
- **Build after every C++ change**; fix errors before moving on.
- **Tests: area tests while working, the full suite once per prompt.** Add a test for new core rules.
  - While working on a prompt, run only the tests for the area being changed: `RunTests` with their names, or a
    narrower filter such as `StartsWith:BanditBaller.Round`.
  - Run the full suite (`StartsWith:BanditBaller`) once per prompt, right before the commit.
  - Docs-only prompts don't need a test run.
- **PIE checks: short when a test covers the rule.** If an automation test covers the rule, do one short PIE
  confirmation (start, look at it, a command or two, stop) instead of a scripted playthrough. Full playthroughs (for
  example several rounds or loops with a report) are for checkpoint prompts only, like M2's Prompt 28.
- **Commit after each working slice** (builds + works in editor). Never push without asking.

## Conventions
- C++ classes use the `GS` prefix (`AGSGameMode`, `UGSWaveDirector`); asset prefixes `BP_`, `DA_`, `DT_`, `GA_`,
  `GE_`, `ST_`, `WBP_`, `M_`, `MI_`, `IA_`, `IMC_`.
- Enemy names: Beat Cop (`BeatCop`) and Patrol Officer (`PatrolOfficer`) in code, assets, commands, logs and tests
  (`DA_Enemy_BeatCop`, `ST_PatrolOfficer`, `DA_PatrolCarbine`, …). Before Prompt 57 they were MeleeBandit /
  RangedBandit / BanditCarbine; older PROGRESS.md entries keep those names. Only the project name (BanditBaller, the
  module, `LogBanditBaller`, the `BanditBaller.` test prefix) keeps "Bandit".
- An enemy's look lives in its `UGSEnemyData`: `Mesh` (`SK_LowPolyBody`), `SlotMaterials` (one `MI_` per body slot,
  instances of `M_LowPoly`, whose `TelegraphAmount` drives the wind-up glow) and `MeshAttachments` (props, the same
  `FGSMeshAttachment` the player uses). A ranged enemy's gun is `Weapon->Mesh`, not an attachment.
- Tuning lives in a `UPrimaryDataAsset` subclass (`UGS…Settings`, `Const`), with an asset in `Data/` assigned on
  `BP_GSGameMode` or the owning Blueprint.
- Native gameplay tags only, in `Abilities/GSGameplayTags.*`.
- File-local constants and helper functions (anonymous namespace, `static`) get names unique to their file, e.g.
  `MaxDynamiteThrowsPerFrame`, not `MaxThrowsPerFrame`. The unity build compiles several `.cpp` files as one, so two
  files with the same local name break the build (it did in Prompt 52).
- Debug commands: `gs.` prefix, cheat-only, implemented in `UGSDebugLibrary` and registered in
  `GSCheatCommands.cpp`. Clients forward them through `ServerExecDebugCommand`. Debug toggles are `gs.Debug.*`
  cvars.
- Logs: `LogBanditBaller`, with bracketed prefixes for noisy systems (`[wave]`, `[cash]`, `[dmg]`, `[shot]`, `[ttk]`, `[xp]`, `[profile]`, `[upgrade]`, `[combo]`, `[blast]`, `[heal]`, `[shield]`, `[secondwind]`, `[gun]`, `[cards]`, `[wire]`, `[dynamite]`, `[coin]`, `[molotov]`, `[tomahawk]`, `[round]` one line per finished round, with health orbs and damage dealt by source).
- Tests: PIE-based latent tests in `Tests/GSGameplayTests.cpp`, built from the `GSTests` helpers (`BeginPIE`,
  `WaitUntil`, `Step`, `EndPIE`, …). `BeginPIE` opens `L_Town_Graybox` first (pass `HubMap` for the hub), since the
  editor starts in the hub. `EndPIE` only requests the end: before a second `BeginPIE` in the same test, wait for
  `GetPIEWorld() == nullptr`. When appending, keep the closing `#endif` last.
- Keep tests fast without changing what they check:
  - Wait on conditions (`WaitUntil`), not fixed times.
  - For long game-time waits (phase timers, spawns, the vacuum, jumps and falls), use `SpeedUpWorld(D)` at the start
    and `RestoreWorldSpeed()` at the end. That is the PIE world's time dilation plus no editor throttling. Use ×4 for
    timers and spawns, ×3 for movement.
  - Shorten real-time widget durations on the test's own widget instance with `SetFloatProperty` (e.g. the HUD's
    banner lengths).
  - Never edit tuning assets or gameplay code to make a test faster.
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
- Console commands: `BanditBallerTools.console_command` (runs in PIE when PIE is running, else in the editor world;
  since Prompt 56). The old way still works: `SlateInspectorToolset` → `Observe(ref "")`, `Snapshot` to find the
  textbox right after the "Cmd" combobox, then `Click` + `Type <ref> "<command>" submit=true`.
- Screenshots in PIE: `console_command("shot")` (game view only) or `"shot showui"` (with the HUD and the editor
  frame) writes `Saved/Screenshots/WindowsEditor/ScreenShotNNNNN.png`; read it with the Read tool. `slomo 0.1` first
  catches a jump or dash mid-move. MCP calls arrive within a frame or two now, so wait with PowerShell `Start-Sleep`
  between a command and the screenshot (camera blends take ~1 s).
- Debug execs for camera checks: `GSAim 1|0` (aim down sights), `GSSwapShoulder`, `GSRun <s>` (hold forward input),
  besides `GSAimAt`, `GSJump`, `GSFire`, `GSPress <InputTag>`.
- PIE: `remove_from_scene` is refused (teleport below KillZ instead). Round 1 Combat starts 3 s in, so send
  `gs.God 1` / `gs.Round.Pause 1` right after starting PIE. With the editor in the background PIE runs at ~3 FPS:
  set `/Script/UnrealEd.Default__EditorPerformanceSettings` `bThrottleCPUWhenNotForeground` false in memory for
  timing work, then restore it.
- `CaptureViewport` shows the editor world, not PIE; use `CaptureEditorImage` (base64 PNG; decode with PowerShell).
- Tests: stop PIE, then `AutomationTestToolset` → `DiscoverTests`. Area tests: `RunTests(["BanditBaller.Round.PhaseLoop",
  ...])` or `RunTestsByFilter("StartsWith:BanditBaller.Round")`. Full suite (once per prompt, before the commit):
  `RunTestsByFilter("StartsWith:BanditBaller")`, about 4 minutes (250 s for 70 tests in Prompt 59). The editor console also works:
  `Automation RunTests BanditBaller`. Each result has a `duration` for speed checks.
- Playing in PIE through MCP: still too slow to fight (each call is a round trip). Read live round state with
  `ObjectTools.get_properties` on `...UEDPIE_0_L_Town_Graybox...:PersistentLevel.BP_GSGameState_C_0` (`RoundPhase`,
  `WaveEnemiesTotal/Remaining/Alive`). Clear waves with `gs.God 1` + `gs.KillAll`, and read results from the `[round]`
  / `[upgrade]` log lines (the log file flushes a few seconds late). Click run-summary buttons with
  `SlateInspectorToolset.Click` on the "Return to Market" button ref.

## Blender
- Blender **5.3** at `C:\Program Files\Blender Foundation\Blender 5.3`, driven through the **MCP for Blender** server
  (`mcp-for-blender`, started with `uvx`; entry `blender` in `.mcp.json`, with `BLENDER_MCP_SAFE_MODE=1` and
  `DISABLE_TELEMETRY=true`). Setup and versions: `docs/PROGRESS.md`, **Blender MCP setup**.
- **Before a prompt that makes models, Blender must be open with its MCP server started:** 3D viewport → `N` →
  **MCP for Blender** tab → **Start MCP Server** (localhost, port 9876). The add-on ("MCP for Blender") must be enabled
  under Edit → Preferences → Add-ons. If it isn't running, ask the user to start it; don't work around it.
- **Session start check:** confirm both `unreal-mcp` and `blender` are connected (the `blender` server's tools load
  only when Claude Code starts; Blender itself is needed only when a prompt uses it).
- **Save the open .blend file before running code in Blender** (`execute_blender_code` runs Python in the open
  scene, and nothing undoes it). Safe mode blocks file access, network, subprocesses and code that keeps running.
- **Only this project's Claude Code session uses Blender's MCP server.** Don't connect a second client (Claude
  Desktop, Cursor, another session) to it, and keep it on localhost: the socket has no authentication.
- **The Claude desktop app runs as a Windows (MSIX) package**: anything it starts that *creates* files under
  `%APPDATA%` / `%LOCALAPPDATA%` gets a private copy in `%LOCALAPPDATA%\Packages\Claude_pzs8sxrjxfjjc\LocalCache`.
  So run uv installs and updates (`uvx mcp-for-blender install-addon`, `uvx mcp-for-blender@latest update`, new Python
  versions) in a normal PowerShell from the Start menu, not from this session or the app's terminal panel. Otherwise
  uv's Python links break and the add-on lands where Blender can't see it.

## Placeholder models
"Make it in Blender" means this section. Set up in Prompt 56; details and the reasons in `docs/PROGRESS.md`
(**Prompt 56**).
- **Style:** very basic low poly that gives the idea of the theme (cops and robbers) and of the final look. Flat shaded
  (every face `use_smooth = False`, FBX smoothing "Face"), clean blocks of colour from a few plain materials, no
  textures, no UVs needed.
- **Budget:** props up to about 300 triangles; characters and vehicles up to about 2,000.
- **Size and axes:** real size. X forward, Z up after import: Blender +X = Unreal +X, Blender +Z = Unreal +Z, Blender
  +Y = Unreal −Y. Props point along +X (a barrel, a baton). Bodies face −Y in Blender (Unreal +Y, as the mannequins;
  the character's mesh component turns them to face +X).
- **Origin:** at the base (a cap, a crate) or the grip (a weapon). Props worn on a body sit on their bone's joint.
- **Names:** meshes `SM_<Name>` / `SK_<Name>`, materials `MI_<Name>` (instances of `M_LowPoly`). A Blender material's
  name becomes the Unreal slot name (`Rubber`, `Top`, …).
- **Folders:** Blender sources `Art/Blender/<Group>.blend` (`Props.blend`, `LowPolyBody.blend`; one file per group);
  exports `Art/Export/<Kind>/<Asset>.fbx`; Unreal `/Game/Art/Placeholders/<Kind>/` with Kind = Characters, Props,
  Enemies, Weapons or City, and `/Game/Art/Placeholders/Materials/` for `M_LowPoly` and the `MI_` colours.
- **Units (important):**
  - Static props: the file's Unit Scale is 1.0 and a Blender unit is a metre.
  - Anything on the `SK_Mannequin` skeleton (`LowPolyBody.blend` and everything modelled around the body): Unit Scale
    0.01 and a Blender unit is a centimetre (lengths still display in metres). Otherwise the FBX puts a ×100 scale on the
    `root` bone and the animations shrink the body 100 times. The armature object must be named `root` (it becomes the
    root bone).
- **The routine** (FBX; glTF was tried and is not used, see PROGRESS):
  1. Blender open with its MCP server; save the .blend before running code.
  2. Build with `execute_blender_code`. Bodies: one block per part, each vertex weighted 1.0 to exactly one bone (no
     weight painting, nothing stretches).
  3. Check in Blender: `look` (angles, solid) and the triangle count (`calc_loop_triangles`); bodies also posed.
  4. Export with `bpy.ops.export_scene.fbx(use_selection=True, apply_unit_scale=True,
     apply_scale_options='FBX_SCALE_NONE', mesh_smooth_type='FACE', bake_anim=False)`, with the object moved to the
     world origin for the export (props) and `object_types={'MESH'}`. Skeletal: select `root` + the mesh,
     `object_types={'ARMATURE','MESH'}`, `add_leaf_bones=False`, `primary_bone_axis='Y'`, `secondary_bone_axis='X'`,
     `use_armature_deform_only=False`, `armature_nodetype='NULL'`, `use_mesh_modifiers=False`. Run the operator inside
     `bpy.context.temp_override(window=…, area=<the VIEW_3D area>, region=…)`.
  5. Import a **new** asset with `StaticMeshTools.import_file` / `SkeletalMeshTools.import_file` (FBX,
     `import_materials=False`; skeletal: `skeleton` = `SK_Mannequin`, then `assign_physics_asset` `PA_Mannequin`).
     **Update an existing** one with `BanditBallerTools.import_mesh_file` (Interchange reimport in place: keeps the
     skeleton, physics asset, materials and every reference). Never delete and re-import a mesh that is in use.
  6. Assign the `MI_` colours per slot (`set_material`), check Nanite is off, `save_assets`.
  7. Check in Unreal: `get_bounds` (size in cm) and `get_triangle_count`; skeletal: `BanditBallerTools.describe_ref_pose`
     must show `root` scale 1 and `pelvis` at (0, 2.38, 98.69) like `SKM_Quinn_Simple`; then look at it animated (PIE,
     or an editor actor playing `MM_Idle`).
- **Worn props** (a mask, a bag, a cap): model them in `LowPolyBody.blend` around the body with the origin on the
  bone's joint, export as static meshes, and list them in the character's `MeshAttachments` (`FGSMeshAttachment`,
  `bModeledInMeshSpace` on): they then sit exactly as modelled, with no hand-tuned offsets.

Enabled plugins of note: ModelContextProtocol + AllToolsets, GameplayAbilities, StateTree, GameplayStateTree,
EnhancedInput, CommonUI, ModelingToolsEditorMode, PCG. No online subsystems yet.
