# BanditBaller — Progress

Read this at the start of every session. After finishing a prompt, add a short entry at the end of the current
milestone section (what was built, new assets/commands, known issues) and update **Next up**.

## The game

Third-person co-op roguelike shooter set in an off-grid bandit gambling town (UE 5.8, C++ + GAS; single player and
2–4 player listen-server co-op). Loop: fight a round of bandits in the town → pick up the cash they drop → on some
rounds the party teleports to the safe **Market District** (shop + casino) → back to town for the next, harder
round.

**Current milestone: M2 — the run loop** (rounds, waves, scaling, economy, progression). Prompts 1–9 are done.

State at the time of writing: `main` @ `371cb6e`, not pushed. The build is clean and all 17 automation tests pass.

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

Paths below: C++ under `Source/BanditBaller/`, assets under `Content/BanditBaller/` (`/Game/BanditBaller/`).

---

## M0 — Project setup (done)

- **Tooling**: git + LFS (`.gitattributes`), `.gitignore`, `.vsconfig`, `.mcp.json` (unreal-mcp), `CLAUDE.md`,
  content folder skeleton. `Config/DefaultEngine.ini`: `CommonGameViewportClient`, `GlobalDefaultGameMode` =
  `BP_GSGameMode`, `EditorStartupMap` / `GameDefaultMap` = `L_Town_Graybox`.
- **Framework classes** (`Core/`): `AGSGameMode`, `AGSGameState`, `AGSPlayerState` (Cash, `OnCashChanged`,
  `AddCash` / `TrySpendCash`), `AGSPlayerController`, `AGSCharacter`; `Abilities/GSAttributeSet`.
  Blueprints in `Core/`: `BP_GSGameMode`, `BP_GSGameState`, `BP_GSPlayerState`, `BP_GSPlayerController`, `BP_GSCharacter`.
- **Level** `Maps/L_Town_Graybox`: 150×150 m slab, central plaza, 6 shacks + crates as cover, `EnemySpawn` Target
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
   enemies; reports kills to the GameMode). `UI/WBP_DeathScreen` (`UGSDeathScreenWidget`; R restarts via
   `?Restart`). HUD health bar, damage numbers, hit marker, red edge flash + low-health pulse;
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

## M2 — The run loop (Prompts 1–9 done)

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
  Test `Player.RestartRestoresInput`.

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

---

## Console commands and CVars

Non-Shipping cheats. They run on the server; clients forward them through `ServerExecDebugCommand`.

| Command / CVar | What it does |
|---|---|
| `gs.Spawn <Enemy> [Count=1]` | Spawn enemies around the player via the spawn finder (`MeleeBandit`, `RangedBandit` or a full `DA_Enemy_*` name). |
| `gs.KillAll` | Kill every living enemy through GE_Damage, credited to the player (they drop cash). |
| `gs.God [0\|1]` | Toggle/set damage immunity (one `State.Invulnerable` count). |
| `gs.GiveWeapon <Weapon>` | Equip `Rifle` / `Pistol` / `BanditCarbine` with a full mag (replaces the equipped one if both slots are full). |
| `gs.SetAttr <Attr> <Value>` | Set a player attribute base value (Health, MaxHealth, MoveSpeed, Damage, FireRate, PickupRadius, XPGain). |
| `gs.XP.Add <N>` | Grant N raw XP to the local player (XPGain not applied; may level up several times). |
| `gs.XP.SetLevel <N>` | Reset to level 1, then add exactly the XP for level N (0 XP into it; clamped to max level). |
| `gs.Damage.Self <n>` | Damage the local player through GAS. |
| `gs.Perf [Seconds=5]` | Log avg/p95/max frame time, game/render/GPU time and enemies alive. |
| `gs.Round.Skip` | Advance to the next round phase. |
| `gs.Round.Set <N>` | Jump to round N and restart it from PreRound. |
| `gs.Round.Pause [0\|1]` | Freeze / resume the phase timer. |
| `gs.Run.Seed [N]` | Print the run seed, or restart the run (stream + stats, round 1) with seed N. |
| `gs.Run.Stats` | Print run stats to log and screen (works on clients). |
| `gs.Market.Teleport` / `gs.Market.Return` | Temporary: move the party to the Market District and back. |
| `gs.Editor.MarketDistrictLevel create\|edit\|persistent` | Editor only: create the district sublevel / make it or the persistent level current. |
| `gs.Editor.BuildEnemyStateTree [melee\|ranged\|all]` | Editor only: regenerate ST_MeleeBandit / ST_RangedBandit from code. |
| `gs.Debug.Round 1` (cvar) | On-screen round, phase, time left, area, market open, wave counts, enemy HP/DMG multipliers. |
| `gs.Debug.Spawns 1` (cvar) | Draw the spawn ring and chosen points (green ring, yellow fallback) for 8 s. |
| `gs.Debug.Weapons 1` (cvar) | Debug-draw shots and log enemy shots (`[shot]`). |
| `gs.Debug.DamageLog 1` (cvar) | Log every health loss (`[dmg] victim took N from X (health) at t=`). |
| `gs.Wave.MaxAlive <N>` (cvar) | Override the alive cap (0 = DA_WaveSettings). |
| `gs.Cash.Lifetime <s>` (cvar) | Lifetime of uncollected cash pieces (0 = DA_CashSettings). |
| `GSFire`, `GSPress <InputTag>`, `GSAimAt X Y Z` | Exec on the controller: drive fire, any ability input, or the camera from the console. |

Launch arg: `-GSSeed=N` fixes the run seed.

MCP toolset `banditballer_toolset.toolsets.BanditBallerTools` (PIE running):
- `spawn_enemies(type, count)`
- `get_player_attributes()`: attributes plus Ammo, Cash, Level, XP, XPForNextLevel, TotalXP, God, Invulnerable,
  Dead.
- `list_enemies_alive()`

## GAS reference

**Attributes** (`UGSAttributeSet`, on the player's PlayerState and on each enemy):

| Attribute | Notes |
|---|---|
| Health / MaxHealth | Clamped 0..MaxHealth; 0 → `State.Dead` via `UGSHealthComponent`. |
| MoveSpeed | Drives MaxWalkSpeed. Player value comes from DA_PlayerMovementSettings (900); enemies from UGSEnemyData ± variance. |
| Damage | Outgoing damage multiplier (player 1.0). Enemies: scales melee AttackDamage / weapon Damage; round-scaled. |
| FireRate | Refire multiplier (1.0). |
| PickupRadius | Cash magnet radius in cm (player 300). |
| XPGain | XP multiplier on kill XP (player 1.0; constructor default 1.0). |
| IncomingDamage | Meta attribute: GE_Damage writes it, the set converts it to Health loss and records the instigator. |

**Gameplay tags** (native, `Abilities/GSGameplayTags.*`; none in config):
- State: `State.Dead`, `State.Sliding`, `State.Dashing`, `State.Firing`, `State.Reloading`,
  `State.Invulnerable`, `State.Attacking`
- Ability: `Ability.Fire`, `Ability.Reload`, `Ability.Slide`, `Ability.Dash`, `Ability.SwapWeapon`,
  `Ability.Attack.Melee`, `Ability.Attack.Ranged`
- Cooldown: `Cooldown.Slide`, `Cooldown.Dash`
- SetByCaller data: `Data.Damage`, `Data.Cooldown`, `Data.HealthMultiplier`, `Data.DamageMultiplier`

**Gameplay Effects** (`Content/BanditBaller/Abilities/`):

| GE | Class | What it does |
|---|---|---|
| `GE_InitPlayerStats` | UGameplayEffect | Instant overrides: MaxHealth 100, Health 100, Damage 1, FireRate 1, PickupRadius 300, XPGain 1. Granted through DA_PlayerAbilitySet. |
| `GE_Damage` | `UGSDamageEffect` | SetByCaller `Data.Damage` → IncomingDamage. Blocked by `State.Invulnerable` / `State.Dead`. All damage uses it. |
| `GE_Cooldown_Slide` | `UGSCooldownEffect_Slide` | Duration from `Data.Cooldown`; grants `Cooldown.Slide`. |
| `GE_Cooldown_Dash` | `UGSCooldownEffect_Dash` | One stack per spent charge; grants `Cooldown.Dash`. |
| `GE_EnemyRoundScaling` | `UGSEnemyRoundScalingEffect` | Infinite; MultiplyCompound MaxHealth / Damage from `Data.HealthMultiplier` / `Data.DamageMultiplier`. |
| `GE_LevelUp_MaxHealth` | `UGSLevelUpMaxHealthEffect` | Infinite, stacking (no limit); +10 MaxHealth per stack, one stack per level-up (DA_LevelRewards). |
| `GE_FullHeal` | `UGSFullHealEffect` | Instant; Health = target's current MaxHealth (attribute-based). Applied after level-up rewards. |

**Abilities:**
- Player (`DA_PlayerAbilitySet`): `GA_Fire`, `GA_Reload`, `GA_SwapWeapon`, `GA_Slide`, `GA_Dash`.
- Enemies (`DA_EnemyAbilities_*`): `GA_MeleeAttack`, `GA_EnemyFire`.

## Automation tests (17, all passing)

`Tests/GSGameplayTests.cpp`. Each test starts PIE in L_Town_Graybox, waits for the player, removes enemies, runs
and ends PIE. Stop PIE before running.

| Test | Checks |
|---|---|
| `GAS.DamageReducesHealth` | GE_Damage lowers Health. |
| `GAS.HealthClampsToZeroAndMax` | Health clamps at 0 and MaxHealth. |
| `GAS.DeathAddsStateDead` | Death adds `State.Dead`. |
| `Weapons.FireConsumesAmmoReloadRefills` | GA_Fire uses ammo, GA_Reload refills. |
| `Abilities.DashInvulnerabilityBlocksDamage` | Dash i-frames block damage. |
| `Map.TownAndMarketDistrict` | Town tags/NavMesh, district contents, teleport there and back, no spawning there. |
| `Round.PhaseLoop` | PreRound on start, timer → Combat, skip into round 2, gs.Round.Set. |
| `Player.RestartRestoresInput` | Die → death screen UI input → restart → game input back. |
| `Spawn.LocationFinder` | 12 points: on NavMesh, reachable, in ring, spaced; same seed → same points; none in the district. |
| `Round.WaveDirector` | Round 1 size matches DT_Waves; killing the wave ends Combat; skipping removes leftovers. |
| `Round.WaveBudget` | Seeded purchase within budget, MinRound respected, alive cap with telegraphs, spawns resume. |
| `Round.DifficultyScaling` | Round 1 base stats; round 10 = base × curve, full health; GameState multipliers. |
| `Balance.TimeToKill` | Rifle/pistol shots-to-kill at rounds 1/5/10 vs both bandits via the real fire path (`[ttk]` log). |
| `Run.SeedAndStats` | Same seed replays rolls; restart resets stats; kills per type; damage dealt/taken; rounds cleared. |
| `Economy.CashPickups` | Exact split; out-of-reach drop stays; radius collects into Cash/CashEarned/HUD + popup; pool reuse; RoundClear vacuum; short lifetime blinks, expires unpaid. |
| `Progression.XPAndLevels` | Curve L2 = 100 / L3 = 125, max 50; partial XP; 260 XP → level 3 with 35 carried over (HUD level + bar follow); kill XP = XPValue × XPGain (×1, ×2); run stats XP/level; SetLevel 10 → 0 XP; cap at max level; new run resets. |
| `Progression.LevelUpRewards` | DA_LevelRewards wiring; +10 per stack; damaged → two level-ups → MaxHealth +20, Health full, 2 stacks, HUD health/level/XP follow; same in RoundClear and the Market District; new run removes stacks and clamps Health; dead player gets the stack but no heal. |

All names start with `BanditBaller.`. Run them with `RunTestsByFilter("StartsWith:BanditBaller")`.

## Tuning values (current)

- **Player**: walk 900 cm/s, accel 6000, jump 800, gravity ×1.75, coyote 0.12 s; camera arm 350 / FOV 90, ADS 180 /
  65. Slide: +600 boost, max 1600, 0.8 s, 1 s cooldown. Dash: 600 cm in 0.15 s, 0.25 s i-frames, 2 charges, 3 s
  recharge each.
- **Weapons** (damage / RPM / mag / reload):
  - Rifle: 20 / 600 auto / 30 / 1.8 s, spread hip 2° and ADS 0.5°.
  - Pistol: 45 / 300 semi / 12 / 1.2 s.
  - BanditCarbine (enemy): 7 / 360 / 4-shot bursts, 3.5° spread, 60 m.
- **Enemies** (`Enemies/DA_Enemy_*`):

  | Enemy | HP | Speed | Combat | SpawnCost | MinRound | Cash |
  |---|---|---|---|---|---|---|
  | MeleeBandit | 60 | 520 (±15%) | 12 dmg melee, 0.55 s wind-up, range 170 | 1 | 1 | $10 |
  | RangedBandit | 45 | 430 (±12%) | range 15–25 m, backs off < 8 m, 0.4 s aim, 4-shot bursts | 2 | 2 | $15 |

  Corpses stay 5 s.
- **Rounds** (DA_RoundSettings): PreRound 3 s, Combat safety fallback 180 s, RoundClear 4 s. The market is always
  closed for now.
- **Spawn ring** (DA_SpawnSettings): 12–25 m, 2 m spacing, 20 attempts, prefer off-screen. The user chose to keep
  12 m.
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
  - Magnet 400 → 3000 cm/s (accel 3000), collected within 80 cm. Pool prewarm 48.
- **XP** (DA_XPCurve; XP needed from the previous level, +25% per level, max level 50):

  | Level | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  |---|---|---|---|---|---|---|---|---|---|
  | XP | 100 | 125 | 156 | 195 | 244 | 305 | 381 | 477 | 596 |

  - Reaching level 10 takes 2579 XP in total. Level 20 needs 5551 from level 19, and level 50 needs 4.48 M.
  - Kill XP: MeleeBandit 10, RangedBandit 15. DT_Waves rounds 1–5 give 40 / 65 / 90 / 115 / 150 XP.
- **Combo curves**: there is no combo system yet.

## Known issues, TODOs and deviations

**Not built yet / stubs**
- The market never opens: `RunMarketDecision` hard-codes "closed". `MarketVisit` only runs via `gs.Market.*`.
  There is no shop or casino logic yet, and CashSpent is never used.
- Crits are stubbed (`TODO(crits)` in `GSAttributeSet.cpp`; damage numbers support them, nothing sets them).
- The `GameOver` phase exists but is unused. Player death reloads the whole map (`?Restart`).
- Cash drops don't scale with the round. Neither does enemy XP.
- The only level-up reward so far is +10 MaxHealth and a full heal. There is no "+N XP" popup or level-up banner
  yet. Player death (`?Restart`) reloads the map, which also resets levels and their MaxHealth stacks.

**Co-op**
- Never tested with 2+ players.
- Known gaps:
  - Market travel doesn't wait for clients to stream the district (`TODO(co-op)` in `GSGameMode.cpp`).
  - Restart on death should become "all players dead" / revive.
  - Cash goes to whoever collects it, not the killer.
  - XP goes only to the killing player (not shared with the party). Kills by non-players give no XP.

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

**Warnings**:
- `C4701` potentially uninitialized `BestLocation` in `AI/GSSpawnLocationFinder.cpp:246`.
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

---

## Next up

**M2 Prompt 10 — not specified yet.** Open M2 items:
- Open the market in `RunMarketDecision` (it is always closed).
- Shop / casino.
- Round scaling for cash and XP drops.
- `GameOver`.
- More entries in DA_LevelRewards.
