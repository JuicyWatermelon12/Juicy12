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
  - The player must take one (no skip) but can reroll the choices a limited number of times per run (3 to start).
  - The game pauses while the player chooses.

The design changed after M2 Prompt 10; see **Design change — permanent cash, hub market, level-up upgrades**. The
scope then changed to single-player first; see **Scope change — single-player first**. Five more decisions came
before M3; see **Design update — decisions before M3**. The market, prestige and economy were settled after that;
see **Design update — market, prestige and economy**.
The in-run market is gone from the round loop (Prompt 14). The streamed district and its debug teleport stay until
the hub map replaces them.

**Current milestone: M2 — the run loop** (rounds, waves, scaling, economy, progression). Prompts 1–26 are done
(11–13 and 17 were design / scope updates and permanent cash).

State at the time of writing: `main` after Prompt 26, not pushed. The build is clean and all 30 automation tests pass.
The game and the editor start in the hub (`L_Hub_Graybox`). Walk into the Start Run portal to play a run in
`L_Town_Graybox`. After death, R on the death screen goes back to the hub. The run summary in between is Prompt 27.

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
| M2 26 | Start Run portal (hub → town, run starts on load, seed hand-off); town cleanup (district sublevel, market travel removed) | see git log |

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

## M2 — The run loop (Prompts 1–26 done)

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
- **Player death:** currently `?Restart` reloads the map. It will become the `GameOver` phase → run summary →
  return to the hub. Run state resets and cash is kept.
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
    - `PlayAreaSize` 15000 × 15000 cm (the slab edges).
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
  - Added `HubMap` (BP_GSGameMode → L_Hub_Graybox) and `OpenHubMap()` (OpenLevel; the death flow in Prompt 27 can
    use it).
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
- The GameOver phase and the run summary are still Prompt 27. This only moves the R press from "restart the town" to
  "go to the hub".
- Tests: `Player.RestartRestoresInput` and `Profile.PermanentCash` now expect the hub after R (AGSHubGameMode, game
  input back, balance kept).
- PIE check:
  - The hub, then `gs.Market.Return` into a run.
  - `gs.Damage.Self 1000` showed the new death screen.
  - A real R key press led to "Run over … returns to the hub" and the hub, at 100 health with the cash unchanged
    ($1,697).

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
| `gs.Upgrade.Grant <Name> [Count=1]` | Add stacks of an upgrade (`ReloadSpeed`, `Damage`, `MoveSpeed`, `ExtraJump` or `DA_Upgrade_*`); stops at MaxStacks. E.g. `gs.Upgrade.Grant ExtraJump 45`. |
| `gs.Upgrade.List` | List every upgrade with rarity, this run's stacks / cap and description, plus ReloadSpeed / Damage / MoveSpeed. |
| `gs.Upgrade.Offer` | Queue a level-up offer without the level (opens at once if none is open) and print the open offer. |
| `gs.Upgrade.Choose <0-2>` | Take a choice of the open offer; the next queued offer opens. |
| `gs.Upgrade.Reroll` | Reroll the open offer for one of this run's rerolls (refused with none left or no offer). |
| `gs.XP.Add <N>` | Grant N raw XP to the local player (XPGain not applied; may level up several times). |
| `gs.XP.SetLevel <N>` | Reset to level 1, then add exactly the XP for level N (0 XP into it; clamped to max level). |
| `gs.Profile.AddCash <N>` | Add N to the permanent cash balance (local profile; saved at the next save point). |
| `gs.Profile.Reset` | Wipe the profile: delete the save file, cash 0. |
| `gs.Profile.Save` | Save the profile now. |
| `gs.Profile.Print` | Print slot, save version, cash and owned items to the log and screen. |
| `gs.Damage.Self <n>` | Damage the local player through GAS. |
| `gs.Perf [Seconds=5]` | Log avg/p95/max frame time, game/render/GPU time and enemies alive. |
| `gs.Round.Skip` | Advance to the next round phase. |
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
| FireRate | Refire multiplier (1.0). |
| PickupRadius | Cash magnet radius in cm (player 300). |
| XPGain | XP multiplier on kill XP (player 1.0; constructor default 1.0). |
| ReloadSpeed | Reload time divisor for GA_Reload (constructor default 1.0, min 0.1). Not replicated yet. |
| ExtraJumps | Air jumps; AGSCharacter keeps JumpMaxCount = 1 + ExtraJumps (constructor default 0, min 0). Not replicated yet. |
| IncomingDamage | Meta attribute: GE_Damage writes it, the set converts it to Health loss and records the instigator. |

**Gameplay tags** (native, `Abilities/GSGameplayTags.*`; none in config):
- State: `State.Dead`, `State.Sliding`, `State.Dashing`, `State.Firing`, `State.Reloading`,
  `State.Invulnerable`, `State.Attacking`, `State.SafeZone` (hub: blocks GA_Fire, GE_Damage ignores the target)
- Ability: `Ability.Fire`, `Ability.Reload`, `Ability.Slide`, `Ability.Dash`, `Ability.SwapWeapon`,
  `Ability.Attack.Melee`, `Ability.Attack.Ranged`
- Cooldown: `Cooldown.Slide`, `Cooldown.Dash`
- SetByCaller data: `Data.Damage`, `Data.Cooldown`, `Data.HealthMultiplier`, `Data.DamageMultiplier`

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

**Abilities:**
- Player (`DA_PlayerAbilitySet`): `GA_Fire`, `GA_Reload`, `GA_SwapWeapon`, `GA_Slide`, `GA_Dash`.
- Enemies (`DA_EnemyAbilities_*`): `GA_MeleeAttack`, `GA_EnemyFire`.

## Automation tests (30, all passing)

`Tests/GSGameplayTests.cpp`. Each test opens L_Town_Graybox in the editor if needed (`Map.Hub`: L_Hub_Graybox),
starts PIE, waits for the player, removes enemies, runs and ends PIE. Stop PIE before running.

| Test | Checks |
|---|---|
| `GAS.DamageReducesHealth` | GE_Damage lowers Health. |
| `GAS.HealthClampsToZeroAndMax` | Health clamps at 0 and MaxHealth. |
| `GAS.DeathAddsStateDead` | Death adds `State.Dead`. |
| `Weapons.FireConsumesAmmoReloadRefills` | GA_Fire uses ammo, GA_Reload refills. |
| `Abilities.DashInvulnerabilityBlocksDamage` | Dash i-frames block damage. |
| `Map.Hub` | PIE in L_Hub_Graybox: AGSHubGameMode (no AGSGameMode), area MarketDistrict, round 0 / WaitingToStart; Casino, 4 stalls, MarketArrival, MarketExitPortalSpot; player starts at MarketArrival; no NavMesh; AGSMapBoundary contains the player and the Casino; gs.Spawn refused, no enemies; State.SafeZone (GE_SafeZone) on the player, GE_Damage blocked, the fire input fires nothing (no State.Firing, ammo unchanged); HUD in the safe area hides every combat element and shows the cash (= profile); still no round half a second later. |
| `Map.Town` | PIE in L_Town_Graybox: AGSGameMode (no hub GameMode), area Town, run started; no sublevels; no TownReturn / MarketPortalSpot / MarketArrival / MarketExitPortalSpot / Casino / MarketStall / Market actors; EnemySpawn points on the NavMesh; NavMesh under the player; boundary; no State.SafeZone; HUD not in a safe area; gs.Market.Teleport opens the hub (AGSHubGameMode, MarketDistrict); gs.Market.Return starts a run and opens the town again (round 1). |
| `Map.StartRunPortal` | PIE in the hub: a portal at MarketExitPortalSpot ("START RUN"), RunMap = L_Town_Graybox; gs.Run.Seed 4242 in the hub sets the next seed; walking into the trigger starts the run (fade to black, a second start refused); the town loads with seed 4242 (GameMode + GameState), round 1, fresh run stats, area Town, same cash, no safe zone; round 1's first spawns: no fallback points, closest ≥ ring min, farthest ≤ ring max, every enemy on the NavMesh and inside the boundary. |
| `Map.Boundary` | AGSMapBoundary in the town (ceiling 7500), player inside, cvar exists; 5 boxes: MapBoundary object type, block Pawn, ignore Visibility/Camera/WorldStatic/WorldDynamic/PhysicsBody, no NavMesh effect; player and enemy capsules block MapBoundary; Visibility / Camera / enemy-shot / pickup traces pass through a wall and the ceiling, a pawn sweep doesn't; spawn points near a corner are inside; player and a frozen enemy launched at a wall stay inside; dash and slide into the wall stop inside; a launch up stops at the ceiling (no damage, same XY); gs.Upgrade.Grant ExtraJump 3; an air jump under the ceiling works and is stopped by it. |
| `Round.PhaseLoop` | PreRound on start, timer → Combat; no market phases in the enum; RoundClear timer → round 2 PreRound; skips round 2 → round 3 PreRound; gs.Round.Set. |
| `Player.RestartRestoresInput` | Die → death screen UI input → restart (R) → the hub map (AGSHubGameMode, no run) → game input back, pawn not dead. |
| `Spawn.LocationFinder` | 12 points: on NavMesh, reachable, in ring, spaced; same seed → same points. (No spawns in the Market District: `Map.Hub`.) |
| `Round.WaveDirector` | Round 1 size matches DT_Waves; killing the wave ends Combat; skipping removes leftovers. |
| `Round.WaveBudget` | Seeded purchase within budget, MinRound respected, alive cap with telegraphs, spawns resume. |
| `Round.DifficultyScaling` | Round 1 base stats; round 10 = base × curve, full health; GameState multipliers. |
| `Balance.TimeToKill` | Rifle/pistol shots-to-kill at rounds 1/5/10 vs both bandits via the real fire path (`[ttk]` log). XPGain 0, so kills don't open upgrade offers. |
| `Run.SeedAndStats` | Same seed replays rolls; restart resets stats; kills per type; damage dealt/taken; rounds cleared. |
| `Economy.CashPickups` | Exact split; out-of-reach drop stays; radius collects into Cash/CashEarned/HUD + popup; pool reuse; RoundClear vacuum; short lifetime blinks, expires unpaid. Counts cash pickups only (the kills' XP orbs never expire). Ends the kill combo before each kill, so every drop is exactly CashValue. |
| `Economy.KillCombo` | DA_ComboSettings on the GameMode (2.5 s, x1/x1.25/x1.5/x2 steps, end on damage, no XP); step lookup at each boundary; kills 1–10 in a row: count +1, multiplier, cash drop = round(CashValue × multiplier), XP orb unchanged, window restarted, HUD count / multiplier / shown from 2, "10 COMBO" / "x2 CASH", best combo; a target dummy kill doesn't count; the window drains with game time and the HUD bar follows; a kill inside the window continues; it times out one window after the last kill (HUD hidden); pickups collected after the combo ended pay the kill-time multipliers; a level-up pause freezes the window (1 s real time), picking resumes it; god mode keeps the combo, health lost ends it (DamageTaken); override bEndComboOnDamage off / bApplyComboToXP on: XP orbs multiplied, damage keeps the combo; a new run ends it (Reset) and clears the best combo; gs.Run.Stats shows it. |
| `Economy.XPOrbs` | XPOrbLifetime 0; kill → no XP, one orb worth XPValue with the orb mesh / material, no expiry; cash expires (gs.Cash.Lifetime 1.5) but the orb stays, no blink; walking in collects it → XP = XPValue × XPGain 1.5, run stats XPEarned, "+N XP" popup at the collection point; out-of-reach orb waits, RoundClear vacuums it; gs.XP.Add is instant; a new run clears orbs on the ground; a dead player collects nothing. |
| `Progression.XPAndLevels` | Curve L2 = 100 / L3 = 125, max 50; partial XP; 260 XP → level 3 with 35 carried over (HUD level + bar follow); kill gives no XP but one orb; collected orb XP = XPValue × XPGain (×1, ×2); run stats XP/level; SetLevel 10 → 0 XP; cap at max level; new run resets. |
| `Progression.LevelUpRewards` | DA_LevelRewards wiring; +10 per stack; damaged → two level-ups → MaxHealth +20, Health full, 2 stacks, HUD health/level/XP follow; same in RoundClear; new run removes stacks and clamps Health; dead player gets the stack but no heal. Takes each upgrade offer (choice 0) so the game unpauses. |
| `Progression.LevelUpFeedback` | DescribeRewards "+10" / "+20 Max Health"; kill → no popup, collected orb → "+XPValue XP" at the collection point; double level-up → banner "Level 3" + "+20 Max Health", label, XP bar pulse, one sound, player glow; non-kill XP pops above the player; level-up during the banner merges (level 4, "+30"); 3 upgrade offers taken; banner/pulse/glow end. |
| `UI.UpgradeChoice` | Choice widget from the PlayerController; level-up pauses (GameMode owns the pause) and the cards wait for the banner; cards match the offer (order, rarity color, "Owned: N"); "Reroll (3 left)", cursor shown; key / click ignored during the 0.3 s lock; R, gamepad Y and the button reroll (label 2/1/0, new cards, lock again), greyed out at 0 and R does nothing; key 2 picks the 2nd card, last pick hides, unpauses, cursor off; 1000 XP → 1 open + 3 queued; D-pad selects, A picks; next offer swaps in at once (no banner wait), locked, still paused; click / numpad 1 / key 3 pick the rest; hidden and unpaused after the queue. |
| `UI.RoundHUD` | PreRound: "ROUND 1", "PRE-ROUND", countdown "Round 1 in 3…" → "… 2…" a second later, no enemy count, ROUND 1 banner, no icons; gs.Round.Set 2 restarts text + countdown and shows ROUND 2; a level-up hides it and queues it again (LEVEL UP!, offer open, paused); 3.5 s with the cards up: still waiting; the pick replays ROUND 2; icons: picked upgrade x1 → same again x2 (one icon) → another upgrade (two icons); Combat: "COMBAT", no countdown, "N enemies left" = wave, a kill updates it; RoundClear: phase text, no enemy count, ROUND CLEAR banner; a level-up during ROUND CLEAR then round 3 starting with the cards up → queued ROUND CLEAR + ROUND 3 → after LEVEL UP! only ROUND 3 shows (stale one dropped); no frame with a round banner over LEVEL UP! or an open offer; a new run clears the icons. |
| `Profile.PermanentCash` | Test slot; reset deletes the save; pickup → profile + HUD + CashEarned; spend / no overspend; no save on change; Combat → RoundClear pays the round-clear bonus into the profile; save when RoundClear ends includes it (version, no owned items); new run resets CashEarned but keeps the balance; death saves; balance survives death + return to the hub (the hub's HUD shows it). |
| `Round.ClearBonus` | DA_RoundClearSettings on the GameMode ($20 / $15 / $15, 8 s + 1.5 s per enemy) and the summary widget on the controller; PreRound starts the round's stats (no payout, no summary); Combat records its start and wave size; round 1 killed fast with no damage → base + no damage + fast clear, kills = wave size, profile += bonus + cash collected; summary shows round, title and each line; after the vacuum cash collected ≥ kills × CashValue, XP = kills × XPValue, profile and CashEarned += cash collected + bonus, summary lines follow; round 2 PreRound hides the summary and starts from zero; damage taken + an impossible time limit (override) → base only, "missed" lines, total = base; RoundClear from PreRound pays nothing and shows nothing; round 10 (skip) pays the same as round 1; gs.Run.Stats shows the last bonus; a new run clears payout and round stats. |
| `Upgrades.StatUpgrades` | The 3 DA_Upgrade_* assets (Common, no cap, description shows +10% / +8%); ReloadSpeed ×5 via `gs.Upgrade.Grant` → 1.5, 5 GE stacks, real reload shorter (≥ ReloadTime / 1.5); Damage ×3 → 1.3; MoveSpeed ×2 → base × 1.16 and MaxWalkSpeed follows; MaxStacks 1 refuses a second pick; unknown name refused; new run removes all upgrades and restores the stats. |
| `Upgrades.ExtraJump` | DA_Upgrade_ExtraJump (Rare, no cap, "+1"); no air jump without it; ×3 → ExtraJumps 3, JumpMaxCount 4; three air jumps (rising, falling, falling fast) each set AirJumpZVelocity, 4th refused, one puff/sound each; landing resets; coyote jump is the ground jump; late jump after walking off uses an air jump; slide-hop then air jump; jump during an air dash is queued and becomes an air jump at the dash end; jump during a ground dash becomes the ground jump; new run → 0 / 1. |
| `Upgrades.Offers` | DA_UpgradePool on the GameMode (4 upgrades, 100/20/6/1, 3 choices, 3 rerolls), 4 rarities; roll rules: 3000 offers all 3 different, Rare in ~42%, same seed same offers, 2 upgrades → offer of 2, MaxStacks excluded, empty pool → none; level-up opens an offer after the MaxHealth reward and heal; double level-up queues 2; 3 rerolls (events 2/1/0, Rerolled) then refused; bad index refused; picks grant a stack, Closed + Opened, last pick only Closed; new run closes the offer, refills rerolls, drops picks; reroll with no offer refused; same run seed → same first offer; simulator deterministic, no short offers, no Epic / Legendary, run stream untouched. |

All names start with `BanditBaller.`. Run them with `RunTestsByFilter("StartsWith:BanditBaller")`.

## Tuning values (current)

- **Player**: walk 900 cm/s, accel 6000, jump 800, air jump 800 (~1.87 m each), gravity ×1.75, coyote 0.12 s; camera arm 350 / FOV 90, ADS 180 /
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
- **Rounds** (DA_RoundSettings): PreRound 3 s, Combat safety fallback 180 s, RoundClear 4 s, then straight into the
  next PreRound.
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
  - Magnet 400 → 3000 cm/s (accel 3000), collected within 80 cm. Pool prewarm 48 (shared with XP orbs).
  - XP orbs: one per kill worth XPValue, same toss / magnet / vacuum, no lifetime (XPOrbLifetime 0), sphere ×0.32
    with M_XPOrb, pulse ±15% at 1.5 Hz.
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
- **Upgrades** (per stack, additive on the base value): Reload Speed +10%, Damage +10%, Move Speed +8% (Common);
  Extra Jump +1 air jump (Rare). None has a cap.
- **Upgrade offers** (DA_UpgradePool, placeholders): rarity weights Common 100 / Rare 20 / Epic 6 / Legendary 1,
  3 choices, 3 rerolls per run.
  - `simulate_upgrade_offers(10000, 1)`, fresh run with no picks: Damage in 86.3% of offers, Reload Speed 86.0%,
    Move Speed 85.9%, Extra Jump 41.7%.
  - That is expected: the lone Rare is in 1 − (5/6)³ = 42.1% of offers and each Common in ~86.0%.
  - By choices: Common 86.1%, Rare 13.9%, Epic and Legendary 0 (empty tiers). No short offers.
- **Combo curves**: there is no combo system yet.

## Known issues, TODOs and deviations

**Not built yet / stubs**
- **Not yet built for the design change:** the run summary and the return to the hub after death (Prompt 27).
- No upgrade is Epic or Legendary yet; those tiers are empty and never offered.
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
- No shop or casino logic yet (M3/M4); CashSpent is never used.
- Crits are stubbed (`TODO(crits)` in `GSAttributeSet.cpp`; damage numbers support them, nothing sets them).
- The `GameOver` phase exists but is unused. Player death shows the death screen, and R goes straight back to the hub
  (no run summary yet, Prompt 27). The cash balance survives it (profile, saved on death).
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
- The only level-up reward so far is +10 MaxHealth and a full heal. Player death (`?Restart`) reloads the map,
  which also resets levels and their MaxHealth stacks.
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
- Level-ups pause the game (upgrade offers). Tests or PIE sessions that need world time after a level-up must pick
  the offers (`gs.Upgrade.Choose 0`, `PickAllUpgradeOffers` in tests).

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
27. **Death → summary → hub.**
    - On player death, enter the GameOver phase.
    - Run summary widget (run stats: rounds, kills, cash earned, level), then Continue → `OpenLevel` back to the hub
      (`AGSGameMode::OpenHubMap` already exists; `HubMap` is set on BP_GSGameMode).
    - Nothing from the run carries over except what is chosen explicitly, so progression, upgrades, health, weapons
      and run stats reset by construction. Cash is kept in the profile. (Death → R → hub already works without the
      summary, see the fix after Prompt 26.)
28. **Tests + M2 checkpoint.** Fill test gaps for the new rules, run the full suite, update this file and close M2.

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
- **Epic / Legendary upgrades:** which upgrades get these tiers is decided later; the rarity weights are
  placeholders.
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
