# BanditBaller — Progress

Read this at the start of every session. After finishing a prompt, add a short entry at the end of the current
milestone section (what was built, new assets/commands, known issues) and update **Next up**.

## The game

Third-person roguelike shooter set in an off-grid bandit gambling town (UE 5.8, C++ + GAS). **Single-player
first**; 2–4 player listen-server co-op is planned for **M6** (see **Later — co-op (M6)**).
- **Hub:** the game starts in the safe **Market District** hub (shop + casino), its own map. A "Start Run" portal
  takes the player to the town map (`OpenLevel`).
- **Run:** PreRound → Combat → RoundClear → PreRound … with harder rounds, until the player dies. Bandits drop cash
  and XP as pickups, collected during the run.
- **After death:** a run summary, then back to the hub to spend or gamble the cash.
- **Permanent progression:** cash and owned items (saved profile).
  - Shop purchases and casino prizes are permanent unlocks that apply to every future run (M3/M4).
  - Casino bets risk cash only, never owned items.
- **Per-run progression** (reset on death): XP, level, and level-up upgrades.
  - Each level-up fully heals, adds MaxHealth, and offers 3 random upgrades in four rarity tiers (Common, Rare, Epic,
    Legendary).
  - The player must take one (no skip) but can reroll the choices a limited number of times per run (3 to start).
  - The game pauses while the player chooses.

The design changed after M2 Prompt 10; see **Design change — permanent cash, hub market, level-up upgrades**. The
scope then changed to single-player first; see **Scope change — single-player first**. Five more decisions came
before M3; see **Design update — decisions before M3**.
The in-run market is gone from the round loop (Prompt 14). The streamed district and its debug teleport stay until
the hub map replaces them.

**Current milestone: M2 — the run loop** (rounds, waves, scaling, economy, progression). Prompts 1–17 are done
(11–13 and 17 were design / scope updates and permanent cash).

State at the time of writing: `main` after Prompt 17, not pushed. The build is clean and all 21 automation tests pass.

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
| M2 17 | Design update: decisions before M3 (docs only) | see git log |

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

## M2 — The run loop (Prompts 1–16 done)

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
   run.
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

---

## Console commands and CVars

Non-Shipping cheats. They run on the server; clients forward them through `ServerExecDebugCommand`.

| Command / CVar | What it does |
|---|---|
| `gs.Spawn <Enemy> [Count=1]` | Spawn enemies around the player via the spawn finder (`MeleeBandit`, `RangedBandit` or a full `DA_Enemy_*` name). |
| `gs.KillAll` | Kill every living enemy through GE_Damage, credited to the player (they drop cash). |
| `gs.God [0\|1]` | Toggle/set damage immunity (one `State.Invulnerable` count). |
| `gs.GiveWeapon <Weapon>` | Equip `Rifle` / `Pistol` / `BanditCarbine` with a full mag (replaces the equipped one if both slots are full). |
| `gs.SetAttr <Attr> <Value>` | Set a player attribute base value (Health, MaxHealth, MoveSpeed, Damage, FireRate, PickupRadius, XPGain, ReloadSpeed). |
| `gs.Upgrade.Grant <Name>` | Add one stack of an upgrade (`ReloadSpeed`, `Damage`, `MoveSpeed` or `DA_Upgrade_*`); refused at MaxStacks. |
| `gs.Upgrade.List` | List every upgrade with rarity, this run's stacks / cap and description, plus ReloadSpeed / Damage / MoveSpeed. |
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
| `gs.Run.Seed [N]` | Print the run seed, or restart the run (stream + stats, round 1) with seed N. |
| `gs.Run.Stats` | Print run stats to log and screen (works on clients). |
| `gs.Market.Teleport` / `gs.Market.Return` | Debug only: move the party to the Market District and back (not part of the run loop since the design change). |
| `gs.Editor.MarketDistrictLevel create\|edit\|persistent` | Editor only: create the district sublevel / make it or the persistent level current. |
| `gs.Editor.BuildEnemyStateTree [melee\|ranged\|all]` | Editor only: regenerate ST_MeleeBandit / ST_RangedBandit from code. |
| `gs.Debug.Round 1` (cvar) | On-screen round, phase, time left, area, wave counts, enemy HP/DMG multipliers. |
| `gs.Debug.Spawns 1` (cvar) | Draw the spawn ring and chosen points (green ring, yellow fallback) for 8 s. |
| `gs.Debug.Weapons 1` (cvar) | Debug-draw shots and log enemy shots (`[shot]`). |
| `gs.Debug.DamageLog 1` (cvar) | Log every health loss (`[dmg] victim took N from X (health) at t=`). |
| `gs.Wave.MaxAlive <N>` (cvar) | Override the alive cap (0 = DA_WaveSettings). |
| `gs.Cash.Lifetime <s>` (cvar) | Lifetime of uncollected cash pieces (0 = DA_CashSettings). |
| `GSFire`, `GSPress <InputTag>`, `GSAimAt X Y Z`, `GSJump` | Exec on the controller: drive fire, any ability input, the camera, or a jump tap from the console. |

Launch arg: `-GSSeed=N` fixes the run seed.

MCP toolset `banditballer_toolset.toolsets.BanditBallerTools` (PIE running):
- `spawn_enemies(type, count)`
- `get_player_attributes()`: attributes plus Ammo, Cash (profile balance), Level, XP, XPForNextLevel, TotalXP, God, Invulnerable,
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
| ReloadSpeed | Reload time divisor for GA_Reload (constructor default 1.0, min 0.1). Not replicated yet. |
| ExtraJumps | Air jumps; AGSCharacter keeps JumpMaxCount = 1 + ExtraJumps (constructor default 0, min 0). Not replicated yet. |
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
| `GE_Upgrade_ReloadSpeed` | `UGSUpgradeEffect_ReloadSpeed` | Infinite, stacking; Multiply (Additive) 1.10 on ReloadSpeed → +10% of base per stack. |
| `GE_Upgrade_Damage` | `UGSUpgradeEffect_Damage` | Infinite, stacking; Multiply (Additive) 1.10 on Damage → +10% of base per stack. |
| `GE_Upgrade_MoveSpeed` | `UGSUpgradeEffect_MoveSpeed` | Infinite, stacking; Multiply (Additive) 1.08 on MoveSpeed → +8% of base per stack. |
| `GE_Upgrade_ExtraJump` | `UGSUpgradeEffect_ExtraJump` | Infinite, stacking; Additive +1 ExtraJumps per stack. |

**Abilities:**
- Player (`DA_PlayerAbilitySet`): `GA_Fire`, `GA_Reload`, `GA_SwapWeapon`, `GA_Slide`, `GA_Dash`.
- Enemies (`DA_EnemyAbilities_*`): `GA_MeleeAttack`, `GA_EnemyFire`.

## Automation tests (21, all passing)

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
| `Round.PhaseLoop` | PreRound on start, timer → Combat; no market phases in the enum; RoundClear timer → round 2 PreRound; skips round 2 → round 3 PreRound; gs.Round.Set. |
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
| `Progression.LevelUpFeedback` | DescribeRewards "+10" / "+20 Max Health"; kill → "+XPValue XP" popup at once; double level-up → banner "Level 3" + "+20 Max Health", label, XP bar pulse, one sound, player glow; non-kill XP pops above the player; level-up during the banner merges (level 4, "+30"); banner/pulse/glow end. |
| `Profile.PermanentCash` | Test slot; reset deletes the save; pickup → profile + HUD + CashEarned; spend / no overspend; no save on change; save when RoundClear ends (version, no owned items); new run resets CashEarned but keeps the balance; death saves; balance survives death + `?Restart` (new HUD shows it). |
| `Upgrades.StatUpgrades` | The 3 DA_Upgrade_* assets (Common, no cap, description shows +10% / +8%); ReloadSpeed ×5 via `gs.Upgrade.Grant` → 1.5, 5 GE stacks, real reload shorter (≥ ReloadTime / 1.5); Damage ×3 → 1.3; MoveSpeed ×2 → base × 1.16 and MaxWalkSpeed follows; MaxStacks 1 refuses a second pick; unknown name refused; new run removes all upgrades and restores the stats. |
| `Upgrades.ExtraJump` | DA_Upgrade_ExtraJump (Rare, no cap, "+1"); no air jump without it; ×3 → ExtraJumps 3, JumpMaxCount 4; three air jumps (rising, falling, falling fast) each set AirJumpZVelocity, 4th refused, one puff/sound each; landing resets; coyote jump is the ground jump; late jump after walking off uses an air jump; slide-hop then air jump; jump during an air dash is queued and becomes an air jump at the dash end; jump during a ground dash becomes the ground jump; new run → 0 / 1. |

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
  - Magnet 400 → 3000 cm/s (accel 3000), collected within 80 cm. Pool prewarm 48.
- **XP** (DA_XPCurve; XP needed from the previous level, +25% per level, max level 50):

  | Level | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  |---|---|---|---|---|---|---|---|---|---|
  | XP | 100 | 125 | 156 | 195 | 244 | 305 | 381 | 477 | 596 |

  - Reaching level 10 takes 2579 XP in total. Level 20 needs 5551 from level 19, and level 50 needs 4.48 M.
  - Kill XP: MeleeBandit 10, RangedBandit 15. DT_Waves rounds 1–5 give 40 / 65 / 90 / 115 / 150 XP.
- **Upgrades** (per stack, additive on the base value): Reload Speed +10%, Damage +10%, Move Speed +8% (Common);
  Extra Jump +1 air jump (Rare). None has a cap.
- **Combo curves**: there is no combo system yet.

## Known issues, TODOs and deviations

**Not built yet / stubs**
- **Not yet built for the design change:**
  - Hub start and Start Run portal.
  - Run summary and return to the hub.
  - Level-up upgrade choices (upgrade data, stat upgrades and `gs.Upgrade.Grant` exist; no pool, offers, rerolls,
    Epic / Legendary tiers or UI; Prompts 18–19).
- **Not yet built for the decisions before M3:**
  - XP pickups: XP is still granted instantly on the kill (Prompt 20).
  - The map boundary (Prompt 21). Until it exists, Extra Jump height is unlimited.
  - Permanent unlocks in `OwnedItems` (M3/M4).
- Upgrade icons are a placeholder engine texture.
- **Still in the code until the hub map (Prompts 25–26):** the streamed `L_MarketDistrict_Graybox`,
  `TravelToMarketDistrict` / `ReturnToTown` and `gs.Market.Teleport` / `Return` (debug only).
- No shop or casino logic yet (M3/M4); CashSpent is never used.
- Crits are stubbed (`TODO(crits)` in `GSAttributeSet.cpp`; damage numbers support them, nothing sets them).
- The `GameOver` phase exists but is unused. Player death reloads the whole map (`?Restart`); the cash balance
  survives it (profile, saved on death).
- Profile: there are no upgrade steps yet (version 1 is the only layout). A crash between save points loses the cash
  earned since the last RoundClear.
- Cash drops don't scale with the round. Neither does enemy XP.
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
- Air jump: the Niagara `RadialBurst` template and the `VR_teleport_Cue` editor sound (may not cook into a packaged
  build).

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

Each step is its own prompt. It builds, is tested and is committed, and it is built and tested for single-player
only. CLAUDE.md's rule "Only do what the current prompt asks" applies: the steps below are suggestions until a
prompt asks for one. (Earlier attempts at run loop cleanup and a per-player profile were reverted in 1a84a63.
Permanent cash was then built as a single local balance in a2dc3b7.)

14. ~~**Run loop cleanup.**~~ Done (see Prompt 14).
15. ~~**Upgrade data + stat upgrades.**~~ Done (see Prompt 15). The upgrade pool with rarity weights moved to 18.
16. ~~**Extra Jump.**~~ Done (see Prompt 16). It stays uncapped (user decision). The map boundary's ceiling
    (Prompt 21) limits the height.
17. ~~**Design update — decisions before M3.**~~ Done (docs only).
18. **Upgrade offers (4 rarity tiers + rerolls).**
    - Append Epic and Legendary to `EGSUpgradeRarity`.
    - An upgrade pool asset: the DA_Upgrade_* list plus a weight per rarity tier.
    - On each level-up, roll 3 distinct choices (run stream, rarity-weighted). Store them as pending offers on the
      PlayerState, queued across several level-ups.
    - Picking goes through a gameplay function that takes the PlayerState. There is no skip.
    - Reroll replaces the current 3 choices (run stream). Rerolls left per run start at 3 (from data) and reset with
      the run.
19. **Choice screen (with reroll).**
    - A choice panel next to the level-up banner: each card shows its rarity, description and current stacks.
    - A reroll button shows the rerolls left. There is no skip button.
    - The game pauses while an offer is open; queued offers keep it paused until the last pick.
20. **XP pickups.**
    - Enemies drop XP as pickups instead of granting it on the kill (replaces the instant `AwardXP` in
      `HandleEnemyKilled`). XPGain still applies.
    - Settle the details: magnet / radius, RoundClear vacuum, lifetime, and the "+N XP" popups (the kill pairing from
      Prompt 10 changes).
21. **Map boundary.**
    - Invisible walls around the town's play area and a ceiling. Collision blocks pawns only; shots, traces, pickups
      and the camera pass through.
    - The ceiling height sets the Extra Jump limit.
    - Test that players and enemies can't pass and shots can.
    - The hub map gets its own boundary in Prompt 25.
22. **Kill combo.**
23. **Round clear bonus.**
24. **Round HUD.**
25. **Hub map + hub GameMode.**
    - **Hub map** `Maps/L_Hub_Graybox`: the district content moves out of the sublevel, gets its own lighting, sky,
      boundary (walls + ceiling) and a PlayerStart at `MarketArrival`. It becomes `GameDefaultMap` /
      `EditorStartupMap`.
    - **GameModes:** a hub GameMode (no rounds; shop, casino and portal later) with its own Blueprint. The current
      `AGSGameMode` keeps the round loop as the town map's GameMode.
    - **Tests** open `L_Town_Graybox` explicitly, since the startup map changes.
26. **Start Run portal + town cleanup.**
    - A portal actor at `MarketExitPortalSpot` opens `L_Town_Graybox` (`OpenLevel`). The run (seed, stats, round 1)
      starts when the town loads.
    - Remove the streamed district, `TravelToMarketDistrict` / `ReturnToTown` and the in-town portal points.
      Repoint `gs.Market.*` to map travel (or retire them). Rework `Map.TownAndMarketDistrict`.
27. **Death → summary → hub.**
    - On player death, enter the GameOver phase.
    - Run summary widget (run stats: rounds, kills, cash earned, level), then Continue → `OpenLevel` back to the hub.
    - Nothing from the run carries over except what is chosen explicitly, so progression, upgrades, health, weapons
      and run stats reset by construction. Cash is kept in the profile. This replaces `?Restart`.
28. **Tests + M2 checkpoint.** Fill test gaps for the new rules, run the full suite, update this file and close M2.

Decided (user):
- **Single-player first;** co-op is M6.
- **Upgrade choice:** the game pauses while an upgrade is being chosen.
- **Hub map:** a separate hub map with its own GameMode; `OpenLevel` between hub and town.
- **Extra Jump:** no cap (MaxStacks 0); it can be picked any number of times. The map boundary's ceiling
  (Prompt 21) limits the height.
- **Permanent unlocks:** shop purchases and casino prizes, once owned, apply to every future run. They are stored in
  the profile (`OwnedItems`) and built in M3/M4.
- **Upgrade rarities and rerolls:** four tiers (Common, Rare, Epic, Legendary). The 3 choices can be rerolled a
  limited number of times per run (3 to start). There is no skip.
- **Casino bets** risk cash only, never owned items.
- **XP pickups:** XP comes from pickups enemies drop, not instantly on the kill.
- **Map boundary:** every map has invisible walls around the play area and a ceiling, blocking pawns only.

Still open from M2: round scaling for cash and XP drops, more DA_LevelRewards entries.

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
- Market travel doesn't wait for clients to stream the district (`TODO(co-op)` in `GSGameMode.cpp`). This goes away
  with the separate hub map.
- The profile is one balance per machine. On a listen server only the host's pickups are credited; remote players'
  pickups are not (`TODO(co-op)` in `AwardCash`). Clients' HUDs show their own machine's profile.
- Cash goes to whoever collects it, not the killer.
- XP goes only to the killing player (not shared with the party). Kills by non-players give no XP.
- The "+N XP" popup pairing is a best guess: several kills replicated in one update show their summed XP at the
  first kill.
