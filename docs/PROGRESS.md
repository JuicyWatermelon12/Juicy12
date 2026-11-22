# BanditBaller — Progress

Read this at the start of every session. After finishing a prompt, add a short entry at the end of the current
milestone section (what was built, new assets/commands, known issues) and update **Next up**.

## The game

Third-person roguelike shooter set in an off-grid bandit gambling town (UE 5.8, C++ + GAS). **Single-player
first**; 2–4 player listen-server co-op is planned for **M6** (see **Later — co-op (M6)**).
- **Hub:** the game starts in the safe **Market District** hub (shop + casino), its own map. A "Start Run" portal
  takes the player to the town map (`OpenLevel`).
- **Run:** PreRound → Combat → RoundClear → PreRound … with harder rounds, until the player dies. Bandits drop cash
  and XP as pickups, collected during the run. Cash and XP per enemy are flat (the same every round); income grows
  through more enemies and faster kills.
- **After death:** a run summary, then back to the hub to spend or gamble the cash.
- **Playthrough progression:** cash, owned items and the gear loadout (saved profile). Everything owned is active or
  usable from the start of the next run, depending on the item type (M3):
  - Usable items (consumables) can be bought again to restock and are used up when used in a run.
  - Gear is bought once and goes into a loadout slot in the hub (swapping out what was there). The loadout applies
    at the start of the next run.
  - Other types (abilities, powerups) are decided in M3.
  - The shop is a static, fixed catalogue (never random). Casino prizes are owned items too; bets risk cash only.
- **Prestige** (later milestone): an optional reset that wipes cash and everything bought or won, in exchange for
  bonuses for the next playthrough based on the total cash earned. Only prestige rewards survive it.
- **Per-run progression** (reset on death): XP, level, and level-up upgrades.
  - Each level-up fully heals, adds MaxHealth, and offers 3 random upgrades in four rarity tiers (Common, Rare, Epic,
    Legendary).
  - The upgrade pool grows from 4 to 15 in **M2b**, a block of eleven upgrades after M2 and before the market (M3);
    see **Design update — M2b upgrades**.
  - The player must take one (no skip) but can reroll the choices a limited number of times per run (3 to start).
  - The game pauses while the player chooses.

The design changed after M2 Prompt 10; see **Design change — permanent cash, hub market, level-up upgrades**. The
scope then changed to single-player first; see **Scope change — single-player first**. Five more decisions came
before M3; see **Design update — decisions before M3**. The market, prestige and economy were settled after that;
see **Design update — market, prestige and economy**. After M2, eleven more level-up upgrades were planned as block
**M2b**, before M3; see **Design update — M2b upgrades**.
The in-run market is gone from the round loop (Prompt 14). The streamed district and its debug teleport stay until
the hub map replaces them.

**M2 — the run loop is complete** (rounds, waves, scaling, economy, progression; Prompts 1–28 and 26b; 11–13 and 17
were design / scope updates and permanent cash). See **M2 checkpoint** under Prompt 28. **Next: M2b**, eleven more
level-up upgrades (Prompts 29–40; 29 is the docs-only plan, then Dash Charge (30), Fire Rate + Magnet (31) and the
held-fire fix (31b), Bigger Mags + Leech (32), Loaded Dice (33), Piercing Rounds (34), Powder Keg (35), Floating Shield (36) and Second Wind (37); all done). **Then M3** (shop, owned items, gear loadout,
two-layer save); not planned in detail yet. Health orbs (35b) were added between 35 and 36; they aren't an upgrade. Powder Keg became a chance to explode in 35c.

State at the time of writing: `main` after "Second Wind upgrade" (Prompt 37), not pushed. The build is clean and all 45 automation tests pass in about 100 s. The game and the editor start in the hub (`L_Hub_Graybox`). Walk into the Start Run portal to
play a run in `L_Town_Graybox` (300 × 300 m since Prompt 26b). Death ends the run (GameOver) and shows the run summary;
"Return to Market" (button, R, gamepad Start / Y) fades out and goes back to the hub.

| Step | What | Commit |
|---|---|---|
| M0 | Tooling, framework classes, greybox town | 73e0e8f … a4a857b |
| M1 1–8 | GAS, input/camera, weapons, health/death, slide/dash, melee + ranged bandits, debug tools/tests | 0edf843 … 693adde |
| Design change | Market District becomes a separate streamed area | 470b78c |
| M2 1 | Round phase state machine | 8572a84 |
| M2 2 | Run seed + run stats (+ respawn input fix) | 0ef5ae5, 4227abc |
| M2 3 | Spawn ring around the player | c571f74 |
| M2 4 | Wave table | 2dc5201 |
| M2 5 | Endless (budget) waves, alive cap, spawn telegraph | 515cf59 |
| M2 6 | Enemy scaling per round | 263bd94, fabf651 |
| M2 7 | Cash pickups and magnet | 64e9ac2 |
| M2 8 | XP on kill and player levels, HUD level + XP bar | b3dda13, 4ba1b32 |
| M2 9 | Level-up rewards (MaxHealth stack + full heal) | 371cb6e |
| M2 10 | Progression feedback (XP popups, level-up banner, pulses, glow, sound) | 72534d1 |
| M2 11 | Design change: permanent cash (profile), Market District as hub between runs, level-up upgrades (docs only) | 0b2b6f5 |
| M2 12 | Permanent cash: profile subsystem + save; cash moved off PlayerState | a2dc3b7 |
| M2 13 | Scope change: single-player first; co-op moves to M6 (docs only) | f1db8b5 |
| M2 14 | Run loop cleanup: in-run market phases removed (RoundClear → PreRound) | 58f29b7 |
| M2 15 | Upgrade data + stat upgrades (ReloadSpeed, Damage, MoveSpeed) | 9dbccbf |
| M2 16 | Extra Jump upgrade (ExtraJumps → JumpMaxCount, consistent air jumps, dash jump queue) | 42a874e |
| M2 17 | Design update: decisions before M3 (docs only) | 14da7c0 |
| Design update | Market, prestige and economy decisions (docs only; not a numbered step) | bb1ded0 |
| M2 18 | Upgrade offers: 4 rarity tiers, DA_UpgradePool, offer queue, choose / reroll, simulator (log only) | bd0e14c |
| M2 19 | Level-up choice screen: pause, cards, input lock, keys / gamepad / mouse, reroll button | 9bd1d3b |
| M2 20 | XP orbs: XP dropped as a pickup and granted at collection | 864d029 |
| M2 21 | Map boundary: invisible walls + 75 m ceiling around the town (pawns only) | 92a7bf0 |
| M2 22 | Kill combo: window, cash multiplier steps, ends on damage, pauses with the game, HUD, best combo | cfcf0fc |
| M2 23 | Round-clear bonus (base + no damage + fast clear, flat) and the RoundClear summary | b04209d |
| M2 24 | Round HUD: round / phase / countdown / enemies left, upgrade icons, queued round banners | 6be7fe1 |
| M2 25 | Hub map L_Hub_Graybox + AGSHubGameMode (safe zone, no round loop); startup map | 815b5db |
| M2 26 | Start Run portal (hub → town, run starts on load, seed hand-off); town cleanup (district sublevel, market travel removed) | 142a81e |
| Fix | Death screen R returns to the hub instead of restarting the town | 7b10eb2 |
| M2 26b | Bigger town: 300 × 300 m, streets, outskirts cover, a landmark per side, NavMesh without islands | 97d2045 |
| M2 27 | Run end: GameOver on death (wave stopped), run summary, Return to Market → hub | d2891c4 |
| M2 28 | Tests for the M2 rules, two PIE loops, M2 checkpoint ("M2 complete") | b3dbae2 |
| Workflow | Faster test runs: area tests while working, full suite once per prompt, short PIE checks; five slowest tests sped up (suite 118 → 80 s) | 84bc97f |
| M2b 29 | Design update: the M2b upgrade list (11 upgrades, pool 4 → 15) and its rules; Prompts 30–40 planned (docs only) | 691d54e |
| M2b 30 | Dash Charge upgrade (ExtraDashCharges → GA_Dash max charges, live); the player starts with 1 dash charge | 252bd0b |
| M2b 31 | Fire Rate (+8% FireRate) and Magnet (+15% PickupRadius) upgrades, both Common | 6a78abe |
| M2b 31b | Fix: exact fire rate for held fire (the burst stays inside GA_Fire on an exact shot schedule) | 0e245f6 |
| M2b 32 | Bigger Mags (magazine × MagazineSizeMultiplier, rounded up) and Leech (heal HealthOnKill per kill) | c3d721e |
| M2b 33 | Loaded Dice: CritChance, one crit roll on the run stream, double damage, critical damage numbers | 0133f66 |
| M2b 34 | Piercing Rounds: Pierce, shots pass through targets (once each, full damage, own crit and number), walls stop them | 2ee3f68 |
| M2b 35 | Powder Keg: the player's kills explode (radius / damage per stack × Damage), chains 0.1 s apart, blast kills are the player's | a650a64 |
| M2b 35b | Health orbs: a 20% chance per player kill (run stream), heal 15 at pickup, ignored at full health, vacuumed at RoundClear | 5a7086a |
| M2b 35c | Powder Keg: a 3% chance per stack that a player kill explodes (run stream), fixed blast (30 × Damage, 3 m), MaxStacks 34 | 07a8341 |
| M2b 36 | Floating Shield: +1 shield per pick; a ready shield cancels a whole hit in the damage path and recharges 10 s on its own; dash i-frames first | 2bbe7d4, ae5d083 |
| M2b 37 | Second Wind: a killing blow uses one instead (30% Health, 2 s invulnerable, one stack gone); flash, sound, banner | see git log |

Paths below: C++ under `Source/BanditBaller/`, assets under `Content/BanditBaller/` (`/Game/BanditBaller/`).

---

## M0 — Project setup (done)

- **Tooling**: git + LFS (`.gitattributes`), `.gitignore`, `.vsconfig`, `.mcp.json` (unreal-mcp), `CLAUDE.md`,
  content folder skeleton. `Config/DefaultEngine.ini`: `CommonGameViewportClient`, `GlobalDefaultGameMode` =
  `BP_GSGameMode`, `EditorStartupMap` / `GameDefaultMap` = `L_Town_Graybox`.
- **Framework classes** (`Core/`): `AGSGameMode`, `AGSGameState`, `AGSPlayerState` (Cash, `OnCashChanged`,
  `AddCash` / `TrySpendCash`), `AGSPlayerController`, `AGSCharacter`; `Abilities/GSAttributeSet`.
  Blueprints in `Core/`: `BP_GSGameMode`, `BP_GSGameState`, `BP_GSPlayerState`, `BP_GSPlayerController`, `BP_GSCharacter`.
- **Level** `Maps/L_Town_Graybox`: 150×150 m slab (300 × 300 m since Prompt 26b), central plaza, 6 shacks + crates as cover, `EnemySpawn` Target
  Points at the edges, PlayerStart, NavMesh. The template `Lvl_ThirdPerson` GameMode override was cleared.

## M1 — Core combat (done)

Commits were labelled "M1: …" to "M7: …" for prompts 1–7 and "M1 complete" for prompt 8.

1. **GAS foundation** (0edf843): ASC (`UGSAbilitySystemComponent`) + `UGSAttributeSet` on `AGSPlayerState` (Mixed
   replication); `AGSCharacter` is the avatar and syncs MoveSpeed → MaxWalkSpeed. Native tags in
   `Abilities/GSGameplayTags.*`. `UGSGameplayAbility` (ActivationInputTag), `UGSAbilitySet` (granted once per
   PlayerState). Assets `Abilities/GE_InitPlayerStats`, `GE_Damage`, `DA_PlayerAbilitySet`.
2. **Input, camera, movement** (01b8693): Enhanced Input `Input/IMC_Default` + `Input/Actions/IA_*` (Fire, Aim,
   Reload, Slide, Dash, SwapWeapon, ShoulderSwap); `Input/DA_PlayerInputConfig` (`UGSInputConfig`) routes inputs to
   `Ability.*` tags. Over-the-shoulder spring arm, ADS FOV/arm blend, shoulder swap, coyote time; tuning in
   `Data/DA_PlayerMovementSettings` (`Core/GSMovementSettings.h`). `UI/WBP_Crosshair`.
3. **Weapons** (138a0ac): `Combat/GSWeaponData.h` (`Weapons/DA_Rifle`, `DA_Pistol`), `AGSWeapon`,
   `UGSWeaponComponent` (2 slots, per-weapon ammo, replicated). `GA_Fire` / `GA_Reload` / `GA_SwapWeapon`
   (`UGSGameplayAbility_Fire/Reload/SwapWeapon`): hitscan on Visibility, hip/ADS spread, recoil, auto-reload.
   `UI/WBP_HUD` (`UGSHUDWidget`). `AGSTargetDummy` / `Enemies/BP_TargetDummy` (3 in the plaza).
4. **Health, hit feedback, death** (239eeb6): `Combat/GSHealthComponent` (shared death flow for players, dummies,
   enemies; reports kills to the GameMode). `UI/WBP_DeathScreen` (`UGSDeathScreenWidget`; R restarted via
   `?Restart`; replaced by the run summary in Prompt 27). HUD health bar, damage numbers, hit marker, red edge flash +
   low-health pulse;
   `Characters/CS_PlayerHit` (`UGSHitCameraShake`).
5. **Slide and dash** (fe0de61, 1f57eb1): `GA_Slide` / `GA_Dash` (`UGSGameplayAbility_Slide/Dash`), cooldowns
   `GE_Cooldown_Slide` / `GE_Cooldown_Dash` (`UGSCooldownEffect*`), dash i-frames (`State.Invulnerable`), 2 dash
   charges. Tuning `Data/DA_SlideSettings` / `DA_DashSettings` (`Abilities/GSMovementAbilitySettings.h`).
   `UI/WBP_AbilityIcon` (`UGSAbilityIconWidget`). CharacterMovement ticks after the character (`bTickBeforeOwner = false`).
6. **Melee bandit** (f16afce): `AI/GSEnemyCharacter` (own ASC + attribute set), `AI/GSEnemyAIController`
   (StateTree + sight), `AI/GSEnemyData.h`, StateTree schema/tasks (`AI/GSEnemyStateTree*`) and a builder command
   that generates the trees from code. `GA_MeleeAttack` (`UGSGameplayAbility_MeleeAttack`, wind-up telegraph).
   Assets `Enemies/DA_Enemy_MeleeBandit`, `BP_Enemy_MeleeBandit`, `DA_EnemyAbilities_MeleeBandit`, `ST_MeleeBandit`,
   `Materials/M_Enemy_MeleeBandit`, `UI/WBP_EnemyHealthBar` (`UGSEnemyHealthBarWidget`).
7. **Ranged bandit** (794788b): `GA_EnemyFire` (`UGSGameplayAbility_EnemyFire`: 0.4 s laser aim telegraph, 4-shot
   burst), `ST_RangedBandit` (move to 15–25 m with line of sight → burst → strafe; back off under 8 m). Assets
   `Enemies/DA_Enemy_RangedBandit`, `BP_Enemy_RangedBandit`, `DA_EnemyAbilities_RangedBandit`,
   `Weapons/DA_BanditCarbine`, `Materials/M_Enemy_RangedBandit`, `M_EnemyLaser`. Enemy shots use an object query
   because the player ignores the Visibility channel.
8. **Debug & tuning tools + tests** (693adde): `Core/GSDebugLibrary.*` (shared implementation),
   `Core/GSCheatCommands.cpp`, `AGSPlayerController::ServerExecDebugCommand`; Python MCP toolset
   `Content/Python/banditballer_toolset` (registered by `Content/Python/init_unreal.py`); automation tests in
   `Tests/GSGameplayTests.cpp`.

## Design change — separate Market District (470b78c)

- The shop/casino moved out of the town plaza into `Maps/L_MarketDistrict_Graybox`: a Blueprint-streamed sublevel
  (unloaded by default) at X = +200000 with a walled 50×50 m square, `Casino` building, 4 `MarketStall`s, Target Points
  `MarketArrival` / `MarketExitPortalSpot`, warm tint, no NavMesh, no spawns.
- The town plaza got Target Points `TownReturn` and `MarketPortalSpot`. The old plaza boxes are untagged `Cover_*`.
- `AGSGameState`: `EGSArea` (Town, MarketDistrict), replicated `CurrentArea` + `OnAreaChanged`, `bMarketOpenThisRound`.
- `AGSGameMode::TravelToMarketDistrict` / `ReturnToTown`: server fades, streams the level and moves the party.
- Enemies ignore players while `CurrentArea != Town`; spawning is refused there.
- Commands `gs.Market.Teleport` / `gs.Market.Return` (temporary), `gs.Editor.MarketDistrictLevel`
  (`Core/GSEditorLevelCommands.cpp`). Test `Map.TownAndMarketDistrict`.

## M2 — The run loop (complete: Prompts 1–28)

### Prompt 1 — Round phase state machine (8572a84)
- `AGSGameMode::SetPhase` (the only way phases change; logs `Round N: Old -> New`) and `AdvancePhase` (transition
  table) over `EGSRoundPhase` (`Core/GSGameState.h`): WaitingToStart, PreRound, Combat, RoundClear,
  MarketDecision, MarketVisit, GameOver. Round 1 starts on `StartPlay`.
- Timings in `UGSRoundSettings` (`Core/GSRoundSettings.h`) → `Data/DA_RoundSettings`, set on BP_GSGameMode.
- `AGSGameState` replicates RoundNumber, RoundPhase, PhaseEndTime (`GetPhaseTimeRemaining()`), and fires
  `OnPhaseChanged` / `OnRoundChanged` on server and clients.
- Commands `gs.Round.Skip`, `gs.Round.Set`, `gs.Round.Pause`, cvar `gs.Debug.Round`. Test `Round.PhaseLoop`.

### Prompt 2 — Run seed and run stats (0ef5ae5)
- `AGSGameMode` owns a seeded `FRandomStream`. The seed comes from `-GSSeed=N`, otherwise it is random, and it is
  logged (`Run started: seed N`). `gs.Run.Seed <N>` restarts the run with that seed.
- `Core/GSRunRandom.*` (`UGSRunRandom`) is the only way gameplay rolls dice.
- `Core/GSRunStats.*` (`UGSRunStats`, a replicated component on `AGSGameState`) tracks rounds cleared, kills
  (total and per type), cash earned/spent, damage dealt/taken and run time. `gs.Run.Stats` prints them.
  Test `Run.SeedAndStats`.
- Fix 4227abc: after dying and pressing R the player couldn't move. `PC::BeginPlay` now sets game-only input.
  Test `Player.RestartRestoresInput` (`Player.ReturnToHubRestoresInput` since Prompt 27).

### Prompt 3 — Spawn ring around the player (c571f74)
- `AI/GSSpawnLocationFinder.*` (`UGSSpawnLocationFinder::FindSpawnLocations`), settings `AI/GSSpawnSettings.h` →
  `Data/DA_SpawnSettings`.
- Per point:
  1. Pick a random point in the ring around the player.
  2. Project it onto the NavMesh.
  3. Reject it if it is out of the ring, unreachable, too close to another chosen point, or on-screen (that last
     check applies to the first half of the attempts only).
- If every attempt fails, the farthest `EnemySpawn` Target Point is used as a fallback.
- Refuses to spawn in the Market District. `gs.Spawn` uses it, and `gs.Debug.Spawns` draws the ring and the chosen
  points. Test `Spawn.LocationFinder`.
- PIE result: 90 spawns at the center, a wall and a corner all came from the ring, with 0 fallbacks.

### Prompt 4 — Wave table (2dc5201)
- `AI/GSWaveDirector.*`: `UGSWaveDirector` component on `AGSGameMode`; row struct `FGSWaveRow` →
  `Data/DT_Waves` (Round1–Round5).
- Spawns the wave over time; Combat ends when everything has spawned and died (the 180 s timer is only a safety
  net). Leaving Combat early cancels the wave and removes leftovers.
- The enemies placed by hand in M1 were removed from the level; rounds spawn them now.
- `AGSGameState` mirrors WaveEnemiesTotal / Remaining / Alive. Test `Round.WaveDirector`.

### Prompt 5 — Endless scaling and alive cap (515cf59)
- Rounds past the last DT_Waves row are generated from a budget in `UGSWaveSettings` (`AI/GSWaveSettings.h`) →
  `Data/DA_WaveSettings`.
- The director buys random unlocked, affordable enemies from `EnemyPool` (run stream), using each enemy's
  `SpawnCost` / `MinRound` from `UGSEnemyData`.
- The alive cap (`MaxAliveEnemies`, overridable with `gs.Wave.MaxAlive`) counts alive plus telegraphing enemies;
  extra spawns wait for room.
- `AI/GSSpawnTelegraph.*` (`AGSSpawnTelegraph`): a replicated ground disc that grows for 0.75 s before each spawn.
- Test `Round.WaveBudget`.
- PIE results: round 8 had 17 enemies at 95 fps, round 15 had 29 at 86 fps, and round 25 held at the 30-alive cap.

### Prompt 6 — Enemy scaling per round (263bd94, fabf651)
- `Core/GSDifficultyCurve.h` (`UGSDifficultyCurve`) → `Data/DA_DifficultyCurve`: Health and Damage multiplier
  curves by round.
- `UGSEnemyRoundScalingEffect` (`Abilities/GSGameplayEffects.h`) → `Abilities/GE_EnemyRoundScaling`: infinite,
  MultiplyCompound on MaxHealth / Damage from SetByCaller `Data.HealthMultiplier` / `Data.DamageMultiplier`.
  `AGSGameMode::ApplyEnemyRoundScaling` applies it on enemy init (server) and refills Health.
- `AGSGameState` replicates `EnemyHealthMultiplier` / `EnemyDamageMultiplier` (shown in `gs.Debug.Round`).
- Tests `Round.DifficultyScaling` and `Balance.TimeToKill`. The latter fires the real rifle and pistol at rounds
  1/5/10 against both bandits and logs `[ttk]` lines.

### Prompt 7 — Cash pickups and magnet (64e9ac2)
- `Economy/GSCashSettings.h` (`UGSCashSettings`) → `Data/DA_CashSettings`, set on BP_GSGameMode.
- `Economy/GSCashPickup.*` (`AGSCashPickup`): a pooled, replicated coin or bill.
- `Economy/GSCashPickupSubsystem.*` (`UGSCashPickupSubsystem`, world subsystem): pool, drop, collect, vacuum.
- On enemy death, `AGSGameMode::HandleEnemyKilled` drops `UGSEnemyData::CashValue`. It is split into a few pieces
  (gold coins, or green bills for pieces worth $4+), scattered with the run stream and tossed onto the floor.
- New attribute `PickupRadius` (300 cm from `GE_InitPlayerStats`). Pieces inside it fly to a living player and are
  paid through `AGSGameMode::AwardCash`, which calls `AGSPlayerState::AddCash`, adds run-stats CashEarned and sends
  the "+$N" popup to that player's client (`ClientNotifyCashCollected`).
- Pieces expire after 20 s and blink for the last 4 s. Entering RoundClear sends every piece to the nearest
  living player.
- Cash now replicates: `OnRep_Cash` fires `OnCashChanged` on clients.
- HUD: `CashText` counter at the top right (WBP_HUD) plus merged green "+$N" popups.
- Assets `Materials/MI_CashCoin`, `MI_CashBill`. Cvar `gs.Cash.Lifetime`. Test `Economy.CashPickups`.

### Prompt 8 — XP on kill and player levels (b3dda13)
- `Core/GSProgressionComponent.*` (`UGSProgressionComponent`, created on `AGSPlayerState`, `GetProgressionComponent()`):
  - Holds Level (starts at 1), CurrentXP, XPForNextLevel, TotalXP and MaxLevel. They replicate as one struct
    (`FGSProgressionState`).
  - `AddXP(N)` (authority only) is the only way XP goes up. A big gain levels up several times in a row and the
    leftover carries over. At max level CurrentXP stays 0.
  - `OnLevelUp(NewLevel)` fires once per level gained, then `OnXPChanged(CurrentXP, XPForNextLevel, Delta)`, on
    the server and on clients.
  - `ResetProgression()` runs on every new run (`AGSGameMode::StartRun`, so `gs.Run.Seed` too).
- `Core/GSXPCurve.h` (`UGSXPCurve`) → `Data/DA_XPCurve`, set as `XPCurve` on BP_GSGameMode.
  - `XPToReachLevel`: X = level, Y = XP needed from the previous level. MaxLevel is 50.
  - New assets get keys 2–50 at 100 × 1.25^(L−2) from the constructor. DA_XPCurve still uses those defaults, so
    the keys only save into the asset once you edit them.
- `UGSEnemyData::XPValue`: MeleeBandit 10, RangedBandit 15.
  - `AGSGameMode::HandleEnemyKilled` → `AwardXP(killer's PlayerState, XPValue)` grants it straight to the killer,
    with no pickup.
  - XP granted = round(XPValue × the new `XPGain` attribute).
  - `AwardXP` also feeds run stats.
- New attribute `XPGain` (1.0 from GE_InitPlayerStats; the set's constructor also defaults it to 1).
- `UGSRunStats`: `XPEarned` (all XP granted, after XPGain) and `LevelReached` (highest player level). Both show in
  `gs.Run.Stats`.
- Commands `gs.XP.Add <N>` (raw XP, XPGain not applied) and `gs.XP.SetLevel <N>`.
  - SetLevel resets the player, then AddXPs exactly the total for level N, so it lands on 0 XP into N.
  - SetLevel's XP isn't counted in run-stats XPEarned.
- `get_player_attributes` now includes Level, XP, XPForNextLevel and TotalXP.
- Test `Progression.XPAndLevels`.
- PIE check:
  - 3 melee + 2 ranged kills = 60 XP.
  - `gs.XP.Add 300` → level 3, 135/156 ("LEVEL UP x2").
  - With XPGain 2 a melee kill gave +20.
  - `gs.XP.SetLevel 10` → 0/745.
  - +100M XP → level 50, 0/0.
  - The run stats matched.
- No level-up rewards yet (next prompt).

### Level / XP on the HUD (4ba1b32)
- `UGSHUDWidget` has new optional bindings, all driven by `UGSProgressionComponent::OnXPChanged` / `OnLevelUp`
  with no polling:
  - `LevelText`: "LV n".
  - `XPBar`: progress towards the next level, full at max.
  - `XPText`: "x / y XP", or "MAX" at max level.
- On a level-up, LevelText "punches" (scales up and back). Tuning: `LevelUpPunchScale` 1.6 and
  `LevelUpPunchDuration` 0.4 s on WBP_HUD's class defaults.
- WBP_HUD layout, top left (cash stays top right):
  - Gold "LV n" (Roboto Bold 30) at (40, 36).
  - Blue 320×12 bar at (40, 80).
  - "x / y XP" (16) at (40, 96).
- Test hook: `GetDisplayedLevel()` / `GetDisplayedXPPercent()`. `Progression.XPAndLevels` checks the HUD after
  the double level-up.
- PIE check: `gs.XP.Add 160` showed LV 2 with "60 / 125 XP" and the bar about half full. `gs.XP.SetLevel 50`
  showed LV 50 with a full bar and "MAX".

### Prompt 9 — Level-up rewards (371cb6e)
- `Core/GSLevelRewards.h` (`UGSLevelRewards`) → `Data/DA_LevelRewards`, set as `LevelRewards` on BP_GSGameMode:
  - `EveryLevelEffects`: [GE_LevelUp_MaxHealth].
  - `LevelSpecificRewards` (Level → effects): empty for now. Future per-level gains go here.
  - `FullHealEffect`: GE_FullHeal.
- `Abilities/GE_LevelUp_MaxHealth` (`UGSLevelUpMaxHealthEffect`): infinite, stacks by target with no limit,
  Additive +10 MaxHealth per stack (edit the modifier magnitude on the asset). One stack per level gained.
- `Abilities/GE_FullHeal` (`UGSFullHealEffect`): instant. It overrides Health with the target's MaxHealth, captured
  live (not snapshotted) when it is applied.
- `UGSProgressionComponent::AddXP` (server), when levels were gained:
  1. Applies the rewards for each level gained.
  2. Applies one full heal. Dead players get the stacks but no heal.
  3. Then fires OnLevelUp / OnXPChanged, so listeners already see the new MaxHealth.
- Nothing checks the phase or area, so RoundClear and the Market District work the same (tested).
- Log line: `[xp] … level-up rewards for levels A-B: MaxHealth X, Health Y`.
- `ResetProgression` (new run, `gs.Run.Seed`, `gs.XP.SetLevel`) removes every lasting reward through stored
  handles.
- New `UGSAttributeSet::PostAttributeChange`: if MaxHealth drops below Health, Health is clamped down (server).
- HUD: the health bar already follows MaxHealth through the attribute delegates. Test hooks
  `GetDisplayedHealth()` / `GetDisplayedMaxHealth()` were added.
- Test `Progression.LevelUpRewards`.
- PIE check:
  - 100/100 → `gs.Damage.Self 40` → 60/100.
  - `gs.XP.Add 225` → level 3 at 120/120. The HUD showed "120 / 120" with a full bar, "LV 3" and "0 / 156 XP".

### Prompt 10 — Progression feedback (72534d1)
- Audit of the existing LevelText / XPBar / XPText: already event-driven (`AddUniqueDynamic` on
  OnXPChanged / OnLevelUp).
  - The per-tick `TryBindProgression` only re-binds when the PlayerState changes; it never reads values.
  - Several level-ups in one frame are handled: OnLevelUp fires once per level (also on clients from `OnRep_State`)
    and each refresh reads the final state. Nothing needed fixing.
- **"+N XP" popups** reuse the painted popup pool (`bXP` entries): light blue, font 17, smaller than the cash
  popup's 24, 120 cm up.
  - The amount comes from OnXPChanged's Delta.
  - The position comes from `UGSHUDWidget::NotifyKill` (called by `AGSPlayerController::ClientNotifyDamageDealt`
    on a kill). The two pair up if they arrive within `XPPopupPairWindow` 0.25 s of each other, in either order.
    (Since Prompt 20 it is `NotifyXPSource`, called where an XP orb is collected; kills give no XP.)
  - XP with no kill (`gs.XP.Add`) pops above the player.
- **Level-up banner** (built with MCP in WBP_HUD, top center at 22% height):
  - VerticalBox `LevelUpBanner` holding `LevelUpTitle` ("LEVEL UP!", gold 54), `LevelUpLevelText` ("Level N") and
    `LevelUpRewardText` (green). The last two are BindWidgetOptional.
  - It fades in with a pop (0.15 s from ×1.3) and fades out over 0.5 s; it lasts 2.5 s in total.
  - Level-ups while it is showing merge into it ("Level 4", "+30 Max Health").
- The reward line comes from `UGSLevelRewards::DescribeRewards(First, Last)`.
  - It sums the static Additive modifiers of the reward effects per attribute and labels them with the attribute's
    display name, so the "+10" lives only in GE_LevelUp_MaxHealth.
  - `AGSGameState` replicates `LevelRewards` (set in `AGSGameMode::StartPlay`) so clients can describe rewards.
- **Pulses**: the existing LevelText punch, plus an XP bar pulse (height ×2 and the fill flashes pale gold over
  0.5 s).
- **Glow**: on OnLevelUp, `AGSCharacter` puts `Materials/M_LevelUpGlow` on the mesh as an overlay material and fades
  its "Glow" parameter 1 → 0 over 0.8 s.
  - The material was built with MCP: unlit, additive, emissive = Color × Glow × Fresnel × 5.
  - The character binds to its own PlayerState's progression, so every machine sees the glow.
- **Sound**: `LevelUpSound` on WBP_HUD plays once per frame, even when several levels come at once. Placeholder:
  `/Engine/EditorSounds/Notifications/CompileSuccess`, which is editor content and may not cook into a packaged
  build.
- Tuning lives on the WBP_HUD class defaults (HUD|Level, HUD|XP) and on BP_GSCharacter (Level Up).
- Test `Progression.LevelUpFeedback`.
- PIE check, with durations raised to 20 s in memory so the captures could catch them (restored afterwards):
  - The banner showed "LEVEL UP! / Level 3 / +20 Max Health".
  - "+200 XP" appeared above the player and "+10 XP" above a kill's damage number.
  - The player mesh brightened during the glow.

## Design change — permanent cash, hub market, level-up upgrades (0b2b6f5)

Prompt 11. Docs only; no code changed yet.

**New design**
- **Permanent cash.** Cash is saved to a player profile and kept across runs, even after death.
  - It lives in a profile subsystem (`UGSProfileSubsystem`, planned), and every cash change goes through it.
  - PlayerState only mirrors it for replication and the HUD.
- **The Market District is the hub between runs, not an in-run stop.**
  - The game starts there.
  - A "Start Run" portal sends the player to the town.
  - After dying (and seeing the run summary) the player returns there to spend or gamble cash.
- **The run loop is PreRound → Combat → RoundClear → PreRound …** until the player dies. No market visits during a
  run.
- **Per-run progression** (reset on death): XP, level, and level-up upgrades.
  - Each level-up still fully heals and adds MaxHealth.
  - It now also offers 3 random upgrade choices with rarities.
  - Starting upgrades:
    - Common: Reload Speed, Damage, Move Speed.
    - Rare: Extra Jump (+1 jump per pick, no cap).
- **Rules (CLAUDE.md):**
  - Run state lives on PlayerState / GameState.
  - Permanent state lives in the profile subsystem, and all cash changes go through it.
  - Hub randomness (casino) gets its own stream, not the run stream.

**Obsolete** (the code still contains it until the follow-up prompts remove or repurpose it)
- The `MarketDecision` and `MarketVisit` round phases (`EGSRoundPhase`, `AGSGameMode::AdvancePhase`,
  `RunMarketDecision`). RoundClear will go straight to the next PreRound. (Removed in Prompt 14.)
- `AGSGameState::bMarketOpenThisRound` (and its line in `gs.Debug.Round`). (Removed in Prompt 14.)
- The in-run market plan: "on some rounds the party teleports to the Market District", the market-access roll on
  the run stream, and the "market is always closed for now" stub.
- `gs.Market.Teleport` / `gs.Market.Return` as part of the loop. **They are kept for now as debug tools.**

**Hub map decision (user, after the first design-change commit): the hub becomes its own map.**
- The Market District moves out of the town map into its own hub map (planned `Maps/L_Hub_Graybox`) with its own
  GameMode.
- The town map keeps the round loop. Map travel moves the party between the two, and each run loads a fresh town.
- This also makes obsolete:
  - streaming `L_MarketDistrict_Graybox` into the town map;
  - `AGSGameMode::TravelToMarketDistrict` / `ReturnToTown` (replaced by map travel);
  - the in-town `MarketPortalSpot` / `TownReturn` points.

**Kept and reused**
- The district's graybox content (square, `Casino`, 4 `MarketStall`s, `MarketArrival` / `MarketExitPortalSpot`):
  it moves into the hub map.
- `EGSArea` / `CurrentArea` and the safe-area rules: the hub map is always MarketDistrict, the town map always Town.
- Test `Map.TownAndMarketDistrict` becomes a hub-map / town-map test.
- XP, levels, DA_LevelRewards (MaxHealth stack + full heal) and the level-up feedback.
- `UGSRunStats`: becomes the run summary after death.

**Changes meaning**
- **`AGSPlayerState::Cash` / `AddCash` / `TrySpendCash`:** these become a mirror of the profile. (Done differently
  in "Permanent cash" below: PlayerState has no cash at all.)
  - `AGSGameMode::AwardCash` will write through the profile subsystem.
  - Run-stats `CashEarned` stays per run.
- **Player death:** `?Restart` reloaded the map at the time. It became the `GameOver` phase → run summary → return
  to the hub (done in Prompt 27). Run state resets and cash is kept.
- **Progression reset:** `ResetProgression` / reward removal on "new run" is already in place. Upgrades will be
  removed the same way.

### Permanent cash — profile subsystem (a2dc3b7)
Prompt 12.
- `Core/GSProfileSaveGame.h` (`UGSProfileSaveGame`): `SaveVersion` (current 1), `Cash`, and `OwnedItems` (reserved,
  empty).
- `Core/GSProfileSubsystem.*` (`UGSProfileSubsystem`, GameInstance subsystem) owns the permanent cash balance:
  - API: `AddCash(N, Reason)`, `TrySpendCash(N, Reason)`, `GetCash()`, `OnCashChanged(NewCash, Delta)`, `Save`,
    `ResetProfile`. `UGSProfileSubsystem::Get(WorldContext)` finds it.
  - **Loads on game start** (`Initialize`).
  - **Saves** at the end of each RoundClear (after its vacuum has paid out), at run end (player death, in
    `AGSGameMode::HandlePlayerDeath`) and on quit (`Deinitialize`, also at the end of PIE). It does not save on
    every change.
  - **Versioning:** older saves are upgraded on load; newer ones load with a warning.
  - **Slots:** `GSProfile`, or `GSProfile_AutomationTest` while automation tests run.
- **Cash moved off `AGSPlayerState`:** `Cash`, its replication, `OnCashChanged`, `AddCash` and `TrySpendCash` are
  removed.
  - `AGSGameMode::AwardCash` → `UGSProfileSubsystem::AddCash(N, "Pickup")`; run stats CashEarned stays per run.
  - The HUD cash counter binds to the profile's `OnCashChanged`.
  - `get_player_attributes` "Cash" reads the profile.
- **Co-op:** only the local player (single player or the listen-server host) is credited. Remote players' pickups
  are not credited yet: one warning per session, `TODO(co-op)` in `AwardCash`. This is the decided "host's save
  holds every player's cash" design, not built in this prompt.
- **Commands** (run on the local machine, not forwarded): `gs.Profile.AddCash <N>`, `gs.Profile.Reset` (deletes the
  save, cash 0), `gs.Profile.Save`, `gs.Profile.Print`. Log prefix `[profile]`.
- Test `Profile.PermanentCash`. `Economy.CashPickups` now reads the profile.
- PIE on the real slot:
  1. New profile (cash 0).
  2. 3 kills + RoundClear vacuum → $35, saved when RoundClear ended.
  3. `gs.Profile.AddCash 50` → $85 (unsaved).
  4. Stopping PIE saved $85 ("Quit"). The next session loaded $85 and the HUD showed "$85".
  5. `gs.Profile.Reset` afterwards left the real profile at $0.

## Scope change — single-player first (f1db8b5)

Prompt 13. Docs only; no code changed.

- **The game is single-player first.** Co-op (2–4 player listen server) is milestone **M6**, after single-player is
  finished or nearly finished.
- **New rule (CLAUDE.md), replacing "Server-authoritative": "Co-op-ready, single-player first".**
  - New work is built and tested for single-player only: no new RPCs, replication or network tests.
  - Existing replication code stays.
  - Game state stays in GameMode / GameState / PlayerState and permanent state in the profile subsystem.
  - State changes only through gameplay functions.
  - Gameplay code takes the PlayerState / Controller explicitly instead of grabbing player 0.
  - Co-op gaps are marked `TODO(co-op)`.
- **Hub ↔ town travel:** single-player uses `OpenLevel`. Seamless / server travel and "the whole party travels
  together" move to M6.
- **Upgrade pause:** single-player pauses the game while choosing. The party pause moves to M6.
- The co-op-only plans (host save with every player's cash, stable player IDs, party pause, Server RPC upgrade picks,
  seamless travel checks) moved to **Later — co-op (M6)**.

### Prompt 14 — Run loop cleanup
- `EGSRoundPhase` is now WaitingToStart, PreRound, Combat, RoundClear, GameOver. MarketDecision and MarketVisit are
  removed.
- `AGSGameMode::AdvancePhase`: RoundClear → `StartNextRound()` (next round's PreRound), both on the RoundClear timer
  and on `gs.Round.Skip`. The profile still saves when RoundClear ends.
- Removed `AGSGameMode::RunMarketDecision`, `AGSGameState::bMarketOpenThisRound` (and its replication,
  `IsMarketOpenThisRound` / `SetMarketOpenThisRound`) and the "Market open" part of the `gs.Debug.Round` line.
- Kept as debug tools until the hub map (Prompt 26): `gs.Market.Teleport` / `gs.Market.Return`,
  `TravelToMarketDistrict` / `ReturnToTown`, and `gs.Round.Set` returning the party to town first.
- Tests:
  - `Round.PhaseLoop` checks that the enum has no market phases, that the RoundClear timer starts round 2's
    PreRound, and that skips walk PreRound → Combat → RoundClear → round 3 PreRound.
  - `Map.TownAndMarketDistrict` no longer checks the market flag.
- PIE check (god mode, waves cleared with `gs.KillAll`): rounds 1–3 each went Combat → RoundClear → next PreRound
  with no other phase in between. The profile saved after each RoundClear ($40 → $105 → $195 on the real slot).
  `gs.Market.Teleport` / `Return` still moved the player there and back.
- Unrelated test-log warning seen: `M_LevelUpGlow` is missing the SkeletalMesh usage flag, so the default material
  is used for the glow until the material is resaved with that flag.

### Prompt 15 — Upgrade data + stat upgrades
- **New attribute `ReloadSpeed`** (`UGSAttributeSet`, constructor default 1.0, clamped to ≥ 0.1). `GA_Reload` waits
  `ReloadTime / ReloadSpeed` (`UGSGameplayAbility_Reload::GetReloadDuration`). Not replicated (`TODO(co-op)`).
- **`Core/GSUpgradeData.*` (`UGSUpgradeData`, Primary Data Asset, type `Upgrade`)**:
  - Fields: DisplayName, Description, Icon (soft texture; placeholder `/Engine/EngineResources/DefaultTexture`),
    Rarity (`EGSUpgradeRarity`: Common, Rare; append new ones at the end), Effect, MaxStacks (0 = no cap).
  - The description uses a `{PerStack}` placeholder. `GetFormattedDescription()` fills it from the effect's first
    static modifier (Multiply magnitude 1.1 → "10"), so the number lives only in the GE.
- **Upgrade effects** (`Abilities/GSGameplayEffects.h`): `UGSUpgradeEffect` base (infinite, stacks by target, no GE
  stack limit) with one Multiply (Additive) modifier of 1 + bonus.
  - GAS scales it by the stack count around the bias of 1, so N stacks give base × (1 + bonus × N). Bonuses add up
    linearly and don't compound.
  - Subclasses `UGSUpgradeEffect_ReloadSpeed` / `_Damage` (1.10) and `_MoveSpeed` (1.08).
  - MoveSpeed reaches `MaxWalkSpeed` through the existing attribute delegate on `AGSCharacter`.
- **Assets** (MCP):
  - `Abilities/GE_Upgrade_ReloadSpeed`, `GE_Upgrade_Damage`, `GE_Upgrade_MoveSpeed`: Blueprint children of the
    classes. Edit the modifier magnitude there to retune.
  - `Data/Upgrades/DA_Upgrade_ReloadSpeed`, `DA_Upgrade_Damage`, `DA_Upgrade_MoveSpeed`: all Common, no cap.
- **`Core/GSUpgradeComponent.*` (`UGSUpgradeComponent`, on `AGSPlayerState`, `GetUpgradeComponent()`)**:
  - `GrantUpgrade(Upgrade, Source)` (authority) checks MaxStacks, applies one stack of the effect and counts it.
  - Read access: `GetStacks(Upgrade)`, `GetUpgradeStacks()`, `GetTotalPicks()`, `OnUpgradeStacksChanged`.
  - `ResetUpgrades()` removes every effect; `AGSGameMode::StartRun` calls it next to `ResetProgression`
    (`gs.XP.SetLevel` doesn't).
  - Not replicated (`TODO(co-op)`). Log prefix `[upgrade]`.
- **Commands:** `gs.Upgrade.Grant <Name>` (`ReloadSpeed`, `Damage`, `MoveSpeed` or the full `DA_Upgrade_*` name)
  and `gs.Upgrade.List` (rarity, stacks / cap, description and the three stats).
- **Test** `Upgrades.StatUpgrades`.
- **PIE check:**
  - Base values: ReloadSpeed 1, Damage 1, MoveSpeed 900.
  - After `gs.Upgrade.Grant` ReloadSpeed ×5, Damage ×3 and MoveSpeed ×2: 1.5, 1.3 and 1044; `gs.Upgrade.List`
    agreed.
  - A rifle reload took about 1.17 s (1.8 / 1.5 = 1.2 s expected; polled from MCP).
  - A rifle shot hit the target dummy for 26.0 (20 × 1.3).
- **Not built yet:** the upgrade pool with rarity weights, offers on level-up and the choice UI (Prompts 18–19).

### Prompt 16 — Extra Jump
- **New attribute `ExtraJumps`** (constructor default 0, clamped to ≥ 0, not replicated, `TODO(co-op)`).
  - `AGSCharacter` binds its change delegate (like MoveSpeed) and keeps `JumpMaxCount = 1 + ExtraJumps`.
  - It is synced on possess and respawn, and changes apply at once, also mid-air.
- **Air jumps** (`AGSCharacter::OnJumped_Implementation`, `JumpCurrentCount >= 2`):
  - Each one sets the upward speed to the new `AirJumpZVelocity` (DA_PlayerMovementSettings, 800 cm/s). That is the
    same speed whether the player is still rising or falling fast, so each air jump adds the same height (~1.87 m)
    from where it starts.
  - Landing resets the count (engine `ResetJumpState`; `AirJumpsSinceLanded` goes back to 0).
- **How it combines with other movement:**
  - **Coyote time:** a jump inside the 0.12 s window after walking off a ledge is the ground jump, and all air
    jumps remain. After the window, the first press uses an air jump (engine rule).
  - **Slide-hop:** unchanged. It is the ground jump; air jumps work after it.
  - **Dash:** a jump pressed during a dash used to be lost: TickDash overwrites the velocity every frame, but the
    jump was still counted. It is now queued (`IsJumpQueuedAfterDash`) and happens when the dash ends. That is a
    ground jump after a ground dash and an air jump after an air dash. Chained dashes keep the queue.
- **Feedback** (placeholder, local only, skipped during client move replays; `TODO(co-op)` to multicast):
  - `AirJumpEffect` = the Niagara template `/Niagara/DefaultAssets/Templates/Systems/RadialBurst`, at the feet,
    scale 0.4.
  - `AirJumpSound` = `/Engine/VREditor/Sounds/VR_teleport_Cue` (editor content).
  - Both are set on BP_GSCharacter.
- **Assets:** `Abilities/GE_Upgrade_ExtraJump` (`UGSUpgradeEffect_ExtraJump`, Additive +1 ExtraJumps per stack) and
  `Data/Upgrades/DA_Upgrade_ExtraJump` (Rare, no cap, "+{PerStack} air jump per stack.").
- **Debug:** `GSJump` exec on the controller taps jump (Jump, then StopJumping on the next tick).
- **Test** `Upgrades.ExtraJump`. It turns off editor CPU throttling while it runs, for the coyote timing.
- **PIE check** (god mode, round paused):
  - `gs.Upgrade.Grant ExtraJump` ×3 → JumpMaxCount 4.
  - Tapping `GSJump` 5 times from the ground: +186 cm, then air jumps of +187, +183 and ~+180 cm (sampled every
    50 ms). The 5th press was refused, and landing reset the count. Apex ~7.2 m above the floor.
  - Ground dash + jump: the jump came out at the dash end with ~840 cm/s horizontal speed carried over.
  - The puff was visible at the feet.
- **Movement issues noticed** (none fixed beyond the dash bug; suggestions only):
  - **Height stacking:** every stack adds ~1.9 m of reachable height, with no cap. **Decided (user): Extra Jump stays
    uncapped**; it can be picked any number of times. The ceiling of the map boundary (Prompt 21) limits the height.
    Until it is built the height is unlimited.
  - **Double-tap waste:** an air jump right after the ground jump gains little, because upward speed only goes from
    ~770 to 800. Mashing jump wastes it. This comes from "same speed every time". A short delay before the first
    air jump is possible.
  - **Air jumps keep horizontal speed:** they don't redirect it (AirControl 0.9 still steers). With no input,
    horizontal speed in the air decays to 0 within ~0.6 s (BrakingDecelerationFalling 1500; this was already so).
  - **Jump during a dash waits up to 0.15 s** for the dash to end, rather than cancelling it.

## Design update — decisions before M3

Prompt 17. Docs only (plus three stale code comments); no gameplay code changed.

**Decisions (user)**
1. **Permanent unlocks.** Shop purchases and casino prizes are permanent: once owned, they apply to every future
   run. (Refined in **Design update — market, prestige and economy**: how an item applies depends on its type,
   usable items are used up, and prestige wipes owned items.)
   - They are stored in the profile (`UGSProfileSaveGame::OwnedItems`, already reserved and empty) and survive death.
   - Built with the shop and casino in M3/M4.
2. **Upgrade rarities and rerolls.**
   - Level-up upgrades get four rarity tiers: Common, Rare, Epic, Legendary. `EGSUpgradeRarity` has Common and Rare
     today; Epic and Legendary are appended in Prompt 18.
   - The player can reroll the 3 choices a limited number of times per run. It starts at 3; the number goes in data.
     It is run state and resets with the run.
   - There is no skip: every offer ends with a pick.
3. **Casino bets risk cash only**, never owned items.
4. **XP comes from pickups** that enemies drop, not instantly on the kill.
   - Today `AGSGameMode::HandleEnemyKilled` grants XP to the killer at once (Prompt 8). Prompt 20 changes that.
   - The pickup details (magnet, RoundClear vacuum, popups, who gets it) are settled in Prompt 20.
5. **Every map gets a boundary:** invisible walls around the play area and a ceiling.
   - It blocks pawns only (players and enemies); shots, traces, pickups and the camera pass through.
   - The ceiling is what limits Extra Jump height; Extra Jump still has no cap.
   - Built in Prompt 21, for the town map and later the hub map.

**Docs changed**
- CLAUDE.md: the game summary (XP pickups, rarity tiers and limited rerolls, permanent unlocks, cash-only bets) and
  a new rule: every map has a boundary that blocks pawns only.
- This file:
  - the game summary;
  - the decisions added to **Decided (user)**, and shop/casino taken off the open items;
  - **Next up** renumbered (18–28), and the step references updated to match;
  - the M2 heading changed to "Prompts 1–16 done";
  - Prompts 11, 12 and 13 marked on the design-change, permanent-cash and scope-change entries;
  - 42a874e added to the Prompt 16 row.
- Stale comments fixed (no behaviour change):
  - `GSRoundSettings.h`: RoundClear is followed by the next PreRound, not "the market decision".
  - `GSRunRandom.h`: the `RollChance` example no longer mentions "market access".
  - `GSCheatCommands.cpp`: `gs.Market.*` are debug-only until Prompt 26, not "driven by the round flow in M2".

## Design update — market, prestige and economy

Docs only; no code changed. Not a numbered step: **Next up** keeps its 18–28 numbering.

**Decisions (user)**
1. **Owned things apply from the next run.** Everything the player owns is active or usable from the start of the
   next run. How depends on the item type.
2. **Item types so far:**
   - **Usable items (consumables):** they can be bought again and again to restock. Using one in a run uses it up.
   - **Gear:** bought once. In the hub the player puts it in a loadout slot, swapping out the gear in that slot. The
     loadout applies at the start of the next run.
   - Other types (abilities, powerups) are decided in M3.
3. **The shop is static:** a fixed catalogue the player works through. Its stock is not random and is never
   rerolled. Only the casino uses the hub random stream.
4. **Prestige** (a later milestone):
   - The player can choose to prestige. It wipes cash and everything bought in the market or won at the casino.
   - In return it gives bonuses for the next playthrough, based on the cash earned in the whole playthrough. Only the
     prestige rewards survive.
   - The profile will therefore have two layers:
     - playthrough data: cash, owned items, loadout, and total cash earned this playthrough;
     - prestige data.
   - None of this is built now; the save layout changes in M3. Today's `UGSProfileSaveGame` has `Cash` and an empty
     `OwnedItems` (version 1).
5. **Flat cash and XP per enemy.** Every round pays the same cash and XP per enemy. Income grows because later rounds
   spawn more enemies and the player kills faster.
   - There is no round scaling for cash or XP drops. This is what the code already does (`UGSEnemyData::CashValue` /
     `XPValue`).
   - The round clear bonus (Prompt 23) is the same amount every round.
6. **Epic and Legendary upgrades are picked later**, when there are more upgrades. The rarity weights in Prompt 18
   are placeholders.

**Changes to earlier decisions**
- "Permanent unlocks apply to every future run" (decisions before M3, item 1) now means: owned items are active or
  usable from the next run, according to their type. Usable items are used up, gear applies through the loadout,
  and a prestige wipes cash and items.
- "Round scaling for cash and XP drops" is no longer an open item; it was decided against.

**Docs changed**
- CLAUDE.md:
  - Game summary: flat cash and XP per enemy, item types and the loadout, the static shop, and prestige as a later
    goal.
  - The profile rule: the two-layer save comes in M3, and owned items apply by type.
  - The randomness rule: only the casino uses the hub stream.
  - New rule: cash and XP per enemy are flat.
- This file: the game summary, this entry, **Decided (user)**, the open items, and notes under Next up steps 18
  and 23.

### Prompt 18 — Upgrade offers (log only, no UI)
- **Rarities:** `EGSUpgradeRarity` is Common, Rare, Epic, Legendary (Epic and Legendary appended). No upgrade uses the
  new tiers yet.
- **`Core/GSUpgradePool.*` (`UGSUpgradePool`) → `Data/DA_UpgradePool`**, set as `UpgradePool` on BP_GSGameMode:
  - `Upgrades`: the four DA_Upgrade_* assets.
  - `RarityWeights`: Common 100, Rare 20, Epic 6, Legendary 1 (placeholders; class defaults, so the asset only
    stores them once edited).
  - `ChoicesPerOffer` 3, `RerollsPerRun` 3.
- **Roll** (`UGSUpgradePool::RollOffer(Stream, GetStacks)`): for each of the 3 choices, pick a rarity by weight, then
  a random upgrade of that rarity.
  - A rarity is left out of the weighting when nothing in it is available: the tier is empty, its upgrades are all
    at MaxStacks, or they are already in this offer.
  - With fewer available upgrades than choices, the offer is shorter. An offer with nothing in it is dropped.
  - Weights are per tier, not per upgrade. Today the lone Rare (Extra Jump) is in 1 − (100/120)³ ≈ 42% of offers.
- **Offers** (`UGSUpgradeComponent` on the PlayerState; run state):
  - Each level-up queues one offer. The component listens to the progression component's `OnLevelUp`, which fires
    after the MaxHealth reward and the heal. Several level-ups at once queue several offers.
  - The front offer is rolled with the run stream (`UGSRunRandom::GetStream`) when it opens. So a queued offer
    already accounts for the pick made in the offer before it (MaxStacks).
  - `AGSGameMode::ChooseUpgrade(PlayerState, Index)` grants the choice (`GrantUpgrade`, source "LevelUp") and opens
    the next queued offer. A bad index or no open offer is refused. There is no skip.
  - `AGSGameMode::RerollUpgradeOffer(PlayerState)` replaces the open offer with a fresh roll for one reroll. It is
    refused with no open offer or no rerolls left. A reroll may repeat upgrades.
  - `AGSGameMode::StartRun` → `ResetUpgrades` drops all offers and refills the rerolls from the pool. The first
    value comes from the pool at BeginPlay.
  - Read access: `HasOpenOffer()`, `GetCurrentChoices()`, `GetQueuedOffers()`, `GetRerollsLeft()`,
    `DescribeOffer()`.
- **Events for the UI:**
  - `OnUpgradeOfferChanged(EGSUpgradeOfferChange)` with Opened / Rerolled / Closed. A pick fires Closed, then Opened
    if another offer is queued. (Changed in Prompt 19: a pick with another offer queued fires only Opened; Closed
    means no offer is left.)
  - `OnRerollsLeftChanged(RerollsLeft)`.
  - Both are BlueprintAssignable, with native mirrors `OnUpgradeOfferChangedNative` / `OnRerollsLeftChangedNative`
    for C++ listeners and tests.
  - Not replicated (`TODO(co-op)`).
- **Console:**
  - `gs.Upgrade.Offer` queues an offer as a level-up would (no level) and prints the open offer.
  - `gs.Upgrade.Choose <0-2>` and `gs.Upgrade.Reroll` act on the open offer and print it.
  - Log lines use the `[upgrade]` prefix ("offer queued / opened / rerolled", "chose [i] …").
- **Simulator:**
  - `UGSDebugLibrary::SimulateUpgradeOffers(Count, Seed)` rolls the project pool with a private `FRandomStream`, as
    for a fresh run with no picks. It never touches the run stream and needs no PIE.
  - MCP tool `simulate_upgrade_offers(count, seed)` returns its report as JSON text: the toolset registry rejects an
    untyped `dict` return and then skips the whole toolset.
- **Test** `Upgrades.Offers`.
- **PIE check** (round paused):
  - `gs.XP.Add 100` opened an offer of Extra Jump / Damage / Move Speed after "MaxHealth 110, Health 110".
  - `gs.XP.Add 281` (two levels) queued 2 offers.
  - Three `gs.Upgrade.Reroll` gave new offers (rerolls 2 → 1 → 0, repeats allowed); the 4th was refused.
  - `gs.Upgrade.Choose 7` was refused.
  - Choose 0 / 1 / 2 granted Damage, Extra Jump and Move Speed, each opening the next offer, and then closed.
  - Reroll with no offer was refused. `gs.Upgrade.Offer` opened a debug offer.
- **Simulation, 10,000 offers, seed 1:** see Tuning values.
- **Note:** `gs.XP.SetLevel N` queues N − 1 offers, as N − 1 level-ups do.

### Prompt 19 — Level-up choice screen
- **Pause** (gameplay, not UI): `AGSGameMode::UpdateUpgradeChoicePause` runs on every offer change
  (`UGSUpgradeComponent::BroadcastOfferChanged`, before the UI hears it).
  - While any player has an open offer it calls `UGameplayStatics::SetGamePaused(true)`; when none is left, false.
  - It only undoes a pause it made itself (`IsPausedForUpgradeChoice()`). `bPauseDuringUpgradeChoice` is on
    BP_GSGameMode's defaults.
  - Log: `[upgrade] Game paused / unpaused for the upgrade choice.`
- **Event change:** a pick that leads into the next queued offer fires only `Opened`, and `Closed` means no offer is
  left. So the screen swaps cards without flicker and the pause stays on between queued offers.
  `Upgrades.Offers` was updated.
- **`UI/GSUpgradeCardWidget.*` → `UI/WBP_UpgradeCard`** (built with MCP):
  - Layout, 300×440: `CardBorder` (5 px frame in the rarity color) → `CardBody` (dark) → key hint "[1]", icon
    (80×80), name, rarity ("RARE", tinted), description (`GetFormattedDescription`), "Owned: N".
  - A transparent, non-focusable `CardButton` on top takes clicks.
  - `RarityColors`: grey Common, blue Rare, purple Epic, gold Legendary. The gamepad-selected card scales to 1.07.
- **`UI/GSUpgradeChoiceWidget.*` → `UI/WBP_UpgradeChoice`** (built with MCP):
  - Layout: full-screen dim; "CHOOSE AN UPGRADE"; "+N more upgrades after this" (hidden when nothing is queued);
    `CardsBox` (cards created from `CardWidgetClass` = WBP_UpgradeCard); a "Reroll (N left)" button; an input hint.
    The content sits below the "LEVEL UP!" banner.
  - Created by `AGSPlayerController::BeginPlay` (`UpgradeChoiceWidgetClass` on BP_GSPlayerController, Z order 10,
    `GetUpgradeChoiceWidget()`).
  - It binds to the PlayerState's `OnUpgradeOfferChanged` / `OnRerollsLeftChanged`. Its tick only retries binding
    and runs the real-time timers below; it doesn't poll offer state.
  - **Fresh offer:** the game is already paused. Held keys are flushed (no stuck fire / aim). After `RevealDelay`
    (1.0 s real time; UMG keeps ticking while paused) the "LEVEL UP!" banner has played and the cards appear with
    UI-only input and the mouse cursor.
  - **Next queued offer or reroll:** the cards swap at once.
  - **Input lock:** every time cards appear or change, input is ignored for `InputLockDuration` 0.3 s while the
    cards fade in. Locked keys are still consumed, so they don't leak to the game.
  - **Input:**
    - Click a card; keys 1–3 or numpad 1–3; R or the Reroll button.
    - Gamepad: D-pad / left stick (or the arrow keys) to select; A to pick (the first A press only selects the first
      card); Y to reroll.
    - Buttons are non-focusable, so Slate navigation can't take focus away.
  - **Reroll button:** "Reroll (N left)", disabled (greyed out) at 0.
  - **Closed:** hidden; game input back and cursor off. The game unpauses through the GameMode.
  - Picks and rerolls call `AGSGameMode::ChooseUpgrade` / `RerollUpgradeOffer`. `TODO(co-op)`: Server RPC for
    clients.
- **Test** `UI.UpgradeChoice`.
- **Tests changed:**
  - Level-ups now pause the game, so `Progression.LevelUpRewards` and `Progression.LevelUpFeedback` take their
    offers (`PickAllUpgradeOffers`, choice 0) before steps that need world time.
  - `Balance.TimeToKill` sets XPGain 0, so kills don't level up (a pause, or a Damage pick, would break the
    measurement).
- **PIE check** (real key presses through Slate; round paused):
  - `gs.XP.Add 100` paused the game and the banner showed alone. After 1 s the cards appeared, with Owned counts and
    a blue frame on the Rare card.
  - R rerolled ("Reroll (2 left)"). 2 picked the second card and unpaused.
  - `gs.XP.Add 1000` (four level-ups, levels 4–7) paused once and showed "+3 more upgrades after this".
    - 1 picked.
    - A key 0.15 s after the swap was ignored.
    - D-pad right ×2 highlighted card 2, and A picked it.
    - 3, then 1 picked the rest. Unpaused once, after the last pick.
  - A stray key afterwards did nothing.
  - The first card layout overflowed on two-line names; the card was made taller with smaller fonts, and checked
    again.

### Prompt 20 — XP orbs
- **XP is a pickup now.** Every killed enemy drops one XP orb worth its `UGSEnemyData::XPValue` (flat, the same every
  round). No XP is granted on the kill itself.
  - `AGSGameMode::HandleEnemyKilled` calls `SpawnXPDrop` next to `SpawnCashDrop`; the instant `AwardXP` on kill is
    gone.
  - Orbs drop whoever killed the enemy (before, kills by non-players gave no XP).
- **Reused the cash pickup system** (`Economy/GSCashPickup*`):
  - `AGSCashPickup` has a kind (`EGSPickupKind`: Cash, XP; `GetKind` / `IsCash` / `IsXPOrb`). Both kinds share one
    pool, the toss, the magnet (PickupRadius, `CollectDistance`), the RoundClear vacuum (`VacuumAll`) and the clear
    on a new run (`ReleaseAll`).
  - `UGSCashPickupSubsystem::SpawnXPDrop` tosses one orb to a random spot around the body (run stream).
  - `CollectPickup` branches by kind: cash → `AwardCash`; XP → `AGSGameMode::AwardXPPickup(Player, XP, Location)`.
  - New queries: `GetNumActiveOfKind`, `GetLifetime(Kind)`, XP counters (`GetNumXPOrbsDropped / Collected /
    Expired`) and `GetLastXPCollectLocation`. The older cash counters count cash pieces only.
  - The kind isn't replicated (`TODO(co-op)`: clients would show the cash look).
- **At pickup:** `AwardXPPickup` → `AwardXP` for the collecting player. XPGain applies; level-ups, rewards, upgrade
  offers and the pause happen then; run stats XPEarned counts it then. `TODO(co-op)`: XP goes to whoever collects
  the orb.
- **"+N XP" popup at the collection point:**
  - `AGSPlayerController::NotifyXPCollected` (not an RPC; local HUD; `TODO(co-op)`) →
    `UGSHUDWidget::NotifyXPSource`, renamed from `NotifyKill`, anchors the popup before the XP arrives.
  - Kills no longer anchor XP popups. XP with no source (`gs.XP.Add`) still pops above the player.
  - Test hook `GetLastXPPopupAnchor()`.
- **Tuning:** in the existing `DA_CashSettings` (`UGSCashSettings`), which now holds all enemy-drop pickup tuning.
  - Chosen because the toss, magnet, collect distance and pool are shared with cash.
  - New "XP Orb" section: `XPOrbLifetime` (0 = never expires, the default), `XPOrbLook`, `XPOrbPulseAmount` 0.15,
    `XPOrbPulseSpeed` 1.5.
  - `gs.Cash.Lifetime` still affects cash only.
- **Look** (placeholder):
  - `Materials/M_XPOrb`: unlit, emissive `Color` (0.02, 0.22, 1) × `Intensity` 3.
  - On the engine sphere at scale 0.32, pulsing in size instead of spinning, with the same bob as cash.
  - The first try (scale 0.22, intensity 6) read as a pale dot on the light floor; this version is clearly blue next
    to the gold coins and green bills.
- **Debug:** the `gs.KillAll` help says the enemies drop cash and XP orbs and that XP comes once they are collected.
  `gs.XP.Add` stays instant.
- **Tests:**
  - New `Economy.XPOrbs`.
  - `Progression.XPAndLevels`: kill → no XP, one orb; vacuum → XPValue, then XPValue × XPGain 2. It now takes its
    two upgrade offers first.
  - `Progression.LevelUpFeedback`: the kill shows no popup; the collected orb shows "+XPValue XP" at the collection
    point.
  - `Economy.CashPickups`: counts cash pickups only; the XP orbs it leaves never expire.
- **PIE check** (round paused):
  - 3 melee bandits killed with `gs.KillAll`: TotalXP stayed 0, and an orb lay beside each corpse with its coins and
    bill.
  - `gs.Round.Skip` ×2 to RoundClear vacuumed 12 pickups (9 cash, 3 orbs): XP +30 (three "+10 XP" log lines at
    pickup) and cash +$30.

### Prompt 21 — Map boundary
- **New object channel `MapBoundary`** (`ECC_GameTraceChannel1`, `Config/DefaultEngine.ini`
  [/Script/Engine.CollisionProfile]; `GSCollision::MapBoundary` in `Core/GSCollisionChannels.h`).
  - Default response Block, so every pawn profile blocks it. The boundary's boxes ignore every channel but Pawn, so
    only pawns are stopped.
  - Passing through:
    - Player shots (Visibility trace).
    - The camera (Camera trace).
    - Enemy shots and line of sight (WorldStatic / WorldDynamic / Pawn object query, which doesn't include the new
      type).
    - Pickup landing traces (WorldStatic object query).
    - Pickups themselves (no collision) and ragdolls (PhysicsBody).
- **`Core/GSMapBoundary.*` (`AGSMapBoundary`)**, placed in `L_Town_Graybox` as `MapBoundary` at (0, 0, 0): the town
  slab is 150 × 150 m, centered on the origin, top at Z 0.
  - The actor location is the play-area center at ground level.
  - Properties:
    - `PlayAreaSize` 15000 × 15000 cm (the slab edges; 30000 × 30000 since Prompt 26b).
    - `CeilingHeight` 7500 cm.
    - `WallThickness` 200, `WallDepthBelowGround` 500.
  - 4 wall boxes and a ceiling box sit just outside the play area. They are rebuilt in OnConstruction / BeginPlay, so
    editing the properties when the map grows is enough.
  - Query-only, hidden, and they don't affect the NavMesh.
  - Helpers: `IsInside(Location, Margin)`, `GetPlayBounds`, `GetCeilingZ`, `Find(World)`.
- **Ceiling:** a normal blocking hit. CharacterMovement removes the upward speed and the player falls back: no damage,
  no teleport. Air jumps still work under it; they bonk. This is the only Extra Jump height limit (no cap).
- **Spawn finder:**
  - Ring points outside the boundary (60 cm margin) are rejected (new `RejectedOutsideBoundary` stat, in the log
    line), with one warning per search if any were.
  - EnemySpawn fallback points outside are skipped with a warning.
  - A chosen point outside would also warn.
- **Debug:** cvar `gs.Debug.Boundary 1` draws the play-area box and a cross on the ceiling. `gs.Upgrade.Grant <Name>
  [Count]` grants several stacks at once (`gs.Upgrade.Grant ExtraJump 45`).
- **Test** `Map.Boundary`.
- **PIE check** (god mode, round paused):
  - `gs.Upgrade.Grant ExtraJump 45` → JumpMaxCount 46. Tapping jump near each apex climbed for 20 s: the capsule top
    peaked at ~74.9 m (centre 7393 cm); the last air jumps bonked (upward speed −200 right after the jump). The 47th
    tap was refused.
  - Dashing into the east wall from 4 m stopped at x = 7458: the wall face is at 7500, minus the capsule radius.
  - Sliding into it: the slide was started with 1400 cm/s of ground speed set on the movement component, since the
    console can't hold a move key. It stopped at the same x = 7458.
  - `gs.Debug.Boundary 1` drew the red box edges up to the ceiling.
- **Note:** GA_Slide doesn't start during a dash, and the dash's exit speed brakes below the slide minimum within
  ~0.2 s, so a "dash, then slide" combo needs the move key held. That is existing behaviour, not changed here.

### Prompt 22 — Kill combo
- **`Core/GSComboSettings.h` (`UGSComboSettings`) → `Data/DA_ComboSettings`**, set as `ComboSettings` on
  BP_GSGameMode. The asset still uses the class defaults:
  - `ComboWindow` 2.5 s.
  - `CashMultiplierSteps` (`FGSComboStep` MinCombo → Multiplier): 1 → x1, 3 → x1.25, 6 → x1.5, 10 → x2. The step
    with the highest MinCombo ≤ the combo applies.
  - `bEndComboOnDamage` true, `bApplyComboToXP` false.
- **`Core/GSComboComponent.*` (`UGSComboComponent`, on `AGSPlayerState`, `GetComboComponent()`)** holds the combo.
  It is run state.
  - `RegisterKill()` adds 1, takes the multiplier for the new count and restarts the window. It returns the count.
  - The window is a world timer, so it stops while the game is paused (the upgrade choice screen) and resumes after.
  - `EndCombo(Reason)`: `EGSComboEndReason` TimedOut, DamageTaken or Reset. `AGSGameMode::StartRun` ends it with
    Reset.
  - Read access: `GetComboCount()`, `GetCashMultiplier()`, `GetTimeRemaining()`, `GetTimeRemainingFraction()`.
  - Events: `OnComboChanged(Count, Multiplier)` (after every kill, and (0, x1) when the combo ends),
    `OnComboEnded(FinalCount, Reason)` and its native mirror `OnComboEndedNative`.
  - Not replicated (`TODO(co-op)`). Log prefix `[combo]` (one line per combo kill, one when a combo ends).
- **Kills** (`AGSGameMode::HandleEnemyKilled`):
  - Only enemy kills count (`AGSEnemyCharacter`), for the killer's PlayerState. Target dummies don't count.
  - The combo is counted before the drops, so a kill pays at the step it reaches: cash =
    round(CashValue × multiplier).
  - The pickup keeps that value however late it is collected, even after the combo ended.
  - XP orbs stay XPValue unless `bApplyComboToXP`.
  - Run stats record the combo after each kill.
- **Damage:** `AGSGameMode::HandleDamageApplied` (health actually lost) → `UGSComboComponent::HandleDamageTaken`.
  It ends the combo if `bEndComboOnDamage`. Damage blocked by god mode or dash i-frames doesn't count.
- **Tests / debug:** `AGSGameMode::GetComboSettings()` returns `SetComboSettingsOverride(Settings)` when one is set,
  otherwise the asset.
- **Run stats:** `BestCombo` (`GetBestCombo`, "Best combo" in `gs.Run.Stats`), reset with the run. It is not
  replicated, unlike the other stats (`TODO(co-op)`), because of the "no new replication" rule.
- **HUD** (WBP_HUD, built with MCP): VerticalBox `ComboPanel` under the cash counter (top right, 92 px down).
  - `ComboCountText`: "5 COMBO", orange 28. It punches ×1.3 on every kill.
  - `ComboMultiplierText`: "x1.25 CASH", size 20. Grey at x1, gold above (`ComboBaseMultiplierColor` /
    `ComboBonusMultiplierColor`). It punches ×1.6 when a new step is reached.
  - `ComboTimerBar`: an orange 220×8 bar in a SizeBox. It drains from the combo's remaining window, read every tick
    (it stays put while paused).
  - Shown from `ComboMinCountToShow` 2 kills. Tuning is on WBP_HUD's class defaults (HUD|Combo).
  - Test hooks: `IsComboShowing()`, `GetDisplayedComboCount/Multiplier/TimerPercent()`, `GetComboCountText()`,
    `GetComboMultiplierText()`.
- **Debug:** `get_player_attributes` also returns Combo, ComboMultiplier and ComboTimeLeft. There are no new
  console commands.
- **Test** `Economy.KillCombo`. `Economy.CashPickups`' kill helper now ends the combo before each kill, so every
  drop is exactly CashValue.
- **PIE check** (god mode, round paused): `gs.Spawn MeleeBandit 5` + `gs.KillAll` → Combo 5, x1.25, 2.02 s left
  0.4 s later. The HUD showed "5 COMBO / x1.25 CASH" with the bar draining under "$880". The combo had ended 3 s later.

### Prompt 23 — Round-clear bonus and RoundClear summary
- **`Core/GSRoundClearSettings.h` (`UGSRoundClearSettings`) → `Data/DA_RoundClearSettings`**, set as
  `RoundClearSettings` on BP_GSGameMode. The asset still uses the class defaults:
  - `BaseBonus` $20, `NoDamageBonus` $15, `FastClearBonus` $15. The same every round: nothing grows with the round
    number.
  - Fast clear: Combat lasted at most `FastClearBaseTime` 8 s + `FastClearTimePerEnemy` 1.5 s × the round's wave
    size (`GetFastClearTime`; round 1's 4 bandits → 14 s). The time limit follows the wave size so the bonus stays
    reachable in big rounds; set the per-enemy time to 0 for a fixed limit.
- **Round stats** (`UGSRunStats`, on the GameState; run state):
  - `FGSRoundStats GetCurrentRound()`: Round, Kills, CashCollected (pickups only), XPEarned, DamageTaken,
    WaveEnemies, CombatStartTime.
  - The record starts over in each PreRound (`BeginRound`). RoundClear still belongs to the round it ends, so the
    vacuum's cash and XP count.
  - `RecordCombatStarted` (time + wave size) when Combat starts. `RecordKill`, `AddCashEarned` (pickups),
    `AddXPEarned` and `AddDamageTaken` also feed the round.
  - `FGSRoundClearPayout GetLastRoundClearPayout()`: round, the three bonuses (0 when missed), `bNoDamage`,
    `bFastClear`, ClearTime, FastClearTime, `GetTotal()`.
  - Neither is replicated (`TODO(co-op)`). `gs.Run.Stats` prints "This round" and "Last clear bonus" lines.
- **Payout** (`AGSGameMode::PayRoundClearBonus`, on every Combat → RoundClear: wave cleared, skip or safety timer):
  - Base, plus no-damage if nobody lost health this round (PreRound included; blocked hits don't count), plus
    fast-clear if ClearTime ≤ FastClearTime.
  - The total goes into the profile (`UGSProfileSubsystem::AddCash(Total, "RoundClear")`) and into run stats
    `CashEarned`. The RoundClear save at the end of the phase includes it.
  - RoundClear entered from another phase (debug `SetPhase`) pays nothing.
  - Log: `[cash] Round N clear bonus: $T (base …, no damage …, fast clear …; cleared in X s of Y s, Z damage taken)`.
  - `TODO(co-op)`: one party-wide bonus into this machine's profile.
  - `SetRoundClearSettingsOverride` lets tests swap the settings.
- **`UI/GSRoundSummaryWidget.*` → `UI/WBP_RoundSummary`** (built with MCP), created by `AGSPlayerController::BeginPlay`
  (`RoundSummaryWidgetClass` on BP_GSPlayerController, Z order 5: above the HUD, below the upgrade choice screen;
  `GetRoundSummaryWidget()`).
  - Shown while the phase is RoundClear and the last payout is for the current round. It is hidden in the next
    PreRound.
  - Driven by `AGSGameState::OnPhaseChanged` and `UGSRunStats::OnStatsChanged`, so cash collected and XP keep counting
    while the vacuum pays out. It fades in over 0.25 s.
  - Layout: a dark panel at 36% screen height, centered.
    - "ROUND N CLEAR" (gold 34).
    - Kills, Cash collected (green), XP earned (blue).
    - A divider, then Round clear, No damage taken and "Fast clear (X s / Y s)". A missed bonus reads "missed" and
      its row is dimmed to 0.4.
    - A divider, then "Round bonus +$N" (gold 26).
  - Test hooks: `IsSummaryShowing()`, `GetShownRound()`, and the `Get…Text()` getters for each line.
- **Tests:** new `Round.ClearBonus`. `Profile.PermanentCash` now expects the round-clear bonus in the balance and in
  the RoundClear save.
- **PIE check** (god mode, round 1 played out, wave killed with `gs.KillAll`, then the RoundClear timer paused):
  - The summary showed "ROUND 1 CLEAR": 4 kills, $46 collected (combo x1.25 on kills 3–4), 40 XP, then +$20,
    +$15, "Fast clear (3 s / 14 s)" +$15 and "Round bonus +$50".
  - The balance went $880 → $976 = $46 + $50 (real profile slot).

### Prompt 24 — Round HUD
Everything below is in WBP_HUD (`UGSHUDWidget`), built with MCP and driven by delegates. The total cash counter
(profile), the level label and the XP bar are unchanged.
- **Round info** (top center, VerticalBox `RoundInfoPanel`):
  - `RoundText` "ROUND 4" (gold 30). `PhaseText` "PRE-ROUND" / "COMBAT" / "ROUND CLEAR" / "GAME OVER" (grey 15,
    letter-spaced).
  - `CountdownText` "Round 4 in 3…" (PreRound only).
    - It is computed from the GameState's phase end time.
    - A world timer re-arms it for the moment the shown second changes, so it stops while the game is paused, like
      the phase timer. There is no per-tick polling.
  - `EnemiesText` "6 enemies left" (Combat only; plural-aware). It shows the wave's remaining enemies: alive plus
    still to spawn.
  - Sources: `AGSGameState::OnPhaseChanged` / `OnRoundChanged` and the new `OnWaveCountsChanged(Total, Remaining,
    Alive)`. `SetWaveCounts` now broadcasts it when a count changes. It is server only: the counts replicate without
    a RepNotify (`TODO(co-op)`).
- **Owned upgrades** (HorizontalBox `UpgradeIconsBox` at (40, 128), under the XP text):
  - One `UpgradeIconWidgetClass` = `UI/WBP_UpgradeIcon` (`UI/GSUpgradeIconWidget.*`, new) per upgrade, in pick order.
  - Each icon: a 44 px icon in a rarity-colored frame (same colors as the cards), "x3" in the corner and the name
    under it (the placeholder icons all look alike).
  - Driven by `UGSUpgradeComponent::OnUpgradeStacksChanged`: a new stack updates or adds an icon; 0 stacks (new run)
    removes it.
- **Round banners** (VerticalBox `RoundBanner`, the level-up banner's spot at 22% height):
  - "ROUND N" / "GET READY" (white 64) when a PreRound starts (also the first one, which starts before the HUD
    binds).
  - "ROUND CLEAR" / "Round N" (green) when Combat ends in RoundClear.
  - Animation: pop + fade like the level-up banner, 1.8 s in total (`RoundBanner*` tuning on the class defaults,
    HUD|Round).
  - **Queue:** one banner at a time.
    - A round banner waits while "LEVEL UP!" shows or an upgrade offer is open (the choice cards; the game is paused
      then).
    - A level-up (or an offer opening) takes the spot at once. The showing round banner is hidden and put back first
      in the queue, then replays when the level-up banner is gone and the offer is closed.
    - A queued banner whose round or phase is over is dropped (e.g. a ROUND CLEAR still waiting when the next
      PreRound starts).
    - The same banner is never queued twice.
  - Test hooks: `GetRoundText/PhaseText/CountdownText/EnemiesText()`, `IsCountdownShowing()`, `IsEnemiesShowing()`,
    `IsRoundBannerShowing()`, `GetRoundBannerTitle()`, `GetQueuedRoundBannerCount()`, `GetRoundBannersShown()`,
    `GetUpgradeIconCount()`, `FindUpgradeIcon()`.
- **Test** `UI.RoundHUD`.
- **PIE check** (rounds 1–3 played with god mode and `gs.KillAll`, plus `gs.Round.Set 4`):
  - Round 1 Combat: "ROUND 1 / COMBAT / 4 enemies left" at the top center.
  - Round 1 RoundClear: the green "ROUND CLEAR / Round 1" banner above the round summary, no overlap.
  - Round 2's vacuum XP leveled up to 2. "LEVEL UP!" and the cards showed with no round banner; after
    `gs.Upgrade.Choose 0` the ROUND CLEAR banner replayed and the "x1 Reload Speed" icon appeared under the XP bar.
  - Round 3 played through to RoundClear.
  - `gs.Round.Set 4`: "ROUND 4 / PRE-ROUND / Round 4 in 2…" and the "ROUND 4 / GET READY" banner.
  - The 3 s PreRounds of rounds 1–3 passed between MCP scripts, so that is why PreRound was captured in round 4.

### Prompt 25 — Hub map + hub GameMode
- **`Maps/L_Hub_Graybox`** (built with MCP):
  - A copy of `L_MarketDistrict_Graybox`, moved from X = +200000 to the origin. It holds:
    - the 70 m ground slab, the walled 50×50 m square (4 m walls);
    - `Casino`, 4 `MarketStall`s and the `MarketArrival` / `MarketExitPortalSpot` Target Points, with their tags;
    - the warm PostProcessVolume and the point light.
  - Its own lighting, in folder Lighting, with the town's settings:
    - Sun (DirectionalLight: movable, intensity 6, atmosphere sun, the town's rotation);
    - SkyLight (movable, real-time capture), SkyAtmosphere, HeightFog (density 0.0436, falloff 0.2) and
      VolumetricCloud.
  - `PlayerStart` at MarketArrival (0, −500, 100), facing the Casino (yaw 90).
  - `AGSMapBoundary` at the square's center with `PlayAreaSize` 5000 × 5000 (the walls' inner faces) and the
    default 75 m ceiling. It also blocks jumping over the walls.
  - No NavMesh and no EnemySpawn points.
  - World Settings `DefaultGameMode` = `BP_GSHubGameMode`.
  - **The streamed `L_MarketDistrict_Graybox` is unchanged** and still used by the town's debug teleport: nothing is
    removed from the town yet (Prompt 26).
- **`Core/GSHubGameMode.*` (`AGSHubGameMode`, `AGameModeBase`) → `Core/BP_GSHubGameMode`:**
  - It uses the same GameState / PlayerState / controller / character Blueprints as BP_GSGameMode.
  - No round loop: no phases, waves, drops or run seed. The GameState stays at round 0, WaitingToStart.
  - `StartPlay` sets `CurrentArea = MarketDistrict`. `gs.Spawn` and the spawn finder already refuse there.
  - Every player gets `SafeZoneEffect` = `Abilities/GE_SafeZone` (new `UGSSafeZoneEffect`: infinite, grants the new
    native tag `State.SafeZone`). It is applied in `HandleStartingNewPlayer` / `RestartPlayer`, once per PlayerState.
  - `GA_Fire` has `State.SafeZone` in its ActivationBlockedTags, so the player can't shoot. `GE_Damage`
    (`UGSDamageEffect`) ignores targets with it, so the player takes no damage.
  - Code that looks for `AGSGameMode` (progression, upgrades, combo, run random, debug commands) already handles its
    absence. The progression component's "no XP curve" warning now only fires under a round GameMode.
- **HUD in the safe area** (`CurrentArea == MarketDistrict`, from `AGSGameState::OnAreaChanged`):
  - Hidden: crosshair (new optional binding `Crosshair`), weapon + ammo, health, level + XP, upgrade icons, combo,
    round info and round banners. The always-on ones get their designed visibility back when the area changes.
  - Visible: the total cash and the slide / dash icons (movement still works).
  - The town's debug Market District teleport hides the same elements.
  - Test hooks `IsInSafeArea()`, `AreCombatElementsHidden()`, `IsCashShown()`.
- **Startup map:** `GameDefaultMap` and `EditorStartupMap` = `L_Hub_Graybox` (`Config/DefaultEngine.ini`).
  `GlobalDefaultGameMode` stays BP_GSGameMode, so `L_Town_Graybox` (no override) keeps running the round loop.
- **Tests:**
  - `BeginPIE(Test, MapPath = TownMap)` opens the map in the editor first if it isn't already the editor's map
    (`FAutomationEditorCommonUtils::LoadMap`). Every existing test still plays in the town.
  - New `Map.Hub`.
- **PIE check:** the editor started in the hub. PIE spawned the player at MarketArrival (0, −500) facing the Casino:
  the warm-lit walled square under a cloudy sky, and only "$1,340" plus the slide / dash icons on the HUD.
- **Fixed while building:** moving the copied actors with location-only transforms reset their scale to 1, and the
  PlayerStart's location reset too. The player then fell through a 1 m ground cube. Scales and yaws were restored
  from the original bounds and the hub test passed after that.

### Prompt 26 — Start Run portal + town cleanup
- **`Market/GSStartRunPortal.*` (`AGSStartRunPortal`) → `Hub/BP_GSStartRunPortal`**, placed in L_Hub_Graybox at
  MarketExitPortalSpot (0, −2150, on the floor), facing the arrival point (yaw 90).
  - C++ components: a `Trigger` capsule (r 110, half-height 130, profile Trigger), a `RingMesh` and a floating
    `PromptText` "START RUN" (TextRender, green, 70). The Blueprint adds a `RingCore`.
  - Placeholder ring: the engine has no torus mesh, so `RingMesh` is a flat engine Cylinder (2.4 m wide) with
    `Materials/MI_PortalGlow` (M_XPOrb, green, intensity 6). `RingCore` is a 2.0 m cylinder on top with
    `MI_PortalCore` (near black), which leaves a glowing band. The ring pulses (cosmetic, local).
  - A player pawn overlapping the trigger (server) calls `AGSHubGameMode::StartRun(PC)` once (`TryStartRun`, also
    callable from code).
- **`AGSHubGameMode::StartRun`:**
  - Fades every player to black over `StartRunFadeTime` 0.5 s (held), disables their input, then opens `RunMap`
    (`L_Town_Graybox`, set on BP_GSHubGameMode) with `UGameplayStatics::OpenLevelBySoftObjectPtr`.
  - A second start while one is under way is refused. `TODO(co-op)`: ServerTravel for the party.
- **The run starts when the town loads** (`AGSGameMode::StartPlay`, unchanged otherwise):
  - Seed: `?GSSeed=N` (URL option, `AGSGameMode::RunSeedOption`), else `-GSSeed=N`, else random.
  - `gs.Run.Seed N` in the hub stores the next seed on the hub GameMode (`SetNextRunSeed`) and the portal passes it
    as `?GSSeed=N`: the one thing chosen to carry over. In the town, `gs.Run.Seed` still restarts in place.
  - Then come the run stats reset and round 1. The cash carries over in the profile (GameInstance).
- **First spawns respect the spawn ring:** checked in PIE and in `Map.StartRunPortal` after the travel.
  - `UGSWaveDirector` now exposes `GetFallbackSpawns()`, `GetOnScreenSpawns()`, `GetClosestSpawnDistance()` and the new
    `GetFarthestSpawnDistance()`.
  - PIE, seed 777 from the hub: `[wave] Round 1 cleared … spawns: closest 12.3 m, on-screen 0, fallback 0` (ring
    12–25 m). Nothing needed fixing.
- **Town cleanup:**
  - The streamed `L_MarketDistrict_Graybox` was removed from L_Town_Graybox (new editor command
    `gs.Editor.RemoveSublevel <ShortName>`, which replaces `gs.Editor.MarketDistrictLevel`). The asset was then
    deleted: its content lives in the hub map.
  - The `MarketPortalSpot` and `TownReturn` Target Points were removed from the town.
  - Removed from `AGSGameMode`: `TravelToMarketDistrict`, `ReturnToTown`, `IsAreaTravelInProgress`,
    `GetMarketDistrictStreamingLevel`, `FindTaggedActor`, `TownReturnTag` / `MarketArrivalTag`, the fade / move /
    tick helpers, and the Market District properties. `SetRound` no longer returns the party to town first.
  - Added `HubMap` (BP_GSGameMode → L_Hub_Graybox) and `OpenHubMap()` (OpenLevel; the death flow uses it since the
    fix below and Prompt 27).
- **`gs.Market.*` repointed:**
  - `gs.Market.Teleport` (in a run) opens the hub map.
  - `gs.Market.Return` (in the hub) starts a run as the portal does.
  - Each refuses with a warning in the other map.
- **Tests:**
  - `Map.TownAndMarketDistrict` became `Map.Town` (town map, plus gs.Market.Teleport / Return travel) and
    `Map.StartRunPortal` (hub → portal → town). `Map.Hub` keeps the hub checks.
  - `Spawn.LocationFinder` and `Progression.LevelUpRewards` lost their in-town district visits.
  - The tests open L_Town_Graybox explicitly (`BeginPIE`, since Prompt 25).
- **PIE check** (editor started in the hub):
  - `gs.Run.Seed 777` → "the next run uses seed 777". The portal showed between the stalls with "START RUN" above it.
  - Stepping onto the green ring faded the screen.
  - The town loaded with "Run started: seed 777 (hub gs.Run.Seed)", the HUD at "ROUND 1 / COMBAT / 4 enemies left"
    and the cash unchanged ($1,340).
  - Round 1's spawns stayed in the ring (above).

### Fix — death returns to the hub
User report: dying and pressing R started a new run in the town; it should go back to the market.
- `AGSGameMode::RequestRestart` (death screen R, through `ServerRequestRestart`; still only honored while dead) now
  ends the run with `OpenHubMap()` instead of `?Restart`. If `HubMap` is unset it falls back to `?Restart`.
  - The profile was already saved at death, so the cash carries over. Everything else from the run is rebuilt by the
    hub map.
  - Log: `Run over: <PC> returns to the hub.`
- `WBP_DeathScreen` text: "YOU DIED — Press R to return to the market" (MCP).
- This only moved the R press from "restart the town" to "go to the hub". The GameOver phase and the run summary came
  in Prompt 27, which also renamed `RequestRestart` / `ServerRequestRestart` to `RequestReturnToHub` /
  `ServerRequestReturnToHub` and replaced `WBP_DeathScreen` with `WBP_RunSummary`.
- Tests: `Player.RestartRestoresInput` and `Profile.PermanentCash` now expect the hub after R (AGSHubGameMode, game
  input back, balance kept).
- PIE check:
  - The hub, then `gs.Market.Return` into a run.
  - `gs.Damage.Self 1000` showed the new death screen.
  - A real R key press led to "Run over … returns to the hub" and the hub, at 100 health with the cash unchanged
    ($1,697).

### Prompt 26b — Bigger town (300 × 300 m)
Map work only, done with MCP. The only C++ change is the new test.
- **Ground:** `Ground` now covers 300 × 300 m (±15000), still centered on the origin with its top at Z 0. Kill Z is
  still below it.
- **Layout.** The old 150 m town is the center, unchanged: plaza, `Cover_PlazaBuilding`, the 6 shacks and the crates.
  - **Streets** (`Town/Roads`, flat 2 cm slabs, `MI_Graybox_Road`):
    - a 12 m ring road at 77–89 m around the old town;
    - an avenue from the ring out to each landmark plaza;
    - two side streets per side, at ±50 m, running out to 140 m.
  - **Landmarks** (`Town/Landmarks/<Side>`, tags `Landmark` + `Landmark_North` / `_East` / `_South` / `_West`). Each
    stands on a 26 m round plaza 125 m from the center:
    - North: a red water tower (4 legs, a platform at 15 m, tank and roof up to 25.4 m).
    - East: a blue grain silo (a 10 m wide, 26 m tall cylinder with a dome up to 31 m).
    - South: a sand church (hall, a 26 m tower and a red spire up to 32 m).
    - West: a red-brown mesa with three stepped tiers (6 / 12 / 18 m), climbable with Extra Jump.
  - **Outskirts** (`Town/Outskirts/<Side>`): cover placed on a 15 m grid with jitter in the bands between the ring road
    and the edges, skipping streets and plazas. Some cells are left empty as open yards. In total:
    - 26 shacks (4 m, wood) and 7 houses (8 m, stone);
    - 13 towers (12 m, stone), whose roofs Extra Jump can reach;
    - 6 free-standing low walls (1.8 m) and 100 crates (1.5 m, some stacked to 3 m).
  - Nothing is walled off. Every part of the ground can be walked to (checked by NavMesh paths, below).
  - **Materials:** new `Materials/Graybox/` instances of the engine `BasicShapeMaterial`: `MI_Graybox_Road`,
    `MI_Graybox_Wood`, `MI_Graybox_Stone`, `MI_Landmark_Red`, `MI_Landmark_Blue`, `MI_Landmark_Sand`,
    `MI_Landmark_Rock`.
- **NavMesh:**
  - `NavMeshBoundsVolume` covers ±150 m and Z −5 to 35 m, above the 32 m spire. NavMesh rebuilt.
  - **Islands inside buildings.** Recast voxelizes surfaces only, so a closed mesh taller than the agent got a
    NavMesh floor inside it. That was an island, and the M0 plaza building and shacks had them too.
    - Every cover piece now has `bRasterizeAsFilledConvexVolume`. Pieces standing on the ground also have
      `bFillCollisionUnderneathForNavmesh`.
    - Pieces taller than the NavMesh bounds (the silo and the church tower, with the old 15 m top) lost their top
      faces to clipping and still got a floor inside. That is why the bounds now reach 35 m.
  - **Result:** the ground NavMesh is one connected area. Roofs, crate tops and the landmark tops keep small NavMesh
    patches that only jumping reaches. Enemies never path there, and the spawn finder rejects them as unreachable.
- **Boundary:** `MapBoundary` `PlayAreaSize` 30000 × 30000. `CeilingHeight` stays 7500.
- **Spawns:** the four `EnemySpawn` fallback points moved to 142 m out on the axes (N / E / S / W), facing the center.
  They are inside the boundary and on the NavMesh. The spawn ring stays 12–25 m.
- **Unchanged:** the PlayerStart (0, 0, facing north) and the lighting. The movable sun, sky light (real-time
  capture), sky atmosphere, height fog and volumetric clouds have no bounds, so they cover the bigger map (checked
  from the corners).
- **Code:** no gameplay change was needed. No hard-coded town size was found: `AGSMapBoundary`'s class default of
  15000 is overridden in both maps, and `Map.Boundary` reads the actor's own bounds.
- **Test** `Map.TownLayout` (new).
- **Perf** (`gs.Perf 5`; PIE, god mode, round paused in PreRound, standing at the PlayerStart, editor throttling off):

  | | Frame avg | p95 | Max | Game | Render | GPU |
  |---|---|---|---|---|---|---|
  | Before (150 m) | 8.35 ms (120 fps) | 8.33 | 20.03 | 2.70 | 2.77 | 3.14 |
  | After (300 m) | 8.37 ms (119 fps) | 8.33 | 19.81 | 2.81 | 2.76 | 3.10 |

  The frame time sits at the 120 fps cap both times; game, render and GPU time moved within noise. For reference,
  round 3 Combat with 8 enemies alive: 8.42 ms avg, game 3.48, render 2.89, GPU 3.31.
- **PIE check:**
  - Rounds 1–3 played with god mode, `gs.KillAll` once each wave had spawned. The bandits pathed to the player. Spawns:
    closest 15.4 / 15.2 / 12.2 m, 0 on-screen, 0 fallback. Round 2's XP leveled up and a card was picked.
  - Every edge and corner: the player was dropped in 5 m from the edge, 3.5 m up, with 2000 cm/s outward speed. Each
    time they stopped at ±149.57 m (the wall minus the capsule radius) and landed. These were teleports; that each
    spot can be walked to comes from the NavMesh path checks in `Map.TownLayout`.
  - From the NE corner, the water tower and the silo stand out over the outskirts. From the SW corner, the church and
    the mesa do.
  - The NavMesh bounds height and the silo fix came after this PIE run. The full suite passed after them.

### Prompt 27 — End of a run: GameOver, run summary, Return to Market
- **Death ends the run** (`AGSGameMode::HandlePlayerDeath`; `TODO(co-op)`: when the whole party is dead):
  1. `SetPhase(GameOver)` from whatever phase it was. GameOver is untimed and final: `gs.Round.Skip` does nothing in
     it, so no RoundClear can follow and no round-clear bonus is paid after death.
  2. The wave stops: `UGSWaveDirector::StopWave(false)` cancels the telegraphs and the spawn queue. The living enemies
     stay where they are (they can't hurt a dead player), and killing them afterwards clears nothing.
  3. Open upgrade offers are closed and queued ones dropped (`UGSUpgradeComponent::DropOffers`). That lifts their
     pause, which would otherwise stop the return timer. GameOver also refuses new offers (`QueueOffer`), e.g. from
     debug XP after death. The upgrades themselves stay for the summary.
  4. Run stats freeze the run time (`UGSRunStats::EndRun`, which existed but was never called).
  5. The profile saves ("RunEnd", as before).
  6. Log: `Run over in round N (seed S): R rounds cleared, level L, K kills, best combo C, $X earned, T s.`
  7. `AGSPlayerController::ClientShowRunSummary` (renamed from `ClientShowDeathScreen`) shows the summary, flushes
     held keys and switches to UI-only input with the mouse cursor.
- **`UI/GSRunSummaryWidget.*` (`UGSRunSummaryWidget`) → `UI/WBP_RunSummary`** (built with MCP; `RunSummaryWidgetClass`
  on BP_GSGameMode, Z order 100). It replaces `WBP_DeathScreen` and `UGSDeathScreenWidget`, which were deleted.
  - Layout: a full-screen dim with a centered dark panel.
    - "YOU DIED" (red 56) over "RUN SUMMARY".
    - Rows: rounds cleared, level reached (gold), kills, best combo (orange), run time ("4:07", "1:02:05" from an
      hour), seed (no digit grouping, so it can be typed back into `gs.Run.Seed`).
    - "Upgrades picked": one `WBP_UpgradeIcon` per upgrade in pick order with its stacks, in a WrapBox. "None" when
      nothing was picked.
    - Cash earned this run (green, pickups + round-clear bonuses) and total cash (gold, the profile).
    - A green "Return to Market" button and an "R / Start / Y" hint.
  - Sources: `UGSRunStats` (`OnStatsChanged`), the owning player's `UGSUpgradeComponent`, `UGSProfileSubsystem`
    (`OnCashChanged`) and the GameState's run seed. It only reads them. `TODO(co-op)`: the upgrades and best combo
    aren't replicated.
  - **Input:** the button, or `ReturnKeys` R, gamepad Start and Y (the old death screen's keys). Ignored for
    `InputLockDuration` 1 s after the screen appears and on key repeats, so a held reload key can't skip it. The
    button is non-focusable; keys are handled by the widget.
  - It fades in over 0.4 s. After Return to Market it fades out over 0.5 s (`FadeOutTime`) and the button greys out.
  - Test hooks: `TryReturnToHub`, `HandleReturnKey`, `IsInputLocked`, `IsReturnRequested`, `GetScreenOpacity`, the
    `Get…Text()` getters, `GetUpgradeIconCount` / `GetUpgradeIcon`, static `FormatRunTime`.
- **Return to the hub:** `ServerRequestReturnToHub` (renamed from `ServerRequestRestart`) →
  `AGSGameMode::RequestReturnToHub` (renamed from `RequestRestart`).
  - Only honored while the requester is dead and no return is under way (`IsReturningToHub`).
  - It fades every camera to black over `ReturnToHubFadeTime` 0.5 s (BP_GSGameMode, held, audio too), then
    `OpenHubMap()`. The `?Restart` fallback is only for a missing `HubMap`.
  - Loading the map resets the audio fade. The next run rebuilds the town, so run state resets by construction; cash
    stays in the profile.
  - Log: `Run over: <PC> returns to the hub (fade 0.50 s).`
- **Tests:**
  - New `Run.GameOver`.
  - `Player.RestartRestoresInput` became `Player.ReturnToHubRestoresInput` (summary, cursor, R after the lock).
  - `Profile.PermanentCash` calls `ServerRequestReturnToHub`.
- **PIE check** (editor started in the hub; `gs.Run.Seed 777`, `gs.Market.Return`):
  - God mode, Damage x2, Extra Jump x1 plus a level-up pick of Extra Jump; round 1 cleared with `gs.KillAll` ($50
    bonus).
  - Death in round 2 Combat (`gs.God 0`, `gs.Damage.Self 1000`): "Combat -> GameOver", the wave stopped, "Saved slot
    GSProfile (RunEnd)".
  - The summary showed 1 round cleared, level 2, 4 kills, best combo 4, 0:32, seed 777, Damage x2 and Extra Jump x2,
    $96 earned, $2,429 total. Round 2's bandits stood around the body.
  - A real R key press 20 s later: "returns to the hub", and the hub opened 0.5 s later. It was unfaded, with game
    input, $2,429, level 1 and no upgrades.
  - An earlier attempt, with god mode typed too late, died for real in round 1. `gs.XP.Add` after that death opened
    an upgrade offer and paused the game under the summary. That is fixed above (offers dropped and refused in
    GameOver) and covered by `Run.GameOver`.

### Prompt 28 — Tests + M2 checkpoint ("M2 complete")
- **Test audit:** each M2 rule was checked against the suite. Rules already covered stayed as they were: offers, Rare
  weight, MaxStacks, empty tiers, reroll limits, per-stack stat amounts, the boundary vs. shots, combo and round-clear
  amounts, level-up rewards, XP orbs and the hub.
- **New tests (3):**
  - `Run.SameSeedSameRun`: replays rounds 1, 2, 5 and 8 (8 is budget-generated) with the same seed. It compares every
    wave's enemies in spawn order and every spawn point (player still), a level-up offer, its 3 rerolls, the refused
    4th, and the next offer. Another seed gives another run.
  - `Run.NewRunResets`: progress in a run, then death → hub → Start Run. The new run is level 1 with no XP, upgrades,
    offers, ExtraJumps, Damage / MoveSpeed / MaxHealth bonuses, kills, cash earned or combo. Its rerolls are back to 3
    and the cash is kept (HUD too).
  - `Economy.FlatDropsPerRound`: melee and ranged bandits killed in round 1 and round 10 drop the same cash, in the
    same pieces, and one orb of the same XP. Their HP doubles.
- **Extended tests:**
  - `Round.PhaseLoop`: death stops the loop. GameOver has no end time, a skip doesn't leave it, and it is still
    GameOver in round 5 after the PreRound time.
  - `Round.WaveDirector`: killing the first arrivals while the rest of the wave is still to come keeps Combat going.
  - `Spawn.LocationFinder`: every point is inside the boundary. A forced fallback (the player 50 m up, where the
    ring has no NavMesh) gives flagged EnemySpawn points, on the NavMesh, inside and spaced. The fallback warning
    must be logged (`AddExpectedMessage`).
  - `Map.TownLayout`: a spawn ring of 6 is rolled at each corner. Every point is inside the boundary, on the
    NavMesh, and in the ring or a fallback.
  - `Progression.XPAndLevels`: one offer per level. Two level-ups give one open + one queued. The jump from level 10
    to the cap gives 40 offers (one open, 39 queued).
  - `Upgrades.ExtraJump`: 25 more stacks give ExtraJumps = stacks and JumpMaxCount = 1 + stacks (no cap).
  - `Profile.PermanentCash`: the quit at the end of PIE saves the balance. The next PIE session (a new GameInstance)
    loads it (HUD too). `gs.Profile.Reset`, run through the console, sets it to 0 and deletes the save.
- **Test hooks:** `UGSWaveDirector::GetSpawnQueue`, `GetPendingSpawns`, `SpawnNextBatchNow`.
- **Test fixes:**
  - `Run.GameOver` uses the existing `CountLivingEnemies` (my duplicate helper is gone).
  - A second PIE session in one test must wait for `GetPIEWorld() == nullptr` after `EndPIE`. The end is only
    requested, so without the wait the next steps ran in the old session. That also made the next test
    (`Progression.LevelUpFeedback`) fail once.
- **New log line** (for the checkpoint numbers): `[round] Round N cleared|died: combat T s, wave W, K kills, $C
  collected + $B bonus, X XP, D damage taken, level L, fallback spawns F`. It is written once per round, when
  RoundClear ends, and for the round a death ends (`AGSGameMode::LogRoundSummary`).
- **Full suite:** 35 tests, all passing (M1's GAS / weapon / dash tests included), in about 118 s.

#### M2 checkpoint — two full loops in PIE
Hub → run → death → hub, twice. The seeds were set in the hub (`gs.Run.Seed`) and the run started with
`gs.Market.Return` (the portal's path). Editor throttling was off; the real profile slot was used.
- **How it was played:** MCP can't react in real time (a command takes 1–3 s to arrive). So the cleared rounds used
  god mode and `gs.KillAll` once the whole wave had spawned, and the death round was played standing still with god
  mode off.
  - "Combat" times below are therefore the spawn time plus my command delay, not real kill speed.
  - Damage taken is 0 in cleared rounds, so the no-damage bonus was always paid.
  - Upgrades: Damage when offered, otherwise a reroll, then the Rare.
- **Warm-up:** the first try (seed 2701, no god mode yet) died in round 1 after **7.3 s of combat** to 4 melee
  bandits: 96 damage taken, standing still.

**Loop 1 (seed 2701): died in round 5, 1:35 run time**

| Round | Wave | Combat | Cash collected + bonus | XP | Level after | Upgrades |
|---|---|---|---|---|---|---|
| 1 | 4 melee | 11.5 s | $46 + $50 | 40 | 1 | — |
| 2 | 5 melee, 1 ranged | 17.6 s | $80 + $35 (missed fast clear by 0.6 s) | 65 | 2 | offered MoveSpeed / Damage / ReloadSpeed → Damage |
| 3 | 6 melee, 2 ranged | 12.0 s | $118 + $50 | 90 | 2 | — |
| 4 | 7 melee, 3 ranged | 13.5 s | $165 + $50 | 115 | 3 | offered MoveSpeed / ReloadSpeed / ExtraJump → reroll → ExtraJump / MoveSpeed / ReloadSpeed → ExtraJump |
| 5 | 9 melee, 4 ranged | died after 10.1 s (113 damage) | — | — | 3 | — |

Totals: 4 rounds cleared, level 3, 28 kills, best combo 10, $594 earned (balance $2,429 → $3,023), 1 reroll used,
0 fallback spawns. The summary showed exactly these numbers.

**Loop 2 (seed 2702): died in round 7, 1:58 run time**

| Round | Wave | Combat | Cash collected + bonus | XP | Level after | Upgrades |
|---|---|---|---|---|---|---|
| 1 | 4 melee | 14.5 s | $46 + $35 (missed fast clear by 0.5 s) | 40 | 1 | — |
| 2 | 5 melee, 1 ranged | 12.6 s | $80 + $50 | 65 | 2 | offered Damage / ReloadSpeed / MoveSpeed → Damage |
| 3 | 6 melee, 2 ranged | 6.7 s | $117 + $50 | 90 | 2 | — |
| 4 | 7 melee, 3 ranged | 14.9 s | $159 + $50 | 115 | 3 | offered ReloadSpeed / MoveSpeed / Damage → Damage |
| 5 | 9 melee, 4 ranged | 12.1 s | $225 + $50 | 150 | 4 | offered ReloadSpeed / MoveSpeed / Damage → reroll → ReloadSpeed / Damage / MoveSpeed → Damage |
| 6 | generated (budget 20): 4 melee, 8 ranged | 5.9 s | $239 + $50 | 160 | 5 | offered Damage / ReloadSpeed / MoveSpeed → Damage |
| 7 | generated (budget 22.5): 4 melee, 9 ranged | died after 6.7 s (138 damage) | — | — | 5 | — |

Totals: 6 rounds cleared, level 5, 53 kills, best combo 13, $1,151 earned (balance $3,023 → $4,174), 1 reroll used,
0 fallback spawns. The summary matched.

- **Spawn fallbacks:** 0 in all 13 waves (the warm-up included). In the 10 cleared waves the closest spawn was
  12.4–17.3 m (ring 12–25 m), and none was on-screen.
- **Frame time** (`gs.Perf 5` in each death round, 120 fps cap; game / render / GPU averages):

  | Loop | Enemies alive | Frame avg | p95 | Max | Game | Render | GPU |
  |---|---|---|---|---|---|---|---|
  | 1 (round 5) | 13 | 8.36 ms (120 fps) | 8.33 | 21.70 | 3.88 | 3.02 | 3.44 |
  | 2 (round 7) | 12 | 8.37 ms (120 fps) | 8.33 | 23.31 | 3.82 | 3.01 | 3.61 |
- **Flow checks:** Start Run carried each seed over. Each death showed the summary; "Return to Market" was clicked
  through Slate (a real button click) and the hub opened 0.5 s later. The hub kept the cash with level 1 and no
  upgrades, and the next run started fresh.
- **Balance observations** (suggestions only, nothing changed):
  - The first generated wave (round 6, budget 20) is **smaller than round 5** (12 vs 13 enemies) and **two-thirds
    ranged**. Ranged bandits cost 2 and the budget buys them freely. Round 7 is 9 of 13 ranged. The DT_Waves → budget
    hand-off feels like a step sideways, and the composition flips.
  - Income per cleared round rose from $81–96 (round 1) to $289 (round 6). Most of it is drops ($46 → $239); the
    flat $20–50 bonus matters mostly early, as designed.
  - Levels: 2 after round 2, then about one per round from round 4. Of the 8 rolled offers (rerolls included),
    Damage was in 6 and Extra Jump in 2. Expected rates are about 86% and 42%; 8 offers is a small sample.
  - Standing still is lethal fast: 7.3 s in round 1 (4 melee), 10.1 s in round 5, 6.7 s in round 7 (ranged-heavy).

## Workflow update — faster test runs (after M2)

Not a numbered step. Docs and test code only; no gameplay, tuning or asset change.

- **New workflow rules (CLAUDE.md):**
  - **Tests:** while working on a prompt, run only the tests for the area being changed (`RunTests` with names, or
    a filter like `StartsWith:BanditBaller.Round`). Run the full suite (`StartsWith:BanditBaller`) once per prompt,
    right before the commit. Docs-only prompts don't need a test run.
  - **PIE checks:** when an automation test covers the rule, one short PIE confirmation instead of a scripted
    playthrough. Full playthroughs are for checkpoint prompts only.
  - **Test speed convention:** wait on conditions rather than fixed times; speed up game-time waits with the new
    helpers; never touch tuning assets or gameplay code for speed.
- **Test speed helpers** (`Tests/GSGameplayTests.cpp`, `GSTests`):
  - `SpeedUpWorld(D)` sets the PIE world's time dilation (game time runs D× faster) and turns off editor CPU
    throttling, so frames stay short with the editor in the background. `RestoreWorldSpeed()` undoes both.
  - Rules are checked in game time (phase lengths, clear times, coyote window), so what the tests check doesn't
    change. Real-time parts (widget animations, input locks, the choice screen's reveal) aren't sped up.
  - `SetFloatProperty(Object, Name, Value)` sets a float tuning property on one object (e.g. a widget instance's
    banner length), never on the asset.
- **The five slowest tests sped up** (each passed 3 runs in a row before the full suite; times from the automation
  results):

  | Test | Before | After | How |
  |---|---|---|---|
  | `UI.RoundHUD` | 14.5 s | 6.1 s | game time ×4; this HUD's LEVEL UP! banner 0.8 s (was 2.5) and round banners 0.7 s (was 1.8); the real-time wait with the cards up follows (1.5 s, was 3.5) |
  | `Round.PhaseLoop` | 11.1 s | 2.9 s | game time ×4 (the PreRound / RoundClear timers and the wait after death) |
  | `Upgrades.ExtraJump` | 10.6 s | 3.7 s | game time ×3 (jumps, falls, dashes); replaces the test's own throttle handling |
  | `Map.TownLayout` | 9.7 s | 2.5 s | game time ×4 (the 8 wall runs) |
  | `Round.ClearBonus` | 8.8 s | 2.3 s | game time ×4 (two waves spawning, the vacuum) |

- **Suite time:** 35 tests, **117.8 s before → 80.5 s after** (−37 s, −32%), all passing. The slowest tests now are
  `Economy.KillCombo` (6.3 s), `UI.RoundHUD` (6.1 s), `Economy.XPOrbs` (5.9 s), `Balance.TimeToKill` (5.1 s) and
  `Economy.CashPickups` (5.0 s).

## Design update — M2b upgrades

Prompt 29. Docs only; no code changed. **M2b** is a block of eleven level-up upgrades after M2 (complete) and before
the market (M3). It grows DA_UpgradePool from 4 to 15 upgrades.

**The upgrades (user).** All of them stack with no cap unless noted. Every number is a starting value and lives in
data.

| Upgrade | Rarity | Per pick | MaxStacks | Attribute | Prompt |
|---|---|---|---|---|---|
| Fire Rate | Common | +8% FireRate | none | `FireRate` (exists) | 31 |
| Magnet | Common | +15% PickupRadius (cash and XP orbs) | none | `PickupRadius` (exists) | 31 |
| Bigger Mags | Common | +15% magazine size | none | new | 32 |
| Dash Charge | Rare | +1 dash charge. The player now starts with 1 charge instead of 2 | none | new | 30 |
| Floating Shield | Rare | +1 shield | none | new | 36 |
| Loaded Dice | Rare | +5% chance for a shot to deal double damage | 20 (= 100%) | new | 33 |
| Leech | Rare | Heal 1 health per kill | none | new | 32 |
| Floating Gun | Epic | 1st pick: a gun over the right shoulder; 2nd: a gun over the left; 3rd and later: +10% damage and +10% fire rate for the guns | none | new | 38, 39 |
| Piercing Rounds | Epic | Shots pass through 1 more enemy | none | new | 34 |
| Powder Keg | Epic | +3% chance that an enemy the player kills explodes and damages enemies nearby (fixed blast; changed in 35c, was "every kill explodes, the blast grows") | 34 (= 100%) | new | 35, 35c |
| Second Wind | Legendary | Survive one killing blow and come back at 30% health | none | new | 37 |

- After M2b the pool holds Common 6 (Reload Speed, Damage, Move Speed + 3), Rare 5 (Extra Jump + 4), Epic 3 and
  Legendary 1. The Epic and Legendary tiers are no longer empty, so all four rarity weights (100 / 20 / 6 / 1, still
  placeholders) start to matter.
- **Floating Shield:** the shields float around the player. Any attack that would damage the player uses up one
  shield instead, and that shield comes back after 10 s.
- **Floating Gun:** the first gun floats above the player's right shoulder, slightly higher than the head, and fires
  projectiles at enemies on its own. The second floats above the left shoulder at the same height. From the 3rd pick
  on, each pick gives the guns +10% damage and +10% fire rate.
- **Second Wind:** one killing blow survived per pick, so each pick is used up when it saves the player.

**Rules for every M2b upgrade (user)**
1. **The Extra Jump pattern:**
   - an attribute changed by an infinite, stacking upgrade effect (`GE_Upgrade_<Name>`, a `UGSUpgradeEffect`
     subclass);
   - a `DA_Upgrade_<Name>` asset with the name, a description with the per-pick value, the rarity and MaxStacks;
   - an entry in DA_UpgradePool;
   - a short name for `gs.Upgrade.Grant` (the asset suffix, e.g. `gs.Upgrade.Grant PowderKeg`; the lookup already
     accepts it);
   - a line in `gs.Upgrade.List`.
2. **The order when the player is attacked:**
   1. dash invulnerability (`State.Invulnerable`) blocks the hit;
   2. otherwise a Floating Shield takes it;
   3. otherwise it comes off Health;
   4. if the hit would kill, Second Wind saves the player.
3. **Kills by the Floating Gun and Powder Keg count as the player's kills:** cash drop, XP orb, combo and Leech.
4. **Randomness and damage:** new random rolls use the run stream (`UGSRunRandom`), e.g. the Loaded Dice roll. All
   damage goes through GE_Damage, including the Floating Gun's projectiles and Powder Keg blasts.
5. **Single-player only, co-op-ready as usual:** no new RPCs or replication. New attributes are not replicated, like
   ReloadSpeed and ExtraJumps, and the gaps get `TODO(co-op)`.

**Changes to existing values and tests**
- Dash: the player starts with 1 charge instead of 2 (`MaxCharges` 2 in DA_DashSettings today; Prompt 30).
  - With one charge the dash icon shows no "N/M" count, since `UGSAbilityIconWidget` only shows it above 1.
  - Tests that dash more than once within the 3 s recharge need checking in Prompt 30.
- Tests and the simulator that assume the 4-upgrade pool (`Upgrades.Offers`: 4 upgrades, Rare in ~42% of offers,
  no Epic / Legendary) change in the prompt that adds each upgrade.

**Already in the code** (checked for this plan)
- `FireRate` already scales the player's refire interval (`UGSWeaponData::GetRefireInterval`, GA_Fire).
- `PickupRadius` drives the magnet that cash and XP orbs share.
- The magazine size is `UGSWeaponData::MagazineSize`. `UGSWeaponComponent` (new weapon, reload, "mag full") and the
  HUD's ammo counter read it directly; no attribute scales it yet.
- Dash charges come from `UGSDashSettings::MaxCharges`; GA_Dash allows a dash while its GE_Cooldown_Dash stacks are
  below that.
- (Built in Prompt 33: `Data.Critical` now marks critical hits.) Crits were stubbed. `UGSAttributeSet` passed `bCritical = false` to the damage numbers (`TODO(crits)`), and the HUD
  already draws critical damage numbers. Loaded Dice's double damage can use that path.
- Player shots are hitscan and stop at the first hit. Piercing and the Floating Gun's projectile are new. (Piercing:
  built in Prompt 34, `UGSGameplayAbility_Fire::TraceShot`.)

**Open questions** (settled in each upgrade's prompt; not decided)
- ~~Bigger Mags: how the size rounds (rifle 30 × 1.15 = 34.5, pistol 12 × 1.15 = 13.8). Does a pick fill the extra
  rounds at once?~~ Decided in Prompt 32: rounded up; a pick doesn't refill.
- ~~Floating Shield:~~ Settled in Prompt 36 and confirmed by the user:
  - ~~Does a hit that a shield takes end the kill combo or cost the no-damage round bonus?~~ No: a blocked hit is no
    health lost, so the combo goes on and the no-damage bonus is kept.
  - ~~Does the 10 s recharge pause with the game, like the combo window?~~ Yes: a world timer.
- Loaded Dice and Piercing Rounds: do they also apply to the Floating Gun's projectiles? Is a piercing shot one dice
  roll for every enemy it hits? Partly decided in Prompt 33: every hit rolls (so a piercing shot rolls per enemy hit),
  and the Floating Gun will call the same roll (`GSCombat::RollCriticalHit`). Piercing for the Floating Gun is still
  open.
- ~~Powder Keg: does an enemy killed by a blast explode too (a chain)? Does "bigger" mean the radius only, or the
  damage too?~~ Decided in Prompt 35: yes, chains, each link 0.1 s after the last; both radius and damage grow.
  Changed in Prompt 35c: stacks raise the chance to explode (3% each), the blast stays the same, and each chain kill
  rolls the chance too.
- Floating Gun: do the player's Damage and Fire Rate upgrades also scale the guns? Which enemy does a gun target?
- ~~Second Wind: is there a short invulnerability after coming back?~~ Decided in Prompt 37 (user): yes, 2 s.

**Docs changed**
- CLAUDE.md: the game summary says the upgrade pool grows to 15 in M2b.
- This file:
  - the game summary, the intro and the step table;
  - this entry;
  - **Decided (user)** and **Next up** (the M2b block, Prompts 29–40, before M3);
  - the known issue about empty Epic and Legendary tiers.

## M2b — More level-up upgrades (Prompts 29–40)

Prompt 29 is the design update above.

### Prompt 30 — Dash Charge
- **New attribute `ExtraDashCharges`** (`UGSAttributeSet`, constructor default 0, clamped to ≥ 0; not replicated,
  `TODO(co-op)`, like ExtraJumps).
- **GA_Dash** (`UGSGameplayAbility_Dash`): max charges = `DA_DashSettings.MaxCharges` + ExtraDashCharges (rounded).
  - `GetMaxCharges` reads the attribute live on every check (`CheckCooldown`, `GetCooldownDisplayInfo`), with no
    cached value. So a pick counts from the next press, and a charge added while every charge is recharging is ready
    at once.
  - New `GetAvailableCharges` and the static `FindGrantedDash(ASC)` (debug and tests).
  - Recharge unchanged: GE_Cooldown_Dash stacks, one per spent charge, come back one after another, 3 s each.
- **DA_DashSettings `MaxCharges` 2 → 1** (asset value; the class default stays 2): the player starts a run with one
  dash.
- **Assets:**
  - `Abilities/GE_Upgrade_DashCharge` (`UGSUpgradeEffect_DashCharge`, Additive +1 ExtraDashCharges per stack).
  - `Data/Upgrades/DA_Upgrade_DashCharge`: Rare, MaxStacks 0 (no cap), "+{PerStack} dash charge per stack."
  - Added to DA_UpgradePool, which now holds 5 upgrades.
- **Dash icon** (WBP_AbilityIcon): display logic unchanged. With one charge it shows READY or the countdown; from two
  charges up, the count ("3/4  3.0s"). It reads the ability every frame, so it follows a pick on the next frame.
  New test hooks `GetAbilityTag()` / `GetStatusString()`.
- **Debug:**
  - `gs.Upgrade.Grant DashCharge [N]` (short name from the asset).
  - `gs.Upgrade.List` adds ExtraJumps to its Stats line and a new line `Dash charges: 4 (base 1 + ExtraDashCharges
    3), 4 ready`.
  - `get_player_attributes` and `gs.SetAttr` pick up the new attribute on their own.
- **Offer odds:** the Rare tier is still in ~42% of offers, but Extra Jump and Dash Charge now share it (each ~24.5%;
  Extra Jump was 41.7%).
- **Tests:**
  - New `Upgrades.DashCharge`.
  - `Upgrades.Offers` expects 5 upgrades. Its Rare check (~42% of offers) still holds: with 3 Commons and 2 Rares
    neither tier runs out within one offer.
  - No other test needed a change: only `Abilities.DashInvulnerabilityBlocksDamage` uses GA_Dash, and it dashes
    once. `Upgrades.ExtraJump` and `Map.Boundary` call `StartDash` directly (no charges).
- **PIE check** (town, god mode, round paused):
  - Before: the dash icon showed READY.
  - `gs.Upgrade.Grant DashCharge 3` → three `[upgrade]` lines and "x3 granted". The icon showed "4/4" and the
    "x3 Dash Charge" icon appeared under the XP bar.
  - `gs.Upgrade.List`: "DA_Upgrade_DashCharge [Rare] x3/inf: +1 dash charge per stack" and "Dash charges: 4 (base 1
    + ExtraDashCharges 3), 4 ready".
  - `GSPress Ability.Dash` → the icon showed "3/4  3.0s".
- **Full suite:** 36 tests, all passing, in 82.5 s.

### Prompt 31 — Fire Rate + Magnet
Two Common upgrades on attributes that already existed, so no new attribute and no gameplay code beyond the effects.
- **Effects** (`Abilities/GSGameplayEffects.h`, the `UGSUpgradeEffect` pattern: Multiply (Additive) 1 + bonus, so N
  stacks give base × (1 + bonus × N)):
  - `UGSUpgradeEffect_FireRate` → `Abilities/GE_Upgrade_FireRate`: ×1.08 on `FireRate`. GA_Fire already divides the
    refire interval by FireRate (`UGSWeaponData::GetRefireInterval`), for every player weapon.
  - `UGSUpgradeEffect_Magnet` → `Abilities/GE_Upgrade_Magnet`: ×1.15 on `PickupRadius`. Cash pieces and XP orbs
    share the magnet (`AGSCashPickup::FindPlayerInRange`), so both reach further.
- **Assets** (Common, MaxStacks 0 = no cap, placeholder icon):
  - `Data/Upgrades/DA_Upgrade_FireRate`: "+{PerStack}% fire rate per stack."
  - `Data/Upgrades/DA_Upgrade_Magnet`: "+{PerStack}% pickup radius per stack (cash and XP orbs)."
  - Both added to DA_UpgradePool (7 upgrades).
- **`gs.Upgrade.List`:** the Stats line now prints ReloadSpeed, Damage, FireRate, MoveSpeed, PickupRadius and
  ExtraJumps. Short names `gs.Upgrade.Grant FireRate` / `gs.Upgrade.Grant Magnet`.
- **Offer odds** (simulator, 10,000 offers, seed 1): each Common is in ~50% of offers, down from ~84%. That includes
  Damage, which now competes with four other Commons. The Rare tier is unchanged at 41.7% (Dash Charge 24.6%, Extra
  Jump 24.5%).
- **Tests:**
  - New `Upgrades.FireRateAndMagnet`.
  - `Upgrades.Offers` expects 7 upgrades; its Rare check still holds (5 Commons and 2 Rares: neither tier runs out
    within one offer).
- **Measured in the test** (held rifle fire, normal game speed, editor throttling off): 0.111 s between shots at
  FireRate 1 (refire 0.100 s) and 0.067 s at FireRate 1.8 (refire 0.056 s). Every re-fire comes about one frame
  (~11 ms) after its refire interval. Fixed in Prompt 31b (now exactly 0.100 s and 0.0556 s).
- **PIE check** (town, god mode, round paused):
  - `gs.Upgrade.Grant FireRate 5` → FireRate 1.40. `gs.Upgrade.Grant Magnet 4` → PickupRadius 480
    (`get_player_attributes` before: 1 and 300).
  - `gs.Upgrade.List`: "DA_Upgrade_FireRate [Common] x5/inf: +8% fire rate per stack.", "DA_Upgrade_Magnet [Common]
    x4/inf: +15% pickup radius per stack (cash and XP orbs).", "Stats: ReloadSpeed 1.00, Damage 1.00, FireRate 1.40,
    MoveSpeed 900, PickupRadius 480, ExtraJumps 0".
  - The HUD showed the "x5 Fire Rate" and "x4 Magnet" icons under the XP bar.
- **Full suite:** 37 tests, all passing, in 84.9 s.

### Prompt 31b — Fix: exact fire rate for held fire
Fixes the known issue found in Prompt 31: every held full-auto shot came about a frame late, because GA_Fire ended
after each refire interval and the character's next tick re-activated it.
- **GA_Fire (`UGSGameplayAbility_Fire`) keeps a held burst going inside one activation:**
  - The press fires the first shot. Each later shot is scheduled one refire interval after the previous shot's
    *scheduled* time (`LastShotTime`), on a world timer. It is never timed from the frame the timer happened to fire
    on, so the rate doesn't drift and doesn't depend on the frame rate.
  - **Catch-up:** if one frame spans more than one interval, that frame fires every shot it owes, so the average rate
    stays exact (capped at 256 per update as a runaway guard).
  - **The interval** (weapon RPM × FireRate) is read again for every shot. A FireRate attribute change also re-times the
    pending shot, so a change in the middle of a burst applies from the next shot. This covers a Fire Rate pick on
    the upgrade screen.
  - **Before every shot** it checks what an activation checks (`CanActivateAbility`: not dead, not reloading, no safe
    zone, a weapon equipped), then that the weapon is full auto, the input is still held
    (`UGSAbilitySystemComponent::IsAbilityInputHeld`, new) and there is ammo.
    - Released, semi-auto or empty: it ends normally, and an empty magazine still starts the auto-reload.
    - Dead, safe zone, reloading or no weapon: it ends as cancelled.
    - Either way it ends at the time the next shot would have been due. So State.Firing still lasts one refire
      interval past the last shot: reload and weapon swap stay blocked until then and work right after.
  - Unchanged: semi-auto fires once per press; a press while active does nothing (tapping can't beat the interval);
    the game pause stops the world timer, so it pauses a burst; spread, recoil, damage, ammo per shot and
    GA_EnemyFire.
  - `ShouldRetriggerWhileInputHeld` still starts a new burst while fire stays held after one ended (after the
    auto-reload, or after leaving a safe zone).
  - `TODO(co-op)`: per-shot server sync for a held burst (the loop runs where the input is held).
- **The rifle's held rate is now exactly 600 rounds a minute** (0.100 s between shots); it was ~540 at ~90 fps. Fire
  Rate ×1.8 now gives exactly 0.0556 s (it gave ×1.66 before).
- **Tests:**
  - New `Weapons.HeldFire`. Its burst helpers (`FHeldBurst`, `PollBurst`, `CheckBurst`) check every frame that the
    shot count equals 1 + floor(elapsed / interval), which holds at any frame rate and time dilation.
  - `Upgrades.FireRateAndMagnet` now checks the exact schedule and averages: 0.1007 s over 12 rifle shots (one 0.11 s
    hitch in that run) and 0.0556 s over 16 shots at FireRate 1.8.
  - No other test allowed for the lost frame. `Balance.TimeToKill` counts shots, and its measured times now equal the
    ideal ones (e.g. round 10 melee: 6 rifle shots in 0.50 s).
- **Measured in `Weapons.HeldFire`** (game time ×4):
  - A whole 30-round magazine held: 0.1000 s apart. The auto-reload started 0.4 ms after the 31st shot would have
    been due (the first frame after it).
  - After a mid-burst change to FireRate 1.8: 0.0556 s.
  - Game time ×16 (frames up to 0.13 s, longer than the 0.0556 s interval): up to 3 shots in one frame, and the count
    matched the schedule in every frame.
- **PIE check** (town, god mode, round paused):
  - Fire was held with the engine's console input injection (`Input.+key LeftMouseButton` / `Input.-key
    LeftMouseButton`).
  - Held: the rifle emptied its magazine, auto-reloaded to 30, and ammo kept dropping (30 → 29 → 28) while held.
    After the release it stayed at 28.
  - No warnings from the fire code.
- **Full suite:** 38 tests, all passing, in 91.4 s.

### Prompt 32 — Bigger Mags + Leech
- **Bigger Mags (Common, no cap):**
  - New attribute `MagazineSizeMultiplier` (default 1, clamped to ≥ 0.1; not replicated, `TODO(co-op)`).
  - `UGSWeaponComponent::GetMagazineSize(Data)` = ceil(`MagazineSize` × multiplier), read live from the owner's ASC.
    Owners without the attribute get the plain MagazineSize. A small margin (−0.001) keeps float noise on an exact
    product from adding a round (30 × 1.6 = 48, not 49).
  - It is used for a new weapon's first magazine, reload refills (`RefillCurrentMagazine`), "magazine full" (so a
    reload is offered again after a pick) and the HUD's "ammo / magazine" (read every frame, so it follows a pick at
    once). New `GetCurrentMagazineSize()`. It applies to both weapons.
  - **A pick doesn't refill:** the rounds in the magazine stay; the next reload fills to the new size.
  - `UGSUpgradeEffect_BiggerMags` → `Abilities/GE_Upgrade_BiggerMags` (×1.15 per stack, additive like the other %
    upgrades). `Data/Upgrades/DA_Upgrade_BiggerMags`: "+{PerStack}% magazine size per stack (all weapons, rounded up)."
  - Sizes: rifle 30 → 35 / 39 / 44 / 48 for 1–4 stacks; pistol 12 → 14 / 16 / 18 / 20.
- **Leech (Rare, no cap):**
  - New attribute `HealthOnKill` (default 0, clamped to ≥ 0; not replicated, `TODO(co-op)`).
  - `AGSGameMode::HandleEnemyKilled`: for an enemy kill credited to a player (the same killer the combo uses; target
    dummies don't count), applies `HealthOnKillEffect` to the killer.
    - The effect is `UGSHealthOnKillEffect`, a native instant GE: Health + the target's own HealthOnKill, captured live.
      It is set on AGSGameMode in C++ and can be overridden on BP_GSGameMode.
    - Health's existing clamp keeps it at or below MaxHealth. It is skipped with HealthOnKill 0 or a dead killer.
    - Log: `[upgrade] <PS>: Leech +N on kill -> Health A -> B`.
  - Kills by the Floating Gun and Powder Keg will heal too, once they go through this kill path (M2b rule).
  - `UGSUpgradeEffect_Leech` → `Abilities/GE_Upgrade_Leech` (+1 HealthOnKill per stack). `Data/Upgrades/DA_Upgrade_Leech`:
    "Heal +{PerStack} health per kill, per stack."
- **Pool:** both added to DA_UpgradePool (9 upgrades: 6 Common, 3 Rare).
- **Debug:**
  - `gs.Upgrade.Grant BiggerMags` / `Leech`.
  - `gs.Upgrade.List` adds MagazineSizeMultiplier and HealthOnKill to its Stats line, and a "Magazine: ammo / size" line.
  - HUD test hook `GetAmmoText()`.
- **Offer odds** (10,000 offers, seed 1): each Common ~42% of offers, each Rare ~16.5%. The Rare tier is still 41.7%.
- **Tests:**
  - New `Upgrades.BiggerMagsAndLeech`.
  - `Upgrades.Offers` expects 9 upgrades; its Rare check still holds (6 Commons, 3 Rares).
  - The held-fire helper's magazine size reads the weapon component.
  - First run note: holding fire "for 2 shots" right after PIE started fired 10, because a long first frame at ×4 made
    the burst catch up (correct behaviour). The test now taps twice instead.
- **PIE check** (town, round paused):
  - `gs.Upgrade.Grant BiggerMags 2`, `gs.Upgrade.Grant Leech 3`, `gs.Damage.Self 20` (god off), `gs.Spawn MeleeBandit 1`
    + `gs.KillAll`.
  - Log: "Leech +3 on kill -> Health 80 -> 83", "MagazineSizeMultiplier 1.30, HealthOnKill 3", "Magazine: 30 / 39".
  - The HUD showed "83 / 100" and the rifle at "30 / 39" (not refilled).
- **Full suite:** 39 tests, all passing, in 92.0 s.

### Prompt 33 — Loaded Dice (critical hits)
- **New attribute `CritChance`** (default 0, clamped to 0..1; not replicated, `TODO(co-op)`).
- **The roll, one function for every player damage source:** `GSCombat::RollCriticalHit(SourceASC)` in
  `Combat/GSCriticalHit.*` returns `FGSCritRoll { bCritical, DamageMultiplier }`. GA_Fire calls it; the Floating Gun
  will too (Prompt 38).
  - Chance = the source's CritChance, rolled with `UGSRunRandom::RollChance` on the run stream.
  - At 0 (no Loaded Dice) and at 1 (20 stacks) it needs no dice and leaves the run stream untouched, so runs without
    the upgrade replay exactly as before.
  - A crit multiplies the hit's damage by `CritMultiplier`.
- **`Combat/GSCombatSettings.h` (`UGSCombatSettings`) → `Data/DA_CombatSettings`**, set as `CombatSettings` on
  BP_GSGameMode: `CritMultiplier` 2.0. Without the asset the class defaults apply.
- **GA_Fire:** one roll per hit that lands on a target with an ASC (misses don't roll). Damage = weapon damage × the
  Damage attribute × the crit multiplier. A critical spec carries the new SetByCaller **`Data.Critical`** = 1.
  Spread, recoil, ammo and enemy fire are unchanged.
- **Damage numbers:** `UGSAttributeSet` reads `Data.Critical` from the damage spec, which replaces the
  `TODO(crits)` stub. The HUD already drew critical numbers differently, and now they appear: size 28 instead of 20,
  gold (1, 0.8, 0.1) instead of white, with a "!" ("40!").
  - Test hooks: `GetDamageNumbersAdded`, `GetCriticalDamageNumbersAdded`, `GetLastDamageNumber`,
    `WasLastDamageNumberCritical`, and the crit / normal font sizes and colours.
- **Upgrade:**
  - `UGSUpgradeEffect_LoadedDice` → `Abilities/GE_Upgrade_LoadedDice` (Additive +0.05 CritChance per stack).
  - `Data/Upgrades/DA_Upgrade_LoadedDice`: Rare, MaxStacks 20 (= 100%), "+{PerStackPercent}% chance per stack for a hit
    to be critical (double damage)."
  - Added to DA_UpgradePool (10 upgrades: 6 Common, 4 Rare).
- **Descriptions:** new `{PerStackPercent}` placeholder (the per-stack value × 100), for chances stored as fractions.
- **Debug:** `gs.Upgrade.Grant LoadedDice [N]`. `gs.Upgrade.List` adds CritChance to its Stats line. `gs.SetAttr CritChance`
  works (it reads any attribute).
- **Offer odds** (10,000 offers, seed 1): each Common ~42%, each Rare ~12.4%. The Rare tier is still 41.7%.
- **Tests:**
  - New `Upgrades.LoadedDice`. Its 50% case gave 1013 crits in 2000 rolls.
  - `Upgrades.Offers` expects 10 upgrades.
  - `Run.SameSeedSameRun` and `Balance.TimeToKill` are unchanged and pass: no dice at CritChance 0.
- **PIE check** (town, god mode, round paused):
  - `gs.Upgrade.Grant LoadedDice 10` → "CritChance 0.50" in `gs.Upgrade.List`.
  - With `gs.Debug.DamageLog 1`, held rifle fire at the center target dummy logged 4 hits: 20, 20, 20 and one crit
    of 40. Recoil then lifted the aim over the dummies.
  - A critical number wasn't caught on screen: the numbers last 0.9 s and the follow-up aimed shot missed. The test
    checks the HUD's critical flag, size and colour instead.
- **Full suite:** 40 tests, all passing, in 92.5 s.

### Prompt 34 — Piercing Rounds
- **New attribute `Pierce`** (default 0, clamped to ≥ 0, rounded when read; not replicated, `TODO(co-op)`).
- **`UGSGameplayAbility_Fire::TraceShot(World, Start, End, Pierce, Params)` → `FGSShotTrace { TargetHits,
  BlockingHit, End }`:** one Visibility trace, repeated.
  - A target is anything with an ASC (enemies, target dummies). Each one hit is added to `TargetHits` and ignored, so
    the trace goes on behind it. **Every target at most once per shot.**
  - It stops after 1 + Pierce live targets, or at the first thing without an ASC (**walls and world geometry stop
    it**). Dead targets are passed without using up the pierce.
  - `End` is the last thing hit (the trace end if nothing was hit).
- **FireShot** (GA_Fire):
  - With Pierce > 0 the muzzle trace runs to the weapon's range. With Pierce 0 it still ends just past the aim point,
    exactly as before.
  - Ammo is spent once per shot. **Every target hit takes full damage**, each with **its own crit roll**
    (`GSCombat::RollCriticalHit`) and its own damage spec carrying its own hit result, so **each enemy gets its own
    damage number** at its own impact point.
  - **An impact effect plays at every target and at the wall that stopped the shot.** The shot's line runs to the
    last thing hit. There is no tracer effect yet; the line is the `gs.Debug.Weapons` debug line.
- **Upgrade:** `UGSUpgradeEffect_PiercingRounds` → `Abilities/GE_Upgrade_PiercingRounds` (+1 Pierce per stack).
  `Data/Upgrades/DA_Upgrade_PiercingRounds`: **Epic**, no cap, "Shots pass through +{PerStack} more enemy per stack, at
  full damage (walls still stop them)." Added to DA_UpgradePool (11: 6 Common, 4 Rare, 1 Epic).
- **The Epic tier is no longer empty.** Piercing Rounds is in 13.5% of offers (simulator, seed 1). The Rare tier
  drops to 40.8% of offers (40.6% expected) and each Common to ~40%.
- **Debug:** `gs.Upgrade.Grant PiercingRounds [N]`. `gs.Upgrade.List` adds Pierce to its Stats line.
- **Tests:**
  - New `Upgrades.PiercingRounds`.
  - `Upgrades.Offers` expects 11 upgrades and Rare in ~40.6% of offers (exact for weights 100 / 20 / 6 with one Epic),
    and its simulator check now expects Epic choices.
  - First run: the test expected a shot with pierce left to end at the trace end. The spec says the line ends at the
    last thing hit, so the test was fixed (the code was right).
  - Test run: the trace hit 1 / 3 / 4 bandits for Pierce 0 / 2 / 10, and 2 with a wall between bandits 2 and 3. The
    real shot dealt 40, 20 and 40 (two crits) to bandits 1–3, and 0 to bandit 4.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after):
  - `gs.Upgrade.Grant PiercingRounds 2`, `gs.Debug.DamageLog 1`, `gs.Debug.Weapons 1`, `gs.Spawn MeleeBandit 3`.
  - The bandits were moved into a line (ActorTools; the objects are named `GSEnemyCharacter_N`, the list shows labels)
    and ran up into a single file next to the player (y −167 / −235 / −303).
  - `GSAimAt 0 -200 120` (three times, so the camera settles after a big turn), then `GSFire`.
  - **One shot: `[dmg]` 20 to each of the three bandits at the same time, and three "20" numbers on screen along the
    line.**
  - The first two tries missed. A single `GSAimAt` aims from where the camera is before it swings.
- **Full suite:** 41 tests, all passing, in 92.7 s.

### Prompt 35 — Powder Keg
- **New attribute `PowderKeg`** (stacks; default 0, clamped to ≥ 0, rounded when read; not replicated, `TODO(co-op)`).
- **Tuning in `DA_CombatSettings`** (`UGSCombatSettings`):
  - `BlastDamage` 30 and `BlastDamagePerExtraStack` 10; `BlastRadius` 300 cm and `BlastRadiusPerExtraStack` 50 cm.
    `GetBlastDamage(N)` / `GetBlastRadius(N)` give 1 stack → 30 / 3 m, 3 stacks → 50 / 4 m.
  - `ChainDelay` 0.1 s.
  - `BlastDamageEffect` = GE_Damage.
  - Placeholders: `BlastEffect` = the Niagara template `SimpleExplosion` (scaled with the radius), `BlastSound` =
    `/Engine/VREditor/Sounds/VR_negative_Cue` (editor content).
- **`AGSGameMode`:**
  - `HandleEnemyKilled` → `TriggerPowderKeg` for an **enemy killed by a player** with PowderKeg > 0. It runs after the
    kill is fully counted (drops, combo, Leech, the broadcast).
  - **`ExplodePowderKeg(Location, Player)`** finds every living `AGSEnemyCharacter` within the radius (3D distance from
    where the enemy died, **no line of sight**; players are never targets, so **the player is never hurt**).
  - It applies GE_Damage with damage = blast damage × the player's **Damage** attribute. Blasts don't roll crits.
  - The spec uses the player's own effect context, so **blast kills are the player's**: cash drop with the combo
    multiplier, XP orb, combo count, Leech, run-stats kills. Blast hits show damage numbers.
  - **Chains:** while a blast applies its damage (`bApplyingPowderKeg`), the enemies it kills don't explode at once.
    Each schedules its own blast `ChainDelay` later on a world timer, so chains ripple and the game pause pauses
    them. A new run clears pending chain blasts.
  - Log: `[blast] Powder Keg xN at <loc>: radius R, damage D, H enemies hit, K killed (blast B this run)`.
    `gs.Debug.Weapons 1` draws the blast sphere.
  - `TODO(co-op)`: multicast the effect and sound.
- **Upgrade:** `UGSUpgradeEffect_PowderKeg` → `Abilities/GE_Upgrade_PowderKeg` (+1 PowderKeg per stack).
  `Data/Upgrades/DA_Upgrade_PowderKeg`: **Epic**, no cap, "Enemies you kill explode and hurt enemies nearby. Each stack
  makes the blast bigger and stronger." The numbers stay in DA_CombatSettings only. Added to DA_UpgradePool (12: 6
  Common, 4 Rare, 2 Epic).
- **Debug:** `gs.Upgrade.Grant PowderKeg [N]`. `gs.Upgrade.List` adds PowderKeg to its Stats line.
- **Offer odds** (10,000 offers, seed 1): Piercing Rounds 7.1% and Powder Keg 7.0% of offers; the Epic tier 13.5%, the
  Rare tier 40.8%, each Common ~39–40%.
- **Tests:**
  - New `Upgrades.PowderKeg`.
  - `Upgrades.Offers` expects 12 upgrades and Rare in ~40.5% of offers.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after):
  - `gs.Upgrade.Grant PowderKeg 3` (4 m, 50 damage) and `gs.Upgrade.Grant Damage 20` (×3: a rifle shot kills, a
    blast deals 150). `gs.Spawn MeleeBandit 25`; the bandits crowded the player within 1.7–2.8 m.
  - One aimed `GSFire` with `gs.Perf 3` running started **a chain of 25 blasts**: the shot's kill exploded and killed
    23; their 23 chain blasts went off 0.1 s later and one killed the last bandit, whose blast followed.
  - The screen showed the shot's "60", a field of "150" blast numbers, the placeholder explosions and "25 COMBO / x2
    CASH". The player stayed at 100/100.
  - **Frame time during the chain** (`gs.Perf 3`, 120 fps cap):

    | Chain | Frame avg | p95 | Max | Game max | Render max | GPU max |
    |---|---|---|---|---|---|---|
    | First (cold: effect and sound loaded for the first time) | 9.43 ms (106 fps) | 8.34 | 139.75 | 89.53 | 64.27 | 5.65 |
    | Second (warm, 25 blasts, 22 + 2 kills) | 8.47 ms (118 fps) | 8.33 | 28.88 | 30.32 | 9.34 | 6.38 |

    The 140 ms spike is the first use of the Niagara system and sound. The warm chain's worst frame is 29 ms.
  - The first chain's 25 XP orbs leveled the player up twice, so the upgrade screen paused the game (and the next
    bandits) until the offers were taken: normal game behaviour.
- **Full suite:** 42 tests, all passing, in 93.0 s.

### Prompt 35b — Health orbs
Not an upgrade: a third kind of enemy drop. Added between Prompts 35 and 36; the other numbers stay.
- **Drops** (`AGSGameMode::HandleEnemyKilled` → `RollHealthOrbDrop`): every enemy **killed by the player** (the same
  killer the combo and Leech use: shots, `gs.KillAll`, Powder Keg blasts; the Floating Gun later) rolls
  `HealthOrbDropChance` on the run stream after its cash and XP drops. At 0 there is no roll; at 1 it always drops
  without a roll. Kills that aren't the player's never drop one.
- **Pickup system:** `EGSPickupKind::Health` on `AGSCashPickup`, in the same pool, with the same toss, magnet
  (PickupRadius), RoundClear vacuum and new-run clear as cash and XP. `UGSCashPickupSubsystem::SpawnHealthDrop`
  tosses one orb worth `HealthOrbHealAmount`. New counters: `GetNumHealthOrbsDropped / Collected / Expired`,
  `GetLastHealthCollectLocation`. `IsHealthOrb()` / `IsOrb()`.
- **Full health:**
  - A player at full health doesn't attract health orbs (`AGSCashPickup::IsAtFullHealth`), so they wait on the ground
    until the player is hurt.
  - An orb already flying to a player who reaches full health on the way (another orb healed them) drops back to the
    ground.
  - The RoundClear vacuum **forces** them in (`StartMagnet(Target, bForced)`, and `ForceMagnet()` for orbs already
    flying). They heal then; anything over MaxHealth is lost.
- **Heal** (`AGSGameMode::AwardHealthPickup`):
  - New native instant GE `UGSHealEffect`: Additive on Health from SetByCaller `Data.Heal` (new native tag). It is
    overridable with `HealthOrbHealEffect` in DA_CashSettings; there is no asset.
  - Health's existing clamp keeps it at or below MaxHealth. A dead player gets nothing, and dead players don't
    attract orbs.
  - It records run stats, plays the placeholder sound at the pickup, and shows the popup.
  - Log: `[heal] <PS>: health orb +15 -> Health A -> B (healed N)`.
  - `TODO(co-op)`: an orb heals whoever collects it; the popup and sound are local.
- **Tuning** (DA_CashSettings, new "Health Orb" section):
  - `HealthOrbDropChance` 0.2, `HealthOrbHealAmount` 15, `HealthOrbLifetime` 0 (never expires).
  - `HealthOrbLook`: the engine sphere at ×0.42 (the XP orb is ×0.32) with `Materials/M_HealthOrb` (a copy of M_XPOrb,
    Color (0.05, 1, 0.12)).
  - `HealthOrbPulseAmount` 0.12 and `HealthOrbPulseSpeed` 1. The orb pulses like the XP orb.
  - `HealthOrbSound` = `/Engine/VREditor/Sounds/VR_confirm_Cue` (editor content, placeholder).
- **Popup:** a green "+N" (`UGSHUDWidget::AddHealthPopup`) through `AGSPlayerController::NotifyHealthCollected` (not
  an RPC). N is the health actually healed, so a vacuumed orb at full health shows nothing.
  - Style: `HealthPopupColor` (0.15, 1, 0.45), size 22, 95 cm up. That is brighter than the cash popup and reads "+15",
    not "+$15".
  - Test hook `GetActiveHealthPopupTotal()`.
- **Run stats:** `GetHealthOrbsCollected()` and `GetHealthHealedByOrbs()` (overheal not counted), reset with the run,
  with a "Health orbs" line in `gs.Run.Stats`. Not replicated (`TODO(co-op)`), like BestCombo.
- **Magnet card:** "+{PerStack}% pickup radius per stack (cash, XP and health orbs)."
- **Debug:**
  - `gs.HealthOrb.Spawn [Count=1]` drops orbs around the player with no roll (`AGSGameMode::SpawnHealthOrbsForDebug`).
  - Tests use `AGSGameMode::SetHealthOrbDropChanceOverride(Chance)`; a negative value clears it.
- **Tests:**
  - New `Economy.HealthOrbs`.
  - `Economy.FlatDropsPerRound` and `Economy.CashPickups` now count cash pieces only: a health orb isn't part of the
    flat cash drop, and like XP orbs it doesn't expire. What they check is unchanged.
  - No other test changed. The Leech and Powder Keg tests pass with their seeds.
- **PIE check** (town, round paused, editor throttling off; restored after):
  - `gs.HealthOrb.Spawn 3` at 100/100: three green orbs, clearly larger than an XP orb, stayed beside the player.
  - `gs.Damage.Self 30`: two flew in. Log: "Health 70 -> 85 (healed 15)" and "85 -> 100 (healed 15)". The third
    stayed on the ground.
- **Full suite:** 43 tests, all passing, in 94.8 s.

### Prompt 35c — Powder Keg: chance to explode
Changes Prompt 35. Stacks no longer grow the blast; they raise the chance that a kill explodes.
- **Chance:** `DA_CombatSettings.ChancePerStack` 0.03 (new). Chance = PowderKeg stacks × ChancePerStack, clamped to
  100% (`UGSCombatSettings::GetExplodeChance`, `AGSGameMode::GetPowderKegChance`).
  - `AGSGameMode::RollPowderKegChance(Player)` rolls once per enemy the player kills, at the moment it dies, on the run
    stream.
  - At 0 stacks there is no roll. At 100% (34 stacks) there are no dice either, like crits and health orbs.
  - Kills that aren't the player's never roll.
- **Fixed blast:** always `BlastDamage` 30 × the player's Damage in `BlastRadius` 3 m. **`BlastDamagePerExtraStack`
  and `BlastRadiusPerExtraStack` were removed** (with `GetBlastDamage` / `GetBlastRadius`). DA_CombatSettings needed
  no change: it uses the class defaults for these (ChancePerStack 0.03 included), so it never stored them. The
  explosion effect plays at scale 1.
- **Unchanged:** no line of sight; never hurts the player; no crits; blast kills are the player's (cash, XP orb, health
  orb chance, combo, Leech, run-stats kills).
- **Chains:** an enemy killed by a blast rolls the same chance when it dies. On a hit it explodes `ChainDelay` 0.1 s
  later (world timer; pauses with the game).
- **DA_Upgrade_PowderKeg:** still Epic. MaxStacks 34 (the first stack count at 100%; 33 = 99%). Card: "+3% chance per
  stack that an enemy you kill explodes and hurts enemies nearby."
  - The "3" is typed in the card text. `{PerStack}` would read the GE's +1 stack, not the chance.
  - `Upgrades.PowderKeg` checks that the card matches ChancePerStack, so a retune without the text fails the test.
- **Log:** `[blast] Powder Keg x10 (30% chance per kill) at <loc>: radius 300, damage 30, H enemies hit, K killed (blast B
  this run)`.
- **`gs.Upgrade.List`:** a new line, "Powder Keg: 10 stack(s), 30% chance per kill (3% per stack), blast 30 damage x
  Damage 1.00 in 3.0 m".
- **Tests:**
  - `Upgrades.PowderKeg` was rewritten (see the test table).
  - Health orbs are off in it (override 0), so the exact Leech health check can't be thrown off by an orb.
  - The chain check now waits for the pending blast instead of a fixed 0.2 s. On a long first frame at ×4 the fixed wait
    ended before the timer ticked.
  - The blast-size check centers the blast at the bandits' height. The radius is measured in 3D, and at floor height
    the "2.9 m" bandit was 3.03 m away.
  - `Economy.HealthOrbs` grants 34 stacks instead of 1 for its blast-kill drop.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after):
  - `gs.Upgrade.Grant PowderKeg 34`, `gs.Spawn MeleeBandit 12`, `gs.KillAll`: 12 blasts. 8 came from the KillAll kills
    at once, and 4 were chain blasts of bandits the blasts killed. Every log line read "x34 (100% chance per kill)".
  - `gs.Run.Seed 3535`, `gs.Upgrade.Grant PowderKeg 10`, `gs.Spawn MeleeBandit 20`, `gs.KillAll`: 7 blasts in about 20
    kills.
  - `gs.Upgrade.List` showed "x10/34" and the chance line above.
- **Full suite:** 43 tests, all passing, in 94.4 s.

### Prompt 36 — Floating Shield
- **New attribute `Shields`** (default 0, clamped to ≥ 0, rounded when read; not replicated, `TODO(co-op)`).
- **`Core/GSShieldComponent.*` (`UGSShieldComponent`, on `AGSPlayerState`, `GetShieldComponent()`)** holds the shields.
  It is run state.
  - One slot per shield, kept in sync with the attribute through its change delegate. A new shield is ready at once,
    even while the others recharge. Removed shields drop their recharge.
  - `TryAbsorbHit(Amount, Instigator)`: the first ready shield cancels the **whole** hit, then recharges for
    `DA_CombatSettings.ShieldCooldown` (10 s, new) on its own world timer. Each shield has its own cooldown, and the
    timers pause with the game.
  - Read access: `GetNumShields`, `GetNumReady`, `IsShieldReady`, `GetCooldownRemaining`, `GetHitsBlocked`.
  - Events (native): `OnShieldBlocked(Index)`, `OnShieldReturned(Index)`, `OnShieldCountChanged`.
  - `AGSGameMode::StartRun` calls `ResetShields()` after the upgrades reset.
  - Log prefix `[shield]`: one line per block ("shield 1 blocked 12 damage from …, back in 10.0 s"), per return and
    per count change.
- **In the damage path, not collision:** `UGSAttributeSet::PostGameplayEffectExecute` asks the player's shield
  component before IncomingDamage turns into Health loss. A block returns early, so no Health changes,
  `HandleDamageApplied` isn't called and no damage numbers appear. Melee swings, enemy shots, Powder Keg (never the
  player), `gs.Damage.Self` and anything else through GE_Damage all count.
- **Damage order:**
  1. Dash invulnerability (`State.Invulnerable`; god mode too) keeps GE_Damage from applying at all, so no shield is
     used.
  2. A ready shield.
  3. Health.
  4. (Second Wind on a killing blow comes in Prompt 37.)
- **Decided here and confirmed by the user** (the design left them open): a blocked hit is not damage taken. The kill combo goes on, the
  no-damage round bonus is kept and run-stats damage taken doesn't change. The recharge pauses with the game.
- **Visuals** (`Core/GSFloatingShieldsComponent.*`, `UGSFloatingShieldsComponent` on `AGSCharacter`,
  `GetFloatingShields()`):
  - One flat disc per shield circles the player, 85 cm out, 40 cm above the capsule center (chest height), evenly spaced
    and facing out, turning at 120°/s.
  - The discs are runtime static mesh components with **no collision**, no overlaps, no navigation effect and no
    shadow.
  - It reads the PlayerState's shield component every frame and binds to its events.
  - **Block:** the shield grows to ×1.8 over 0.15 s and disappears. `BlockEffect` (the Niagara `RadialBurst` template,
    ×0.5) and `BlockSound` (`Object_Snaps_To_Another_Actor`, editor content) play there.
  - **Return:** it pops back in from nothing over 0.35 s with `ReturnSound` (`VR_open_Cue`).
  - Everything is set on BP_GSCharacter's `FloatingShields`. The mesh is the engine Cylinder at (0.45, 0.45, 0.06), with
    the new `Materials/M_FloatingShield` (a copy of M_XPOrb, light blue (0.35, 0.8, 1), intensity 2).
  - Test hooks: `GetNumShieldMeshes`, `GetShieldMesh`, `IsShieldShown`, `GetBlocksShown`, `GetReturnsShown`.
  - `TODO(co-op)`: the shield states aren't replicated.
- **Upgrade:** `UGSUpgradeEffect_FloatingShield` → `Abilities/GE_Upgrade_FloatingShield` (+1 Shields per stack).
  `Data/Upgrades/DA_Upgrade_FloatingShield`: **Rare**, no cap, "+{PerStack} floating shield per stack. A shield blocks
  one whole hit, then comes back after a cooldown." Added to DA_UpgradePool (13: 6 Common, 5 Rare, 2 Epic).
- **Debug:**
  - `gs.Upgrade.Grant FloatingShield [N]`.
  - `gs.Upgrade.List` adds Shields to its Stats line and a new line, "Floating shields: 2 (1 ready: ready, 7.5 s), 1
    hit(s) blocked this run".
- **Offer odds** (10,000 offers, seed 1): Floating Shield in 9.5% of offers. Each Rare is now ~9.7% (was ~12%); the
  Rare tier is still 40.8%.
- **Tests:**
  - New `Upgrades.FloatingShield`. Shield 1 came back 10.03 s after its block. A real melee swing (12) and all 4 shots
    of a ranged bandit's burst were blocked.
  - `Upgrades.Offers` expects 13 upgrades. Its Rare rate is unchanged: with 5 Rares the tier can't run out within an
    offer.
  - Also fixed: a stale comment on the PowderKeg attribute (it still described the Prompt 35 blast).
- **PIE check** (town, editor throttling off; restored after):
  - `gs.Round.Pause 1`, `gs.Upgrade.Grant FloatingShield 2`: two light-blue discs circled the player at chest height on
    opposite sides, and the HUD showed "x2 Floating Shield".
  - `gs.Round.Pause 0` with god mode off, standing still in round 1: "shield 1 blocked 12 damage from
    GSEnemyCharacter_0" and "shield 2 blocked 12 damage from GSEnemyCharacter_1". After that the melee bandits killed
    the player, and both shields came back 10 s after their blocks.
- **Full suite:** 44 tests, all passing, in 98.5 s.

### Prompt 37 — Second Wind
- **New attribute `SecondWinds`** (default 0, clamped to ≥ 0, rounded when read; not replicated, `TODO(co-op)`).
- **In the damage path** (`UGSAttributeSet::PostGameplayEffectExecute`). The order is complete now:
  1. Dash invulnerability (GE_Damage isn't applied).
  2. A ready Floating Shield.
  3. Health.
  4. Second Wind, only when the hit would take Health to 0.
  - On a killing blow with a Second Wind, Health goes straight to the restored value and never touches 0, so nothing
    dies. `UGSSecondWindComponent::UseSecondWind` then handles the rest (below).
- **`Core/GSSecondWindComponent.*` (`UGSSecondWindComponent`, on `AGSPlayerState`, `GetSecondWindComponent()`)**, run
  state:
  - `CanSave()`, `GetRestoredHealth(MaxHealth)` (`SecondWindHealthFraction` × MaxHealth, rounded to whole points: in
    float, 100 × 0.3 is a hair above 30, and the HUD showed "31").
  - `UseSecondWind`:
    1. Applies the new native `UGSTimedInvulnerabilityEffect` (has a duration from SetByCaller `Data.Duration`, a new
       tag; grants `State.Invulnerable`) for `SecondWindInvulnerableTime`.
    2. Removes one stack of the upgrade with the new `UGSUpgradeComponent::RemoveStackByEffect`, which also drops the
       attribute and fires `OnUpgradeStacksChanged`. So the card's "Owned", the HUD icon and `gs.Upgrade.List` show
       what's left, and picking it again adds one. Without an upgrade stack (e.g. `gs.SetAttr SecondWinds`) it lowers
       the base value instead.
    3. Fires `OnSecondWindUsed(Left)`.
  - `ResetSecondWind()` from `AGSGameMode::StartRun` clears the uses and any invulnerability left.
  - Log: `[secondwind] <PS>: Second Wind! A killing blow from X at 100 Health -> Health 30, invulnerable 2.0 s, 0
    left`, plus the `[upgrade] … used up (Second Wind), N left` line.
- **Decided here and confirmed by the user** (the prompt didn't say): the saving hit still counts as damage taken, up to the Health it removed
  (e.g. 100), so it ends the kill combo and costs the no-damage round bonus. The player was hit; only the death is
  prevented.
- **Tuning** in DA_CombatSettings (new "Second Wind" section, class defaults): `SecondWindHealthFraction` 0.3,
  `SecondWindInvulnerableTime` 2 s, `SecondWindInvulnerableEffect` (unset = the native effect).
- **Feedback** (`UGSHUDWidget`, bound to `OnSecondWindUsed`; tuning under HUD|Second Wind on WBP_HUD):
  - A strong full-screen gold flash: (1, 0.85, 0.4) from 0.85 alpha, fading over 0.7 s real time.
  - `SecondWindSound`: 2D, the placeholder `Teleport_Committed`.
  - A "SECOND WIND" banner (gold-orange) with "N left", or "THAT WAS YOUR LAST", in the round banner's spot.
    - It shows at once. A showing round banner goes back to the front of the queue.
    - The banner code now has one `ShowBanner(Request)` for every kind, and `GetRoundBannerSubtitle()`.
  - Test hooks: `IsSecondWindFlashShowing`, `GetSecondWindFlashAlpha`, `GetSecondWindSoundsPlayed`,
    `GetSecondWindBannersShown`, `IsSecondWindBannerShowing`.
- **Upgrade:** `UGSUpgradeEffect_SecondWind` → `Abilities/GE_Upgrade_SecondWind` (+1 SecondWinds per stack).
  `Data/Upgrades/DA_Upgrade_SecondWind`: **Legendary**, no cap, "+{PerStack} Second Wind per stack. Survive a killing
  blow: come back with part of your health and a moment of invulnerability. Each one is used up when it saves you."
  Added to DA_UpgradePool (14: 6 Common, 5 Rare, 2 Epic, 1 Legendary). **Every tier has upgrades now.**
- **Debug:**
  - `gs.Upgrade.Grant SecondWind [N]`.
  - `gs.Upgrade.List` adds SecondWinds to its Stats line and a new line, "Second Wind: 0 left, 1 used this run".
- **Offer odds** (10,000 offers, seed 1): Second Wind in 2.3% of offers (about one in 44). The Rare tier dips to 40.6%
  (40.2% expected).
- **Tests:**
  - New `Upgrades.SecondWind`.
  - `Upgrades.Offers` expects 14 upgrades, Rare in ~40.2%, and Legendary choices in the simulator.
  - Two fixes in the new test while building it (not the game):
    - The HUD flash fades in real time while the test runs ×4, so the test shortens its own HUD's flash
      (`SetFloatProperty`).
    - The 2 s invulnerability is checked on the active effect's duration. The frame-sampled wait only checks that it
      doesn't end early: a long first frame at ×4 once overshot to 3.03 s.
- **PIE check** (town, round paused, editor throttling off; restored after):
  - `gs.Upgrade.Grant SecondWind`, `gs.Damage.Self 9999`: the screen flashed gold under "SECOND WIND / THAT WAS YOUR
    LAST", and the player stood at 30 Health ("31" before the rounding fix).
  - `gs.Upgrade.List`: "x0/inf" and "Second Wind: 0 left, 1 used this run".
  - A second `gs.Damage.Self 9999` after the invulnerability killed the player (GameOver).
- **Full suite:** 45 tests, all passing, in 99.9 s.

---

## Console commands and CVars

Non-Shipping cheats. They run on the server; clients forward them through `ServerExecDebugCommand`.

| Command / CVar | What it does |
|---|---|
| `gs.Spawn <Enemy> [Count=1]` | Spawn enemies around the player via the spawn finder (`MeleeBandit`, `RangedBandit` or a full `DA_Enemy_*` name). |
| `gs.KillAll` | Kill every living enemy through GE_Damage, credited to the player (they drop cash and XP orbs; no XP until the orbs are collected). |
| `gs.God [0\|1]` | Toggle/set damage immunity (one `State.Invulnerable` count). |
| `gs.GiveWeapon <Weapon>` | Equip `Rifle` / `Pistol` / `BanditCarbine` with a full mag (replaces the equipped one if both slots are full). |
| `gs.SetAttr <Attr> <Value>` | Set a player attribute base value (Health, MaxHealth, MoveSpeed, Damage, FireRate, PickupRadius, XPGain, ReloadSpeed). |
| `gs.Upgrade.Grant <Name> [Count=1]` | Add stacks of an upgrade (`ReloadSpeed`, `Damage`, `MoveSpeed`, `FireRate`, `Magnet`, `BiggerMags`, `ExtraJump`, `DashCharge`, `Leech`, `LoadedDice`, `PiercingRounds`, `PowderKeg`, `FloatingShield`, `SecondWind` or `DA_Upgrade_*`); stops at MaxStacks. E.g. `gs.Upgrade.Grant ExtraJump 45`. |
| `gs.Upgrade.List` | List every upgrade with rarity, this run's stacks / cap and description, plus ReloadSpeed / Damage / FireRate / MoveSpeed / PickupRadius / ExtraJumps / MagazineSizeMultiplier / HealthOnKill / CritChance / Pierce / PowderKeg / Shields / SecondWinds, the magazine (ammo / size), the dash charges (max = base + ExtraDashCharges, ready now), the floating shields (ready / each recharge / hits blocked), Second Wind (left / used this run) and Powder Keg (stacks, chance per kill, blast damage and radius). |
| `gs.Upgrade.Offer` | Queue a level-up offer without the level (opens at once if none is open) and print the open offer. Refused in GameOver. |
| `gs.Upgrade.Choose <0-2>` | Take a choice of the open offer; the next queued offer opens. |
| `gs.Upgrade.Reroll` | Reroll the open offer for one of this run's rerolls (refused with none left or no offer). |
| `gs.HealthOrb.Spawn [Count=1]` | Drop health orbs around the local player (no drop roll). At full health they wait on the ground until the player is hurt. |
| `gs.XP.Add <N>` | Grant N raw XP to the local player (XPGain not applied; may level up several times). |
| `gs.XP.SetLevel <N>` | Reset to level 1, then add exactly the XP for level N (0 XP into it; clamped to max level). |
| `gs.Profile.AddCash <N>` | Add N to the permanent cash balance (local profile; saved at the next save point). |
| `gs.Profile.Reset` | Wipe the profile: delete the save file, cash 0. |
| `gs.Profile.Save` | Save the profile now. |
| `gs.Profile.Print` | Print slot, save version, cash and owned items to the log and screen. |
| `gs.Damage.Self <n>` | Damage the local player through GAS. |
| `gs.Perf [Seconds=5]` | Log avg/p95/max frame time, game/render/GPU time and enemies alive. |
| `gs.Round.Skip` | Advance to the next round phase (nothing in GameOver, after death). |
| `gs.Round.Set <N>` | Jump to round N and restart it from PreRound. |
| `gs.Round.Pause [0\|1]` | Freeze / resume the phase timer. |
| `gs.Run.Seed [N]` | Town: print the run seed, or restart the run (stream + stats, round 1) with seed N. Hub: print / set the next run's seed (0 = random); the portal passes it on. |
| `gs.Run.Stats` | Print run stats to log and screen (works on clients), plus this round's stats and the last round-clear bonus. |
| `gs.Market.Teleport` / `gs.Market.Return` | Debug shortcuts. Teleport (in a run) opens the hub map (`AGSGameMode::OpenHubMap`). Return (in the hub) starts a run like the portal (`AGSHubGameMode::StartRun`). |
| `gs.Editor.RemoveSublevel <ShortName>` | Editor only: remove a streaming sublevel from the open map (MCP can't); save the map afterwards. |
| `gs.Editor.BuildEnemyStateTree [melee\|ranged\|all]` | Editor only: regenerate ST_MeleeBandit / ST_RangedBandit from code. |
| `gs.Debug.Round 1` (cvar) | On-screen round, phase, time left, area, wave counts, enemy HP/DMG multipliers. |
| `gs.Debug.Spawns 1` (cvar) | Draw the spawn ring and chosen points (green ring, yellow fallback) for 8 s. |
| `gs.Debug.Boundary 1` (cvar) | Draw the map boundary's play area (walls + ceiling) every frame. |
| `gs.Debug.Weapons 1` (cvar) | Debug-draw shots and log enemy shots (`[shot]`). |
| `gs.Debug.DamageLog 1` (cvar) | Log every health loss (`[dmg] victim took N from X (health) at t=`). |
| `gs.Wave.MaxAlive <N>` (cvar) | Override the alive cap (0 = DA_WaveSettings). |
| `gs.Cash.Lifetime <s>` (cvar) | Lifetime of uncollected cash pieces (0 = DA_CashSettings). |
| `GSFire`, `GSPress <InputTag>`, `GSAimAt X Y Z`, `GSJump` | Exec on the controller: drive fire, any ability input, the camera, or a jump tap from the console. |

Launch arg: `-GSSeed=N` fixes the run seed.

MCP toolset `banditballer_toolset.toolsets.BanditBallerTools`:
- `spawn_enemies(type, count)` (PIE)
- `get_player_attributes()` (PIE): attributes plus Ammo, Cash (profile balance), Level, XP, XPForNextLevel, TotalXP,
  Combo, ComboMultiplier, ComboTimeLeft, God, Invulnerable, Dead.
- `list_enemies_alive()` (PIE)
- `simulate_upgrade_offers(count=10000, seed=1)` (no PIE needed): rolls DA_UpgradePool offers with its own stream and
  returns JSON text with how often each upgrade and rarity appears.

## GAS reference

**Attributes** (`UGSAttributeSet`, on the player's PlayerState and on each enemy):

| Attribute | Notes |
|---|---|
| Health / MaxHealth | Clamped 0..MaxHealth; 0 → `State.Dead` via `UGSHealthComponent`. |
| MoveSpeed | Drives MaxWalkSpeed. Player value comes from DA_PlayerMovementSettings (900); enemies from UGSEnemyData ± variance. |
| Damage | Outgoing damage multiplier (player 1.0). Enemies: scales melee AttackDamage / weapon Damage; round-scaled. |
| FireRate | Refire multiplier (1.0): GA_Fire's refire interval divides by it. Raised by the Fire Rate upgrade. |
| PickupRadius | Magnet radius in cm for cash pieces and XP orbs (player 300). Raised by the Magnet upgrade. |
| XPGain | XP multiplier on kill XP (player 1.0; constructor default 1.0). |
| ReloadSpeed | Reload time divisor for GA_Reload (constructor default 1.0, min 0.1). Not replicated yet. |
| ExtraJumps | Air jumps; AGSCharacter keeps JumpMaxCount = 1 + ExtraJumps (constructor default 0, min 0). Not replicated yet. |
| ExtraDashCharges | Dash charges on top of DA_DashSettings MaxCharges; GA_Dash reads it live (constructor default 0, min 0). Not replicated yet. |
| MagazineSizeMultiplier | Magazine = ceil(weapon MagazineSize × it) for every player weapon, read live by UGSWeaponComponent (constructor default 1, min 0.1). Raised by Bigger Mags. Not replicated yet. |
| HealthOnKill | Health healed per enemy kill credited to the player (UGSHealthOnKillEffect, never above MaxHealth; constructor default 0, min 0). Raised by Leech. Not replicated yet. |
| CritChance | Chance (0..1) that a hit from the player's weapons is critical (GSCombat::RollCriticalHit, run stream; damage × DA_CombatSettings CritMultiplier 2). Constructor default 0, clamped 0..1. Raised by Loaded Dice. Not replicated yet. |
| Pierce | Targets a player shot passes through (UGSGameplayAbility_Fire::TraceShot): up to 1 + Pierce targets, each once, full damage; walls stop it. Constructor default 0, min 0, rounded. Raised by Piercing Rounds. Not replicated yet. |
| SecondWinds | Second Winds left: a hit that would take Health to 0 uses one instead (UGSSecondWindComponent on the PlayerState): Health goes to 30% of MaxHealth (whole points), 2 s of State.Invulnerable, one stack of the upgrade removed. Constructor default 0, min 0, rounded. Raised by Second Wind. Not replicated yet. |
| Shields | Floating shields: each blocks one whole hit in the damage path, then recharges for DA_CombatSettings ShieldCooldown (UGSShieldComponent on the PlayerState; drawn by AGSCharacter's UGSFloatingShieldsComponent). Constructor default 0, min 0, rounded. Raised by Floating Shield. Not replicated yet. |
| PowderKeg | Powder Keg stacks: each of the player's enemy kills explodes with chance stacks × DA_CombatSettings ChancePerStack (3%, max 100%; run stream; AGSGameMode::RollPowderKegChance / ExplodePowderKeg). The blast is fixed: BlastDamage × Damage in BlastRadius. Constructor default 0, min 0, rounded. Not replicated yet. |
| IncomingDamage | Meta attribute: GE_Damage writes it, the set converts it to Health loss and records the instigator. |

**Gameplay tags** (native, `Abilities/GSGameplayTags.*`; none in config):
- State: `State.Dead`, `State.Sliding`, `State.Dashing`, `State.Firing`, `State.Reloading`,
  `State.Invulnerable`, `State.Attacking`, `State.SafeZone` (hub: blocks GA_Fire, GE_Damage ignores the target)
- Ability: `Ability.Fire`, `Ability.Reload`, `Ability.Slide`, `Ability.Dash`, `Ability.SwapWeapon`,
  `Ability.Attack.Melee`, `Ability.Attack.Ranged`
- Cooldown: `Cooldown.Slide`, `Cooldown.Dash`
- SetByCaller data: `Data.Damage`, `Data.Critical` (1 on a critical hit's damage spec), `Data.Heal` (health orbs), `Data.Duration` (timed state effects: Second Wind invulnerability), `Data.Cooldown`,
  `Data.HealthMultiplier`, `Data.DamageMultiplier`

**Gameplay Effects** (`Content/BanditBaller/Abilities/`):

| GE | Class | What it does |
|---|---|---|
| `GE_InitPlayerStats` | UGameplayEffect | Instant overrides: MaxHealth 100, Health 100, Damage 1, FireRate 1, PickupRadius 300, XPGain 1. Granted through DA_PlayerAbilitySet. |
| `GE_Damage` | `UGSDamageEffect` | SetByCaller `Data.Damage` → IncomingDamage. Blocked by `State.Invulnerable` / `State.Dead` / `State.SafeZone`. All damage uses it. |
| `GE_SafeZone` | `UGSSafeZoneEffect` | Infinite; grants `State.SafeZone`. AGSHubGameMode applies it to every player in the hub. |
| `GE_Cooldown_Slide` | `UGSCooldownEffect_Slide` | Duration from `Data.Cooldown`; grants `Cooldown.Slide`. |
| `GE_Cooldown_Dash` | `UGSCooldownEffect_Dash` | One stack per spent charge; grants `Cooldown.Dash`. |
| `GE_EnemyRoundScaling` | `UGSEnemyRoundScalingEffect` | Infinite; MultiplyCompound MaxHealth / Damage from `Data.HealthMultiplier` / `Data.DamageMultiplier`. |
| `GE_LevelUp_MaxHealth` | `UGSLevelUpMaxHealthEffect` | Infinite, stacking (no limit); +10 MaxHealth per stack, one stack per level-up (DA_LevelRewards). |
| `GE_FullHeal` | `UGSFullHealEffect` | Instant; Health = target's current MaxHealth (attribute-based). Applied after level-up rewards. |
| `GE_Upgrade_ReloadSpeed` | `UGSUpgradeEffect_ReloadSpeed` | Infinite, stacking; Multiply (Additive) 1.10 on ReloadSpeed → +10% of base per stack. |
| `GE_Upgrade_Damage` | `UGSUpgradeEffect_Damage` | Infinite, stacking; Multiply (Additive) 1.10 on Damage → +10% of base per stack. |
| `GE_Upgrade_MoveSpeed` | `UGSUpgradeEffect_MoveSpeed` | Infinite, stacking; Multiply (Additive) 1.08 on MoveSpeed → +8% of base per stack. |
| `GE_Upgrade_ExtraJump` | `UGSUpgradeEffect_ExtraJump` | Infinite, stacking; Additive +1 ExtraJumps per stack. |
| `GE_Upgrade_DashCharge` | `UGSUpgradeEffect_DashCharge` | Infinite, stacking; Additive +1 ExtraDashCharges per stack. |
| `GE_Upgrade_FireRate` | `UGSUpgradeEffect_FireRate` | Infinite, stacking; Multiply (Additive) 1.08 on FireRate → +8% of base per stack. |
| `GE_Upgrade_Magnet` | `UGSUpgradeEffect_Magnet` | Infinite, stacking; Multiply (Additive) 1.15 on PickupRadius → +15% of base per stack. |
| `GE_Upgrade_BiggerMags` | `UGSUpgradeEffect_BiggerMags` | Infinite, stacking; Multiply (Additive) 1.15 on MagazineSizeMultiplier → +15% per stack. |
| `GE_Upgrade_Leech` | `UGSUpgradeEffect_Leech` | Infinite, stacking; Additive +1 HealthOnKill per stack. |
| `GE_Upgrade_PiercingRounds` | `UGSUpgradeEffect_PiercingRounds` | Infinite, stacking; Additive +1 Pierce per stack. |
| `GE_Upgrade_PowderKeg` | `UGSUpgradeEffect_PowderKeg` | Infinite, stacking; Additive +1 PowderKeg per stack. |
| `GE_Upgrade_FloatingShield` | `UGSUpgradeEffect_FloatingShield` | Infinite, stacking; Additive +1 Shields per stack. |
| `GE_Upgrade_SecondWind` | `UGSUpgradeEffect_SecondWind` | Infinite, stacking; Additive +1 SecondWinds per stack. A used Second Wind removes one stack. |
| (native, no asset) | `UGSTimedInvulnerabilityEffect` | Has duration (SetByCaller `Data.Duration`); grants `State.Invulnerable`. Second Wind applies it for 2 s (DA_CombatSettings `SecondWindInvulnerableEffect` can replace it). |
| `GE_Upgrade_LoadedDice` | `UGSUpgradeEffect_LoadedDice` | Infinite, stacking; Additive +0.05 CritChance per stack (MaxStacks 20 on the upgrade). |
| (native, no asset) | `UGSHealEffect` | Instant; Health + SetByCaller `Data.Heal`, clamped to MaxHealth. AGSGameMode::AwardHealthPickup applies it when a health orb is collected (DA_CashSettings `HealthOrbHealEffect` can replace it). |
| (native, no asset) | `UGSHealthOnKillEffect` | Instant; Health + the target's HealthOnKill (captured live). AGSGameMode applies it to the killer per enemy kill (`HealthOnKillEffect`). |

**Abilities:**
- Player (`DA_PlayerAbilitySet`): `GA_Fire`, `GA_Reload`, `GA_SwapWeapon`, `GA_Slide`, `GA_Dash`.
- Enemies (`DA_EnemyAbilities_*`): `GA_MeleeAttack`, `GA_EnemyFire`.

## Automation tests (45, all passing, about 100 s)

`Tests/GSGameplayTests.cpp`. Each test opens L_Town_Graybox in the editor if needed (`Map.Hub`: L_Hub_Graybox),
starts PIE, waits for the player, removes enemies, runs and ends PIE. Stop PIE before running. While working, run
the area's tests; run the full suite once per prompt before the commit (see **Workflow update — faster test runs**).
`UI.RoundHUD`, `Round.PhaseLoop`, `Round.ClearBonus`, `Map.TownLayout` and `Upgrades.DashCharge` run game time ×4 and
`Upgrades.ExtraJump` ×3 (`SpeedUpWorld`). `Economy.HealthOrbs` also runs ×4. `Upgrades.FireRateAndMagnet` measures its shots at ×1 (throttling off) and
runs its magnet part at ×4. `Weapons.HeldFire` runs at ×4, and ×16 for its slow-frame burst. `UI.RoundHUD` also shortens its HUD's banners.

| Test | Checks |
|---|---|
| `GAS.DamageReducesHealth` | GE_Damage lowers Health. |
| `GAS.HealthClampsToZeroAndMax` | Health clamps at 0 and MaxHealth. |
| `GAS.DeathAddsStateDead` | Death adds `State.Dead`. |
| `Weapons.FireConsumesAmmoReloadRefills` | GA_Fire uses ammo, GA_Reload refills. |
| `Weapons.HeldFire` | Rifle 600 RPM (0.100 s). Every frame of every burst: shots = 1 + floor(elapsed / interval). Held 6 shots: reload refused while firing; release: no further shot, State.Firing ends in the first frame at or after the next shot's due time, and a reload starts in that frame. A whole magazine held: 30 shots on schedule, the auto-reload starts when the 31st would have been due, held fire resumes after it. Fire Rate x10 granted mid-burst: the next shots follow 0.0556 s from the last shot's scheduled time. Game time ×16 (frames longer than the interval): catch-up fires several shots in one frame, still on schedule. Game paused mid-burst: no shot, no game time, the burst resumes on schedule. Safe zone mid-burst: ends at the next due time, no reload, held fire doesn't restart until it's gone, then resumes. Pistol (semi-auto): one shot per press however long it's held, a second press inside the interval does nothing, the next press after it fires. Death mid-burst: firing ends at once, no shot, no reload. |
| `Abilities.DashInvulnerabilityBlocksDamage` | Dash i-frames block damage. |
| `Map.Hub` | PIE in L_Hub_Graybox: AGSHubGameMode (no AGSGameMode), area MarketDistrict, round 0 / WaitingToStart; Casino, 4 stalls, MarketArrival, MarketExitPortalSpot; player starts at MarketArrival; no NavMesh; AGSMapBoundary contains the player and the Casino; gs.Spawn refused, no enemies; State.SafeZone (GE_SafeZone) on the player, GE_Damage blocked, the fire input fires nothing (no State.Firing, ammo unchanged); HUD in the safe area hides every combat element and shows the cash (= profile); still no round half a second later. |
| `Map.Town` | PIE in L_Town_Graybox: AGSGameMode (no hub GameMode), area Town, run started; no sublevels; no TownReturn / MarketPortalSpot / MarketArrival / MarketExitPortalSpot / Casino / MarketStall / Market actors; EnemySpawn points on the NavMesh; NavMesh under the player; boundary; no State.SafeZone; HUD not in a safe area; gs.Market.Teleport opens the hub (AGSHubGameMode, MarketDistrict); gs.Market.Return starts a run and opens the town again (round 1). |
| `Map.TownLayout` | Town 300 × 300 m: boundary PlayAreaSize 30000 × 30000, centered, ceiling 7500; ground with its top at Z 0 at the center, the edge midpoints and the corners; Kill Z below the ground; NavMesh bounds cover the ground; a tagged landmark on each side, more than 110 m out and at least 15 m tall; PlayerStart at the center; 4 EnemySpawn points at least 130 m out, inside the boundary, on the NavMesh; no ground NavMesh islands (5 m grid, at least 2800 ground samples, every one with a path from the PlayerStart); a path to every edge and corner; launched into the wall at each edge and corner, the player stays inside and on the ground; a spawn ring of 6 rolled at each corner: every point inside the boundary, on the NavMesh, in the ring or a fallback. |
| `Map.StartRunPortal` | PIE in the hub: a portal at MarketExitPortalSpot ("START RUN"), RunMap = L_Town_Graybox; gs.Run.Seed 4242 in the hub sets the next seed; walking into the trigger starts the run (fade to black, a second start refused); the town loads with seed 4242 (GameMode + GameState), round 1, fresh run stats, area Town, same cash, no safe zone; round 1's first spawns: no fallback points, closest ≥ ring min, farthest ≤ ring max, every enemy on the NavMesh and inside the boundary. |
| `Map.Boundary` | AGSMapBoundary in the town (ceiling 7500), player inside, cvar exists; 5 boxes: MapBoundary object type, block Pawn, ignore Visibility/Camera/WorldStatic/WorldDynamic/PhysicsBody, no NavMesh effect; player and enemy capsules block MapBoundary; Visibility / Camera / enemy-shot / pickup traces pass through a wall and the ceiling, a pawn sweep doesn't; spawn points near a corner are inside; player and a frozen enemy launched at a wall stay inside; dash and slide into the wall stop inside; a launch up stops at the ceiling (no damage, same XY); gs.Upgrade.Grant ExtraJump 3; an air jump under the ceiling works and is stopped by it. |
| `Round.PhaseLoop` | PreRound on start, timer → Combat; no market phases in the enum; RoundClear timer → round 2 PreRound; skips round 2 → round 3 PreRound; gs.Round.Set; death → GameOver (no end time, a skip doesn't leave it), still GameOver in round 5 after the PreRound time. |
| `Player.ReturnToHubRestoresInput` | Die → run summary with UI input and the cursor → R once the input lock is over (the widget's key path) → returning → the hub map (AGSHubGameMode, no run) → game input back, no cursor, no summary, pawn not dead. |
| `Run.GameOver` | Return refused while alive; run time format; a level-up pick + 3 upgrade stacks, round 1 with 2 kills, a pickup and its clear bonus; round 2 wave running with enemies to come; an offer open at death (paused) → death: GameOver, still round 2, wave stopped (no telegraphs, nothing left to spawn), living enemies stay, untimed, a skip does nothing, offer closed and unpaused; debug XP after death levels up with no offer or pause; run time recorded, profile saved; summary shown with UI input + cursor, locked (R and the button do nothing); 3 s later: no spawns or new enemies, no RoundClear / bonus, run time frozen, same cash; killing the leftovers clears nothing; summary lines (rounds, level, kills, combo, cash earned, total cash, run time, seed, upgrades text + one icon per upgrade with stacks); held R (repeat) and Space do nothing; the "Return to Market" button → returning, second press / request refused, camera fades to black, the summary fades out before the hub opens; the hub: cash kept, no summary, game input, no cursor, unfaded, no upgrades. |
| `Spawn.LocationFinder` | 12 points: on NavMesh, reachable, in ring, spaced, inside the boundary; same seed → same points; no ring NavMesh (player 50 m up) → 2 flagged EnemySpawn fallbacks, on NavMesh, inside, spaced, and the fallback warning is logged. (No spawns in the Market District: `Map.Hub`.) |
| `Round.WaveDirector` | Round 1 size matches DT_Waves; killing the first arrivals while more are to come keeps Combat; killing the whole wave ends Combat; skipping removes leftovers. |
| `Round.WaveBudget` | Seeded purchase within budget, MinRound respected, alive cap with telegraphs, spawns resume. |
| `Round.DifficultyScaling` | Round 1 base stats; round 10 = base × curve, full health; GameState multipliers. |
| `Balance.TimeToKill` | Rifle/pistol shots-to-kill at rounds 1/5/10 vs both bandits via the real fire path (`[ttk]` log). XPGain 0, so kills don't open upgrade offers. |
| `Run.SeedAndStats` | Same seed replays rolls; restart resets stats; kills per type; damage dealt/taken; rounds cleared. |
| `Run.SameSeedSameRun` | Rounds 1, 2, 5, 8 (generated) replayed with one seed, in one frame: the same enemies in spawn order and the same spawn point for each (all batches run at once, player still), the same level-up offer, 3 rerolls, refused 4th and next offer; every enemy placed; another seed gives another run. |
| `Run.NewRunResets` | A run with 2 level-ups (a reroll), 6 upgrade picks, air jumps, +20 MaxHealth, a kill and $77 → death → hub (cash kept) → Start Run → town: level 1, 0 XP, no upgrades / offers, rerolls back to 3, ExtraJumps 0 / JumpMaxCount 1, base Damage / MoveSpeed / MaxHealth at full health, round 1, no kills / cash earned / combo, cash kept; HUD level 1, no icons, cash. |
| `Economy.FlatDropsPerRound` | Melee and ranged bandits (frozen) killed in round 1 and round 10 with no combo: round 10 has more HP, but the cash (CashValue, same pieces) and the single XP orb (XPValue) are the same. |
| `Economy.CashPickups` | Exact split; out-of-reach drop stays; radius collects into Cash/CashEarned/HUD + popup; pool reuse; RoundClear vacuum; short lifetime blinks, expires unpaid. Counts cash pickups only (the kills' XP orbs never expire). Ends the kill combo before each kill, so every drop is exactly CashValue. |
| `Economy.KillCombo` | DA_ComboSettings on the GameMode (2.5 s, x1/x1.25/x1.5/x2 steps, end on damage, no XP); step lookup at each boundary; kills 1–10 in a row: count +1, multiplier, cash drop = round(CashValue × multiplier), XP orb unchanged, window restarted, HUD count / multiplier / shown from 2, "10 COMBO" / "x2 CASH", best combo; a target dummy kill doesn't count; the window drains with game time and the HUD bar follows; a kill inside the window continues; it times out one window after the last kill (HUD hidden); pickups collected after the combo ended pay the kill-time multipliers; a level-up pause freezes the window (1 s real time), picking resumes it; god mode keeps the combo, health lost ends it (DamageTaken); override bEndComboOnDamage off / bApplyComboToXP on: XP orbs multiplied, damage keeps the combo; a new run ends it (Reset) and clears the best combo; gs.Run.Stats shows it. |
| `Economy.HealthOrbs` | DA_CashSettings: chance 20%, heal 15, no lifetime, M_HealthOrb look larger than the XP orb, a sound; the Magnet card mentions health orbs; chance 0: 10 player kills drop none; chance 1: 5 kills drop 5; an orb worth 15 with no expiry and the orb look; at full health, standing on it: not attracted or collected (1.5 s); hurt to 60: collected, Health 75 (+15), run stats 1 orb / 15 healed, green popup; `gs.HealthOrb.Spawn 2` at 75: both collected, Health = MaxHealth (only 25 healed, 40 in total); a spawned orb at full health waits; the RoundClear vacuum collects it anyway, nothing healed over MaxHealth; a kill that isn't the player's drops none; a shot kill + its Powder Keg blast kill drop 2; a new run clears orbs and the stats; a dead player collects nothing. Game time ×4. |
| `Economy.XPOrbs` | XPOrbLifetime 0; kill → no XP, one orb worth XPValue with the orb mesh / material, no expiry; cash expires (gs.Cash.Lifetime 1.5) but the orb stays, no blink; walking in collects it → XP = XPValue × XPGain 1.5, run stats XPEarned, "+N XP" popup at the collection point; out-of-reach orb waits, RoundClear vacuums it; gs.XP.Add is instant; a new run clears orbs on the ground; a dead player collects nothing. |
| `Progression.XPAndLevels` | Curve L2 = 100 / L3 = 125, max 50; partial XP; 260 XP → level 3 with 35 carried over (HUD level + bar follow), one offer open + one queued; kill gives no XP but one orb; collected orb XP = XPValue × XPGain (×1, ×2); run stats XP/level; SetLevel 10 → 0 XP; level 10 → cap in one gain queues one offer per level (40); cap at max level; new run resets. |
| `Progression.LevelUpRewards` | DA_LevelRewards wiring; +10 per stack; damaged → two level-ups → MaxHealth +20, Health full, 2 stacks, HUD health/level/XP follow; same in RoundClear; new run removes stacks and clamps Health; dead player gets the stack but no heal. Takes each upgrade offer (choice 0) so the game unpauses. |
| `Progression.LevelUpFeedback` | DescribeRewards "+10" / "+20 Max Health"; kill → no popup, collected orb → "+XPValue XP" at the collection point; double level-up → banner "Level 3" + "+20 Max Health", label, XP bar pulse, one sound, player glow; non-kill XP pops above the player; level-up during the banner merges (level 4, "+30"); 3 upgrade offers taken; banner/pulse/glow end. |
| `UI.UpgradeChoice` | Choice widget from the PlayerController; level-up pauses (GameMode owns the pause) and the cards wait for the banner; cards match the offer (order, rarity color, "Owned: N"); "Reroll (3 left)", cursor shown; key / click ignored during the 0.3 s lock; R, gamepad Y and the button reroll (label 2/1/0, new cards, lock again), greyed out at 0 and R does nothing; key 2 picks the 2nd card, last pick hides, unpauses, cursor off; 1000 XP → 1 open + 3 queued; D-pad selects, A picks; next offer swaps in at once (no banner wait), locked, still paused; click / numpad 1 / key 3 pick the rest; hidden and unpaused after the queue. |
| `UI.RoundHUD` | PreRound: "ROUND 1", "PRE-ROUND", countdown "Round 1 in 3…" → "… 2…" a second later, no enemy count, ROUND 1 banner, no icons; gs.Round.Set 2 restarts text + countdown and shows ROUND 2; a level-up hides it and queues it again (LEVEL UP!, offer open, paused); 3.5 s with the cards up: still waiting; the pick replays ROUND 2; icons: picked upgrade x1 → same again x2 (one icon) → another upgrade (two icons); Combat: "COMBAT", no countdown, "N enemies left" = wave, a kill updates it; RoundClear: phase text, no enemy count, ROUND CLEAR banner; a level-up during ROUND CLEAR then round 3 starting with the cards up → queued ROUND CLEAR + ROUND 3 → after LEVEL UP! only ROUND 3 shows (stale one dropped); no frame with a round banner over LEVEL UP! or an open offer; a new run clears the icons. |
| `Profile.PermanentCash` | Test slot; reset deletes the save; pickup → profile + HUD + CashEarned; spend / no overspend; no save on change; Combat → RoundClear pays the round-clear bonus into the profile; save when RoundClear ends includes it (version, no owned items); new run resets CashEarned but keeps the balance; death saves; balance survives death + return to the hub (the hub's HUD shows it); quitting saves it and the next PIE session (new GameInstance) loads it (HUD too); `gs.Profile.Reset` through the console → 0 and no save file. |
| `Round.ClearBonus` | DA_RoundClearSettings on the GameMode ($20 / $15 / $15, 8 s + 1.5 s per enemy) and the summary widget on the controller; PreRound starts the round's stats (no payout, no summary); Combat records its start and wave size; round 1 killed fast with no damage → base + no damage + fast clear, kills = wave size, profile += bonus + cash collected; summary shows round, title and each line; after the vacuum cash collected ≥ kills × CashValue, XP = kills × XPValue, profile and CashEarned += cash collected + bonus, summary lines follow; round 2 PreRound hides the summary and starts from zero; damage taken + an impossible time limit (override) → base only, "missed" lines, total = base; RoundClear from PreRound pays nothing and shows nothing; round 10 (skip) pays the same as round 1; gs.Run.Stats shows the last bonus; a new run clears payout and round stats. |
| `Upgrades.StatUpgrades` | The 3 DA_Upgrade_* assets (Common, no cap, description shows +10% / +8%); ReloadSpeed ×5 via `gs.Upgrade.Grant` → 1.5, 5 GE stacks, real reload shorter (≥ ReloadTime / 1.5); Damage ×3 → 1.3; MoveSpeed ×2 → base × 1.16 and MaxWalkSpeed follows; MaxStacks 1 refuses a second pick; unknown name refused; new run removes all upgrades and restores the stats. |
| `Upgrades.FloatingShield` | DA_Upgrade_FloatingShield (Rare, no cap, "+1", GE class, in the pool); ShieldCooldown 10 s; Shields 0, no shields or meshes; no shield: a hit takes Health; `gs.Upgrade.Grant FloatingShield 2` → 2 shields, both ready, 2 meshes shown, no collision, on the orbit at chest height, 180° apart; a real dash's i-frames block a hit with no shield used; block 1: no Health lost, 1 of 2 ready, the first shield gone, 10 s cooldown, counted, no run-stats damage taken, the combo goes on, block flash shown; the blocked mesh hidden, the other shown; block 2 (2 s later): none ready, each shield on its own cooldown; a third hit takes Health (run stats 25 damage); shield 1 back 10 s after its block (return shown, visible again), shield 2 back 10 s after its own; a pick while one recharges: the new shield ready at once, 3 meshes 120° apart; a real melee bandit's swing and a ranged bandit's burst are blocked (no Health lost; 4 of 4 shots in the run); a new run → Shields 0, no shields, no meshes. Health orbs off. Game time ×4. |
| `Upgrades.FireRateAndMagnet` | DA_Upgrade_FireRate (+8%) and DA_Upgrade_Magnet (+15%): Common, no cap, their GE classes, in DA_UpgradePool; FireRate 1 and PickupRadius 300 at start; rifle held for 12 shots: on the exact schedule every frame, 0.100 s apart on average (within a frame over the burst); `gs.Upgrade.Grant FireRate 10` → FireRate 1.8, refire interval / 1.8 (0.0556 s), 16 held shots on that schedule, 1.8 times as fast; a bandit's drop (cash + XP orb) lands 380–660 cm away: nothing flies in at radius 300; `gs.Upgrade.Grant Magnet 10` → PickupRadius 750, the cash and the orb fly in (cash += CashValue, XP += XPValue); a new run resets both. |
| `Upgrades.PowderKeg` | DA_CombatSettings: ChancePerStack 3%, blast 30 / 3 m, chain delay 0.1 s, damage effect, placeholder effect and sound; chance 0 at 0 stacks, 3% at 1, 30% at 10, 99% at 33, 100% at 34 and clamped above; DA_Upgrade_PowderKeg: Epic, MaxStacks 34 (the first count at 100%), card text "+3% chance per stack" matches ChancePerStack, GE class, in the pool; 0 stacks: 100 kill rolls never explode and draw nothing from the run stream, a real kill doesn't explode; 34 stacks (console; a 35th refused): 100 rolls all explode with no dice; A's kill explodes at once, B (20 HP, 2 m) dies, C (4.5 m) and D (4 m) untouched, the player (1 m, god off) unhurt and Leech healed both kills, combo +2, run-stats kills +2, cash and 2 XP orbs, B's chain blast pending; it fires and hits C for 30, D untouched; a kill that isn't the player's doesn't explode even at 100%; the blast is the same at 1 and 20 stacks (30 at 2.9 m, nothing at 3.1 m); Damage x1.1 → 33; 10 stacks: 1,000 kill rolls within 26–34% (314 in the run) and the same stream state replays them; a new run → 0. Health orbs off (override 0). Game time ×4. |
| `Upgrades.PiercingRounds` | DA_Upgrade_PiercingRounds (Epic, no cap, "+1", GE class, in the pool); Pierce 0 at start; TraceShot along 4 frozen bandits in a line: Pierce 0 → the nearest, ending in it; Pierce 2 → bandits 1–3 in order, ending in the third; Pierce 10 → all 4, each once, ending at the last; a line with nothing on it runs to its end; a spawned wall between bandits 2 and 3 stops a Pierce 10 shot after 2, ending at the wall; a dead bandit is passed without using the pierce; a real shot from a zero-spread copy of the rifle with Piercing x2 and Loaded Dice x10 at 3 bandits placed on its exact line (+ a 4th): bandits 1–3 each take one full hit (20 or 40), bandit 4 none, 3 damage numbers, critical numbers = crits, exactly 3 crit rolls drawn from the run stream; a new run → Pierce 0. |
| `Upgrades.LoadedDice` | DA_Upgrade_LoadedDice (Rare, MaxStacks 20, "+5%", GE class, in the pool); DA_CombatSettings on the GameMode, CritMultiplier 2; CritChance 0: 100 rolls, no crit, run stream untouched; 10 stacks (console) → 0.5: 2000 rolls ~50% crits (±4%), multiplier 2 on crits and 1 otherwise, the stream advanced, and the same stream state replays the same crits; 20 stacks → 1: every roll a crit, stream untouched, a 21st stack refused; the attribute clamps to 0..1; a new run → 0; a real rifle hit on a frozen bandit 4 m ahead deals 20 with a plain damage number, then with 20 stacks 40 with a critical one (one more critical number; crits are bigger and another colour). |
| `Upgrades.BiggerMagsAndLeech` | DA_Upgrade_BiggerMags (Common, +15%) and DA_Upgrade_Leech (Rare, +1): no cap, their GE classes, in DA_UpgradePool; multiplier 1, HealthOnKill 0, rifle magazine 30, HUD "30 / 30"; two taps → 28, one Bigger Mags stack (console) → ×1.15, magazine 35, still 28 rounds (no refill, not full), HUD "28 / 35"; a reload fills 35; 4 stacks → ×1.6: rifle exactly 48, pistol ceil(19.2) = 20, rifle still 35 rounds, HUD "35 / 48"; the pistol equips with 20 ("20 / 20"); Leech: damaged to 70, a kill without Leech heals nothing, x1 → +1 per kill, x5 → +5, a kill that isn't the player's heals nothing, at 98 a kill stops at MaxHealth 100; a new run resets both (multiplier 1, magazine 30, HealthOnKill 0). Game time ×4. |
| `Upgrades.DashCharge` | DA_DashSettings MaxCharges 1; DA_Upgrade_DashCharge (Rare, no cap, "+1", GE_Upgrade_DashCharge, in DA_UpgradePool); ExtraDashCharges 0, 1 max charge, icon READY; a dash uses it, a second press inside the recharge is refused, icon shows the countdown with no count; a pick (console) during the recharge: ExtraDashCharges 1, max 2 and one charge ready at once, icon "1/2" a frame later, the second dash works; charges come back one recharge (3 s) after the first dash and one more recharge later; `gs.Upgrade.Grant DashCharge 2` → 3 stacks, 4 charges all ready, icon "4/4", four dashes in a row, the 5th refused, icon "0/4"; a new run → ExtraDashCharges 0, 1 max charge, no stacks. Game time ×4. |
| `Upgrades.ExtraJump` | DA_Upgrade_ExtraJump (Rare, no cap, "+1"); no air jump without it; ×3 → ExtraJumps 3, JumpMaxCount 4; three air jumps (rising, falling, falling fast) each set AirJumpZVelocity, 4th refused, one puff/sound each; landing resets; coyote jump is the ground jump; late jump after walking off uses an air jump; slide-hop then air jump; jump during an air dash is queued and becomes an air jump at the dash end; jump during a ground dash becomes the ground jump; 25 more stacks (no cap) → ExtraJumps = stacks, JumpMaxCount = 1 + stacks; new run → 0 / 1. |
| `Upgrades.SecondWind` | DA_Upgrade_SecondWind (Legendary, no cap, "+1", GE class, in the pool); DA_CombatSettings 30% / 2 s; SecondWinds 0, can't save; `gs.Upgrade.Grant SecondWind 2` → 2, Owned 2; with a combo running, a 9999 hit from a bandit: not dead, still in the run, Health = 30 (whole points), invulnerable for exactly 2 s (the effect's duration), SecondWinds 1, Owned 1, used once, damage taken += the old Health, combo ended; HUD: strong flash, one sound, "SECOND WIND / 1 left" banner, Health shown as 30, upgrade icon x1; a second killing blow while invulnerable does nothing; the invulnerability doesn't end early; a hit that doesn't kill uses none; a ready Floating Shield blocks a killing blow first (no Second Wind used); the last Second Wind saves again (Health 30, none left, Owned 0, "THAT WAS YOUR LAST", icon gone, two sounds); picked again → 1; a new run → 0, no uses; with none, a killing blow kills (GameOver). Health orbs off. Game time ×4; the HUD flash shortened on this HUD (it fades in real time). |
| `Upgrades.Offers` | DA_UpgradePool on the GameMode (14 upgrades, 100/20/6/1, 3 choices, 3 rerolls), 4 rarities; roll rules: 3000 offers all 3 different, Rare in ~40.2% (exact for these weights with 2 Epics and 1 Legendary), same seed same offers, 2 upgrades → offer of 2, MaxStacks excluded, empty pool → none; level-up opens an offer after the MaxHealth reward and heal; double level-up queues 2; 3 rerolls (events 2/1/0, Rerolled) then refused; bad index refused; picks grant a stack, Closed + Opened, last pick only Closed; new run closes the offer, refills rerolls, drops picks; reroll with no offer refused; same run seed → same first offer; simulator deterministic, no short offers, Epic and Legendary choices appear, run stream untouched. |

All names start with `BanditBaller.`. Run them with `RunTestsByFilter("StartsWith:BanditBaller")`.

## Tuning values (current)

- **Player**: walk 900 cm/s, accel 6000, jump 800, air jump 800 (~1.87 m each), gravity ×1.75, coyote 0.12 s; camera arm 350 / FOV 90, ADS 180 /
  65. Slide: +600 boost, max 1600, 0.8 s, 1 s cooldown. Dash: 600 cm in 0.15 s, 0.25 s i-frames, 1 charge (+1 per
  Dash Charge pick), 3 s recharge each, one after another.
- **Weapons** (damage / RPM / mag / reload):
  - Rifle: 20 / 600 auto / 30 / 1.8 s, spread hip 2° and ADS 0.5°. Held fire is exactly 600 rounds a minute
    (0.100 s apart, Prompt 31b), at any frame rate.
  - Pistol: 45 / 300 semi / 12 / 1.2 s.
  - BanditCarbine (enemy): 7 / 360 / 4-shot bursts, 3.5° spread, 60 m.
- **Enemies** (`Enemies/DA_Enemy_*`):

  | Enemy | HP | Speed | Combat | SpawnCost | MinRound | Cash |
  |---|---|---|---|---|---|---|
  | MeleeBandit | 60 | 520 (±15%) | 12 dmg melee, 0.55 s wind-up, range 170 | 1 | 1 | $10 |
  | RangedBandit | 45 | 430 (±12%) | range 15–25 m, backs off < 8 m, 0.4 s aim, 4-shot bursts | 2 | 2 | $15 |

  Corpses stay 5 s.
- **Rounds** (DA_RoundSettings): PreRound 3 s, Combat safety fallback 180 s, RoundClear 4 s, then straight into the
  next PreRound. GameOver (death) is untimed.
- **Run end:** run summary input lock 1 s, fade-in 0.4 s and fade-out 0.5 s (WBP_RunSummary class defaults); camera
  fade before the hub opens 0.5 s (`ReturnToHubFadeTime`, BP_GSGameMode).
- **Spawn ring** (DA_SpawnSettings): 12–25 m, 2 m spacing, 20 attempts, prefer off-screen. The user chose to keep
  12 m.
- **Town map** (L_Town_Graybox, Prompt 26b): 300 × 300 m, boundary 30000 × 30000 cm with a 75 m ceiling, NavMesh
  bounds Z −5 to 35 m, EnemySpawn fallback points 142 m out on the axes, landmarks 125 m out.
- **Wave table** (DT_Waves; interval s / batch):

  | Round | Enemies | Interval / batch |
  |---|---|---|
  | 1 | 4 melee | 2.0 / 2 |
  | 2 | 5 melee + 1 ranged | 2.0 / 2 |
  | 3 | 6 + 2 | 1.8 / 2 |
  | 4 | 7 + 3 | 1.8 / 3 |
  | 5 | 9 + 4 | 1.6 / 3 |
- **Generated waves** (DA_WaveSettings): Budget = 5 + 2.5 × round (BudgetCurve empty), e.g. round 8 = 25,
  round 15 = 42.5. MaxGeneratedEnemies 150, batch 3 every 1.5 s, MaxAliveEnemies 30, telegraph 0.75 s
  (M_EnemyLaser). Pool: MeleeBandit, RangedBandit.
- **Difficulty curve** (DA_DifficultyCurve, linear, extrapolates past 30):

  | Round | 1 | 5 | 10 | 20 | 30 |
  |---|---|---|---|---|---|
  | Health ×| 1.0 | 1.4 | 2.0 | 3.5 | 5.0 |
  | Damage × | 1.0 | 1.15 | 1.35 | 1.8 | 2.3 |

  Measured time-to-kill: round 10 melee (120 HP) takes 6 rifle shots (0.50 s) or 3 pistol shots (0.40 s).
- **Cash** (DA_CashSettings):
  - Split: TargetPieceValue 4, 2–6 pieces, bills at $4+. A melee bandit drops $4 + $3 + $3; a ranged bandit drops
    $4 + $4 + $4 + $3.
  - Scatter 40–140 cm, toss 0.35 s / 80 cm high, rest 25 cm above the floor.
  - Lifetime 20 s, blink for the last 4 s.
  - Magnet 400 → 3000 cm/s (accel 3000), collected within 80 cm. Pool prewarm 48 (shared with XP orbs).
  - XP orbs: one per kill worth XPValue, same toss / magnet / vacuum, no lifetime (XPOrbLifetime 0), sphere ×0.32
    with M_XPOrb, pulse ±15% at 1.5 Hz.
  - Health orbs: 20% chance per player kill (run stream), heal 15 (never over MaxHealth), same toss / magnet /
    vacuum, ignored at full health (except by the vacuum), no lifetime, sphere ×0.42 with M_HealthOrb, pulse ±12%
    at 1 Hz. On average one in five kills drops one: a round 5 wave (13 enemies) drops about 2.6 orbs, ~39 health.
- **XP** (DA_XPCurve; XP needed from the previous level, +25% per level, max level 50):

  | Level | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  |---|---|---|---|---|---|---|---|---|---|
  | XP | 100 | 125 | 156 | 195 | 244 | 305 | 381 | 477 | 596 |

  - Reaching level 10 takes 2579 XP in total. Level 20 needs 5551 from level 19, and level 50 needs 4.48 M.
  - XP per enemy (orb value): MeleeBandit 10, RangedBandit 15. DT_Waves rounds 1–5 drop 40 / 65 / 90 / 115 / 150 XP.
- **Kill combo** (DA_ComboSettings): 2.5 s window, restarted by every kill. Cash x1 from 1 kill, x1.25 from 3,
  x1.5 from 6 and x2 from 10. Damage ends it; XP isn't multiplied.
  - Melee bandit drops: $10 / $13 / $15 / $20.
  - Ranged bandit drops: $15 / $19 / $23 / $30.
- **Round-clear bonus** (DA_RoundClearSettings, the same every round): base $20, + $15 for no damage taken, + $15
  for a fast clear, so up to $50 a round.
  - The fast-clear limit is 8 s + 1.5 s per wave enemy: round 1 (4) 14 s, round 5 (13) 27.5 s, round 15 (~29)
    ~52 s.
  - Round 1 pays $40 of drops (more with the combo), so a perfect clear more than doubles it there. The bonus
    matters less as waves grow.
- **Upgrades** (per stack, additive on the base value): Reload Speed +10%, Damage +10%, Move Speed +8%, Fire Rate
  +8%, Magnet +15% pickup radius, Bigger Mags +15% magazine size (rounded up) (Common); Extra Jump +1 air jump, Dash
  Charge +1 dash charge, Leech +1 health per kill, Loaded Dice +5% crit chance, Floating Shield +1 shield (Rare);
  Piercing Rounds +1 enemy
  pierced, Powder Keg +3% chance that a kill explodes (Epic); Second Wind +1 (Legendary, used up when it saves the
  player). Loaded Dice (20 stacks = 100%) and Powder Keg (34 stacks = 100%) are capped.
- **Second Wind** (DA_CombatSettings): back at `SecondWindHealthFraction` 30% of MaxHealth (rounded to whole points),
  `SecondWindInvulnerableTime` 2 s. HUD (WBP_HUD, HUD|Second Wind): full-screen flash (1, 0.85, 0.4) from 0.85 alpha
  over 0.7 s, `SecondWindSound` = `/Engine/VREditor/Sounds/UI/Teleport_Committed` (editor content), banner color
  (1, 0.7, 0.15) for the round banner's 1.8 s.
- **Critical hits** (DA_CombatSettings): CritMultiplier 2.0. Crit damage numbers: size 28 (normal 20), gold, "N!".
- **Powder Keg** (DA_CombatSettings): `ChancePerStack` 3% that a player kill explodes (10 stacks 30%, 34 stacks 100%),
  rolled per kill on the run stream. The blast is always 30 damage × the player's Damage in 3 m, at any stack count;
  chain kills roll again and explode 0.1 s later; no line of sight; never hurts the player; no crits.
- **Floating Shield** (DA_CombatSettings): `ShieldCooldown` 10 s per shield, each on its own world timer (pauses with
  the game). Visuals on BP_GSCharacter's `FloatingShields` component: engine cylinder at (0.45, 0.45, 0.06) with
  M_FloatingShield, orbit 85 cm, 40 cm above the capsule center (chest height), 120°/s; block flash ×1.8 over 0.15 s,
  return pop 0.35 s.
- **Upgrade offers** (DA_UpgradePool, 14 upgrades; placeholders): rarity weights Common 100 / Rare 20 / Epic 6 /
  Legendary 1, 3 choices, 3 rerolls per run.
  - `simulate_upgrade_offers(10000, 1)`, fresh run with no picks (Prompt 37): Move Speed in 39.9% of offers, Magnet
    39.5%, Reload Speed 39.3%, Damage 39.2%, Bigger Mags 38.9%, Fire Rate 38.9%, Dash Charge 9.7%, Extra Jump 9.6%,
    Leech 9.6%, Floating Shield 9.5%, Loaded Dice 9.5%, Powder Keg 7.3%, Piercing Rounds 6.8%, Second Wind 2.3%.
  - The Rare tier is in 40.6% of offers (40.2% expected), shared by five upgrades. The Epic tier (Piercing Rounds,
    Powder Keg) is in 13.4%. The Legendary tier (Second Wind) is in 2.3%: about one offer in 44.
  - By choices: Common 78.6%, Rare 16.0%, Epic 4.7%, Legendary 0.8%. No short offers.
- **Combo curves**: there is no combo system yet.

## Known issues, TODOs and deviations

**Not built yet / stubs**
- Floating Shield (Prompt 36):
  - The shields are flat placeholder discs, and the block and return sounds are editor sounds that may not cook.
  - There is no HUD readout of ready shields or their recharge (only the discs, and `gs.Upgrade.List`).
  - The shields keep recharging after death (harmless: GameOver).
  - A shield always takes a whole hit, however small; there is no partial block.
  - The shield states aren't replicated (`TODO(co-op)`).
- Health orbs (Prompt 35b):
  - The sound is an editor sound that may not cook (`VR_confirm_Cue`), and the look is a plain emissive sphere.
  - The orbs aren't in the run summary (only `gs.Run.Stats`).
  - The orb's kind isn't replicated, like XP orbs (`TODO(co-op)`).
  - The 20% chance and the heal of 15 are first-pass numbers, not balanced against real fights.
- Second Wind (Prompt 37):
  - The sound is an editor sound that may not cook (`Teleport_Committed`); the flash is a plain full-screen color.
  - The banner reuses the round banner's widgets and spot. It shows at once and pushes a round banner back into the
    queue, but it can overlap a "LEVEL UP!" banner that is showing at the same moment.
  - The run summary's upgrade icons show only the Second Winds still left (a used one is removed from the stacks).
  - Pre-existing, seen here: the `[round] … died` line doesn't include the killing blow's damage, because death fires
    from the Health change before `HandleDamageApplied` records it.
  - Not replicated (`TODO(co-op)`).
- Every rarity tier now has upgrades. Epic has Piercing Rounds and Powder Keg so far (Floating Gun comes in Prompts
  38–39).
- Powder Keg (Prompt 35):
  - The explosion and sound are placeholders (the Niagara template `SimpleExplosion`; an editor sound that may not
    cook). The first blast of a session costs a ~140 ms frame while they load; warm chains peak at ~29 ms.
  - Effect and sound play only on the server (`TODO(co-op)`).
  - A blast that kills many enemies drops their cash and XP orbs together. In the PIE chain the orbs leveled the
    player up twice at once (the upgrade screen paused the game).
  - Since Prompt 35c the 3% in the card text is typed by hand. Retuning ChancePerStack needs the card text and
    MaxStacks (34 = 100% at 3%) changed too; `Upgrades.PowderKeg` fails until they match.
- Piercing Rounds (Prompt 34): there is no tracer effect, so the shot's line through the targets is only visible with
  `gs.Debug.Weapons 1`. Target dummies count as targets (they have an ASC), so shots pierce them too.
- Bigger Mags (Prompt 32): after a run restart in the same map (`gs.Run.Seed`, debug) the magazine can hold more
  rounds than its new, smaller size until it is fired down; it counts as full meanwhile. A real run loads a fresh town.
- Dash Charge (Prompt 30): charges spent before a run restart in the same map (`gs.Run.Seed`, debug) keep
  recharging after the Dash Charge stacks are removed. Until enough of them come back the player can have 0 of 1
  charges for a few seconds. A real run loads a fresh town, so this only happens with the debug restart.
- **Not yet built for the decisions before M3:**
  - Owned items in `OwnedItems`, item types (usable items, gear) and the gear loadout (M3/M4).
- Prestige and the two-layer profile (playthrough data + prestige data) are a later milestone; the save layout
  changes in M3.
- Upgrade icons are a placeholder engine texture (shown on the cards).
- Choice screen: no sound, no card hover highlight and no UMG animations (the fade-in is code). Only the
  single-player / host path exists (`TODO(co-op)`).
- `Balance.TimeToKill` failed once in a full run (round 1 melee vs rifle: 4 shots instead of 3, the first case
  after PIE starts) and passed when re-run alone and in the next full run. Probably a missed first shot; watch it.
- Start Run portal (placeholder): a flat glowing ring made of two engine cylinders (no torus in the engine content)
  and a world-space "START RUN" text. There is no sound, no fade-in when the town opens, and no prompt on the HUD.
- `EGSArea::MarketDistrict` now only means "in the hub map". The in-town district and its travel are gone.
- Town map (Prompt 26b):
  - The outskirts, streets and landmarks are plain graybox shapes (no props or detail). The cover density was
    matched by eye, not measured.
  - Roofs, crate tops and landmark tops have small NavMesh patches that walking can't reach (only jumping does). This
    is expected; nothing paths or spawns there.
  - New cover needs `bRasterizeAsFilledConvexVolume`, and the NavMesh bounds must reach above its top. Otherwise
    Recast builds an island inside it. `Map.TownLayout` finds those on its 5 m grid.
  - The edges and corners were visited in PIE by teleport plus a push into the wall, not by a full walk from the
    center. Walking reachability comes from the NavMesh path checks.
- No shop or casino logic yet (M3/M4); CashSpent is never used.
- Crits come only from the player's weapons (Loaded Dice, Prompt 33). Enemy shots never crit. There is no crit
  sound or VFX, only the damage number.
- Run end (Prompt 27):
  - The run summary is placeholder UMG: no sound, no count-up and no animation beyond the code fades. The upgrade
    icons are the small HUD icons (placeholder texture).
  - Enemies left alive at death keep running their AI around the body. Nothing despawns them; the summary covers the
    scene.
  - `gs.Round.Set` and `gs.Run.Seed` still work in GameOver (debug). They restart rounds with a dead player, whose
    run summary stays up.
  - GameOver logs the wave's "stopped early" line as a warning, like any early stop.
- M2 checkpoint (Prompt 28):
  - Balance is unexamined with real shooting. The PIE loops cleared waves with god mode and `gs.KillAll`, so their
    round times are the spawn time plus command delay, and the no-damage bonus was always paid.
  - Generated waves start smaller than the last DT_Waves row (round 6: 12 enemies vs 13 in round 5) and are mostly
    ranged (8 of 12, 9 of 13).
  - A player who stands still dies in 7–10 s of combat in rounds 1–7.
- Profile: there are no upgrade steps yet (version 1 is the only layout). A crash between save points loses the cash
  earned since the last RoundClear.
- Cash drops and enemy XP don't scale with the round. This is by design (flat economy, user decision). The kill
  combo multiplies the cash of a kill; it depends on kill speed, not the round.
- Kill combo: no sound or VFX yet (only the HUD panel and its punches). The combo steps are first-pass numbers,
  not balanced against real fights.
- Round-clear bonus: every Combat → RoundClear pays it. That includes `gs.Round.Skip` (debug; a skip right after
  Combat starts earns the fast-clear and no-damage bonuses) and the 180 s Combat safety timer (enemies left alive,
  never fast). The bonus amounts and the time limit are first-pass numbers. The summary has no sound or count-up
  animation.
- Round HUD:
  - With `gs.Round.Pause 1` in PreRound the countdown still counts down to "in 0…", because it follows the phase end
    time and the debug pause only freezes the phase timer.
  - The round banners have no sound.
  - The upgrade icons use the placeholder engine texture (hence the name under each icon).
  - "ROUND N" is white and reads poorly against the light graybox walls while it fades.
- The only level-up reward so far is +10 MaxHealth and a full heal. Player death ends the run; the next run loads a
  fresh town, which resets levels and their MaxHealth stacks.
- Level-up feedback is placeholder: an editor chime for the sound and a plain overlay glow, with no VFX or
  animation assets.

**Co-op**: deferred to M6. Known gaps are listed in **Later — co-op (M6)**.

**Not visually verified** (covered by tests or logs only)
- Spawn telegraph disc, "+$N" popup, pickup blink.
- Wave PIE runs used `gs.KillAll`, not real shooting.

**Done differently from the original prompts**
- M1 enemies (5 melee, 3 ranged) were placed in the level. Prompt 4 removed them; the 3 target dummies remain.
- `gs.Spawn` originally took a radius argument. Since Prompt 3 it uses the spawn finder (no radius).
- Enemy base stats are written with `SetNumericAttributeBase` from UGSEnemyData (not an init GE); only round
  scaling is a GE.
- Player MoveSpeed comes from DA_PlayerMovementSettings, not GE_InitPlayerStats.
- Cash pickup details:
  - "+$N" popups collected within 0.35 s merge.
  - A target farther than 100 m (teleported away) is paid instantly.
  - Dead players collect nothing.

**Balance**: left as-is after M1 (user choice). Standing still, the M1 test group (5 melee + 3 ranged) killed the
player in 9.4 s; the 3 ranged bandits alone took 19.3 s.

**Placeholder art**:
- Mannequins with tinted materials.
- `MI_CashCoin` / `MI_CashBill` are instances of `M_Enemy_MeleeBandit` (changing that material changes the cash).
- The telegraph uses `M_EnemyLaser`.
- Level-up: `M_LevelUpGlow` overlay and the `CompileSuccess` editor sound.
- XP orb: `M_XPOrb` on the engine sphere.
- Health orb: `M_HealthOrb` (green copy of M_XPOrb) on the engine sphere, and the `VR_confirm_Cue` editor sound.
- Air jump: the Niagara `RadialBurst` template and the `VR_teleport_Cue` editor sound (may not cook into a packaged
  build).

**Warnings**:
- `C4701` potentially uninitialized `BestLocation` in `AI/GSSpawnLocationFinder.cpp:262`.
- Every PIE test logs `LogCrowdFollowing: Unable to find RecastNavMesh instance…` (harmless so far).

**Leftover template code and content**: `Content/ThirdPerson`, `Variant_*` (reference only), and
`BanditBallerCharacter` / `GameMode` / `PlayerController`. `AGSPlayerController` derives from
`ABanditBallerPlayerController`.

**Environment gotchas**:
- There is no Python on this machine (`python` isn't found). Use PowerShell, the Edit tool, or MCP's
  `ProgrammaticToolset`.
- Bash heredocs with quotes fail. Write to the scratchpad and append instead.
- After `ObjectTools.set_properties` on a Blueprint CDO, run `BlueprintTools.compile_blueprint` before
  saving/PIE, or spawned instances keep the old value.
- With the editor in the background PIE runs at about 3 FPS. Set
  `EditorPerformanceSettings.bThrottleCPUWhenNotForeground` false in memory for timing work.
- Round 1 Combat starts 3 s into PIE. Send `gs.God 1` / `gs.Round.Pause 1` right after starting PIE, or the wave
  can kill the player first. Starting PIE and typing the commands in the same `ProgrammaticToolset` script avoids
  that race. Slow Slate lookups in between can let the wave kill the player.
- `ObjectTools.set_properties` refuses an array whose size and elements both change ("insertion points are
  ambiguous"). To append a GE modifier, read the array, append a deep copy and set it back in one script.
- The MCP toolset registry needs typed containers in tool signatures (`dict[str, float]`, `list[...]`). A bare `dict`
  fails registration of the whole `BanditBallerTools` toolset (`LogPython: Error: … missing specification for
  contained type`), so nested results are returned as JSON text. Restart the editor after Python edits.
- Screenshots: `CaptureEditorImage` → `AssetTools.write_file` into `Saved/` as `.txt` (only .csv / .html / .json /
  .md / .py / .txt are allowed), then base64-decode with PowerShell. `SlateInspectorToolset.PressKey` (key names
  like `Two`, `R`, `Gamepad_DPad_Right`) sends real key presses to the focused widget, e.g. the choice screen.
- Holding an input in PIE from MCP: the engine console commands `Input.+key <Key>` / `Input.-key <Key>` hold and
  release a key (`LeftMouseButton` holds fire). Reads inside one `ProgrammaticToolset` script all come from the same
  frame (no world time passes between calls in a script), so read changing values across separate tool calls.
- Level-ups pause the game (upgrade offers). Tests or PIE sessions that need world time after a level-up must pick
  the offers (`gs.Upgrade.Choose 0`, `PickAllUpgradeOffers` in tests).
- NavMesh from MCP: `RebuildNavigation` in the editor console rebuilds it. `ShowFlag.Navigation` doesn't show up in
  `CaptureViewport`, so check islands with `Map.TownLayout`. A `NavModifierComponent` uses each collision element's
  bounding box (a cylinder becomes a square), so it is no good for clearing the inside of round meshes.
- `CaptureEditorImage` returns about 1 MB of base64. Write it to a file inside the script
  (`AssetTools.write_file`, `file_path` absolute under `Saved/`) rather than returning it.

---

## Next up

Each step is its own prompt. It builds, is tested and is committed, and it is built and tested for single-player
only. CLAUDE.md's rule "Only do what the current prompt asks" applies: the steps below are suggestions until a
prompt asks for one. (Earlier attempts at run loop cleanup and a per-player profile were reverted in 1a84a63.
Permanent cash was then built as a single local balance in a2dc3b7.)

14. ~~**Run loop cleanup.**~~ Done (see Prompt 14).
15. ~~**Upgrade data + stat upgrades.**~~ Done (see Prompt 15). The upgrade pool with rarity weights moved to 18.
16. ~~**Extra Jump.**~~ Done (see Prompt 16). It stays uncapped (user decision). The map boundary's ceiling
    (Prompt 21) limits the height.
17. ~~**Design update — decisions before M3.**~~ Done (docs only).
18. ~~**Upgrade offers (4 rarity tiers + rerolls).**~~ Done (see Prompt 18; log / console only). The weights are
    placeholders, and which upgrades become Epic or Legendary is decided later.
19. ~~**Choice screen (with reroll).**~~ Done (see Prompt 19).
20. ~~**XP pickups.**~~ Done (see Prompt 20).
21. ~~**Map boundary.**~~ Done for the town (see Prompt 21). The hub map gets its own `AGSMapBoundary` in Prompt 25.
22. ~~**Kill combo.**~~ Done (see Prompt 22).
23. ~~**Round clear bonus.**~~ Done (see Prompt 23): flat base + no-damage + fast-clear bonuses and the RoundClear
    summary.
24. ~~**Round HUD.**~~ Done (see Prompt 24).
25. ~~**Hub map + hub GameMode.**~~ Done (see Prompt 25). Original plan:
    - **Hub map** `Maps/L_Hub_Graybox`: the district content moves out of the sublevel, gets its own lighting, sky,
      boundary (walls + ceiling) and a PlayerStart at `MarketArrival`. It becomes `GameDefaultMap` /
      `EditorStartupMap`.
    - **GameModes:** a hub GameMode (no rounds; shop, casino and portal later) with its own Blueprint. The current
      `AGSGameMode` keeps the round loop as the town map's GameMode.
    - **Tests** open `L_Town_Graybox` explicitly, since the startup map changes (done: `BeginPIE`).
26. ~~**Start Run portal + town cleanup.**~~ Done (see Prompt 26).
26b. ~~**Bigger town.**~~ Done (see Prompt 26b): 300 × 300 m with streets, outskirts cover, a landmark on each side,
    the boundary and spawn points moved out, and a NavMesh without islands.
27. ~~**Death → summary → hub.**~~ Done (see Prompt 27): GameOver on death (wave stopped, offers dropped), the run
    summary, and "Return to Market" (button, R, Start / Y) with a fade back to the hub.
28. ~~**Tests + M2 checkpoint.**~~ Done (see Prompt 28 and **M2 checkpoint**). **M2 is complete.**

**Next: M2b — more level-up upgrades** (after M2, before M3; see **Design update — M2b upgrades** for the list, the
rules and the open questions). Each prompt adds its upgrades to DA_UpgradePool and updates the tests that assume the
smaller pool.

29. ~~**M2b docs.**~~ Done (see **Design update — M2b upgrades**; docs only).
30. ~~**Dash Charge** (Rare): +1 dash charge per pick; the player starts with 1 charge instead of 2.~~ Done (see
    Prompt 30).
31. ~~**Fire Rate + Magnet** (Common): +8% FireRate and +15% PickupRadius per pick, on the existing attributes.~~
    Done (see Prompt 31).
31b. ~~**Fix: exact fire rate for held fire.**~~ Done (see Prompt 31b): a held burst stays inside GA_Fire on an exact
    shot schedule; the rifle fires exactly 600 rounds a minute.
32. ~~**Bigger Mags + Leech** (Common, Rare): +15% magazine size per pick (rounding decided here); heal 1 health per
    kill per pick.~~ Done (see Prompt 32): rounded up, no refill on a pick.
33. ~~**Loaded Dice** (Rare, MaxStacks 20): +5% chance per pick for a shot to deal double damage, rolled on the run
    stream.~~ Done (see Prompt 33): one roll per hit (`GSCombat::RollCriticalHit`), CritMultiplier 2 in
    DA_CombatSettings.
34. ~~**Piercing Rounds** (Epic): shots pass through 1 more enemy per pick.~~ Done (see Prompt 34).
35. ~~**Powder Keg** (Epic): enemies the player kills explode and damage enemies nearby; the blast grows per pick.
    Blast kills count as the player's.~~ Done (see Prompt 35): chains 0.1 s apart; a 25-blast chain peaks at ~29 ms
    warm.
35b. ~~**Health orbs** (not an upgrade): enemies the player kills have a chance (20%, run stream) to drop a health orb
    that heals 15 when collected (never over MaxHealth). A third pickup kind on the cash / XP pickup system; full-health
    players don't attract it; the RoundClear vacuum collects it; it doesn't expire.~~ Done (see Prompt 35b).
35c. ~~**Powder Keg: chance to explode.** Each stack adds a 3% chance (ChancePerStack in DA_CombatSettings, rolled on
    the run stream per player kill) that the enemy explodes; the blast is a fixed 30 damage (× Damage) in 3 m; MaxStacks
    34 (100%); chains roll the same chance after 0.1 s.~~ Done (see Prompt 35c).
36. ~~**Floating Shield** (Rare): +1 shield per pick; a shield takes a hit and comes back after 10 s. The damage order
    starts here (dash invulnerability → shield → Health).~~ Done (see Prompt 36).
37. ~~**Second Wind** (Legendary): survive one killing blow per pick at 30% health; the last step of the damage order.~~
    Done (see Prompt 37).
38. **Floating Gun, part 1** (Epic): the projectile and the first gun (right shoulder). Its kills count as the
    player's.
39. **Floating Gun, part 2:** the second gun (left shoulder) and the +10% damage / fire rate picks from the 3rd on.
40. **M2b tests + checkpoint:** tests for the M2b rules and a PIE checkpoint, like Prompt 28.

**Then M3** (after M2b; not planned in detail yet; each item needs its own prompt). From the decisions so far:
- The two-layer save (playthrough data: cash, owned items, loadout, total cash earned; prestige data, empty until
  prestige), with an upgrade step from save version 1.
- Item types: usable items (consumables, bought again to restock, used up in a run) and gear (bought once, one per
  loadout slot). Other types (abilities, powerups) are decided in M3.
- The static shop in the hub (a fixed catalogue, `TrySpendCash`), owned items in the profile, the hub loadout, and
  owned things applied at the start of the next run.

Suggestions from the M2 checkpoint (not scheduled): look at the DT_Waves → budget hand-off (round 6 is smaller than
round 5 and mostly ranged), and replace `gs.KillAll` play with real shooting before balance work.

Decided (user):
- **Single-player first;** co-op is M6.
- **Upgrade choice:** the game pauses while an upgrade is being chosen.
- **Hub map:** a separate hub map with its own GameMode; `OpenLevel` between hub and town.
- **Extra Jump:** no cap (MaxStacks 0); it can be picked any number of times. The map boundary's ceiling
  (Prompt 21) limits the height.
- **Owned items:** shop purchases and casino prizes are stored in the profile (`OwnedItems`) and are active or
  usable from the start of the next run, by item type. Built in M3/M4.
  - Usable items (consumables) can be bought again to restock; using one in a run uses it up.
  - Gear is bought once and applies through the hub loadout (one item per slot, swapped in the hub) from the next
    run.
  - Other types (abilities, powerups) are decided in M3.
- **Static shop:** a fixed catalogue, never random or rerolled. Only the casino uses the hub random stream.
- **Prestige** (later milestone): an optional reset that wipes cash and all bought or won items for bonuses based on
  the playthrough's total cash earned. The profile gets two layers, playthrough data and prestige data, when the
  save layout changes in M3.
- **Flat economy:** cash and XP per enemy are the same every round, and so is the round clear bonus. There is no
  round scaling for drops.
- **Epic / Legendary upgrades:** Floating Gun, Piercing Rounds and Powder Keg are Epic and Second Wind is Legendary
  (M2b). The rarity weights are still placeholders.
- **M2b upgrades** (after M2, before M3; the pool grows from 4 to 15). Every number is a starting value in data; no
  cap unless noted.
  - Common: Fire Rate (+8% FireRate), Magnet (+15% PickupRadius, cash and XP orbs), Bigger Mags (+15% magazine
    size).
  - Rare: Dash Charge (+1 charge; the player starts with 1 instead of 2), Floating Shield (+1 shield; a hit uses one
    up, and it comes back after 10 s), Loaded Dice (+5% double-damage chance per shot, MaxStacks 20 = 100%), Leech
    (heal 1 per kill).
  - Epic: Floating Gun (a gun over the right shoulder, then one over the left, then +10% gun damage and fire rate
    per pick), Piercing Rounds (+1 enemy pierced), Powder Keg (+3% chance per pick that a kill explodes and damages
    nearby enemies; a fixed blast; MaxStacks 34 = 100%; changed in 35c).
  - Legendary: Second Wind (survive one killing blow per pick at 30% health).
  - Rules: the Extra Jump pattern (attribute + infinite stacking GE, DA_Upgrade_* asset, DA_UpgradePool,
    `gs.Upgrade.Grant` short name, `gs.Upgrade.List` line). Damage order: dash invulnerability → Floating Shield →
    Health → Second Wind on a killing blow. Floating Gun and Powder Keg kills are the player's (cash, XP orb,
    combo, Leech). New rolls use the run stream; all damage goes through GE_Damage. Single-player only,
    co-op-ready.
- **Second Wind** (Prompt 37): a killing blow uses one instead: back at 30% of MaxHealth, invulnerable for 2 s, and
  one stack of the upgrade is removed (the card's "Owned" and `gs.Upgrade.List` show what's left; picking it again adds
  one). Order: shields first, then Health, then Second Wind only if the hit would kill. Feedback: a strong screen
  flash, a sound and a "SECOND WIND" banner. The saving hit counts as damage taken (the Health it removed): it ends
  the kill combo and costs that round's no-damage bonus.
- **Floating Shield** (Prompt 36): a hit a shield blocks is not damage taken (the kill combo goes on, the no-damage
  round bonus is kept, run-stats damage taken doesn't change). The shield recharge pauses with the game.
- **Upgrade rarities and rerolls:** four tiers (Common, Rare, Epic, Legendary). The 3 choices can be rerolled a
  limited number of times per run (3 to start). There is no skip.
- **Casino bets** risk cash only, never owned items.
- **XP pickups:** XP comes from pickups enemies drop, not instantly on the kill.
- **Map boundary:** every map has invisible walls around the play area and a ceiling, blocking pawns only.

Still open from M2: more DA_LevelRewards entries.

## Later — co-op (M6)

Not built now; listed so single-player work stays co-op-ready. Co-op has never been tested with 2+ players.

**Planned**
- **Travel:** seamless / server travel between hub and town; the whole party travels together. Check first that
  seamless travel keeps clients connected in PIE.
- **Co-op profiles (decided):** the host's save holds every player's cash, keyed by a stable player ID.
  - Open question: a stable player ID without an online subsystem. Candidate: a GUID each client generates once,
    stores locally and sends when it joins.
  - Remote players' pickups would then be credited.
- **Upgrade picks:** clients pick through a Server RPC; offers replicate to the owning client.
- **Party pause:** the whole party pauses while any player has an open offer, and the others see who is choosing.
  Queued offers keep it paused until the last pick.
- **Death:** "all players dead" ends the run, with revive, instead of one player's death.

**Known gaps in today's code**
- Hub ↔ town travel is single-player `OpenLevel` (`AGSHubGameMode::StartRun`, `AGSGameMode::OpenHubMap`;
  `TODO(co-op)`). M6 needs ServerTravel so the whole party goes.
- The profile is one balance per machine. On a listen server only the host's pickups are credited; remote players'
  pickups are not (`TODO(co-op)` in `AwardCash`). Clients' HUDs show their own machine's profile.
- Cash goes to whoever collects it, not the killer.
- XP goes to whoever collects the orb (not shared with the party), like cash.
- The XP orb's kind isn't replicated, so clients would show it with the cash look.
- The "+N XP" collection point reaches only the local HUD (`NotifyXPCollected` is not an RPC); a remote client's
  popup falls back to above its pawn.
- The kill combo (`UGSComboComponent`: count, multiplier, window) and run stats `BestCombo` aren't replicated, so a
  remote client's HUD shows no combo.
- The round stats (`FGSRoundStats`) and the round-clear payout aren't replicated, so a remote client's RoundClear
  summary stays hidden. The bonus is one party-wide payout into this machine's profile, and "no damage" is
  party-wide.
- `AGSGameState::OnWaveCountsChanged` fires on the server only (the wave counts have no RepNotify), so a remote
  client's "enemies left" doesn't update. Its upgrade icons need the upgrade stacks replicated.
- One player's death ends the run (GameOver) for everyone (`TODO(co-op)` in `HandlePlayerDeath`). The run summary
  reads the owning player's upgrades and the best combo, which aren't replicated, and this machine's profile.
  `RequestReturnToHub` only checks that the requester is dead.
