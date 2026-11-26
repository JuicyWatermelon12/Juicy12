# BanditBaller — Progress

Read this at the start of every session. After finishing a prompt, add a short entry at the end of the current
milestone section (what was built, new assets/commands, known issues) and update **Next up**.

## The game

Third-person roguelike shooter, cops and robbers (UE 5.8, C++ + GAS): the player is the robber, out to get all the
cash, and the enemies are law enforcement (since M2d; it was "bandits in a bandit town"). **Single-player first**;
2–4 player listen-server co-op is planned for **M6** (see **Later — co-op (M6)**).
- **Hub:** the game starts in the safe **Market District** hub, the black market and casino (shop + casino), its own
  map. A "Start Run" portal takes the player to the run map (`OpenLevel`): today the 300 × 300 m town, from Prompt 67 a
  500 × 500 m city with a 100 m ceiling.
- **Run:** a 3 s countdown, then rounds that roll over into each other (since Prompt 59): the next round's cops start
  arriving when only 3 of the current round are left, in a steady stream, and each round's clear pays a small pop-up
  bonus while play goes on; after every 10th round a 10 s breather. Harder rounds, until the player dies. Cops drop cash
  and XP as pickups, collected during the run. Cash and XP per enemy are flat (the same every round); income grows
  through more enemies and faster kills.
- **Enemies:** eight law-enforcement types that unlock round by round (Beat Cop and Patrol Officer built, `BeatCop` /
  `PatrolOfficer` in code since Prompt 57; Rusher, Shield, Grenadier, Heavy, Sniper and Drone in M2d). Dodgeable
  projectile shots, at most 40% ranged per generated wave, up to 100 alive.
- **Old enemy names:** entries before Prompt 57 use the old code names. MeleeBandit = Beat Cop (`BeatCop`),
  RangedBandit = Patrol Officer (`PatrolOfficer`), BanditCarbine = `PatrolCarbine`, and "bandit(s)" = cop(s).
- **Look:** low poly with high-quality shaders and lighting (reference: RevenBlade); placeholders made in Blender
  until the art pass (M5).
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
- **Per-run progression** (reset on death): XP, level, and level-up upgrades. Since M2c levels are endless: no
  level cap, and the XP needed per level grows by a fixed amount.
  - Each level-up fully heals, adds MaxHealth, and offers 3 random upgrade cards.
  - Upgrades are Stat Boosts, Abilities (with Legendary abilities as a sub-group) or Weapons (automatic attacks; at
    most 3 per run). Each card rolls its own rarity (Common, Rare, Epic, Legendary), and a higher rarity gives a
    bigger pick. Nothing has a pick cap.
  - The upgrade pool grew from 4 to 15 in **M2b** (see **Design update — M2b upgrades**). **M2c** reworks it; see
    **Design update — upgrade rework (M2c)**.
  - The player must take one (no skip) but can reroll the choices a limited number of times per run (3 to start).
  - The game pauses while the player chooses.

The design changed after M2 Prompt 10; see **Design change — permanent cash, hub market, level-up upgrades**. The
scope then changed to single-player first; see **Scope change — single-player first**. Five more decisions came
before M3; see **Design update — decisions before M3**. The market, prestige and economy were settled after that;
see **Design update — market, prestige and economy**. After M2, eleven more level-up upgrades were planned as block
**M2b**, before M3; see **Design update — M2b upgrades**. Before the M2b checkpoint the upgrade design was reworked
as block **M2c**; see **Design update — upgrade rework (M2c)**. After the weapons checkpoint the theme, look, map and
round flow changed (cops and robbers in a city, roll-over rounds, more enemy types) as block **M2d**; see **Design
update — cops, city and round flow (M2d)**.
The in-run market is gone from the round loop (Prompt 14). The streamed district and its debug teleport stay until
the hub map replaces them.

**M2 — the run loop is complete** (rounds, waves, scaling, economy, progression; Prompts 1–28 and 26b; 11–13 and 17
were design / scope updates and permanent cash). See **M2 checkpoint** under Prompt 28. **M2b**, eleven more
level-up upgrades, is built (Prompts 29–39; 29 is the docs-only plan, then Dash Charge (30), Fire Rate + Magnet (31) and the
held-fire fix (31b), Bigger Mags + Leech (32), Loaded Dice (33), Piercing Rounds (34), Powder Keg (35), Floating Shield (36), Second Wind (37) and Floating Gun (38, 39)). Health orbs (35b) were added between 35 and 36; they aren't an upgrade. Powder Keg became a chance to explode in 35c.
**M2c**, the upgrade rework, is built (Prompts 40–48; 40 is the docs-only plan, then upgrade data (41), the offer roll
(42), weapon slots (43), overflow effects (44), the Move Speed fix for the slide and dash (44b), endless levels (45), Razor Cards (46), Barbed Wire (47) and the
M2b + M2c checkpoint (48), which replaced the old M2b checkpoint). See **M2b + M2c checkpoint** under Prompt 48.
**Weapons** after M2c: Dynamite (49), Lucky Coin (50), Molotov (51) and Tomahawk (52) are built: all seven weapons exist.
Dynamite and the Molotov lead a moving target since 52b. **The weapons checkpoint (53) is done:** see **Weapons checkpoint**
under Prompt 53. **The weapons block is complete.**
**M2d** (cops, city and round flow) is planned (Prompt 54, docs only), Blender is connected through MCP (55), and the
Blender placeholder pipeline, the low-poly body and the robber are built (56), and the enemies are the Beat Cop and the
Patrol Officer with cop models (57); enemy shots are dodgeable bolts and generated waves are at most 40% ranged (58);
rounds roll over with a breather every 10 rounds (59); up to 100 cops alive at 60 fps, bigger waves, at most 20 corpses
(60); the Patrol Officer lost its aim laser (59b, run after 60); **next is Prompt 61** (the Rusher). After M2d's
checkpoint (69): **M3** (shop, owned items, gear loadout, two-layer save); not planned in detail yet.

State at the time of writing: `main` after "Fix: no aim laser on the Patrol Officer" (Prompt 59b, run after 60), not pushed. The build is clean and all 73 automation tests pass in about 214 s. The game and the editor start in the hub (`L_Hub_Graybox`). Walk into the Start Run portal to
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
| M2b 37 | Second Wind: a killing blow uses one instead (30% Health, 2 s invulnerable, one stack gone); flash, sound, banner | 5adacd1, aa887b5 |
| M2b 38 | Floating Gun part 1: level 1 = a gun over the right shoulder that shoots the nearest enemy with pooled projectiles (8 × Damage, crits, 1.5 / s on an exact schedule, 20 m); kept off the crosshair | c7f3d6f |
| M2b 39 | Floating Gun part 2: level 2 = a second gun over the left shoulder (never more than two), the guns prefer different targets, level 3+ = +10% damage and fire rate each; card text per next level | 062273f |
| M2c 40 | Design update: upgrade rework (categories, rarity per card, weapons with 3 slots, no pick caps, endless levels); Prompts 41–53 planned (docs only) | fc5bcd2 |
| M2c 41 | Upgrade picks: category, BaseValue, rarity multipliers in DA_UpgradePool, one GE per upgrade with the total (SetByCaller), FloatingGunPower, `gs.Upgrade.Grant <Name> [Count] [Rarity]`; offered picks still Common | f1dbf24 |
| M2c 42 | Per-card roll: Legendary ability chance 0.8%, then an upgrade by AppearanceWeight and a rarity from its category's weights (whole units: Rare → Common); cards show category, rolled rarity, pick line and "Now:"; simulator by category | 8602397 |
| M2c 43 | Weapon slots: MaxWeapons 3 in DA_UpgradePool, a weapon card in the first offer (kept by its rerolls), "NEW WEAPON" / "WEAPON UPGRADE" cards, three HUD weapon slots (icon only), weapons line in the run summary | e20b92f |
| M2c 44 | No pick caps (MaxStacks removed); Loaded Dice past 100% = triple-hit chance (x3, own damage number), Powder Keg past 100% = double-size blast chance (6 m); overflow on cards and gs.Upgrade.List; high-value stat check | 7247435 |
| M2c 44b | Fix: slide and dash scale with Move Speed (speed ratio = MoveSpeed / base walk speed, at least 1, kept per slide / dash); x33 checked against the boundary and town cover | aaac26b |
| M2c 45 | Endless levels: no cap; DA_XPCurve BaseXP 100 for level 2, +30 per level after that (replaces +25%); rewards, offers and HUD at any level | a3671e6 |
| M2c 46 | Razor Cards (weapon): homing cards at the nearest enemy (6 damage, 2 a second, 25 m, 30 m/s); homing on AGSProjectile; DA_RazorCardsSettings, DA_Upgrade_RazorCards, pool of 16 | b2af04e |
| M2c 47 | Barbed Wire (weapon): a ring on the ground (3.5 m) that hurts every enemy inside every 0.5 s (5 damage); +10% damage, +5% radius per power; `{Value2}` in card text; pool of 17 | 96adb88 |
| M2c 48 | Tests for the M2b + M2c rules (4 new), damage dealt by source and health orbs per round in run stats, auto-pick cvar, two PIE loops, M2b + M2c checkpoint | 4b08d56 |
| W 49 | Dynamite (weapon): every 4 s a stick lobbed at the enemy in range with the most neighbours, a 3 m blast where it lands (40 damage, crit per enemy); `AGSLobbedThrow` + `UGSThrownSubsystem`; Dynamite damage source; pool of 18 | f5d3d83 |
| W 50 | Lucky Coin (weapon): every 1.5 s a coin at the nearest enemy, bouncing to the nearest other enemy within 8 m (3 bounces, +1 per 2 power), each enemy once per flip; `AGSProjectile` ignore list + finish callback; pool of 19 | bceb293 |
| W 51 | Molotov (weapon): every 5 s a bottle lobbed at the nearest enemy within 15 m; a 2.5 m fire patch where it lands burns every enemy in it every 0.5 s for 4 s (4 per burn); `AGSFirePatch`; `gs.Molotov.Patches`; pool of 20 | e4746d4 |
| W 52 | Tomahawk (weapon): every 3 s a tomahawk at the nearest enemy flies 18 m out and back, hitting every enemy on its line once each way (12 per hit); a wall turns it back; +1 tomahawk per 3 power; `AGSTomahawk`; pool of 21 | bae9432 |
| W 52b | Fix: Dynamite and Molotov lead a moving target (LeadFactor 1, in data); PROGRESS.md repair; the four new weapon tests run on faster settings copies (suite 182 → 160 s); unity-build naming rule | 4878371 |
| W 53 | Weapons checkpoint: `Checkpoint.SevenWeapons` (pool contents, the slot limit for all 35 sets of three, real runs with rerolls, first offers), the simulator with owned upgrades, a DPS table for all seven weapons, two PIE loops with auto-pick, frame time with three weapons and 30+ enemies | 412063a |
| M2d 54 | Design update: cops and robbers, the city map, the low-poly look, Blender placeholders, enemy names, dodgeable shots, the 40% ranged cap, eight enemy types by round, 100 alive, roll-over rounds and the breather; Prompts 55–69 planned (docs only) | 23a6880 |
| M2d 55 | Blender MCP setup: uv 0.13.9, mcp-for-blender 2.3.1 and its Blender 5.3 add-on, the `blender` server in `.mcp.json` (safe mode, no telemetry), CLAUDE.md **Blender** section (tooling only) | fd5d46d, e0c39e6 |
| M2d 56 | Blender placeholder pipeline (FBX; `M_LowPoly`; CLAUDE.md **Placeholder models**), baton / cap / carbine, `SK_LowPolyBody` (1,524 triangles) on `SK_Mannequin`, the robber on the player (ski mask and loot bag as worn props, `FGSMeshAttachment`), `Art.PlayerMesh`, MCP tools `export_asset_fbx` / `import_mesh_file` / `describe_ref_pose` / `console_command` | 927ee52 |
| M2d 57 | Rename the enemies: MeleeBandit → Beat Cop (`BeatCop`), RangedBandit → Patrol Officer (`PatrolOfficer`), BanditCarbine → `PatrolCarbine` (assets, code, commands, tests); cop models on `SK_LowPolyBody` (`SlotMaterials`, `MeshAttachments` on `UGSEnemyData`; police cap placed on the head, baton, carbine); `TelegraphAmount` in `M_LowPoly`; `Enemies.CopLooks` | 8ece272 |
| M2d 58 | Dodgeable enemy shots: `GA_EnemyFire` fires a pooled `AGSProjectile` bolt per shot (`bEnemyShot`: hits the player or world only, passes other enemies), 2,800 cm/s at the chest with no lead, `SM_EnemyBolt` (Blender) with `MI_EnemyBolt_*`, no tracer; ranged cap `MaxRangedShare` 0.4 (`UGSEnemyData::bRanged`); `Enemies.DodgeableShots`, `Round.RangedCap` | 995328a |
| M2d 59 | Roll-over rounds: phases PreRound (round 1 only) / Combat / Breather / GameOver, no RoundClear; the next round starts at `RollOverRemaining` 3 left; several open rounds in `UGSWaveDirector`, one enemy at a time (20 s window, 0.25–2 s gap), waves from a per-round stream; per-round stats, bonus, vacuum and save at each round's clear; HUD "N enemies left · N alive", ROUND N banner, round-clear pop-up (`WBP_RoundSummary` removed); a 10 s breather after every 10th round; `Round.RollOver`, `Round.Breather`, `Round.RollOverPace` | abce280 |
| M2d 60 | One hundred enemies: `MaxAliveEnemies` 100, generated budget 5 + 5 × round, `MaxCorpses` 20 (oldest corpse removed first); performance switches on `UGSEnemyData` (no body-mesh collision while alive, no off-screen pose, animation rate optimization, no capsule overlap events), pooled one-shot effects (`GSEffects`, `gs.Debug.NoFXPool`); 100 cops with three area weapons 15.5 → 12.0 ms; the four slowest tests sped up (suite 251 → 214 s); `Round.AliveCap100`, `Round.BiggerWaves`, `Enemies.CorpseLimit` | f9831a1 |
| M2d 59b | Fix (run after 60): no aim laser on the Patrol Officer (`DA_Enemy_PatrolOfficer.AimLaserMaterial` cleared; the 0.4 s body glow is its warning); the laser code and `M_EnemyLaser` kept for the Sniper and the spawn markers; `Enemies.CopLooks` checks no laser through the aim and burst | see git log |

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
    static modifier (Multiply magnitude 1.1 → "10"), so the number lives only in the GE. (Since Prompt 39 an optional
    `DescriptionsByOwnedStacks` gives a different card text per number of stacks owned.)
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

(Partly replaced by **Design update — upgrade rework (M2c)**: rarity is rolled per card instead of fixed per
upgrade, there are no MaxStacks, and Floating Gun is a weapon. The checkpoint planned here as Prompt 40 is now
Prompt 48.)

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
- ~~Loaded Dice and Piercing Rounds: do they also apply to the Floating Gun's projectiles? Is a piercing shot one dice
  roll for every enemy it hits?~~ Decided in Prompts 33 and 38: every hit rolls (so a piercing shot rolls per enemy
  hit), the Floating Gun's projectiles roll too (`GSCombat::RollCriticalHit`), and Piercing Rounds doesn't apply to
  them.
- ~~Powder Keg: does an enemy killed by a blast explode too (a chain)? Does "bigger" mean the radius only, or the
  damage too?~~ Decided in Prompt 35: yes, chains, each link 0.1 s after the last; both radius and damage grow.
  Changed in Prompt 35c: stacks raise the chance to explode (3% each), the blast stays the same, and each chain kill
  rolls the chance too.
- Floating Gun: ~~does the player's Damage upgrade scale the guns? Which enemy does a gun target?~~ Decided in Prompt
  38: the gun's damage is × the player's Damage attribute, and it targets the nearest living enemy in range with a
  clear line. Still open: does the player's Fire Rate also speed up the guns? (Today it doesn't.)
- ~~Second Wind: is there a short invulnerability after coming back?~~ Decided in Prompt 37 (user): yes, 2 s.

**Docs changed**
- CLAUDE.md: the game summary says the upgrade pool grows to 15 in M2b.
- This file:
  - the game summary, the intro and the step table;
  - this entry;
  - **Decided (user)** and **Next up** (the M2b block, Prompts 29–40, before M3);
  - the known issue about empty Epic and Legendary tiers.

## M2b — More level-up upgrades (Prompts 29–39; checkpoint in Prompt 48)

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

### Prompt 38 — Floating Gun, part 1
Level 1 only: one gun over the right shoulder. The second gun and the +10% picks are Prompt 39.
- **New attribute `FloatingGunLevel`** (default 0, clamped to ≥ 0; not replicated, `TODO(co-op)`). Each pick adds 1.
- **`Combat/GSFloatingGunSettings.h` (`UGSFloatingGunSettings`) → `Data/DA_FloatingGunSettings`**, set as
  `FloatingGunSettings` on BP_GSGameMode (`AGSGameMode::GetFloatingGunSettings()`):
  - Shooting: `Damage` 8, `ShotsPerSecond` 1.5, `Range` 2000 cm, `TargetRefreshInterval` 0.1 s, `bLeadTargets`,
    `DamageEffect` = GE_Damage.
  - Projectile: `ProjectileSpeed` 6000 cm/s (60 m/s), `ProjectileLifetime` 0.75 s (45 m), `ProjectileRadius` 8 cm,
    `ProjectilePoolPrewarm` 16, the engine sphere ×0.12 with `Materials/M_FloatingGunProjectile`.
  - Placement: `ShoulderOffset` (−15, 45, 115) from the capsule center in the camera's yaw frame, `FollowSpeed` 14,
    `TurnSpeed` 12, and the crosshair clearance below.
  - Look (placeholders): the engine cube at (0.4, 0.1, 0.12) with `Materials/M_FloatingGun` (a copy of M_XPOrb, gold),
    `MuzzleOffset` 25 cm, `FireSound` = `VR_click1_Cue` (editor content), `ImpactEffect` = the Niagara `RadialBurst`
    template at ×0.25.
- **The game's first projectile: `Combat/GSProjectile.*` (`AGSProjectile`) and `Combat/GSProjectileSubsystem.*`
  (`UGSProjectileSubsystem`, a world subsystem pool).**
  - Launched with an `FGSProjectileLaunch` (start, direction, speed, lifetime, radius, damage, source ASC, damage
    effect, look). It flies straight and sweeps each tick against WorldStatic, WorldDynamic and Pawn.
  - **It ignores the player** (player pawns are skipped), pickups (no collision) and the map boundary (its own object
    type, not in the sweep). Dead enemies are passed through.
  - **The first living target with an ASC** takes GE_Damage from the source's ASC: SetByCaller `Data.Damage`, its own
    crit roll (`GSCombat::RollCriticalHit`, so Loaded Dice applies; a crit sets `Data.Critical`), and the hit result.
    World geometry stops it. Either way it plays the impact effect and goes back to the pool. It also goes back
    when its lifetime runs out.
  - Damage is set when it is fired (Damage attribute at that moment); the crit is rolled when it hits.
  - Counters for tests and debug: active, pooled, created, fired, hit targets, hit world, expired. A new run
    (`AGSGameMode::StartRun`) releases every projectile.
  - Not replicated (`TODO(co-op)`).
- **`Combat/GSFloatingGun.*` (`AGSFloatingGun`)**: the gun actor, owned by the character. No collision, not
  replicated (`TODO(co-op)`), ticks after physics so it follows the moved character.
  - **Placement:** its rest point is the capsule center + `ShoulderOffset` turned by the camera's yaw (Y mirrored for
    a left gun later). So it stays over the right shoulder *as the player sees it*, whichever way the character
    faces. That is 19 cm above the top of the head, 45 cm to the right. It follows with `VInterpTo` (it snaps if more
    than 5 m behind, e.g. after a teleport), faces its target or the camera's direction otherwise.
  - **Off the crosshair** (local player only): the rest point is projected on screen. Within `CrosshairClearRadius`
    (10% of the screen height) of the screen center, it slides away from the center (camera right / up) at
    `CrosshairAvoidSpeed` 600 cm/s, up to `CrosshairMaxAvoid` 150 cm. Beyond 1.5 × the radius it eases back.
  - **Targeting:** every `TargetRefreshInterval`, and before each shot, the nearest living `AGSEnemyCharacter` within
    `Range` of the gun with a clear line (WorldStatic / WorldDynamic trace; walls block it, enemies don't).
  - **Shot schedule (like Prompt 31b):** each shot is due one interval after the previous shot's *scheduled* time,
    never timed from the frame it happened to fire on. A frame that spans more than one interval fires each shot it
    owes; a late shot starts as far along its path as it would have flown. With no target (or no shooting allowed)
    the schedule doesn't bank shots: the next shot is due at once when a target appears, then on the schedule.
  - **Aim:** at the target's center, leading a moving target by the flight time.
  - **No shooting** while the player is dead (`State.Dead`), in a safe zone (`State.SafeZone`, the hub) or with no
    ASC. The game pause stops it (no ticks).
  - **Damage:** `Damage` × the player's **Damage** attribute. Piercing Rounds doesn't apply (one target per
    projectile).
  - **Its kills are the player's:** the damage comes from the player's ASC, so `HandleEnemyKilled` credits the player
    like a shot: cash with the combo multiplier, XP orb, health orb chance, combo, Leech, Powder Keg and run-stats
    kills.
  - Test hooks: `GetTarget`, `GetShotsFired`, `GetBurstStartTime`, `GetBurstShots`, `GetRestLocation`,
    `GetCrosshairAvoidOffset`, `GetCrosshairDistance`, `GetSide`, `GetMesh`, `FindTarget`, `CanShoot`,
    `GetMuzzleLocation`.
- **`Combat/GSFloatingGunComponent.*` (`UGSFloatingGunComponent`, on `AGSCharacter`, `GetFloatingGuns()`)** keeps the
  guns in step with FloatingGunLevel (server): level 1 or more → one gun on the right. It destroys them at 0 (a new
  run) and when the character ends. Log: `[gun] <Character>: Floating Gun level 1 -> gun 1 over the right shoulder`.
- **Character debug hooks** (tests): `SetAimingForDebug(bool)`, `SwapShoulderForDebug()`, `GetShoulderSide()`.
- **Upgrade:** `UGSUpgradeEffect_FloatingGun` → `Abilities/GE_Upgrade_FloatingGun` (+1 FloatingGunLevel per stack).
  `Data/Upgrades/DA_Upgrade_FloatingGun`: **Epic**, no cap, "+{PerStack} Floating Gun level per stack. Level 1: a gun
  floats over your right shoulder and shoots the nearest enemy on its own." Added to DA_UpgradePool (15: 6 Common,
  5 Rare, 3 Epic, 1 Legendary). **The M2b pool is complete.** A second pick raises the level but changes nothing yet
  (Prompt 39).
- **Debug:** `gs.Upgrade.Grant FloatingGun [N]`. `gs.Upgrade.List` adds FloatingGunLevel to its Stats line and a new
  line, "Floating Gun: level 1, 1 gun(s): right shoulder: 12 shot(s), target GSEnemyCharacter_3".
- **Offer odds** (10,000 offers, seed 1): Floating Gun in 4.6% of offers. The three Epics share the tier (13.4%):
  Powder Keg 5.0%, Piercing Rounds 4.5%. The Rare tier stays 40.6%.
- **Tests:**
  - New `Upgrades.FloatingGun`.
  - `Upgrades.Offers` expects 15 upgrades. Its Rare rate is unchanged (40.2%): the Rare tier can't run out within an
    offer.
  - First run: the Damage × crit check read 16 instead of 24. A shot fired before the Damage grant landed after Loaded
    Dice was granted (damage is set at launch, the crit is rolled at the hit). The test now grants with no projectile
    in flight. The code was right.
- **Crosshair check** (the test logs each view's distance from the gun to the crosshair, as a fraction of the screen
  height; the clear radius is 0.10):

  | Camera | Pitch −70 | −50 | −30 | 0 | +25 | +50 |
  |---|---|---|---|---|---|---|
  | Right shoulder, hip | 0.142 | 0.124 (pushed 20 cm) | 0.133 (pushed 10 cm) | 0.151 | 0.140 | 0.283 |
  | Right shoulder, ADS | 0.791 | 0.258 | 0.451 | 0.482 | 0.342 | 0.203 |
  | Left shoulder, hip | 0.415 | 0.370 | 0.349 | 0.315 | 0.285 | 0.391 |
  | Left shoulder, ADS | 2.310 | 1.781 | 1.399 | 1.002 | 0.756 | 0.610 |

  - The closest in any view is 0.124 (right shoulder, hip fire, looking down at −50°). Values above 0.5 are off
    screen (left-shoulder ADS at level or lower pitch).
  - Only right-shoulder hip fire looking down needs the push, and it stays small (10–20 cm).
- **PIE check** (town, god mode, round paused, editor throttling off; restored after):
  - `gs.Upgrade.Grant FloatingGun`, `gs.Spawn MeleeBandit 3`: the gold gun appeared above the right shoulder and shot
    the bandits standing at the player, 8 per hit ("[dmg]" and the bandits' health: 60 → 20 / 12). It killed every
    bandit spawned in the session (XP orbs and cash dropped, "2 COMBO").
  - Level aim (`GSAimAt`), screenshots:
    - **Right shoulder, hip:** the player is left of the crosshair; the gun floats above and right of the head, about
      80 px up and left of the crosshair (1280 × 720 editor capture). Clear.
    - **Left shoulder, hip:** the player is right of the crosshair; the gun is above their right shoulder, about 150 px
      right and 75 px up from it. Clear.
    - **Right shoulder, ADS:** the gun is about 140 px up and left of the crosshair. Clear, and it looks bigger.
    - **Left shoulder, ADS:** the gun is large, in the top-right corner, partly off screen. Clear.
  - An earlier capture right after a shoulder swap caught the camera mid-blend with motion blur. One with the camera
    pitched far up (looking at the sky, the player very close) showed the player's body over the crosshair, not the
    gun.
- **Full suite:** 46 tests, all passing, in 109.4 s.

### Prompt 39 — Floating Gun, part 2
- **Levels** (`UGSFloatingGunSettings`, rules as static helpers, bonuses in data):
  - `GetGunCount(Level)`: level 1 → one gun (right shoulder), level 2 and up → two (the second over the left
    shoulder). `MaxGuns` 2 is a rule, not tuning: there are never more than two.
  - `GetBonusLevels(Level)` = levels past 2. From level 3 on, each level adds `DamageBonusPerLevel` 0.1 and
    `FireRateBonusPerLevel` 0.1 (new, DA_FloatingGunSettings; the asset uses the class defaults), added up, not
    compounded: level 5 = ×1.3 damage and ×1.3 fire rate (1.95 shots a second per gun).
  - `GetDamageMultiplier(Level)`, `GetFireRateMultiplier(Level)`, `GetShotInterval(Level)`.
- **`UGSFloatingGunComponent`** keeps `GetGunCount(Level)` guns: the second spawns with Side −1, so it mirrors the
  first (−45 cm in the camera's yaw frame, the same 115 cm height). Level 0 (a new run) destroys both. A level with a
  bonus logs `[gun] … Floating Gun level 5 -> both guns x1.30 damage, x1.30 fire rate (1.95 shots/s each)`.
- **`AGSFloatingGun`:**
  - Reads its level from the owner's component every shot: `GetLevel`, `GetShotInterval`, `GetDamageMultiplier`.
    Damage = `Damage` × the level multiplier × the player's Damage.
  - **A fire rate change mid-burst** (a level pick) re-times like GA_Fire: the next shot is due one new interval after
    the last shot's scheduled time, and the burst's schedule starts over from that shot (`GetBurstInterval`).
  - **Targets:** each gun picks on its own, the nearest from where it floats, but skips the other gun's target
    (`FindOtherGun`, `FindTargetAvoiding`). It takes the other gun's target only when nothing else is in range with a
    clear line. Both re-pick every 0.1 s, so when a second enemy comes into range they split up, and when one dies
    both go to the one left.
- **Card text by level** (`UGSUpgradeData::DescriptionsByOwnedStacks`, new and optional): entry N is the card for
  the next pick when N stacks are owned; past the end the last entry repeats. `GetFormattedDescription(OwnedStacks)`
  picks it; the choice card, the `[upgrade]` grant line, `gs.Upgrade.List` and `gs.Upgrade.Offer` pass the stacks.
  DA_Upgrade_FloatingGun:
  - none owned: "Adds a floating gun over your right shoulder that shoots the nearest enemy on its own."
  - one owned: "Adds a second floating gun, over your left shoulder."
  - two or more: "+10% floating gun damage and fire rate."
  - Its `Description` (used where there is no stack count) sums up all three levels.
  - The "10" is typed in the card text, like Powder Keg's "3%". `Upgrades.FloatingGunLevels` fails if it doesn't
    match DA_FloatingGunSettings, or if the damage and fire rate bonuses differ (the card has one number).
- **Debug:** `gs.Upgrade.List`'s Floating Gun line adds the multiplier and the rate: "Floating Gun: level 5, 2
  gun(s), x1.30 damage, 1.95 shots/s each: right shoulder: 34 shot(s), target …; left shoulder: 16 shot(s), target
  …". UGSUpgradeCardWidget test hook `GetDescriptionText()`.
- **Damage per second** (Damage 1, no crits; from the settings and measured in the test):

  | Source | DPS |
  |---|---|
  | One gun, level 1 or 2 | 12.0 (8 × 1.5 shots/s; measured 12.0 per gun at level 2) |
  | Both guns, level 2 | 24.0 |
  | One gun, level 5 | 20.3 (10.4 × 1.95; measured 20.8 over 4 s) |
  | Both guns, level 5 | 40.6 |
  | Rifle, while firing | 200 (20 × 10 shots/s) |
  | Rifle, with reloads | 125 (30 rounds in 3.0 s, then a 1.8 s reload) |

  One gun at level 1 is 6% of the rifle's held-fire damage (10% with reloads). Both guns at level 5 are about 20%
  (32% with reloads), and they keep shooting while the player reloads, moves or aims elsewhere.
- **Tests:**
  - New `Upgrades.FloatingGunLevels`. Measured: right gun rest (−15, 45, 115), left (−15, −45, 115); 182
    gun-frames on schedule through the level 5 pick; 16 hits of exactly 10.4 at level 5.
  - `Upgrades.FloatingGun` (Prompt 38) is unchanged and passes: level 1 behaves as before.
  - `UI.UpgradeChoice` and `Upgrades.Offers` pass with the card change.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after):
  - `gs.Upgrade.Grant FloatingGun 1`: one gun; `[dmg]` 8 per hit on the nearest bandit, about every 0.6 s (the
    bandit walking in shortens the flight).
  - `gs.Upgrade.Grant FloatingGun 1` (level 2): a second gold gun appeared over the left shoulder, level with the
    first. Both aimed at the bandit next to the player (two "8" numbers on it). With a level aim, the right gun stayed
    about 80 px up and left of the crosshair; the left gun is further left. The upgrade log read "Adds a second
    floating gun, over your left shoulder."
  - `gs.Upgrade.Grant FloatingGun 3` (level 5): the log printed the three "+10% floating gun damage and fire rate"
    picks and the x1.30 line. `[dmg]` showed 10.4 per hit, the two guns on two different bandits
    (GSEnemyCharacter6 and 9), each about 0.5 s apart. The guns' kills then dropped enough XP to level the player up,
    and the choice screen opened over the two guns and a 7-kill combo (normal game behaviour).
- **Full suite:** 47 tests, all passing, in 112.6 s.

## Design update — upgrade rework (M2c)

Prompt 40. Docs only; no code changed. **M2c** reworks the level-up upgrades after M2b is built and before its
checkpoint. The old Prompt 40 (M2b tests + checkpoint) becomes Prompt 48 and covers M2b and M2c together. Until
Prompts 41–45 change it, the code still follows the M2b design: one rarity and a MaxStacks per upgrade, MaxLevel 50
and +25% XP per level.

**1. Categories (user).** Every upgrade is in exactly one:

| Category | Upgrades |
|---|---|
| Stat Boosts | Damage, Fire Rate, Reload Speed, Move Speed, Magnet, Bigger Mags, Loaded Dice |
| Abilities | Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech, Powder Keg |
| Legendary abilities (a sub-group of Abilities) | Second Wind today; more later |
| Weapons (automatic attacks) | Floating Gun today; Razor Cards (46) and Barbed Wire (47) next; Dynamite, Lucky Coin, Molotov and Tomahawk after the checkpoint (49–52) |

What each new weapon does is designed in its own prompt.

**2. Rarity is rolled for each card, not fixed per upgrade.** Any stat boost, ability or weapon can come up Common,
Rare, Epic or Legendary. A higher rarity gives a bigger increase for that pick:

| Rarity | Multiplier | Whole-unit upgrades |
|---|---|---|
| Common | ×1 | +1 |
| Rare | ×1.5 | no Rare version |
| Epic | ×2 | +2 |
| Legendary | ×3 | +3 |

- Whole-unit upgrades: Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech.
- With today's per-pick values as the Common value (all of them stay in data):

  | Upgrade | Common | Rare | Epic | Legendary |
  |---|---|---|---|---|
  | Damage, Reload Speed | +10% | +15% | +20% | +30% |
  | Fire Rate, Move Speed | +8% | +12% | +16% | +24% |
  | Magnet, Bigger Mags | +15% | +22.5% | +30% | +45% |
  | Loaded Dice (crit chance) | +5% | +7.5% | +10% | +15% |
  | Powder Keg (chance to explode) | +3% | +4.5% | +6% | +9% |
  | Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech | +1 | — | +2 | +3 |

- Weapons use the multiplier as weapon power (6 below).

**3. Rarity weights per card** (placeholders, in data):

| Category | Common | Rare | Epic | Legendary |
|---|---|---|---|---|
| Stat Boosts and Weapons | 100 (78.7%) | 20 (15.7%) | 6 (4.7%) | 1 (0.79%) |
| Abilities | 100 (88.1%) | 10 (8.8%) | 3 (2.6%) | 0.5 (0.44%) |

- Strong ability picks are rarer than strong stat boosts.
- For the whole-unit abilities this doc assumes Rare is left out of the roll: 96.6% Common, 2.9% Epic, 0.48%
  Legendary (an open question, below).

**4. Legendary abilities** have no rarities.
- Each card has a small fixed chance, 0.8% (about the Legendary rate, 1 / 127), to be a Legendary ability instead of
  a normal card. The chance is in data.
- A pick adds one charge.
- Second Wind charges are used one at a time: the next one can save the player once the invulnerability from the
  last has ended. Today's code already works this way: the 2 s `State.Invulnerable` keeps GE_Damage from applying,
  so a killing blow inside it uses no charge (`Upgrades.SecondWind` checks this).

**5. Weapons in a run.**
- A run holds at most `MaxWeapons` 3 different weapons (in data).
- Until the player owns 3, any weapon can be offered: owned ones as upgrades, the others as new weapons. After that
  only the owned weapons are offered, as upgrades.
- The rifle and pistol (GA_Fire and UGSWeaponComponent's two slots) are not part of this. They don't count toward
  MaxWeapons and are never offered.
- The first level-up of a run always includes one weapon card.

**6. Weapon picks and power.**
- The first pick of a weapon gives the weapon.
- Every pick adds weapon power equal to its rarity multiplier. The first pick adds the multiplier − 1, so a Common
  first pick is the plain weapon:

  | Pick | Common | Rare | Epic | Legendary |
  |---|---|---|---|---|
  | First (gives the weapon) | +0 | +0.5 | +1 | +2 |
  | Later | +1 | +1.5 | +2 | +3 |

- Each point of power is one upgrade step, defined per weapon (in its data).
- Floating Gun today (Prompt 39): level 1 = one gun, level 2 = the second gun, level 3+ = +10% damage and fire rate
  each. As power steps that would be step 1 = the second gun and each later step +10% (to confirm in Prompt 41). A
  Legendary first pick would then give both guns and +10%.

**7. No limits.** Nothing has a pick cap: the MaxStacks of Loaded Dice (20) and Powder Keg (34) go; the other
upgrades had none. Where a stat reaches a natural limit it gets a secondary effect:
- **Loaded Dice past 100%:** every hit crits, and the chance above 100% is the chance of a triple-damage hit instead
  (e.g. 130% → 30% of hits deal triple damage).
- **Powder Keg past 100%:** every player kill explodes, and the chance above 100% is the chance of a double-size
  blast.
- Extra Jump is still limited only by the map boundary's ceiling.

**8. Endless levels.** There is no level cap (MaxLevel 50 goes). The XP needed per level grows by a fixed amount
each level instead of 25% (today: 100 for level 2, then ×1.25). The amounts are placeholders in DA_XPCurve, picked in
Prompt 45.

**9. HUD.** Three weapon slots in the top left, shown like the upgrade icons and filled in pick order as weapons are
picked. Icon only.

**Rules for every weapon (user)**
- It attacks on its own.
- Its damage is multiplied by the player's Damage attribute, and it can crit (Loaded Dice).
- Piercing Rounds doesn't apply to it.
- Its kills are the player's: cash drop with the combo multiplier, XP orb, health orb chance, combo, Leech, Powder Keg
  and run-stats kills.
- It doesn't act while the player is dead, in a safe zone (`State.SafeZone`) or while the game is paused.
- Its timing follows an exact schedule, as in Prompt 31b (and the Floating Gun since Prompt 38).
- It has no collision.
- A new run removes it.
- The M2b rules still hold for every upgrade: an attribute + infinite stacking GE where it fits, a DA_Upgrade_* asset,
  an entry in DA_UpgradePool, a `gs.Upgrade.Grant` short name and a `gs.Upgrade.List` line; the damage order (dash
  invulnerability → Floating Shield → Health → Second Wind); new rolls on the run stream; all damage through GE_Damage;
  single-player only, co-op-ready.

**What changes in the code and tests** (checked for this plan; built in Prompts 41–45)
- `UGSUpgradeData`: `Rarity` and `MaxStacks` go. A category, the per-pick base value, the whole-unit flag and (for
  weapons) the power steps come in (41). How a pick's size reaches its GE (a SetByCaller magnitude, or one stack per
  unit) is decided in 41.
- `UGSUpgradeComponent` counts one stack per pick today. With rarity a pick is no longer one stack, so the card's
  "Owned: N", the HUD icon's "xN", `gs.Upgrade.List` and the run summary need another measure (41).
- `UGSUpgradePool` / `RollOffer`: weights per category, the Legendary ability chance, the weapon rules and the first
  level-up's weapon card (42). The simulator (`simulate_upgrade_offers`) reports rarities per category.
- The cards show the rolled rarity's colour and the pick's value (42). Hand-typed card numbers (Powder Keg's "3%",
  the Floating Gun's "+10%") need a value per rarity; DA_Upgrade_FloatingGun's `DescriptionsByOwnedStacks` becomes
  per power step.
- `FloatingGunLevel` becomes weapon power (41); the weapon slots are a new HUD element (43).
- `CritChance` is clamped to 0..1 and `UGSCombatSettings::GetExplodeChance` to 100% today (44).
- DA_XPCurve MaxLevel 50, the `gs.XP.SetLevel` clamp and the XP bar's "MAX" (45).
- Tests that assume the old design: `Upgrades.Offers` (rarity per tier, Rare in ~40.2%), `Upgrades.StatUpgrades`
  (MaxStacks 1 refuses a second pick), `Upgrades.LoadedDice` (MaxStacks 20, the clamp), `Upgrades.PowderKeg`
  (MaxStacks 34, the 100% clamp, the card text), `Upgrades.FloatingGun` and `Upgrades.FloatingGunLevels` (levels,
  card texts), `Upgrades.SecondWind` (stacks), `Progression.XPAndLevels` (level 3 = 125, the cap at 50, 40 offers
  to the cap), `UI.UpgradeChoice` (rarity colour from the upgrade), `UI.RoundHUD` and `Run.NewRunResets` (icons and
  stacks).

**Open questions** (settled in each prompt; not decided)
- ~~How a card is rolled~~ Decided in Prompt 42 (user): the Legendary ability chance first, else an upgrade by
  AppearanceWeight across all three categories, then its rarity from its category's weights.
- ~~Whole-unit upgrades: is Rare left out of their roll, or does a Rare roll become Common?~~ Decided in Prompt 42
  (user): a Rare roll counts as Common.
- Can an offer show the same upgrade twice at different rarities? Today an offer never repeats an upgrade. (42)
- With fewer than 3 weapons owned, can an offer hold several weapon cards? Does a reroll of the first level-up's offer
  keep a weapon card? (42)
- Weapon power from Rare picks has halves (+0.5, +1.5): does a half point carry over to the next step, or count as
  half a step? (41)
- The Floating Gun's steps: step 1 = the second gun, then +10% each (assumed above)? (41)
- Powder Keg's double-size blast: double radius only, or double damage too? Loaded Dice's triple damage: ×3 in place of
  CritMultiplier ×2 (assumed above)? (44)
- ~~The fixed XP amount per level.~~ Decided in Prompt 45 (user): 100 XP for level 2, then +30 for each level after
  that.
- Does the player's Fire Rate speed up weapons? (Still open from Prompt 38; today it doesn't.)
- Do owned weapons also show in the upgrade icon row, or only in the weapon slots? (43)
- With more Legendary abilities later: which one a Legendary card shows.

**Docs changed**
- CLAUDE.md: the game summary (categories, rarity per card, at most 3 weapons, no pick caps, endless levels) and a new
  **Level-up upgrades** rule (rarities, weights, Legendary abilities, no caps, weapons and the rules for every weapon).
- This file: the game summary, the intro and the step table, this entry, a note on **Design update — M2b upgrades**,
  the M2b heading, a known issue, **Next up** (Prompts 40–53) and **Decided (user)**.

## M2c — Upgrade rework (Prompts 40–48)

Prompt 40 is the design update above.

### Prompt 41 — Upgrade data and per-pick values
How a pick is stored and applied. The offer roll and the cards are unchanged until Prompt 42, so **every offered pick
is still granted as Common**, and a Common pick gives exactly what a pick gave before.
- **`UGSUpgradeData`** (new fields, set on all 15 `DA_Upgrade_*` with MCP):
  - `Category` (`EGSUpgradeCategory`: StatBoost, Ability, Weapon), `bLegendaryAbility`, `AppearanceWeight` (1, used
    from Prompt 42), `BaseValue` (the value of one Common pick, in the effect's units) and `bWholeUnits`.
  - `WeaponGivingPicks` (weapons only): how many first picks give the weapon itself. Floating Gun 2 (its two guns),
    default 1.
  - The fixed `Rarity` and `MaxStacks` stay for now: the offer roll still uses the rarity (42), and MaxStacks still
    caps picks (44).
  - `IsPercentUpgrade()`, `ToDisplayValue`, `FormatValue` ("+25%", "+3"), `GetCategoryName`, `ParseRarity`.
    `GetPerStackValue()` now shows BaseValue (×100 for percent upgrades), so every card text is unchanged.

  | Category | Upgrades | BaseValue |
  |---|---|---|
  | Stat Boost | Damage, Reload Speed (0.10); Fire Rate, Move Speed (0.08); Magnet, Bigger Mags (0.15) — all percent; Loaded Dice (0.05 crit chance) | as listed |
  | Ability, whole units | Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech | 1 |
  | Ability | Powder Keg (1 stack = 3%; not whole units, so a Rare pick is 1.5 stacks = 4.5%) | 1 |
  | Legendary ability | Second Wind | 1 (one charge per pick, whatever the rarity) |
  | Weapon | Floating Gun | 1 point of power |
- **Pick values** (`UGSUpgradePool`, DA_UpgradePool; class defaults, so the asset didn't change):
  `RarityValueMultipliers` 1 / 1.5 / 2 / 3 and `WholeUnitValues` Common 1, Epic 2, Legendary 3 (no Rare).
  `GetPickValue(Upgrade, Rarity, PicksBefore)`:
  - Legendary ability: BaseValue; whole units: BaseValue × WholeUnitValues (a Rare pick is refused, `HasRarity`);
  - weapon: BaseValue × (multiplier − 1) for the first `WeaponGivingPicks` picks, then BaseValue × multiplier;
  - otherwise BaseValue × multiplier.
- **`UGSUpgradeComponent`:**
  - `GrantUpgrade(Upgrade, Rarity, Source)`; the old `GrantUpgrade(Upgrade, Source)` is a Common pick. Offered picks
    (`ChooseUpgrade`) use it.
  - Each entry (`FGSUpgradeStack`) keeps `Stacks` (= picks), `TotalValue` and every pick (`FGSUpgradePick`: rarity,
    value). New `GetTotalValue`, `GetPicks`, `FindEntry`.
  - **One active effect per upgrade:** the first pick applies the upgrade's GE, later picks update its SetByCaller
    values in place (`UpdateActiveGameplayEffectSetByCallerMagnitudes`). The attribute goes straight to the new
    total and never dips, so shield cooldowns, dash charges and the guns keep their state.
  - `RemoveStackByEffect` (a used Second Wind) takes back the last pick: its value comes off the total, and at 0 picks
    the effect and the entry go.
  - Log: `[upgrade] <PS>: DA_Upgrade_Damage x2 (Debug, Epic +20%, total +40%) -> <card text>`.
- **Upgrade effects** (`UGSUpgradeEffect`): no static magnitudes and no stacking any more. New SetByCaller tags:
  - `Data.UpgradeMultiplier` = 1 + total (the Multiply (Additive) modifier of percent upgrades: base × (1 + total));
  - `Data.UpgradeValue` = total (Additive modifiers);
  - `Data.UpgradePicks` = the number of picks.
  - The `GE_Upgrade_*` Blueprints inherit the new modifiers (checked on GE_Upgrade_Damage); no asset change.
- **Floating Gun:**
  - `FloatingGunLevel` is now the number of picks (Data.UpgradePicks): picks 1 and 2 give the two guns.
  - New attribute **`FloatingGunPower`** (total value; not replicated, `TODO(co-op)`) gives the +damage / +fire rate
    steps: picks 1 and 2 add multiplier − 1, later picks the multiplier. Fractions count (power 1.5 = +15%).
  - `DA_FloatingGunSettings`: `DamageBonusPerLevel` / `FireRateBonusPerLevel` renamed to `DamageBonusPerPower` /
    `FireRateBonusPerPower` (the asset used the class defaults). `GetDamageMultiplier(Power)`,
    `GetFireRateMultiplier(Power)`, `GetShotInterval(Power)`; `GetBonusLevels` is gone. Common picks give the same
    numbers as before (5 picks = power 3 = ×1.3, 1.95 shots a second).
  - `UGSFloatingGunComponent::GetPower`, `AGSFloatingGun::GetPower`; the `[gun]` line logs each power change.
- **Powder Keg:** the stacks aren't rounded any more (`GetExplodeChance(float)`), so a Rare pick (1.5) adds 4.5%.
  The `[blast]` line prints fractional stacks.
- **Console:**
  - `gs.Upgrade.Grant <Name> [Count] [Rarity]` (both optional, any order: `Damage 2 Epic`, `Damage Epic`, `L` for
    Legendary). A whole-unit upgrade at Rare is refused with a warning.
  - `gs.Upgrade.List` groups the upgrades by category (Stat Boosts, Abilities, Legendary abilities, Weapons). Each line
    shows the picks by rarity, the total value, the Common pick's value, "whole units" and a cap when there is one.
    The Stats line adds FloatingGunPower, the Floating Gun line shows picks and power.
- **Tests:**
  - New `Upgrades.PickValues` (all 15 upgrades).
  - `Upgrades.StatUpgrades`: one active GE per upgrade instead of N stacks; total value; the capped test upgrade
    gets a BaseValue.
  - `Upgrades.FloatingGunLevels`: power instead of levels (power 0 / 1 / 1.5 / 3), and power 0 after two Common picks,
    3 after five.
  - `Upgrades.Offers`' test upgrades get a BaseValue. Every other test passed unchanged (Common = the old pick).
- **PIE check** (town, god mode, round paused): `gs.Upgrade.Grant Damage 2 Epic` → Damage 1.40;
  `ExtraJump Legendary` → ExtraJumps 3; `DashCharge Rare` → refused ("has no Rare version"); `FloatingGun 1 Rare` then
  `FloatingGun Legendary` → two guns, power 2.5, ×1.25 damage, 1.88 shots a second; `Magnet` (Common) → PickupRadius
  345. `gs.Upgrade.List` showed the four groups and the per-rarity picks.
- **Known issue (cosmetic):** after a refused grant, `gs.Upgrade.Grant` also prints "x0 Rare granted (now 0 pick(s))".
- **Full suite:** 48 tests, all passing, in 113.0 s.

### Prompt 42 — Offer roll and cards
- **The roll** (`UGSUpgradePool::RollOffer` → `TArray<FGSUpgradeChoice>`, each card an upgrade and its rolled rarity;
  run stream as before). For each of the `ChoicesPerOffer` 3 cards:
  1. Roll `LegendaryAbilityChance` (0.008). On a hit the card is a Legendary ability not yet in the offer, picked by
     `AppearanceWeight`, at rarity Legendary.
  2. Otherwise a stat boost, ability or weapon not yet in the offer, by `AppearanceWeight`, then its rarity from its
     category's weights: `StatBoostRarityWeights` and `WeaponRarityWeights` 100 / 20 / 6 / 1, `AbilityRarityWeights`
     100 / 10 / 3 / 0.5 (placeholders, DA_UpgradePool class defaults).
  - **A whole-unit upgrade has no Rare: a Rare roll counts as Common** (`ResolveRarity`). Decided (user).
  - A hit with no Legendary ability left falls back to step 2, and step 2 with nothing left falls back to a Legendary
    ability. Upgrades at MaxStacks are still never offered (until Prompt 44).
  - The old single `RarityWeights` map and the fixed `UGSUpgradeData::Rarity` are gone.
- **Offers** (`UGSUpgradeComponent`): the open offer is `TArray<FGSUpgradeChoice>` (`GetCurrentOffer()`;
  `GetCurrentChoices()` still returns just the upgrades). `GetChoiceValue(Choice)` is what taking a card would add.
  **`ChooseUpgrade` grants the card's rolled rarity.** Rerolls, the queue and events are unchanged. The `[upgrade]
  offer opened` line lists each card as "DA_Upgrade_Damage (Common Stat Boost: +10% damage, now +15% damage, x1)".
- **Card text in data** (`UGSUpgradeData`, set on all 15 assets with MCP):
  - `ValueText`: what a value means, with plurals ("+{Value}% damage", "+{Value} {Value}|plural(one=dash
    charge,other=dash charges)"). `FormatValueText(Value)` fills it; `FormatValue` (logs, console) uses it too.
  - `DisplayScale` (0 = automatic: 100 for percent upgrades, 1 otherwise): Loaded Dice 100, Powder Keg 3 (= ChancePerStack
    in percent), Floating Gun 10 (= DamageBonusPerPower in percent). Values are shown rounded to 0.1 ("+22.5%").
  - **Descriptions rewritten without numbers** (they never contradict the pick line): e.g. Damage "Every hit from your
    weapons does more damage.", Floating Shield "A shield circles you and blocks one whole hit, then comes back after a
    cooldown." Floating Gun's three step texts are "Adds a floating gun over your right shoulder…", "Adds a second
    floating gun, over your left shoulder." and "Makes both floating guns stronger."
- **WBP_UpgradeCard** (MCP; `UGSUpgradeCardWidget::SetChoice(Upgrade, Rarity, PickValue, OwnedPicks, OwnedTotal, Index)`):
  - Top to bottom: key hint, **category** ("STAT BOOST", grey, 12, new `CategoryText`), icon (now 64 px), name,
    **rarity** (the card's rolled rarity, in its color; the frame too), **pick line** (new `PickText`, gold 22: "+15%
    damage", "+2 dash charges"; hidden for a Common pick that only gives a weapon or one of its guns), description (15),
    **"Now: +20% damage"** or "Now: none" (`OwnedText` renamed `NowText`). The card is 480 px tall (was 440).
  - Test hooks: `GetRarity`, `GetCategoryText`, `GetPickText`, `GetNowText`.
- **HUD and run summary icons:** still "xN" = picks; the frame takes the rarest pick so far
  (`FGSUpgradeStack::GetBestRarity`).
- **Simulator:** `UGSDebugLibrary::SimulateUpgradeOffers` and `simulate_upgrade_offers` add cards per upgrade, cards and
  offers per category ("Stat Boost", "Ability", "Legendary ability", "Weapon") and each category's rarity split.
- **Simulation, 10,000 offers, seed 1** (30,000 cards, no short offers):

  | Category | Cards | Offers with one | Common | Rare | Epic | Legendary |
  |---|---|---|---|---|---|---|
  | Stat Boost (7) | 49.8% | 90.3% | 78.6% | 15.9% | 4.7% | 0.78% |
  | Ability (6) | 42.4% | 84.4% | 95.4% | 1.5% (Powder Keg only) | 2.7% | 0.44% |
  | Legendary ability (Second Wind) | 0.77% | 2.3% | | | | 100% |
  | Weapon (Floating Gun) | 7.0% | 21.1% | 79.5% | 15.5% | 4.5% | 0.57% |

  - Each of the 14 other upgrades is on 6.9–7.3% of cards (20.7–21.9% of offers); Second Wind on 0.77%.
  - By rarity: Common 85.2% of cards, Rare 9.6%, Epic 3.8%, Legendary 1.4% (Second Wind included). An offer has a Rare
    card in 26.2% of offers, an Epic in 10.9%, a Legendary in 4.1%.
- **Not built here** (Next up's Prompt 42 text listed them; the prompt didn't): `MaxWeapons` and the first level-up's
  weapon card. With one weapon in the pool they change nothing yet; they move to Prompt 43.
- **Tests:**
  - `Upgrades.Offers` rewritten for the per-card roll: category weights and the 0.8% chance; 30,000 offers checked
    for 3 different cards, Legendary ability cards at 0.8% (always Legendary), even AppearanceWeight spread, each
    category's rarity split and no Rare whole-unit card; forced Legendary (chance 1 / 0); Rare-only weights giving
    Rare vs Common to whole units; per-category weights; AppearanceWeight 0; a pick gets the card's rarity and value; the
    same seed gives the same offer with rarities; simulator categories.
  - `UI.UpgradeChoice`: every card's category, rarity, pick and Now lines, frame color and description; the pick has the
    card's rarity.
  - `Upgrades.PickValues`: every upgrade's Common and Legendary card line (plurals) and number-free descriptions.
  - Upgrade tests check the category and the Common card line instead of the fixed rarity and the old description;
    `Upgrades.PowderKeg` and `Upgrades.FloatingGunLevels` check DisplayScale against their settings.
  - `Run.NewRunResets` picks a card that can't stop its killing blow (it had picked a Floating Shield, which blocked it,
    as designed).
- **PIE check** (town, round paused): a Rare Damage pick, then `gs.XP.Add 100` opened Floating Shield / Damage / Dash
  Charge (all Common). The cards showed "ABILITY · Floating Shield · COMMON · +1 floating shield · … · Now: none",
  "STAT BOOST · Damage · COMMON · +10% damage · … · Now: +15% damage" and "+1 dash charge". `gs.Upgrade.Choose 1` took
  Damage to +25%.
- **Environment:** Windows Smart App Control (switched to On) blocked the freshly built game DLL ("Bad Image",
  0xc0e90002). The user turned it off; see Environment gotchas.
- **Full suite:** 48 tests, all passing, in 114.1 s.

### Prompt 43 — Weapon slots
- **`DA_UpgradePool.MaxWeapons` 3** (class default). `UGSUpgradePool::CountOwnedWeapons` / `CanOfferWeapon`: a weapon the
  player doesn't own is a candidate only while they own fewer than MaxWeapons different weapons; owned weapons always
  stay candidates (as upgrades). The rule is inside `RollOffer`, so offers and rerolls both follow it. The rifle and
  pistol aren't upgrades and don't count.
- **The first offer of a run has a weapon card:** `RollOffer(…, bWeaponCard)` rolls one weapon first (by
  AppearanceWeight, its rarity from the Weapon weights as usual), fills the other cards as usual (they may hold
  another weapon) and puts the weapon at a random position. `UGSUpgradeComponent::IsFirstOfferPending()` is set on a
  new run and cleared by the first pick, so **rerolls of the first offer keep a weapon card** (a reroll can't skip it).
  With no weapon allowed (e.g. MaxWeapons 0) the offer is normal.
- **Card label** (`UGSUpgradeCardWidget::GetCategoryLabel`): a weapon card's category line reads "NEW WEAPON" when the
  player doesn't own it and "WEAPON UPGRADE" when they do.
- **HUD weapon slots** (WBP_HUD, MCP): a new HorizontalBox `TopLeftIconsRow` at the old spot (40, 128) holds
  `WeaponSlotsBox` (new binding) and then `UpgradeIconsBox`, 14 px apart.
  - MaxWeapons slots, built from WBP_UpgradeIcon in a weapon-slot mode (`UGSUpgradeIconWidget::SetWeaponSlot`): icon
    only (no count, no name), a neutral frame (`WeaponSlotColor` with a weapon, a dim `EmptyWeaponSlotColor` when
    empty), no rarity colors. The weapons fill the slots in the order they were first picked
    (`UGSUpgradeComponent::GetOwnedWeapons`).
  - **Weapons no longer appear in the upgrade icon row.** The slots are hidden in the hub with the other combat
    elements and empty again on a new run.
  - Test hooks: `GetWeaponSlotCount`, `GetWeaponInSlot`, `GetWeaponSlot`; icon `IsWeaponSlot`, `IsIconShown`.
- **Run summary** (WBP_RunSummary, MCP): a "Weapons" row after the seed (`WeaponsValue`: "Floating Gun", or "None");
  weapons leave the summary's upgrade icons and "Upgrades picked" text (`GetWeaponsText`).
- **Debug:** `gs.Upgrade.List` adds "Weapon slots (MaxWeapons 3): Floating Gun, -, -; first offer's weapon card done".
- **Tests** (`AGSGameMode::SetUpgradePoolOverride(Pool)`, new, lets a test use a copy of DA_UpgradePool):
  - New `Upgrades.WeaponSlots`.
  - `UI.RoundHUD` picks a non-weapon card from the first offer (it checks the upgrade icon row); `Run.GameOver` expects
    weapons in the summary's weapons line, not its icons; `UI.UpgradeChoice` and `Upgrades.FloatingGunLevels` expect the
    new weapon labels.
- **PIE check** (town, round paused): a Damage pick, then `gs.XP.Add 100`. The first offer showed Leech (Common),
  Loaded Dice (Epic, "+10% critical hit chance") and "NEW WEAPON · Floating Gun · COMMON". The HUD showed three empty
  slots before the Damage icon. After `gs.Upgrade.Choose 2` the first slot showed the weapon icon in a light frame,
  slots 2 and 3 stayed empty, and Damage stayed in the upgrade row.
- **Full suite:** 49 tests, all passing, in 114.3 s.

### Prompt 44 — Overflow effects and no pick caps
- **No pick caps.** `UGSUpgradeData::MaxStacks` and `CanAddStack` are removed, along with their checks in the roll and in
  `GrantUpgrade`. Loaded Dice (was 20) and Powder Keg (was 34) were the only capped upgrades; their assets were
  re-saved.
- **Loaded Dice past 100%** (`GSCombat::RollCriticalHit`; `CritChance` is now only clamped to >= 0):
  - From 100% every hit crits. `CritChance − 1` is the chance that the crit is a **triple hit**
    (`DA_CombatSettings.TripleCritMultiplier` 3, new) instead of double: 130% = every hit crits, 30% of them triple.
    From 200% every hit is triple.
  - The triple roll is a second roll on the run stream, only when a crit chance is past 100%, and none from 200%. Runs
    below 100% use the stream exactly as before.
  - `FGSCritRoll::bTriple` / `GetLevel()` (0 / 1 / 2). Damage specs carry `Data.Critical` = 1 (crit) or 2 (triple).
    The attacker's damage-dealt client call now passes that level instead of a bool (`ClientNotifyDamageDealt(…,
    uint8 CritLevel, …)`: a changed parameter on an existing call, no new RPC).
  - **Triple damage numbers:** "60!!", size 36 (crit 28, normal 20), orange-red (1, 0.35, 0.08) (`TripleCritFontSize`
    / `TripleCritColor` on WBP_HUD). Test hooks: `GetTripleDamageNumbersAdded`, `GetLastDamageNumberCritLevel`.
- **Powder Keg past 100%:** every kill explodes, and the chance past 100% is rolled per blast
  (`AGSGameMode::RollPowderKegDoubleSize`, run stream; no roll below 100% or from 200%) for a **double-size blast**:
  `BlastRadius × DoubleBlastRadiusMultiplier` (2, new; 6 m), the same damage, the explosion effect at scale 2.
  - `UGSCombatSettings::GetRawExplodeChance` / `GetDoubleBlastChance`.
  - The `[blast]` line says "DOUBLE SIZE, radius 600".
  - `GetDoublePowderKegBlasts` / `GetLastPowderKegRadius` (reset on a new run).
  - Note: 34 stacks is 102% (34 × 3%), so the old "MaxStacks 34" already had a 2% double-size chance.
- **Chances snap to 0 and 1** within 0.0001 (`GSCombat::SnapChance`): 20 × 0.05 sums to 1.0000001 in float, which would
  otherwise roll a 0.00001% triple. Exactly 100% (and 200%) stay sure things that need no dice.
- **Cards and `gs.Upgrade.List` show the overflow** (`UGSUpgradeData::OverflowText` + `FormatNowText`, data):
  - Loaded Dice: "Now: +130% critical hit chance (30% of crits deal triple damage)".
  - Powder Keg: "Now: +120% chance that a kill explodes (20% of blasts are double size)".
  - `gs.Upgrade.List` prints the same totals, and new lines: "Loaded Dice: 130.0% crit chance (x2.0); past 100%:
    every hit crits, 30.0% of them triple (x3.0)" and "Powder Keg: … past 100%: every kill explodes, 20.0% of blasts
    double size (6.0 m), N so far".
  - Their descriptions mention it without numbers ("Once every hit is critical, crits can deal triple damage.").
- **WBP_UpgradeCard:** the Now line wraps (it ran across the next cards in the PIE check), description 14 and pick line
  20 pt, the card 540 px tall.
- **`AGSGameMode::SetCombatSettingsOverride`** (tests; new). The GameMode's own reads go through `GetCombatSettings()`.
- **Very high stat values** (no limits added). Checked in code and in PIE (`gs.SetAttr`, god mode):

  | Stat | Value tried | Result |
  |---|---|---|
  | Fire Rate | ×100 (rifle 60,000 rounds a minute) | Works: 85 shots in 6 frames (0.094 s), about 900 a second, frame avg 8.9 ms at 112 fps (one 51 ms spike). The burst fires several shots per frame on the exact schedule (`Weapons.HeldFire` covers that). Limits: at most 256 shots per frame (`MaxShotsPerUpdate`), so the rate tops out at 256 × fps (rifle FireRate ~2,500 at 60 fps); recoil is applied per shot, so at high rates the view climbs fast; every shot traces and plays its impact effect and sound, so the cost grows with the rate. A 30-round magazine is gone in 0.03 s. |
  | Reload Speed | ×1000 | Works: the reload ends the next frame (1.8 ms). Only the attribute's minimum (0.1) is clamped. |
  | Move Speed | ×33 (30,000 cm/s) | MaxWalkSpeed follows. A 300 m/s launch into the east wall stopped at x = 14,956.75 (wall face minus the capsule radius): no tunnelling through the boundary. **Design snags** (the slide and dash part is fixed in Prompt 44b: both scale with Move Speed now): the slide starts at most at `DA_SlideSettings.MaxSpeed` 1,600 cm/s, so once walking is faster (after about 11 Common Move Speed picks: 900 × 1.88 = 1,692) a slide slows the player down; the dash (fixed 4,000 cm/s, then `ExitSpeed`) does too, past about 44 picks. Acceleration stays 6,000 cm/s², so top speed takes v / 6,000 s to reach (5 s at 30,000). |
  | Magnet | 100,000 cm (1 km) | Works: a 10-kill wave's drops all came in within about 2 s; the check is one distance test per pickup per tick. **Quirk:** a pickup more than `MaxMagnetDistance` 100 m from its target is paid at once (a rule for teleports), so with a radius over 100 m, far drops pay out instantly instead of flying. |
  | Bigger Mags | ×1000 | Works: a 30,000-round magazine, the reload filled it, the HUD read "29,705 / 30,000". The int magazine size would only overflow past a ×70,000,000 multiplier (hundreds of millions of picks). |

  - Also seen: MCP's `Input.+key` releases itself after a few frames, so held-input checks from MCP read short bursts.
- **Tests:**
  - `Upgrades.LoadedDice`: 100% every hit crits, never triple, no dice; 21+ picks granted; 150%: every hit crits,
    ~50% triple (×3, level 2), replayable from the stream; 200%: every hit triple, no dice; the overflow text; a real
    rifle hit at 200% deals 60 with a triple number.
  - `Upgrades.PowderKeg`: raw and double-size chances; overflow text; the chain part runs on a settings copy with a
    same-size double blast (34 stacks = 102%); 67 stacks → a 6 m blast with the same damage, no dice; 40 stacks →
    ~20% of 1,000 size rolls double, replayable.
  - `Upgrades.StatUpgrades`: 50 more Damage picks are all granted (Damage 6.3). `Upgrades.Offers`: an upgrade with
    1,000 picks is still offered. Every "has no cap" check is gone, and the card test reads the Now line with overflow.
  - First run: `Upgrades.LoadedDice` missed its first rifle shot right after an editor start (passed on the next runs;
    like the known first-shot flake in `Balance.TimeToKill`).
- **PIE check** (town, god mode): Loaded Dice ×26 and Powder Keg ×40 → `gs.Upgrade.List` printed both overflow lines.
  Two `gs.KillAll` waves: 14 blasts with no double one (P ≈ 4%), then 3 DOUBLE SIZE blasts of 6 m in the next 20. A
  rerolled offer showed the Powder Keg card's wrapped Now line (above), next to a Legendary Second Wind.
- **Full suite:** 49 tests, all passing, in 114.0 s.

### Prompt 44b — Fix: slide and dash scale with Move Speed
Fixes the Move Speed snag found in Prompt 44: the slide and the dash used fixed speeds, so once Move Speed picks made
running faster than them, sliding and dashing slowed the player down.
- **Speed ratio** (`AGSCharacter::GetMoveSpeedRatio`): the MoveSpeed attribute ÷ `DA_PlayerMovementSettings.MaxWalkSpeed`,
  never below 1 (also 1 without an ASC or settings). It is read when a slide or a dash starts and kept for that slide or
  dash (`GetSlideSpeedRatio` / `GetDashSpeedRatio`), so a pick in the middle of one doesn't change it.
- **Slide:** SpeedBoost, MaxSpeed and EndSpeed × the ratio (`UGSSlideSettings::GetSpeedBoost / GetMaxSpeed / GetEndSpeed`).
  MinStartSpeed, Duration, the decay curve, steering, the slide-hop multiplier and the cooldown are unchanged.
- **Dash:** Distance and ExitSpeed × the ratio (`UGSDashSettings::GetDistance / GetExitSpeed`; `GetDashSpeed(Ratio)` =
  scaled distance ÷ Duration). Duration, the invulnerability, the charges and the recharge are unchanged, so a dash still
  takes 0.15 s, covers more ground at a higher Move Speed and ends at the running speed (ExitSpeed 900 = the base walk
  speed, × the ratio).
- DA_SlideSettings and DA_DashSettings keep their values as the base; the scaling is done where they are read. At the
  base Move Speed the ratio is exactly 1 and every number is as before.
- **`gs.Upgrade.List`:** the Stats line ends with "speed ratio x2.20, slide cap 3520 cm/s, dash distance 1320 cm"
  (new `UGSGameplayAbility_Slide::FindGrantedSlide` / `GetSettings`, like the dash's).
- **Measured** (`Upgrades.MoveSpeedSlideDash`, real slide and dash from a full run on the ring road):

  | Move Speed | Slide (run → start → end) | Dash |
  |---|---|---|
  | ×1 (900) | 900 → 1,500 → 600 cm/s | 600 cm in 0.158 s, exit 900 cm/s |
  | ×2 (1,800) | 1,800 → 3,000 → 1,200 cm/s | 1,200 cm in 0.158 s, exit 1,800 cm/s |
  | ×33 (29,700) | start 49,500 cm/s into the east wall: stopped at x = 14,956.6 | 19,800 cm (198 m) into the east wall: stopped at x = 14,956.8 (wall at 15,000) |

- **×33 against town cover** (`Upgrades.MoveSpeedCover`: a dash from 6 m out and a slide from a full run into a crate,
  a low wall, a shack, a tower and the silo, at ×1, ×5 and ×33, sampling every frame in the piece's own frame):
  - **Nothing passes through.** No frame had the capsule inside a piece, the player never left the ground (top Z 98–100),
    and wherever it ended up behind a piece it had gone round it first.
  - **Head-on, flat faces stop it dead** (the silo at yaw 0: the ×33 dash stopped at 557 cm, its face at 558).
  - **Turned cover deflects it.** Most cover is turned a few degrees (jitter), so the dash or slide slides along the
    face, round the corner and keeps going: the ×33 dash into the shack (yaw −18°) ended 184 m further on and 67 m to the
    side; the ×33 slide past the tower went 127 m. At ×1 the same hits stop at the face or slide off by under 70 cm. Not
    limited (as asked); see Known issues.
- **Tests:** new `Upgrades.MoveSpeedSlideDash` and `Upgrades.MoveSpeedCover`. Every existing test passed unchanged.
  - Two test-only fixes while building: the slide test started a slide in the frame the player landed from the teleport
    (it ended at once); both tests now wait until the player stands. The cover test's first "through" check used
    axis-aligned bounds, which flagged skidding round turned cover; it now works in each piece's own frame.
- **PIE check** (town, god mode, round paused): `gs.Upgrade.Grant MoveSpeed 15` → MoveSpeed 1980, MaxWalkSpeed 1980,
  `gs.Upgrade.List` "speed ratio x2.20, slide cap 3520 cm/s, dash distance 1320 cm". `GSPress Ability.Dash` from the
  PlayerStart moved the player 13.8 m (the 13.2 m dash plus a short coast at the 1,980 cm/s exit speed).
- **Full suite:** 51 tests, all passing, in 122.6 s.

### Prompt 45 — Endless levels
- **No level cap.** `UGSXPCurve::MaxLevel` and the curve keys (`XPToReachLevel`) are gone. `DA_XPCurve` now has two
  numbers: **`BaseXP` 100** (level 1 → 2) and **`XPIncreasePerLevel` 30** (each later level needs that much more than
  the one before). `GetXPToReachLevel(L)` = 100 + 30 × (L − 2); `GetTotalXPToReachLevel(L)` is the series in closed form,
  so it is cheap at any level. The asset was re-saved with the two values (the old keys dropped out).
- **`UGSProgressionComponent`:** `FGSProgressionState::MaxLevel`, `GetMaxLevel` and `IsMaxLevel` are removed. There is
  always a next level when there is a curve (`XPForNextLevel` is 0 only without one, e.g. the hub), XP past a level
  always carries over, and a big gain levels up as many times as it covers. `GetLevelProgress` is 0 without a curve
  (was 1 at max level).
- **`gs.XP.SetLevel N`:** any N ≥ 1. XP amounts are int32, so above about level 11,960 (2.1 billion total XP) it uses
  the highest level that fits, with a warning.
- **Rewards and offers at any level:** unchanged code, now unbounded. Every level gained applies `DA_LevelRewards`
  (+10 MaxHealth) and one full heal follows, and every level queues an offer. Level 100: MaxHealth 1,090, 99 offers.
- **HUD:** the "MAX" XP text is gone; it is always "x / y XP" (with thousands separators). The level label is "LV N"
  ("LV 1,000" from four digits). New test hooks `GetLevelLabelText()` / `GetXPLabelText()`.
- **XP needed** (from the previous level / total from level 1): L2 100 / 100, L3 130 / 230, L4 160 / 390, L5 190 / 580,
  L6 220 / 800, L7 250 / 1,050, L8 280 / 1,330, L9 310 / 1,640, L10 340 / 1,980, L25 790 / 10,680, L50 1,540 / 40,180,
  L100 3,070 / 155,430.
- **Levels per cleared round** (every enemy killed and orb collected, XPGain 1, every round cleared from round 1;
  rounds 10 and 20 measured with seeds 2701 / 2702, the other generated rounds at the measured ~8.3 XP per budget point):

  | Round | Wave | XP | Level during the round | XP that level needs | Levels |
  |---|---|---|---|---|---|
  | 1 | 4 melee | 40 | 1 | 100 | 0.4 |
  | 5 | 9 melee, 4 ranged | 150 | 3.5 → 4.4 | 160–190 | ~0.9 |
  | 10 | 8 melee, 11 ranged (19) | 245 | 7.6 → 8.5 | 280–310 | ~0.8 |
  | 20 | 17–19 melee, 18–19 ranged (36–37) | 455–460 | 16.0 → 16.8 | 550 | ~0.85 |

  XP per round and XP per level both grow linearly, so a cleared round stays worth a little under a level. A run
  reaches level ~8.5 after round 10 and ~16.8 after round 20 (the old +25% curve: ~7.9 and ~12.7, and falling
  further behind every round).
- **Tests:** `Progression.XPAndLevels` reworked for the new curve (see the table; the old "cap at 50 / 40 offers" part
  is now "level 10 → 60 in one gain, 50 offers, the remainder carries over"). New `Progression.EndlessLevels`.
  `Upgrades.Offers`, `UI.UpgradeChoice` and `Run.NewRunResets` had hard-coded XP for the old curve (125 + 156, 1000,
  225); they now read the amounts from the curve and check the same things.
- **PIE check** (town, god mode, round paused, real profile): `gs.XP.SetLevel 100`, the 99 offers taken with
  `gs.Upgrade.Choose`, `gs.XP.Add 1535`. The HUD read "LV 100" over a half-full bar and "1,535 / 3,070 XP", and
  "1,090 / 1,090" health; the three-digit label fits with room to spare.
- **Full suite:** 52 tests, all passing, in 122.8 s.

### Prompt 46 — Razor Cards
The second weapon (M2c weapon rules). It throws playing cards from the player, one at a time, at the nearest living
enemy in range with a clear line; each card homes in on the enemy it was thrown at.
- **Homing on `AGSProjectile`** (`FGSProjectileLaunch::HomingTarget` / `HomingTurnRate`): each tick, before the sweep,
  the velocity turns toward the target's center by at most the turn rate, at the same speed. If the target dies or goes
  away first, the projectile drops it (`GetHomingTarget()` is null) and flies on straight until it hits something or its
  lifetime ends. Without a target nothing changes (the Floating Gun's bullets). Projectiles now also face their direction
  of travel every tick, and `SpinRate` spins them around their own up axis (cosmetic). New `GetLastHitActor()`.
- **`Combat/GSRazorCardsSettings.h` (`UGSRazorCardsSettings`) → `Data/DA_RazorCardsSettings`**, set as
  `RazorCardsSettings` on BP_GSGameMode (`AGSGameMode::GetRazorCardsSettings()`): Damage 6, CardsPerSecond 2, Range
  2,500 cm, TargetRefreshInterval 0.1 s, DamageBonusPerPower / FireRateBonusPerPower 0.1, DamageEffect GE_Damage,
  ThrowOffset (30, 0, 40); CardSpeed 3,000 cm/s, HomingTurnRate 720°/s, CardLifetime 1.25 s, CardRadius 10, PoolPrewarm 8;
  look: the engine cube at (0.25, 0.18, 0.01) with the new `Materials/MI_RazorCard` (an instance of M_XPOrb: white,
  intensity 1.5), CardSpinRate 1,080°/s, ThrowSound `VR_click3_Cue` (editor content), RadialBurst ×0.2 on impact.
- **`Combat/GSRazorCardsComponent.*` (`UGSRazorCardsComponent`, on `AGSCharacter`, `GetRazorCards()`)**: throws while
  the new attribute **`RazorCardsLevel`** (picks) is 1 or more; **`RazorCardsPower`** gives the +10% damage / fire rate
  per point (both not replicated, `TODO(co-op)`). No actor of its own: cards leave from 30 cm in front of and 40 cm above
  the capsule center.
  - Targeting: the nearest living `AGSEnemyCharacter` within Range of the throw point with a clear line (WorldStatic /
    WorldDynamic trace), every 0.1 s and before each card.
  - The exact schedule of the Floating Gun (Prompt 31b rules): a card is due one interval after the previous card's
    scheduled time, a long frame throws all it owes (a late card starts further along), nothing is banked without a
    target, a fire rate change re-times from the last card.
  - Every weapon rule: damage = Damage × the power bonus × the player's Damage, a crit roll per card, Piercing Rounds
    doesn't apply (a card stops at the first target), GE_Damage from the player's ASC (its kills are the player's), no
    cards while dead, in a safe zone or paused, no collision, a new run (level 0) stops it.
  - Log prefix `[cards]`: one line when the picks or power change ("Razor Cards 3 pick(s), power 3 -> x1.30 damage
    (7.8 per card), 2.60 cards/s").
- **Upgrade:** `UGSUpgradeEffect_RazorCards` → `Abilities/GE_Upgrade_RazorCards` (picks on RazorCardsLevel, total on
  RazorCardsPower). `Data/Upgrades/DA_Upgrade_RazorCards`: Weapon, BaseValue 1, WeaponGivingPicks 1, DisplayScale 10,
  ValueText "+{Value}% Razor Cards damage and fire rate"; card texts "New weapon: throws homing cards at the nearest
  enemy." (none owned) and "Makes Razor Cards stronger." (the pick line shows the "+10% Razor Cards damage and fire
  rate"). Added to DA_UpgradePool (16 upgrades; two weapons). It shows in the HUD's weapon slots and the run summary's
  weapons line like the Floating Gun.
- **Debug:** `gs.Upgrade.Grant RazorCards [N] [Rarity]`; `gs.Upgrade.List` adds RazorCardsLevel / RazorCardsPower to
  its Stats line and a line "Razor Cards: 1 pick(s), power 0, x1.00 damage (6.0 per card), 2.00 cards/s, 71 card(s)
  thrown, target GSEnemyCharacter_8".
- **Damage per second** (see Tuning values): one pick of either weapon is 12 DPS (Razor Cards 6 × 2; one Floating Gun
  8 × 1.5). The gun's second pick doubles it with the second gun (24), Razor Cards' second pick adds 10% (13.2); at
  power 3 Razor Cards does 20.3 and two guns 40.6. Razor Cards trades damage for never missing.
- **Tests:** new `Upgrades.RazorCards`. Measured: 4 cards in 2 s at power 0 (50 frames on schedule), 6.0 / 6.6 / 7.8
  per card at power 0 / 1 / 3, 5 cards in 2 s at power 3 (19.5 damage a second against one bandit), and 8 of 8 cards
  hit a bandit running past at 584 cm/s. `Upgrades.Offers` (16 upgrades, 15 even), `Upgrades.PickValues` (Razor Cards'
  values and card lines) and `Upgrades.WeaponSlots` (the first offer's weapon card can be either weapon; the slot-limit
  step leaves Razor Cards out of its pool copy so it has exactly two weapons) were updated.
- **PIE check** (town, god mode, round paused): `gs.Upgrade.Grant RazorCards`, `gs.Spawn MeleeBandit 3`: the cards killed
  all three before the next capture (the first capture caught the cards as white streaks next to the camera, the
  bandits behind the player). Four more bandits: two were at 12 HP (8 cards each) a few seconds later and the HUD read
  "2 COMBO" with their XP collected; the weapon slot showed the Razor Cards icon. `gs.Upgrade.List`: 71 cards thrown.
- **Full suite:** 53 tests, all passing, in 127.1 s.

### Prompt 47 — Barbed Wire
The third weapon (M2c weapon rules): a ring on the ground around the player that damages every living enemy inside it
every 0.5 s.
- **`Combat/GSBarbedWireSettings.h` (`UGSBarbedWireSettings`) → `Data/DA_BarbedWireSettings`**, set as
  `BarbedWireSettings` on BP_GSGameMode (`AGSGameMode::GetBarbedWireSettings()`): Radius 350 cm, MaxHeightDifference
  200 cm, Damage 5, TickInterval 0.5 s, DamageBonusPerPower 0.1, RadiusBonusPerPower 0.05, DamageEffect GE_Damage; look:
  RingSegments 48 of SegmentMesh (the engine cube) with the new `Materials/MI_BarbedWire` (an instance of M_XPOrb, grey,
  intensity 0.8), WireThickness 4 cm, HeightAboveFloor 3 cm, HitEffect RadialBurst at HitEffectScale ×0.06.
- **`Combat/GSBarbedWireComponent.*` (`UGSBarbedWireComponent`, on `AGSCharacter`, `GetBarbedWire()`)**: active while the
  new attribute **`BarbedWireLevel`** (picks) is 1 or more; **`BarbedWirePower`** gives +10% damage and +5% radius per
  point (both not replicated, `TODO(co-op)`).
  - **Inside:** an enemy's center within the radius of the player's center, measured flat, and no more than 2 m above or
    below. Walls don't matter.
  - **Ticks** on an exact schedule: the first one as soon as it is owned, then one every 0.5 s from it; a long frame runs
    every tick it owes; while dead or in a safe zone nothing is banked; a paused game doesn't tick it.
  - **Each tick**: everyone inside is collected first, then each takes GE_Damage from the player's ASC (Damage × the
    power bonus × the player's Damage, its own crit roll, a hit result at the enemy for the damage number) and gets the
    hit effect. Its kills are the player's.
  - **The ring**: a runtime `UInstancedStaticMeshComponent` (no collision, overlaps, navigation or shadow), placed at the
    feet every frame in world space (it doesn't turn with the player), rebuilt when the radius changes, hidden when not
    owned or dead. A new run (level 0) hides it and stops the ticks.
  - Log prefix `[wire]`: one line when the picks or power change ("Barbed Wire 11 pick(s), power 30 -> x4.00 damage
    (20.00 per tick), radius 875 cm (x2.50), every 0.50 s").
- **Card text with two numbers:** new `UGSUpgradeData::SecondaryDisplayScale` and a `{Value2}` placeholder in ValueText.
  `Data/Upgrades/DA_Upgrade_BarbedWire`: Weapon, BaseValue 1, WeaponGivingPicks 1, DisplayScale 10, SecondaryDisplayScale
  5, ValueText "+{Value}% Barbed Wire damage, +{Value2}% size" ("+10% Barbed Wire damage, +5% size" for a Common pick,
  "+15%, +7.5%" for a Rare one); card texts "New weapon: a ring around you that hurts every enemy inside it." and "Makes
  Barbed Wire stronger and bigger.". `UGSUpgradeEffect_BarbedWire` → `Abilities/GE_Upgrade_BarbedWire`. Added to
  DA_UpgradePool (17 upgrades, three weapons).
- **`M_XPOrb`** now has the "Used with Instanced Static Meshes" usage flag; the ring's material instance needs it (the
  first test run logged that the default material would be used).
- **Debug:** `gs.Upgrade.Grant BarbedWire [N] [Rarity]`; `gs.Upgrade.List` adds BarbedWireLevel / BarbedWirePower and a line
  "Barbed Wire: 11 pick(s), power 30, radius 875 cm, x4.00 damage (20.00 per tick x Damage), every 0.50 s, 18 tick(s),
  372 hit(s), 30 inside last tick".
- **Tests:** new `Upgrades.BarbedWire` (see the test table). `Upgrades.Offers` (17 upgrades, 16 even), `Upgrades.PickValues`
  (Barbed Wire's values and card lines) and `Upgrades.WeaponSlots` (any of three weapons in the first offer; the slot-limit
  step leaves Razor Cards and Barbed Wire out of its pool copy) were updated.
- **PIE check and frame time with 30 enemies inside** (town, god mode, round paused, editor throttling off, 120 fps
  cap): Barbed Wire with 10 Legendary picks (power 30, radius 8.75 m, so the whole crowd fits), the player's Damage set to
  0.01 so nobody died (each tick still hits all 30 and plays 30 hit effects), `gs.Spawn MeleeBandit 30`; once all 30 were
  within 2.7 m, `gs.Perf 5` twice, then a new run with the same 30 bandits and no ring as a control:

  | | Frame avg | p95 | Max | Game avg | Render avg | GPU avg |
  |---|---|---|---|---|---|---|
  | Ring, 30 inside (1st) | 8.95 ms (112 fps) | 10.08 | 115.66 | 8.54 | 4.46 | 4.82 |
  | Ring, 30 inside (2nd) | 8.53 ms (117 fps) | 9.36 | 34.12 | 7.97 | 4.57 | 4.86 |
  | No ring, same 30 (control) | 8.44 ms (119 fps) | 8.33 | 28.86 | 5.44 | 3.13 | 4.49 |

  The ring with 30 inside costs about 2.5 ms of game thread and 1.4 ms of render on average (30 damage specs, 30 damage
  numbers and 30 particle bursts every 0.5 s), and the frame stays at about 117 fps. The first sample's 116 ms spike
  didn't repeat (likely the first use of the hit effect at that scale). `gs.Upgrade.List`: 372 hits in 18 ticks, "30
  inside last tick". The screenshot showed the crowd inside the grey ring and the hit bursts on every bandit; the bursts
  filled the screen, so the hit effect scale was halved to ×0.06 afterwards.
- **Full suite:** 54 tests, all passing, in 129.5 s.

### Prompt 48 — Tests + M2b and M2c checkpoint
- **Test audit:** every rule in the prompt's list was checked against the suite. The rule → test map:

  | Rule | Tests |
  |---|---|
  | Every upgrade at Common / Rare / Epic / Legendary changes its stat by the right amount; whole units give 1 / 2 / 3 and never roll Rare; a new run resets everything | `Upgrades.PickValues`, `Upgrades.Offers` (no Rare whole-unit card in 30,000 offers), new `Checkpoint.WeaponPicksAndNewRun`, `Run.NewRunResets` |
  | Offers: three different cards, weights per category, the Legendary ability chance (always Legendary), the same seed gives the same offers and rerolls | `Upgrades.Offers`, `Run.SameSeedSameRun` |
  | Weapons: the first offer's weapon card, no unowned weapon with MaxWeapons owned (rerolls too), owned weapons still offered, HUD slots | `Upgrades.WeaponSlots`, new `Checkpoint.WeaponLimitAllThree` |
  | A new run has 1 dash charge; Bigger Mags rounds up; Leech never heals above MaxHealth | `Upgrades.DashCharge`, `Checkpoint.WeaponPicksAndNewRun`, `Upgrades.BiggerMagsAndLeech` |
  | Loaded Dice and Powder Keg at 0%, 100% and past 100%; the blast hurts enemies, never the player; a chain ends | `Upgrades.LoadedDice`, `Upgrades.PowderKeg`, new `Upgrades.PowderKegChainEnds` |
  | Piercing Rounds: up to N + 1 enemies in a line, each once, walls stop it | `Upgrades.PiercingRounds` |
  | Attack order: dash i-frames use no shield; a shield cancels a hit and comes back after its cooldown; Second Wind only on a killing hit, one charge each | `Abilities.DashInvulnerabilityBlocksDamage`, `Upgrades.FloatingShield`, `Upgrades.SecondWind` |
  | The three weapons: valid targets only, power scaling, kills are the player's | `Upgrades.FloatingGun`, `Upgrades.FloatingGunLevels`, `Upgrades.RazorCards`, `Upgrades.BarbedWire` |
  | Health orbs from weapon and Powder Keg kills at the same chance as own kills | new `Economy.HealthOrbsAllSources` |
  | Levels: no cap, a fixed step per level | `Progression.XPAndLevels`, `Progression.EndlessLevels` |

- **New tests (4):** `Checkpoint.WeaponPicksAndNewRun`, `Checkpoint.WeaponLimitAllThree`, `Upgrades.PowderKegChainEnds`
  and `Economy.HealthOrbsAllSources` (see the test table).
- **Damage dealt by source** (`UGSRunStats`; run state, reset with the run, not replicated, `TODO(co-op)`):
  - `EGSDamageSource`: Gun (rifle / pistol), FloatingGun, RazorCards, BarbedWire, PowderKeg, Other (`gs.KillAll`,
    `gs.Damage`, anything untagged).
  - Each damage spec carries a native tag `Damage.Source.Gun / FloatingGun / RazorCards / BarbedWire / PowderKeg`
    (dynamic asset tags; GA_Fire, the projectile through `FGSProjectileLaunch::DamageSource`, Barbed Wire and the Powder
    Keg blast set it). `UGSAttributeSet` reads it, and `AGSGameMode::HandleDamageApplied` passes it to
    `AddDamageDealt(Amount, Source)`.
  - Read access: `GetDamageDealtBy(Source)`, and per round in `FGSRoundStats::GetDamageBy`.
- **Health orbs per round:** `FGSRoundStats` counts the orbs dropped and collected and the health they healed;
  `UGSRunStats::GetHealthOrbsDropped()` gives the run total.
- **Log lines:**
  - `[round]` now ends with ", health orbs D dropped / C collected / H healed, damage dealt: rifle/pistol N, Floating
    Gun N, …".
  - "Run over in round N …" adds the damage dealt by source and the orbs.
  - `gs.Run.Stats` adds a "Damage by source" line and the orbs dropped.
- **`gs.Debug.AutoPickUpgrades 1`** (new cheat cvar, `UGSDebugLibrary::ScheduleAutoPickUpgrades` /
  `ChooseUpgradeForAutoPick`): with offers open, a frame later every open offer is taken by a fixed policy. That lets a
  playtest run from the console without stopping at the choice screen. The policy: an unowned weapon while a slot is
  free, else an owned weapon, else the rarest card. An offer with only Common cards and no weapon is rerolled first
  while rerolls last. `AGSGameMode::UpdateUpgradeChoicePause` schedules it; it does nothing at 0.
- **Simulation** (`simulate_upgrade_offers(10000, 1)`, the 17-upgrade pool; 30,000 cards, no short offers):

  | Upgrade | Cards | Offers | | Upgrade | Cards | Offers |
  |---|---|---|---|---|---|---|
  | Move Speed | 6.41% | 19.2% | | Damage | 6.19% | 18.6% |
  | Extra Jump | 6.41% | 19.2% | | Bigger Mags | 6.17% | 18.5% |
  | Powder Keg | 6.39% | 19.2% | | Leech | 6.16% | 18.5% |
  | Fire Rate | 6.33% | 19.0% | | Dash Charge | 6.14% | 18.4% |
  | Piercing Rounds | 6.25% | 18.8% | | Reload Speed | 6.13% | 18.4% |
  | Magnet | 6.24% | 18.7% | | Razor Cards | 6.08% | 18.2% |
  | Barbed Wire | 6.22% | 18.7% | | Loaded Dice | 5.96% | 17.9% |
  | Floating Shield | 6.21% | 18.6% | | Floating Gun | 5.94% | 17.8% |
  | Second Wind | 0.77% | 2.3% | | | | |

  | Category | Cards | Offers with one | Common | Rare | Epic | Legendary |
  |---|---|---|---|---|---|---|
  | Stat Boost (7) | 43.4% | 84.6% | 78.5% | 15.9% | 4.6% | 0.90% |
  | Ability (6) | 37.6% | 78.3% | 95.3% | 1.5% (Powder Keg only) | 2.7% | 0.48% |
  | Legendary ability (Second Wind) | 0.77% | 2.3% | | | | 100% |
  | Weapon (3) | 18.2% | 48.1% | 79.3% | 16.0% | 4.2% | 0.53% |

  - The weights work out as planned: Stat Boosts and Weapons at 78.7 / 15.7 / 4.7 / 0.79% (100 / 20 / 6 / 1), Abilities at
    95.3% Common because a whole unit's Rare counts as Common.
  - By rarity: Common 84.4% of cards (in 99.6% of offers), Rare 10.4% (28.1% of offers), Epic 3.8% (10.9%), Legendary
    1.44% (4.3%, Second Wind included).

#### M2b + M2c checkpoint — two full loops in PIE
Hub → run → death → hub, twice, and then a third Start Run to see it start fresh. The seeds were set in the hub
(`gs.Run.Seed 4801` / `4802`) and each run started with `gs.Market.Return` (the portal's path). Editor throttling was
off; the real profile slot was used.
- **How it was played** (as in Prompt 28, MCP can't react in real time):
  - Cleared rounds: god mode on. The weapons acted on their own. Each script pass aimed a rifle burst at the nearest
    enemy, and every second pass sent `gs.KillAll` once the whole wave had spawned. Some rounds the weapons cleared
    alone before a KillAll pass came.
  - The death round: god mode off, standing still, no KillAll.
  - Upgrades: `gs.Debug.AutoPickUpgrades 1` (policy above), so the picks went to weapons whenever one was offered.
  - Read the numbers with that in mind:
    - "Combat" is spawn time plus command delay, not kill speed.
    - `gs.KillAll` damage is "other".
    - Health stayed full, so the orbs healed nothing.
    - The rifle numbers are low because `Input.+key` releases fire after a few frames.

**Loop 1 (seed 4801): died in round 5, 1:38 run time**

| Round | Wave | Combat | Cash collected + bonus | XP | Level after | Health orbs (dropped / collected / healed) | Damage dealt | Offer → pick |
|---|---|---|---|---|---|---|---|---|
| 1 | 4 melee | 28.1 s | $46 + $35 (missed fast clear) | 40 | 1 | 0 / 0 / 0 | rifle 120, KillAll 240 | — |
| 2 | 5 melee, 1 ranged | 12.8 s | $79 + $50 | 65 | 2 | 0 / 0 / 0 | KillAll 380 | Damage (Rare) / Barbed Wire (Common) / Loaded Dice (Common) → **Barbed Wire, Common** |
| 3 | 6 melee, 2 ranged | 8.9 s | $117 + $50 | 90 | 2 | 1 / 1 / 0 | Barbed Wire 155, KillAll 385 | — |
| 4 | 7 melee, 3 ranged | 11.4 s | $145 + $50 | 115 | 3 | 3 / 3 / 0 | rifle 146, Barbed Wire 240, KillAll 336 | Floating Gun (Common) / Extra Jump (Common) / Move Speed (Epic) → **Floating Gun, Common** |
| 5 | 9 melee, 4 ranged | died after 6.4 s (120 damage) | — | — | 3 | 0 / 0 / 0 | Floating Gun 72, Barbed Wire 75 | — |

Totals: 4 rounds cleared, level 3, 29 kills, best combo 8, $572 earned (balance $7,309 → $7,881), 0 rerolls used, 0
fallback spawns. Weapons held: Barbed Wire (power 0), Floating Gun (one gun, power 0). Damage over the run: rifle 266,
Barbed Wire 470, Floating Gun 72, `gs.KillAll` 1,340 (2,148 in total). Health orbs: 4 dropped, 4 collected, 0 healed.
The summary showed these numbers ("Weapons: Barbed Wire, Floating Gun"; "Upgrades picked: None", since only weapons
were taken).

**Loop 2 (seed 4802): died in round 8, 3:13 run time**

| Round | Wave | Combat | Cash collected + bonus | XP | Level after | Health orbs (dropped / collected / healed) | Damage dealt | Offer → pick |
|---|---|---|---|---|---|---|---|---|
| 1 | 4 melee | 16.9 s | $43 + $35 (missed fast clear) | 40 | 1 | 0 / 0 / 0 | rifle 60, KillAll 180 | — |
| 2 | 5 melee, 1 ranged | 13.4 s | $79 + $50 | 65 | 2 | 1 / 1 / 0 | KillAll 380 | Magnet (Common) / Piercing Rounds (Common) / Floating Gun (Common) → **Floating Gun, Common** |
| 3 | 6 melee, 2 ranged | 7.2 s | $117 + $50 | 90 | 2 | 2 / 2 / 0 | Floating Gun 80, KillAll 460 | — |
| 4 | 7 melee, 3 ranged | 13.2 s | $165 + $50 | 115 | 3 | 2 / 2 / 0 | Floating Gun 150, KillAll 572 | Leech (Common) / Move Speed (Common) / Floating Gun (Rare) → **Floating Gun, Rare** (second gun, power 0.5) |
| 5 | 9 melee, 4 ranged | 39.2 s (the guns alone) | $150 + $35 (missed fast clear) | 150 | 4 | 1 / 1 / 0 | Floating Gun 1,045 | Piercing Rounds (Common) / Move Speed (Common) / Floating Gun (Rare) → **Floating Gun, Rare** (power 2: ×1.2, 1.8 shots/s each) |
| 6 | generated, 12 | 32.2 s (the guns alone) | $168 + $35 (missed fast clear) | 160 | 5 | 2 / 2 / 0 | Floating Gun 922 | Extra Jump / Razor Cards / Floating Shield (all Common) → **Razor Cards, Common** |
| 7 | generated, 15 | 11.2 s | $259 + $50 | 185 | 6 | 2 / 2 / 0 | Floating Gun 360, Razor Cards 119, KillAll 825 | Floating Shield / Damage / Razor Cards (all Common) → **Razor Cards, Common** (power 1) |
| 8 | generated, 17 | died after 8.0 s (147 damage) | $20 | 20 | 6 | 0 / 0 / 0 | Floating Gun 265, Razor Cards 101 | — |

Totals: 7 rounds cleared, level 6, 70 kills, best combo 12, $1,306 earned (balance $7,881 → $9,187), 0 rerolls used, 0
fallback spawns. Weapons held: Floating Gun (two guns, power 2), Razor Cards (power 1). Damage over the run: Floating Gun
2,821, Razor Cards 220, rifle 60, `gs.KillAll` 2,416 (5,518 in total). Health orbs: 10 dropped, 10 collected, 0 healed.
The summary matched ("Weapons: Floating Gun, Razor Cards").

- **After loop 2:** "Return to Market" (a Slate button click) opened the hub with $9,187, level 1, 100 / 100 health and
  no weapons. `gs.Market.Return` then started a third run (random seed) at round 1, level 1, with no weapons, shields or
  dash charges past the base, and the cash kept.
- **Spawns:** 0 fallbacks in all 13 waves. In the 11 cleared waves the closest spawn was 12.3–18.5 m (ring 12–25 m),
  and none was on-screen.
- **Frame time** (`gs.Perf 5` right after each death, the enemies still alive and running their AI, the weapons idle
  because the player was dead; 120 fps cap):

  | Loop | Enemies alive | Frame avg | p95 | Max | Game | Render | GPU |
  |---|---|---|---|---|---|---|---|
  | 1 (round 5) | 12 | 8.43 ms (119 fps) | 8.33 | 52.28 | 3.49 | 2.77 | 3.42 |
  | 2 (round 8) | 13 | 8.36 ms (120 fps) | 8.33 | 21.82 | 3.51 | 2.79 | 3.88 |

  The weapons' own cost was measured in their prompts (Barbed Wire with 30 inside: about +2.5 ms game thread).
- **Observations** (suggestions only, nothing changed):
  - **The Floating Gun carries a run.** Two guns at power 2 cleared rounds 5 and 6 on their own while the player stood
    in god mode, and dealt 51% of loop 2's damage (87% of what wasn't `gs.KillAll`). Barbed Wire at power 0 (470 over
    loop 1) and Razor Cards at power 0–1 (220 over two rounds) did far less. The Floating Gun's second pick doubles it
    (the second gun) where the others gain 10%. A balance pass should look at that.
  - **Weapons alone don't keep a still player alive.** Loop 1 died in round 5 in 6.4 s (two weapons at power 0) and
    loop 2 in round 8 in 8.0 s (four weapon picks), each to 120–147 damage while standing still.
  - **Levels:** loop 1 levels 1, 2, 2, 3 after rounds 1–4; loop 2 levels 1, 2, 2, 3, 4, 5, 6 after rounds 1–7, about one
    a round from round 4 (Prompt 45's estimate: a little under one per cleared round).
  - **Offers:** 7 offers, all with a weapon card (the first is guaranteed; the other six came at the normal 48%, about a
    1.2% streak). 5 of the 21 cards were not Common: Damage Rare, Move Speed Epic and Floating Gun Rare ×3.
  - **Health orbs:** 14 dropped from 99 kills (14%; the chance is 20%), all collected, 0 healed, because god mode kept
    health full and the vacuum at full health heals nothing. The death rounds had too few player kills to drop one.
    How much they heal in a real fight is still untested.
  - **Fast clear** was missed in round 1 of both loops (17–28 s against 14 s) and in loop 2's rounds 5–6 (the guns alone
    took 32–39 s against 26–27.5 s).
  - **Round 1 of loop 1 counted 5 kills for a wave of 4.** It may have been a target dummy hit by the rifle; not looked
    into.
- **Full suite:** 58 tests, all passing, in 131.7 s.

## Weapons — the last four (Prompts 49–53)

The four remaining weapons of the M2c design, each its own prompt, then a weapons checkpoint (53). Every one follows
**Rules for every weapon** (M2c design update) and is built like Razor Cards and Barbed Wire.

### Prompt 49 — Dynamite
Every 4 s it lobs a stick of dynamite at the enemy in range with the most other enemies around it; the stick explodes
where it lands.
- **New: thrown weapon actors** (shared with the Molotov and later weapons):
  - `Combat/GSThrownActor.*` (`AGSThrownActor`): a pooled, abstract actor with a mesh and no collision, hidden and not
    ticking while pooled. It ticks with the world, so it pauses with the game. Not replicated (`TODO(co-op)`).
  - `Combat/GSThrownSubsystem.*` (`UGSThrownSubsystem`, world subsystem): the pool. `Acquire<T>()`, `Release`,
    `ReleaseAll`, `GetActive`, `GetNumActive(Class)`. `AGSGameMode::StartRun` calls `ReleaseAll()` next to the projectile
    pool's, so a new run drops everything in the air.
  - `Combat/GSLobbedThrow.*` (`AGSLobbedThrow`): flies a fixed arc from a start to a landing point over a flight time
    (apex `ArcHeight` above the straight line), tumbling, with no collision (it goes over cover). When it lands it goes
    back to the pool and then calls its `OnLanded` callback. A late throw starts that far along the arc. Dropped by a new
    run, it never lands.
  - **Chosen over `AGSProjectile`**, which sweeps and stops at the first target or wall: a lob has to pass over cover and
    land at a set point at a set time.
- **`Combat/GSDynamiteSettings.h` (`UGSDynamiteSettings`) → `Data/DA_DynamiteSettings`**, set as `DynamiteSettings` on
  BP_GSGameMode (`AGSGameMode::GetDynamiteSettings()`): Damage 40, BlastRadius 300 cm, ThrowInterval 4 s, Range 2,000 cm,
  TargetRefreshInterval 0.1 s, DamageBonusPerPower 0.1, RadiusBonusPerPower 0.05, DamageEffect GE_Damage, ThrowOffset
  (20, 0, 60); FlightTime 0.6 s, ArcHeight 300 cm; look: the engine cylinder at (0.08, 0.08, 0.3) with the new
  `Materials/MI_Dynamite` (an instance of M_XPOrb, red), tumbling 720°/s, ThrowSound `VR_click3_Cue`, BlastEffect
  `SimpleExplosion` scaled to radius / `BlastEffectRadius` 300, BlastSound `VR_negative_Cue` (the Powder Keg's; editor
  content).
- **`Combat/GSDynamiteComponent.*` (`UGSDynamiteComponent`, on `AGSCharacter`, `GetDynamite()`)**: active while the new
  attribute **`DynamiteLevel`** (picks) is 1 or more; **`DynamitePower`** gives +10% damage and +5% blast radius per point
  (both not replicated, `TODO(co-op)`).
  - **Target** (picked at the throw): among living enemies within 20 m of the player, the one with the most other living
    enemies within the current blast radius of it (`CountNeighbours`); the nearest one on a tie. No clear line is needed.
  - **Schedule** like the other weapons: a throw is due one interval after the last throw's scheduled time; a long frame
    throws all it owes; while no enemy is in range (checked every 0.1 s), dead or in a safe zone nothing is banked, and the
    first throw comes at once when an enemy is in range again (0.03 s in the test). Power doesn't change the interval.
  - **The stick** lands at the target's feet where it stood when thrown, 0.6 s later. **The blast** is centered on the
    target's body (its position at the throw) and hits every living enemy within the blast radius, 3D distance, no line
    of sight, like Powder Keg. Players are never hit. Each enemy hit gets GE_Damage from the player's ASC with its own
    crit roll (Loaded Dice), a hit result for its damage number and the new `Damage.Source.Dynamite` tag. The damage and
    the radius are set at the throw.
  - Its kills are the player's (cash, XP orb, health orb chance, combo, Leech, Powder Keg, run-stats kills).
  - Test hooks: `GetThrows`, `GetBlasts`, `GetLastBlastHits`, `GetHitsDealt`, `GetLastBlastCenter/Radius`, `GetLastLanding`,
    `GetLastTarget`, `GetBurstStartTime`, `GetBurstShots`, `Explode`, `FindTarget`, `CanThrow`.
  - Log prefix `[dynamite]`: one line when the picks or power change, one per blast ("blast 3 at …: radius 300, damage
    40.0, 3 enemies hit").
- **Run stats:** `EGSDamageSource::Dynamite` ("Dynamite" in the `[round]` line, "Run over" and `gs.Run.Stats`).
- **Upgrade:** `UGSUpgradeEffect_Dynamite` → `Abilities/GE_Upgrade_Dynamite`. `Data/Upgrades/DA_Upgrade_Dynamite`: Weapon,
  BaseValue 1, WeaponGivingPicks 1, DisplayScale 10, SecondaryDisplayScale 5, ValueText "+{Value}% Dynamite damage,
  +{Value2}% blast size"; card texts "New weapon: lobs dynamite at the biggest group of enemies nearby." and "Makes
  Dynamite stronger and its blast bigger.". Added to DA_UpgradePool (18 upgrades, four weapons).
- **Debug:** `gs.Upgrade.Grant Dynamite [N] [Rarity]`; `gs.Upgrade.List` adds DynamiteLevel / DynamitePower and a line
  "Dynamite: 1 pick(s), power 0, x1.00 damage (40.0 per blast x Damage), blast radius 300 cm, every 4.00 s, 3 thrown, 3
  blast(s), 7 hit(s), 3 hit by the last blast".
- **Damage per second** (Damage 1, no crits; measured in the test): **10 against one enemy** (40 every 4 s; 120 in 3
  blasts), **50 against a group of five** within 3 m (600 in 3 blasts). Next to the others at power 0: Floating Gun 12 on
  one target (24 with both guns), Razor Cards 12 on one target, Barbed Wire 10 on every enemy inside (50 for five).
- **Tests:** new `Upgrades.Dynamite` (see the test table). Changed, because a fourth weapon made them untrue:
  - `Upgrades.Offers`: 18 upgrades instead of 17, and "the 17 other upgrades come up evenly" instead of 16.
  - `Upgrades.PickValues`: Dynamite's expectation and its card lines; "all 18 upgrades checked".
  - `Upgrades.WeaponSlots`: its two-weapon pool copy also leaves Dynamite out. Its "later offers are rolled as usual"
    check was `Without > 5` of 20 offers with a comment of "about 79% have none"; with four weapons that share is about
    42%, and with seven it is about 27%, where a fixed 5 of 20 would start failing. It now rolls 200 later offers and checks
    that the share without a weapon card is within 10 points of the share the pool gives (42.1% expected, 40.0% measured).
  - `Checkpoint.WeaponPicksAndNewRun`: Dynamite's power after each rarity (0 / 1.5 / 3.5 / 6.5); before the new run
    Dynamite is owned, after it it is gone and nothing thrown is left in play.
  - `Checkpoint.WeaponLimitAllThree`: a new first step with the **real** pool. Three of its weapons owned, 60 offers and
    their rerolls never offer the unowned real weapon (Dynamite), and the owned ones are still offered (31 cards). The
    test-weapon steps are unchanged.
- **Decided here, to confirm:**
  - The blast is centered on the target's body where it stood at the throw (Powder Keg also measures from the body), and
    the stick lands at its feet. A bandit that keeps walking can leave the blast: a melee bandit covers about 3 m in the
    0.6 s flight.
  - A stick already in the air when the player dies still explodes (like the Floating Gun's bullets in flight). A new run
    drops it without exploding.
  - The neighbour count uses the current blast radius (it grows with power).
  - The damage and radius are set at the throw; the target is picked at the throw.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after): `gs.Upgrade.Grant Dynamite`,
  `gs.Spawn MeleeBandit 5`. The weapon slot showed the Dynamite icon. Blasts came 4.0 s apart: 2, 2, 3 and 3 enemies hit.
  `gs.Upgrade.List` printed the Dynamite line above. (A first try sent the commands too late: round 1 killed the player,
  and the dead player threw nothing, as designed.)
- **Full suite:** 59 tests, all passing, in 154.4 s.

### Prompt 50 — Lucky Coin
Every 1.5 s it flips a coin at the nearest enemy; the coin bounces from enemy to enemy, never hitting one twice in a
flip.
- **`AGSProjectile`, two optional launch fields** (the Floating Gun and Razor Cards don't set them; their tests pass
  unchanged):
  - `IgnoredTargets`: living targets the projectile flies through, like corpses.
  - `OnFinished(Reason, HitActor, Where)`: called once when the flight ends by a hit, a wall or the end of its lifetime,
    after the damage and after the projectile went back to the pool (so it can launch the next one). A new run's
    `ReleaseAll` doesn't call it. `EGSProjectileEnd` moved above `FGSProjectileLaunch` in `GSProjectile.h`.
- **`Combat/GSLuckyCoinSettings.h` (`UGSLuckyCoinSettings`) → `Data/DA_LuckyCoinSettings`**, set as `LuckyCoinSettings` on
  BP_GSGameMode (`AGSGameMode::GetLuckyCoinSettings()`): Damage 10, FlipInterval 1.5 s, Range 2,000 cm, BounceRange 800 cm,
  BaseBounces 3, PowerPerBounce 2, DamageBonusPerPower 0.1, TargetRefreshInterval 0.1 s, DamageEffect GE_Damage,
  ThrowOffset (30, 0, 40); CoinSpeed 2,500 cm/s, HomingTurnRate 1,440°/s, CoinLifetime 1 s, CoinRadius 12 cm, PoolPrewarm 6;
  look: the engine cylinder at (0.12, 0.12, 0.02) with the new `Materials/MI_LuckyCoin` (an instance of M_XPOrb, gold),
  spinning 1,440°/s, FlipSound `VR_click1_Cue` (editor content), RadialBurst ×0.15 on impact. `GetBounces(Power)` =
  BaseBounces + floor(power / PowerPerBounce).
- **`Combat/GSLuckyCoinComponent.*` (`UGSLuckyCoinComponent`, on `AGSCharacter`, `GetLuckyCoin()`)**: active while the new
  attribute **`LuckyCoinLevel`** (picks) is 1 or more; **`LuckyCoinPower`** gives +10% damage per point and a bounce per 2
  full points (both not replicated, `TODO(co-op)`).
  - **Target**: the nearest living enemy within 20 m of the throw point with a clear line (Razor Cards' rule). Exact
    schedule like the other weapons; no target, dead or a safe zone: nothing banked. Power doesn't change the interval.
  - **A flip**: a homing coin (`AGSProjectile`, `Damage.Source.LuckyCoin`) at the target. When it hits, the flip
    records the enemy and, while bounces are left, launches the next coin from the hit point at the nearest other living
    enemy within 8 m of the one just hit, with a clear line, that this flip hasn't hit (`FindBounceTarget`). Every coin of
    the flip flies through the enemies already hit, so no enemy is hit twice. The flip ends when the bounces run out,
    nobody is in reach, a coin misses (wall or lifetime), or the player can't act any more.
  - Every hit: GE_Damage from the player's ASC with its own crit roll; the damage is set at the flip. Its kills are the
    player's.
  - Test hooks: `GetFlips`, `GetFlipsDone`, `GetFlipsInAir`, `GetHitsDealt`, `GetLastFlipHits` (in order),
    `GetLastFlipDamage`, `GetBurstStartTime`, `GetBurstShots`, `FindTarget`, `FindBounceTarget`, `CanFlip`.
  - Log prefix `[coin]`: one line when the picks or power change ("Lucky Coin 3 pick(s), power 2 -> x1.20 damage (12.0
    per hit), 4 bounce(s) (up to 5 hits a flip), every 1.50 s").
- **Run stats:** `EGSDamageSource::LuckyCoin` ("Lucky Coin").
- **Upgrade:** `UGSUpgradeEffect_LuckyCoin` → `Abilities/GE_Upgrade_LuckyCoin`. `Data/Upgrades/DA_Upgrade_LuckyCoin`: Weapon,
  BaseValue 1, WeaponGivingPicks 1, DisplayScale 10, ValueText "+{Value}% Lucky Coin damage"; card texts "New weapon:
  flips a coin that bounces from enemy to enemy." and "Makes the Lucky Coin stronger and, in time, adds bounces.". Added
  to DA_UpgradePool (19 upgrades, five weapons).
- **Debug:** `gs.Upgrade.Grant LuckyCoin [N] [Rarity]`; `gs.Upgrade.List` adds LuckyCoinLevel / LuckyCoinPower and a line
  "Lucky Coin: 1 pick(s), power 0, x1.00 damage (10.0 per hit x Damage), 3 bounce(s) (up to 4 hits a flip), every 1.50 s,
  7 flip(s), 22 hit(s), 4 hit by the last flip, target GSEnemyCharacter_3".
- **Damage per second** (Damage 1, no crits; measured in the test): **6.7 against one enemy** (10 every 1.5 s; 30 in 3
  flips), **26.7 against five** standing 4 m apart in a row (four hit each flip; 120 in 3 flips). It needs a group within
  8 m of each other to bounce; at power 2 a flip hits all five (40 DPS at 12 per hit).
- **Tests:** new `Upgrades.LuckyCoin` (see the test table). Changed, because a fifth weapon made them untrue:
  - `Upgrades.Offers`: 19 upgrades, "the 18 other upgrades come up evenly".
  - `Upgrades.PickValues`: Lucky Coin's expectation and card lines; "all 19 upgrades checked".
  - `Upgrades.WeaponSlots`: its two-weapon pool copy also leaves Lucky Coin out.
  - `Checkpoint.WeaponPicksAndNewRun`: Lucky Coin's power after each rarity; owned before the new run, gone after it.
- **Decided here, to confirm:**
  - A bounce needs a clear line from the enemy just hit, like the first flip.
  - A coin hits the first living enemy it touches (like the other projectiles), which can be an enemy in front of its
    target; the chain goes on from that one.
  - A bounce's distance is measured from the enemy just hit; the next coin starts at the hit point.
  - The flip's damage is set when it is flipped; bounces use it too.
  - "Every 2 full points": power 3.5 gives 4 bounces (floor), as the prompt says.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after): `gs.Upgrade.Grant LuckyCoin`,
  `gs.Spawn MeleeBandit 5`. In about 10 s: 7 flips, 22 hits, the last flip hit 4; two bandits died ("2 COMBO", "+$20",
  "+10 XP" twice) and the weapon slot showed the Lucky Coin icon.
- **Full suite:** 60 tests, all passing, in 161.6 s.

### Prompt 51 — Molotov
Every 5 s it throws a bottle at the nearest enemy; where it lands it leaves a patch of fire that burns for 4 s.
- **Throw:** the Dynamite's `AGSLobbedThrow` (0.6 s arc, no collision, over cover) at the nearest living enemy within
  15 m, no clear line needed. It lands at the target's feet where it stood when thrown. Exact schedule like the other
  weapons; nothing banked with no enemy in range, dead or in a safe zone (the throw came 0.03 s after the bandit walked
  into range in the test).
- **New: `Combat/GSFirePatch.*` (`AGSFirePatch`)**, an `AGSThrownActor` in the `UGSThrownSubsystem` pool (so a new run
  removes every patch): a flat disc of the engine cylinder scaled to the radius with the new `MI_MolotovFire` (orange,
  glowing), no collision.
  - It burns on an exact schedule of its own age: the first burn when it lands, then every 0.5 s while its age is below
    4 s, so 8 burns; then it goes back to the pool. It ages with the world tick, so it pauses with the game.
  - Each burn hits every living enemy whose center is within the radius of the patch center (flat) and no more than 2 m
    above or below, like Barbed Wire's "inside": GE_Damage from the player's ASC with its own crit roll, a hit result
    for the damage number and the new `Damage.Source.Molotov` tag. Players are never hit (the player stood in the fire
    with god mode off in the test).
  - A burn is skipped while the player is dead or in a safe zone (`UGSMolotovComponent::CanBurn`); the slot is used, not
    banked.
  - The damage and radius are set at the throw (`FGSFirePatchParams`).
- **`Combat/GSMolotovSettings.h` (`UGSMolotovSettings`) → `Data/DA_MolotovSettings`**, set as `MolotovSettings` on
  BP_GSGameMode (`AGSGameMode::GetMolotovSettings()`): ThrowInterval 5 s, Range 1,500 cm, TargetRefreshInterval 0.1 s,
  FlightTime 0.6 s, ArcHeight 300 cm, ThrowOffset (20, 0, 60); BurnDamage 4, BurnInterval 0.5 s, PatchDuration 4 s,
  PatchRadius 250 cm, MaxHeightDifference 200 cm, DamageBonusPerPower 0.1, RadiusBonusPerPower 0.05, DamageEffect GE_Damage;
  look: the bottle is the engine cylinder at (0.08, 0.08, 0.22) with `MI_MolotovBottle` (dark green), tumbling 540°/s;
  the patch is the engine cylinder, 2 cm thick, 2 cm above the floor, with `MI_MolotovFire`; ThrowSound `VR_click3_Cue`,
  LandSound `VR_negative_Cue` (editor content).
- **`Combat/GSMolotovComponent.*` (`UGSMolotovComponent`, on `AGSCharacter`, `GetMolotov()`)**: active while the new
  attribute **`MolotovLevel`** (picks) is 1 or more; **`MolotovPower`** gives +10% burn damage and +5% patch radius per
  point (both not replicated, `TODO(co-op)`). `IgnitePatch` lights a patch (a landing; the debug command). Test hooks:
  `GetThrows`, `GetPatchesLit`, `GetBurns`, `GetHitsDealt`, `GetActivePatches`, `GetLastLanding`, `GetLastTarget`,
  `GetBurstStartTime`, `GetBurstShots`, `FindTarget`, `CanThrow`, `CanBurn`. Log prefix `[molotov]`: one line when the
  picks or power change, one per patch lit ("patch 5 at …: radius 612, 0.16 per burn every 0.50 s for 4.0 s, 26
  enemies in it").
- **Run stats:** `EGSDamageSource::Molotov` ("Molotov").
- **Upgrade:** `UGSUpgradeEffect_Molotov` → `Abilities/GE_Upgrade_Molotov`. `Data/Upgrades/DA_Upgrade_Molotov`: Weapon,
  BaseValue 1, WeaponGivingPicks 1, DisplayScale 10, SecondaryDisplayScale 5, ValueText "+{Value}% Molotov burn damage,
  +{Value2}% patch size"; card texts "New weapon: throws a bottle that leaves a patch of fire on the ground." and "Makes
  the Molotov's fire stronger and its patch bigger.". Added to DA_UpgradePool (20 upgrades, six weapons).
- **Debug:** `gs.Upgrade.Grant Molotov [N] [Rarity]`; **`gs.Molotov.Patches [Count=4]`** (new; lights patches 4 m around
  the player at the current power, for the frame-time check); `gs.Upgrade.List` adds MolotovLevel / MolotovPower and a
  line "Molotov: … patch radius …, every 5.00 s, N thrown, N patch(es) lit, N burning now, N burn(s), N hit(s)".
- **Damage per second** (Damage 1, no crits; measured in the test): at power 0 each enemy that stays in the patch takes
  32 per patch (8 burns of 4), so **6.4 against one enemy** (8 while it burns) and **32 against five** in the patch. At
  power 3 the test measured 41.6 per patch: 8.3 against one, 41.6 against five.
- **Frame time, 4 patches and 30 enemies** (PIE, town, god mode, round paused, editor throttling off, 120 fps cap): 30
  melee bandits around the player, Molotov at power 29 (10 Legendary picks: a 6.1 m patch, so every patch covers the
  crowd) with the player's Damage at 0.01 so nobody died, `gs.Molotov.Patches 4` (plus the Molotov's own patch every 5 s:
  4–5 patches burning, 25–30 enemies in each), `gs.Perf 3`, twice; then a new run (`gs.Run.Seed`) with the same 30
  bandits and no Molotov:

  | | Frame avg | p95 | Max | Game avg | Render avg | GPU avg |
  |---|---|---|---|---|---|---|
  | 4–5 patches, 30 enemies (1st) | 8.90 ms (112 fps) | 8.33 | 107.44 | 5.62 | 3.02 | 4.34 |
  | 4–5 patches, 30 enemies (2nd) | 8.39 ms (119 fps) | 8.33 | 26.65 | 4.91 | 2.99 | 4.38 |
  | No Molotov, same 30 (control) | 8.38 ms (119 fps) | 8.33 | 25.76 | 4.73 | 2.93 | 4.43 |

  About 0.2–0.9 ms more game thread on average (every patch damages up to 30 enemies every 0.5 s: about 240 damage specs
  and damage numbers a second with 4 patches). The first sample's 107 ms spike didn't repeat (the first damage numbers
  and patches of that size).
- **Tests:** new `Upgrades.Molotov` (see the test table). Changed, because a sixth weapon made them untrue:
  - `Upgrades.Offers`: 20 upgrades, "the 19 other upgrades come up evenly".
  - `Upgrades.PickValues`: Molotov's expectation and card lines; "all 20 upgrades checked".
  - `Upgrades.WeaponSlots`: its two-weapon pool copy also leaves Molotov out.
  - `Checkpoint.WeaponPicksAndNewRun`: Molotov's power after each rarity; owned before the new run, gone after it.
  - First run of the new test: four checks missed by 0.01–0.03, which is float rounding on 100,000 HP test bandits
    (eight burns of 4.4 / 5.2); their tolerance is 0.1 (0.5 for the sum of five). The burns themselves were right.
- **Decided here, to confirm:**
  - The first burn is when the bottle lands, then every 0.5 s: 8 burns in 4 s.
  - "Standing in the patch" = the enemy's center within the radius, flat, and within 2 m up or down (like Barbed Wire).
  - A burning patch skips its burns while the player is dead or in a safe zone (the weapon doesn't act then); a bottle
    already in the air still lands and lights its patch.
  - Damage and radius are set when the bottle is thrown.
  - Added `gs.Molotov.Patches` to measure the 4-patch frame time.
- **PIE check:** the frame-time run above. The patches showed as flat orange discs under the crowd, a damage number on
  every bandit each burn; the `[molotov]` lines showed a patch every 5.0 s with all 30 enemies in it.
- **Full suite:** 61 tests, all passing, in 173.7 s.

### Prompt 52 — Tomahawk
Every 3 s it throws a tomahawk at the nearest enemy; it cuts through every enemy on its line, out and back.
- **New: `Combat/GSTomahawk.*` (`AGSTomahawk`)**, an `AGSThrownActor` in the `UGSThrownSubsystem` pool (a new run removes
  the tomahawks in the air):
  - **Out:** it flies level (horizontally, at the throw height) toward where its target was, at `Speed` 20 m/s for
    `Distance` 18 m, sweeping a 30 cm sphere against world geometry and pawns. Every living enemy it touches is hit once
    on the way out. The first wall (world geometry) turns it back on the spot.
  - **Back:** it homes on the player's current position, sweeping pawns only, so it flies through walls and always comes
    home; every living enemy it touches is hit once on the way back. It is caught within 1 m of the player (and gone after
    `MaxFlightTime` 4 s whatever happens).
  - Every hit: GE_Damage from the player's ASC with its own crit roll, a hit result for its damage number, the new
    `Damage.Source.Tomahawk` tag and the hit effect. Players are never hit. It ticks with the world, so it pauses with the
    game.
- **`Combat/GSTomahawkSettings.h` (`UGSTomahawkSettings`) → `Data/DA_TomahawkSettings`**, set as `TomahawkSettings` on
  BP_GSGameMode (`AGSGameMode::GetTomahawkSettings()`): Damage 12, ThrowInterval 3 s, Range 1,800 cm,
  TargetRefreshInterval 0.1 s, BaseTomahawks 1, PowerPerTomahawk 3, DamageBonusPerPower 0.1, DamageEffect GE_Damage,
  ThrowOffset (30, 0, 40); Distance 1,800 cm, Speed 2,000 cm/s, SweepRadius 30 cm, CatchDistance 100 cm, MaxFlightTime
  4 s; look: the engine cone at (0.3, 0.05, 0.3) with the new `Materials/MI_Tomahawk` (an instance of M_XPOrb, grey),
  spinning end over end 900°/s, ThrowSound `VR_click3_Cue` (editor content), RadialBurst ×0.1 on every hit.
  `GetTomahawks(Power)` = BaseTomahawks + floor(power / PowerPerTomahawk).
- **`Combat/GSTomahawkComponent.*` (`UGSTomahawkComponent`, on `AGSCharacter`, `GetTomahawk()`)**: active while the new
  attribute **`TomahawkLevel`** (picks) is 1 or more; **`TomahawkPower`** gives +10% damage per point and a tomahawk per 3
  full points (both not replicated, `TODO(co-op)`).
  - **Targets**: living enemies within 18 m of the throw point with a clear line, nearest first (`FindTargets`). Each
    tomahawk of a throw goes at the next one; with fewer targets than tomahawks they go round the targets again.
  - Exact schedule like the other weapons; no target, dead or a safe zone: nothing banked. Power doesn't change the
    interval. The damage is set at the throw.
  - Test hooks: `GetThrows`, `GetTomahawksThrown`, `GetHitsDealt`, `GetLastTargets`, `GetActiveTomahawks`,
    `GetBurstStartTime`, `GetBurstShots`, `FindTargets`, `CanThrow`. Log prefix `[tomahawk]`: one line when the picks or
    power change ("Tomahawk 3 pick(s), power 3 -> x1.30 damage (15.6 per hit), 2 tomahawk(s) a throw, every 3.00 s").
- **Run stats:** `EGSDamageSource::Tomahawk` ("Tomahawk").
- **Upgrade:** `UGSUpgradeEffect_Tomahawk` → `Abilities/GE_Upgrade_Tomahawk`. `Data/Upgrades/DA_Upgrade_Tomahawk`: Weapon,
  BaseValue 1, WeaponGivingPicks 1, DisplayScale 10, ValueText "+{Value}% Tomahawk damage"; card texts "New weapon: throws
  a tomahawk that cuts through enemies and comes back." and "Makes the Tomahawk stronger and, in time, adds tomahawks.".
  Added to DA_UpgradePool (21 upgrades, all seven weapons).
- **Debug:** `gs.Upgrade.Grant Tomahawk [N] [Rarity]`; `gs.Upgrade.List` adds TomahawkLevel / TomahawkPower and a line
  "Tomahawk: 1 pick(s), power 0, x1.00 damage (12.0 per hit x Damage), 1 tomahawk(s) a throw, every 3.00 s, 4 throw(s), 4
  tomahawk(s) thrown, 1 in the air, 7 hit(s)".
- **Damage per second** (Damage 1, no crits; measured in the test): **8 against one enemy** (12 out + 12 back every 3 s;
  48 in 2 throws), **40 against a line of five** 3 m apart (240 in 2 throws). Off the line it does nothing: a crowd
  around the player is hit only where the tomahawk's line crosses it.
- **Unity-build fix:** the Dynamite and Molotov components (and this one) each had an anonymous-namespace constant named
  `MaxThrowsPerFrame`; with the new files the unity build put them in one translation unit, so they are now
  `MaxDynamiteThrowsPerFrame`, `MaxMolotovThrowsPerFrame` and `MaxTomahawkThrowsPerFrame` (no behaviour change).
- **Tests:** new `Upgrades.Tomahawk` (see the test table). Changed, because a seventh weapon made them untrue:
  - `Upgrades.Offers`: 21 upgrades, "the 20 other upgrades come up evenly".
  - `Upgrades.PickValues`: Tomahawk's expectation and card lines; "all 21 upgrades checked".
  - `Upgrades.WeaponSlots`: its two-weapon pool copy also leaves Tomahawk out. Its "later offers" share check (Prompt 49)
    failed with seven weapons: 37.0% weapon-free against 25.1% expected. The expected share assumed no weapons owned, but
    an offer of three weapons makes the "first non-weapon card" pick a weapon, and with three owned only those three are
    offered. The expected share is now worked out per offer from the weapons that can be offered at that moment (36.0%
    expected, 37.0% measured). The ±10-point tolerance is unchanged.
  - `Checkpoint.WeaponPicksAndNewRun`: Tomahawk's power after each rarity; owned before the new run, gone after it.
- **Decided here, to confirm:**
  - The tomahawk flies level at the throw height (the target's height is ignored), so it doesn't dive into the ground at
    a target standing lower or rise over one standing higher.
  - On the way back it flies through walls and homes on where the player is now, so it always comes home.
  - With fewer targets than tomahawks, the extra ones go at the same targets again.
  - "An enemy is hit at most once on the way out and once on the way back" is per tomahawk: two tomahawks on the same line
    hit an enemy four times.
  - The damage is set at the throw.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after): `gs.Upgrade.Grant Tomahawk`,
  `gs.Spawn MeleeBandit 5`. In about 10 s: 4 throws, 7 hits, a "12" damage number on a bandit; the weapon slot showed the
  Tomahawk icon.
- **Full suite:** 62 tests, all passing, in 181.9 s.

### Prompt 52b — Fix: Dynamite and Molotov lead their target
Between 52 and 53. Both lobbed weapons used to land where the target stood at the throw, so a running bandit was out of the
blast or the patch by the time it came down. They now aim where it will be.
- **The aim** (`AGSLobbedThrow::AimAt(Target, FlightTime, LeadFactor)` → `FGSLobAim { Landing, BodyCenter, bLed }`, shared by
  both): the target's position plus its flat velocity (Z dropped) × the flight time left × `LeadFactor`, then a WorldStatic
  trace down from body height to the ground under that point. The landing is that ground point; `BodyCenter` is the body
  over it (half the capsule up), which is where Dynamite's blast is centred. With `LeadFactor` 0, a target standing still
  or no ground there (off the map) it returns exactly the old point: the target's feet where it stands.
  - "Flight time left": the flight time minus the part a late throw has already flown (at most a frame).
  - Dynamite's blast is still centred at body height over the landing (Powder Keg measures the same way); the Molotov's
    patch is centred on the landing. Nothing else changed: target choice (Dynamite's neighbour count still uses current
    positions), flight time, schedule, damage, radius.
- **`LeadFactor`** (new, default 1.0; 0 = the old landing point) on `UGSDynamiteSettings` ("Stick") and
  `UGSMolotovSettings` ("Throwing"). The assets use the class default.
- **Counters and logs:**
  - Dynamite: `WasLastThrowLed`, `GetTargetHits` / `DidLastBlastHitTarget` (blasts that caught the enemy the stick was
    thrown at; `Explode` takes the target as an optional last argument).
  - Molotov: `WasLastThrowLed`, `GetFirstBurnTargetHits` / `DidLastPatchHitTarget` (patches whose first burn caught the
    target; checked before that burn, in case it kills it; `IgnitePatch` takes the target as an optional last argument).
  - The throw lines are now `Log` (were `Verbose`) and add the target's speed and the lead: "[dynamite] stick 44 at
    GSEnemyCharacter_0 (2 neighbour(s), moving 452 cm/s), 0.4 damage in 300 cm, 0.000 s late, led 271 cm ahead". The
    blast and patch lines end with "target hit" / "target missed" / "target in it".
  - `gs.Upgrade.List`: "lead x1.00" and "(N caught their target)" on the Dynamite and Molotov lines.
- **Test hooks:** `AGSGameMode::SetDynamiteSettingsOverride`, `SetLuckyCoinSettingsOverride`, `SetMolotovSettingsOverride`
  and `SetTomahawkSettingsOverride` (like `SetCombatSettingsOverride`); the `Get…Settings()` getters return the override when
  one is set.
- **Tests** (see the test table):
  - `Upgrades.Dynamite`: `AimAt` on its own (800 cm/s × 0.6 s = 480 cm ahead, flat, on the ground; LeadFactor 0.5 = 240 cm;
    0 = the old point; vertical speed only = no lead; running off the edge of the town's ground = where it stands now); a
    standing target at LeadFactor 1 lands exactly where LeadFactor 0 does and the blast is as before; a bandit running
    straight down the lane at a steady 800 cm/s is inside the blast with LeadFactor 1 (27 cm from its centre) and outside
    with 0 (453 cm).
  - `Upgrades.Molotov`: a standing target's first burn catches it; the same runner is in the patch at its first burn with
    LeadFactor 1 (27 cm from the centre) and outside it with 0 (453 cm).
  - The existing checks are unchanged.
- **PIE check** (town, god mode, round paused, editor throttling off; restored after): Dynamite and Molotov at power 0,
  the player's Damage at 0.01 (nobody dies, nobody levels up), 3 melee bandits. The player teleported (`BugItGo`) back and
  forth between two spots 50 m apart on the ring road every 5–7 s. That kept the bandits running at the player when they
  came into range (Dynamite 20 m, Molotov 15 m). `LeadFactor` was set on the two assets in memory, 0 then 1, and set back to
  1 afterwards. Counted from the throw and blast / patch log lines, the first 10 throws at a bandit moving 300 cm/s or more:

  | | LeadFactor 0 | LeadFactor 1 |
  |---|---|---|
  | Dynamite: blasts that caught their target | 7 of 10 | 9 of 10 |
  | Molotov: first burns that caught their target | 0 of 10 | 10 of 10 |

  - Dynamite's 3 m blast (3D, from body height) still catches a melee bandit at LeadFactor 0 most of the time: at 452–556
    cm/s it covers 2.7–3.3 m in the 0.6 s flight. All three misses were the bandit running at 530 cm/s, and the 452 cm/s
    one was always caught. The one miss at LeadFactor 1 was the 556 cm/s bandit (likely turning or stopping at the
    player after the throw).
  - The Molotov's 2.5 m patch (flat) missed every running bandit without the lead and caught every one with it.
  - Throws at bandits that had already reached the player (standing): all hit with either setting (29 Dynamite, 21 Molotov).
- **Faster weapon tests.** Each of the four tests now runs on a copy of its settings asset with shorter times, set as the
  GameMode override (`MakeWeaponSettingsCopy`). The asset's own values are still checked first. Every fixed wait shrinks by
  the same factor, so it still covers the same number of intervals ("no throws in 2.25 intervals" was "in 9 s"). The DPS info
  lines are worked out at the asset's intervals. No check was removed or loosened.

  | Test | Test copy | Before | After |
  |---|---|---|---|
  | `Upgrades.Dynamite` | a throw every 1 s (4 s) | 22.7 s | 7.2 s (lead checks included) |
  | `Upgrades.LuckyCoin` | a flip every 1.25 s (1.5 s; a five-enemy flip takes 0.9–1.1 s and needs a gap before the next) | 7.0 s | 6.0 s |
  | `Upgrades.Molotov` | a throw every 2.75 s (5 s), a burn every 0.25 s (0.5 s) for 2 s (4 s): still 8 burns a patch | 12.1 s | 8.8 s (lead checks included) |
  | `Upgrades.Tomahawk` | a throw every 2.25 s (3 s; out and back takes about 1.8 s) | 8.0 s | 6.3 s |

  The four together went from 49.9 s to 28.7 s (run on their own), and the full suite from 181.9 s to 160.1 s.
- **How `Upgrades.WeaponSlots` works out its expected share** of later offers with no weapon card (changed in Prompts 49
  and 52). Checked: it doesn't call the offer roll it tests, so nothing needed changing.
  - From the pool's contents: it counts the pool's non-weapon upgrades (Legendary abilities left out) and lists its weapons.
  - For each of the 200 offers, it counts the weapons that can be offered at that moment, using the pool's weapon-slot
    rule `CanOfferWeapon` with the owned picks.
  - It takes the chance that three cards drawn without repeats from those upgrades are all non-weapons:
    ∏ (N − k) / (N + W − k) for k = 0..2. It averages that over the 200 offers.
  - Only then does it roll each offer (`QueueUpgradeOfferForDebug`) and count the ones with no weapon card. The roll's
    result never feeds the expected share.
  - The formula counts upgrades, so it assumes equal `AppearanceWeight` (all 1 today). A pool with uneven weights would
    need weights in the formula.
- **CLAUDE.md:** a new convention: file-local constants and helper functions (anonymous namespace, `static`) get names
  unique to their file, because the unity build compiles several files as one (it broke the build in Prompt 52). The
  test-run note now says 160 s.
- **docs/PROGRESS.md repaired:** the Prompt 52 entry had been written into the middle of the intro sentence at the top of
  the file. It was moved here after Prompt 51, and the intro sentence was restored. Prompt 51's last line ran into the
  separator ("173.7 s.---"); fixed. Nothing else changed: the text was compared character by character apart from the
  line breaks.
- **Decided here, to confirm:**
  - The lead uses the flight time left (a late throw has already flown part of it), not the full flight time.
  - Dynamite's blast stays centred at body height over the new landing point, as before over the old one. "Centred on the
    landing point" is read as "over it", so the 3D radius measures the same way.
  - The ground trace is world geometry only (WorldStatic), from body height down 5 m past the feet. A point ahead over a
    crate lands on the crate; one with nothing under it falls back to the old point.

### Prompt 53 — Weapons checkpoint
All seven weapons together: the weapon-slot rules with the full pool, the offer rates, every weapon's damage per second,
and two full loops in PIE. **The weapons block (Prompts 49–53) is complete.**
- **New test `Checkpoint.SevenWeapons`** (see the test table), on the real DA_UpgradePool:
  - **Pool contents:** 21 upgrades, each once and in its category. Stat Boosts: Damage, Fire Rate, Reload Speed, Move Speed,
    Magnet, Bigger Mags, Loaded Dice. Abilities: Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech, Powder
    Keg, and Second Wind (the only Legendary ability). Weapons: Floating Gun, Razor Cards, Barbed Wire, Dynamite, Lucky
    Coin, Molotov, Tomahawk. Every DA_Upgrade_* asset in the project is in the pool. MaxWeapons 3, three HUD slots.
  - **The roll, for each of the 35 sets of three owned weapons** (200 offers each, every other one a first offer with its
    weapon card; 7,000 offers): none of the other four weapons is ever rolled, and each owned one still is.
  - **8 real runs** (seeds 5310–5317, 40 offers each), taking a weapon card whenever there is one. Each run's three
    rerolls are kept for after the slots are full. Every run filled its three slots and never held more. In the 293
    offers and 24 rerolls after that, no unowned weapon came up. Weapons picked, in order: Barbed Wire / Lucky Coin /
    Dynamite; Molotov / Razor Cards / Floating Gun; Barbed Wire / Tomahawk / Molotov; Floating Gun / Molotov / Razor Cards;
    Razor Cards / Barbed Wire / Dynamite; Razor Cards / Lucky Coin / Dynamite; Dynamite / Molotov / Barbed Wire; Razor
    Cards / Tomahawk / Molotov.
  - **First offers:** 48 new runs (seeds 5400–5447), each first offer rerolled 3 times: all 192 had a weapon card. Each of
    the seven weapons came up: Molotov 49, Floating Gun 48, Dynamite 47, Tomahawk 42, Razor Cards 42, Lucky Coin 41,
    Barbed Wire 41 cards.
  - It takes 0.3 s.
- **Simulator with owned upgrades:** new `UGSDebugLibrary::SimulateUpgradeOffersOwning(Count, Seed, OwnedUpgrades)`
  (`SimulateUpgradeOffers` calls it with none; `FGSUpgradeOfferSimulation::Owned` lists them). The MCP tool
  `simulate_upgrade_offers(count, seed, owned)` takes a comma-separated `owned` list, each counted as one pick.
- **Weapon offers** (`simulate_upgrade_offers(10000, 1)`; later offers, not the first one with its guaranteed weapon card):

  | Weapon | Nothing owned (slots free): offers with it | Floating Gun, Dynamite and Molotov owned (slots full) |
  |---|---|---|
  | Floating Gun | 14.9% | 17.8% |
  | Razor Cards | 15.5% | 0 |
  | Barbed Wire | 14.9% | 0 |
  | Dynamite | 15.0% | 18.2% |
  | Lucky Coin | 14.5% | 0 |
  | Molotov | 14.2% | 18.7% |
  | Tomahawk | 14.9% | 0 |
  | Any weapon card | 74.2% of offers (34.6% of cards) | 48.1% of offers (18.2% of cards) |

  - With the slots free each weapon is on about 5% of cards. Once they are full the three owned weapons share the Weapon
    category's AppearanceWeight between them, so each comes up a little more often than before (about 6% of cards).
  - The share of offers with a weapon drops from 74% to 48%; Stat Boosts go from 35% of cards to 43% and Abilities from
    30% to 38%.
  - No short offers. The weapon cards' rarity split stays at 79 / 16 / 4.5 / 0.6% either way.
- **Damage per second** (Damage 1, no crits; from the settings, per weapon rules; the power-0 and power-3 numbers match
  what the weapon tests measured):

  | Weapon | Power 0: one enemy | Power 0: five | Power 5: one enemy | Power 5: five |
  |---|---|---|---|---|
  | Floating Gun, one gun | 12 | 12 | 27 | 27 |
  | Floating Gun, two guns | 24 | 24 | 54 | 54 |
  | Razor Cards | 12 | 12 | 27 | 27 |
  | Barbed Wire | 10 | 50 | 15 | 75 |
  | Dynamite | 10 | 50 | 15 | 75 |
  | Lucky Coin | 6.7 | 26.7 | 10 | 50 |
  | Molotov | 6.4 | 32 | 9.6 | 48 |
  | Tomahawk | 8 | 40 | 24 | 120 |

  - "Five" = five enemies close together, as in each weapon's test: within 1.2 m of each other (inside Barbed Wire's
    ring, Dynamite's blast and the Molotov's patch). Lucky Coin needs them within 8 m of each other for its bounces.
    Tomahawk needs them on its line (a row 3 m apart). A clump only partly on the line takes less.
  - Power 5: +50% damage for every weapon. Floating Gun and Razor Cards also get +50% fire rate (27 = 8 × 1.5 × 1.5 × 1.5
    and 6 × 1.5 × 2 × 1.5). Barbed Wire's radius is 4.375 m, Dynamite's blast 3.75 m and the Molotov's patch 3.125 m. Lucky
    Coin bounces 5 times (six hits a flip, so all five are hit). Tomahawk throws 2 tomahawks; with one enemy both go at it
    (4 hits a throw).
  - A Floating Gun at power 5 always has both guns (power 5 needs at least three picks, and two picks give the second
    gun); the one-gun row is per gun.
  - Single-target weapons (Floating Gun, Razor Cards) don't gain from a group. Area weapons (Barbed Wire, Dynamite,
    Molotov, Lucky Coin, Tomahawk) do five times as much against five.

#### Weapons checkpoint — two full loops in PIE
Hub → run → death → hub, twice. The seeds were set in the hub (`gs.Run.Seed 5301` / `5302`) and each run started with
`gs.Market.Return`. `gs.Debug.AutoPickUpgrades 1` took every offer. Editor throttling was off, and the real profile slot
was used.
- **How it was played**, as in Prompt 48:
  - Cleared rounds: god mode on, weapons acting on their own, and one aimed rifle tap per script pass. Every second pass
    sent `gs.KillAll` once the whole wave had spawned.
  - The death round: god mode off, standing still.
  - "Combat" times are spawn time plus command delay. `gs.KillAll` damage counts as "other". Health stayed full, so the
    orbs healed nothing. The rifle's numbers are small (one tap a pass).

**Loop 1 (seed 5301): died in round 5, 1:45 run time**

| Round | Wave | Combat | Damage dealt | Level after | Offer → pick |
|---|---|---|---|---|---|
| 1 | 4 | 37.3 s | rifle 40, KillAll 200 | 1 | — |
| 2 | 6 | 7.6 s | KillAll 380 | 2 | Lucky Coin / Floating Shield / Molotov (all Common) → **Lucky Coin** |
| 3 | 8 | 13.9 s | Lucky Coin 222, KillAll 318 | 2 | — |
| 4 | 10 | 8.6 s | Lucky Coin 170, KillAll 552 | 3 | Bigger Mags / Damage / Lucky Coin (all Common) → **Lucky Coin** (power 1) |
| 5 | 13 | died after 6.7 s (113 damage) | Lucky Coin 99 | 3 | — |

Totals: 4 rounds cleared, level 3, 28 kills, best combo 10, $583 earned. Damage: Lucky Coin 491, rifle 40, `gs.KillAll`
1,449 (1,980 in total). Health orbs: 4 dropped, all collected. One weapon (Lucky Coin) in two offers.

**Loop 2 (seed 5302): died in round 10, 3:25 run time**

| Round | Wave | Combat | Damage dealt | Level after | Offer → pick |
|---|---|---|---|---|---|
| 1 | 4 | 20.0 s | rifle 40, KillAll 200 | 1 | — |
| 2 | 6 | 14.2 s | rifle 20, KillAll 360 | 2 | Extra Jump / Dynamite / Bigger Mags → **Dynamite** |
| 3 | 8 | 12.9 s | Dynamite 184, KillAll 356 | 2 | — |
| 4 | 10 | 13.4 s | Dynamite 470, KillAll 252 | 3 | Barbed Wire / Lucky Coin / Reload Speed → **Barbed Wire** |
| 5 | 13 | 13.9 s | Barbed Wire 377, Dynamite 419, KillAll 212 | 4 | Dash Charge / Molotov / Floating Gun → **Molotov** (slots full) |
| 6 | 14 | 14.5 s | rifle 20, Barbed Wire 282, Dynamite 378, Molotov 129, KillAll 331 | 5 | Fire Rate / Loaded Dice / Bigger Mags (Legendary) → **Bigger Mags, Legendary** |
| 7 | 15 | 17.1 s | Barbed Wire 371, Dynamite 359, Molotov 97, KillAll 477 | 6 | Reload Speed (Rare) / Fire Rate / Damage → **Reload Speed, Rare** |
| 8 | 17 | 13.9 s | Barbed Wire 371, Dynamite 374, Molotov 174, KillAll 665 | 6 | — |
| 9 | 18 | 13.8 s | Barbed Wire 455, Dynamite 428, Molotov 184, KillAll 709 | 7 | Magnet / Floating Shield / Dynamite → **Dynamite** (power 1) |
| 10 | 19 | died after 5.1 s (158 damage) | Barbed Wire 10, Dynamite 44, Molotov 4 | 7 | — |

Totals: 9 rounds cleared, level 7, 105 kills, best combo 16, $2,238 earned (balance $12,846). Damage: Dynamite 2,656,
Barbed Wire 1,867, Molotov 587, rifle 80, `gs.KillAll` 3,561 (8,751 in total). Health orbs: 17 dropped, all collected,
0 healed. The summary showed the same numbers, "Return to Market" (a Slate click) opened the hub both times, and the cash
was kept.

- **Weapons offered:** loop 1 Lucky Coin ×2 and Molotov; loop 2 Dynamite ×2, Barbed Wire, Lucky Coin, Molotov and
  Floating Gun. Not offered in either loop: Razor Cards, Tomahawk. Once loop 2's three slots were full (after round 5),
  its three later offers had only one weapon card: its own Dynamite.
- **Frame time with all three weapon slots active and 30 enemies:** a third run (seed 5302 again) with loop 2's weapons
  at its final picks (Dynamite ×2, Barbed Wire, Molotov), god mode, the round paused. `gs.Spawn MeleeBandit 30` gave 34
  alive with round 1's 4, all within 4.2 m of the player. The player's Damage was set to 0.01, so nobody died and every
  hit still ran. Then `gs.Perf 5` twice. The control was a new run (`gs.Run.Seed 5399`) with no weapons and the same 30
  spawned bandits (the restart removed the wave's 4). 120 fps cap:

  | | Enemies | Frame avg | p95 | Max | Game avg | Render avg | GPU avg |
  |---|---|---|---|---|---|---|---|
  | Three weapons (1st) | 34 | 8.59 ms (116 fps) | 9.79 | 28.88 | 8.27 | 4.73 | 4.15 |
  | Three weapons (2nd) | 34 | 8.58 ms (117 fps) | 9.57 | 28.72 | 8.23 | 4.63 | 4.16 |
  | No weapons (control) | 30 | 8.42 ms (119 fps) | 8.33 | 31.30 | 5.30 | 3.10 | 3.87 |

  The three weapons cost about 3 ms of game thread and 1.5 ms of render with a crowd in all of them (a ring tick, a
  blast and a patch hitting 30+ enemies, each with a damage number), and the frame stays at about 116 fps.
- **Observations** (suggestions only, nothing changed):
  - **Area weapons carry a crowd.** In loop 2 Dynamite and Barbed Wire did 56% of all damage (86% of what wasn't
    `gs.KillAll`) from round 5 on, at power 0–1. Against a group they are the strongest weapons at power 0 (50 DPS each),
    where Floating Gun and Razor Cards stay at 12 / 24.
  - **Molotov did least** of the three (587), as its table numbers predict (32 per patch every 5 s on a crowd).
  - **Lucky Coin alone** (loop 1) did 491 over three rounds: one weapon at power 1 doesn't clear rounds.
  - **The Tomahawk is the strongest at power 5** against a line (120), but only against enemies on its line.
  - **Weapon cards are common:** 6 of the 8 offers in the two loops had at least one (the simulator gives 74% with free
    slots and 48% once they are full).
  - Fast clear was missed in round 1 of both loops (37 s and 20 s against 14 s), as in Prompt 48.
- **Full suite:** 63 tests, all passing, in 160.4 s.

## Design update — cops, city and round flow (M2d)

Prompt 54. Docs only; no code changed. **M2d** is a block after the weapons checkpoint (Prompt 53) and before the
market (M3). It gives the game a new theme, look and round flow, and adds six enemy types. Until the prompts below
build them, the code still follows the old design: bandits, the 300 × 300 m town, hit-scan enemy shots, an alive cap
of 30 and separate rounds with a countdown.

**Decisions (user).** Every number is a placeholder and lives in data.

1. **Theme: cops and robbers.** The player is the robber, out to get all the cash. The enemies are law enforcement.
   This replaces "bandits in a bandit town". The project and code names (BanditBaller, the `GS` prefix) stay.
2. **Setting.** A run takes place in a city. The hub stays the black market and casino (`L_Hub_Graybox`). The run map
   becomes a **city graybox of 500 × 500 m with a 100 m ceiling**, built later in this block (Prompt 67). It replaces
   the 300 × 300 m `L_Town_Graybox` and its 75 m ceiling.
3. **Look: low poly** (faceted, flat shaded, clean blocks of colour, no texture detail) **with high-quality shaders
   and lighting:** coloured rim light, glowing emissives with bloom, coloured shadows, soft gradient skies. The
   reference is the game RevenBlade. The art pass is still M5; until then placeholders follow the low-poly look.
4. **Placeholder models are made in Blender from now on** (set up in Prompts 55 and 56), not from engine cubes, cones
   and cylinders.
   - That includes new models for the player (the robber) and for every enemy: very basic low poly that gives the
     idea of the theme and the final look, with props such as a cap, a baton, a carbine and a shield.
   - No new animations yet: the models use the skeleton and the animations the game has today.
   - The older engine-shape placeholders (weapons, pickups, the portal, cover) are remade in Prompt 68.
5. **Enemy names.** MeleeBandit becomes the **Beat Cop** (a regular cop with a baton). RangedBandit becomes the
   **Patrol Officer** (carbine). The code, assets, commands and tests keep the old names until Prompt 57.
6. **Dodgeable ranged shots.** Ranged enemies fire projectiles the player can dodge: visible, about 28 m/s, aimed at
   where the player was when the shot was fired (no lead). The aim warning and the burst stay. No enemy uses instant
   hit-scan shots. (Today GA_EnemyFire traces each shot instantly, an object query, after its 0.4 s laser aim.)
7. **At most 40% of a generated wave is ranged** (all ranged types together), for now. (Today the budget buys ranged
   bandits freely: round 6 is 8 of 12 ranged, round 7 9 of 13; see the M2 checkpoint.)
8. **Enemy types unlock as the rounds go on**, so later rounds mix more types. There is no wanted level and there are
   no agency tiers. Eight types in all (placeholder unlock rounds; `UGSEnemyData::MinRound` already exists):

   | Enemy | Unlocks in round | Kind | Status | Prompt |
   |---|---|---|---|---|
   | Beat Cop | 1 | melee (baton) | built as MeleeBandit | 57 |
   | Patrol Officer | 2 | ranged (carbine) | built as RangedBandit | 57, 58 |
   | Rusher | 4 | melee | new | 61 |
   | Shield | 6 | melee | new | 62 |
   | Grenadier | 8 | ranged | new | 64 |
   | Heavy | 10 | melee | new | 63 |
   | Sniper | 12 | ranged | new | 65 |
   | Drone | 14 | ranged | new | 66 |

9. **The new types in short** (the details come with their prompts):
   - **Rusher:** half a Beat Cop's health, about 1.5 times its speed, comes in packs of 4–6, a quick weak hit.
   - **Shield:** the shield stops the rifle, the pistol and weapon projectiles from the front. Area damage, Piercing
     Rounds and hits from the side or behind get through. The shield breaks after enough damage and leaves a normal
     cop. Slow; melee only (a bash).
   - **Grenadier:** keeps 12–20 m away and lobs an explosive grenade every 4 s; a red circle shows for 1.5 s before
     it explodes. It doesn't hurt other enemies.
   - **Heavy:** about 8 times a Beat Cop's health, slow, a ground slam that marks a 3 m circle for 1 s before it
     lands. About 4 times the cash, and it always drops a health orb (instead of the 20% chance).
   - **Sniper:** stands 30–45 m away, on the ground for now. A laser follows the player for 1.5 s, locks in place and
     flashes for 0.4 s, then it fires one very fast visible projectile along the locked line for heavy damage.
     Moving or dashing during the lock dodges it; cover blocks it.
   - **Drone:** flies over cover at about 3 m, low health. It hovers, beeps just before it starts shooting, then fires
     dodgeable shots. When it is shot down it explodes and hurts enemies near it, never the player.
10. **Many more enemies.** The alive cap (`DA_WaveSettings.MaxAliveEnemies`, 30 today) goes to **100** in this block,
    and the game must hold **60 fps on the user's PC with 100 alive** (Prompt 60). The long-term goal is thousands of
    enemies at once, like Megabonk; that is a later block and is not designed yet.
11. **Roll-over rounds.** Play stays split into rounds but feels continuous:
    - The next round's enemies start arriving when only a few of the current round are left, with no countdown.
    - Enemies arrive in a steady stream, not in a few batches. (Today: DT_Waves rows spawn in batches of 2–3 every
      1.6–2 s, generated waves 3 every 1.5 s; a wave ends when all have spawned and died; PreRound is a 3 s countdown
      and RoundClear a 4 s pause with the summary panel.)
    - The round-clear bonus shows as a small pop-up while play goes on (not the RoundClear summary panel).
    - Level-ups still pause the game for the card pick.
12. **A breather every 10 rounds:** 10 seconds (both numbers are placeholders). A boss goes there later.
13. **The block ends with a checkpoint (Prompt 69)** whose income numbers (cash per round and per run at real
    play) set the M3 prices.

**What this changes in the code and tests** (checked for this plan; built in Prompts 55–69)
- Names (57): `DA_Enemy_MeleeBandit` / `_RangedBandit`, their BPs, ability sets, StateTrees, materials, `gs.Spawn`
  names, the MCP toolset's `spawn_enemies` types, log lines and every test that names them.
- Models (56, 57, 68): the player and enemies use the template mannequins with tinted materials today; the pickups,
  weapons, portal and cover use engine shapes.
- Shots (58): GA_EnemyFire's instant trace becomes a projectile. The player-side projectile (`AGSProjectile`) ignores
  player pawns, so enemy shots need their own path. `Balance.TimeToKill` and the Floating Shield test (a ranged burst
  blocked) rely on enemy shots landing.
- The ranged cap (58): the budget purchase in `UGSWaveDirector`; `Round.WaveBudget` checks today's purchase.
- Round flow (59): `EGSRoundPhase` (PreRound / Combat / RoundClear), the profile save at the end of RoundClear, the
  round-clear bonus (its fast-clear and no-damage rules assume one round at a time), `WBP_RoundSummary`, the round
  banners and countdown in WBP_HUD, and `Round.PhaseLoop`, `Round.ClearBonus`, `Round.WaveDirector`, `UI.RoundHUD`,
  `Profile.PermanentCash`.
- Alive cap (60): `MaxAliveEnemies`, `MaxGeneratedEnemies` 150, per-enemy cost (StateTree, sight perception, health
  bar widgets, CharacterMovement), the weapons' target searches over every enemy.
- City (67): `L_Town_Graybox`, `BeginPIE`'s default map, `Map.Town`, `Map.TownLayout`, `Map.Boundary`, the hub's
  RunMap, and the spawn ring (12–25 m) in a bigger map.

**Open questions** (settled in each prompt; not decided)
- Roll-over (59): how many enemies are "a few left" (a count or a share of the round), whether PreRound and RoundClear
  stay as phases, when the profile saves without a RoundClear pause, and how the no-damage and fast-clear bonuses work
  when two rounds overlap.
- Breather (59): does it count as a round, and is the pickup vacuum there or at every round's end?
- Shield (62): the angle that counts as "from the front", the shield's health, and whether the weapons' area damage
  (Barbed Wire, Dynamite, Molotov, Powder Keg) all counts as area.
- Drone (66): do enemies killed by its explosion count as the player's kills (cash, XP, combo)?
- Heavy (63): does its always-dropped health orb replace or add to the 20% roll?
- How the new types fit the budget waves: their SpawnCost, and whether DT_Waves (rounds 1–5) changes.

**Docs changed**
- CLAUDE.md: the game summary (theme, city, roll-over rounds, enemies), a new **Art and placeholders** rule, the
  combat rule's map note, the enemy-name convention (old code names until Prompt 57) and the current block.
- This file: the game summary, the intro and the step table, this entry, **Decided (user)**, the open items (the
  round-6 ranged share is decided by item 7), a known issue and **Next up** (Prompts 54–69, then M3).

## M2d — Cops, city and round flow (Prompts 54–69)

Prompt 54 is the design update above.

### Prompt 55 — Blender MCP setup
Tooling only: no game code, assets or tests changed.
- **Installed:**
  - **uv / uvx 0.13.9** (`winget install --id=astral-sh.uv -e`, approved by the user). uvx.exe is at
    `%LOCALAPPDATA%\Microsoft\WinGet\Packages\astral-sh.uv_Microsoft.Winget.Source_8wekyb3d8bbwe\uvx.exe`; winget added
    the `uv` / `uvx` aliases and the PATH entry (new terminals only).
  - **mcp-for-blender 2.3.1** (PyPI; the project was formerly `blender-mcp`, which is now a wrapper that installs it),
    run with uv's managed **Python 3.15.2** and **mcp 1.30.0**.
  - **The Blender add-on "MCP for Blender" 2.0** (`bl_info` version), copied by `uvx mcp-for-blender install-addon` to
    `%APPDATA%\Blender Foundation\Blender\5.3\scripts\addons\blender_mcp.py`, for **Blender 5.3** (`C:\Program
    Files\Blender Foundation\Blender 5.3`).
- **`.mcp.json`** has a second server next to `unreal-mcp`:

  ```json
  "blender": {
    "type": "stdio",
    "command": "C:\\Users\\bbehm\\AppData\\Local\\Microsoft\\WinGet\\Packages\\astral-sh.uv_Microsoft.Winget.Source_8wekyb3d8bbwe\\uvx.exe",
    "args": ["mcp-for-blender"],
    "env": { "BLENDER_HOST": "localhost", "BLENDER_PORT": "9876", "BLENDER_MCP_SAFE_MODE": "1", "DISABLE_TELEMETRY": "true" }
  }
  ```

  - The full path to uvx.exe is used because Claude Code was started before uv was on PATH.
  - `BLENDER_HOST` / `BLENDER_PORT` are the defaults, written out so the server stays on localhost:9876.
  - `BLENDER_MCP_SAFE_MODE=1` makes the server check scripts before they run in Blender: it blocks file access,
    running other programs, network access and code that keeps running after the script. `DISABLE_TELEMETRY=true`
    turns off the server's telemetry. The safe-mode variable is in the README's table. `DISABLE_TELEMETRY` was only
    in the summarized part of the README that was read.
- **Followed from the README** (https://github.com/ahujasid/blender-mcp, now `ahujasid/mcp-for-blender`): the uvx
  command, `install-addon`, the add-on and tab names, port 9876, and "only one MCP client". Not used: the one-line
  `irm … | iex` installer and `uvx mcp-for-blender setup`, which also configure other MCP clients.
- **Snag: the Claude desktop app runs as an MSIX package.** Files its child processes *create* under `%APPDATA%` /
  `%LOCALAPPDATA%` go to a private copy in `%LOCALAPPDATA%\Packages\Claude_pzs8sxrjxfjjc\LocalCache`.
  - The first `uvx mcp-for-blender install-addon`, run from the session, downloaded Python into that private copy. uv's
    `cpython-3.15` junction then pointed at the real, empty path ("Missing expected target directory for Python minor
    version link").
  - The add-on file would also have gone into the private copy, where Blender can't see it.
  - The private `uv` folders were deleted, and the user ran `install-addon` in a normal PowerShell. From the session
    `uvx --offline mcp-for-blender --help` then ran fine (31 packages into the cache).
  - Rule (CLAUDE.md, **Blender**): uv installs and updates run outside the Claude app.
- **The user's steps in Blender** (once, before the first model prompt):
  1. Start Blender 5.3.
  2. Edit → Preferences → Add-ons, search "MCP for Blender", tick **Interface: MCP for Blender**. Preferences save
     automatically (or use the ☰ menu → Save Preferences).
  3. In the 3D viewport press `N`, open the **MCP for Blender** tab and click **Start MCP Server**. Leave the host on
     localhost and the port on 9876.
  4. Don't press "Connect to Claude" or connect any other client: only this project's Claude Code session uses it.
- **Checked after a Claude Code restart:** the `blender` server loaded and `get_addon_status` reached Blender 5.3.0
  (add-on 2.0, protocol 16, up to date, telemetry off; no asset libraries or 3D generators switched on). `unreal-mcp`
  answered `list_toolsets` once the editor was started (it refuses connections while the editor is closed).
- **CLAUDE.md:** a new **Blender** section (Blender open with its server started before a model prompt; the session
  start check confirms `blender` too; save .blend files before running code; one client only; the MSIX rule) and
  Blender MCP in the tooling split.

### Prompt 56 — Blender placeholder pipeline and the robber
How placeholder models get from Blender into Unreal, the first props, a low-poly body on the mannequin skeleton, and
the robber on the player. Nothing about how the game plays changed. The routine and the style rules are in CLAUDE.md
(**Placeholder models**).
- **Folders:** `Art/Blender/` (`Props.blend`, `LowPolyBody.blend`) and `Art/Export/<Kind>/` in the repo;
  `.gitattributes` already sends `.blend`, `.fbx`, `.glb` / `.gltf` and `.bin` to LFS; `.gitignore` now skips Blender's
  `.blend1` backups. In Unreal `/Game/Art/Placeholders/` with `Characters`, `Props`, `Enemies`, `Weapons`, `City` and
  `Materials`.
- **FBX, not glTF** (the baton both ways): both gave the same geometry in UE 5.8 (140 triangles, 240 vertices, flat
  shading kept, X forward, the right size, the Blender material names as slots). FBX through the MCP tools imports one
  mesh where it is asked, with no extra assets and Nanite off, and it is the only path that binds a mesh to an
  existing skeleton (`SkeletalMeshTools.import_file`). glTF needs Interchange (the MCP import only takes FBX / OBJ),
  which adds its own `<file>/StaticMeshes` and `/Materials` folders, makes three material assets we'd throw away and
  turns Nanite on.
- **`/Game/Art/Placeholders/Materials/M_LowPoly`**: `Color` (vector) → Base Color, `Roughness` (0.75), `Metallic` (0),
  `EmissiveColor` × `EmissiveStrength` (0) → Emissive; usage flags for skeletal and instanced meshes. 26 `MI_` colour
  instances so far (`MI_LowPoly_*` for props and the body's neutral defaults, `MI_Robber_*` for the player).
- **Props** (`Props.blend`, Unit Scale 1, metres), checked in Blender and in Unreal (sizes below are the UE bounds):

  | Asset | Folder | Triangles | Size | Origin | Slots |
  |---|---|---|---|---|---|
  | `SM_PoliceBaton` | Props | 140 | 62 cm long (−2 to 60 on X) | grip end | Baton, Rubber, Metal |
  | `SM_PoliceCap` | Props | 134 | 28 × 24 × 12 cm, visor toward +X | bottom of the band | CapNavy, CapVisor, CapBadge (gold, faint glow) |
  | `SM_PatrolCarbine` | Weapons | 148 | 87 cm (−28.5 to 58.5 on X) | top of the pistol grip, muzzle toward +X | GunMetal, Polymer, GunAccent (tan magazine) |

  Not attached to anything yet (Prompt 57).
- **`SK_LowPolyBody`** (`/Game/Art/Placeholders/Characters`), on the existing **`SK_Mannequin`** skeleton with
  **`PA_Mannequin`**, so `ABP_Unarmed` and every existing animation drive it unchanged:
  - Made from `SKM_Quinn_Simple` exported from Unreal (new tool `export_asset_fbx`) and imported into Blender: the
    armature (88 bones) was kept, Quinn's mesh dropped, and blocks built on the bones: head (+ nose, ears, a headwear
    block), neck, chest, belly, hips with a belt, upper and lower arms, hands with a thumb, upper and lower legs, feet.
    17 bones carry geometry; **every vertex is weighted 1.0 to exactly one bone** (checked: 0 vertices with another
    count or weight), so nothing needs weight painting and nothing stretches.
  - **1,524 triangles** (Quinn's LOD 0: 87,280). Six slots for re-colouring: Skin, Top, Trousers, Shoes, Gloves,
    Headwear (neutral `MI_LowPoly_Body*` defaults on the asset).
  - Its reference pose matches `SKM_Quinn_Simple` bone for bone (new `describe_ref_pose`): `root` scale 1, `pelvis`
    (0, 2.38, 98.69), `head` (0, 0.14, 162.5), the same rotations. Bounds 181.5 cm tall (Quinn 180.2).
  - **The trap that cost the most time:** exported from a metre-scale file, Blender's FBX writes the ×100 unit scale on
    the armature node. Unreal then keeps `root` at scale 100 with every other bone in metres; the mesh looks right in
    its reference pose (and its bounds are right), but every animation sets `root` back to 1 and the body shrinks to
    1.8 cm. "Apply Transform" (`bake_space_transform`) and `FBX_SCALE_ALL` didn't help (the second gave 0.9 cm bounds).
    Fix: the body file works in centimetres (Unit Scale 0.01, data ×100), so the exporter needs no scale at all.
    `Art.PlayerMesh` now checks `root` scale 1 and the animated height.
  - Updating an asset already in use goes through the new `import_mesh_file` (Interchange reimport in place). The MCP
    import refuses an existing name, and delete + import failed while the old object stayed in memory.
- **The robber** (the player, `BP_GSCharacter`, in the hub and in a run): `SK_LowPolyBody` with the robber's colour
  overrides (`MI_Robber_*`: charcoal top, dark navy trousers, black gloves, dark brown shoes) and two **worn props, not
  part of the mesh**, so the body stays one shared mesh for every character:
  - `SM_SkiMask` (242 triangles): black, crown over the head, a strip round the sides and back, a chin piece with a
    nose, a neck cuff, a thin red trim; only the eyes show through a slit. On the `head` bone.
  - `SM_LootBag` (308 triangles): a tan sack on the back with a tie, two dark straps over the shoulders, and a green
    "$" with a faint glow. On `spine_04`.
  - Robber total: 2,074 triangles (body + mask + bag).
  - **`Core/GSMeshAttachment.*`** (new): `FGSMeshAttachment` (Mesh, Bone, `bModeledInMeshSpace`, Offset, Materials)
    and `GSMeshAttachments::Apply`. `AGSCharacter::MeshAttachments` (set on BP_GSCharacter) builds one static mesh
    component per entry in `OnConstruction`: no collision, no overlaps, no navigation effect, created by the
    construction script. With `bModeledInMeshSpace` the prop keeps the mesh's own axes at the bone's joint in the
    reference pose and then moves rigidly with the bone, so a prop modelled around the body in Blender fits without
    any hand-tuned offset (both props have offset 0). `GetMeshAttachmentComponents()` for tests. The cops can use the
    same list for caps and batons (Prompt 57; enemies don't use it yet).
- **New MCP tools** (`BanditBallerTools`, editor): `export_asset_fbx(asset_path, file_path)` (skeletal or static mesh,
  LOD 0, into the project folder), `import_mesh_file(file_path, destination_path, asset_name)` (Unreal's default
  importer, Interchange in 5.8; replaces an existing asset in place), `describe_ref_pose(mesh_path, bones, max_bones)`
  (`UGSDebugLibrary::DescribeRefPose`) and `console_command(command)` (runs in PIE or the editor world; much faster than
  typing into the Cmd box).
- **New debug execs** on the controller: `GSAim 1|0`, `GSSwapShoulder`, `GSRun <seconds>` (MCP can't hold keys).
- **Checks in PIE** (town and hub, screenshots with `shot` / `shot showui`):

  | Check | Result |
  |---|---|
  | Right shoulder, hip fire, level / looking up / looking down ~50° | Crosshair clear. The masked head comes within ~50 px when looking up; the bag never comes near it. |
  | Right shoulder, aiming down sights | Clear; the robber and bag on the left third. |
  | Left shoulder, hip fire and aiming down sights | Clear; the robber on the right. (A shot taken during the 1 s shoulder blend shows the body crossing the centre, as before.) |
  | Rifle and pistol | Both sit in the right hand as before (`hand_r`, unchanged weapon transforms); both fire (30 → 29, 12 → 11). They are still the engine-cube placeholders. |
  | Run, jump, slide, dash | All play with the existing animations; the blocks stay together at the joints (small gaps at bent knees and elbows). The slide plays the run pose (there is no slide animation), as before. |
  | Floating Gun (2 guns) and Floating Shields (3) | Same positions as before: the guns above the shoulders, the shields at chest height; neither collides with the bag. Shields orbit through the view as before. |
  | Death | The ragdoll (`PA_Mannequin`) falls; the mask and the bag stay on their bones. |
  | Hub | The robber under the hub's warm light. |

- **Frame time** (`gs.Perf 5`, town, PlayerStart, round paused, no enemies, 120 fps cap): before (Quinn, its own
  session) 8.33 ms, game 2.46, render 2.76, GPU 2.66; after (the robber) 8.33 ms, game 2.57, render 2.83, GPU 2.64. A/B
  in that same session with Quinn swapped back in: 8.37 ms, game 2.66, render 2.87, GPU 2.71. The difference is noise:
  one character's triangles don't show at this scale.
- **Tests:** new `Art.PlayerMesh` (see the test table). No existing test named the old player mesh, so none changed.
- **Fixed along the way:** the export tool first lowercased the folders it created (`art/export/reference`); fixed, and
  the folder renamed back to `Art`. A "save all dirty" call had saved `L_Hub_Graybox` while test actors were placed
  in it; the map was restored from git (no change to the hub).
- **Full suite:** 64 tests, all passing, in 161.0 s.

### Prompt 57 — Rename the enemies: Beat Cop and Patrol Officer
The two enemy types get their cop names everywhere and cop models in Blender. Nothing about how they play changed:
health, speed, damage, attacks, AI, cash, XP and wave contents are as before.
- **Old names → new names** (entries before this one keep the old names): MeleeBandit → **Beat Cop** (`BeatCop`),
  RangedBandit → **Patrol Officer** (`PatrolOfficer`), BanditCarbine → `PatrolCarbine`, "bandit(s)" → cop(s).
- **Assets renamed** (MCP `AssetTools.move`; every referencer re-saved, no redirectors left on disk):

  | Old | New |
  |---|---|
  | `Enemies/DA_Enemy_MeleeBandit`, `DA_Enemy_RangedBandit` | `DA_Enemy_BeatCop`, `DA_Enemy_PatrolOfficer` |
  | `Enemies/BP_Enemy_MeleeBandit`, `BP_Enemy_RangedBandit` | `BP_Enemy_BeatCop`, `BP_Enemy_PatrolOfficer` |
  | `Enemies/DA_EnemyAbilities_MeleeBandit`, `_RangedBandit` | `DA_EnemyAbilities_BeatCop`, `_PatrolOfficer` |
  | `Enemies/ST_MeleeBandit`, `ST_RangedBandit` | `ST_BeatCop`, `ST_PatrolOfficer` |
  | `Materials/M_Enemy_MeleeBandit`, `M_Enemy_RangedBandit` | `M_Enemy_BeatCop`, `M_Enemy_PatrolOfficer` |
  | `Weapons/DA_BanditCarbine` | `DA_PatrolCarbine` |

  - DT_Waves (rounds 1–5) and the DA_WaveSettings pool follow through their references; their contents are unchanged.
  - `DisplayName`: "Beat Cop", "Patrol Officer", "Patrol Carbine".
- **Code and text:** the StateTree builder's default paths (`ST_BeatCop`, `ST_PatrolOfficer`), the `gs.Spawn` help
  (`gs.Spawn BeatCop 10`), `UGSDebugLibrary` lookups and comments, the wave log's composition ("20 BeatCop, 10
  PatrolOfficer"), run-stats kill types (`BeatCop`, `PatrolOfficer`, from the asset names), the MCP toolset's
  `spawn_enemies` docs, and in the tests every name, helper (`SpawnToughCop`, `SpawnFrozenCop`, …), check text and
  comment. No old name is kept as an alias: `gs.Spawn MeleeBandit` now finds nothing.
- **What is left named "Bandit"** (searched in `Source`, `Config`, `Content/Python` and the asset names): only the
  project name. That is the module and `BanditBaller.h`, `LogBanditBaller`, the template classes
  `ABanditBallerCharacter` / `GameMode` / `PlayerController` (`AGSPlayerController` derives from the last), the
  `/Game/BanditBaller` content folder, the `banditballer_toolset` Python package and the `BanditBaller.` test prefix.
- **`UGSEnemyData` look fields** (for every later enemy type too):
  - `SlotMaterials`: a material per body slot, by slot name (Skin, Top, Trousers, Shoes, Gloves, Headwear).
  - `BodyMaterial` stays as the fallback for a slot without an entry (both cops leave it empty now).
  - `MeshAttachments`: props as `FGSMeshAttachment`, the struct the player uses.
- **`AGSEnemyCharacter`:** one dynamic material per slot (the slot's entry, else the fallback), so `TelegraphAmount`
  reaches every slot; props through `GSMeshAttachments::Apply` (no collision). Test getters
  `GetMeshAttachmentComponents`, `GetWeaponMeshComponent`, `GetAimLaserComponent`, `GetBodyMaterials`, `IsTelegraphing`.
- **`M_LowPoly`:** new `TelegraphColor` (6, 4.5, 0.5, the old enemy materials' colour) × `TelegraphAmount` (0) added to
  its emissive, so the wind-up glow shows on the low-poly body.
- **The look** (both on `SK_LowPolyBody` with `ABP_Unarmed`, in place of the mannequins):

  | | Beat Cop | Patrol Officer |
  |---|---|---|
  | Top | `MI_BeatCop_Top` navy (0.03, 0.07, 0.30) | `MI_PatrolOfficer_Top` light blue (0.30, 0.50, 0.85) |
  | Trousers | `MI_BeatCop_Trousers` navy (0.022, 0.045, 0.19) | `MI_PatrolOfficer_Trousers` near black (0.02, 0.028, 0.07) |
  | Skin, hands (Gloves slot) | `MI_LowPoly_BodySkin` | the same |
  | Shoes, hair (Headwear slot) | `MI_LowPoly_BodyShoes`, `MI_LowPoly_BodyHeadwear` | the same |
  | Props | `SM_PoliceCap` on `head`, `SM_PoliceBaton` on `hand_r` | `SM_PoliceCap` on `head` |
  | Gun | none | `SM_PatrolCarbine` from `DA_PatrolCarbine.Mesh` (no second attachment) |

  - The robber is charcoal and near black with a black ski mask and a tan bag; the cops are clearly blue with skin
    faces and caps. In PIE the Patrol Officer read as a light-blue shirt under a cap at about 17 m, and the Beat Cop as
    an all-blue uniform.
  - **The cap was re-placed around the head** in `LowPolyBody.blend` (worn-props rule), origin on the `head` joint,
    108 triangles (crown, band, visor to the front, a gold badge), and `SM_PoliceCap` was updated in place with
    `import_mesh_file` (its slots CapNavy / CapVisor / CapBadge kept their materials). It is worn with
    `bModeledInMeshSpace`, no offset.
  - The baton: `hand_r`, bone-relative, yaw 90 (like the player's rifle).
  - `DA_PatrolCarbine`: `MeshTransform` yaw 90 on `hand_r` at the grip, `MuzzleOffset` (58, 0, 3) at the barrel's end,
    so the aim laser and the muzzle effect start at the barrel.
- **Frame time with 34 enemies** (`gs.Perf 5` twice, PIE, town, god mode, round 1's 4 + `gs.Spawn` 15 + 15 around the
  player, 120 fps cap):

  | | Frame avg | p95 | Max | Game avg | Render avg | GPU avg |
  |---|---|---|---|---|---|---|
  | Before (mannequins) | 8.33 ms (120 fps) | 8.33 | 8.61 / 8.50 | 4.34 / 4.35 | 2.91 / 2.95 | 2.97 / 2.98 |
  | After (low-poly cops) | 8.33 ms (120 fps) | 8.33 | 8.42 / 8.44 | 4.93 / 4.90 | 3.09 / 3.08 | 2.85 / 2.88 |

  About +0.6 ms game thread and +0.15 ms render (two or three prop components on every cop); the GPU is slightly lower
  (1,524-triangle bodies instead of the mannequins). The frame stays at the cap.
- **Tests:** new `Enemies.CopLooks` (see the test table). Every existing test passed with only names changed (the
  rename pass above); none had its checks changed.
- **PIE check** (town, god mode): `gs.Spawn BeatCop` / `PatrolOfficer`: the Beat Cop in navy with cap and baton walked
  up and swung (its body glowed yellow during the wind-up), the Patrol Officer in light blue with cap and carbine kept
  its distance; both clearly apart from the robber.
- **Decided here, to confirm:**
  - The cap is re-placed around the head in `LowPolyBody.blend` and updated in place (it was a free-standing prop in
    `Props.blend`, unused).
  - Hands use the skin colour (cops wear no gloves); the Headwear slot (the hair block) keeps the neutral dark brown.
  - The wind-up glow is on the body slots only, not on the props, and keeps the old colour and strength.
  - `M_Enemy_BeatCop` stays as the cash materials' parent; `M_Enemy_PatrolOfficer` stays, unused.
- **Full suite:** 65 tests, all passing, in 161.6 s.

### Prompt 58 — Dodgeable enemy shots and the ranged cap
The Patrol Officer's shots are visible bolts the player can step out of, and a generated wave is at most 40% ranged.
- **Enemy shots are projectiles** (`GA_EnemyFire::FireShot`): no instant trace any more, for any enemy.
  - Each shot aims at the target's chest (`UGSEnemyData::AimHeightOffset` 35 cm above its capsule centre) where it is
    when *that* shot is fired, with no lead, plus the weapon's spread (`UGSRunRandom::VRandCone`, run stream, as before).
  - It launches a pooled `AGSProjectile` (the Floating Gun's and Razor Cards' pool) from the muzzle, flying straight at
    `UGSWeaponData::ProjectileSpeed` (2,800 cm/s on `DA_PatrolCarbine`) for up to the weapon's `Range` (60 m, so
    2.14 s).
  - Unchanged: 7 damage per shot × the enemy's Damage, the 0.4 s aim warning with the laser and the body glow, the
    4-shot burst at 360 RPM, the burst recovery and the AI's ranges.
  - `MulticastShotFired` (existing replication) now plays only the muzzle effect and the fire sound; the 0.06 s
    laser "tracer" flash along the shot is removed (it showed an instant hit).
  - `TODO(co-op)`: enemy shots are local (the projectile isn't replicated).
- **`FGSProjectileLaunch::bEnemyShot`** (new, `AGSProjectile::HandleEnemyShotHit`):
  - It hits a living player (an ASC) or world geometry (cover, walls, floors), which stops it.
  - It passes through every other pawn and anything else with an ASC (other enemies, target dummies), pickups (no
    collision), the map boundary (its own object type isn't swept) and dead players.
  - The damage is `GE_Damage` from the enemy's ASC with `Data.Damage` and the hit result, and **no critical roll**. So
    the existing damage path decides the hit in its usual order: dash invulnerability (GE_Damage blocked), a Floating
    Shield (each shot is its own hit and uses its own shield), Health, then Second Wind.
  - `ImpactSound` (new, optional) plays with the impact effect; `IsEnemyShot()` for tests.
  - A new run already releases every projectile (`UGSProjectileSubsystem::ReleaseAll` in `StartRun`), and projectiles
    tick with the world, so they stop while the game is paused.
- **`UGSWeaponData` "Projectile" section** (enemy weapons): `ProjectileSpeed`, `ProjectileRadius` (10 cm),
  `ProjectileMesh`, `ProjectileMaterial` (slot 0), `ProjectileScale`, `ProjectileImpactEffect` and its scale,
  `ProjectileImpactSound`.
- **The bolt** (Blender, `Art/Blender/Props.blend` → `Art/Export/Props/SM_EnemyBolt.fbx` →
  `/Game/Art/Placeholders/Weapons/SM_EnemyBolt`): a faceted six-sided head (16 cm in front of the origin) with a
  tapered tail 62 cm behind it as its short trail, 78 cm long and 11 cm wide, 24 triangles, two slots:
  - `BoltCore`: `MI_EnemyBolt_Core`, hot magenta (1, 0.06, 0.4) with emissive strength 30;
  - `BoltTrail`: `MI_EnemyBolt_Trail`, (0.9, 0.03, 0.28) with emissive strength 10.

  None of the player's weapons uses magenta: the gun bullets and Lucky Coin are gold, the cards white, the tomahawk
  grey, the dynamite red, the fire orange, the orbs blue and green. In PIE the bolts read clearly against the floor and
  sky. Impact: the Niagara `DirectionalBurstLightweight` template at ×0.15 (a small flash; `RadialBurst` was tried at
  ×0.2 and ×0.06 and stayed a big spark fountain on the player). Sounds (reused, editor content): `VR_click2_Cue` on
  firing (the carbine had none) and `VR_click1_Cue` on impact.
- **The ranged cap:**
  - `UGSEnemyData::bRanged` (true on `DA_Enemy_PatrolOfficer`; every later ranged type sets it) and
    `UGSWaveSettings::MaxRangedShare` 0.4 (class default; `DA_WaveSettings` uses it).
  - `UGSWaveDirector::PurchaseWave(…, MaxRangedShare)`: a ranged pick is allowed only while (ranged + 1) ≤ share ×
    (count + 1); otherwise the pick is among the other affordable types, so the budget the wave can't spend on ranged
    enemies goes to them and the wave never ends above the share. `MaxRangedShare` 1 = the old purchase exactly.
    `GetRangedShare` (new). The `[wave]` start line of a generated wave adds "N% ranged (cap 40%)".
  - DT_Waves rows are not capped. Their ranged share: Round 1 0% (4 Beat Cops), Round 2 17% (5 + 1), Round 3 25%
    (6 + 2), Round 4 30% (7 + 3), Round 5 31% (9 + 4).
- **Generated waves, rounds 6–15, before and after the cap** (the same purchase with seed 5801, no cap vs 40%):

  | Round (budget) | Before | After |
  |---|---|---|
  | 6 (20) | 8 Beat Cops, 6 Patrol Officers (43%) | 10, 5 (33%) |
  | 7 (22.5) | 8, 7 (47%) | 10, 6 (38%) |
  | 8 (25) | 9, 8 (47%) | 11, 7 (39%) |
  | 9 (27.5) | 9, 9 (50%) | 13, 7 (35%) |
  | 10 (30) | 10, 10 (50%) | 14, 8 (36%) |
  | 11 (32.5) | 10, 11 (52%) | 14, 9 (39%) |
  | 12 (35) | 11, 12 (52%) | 15, 10 (40%) |
  | 13 (37.5) | 13, 12 (48%) | 17, 10 (37%) |
  | 14 (40) | 12, 14 (54%) | 20, 10 (33%) |
  | 15 (42.5) | 14, 14 (50%) | 20, 11 (35%) |

  A Beat Cop costs 1 and a Patrol Officer 2, so the same budget now buys **more enemies** (round 10: 22 instead of 20;
  round 14: 30 instead of 26). Over rounds 6–30 and 10 seeds the highest share was 40%.
- **Flight time** (measured in `Enemies.DodgeableShots`, game time, muzzle to the player's capsule):

  | Distance | Shot arrives after | A walking player (900 cm/s) moves |
  |---|---|---|
  | 15 m | 0.51 s | 4.6 m |
  | 20 m | 0.69 s | 6.2 m |
  | 25 m | 0.87 s | 7.8 m |

- **Frame time with 12 Patrol Officers firing** (PIE, town, god mode, round paused, `gs.Spawn PatrolOfficer 12`, all
  15–21 m out and bursting, `gs.Perf 5` twice, 120 fps cap): frame 8.34 ms (120 fps) both times, p95 8.33, max 9.30 /
  11.08; game 4.48 / 4.18, render 3.49 / 3.35, GPU 2.95 / 3.00. The bolts cost nothing visible at this count.
- **Tests:**
  - New `Enemies.DodgeableShots` and `Round.RangedCap` (see the test table).
  - **No test that assumed instant shots needed a change.** `Upgrades.FloatingShield` (a Patrol Officer's burst blocked)
    already waits for the block and then 2 s for the rest of the burst; `Map.Boundary`'s "enemy shots pass through"
    trace still checks `GetShotObjectQueryParams`, which the aim laser uses, and the bolt sweeps the same object types.
  - **Changed: `Run.GameOver`.** Not because of the shots: the run's seed is random, and in one full-suite run the
    level-up card the test always picked (choice 0) was a Floating Shield, which blocked the test's killing blow (the
    same flake `Run.NewRunResets` had in Prompt 42). It now picks a card that can't stop a killing blow (not Floating
    Shield or Second Wind), as `Run.NewRunResets` does. Everything it checks is unchanged.
  - `Upgrades.LoadedDice` missed its first rifle shot once right after an editor start (the known flake from Prompt
    44); it passed alone and in the next full run.
- **PIE check** (town, god mode, round paused): 12 Patrol Officers bursting at the player from 15–21 m: magenta bolts
  with their tails flying in, each burst's aim laser first, the `[shot]` log showing "fired at … 18 m away, 0.66 s to
  get there" and "hit … after 0.64 s".
- **Decided here, to confirm:**
  - The aim point is 35 cm above the capsule centre (the chest), in data (`AimHeightOffset`).
  - Enemy shots never crit (like before) and pass through target dummies as well as other enemies.
  - The shot's damage is set when it is fired (the enemy's Damage at that moment), like the player's projectiles.
  - The bolt and its trail are one mesh (opaque, emissive), not a particle trail.
  - Sounds: the carbine gets a fire sound (`VR_click2_Cue`) and the bolt an impact click; both are editor sounds.
- **Full suite:** 67 tests, all passing, in 179.7 s.

### Prompt 59 — Roll-over rounds and the breather
Rounds stay, but play no longer stops between them: the next round's enemies start arriving while the current round
still has a few left, every round pays its own bonus at its own clear, and every 10 rounds there is a 10 s breather.
- **Phases** (`EGSRoundPhase`): WaitingToStart, PreRound (only the 3 s countdown before round 1), Combat, **Breather**,
  GameOver. **RoundClear is gone.** `RoundNumber` is always the newest round.

  | From | To | When |
  |---|---|---|
  | PreRound | Combat | its 3 s timer; round 1 starts |
  | Combat | Combat (round N + 1 starts) | the newest round N has `RollOverRemaining` 3 or fewer enemies left (not yet spawned + telegraphing + alive) and at least one of its own enemies has died |
  | Combat | Breather | the newest round is a breather round (N % `BreatherEveryRounds` 10 = 0), so nothing rolls over, and every open round is cleared |
  | Breather | Combat | its `BreatherDuration` 10 s timer; round N + 1 starts |
  | any | GameOver | the player dies |

- **Several open rounds at once** (`UGSWaveDirector`, rewritten): one `FGSRoundWave` per open round, with its queue,
  counts (total / spawned / killed / lost / telegraphing), its living enemies, start time and spawn gap. Every enemy
  carries its round (`AGSEnemyCharacter::GetWaveRound`, set before it begins play). A round's leftovers stay and stay
  its own: a round clears when its own last enemy dies (`OnRoundCleared`); `OnRollOverReady` fires once per round.
  Events are sent after the director's state is consistent, so the GameMode can start the next round from inside them.
  - **Steady spawns:** each round places its enemies **one at a time**, the next one gap after the last, gap =
    `RoundSpawnWindow` 20 s ÷ wave size, kept between `MinSpawnGap` 0.25 s and `MaxSpawnGap` 2 s
    (`UGSRoundSettings::GetSpawnGap`). Older rounds go first. The alive cap (`MaxAliveEnemies` 30, unchanged) counts
    every open round together; while it is full nothing is banked: when room appears one enemy comes and the next is
    due one gap later.
  - **Removed:** `FGSWaveRow::SpawnInterval` / `SpawnBatchSize` (DT_Waves) and `UGSWaveSettings::GeneratedSpawnInterval`
    / `GeneratedBatchSize`. `UGSRoundSettings::RoundClearDuration` is removed too.
  - **Same seed, same waves:** a round's contents (its DT_Waves row or bought wave, then the shuffle) come from a stream
    made from the run seed and the round (`UGSRunRandom::MakeRoundStream`), so the waves don't depend on how fast the
    player kills or on anything else that drew from the run stream before. Spawn points, drops, crits and the AI stay
    on the run stream. `BuildRoundWave(Round, OutQueue)` gives a round's wave without starting it.
  - GameState counts: `WaveEnemiesTotal` / `Remaining` are the newest round's, `WaveEnemiesAlive` is every round's.
  - API: `StartRoundWave`, `StopAll(bDespawn)` (death: leftovers stay; `gs.Round.Set`: removed),
    `ForceClearOpenRounds(Why)` (skip, safety guard), `FindRoundWave`, `GetActiveRounds`, `GetNewestRound`,
    `IsRollOverReady`. The "current wave" getters report the newest open round (else the last finished one).
- **The round loop** (`AGSGameMode`): `StartRound(N)`, `TryRollOver` (held while `gs.Round.Pause` is on),
  `HandleRoundCleared`, `HandleSafetyTimerExpired`. `DA_RoundSettings` (`UGSRoundSettings`): PreRoundDuration 3,
  **RollOverRemaining 3, RoundSpawnWindow 20, MinSpawnGap 0.25, MaxSpawnGap 2, BreatherEveryRounds 10, BreatherDuration
  10, ClearSaveMaxWait 3** (new), CombatFallbackDuration 180 (now only the guard below). `SetRoundSettingsOverride`
  for tests.
  - **The breather:** after a breather round nothing rolls over: the run waits until every open round is cleared,
    then Breather for 10 s with nothing spawning, then the next round. It has no round number of its own (still N), no
    bonus and no stats row. The vacuum is the one at that round's clear.
  - **Safety guard:** the 180 s Combat fallback now only guards the wait before a breather. It starts when the breather
    round is down to where it would have rolled over; when it runs out, the stuck leftovers are removed and the open
    rounds clear as usual (their fast-clear bonus is usually missed).
  - Death: GameOver stops every open round (no spawns, telegraphs cancelled, leftovers stay), their stats records close
    without a clear, and no bonus is paid for them. Killing the leftovers afterwards pays nothing.
- **Per-round stats** (`UGSRunStats`): a record (`FGSRoundStats`) for every round of the run, with `bOpen`, `bCleared`,
  `ClearedAt` and its `Payout` (new). `FindRound(N)`, `GetRounds()`, `GetOpenRounds()`; `GetCurrentRound()` is the
  newest. Kills, cash, XP, health orbs and damage dealt count to **the enemy's round**: pickups carry their round
  (`AGSCashPickup::SourceRound`; `UGSCashPickupSubsystem::SetDropRound` around a kill's drops), and the awards take it.
  Enemies with no round (debug spawns) and dummies count to the newest round. **Damage taken counts to every open
  round**, so one hit while two rounds overlap costs both their no-damage bonus. Run-summary "rounds cleared" counts
  cleared rounds.
- **At a round's clear** (when its last enemy dies): the round is recorded as cleared, its bonus is worked out and
  paid as before (base $20; no damage between its start and its clear $15; fast clear $15 within 8 s + 1.5 s per enemy
  of its own start), every pickup on the ground is vacuumed, and **the profile is saved once that vacuum has landed**
  (at most `ClearSaveMaxWait` 3 s later; `GetRoundClearSaves`, `IsClearSavePending`). The `[round] Round N cleared`
  line is written with that save, so its cash and XP include the vacuum as before. Saves also happen at death and on
  quit, as before.
- **HUD** (WBP_HUD, MCP):
  - Round line: "ROUND 4", phase "COMBAT" / "BREATHER" / "PRE-ROUND", "7 enemies left · 12 alive" (the newest round's
    enemies left, every round's alive). The countdown shows before round 1 ("Round 1 in 3…") and in the breather
    ("Round 11 in 10…").
  - A short **"ROUND N"** banner (still "GET READY") when a round starts, also when it rolls over. **No ROUND CLEAR
    banner.**
  - **Round-clear pop-up** (new `RoundClearPopup` VerticalBox under the round info with `RoundClearPopupTitle` /
    `RoundClearPopupLines`): "Round 4 clear  +$50" and "Round clear +$20 · No damage +$15 · Fast clear +$15 (12 s / 23
    s)" (or "missed"), green, about 3 s (`RoundClearPopupDuration`), fading. It is bound to the new
    `UGSRunStats::OnRoundClearPaid`. It shares the banner queue: one banner or pop-up at a time, none while LEVEL UP!
    or the cards are up; a queued "ROUND N" is dropped once a newer round has started; pop-ups are never dropped.
  - Test hooks: `IsAnyBannerShowing`, `IsRoundClearPopupShowing`, `GetRoundClearPopupRound / Title / Lines`,
    `GetRoundClearPopupsShown`, `GetQueuedRoundClearPopupCount`.
- **Removed:** `UGSRoundSummaryWidget` (`UI/GSRoundSummaryWidget.*`), `WBP_RoundSummary` and
  `AGSPlayerController::RoundSummaryWidgetClass` / `GetRoundSummaryWidget` (BP_GSPlayerController re-saved).
- **Debug:**
  - `gs.Round.Skip`: PreRound → Combat; in Combat it removes what is left of every open round, so each clears now
    (bonus, vacuum, save) and the run moves on (the next round, or the breather); in the breather it starts the next
    round.
  - `gs.Round.Set N`: stops every open round without a bonus (leftovers removed), then round N's 3 s countdown.
  - `gs.Round.Pause 1`: freezes the phase timer and the safety guard and holds the roll-over.
  - `gs.Debug.Round 1` adds the open rounds (each with its enemies left / alive and "ready"), the roll-over point, the
    breather rule, the safety guard and "PAUSED".
- **Numbers** (`Round.RollOverPace`, game time; `gs.KillAll` once a game second from round 1's Combat):
  - **Empty time between rounds 1–6:** before, 7.75 s each (RoundClear 4 s + PreRound 3 s + the 0.75 s telegraph,
    from the settings). After, **0 s**: in every pair the next round's first enemy was on the field 4.3–5.3 s before
    the round before it cleared.

    | Rounds | Empty before | Empty after | Next round's first enemy arrived before the clear by |
    |---|---|---|---|
    | 1–2 | 7.75 s | 0 s | 5.26 s |
    | 2–3 | 7.75 s | 0 s | 5.25 s |
    | 3–4 | 7.75 s | 0 s | 5.25 s |
    | 4–5 | 7.75 s | 0 s | 5.25 s |
    | 5–6 | 7.75 s | 0 s | 4.25 s |

  - **Time to reach round 10** with `gs.KillAll` once a second: **109.1 s** after round 1's Combat started.
  - **Arrival vs the fast-clear limit, rounds 1–20** (last enemy on the field after the round starts =
    (size − 1) × gap + 0.75 s; rounds 1–8 measured, the same to 0.1 s; rounds 9–20 from the wave sizes of seed 5903):
    **no round's limit is shorter than its arrival time** (limit not changed).

    | Round | Size | Gap | Last arrives | Fast-clear limit |
    |---|---|---|---|---|
    | 1 | 4 | 2.00 s | 6.8 s | 14.0 s |
    | 2 | 6 | 2.00 s | 10.8 s | 17.0 s |
    | 3 | 8 | 2.00 s | 14.8 s | 20.0 s |
    | 4 | 10 | 2.00 s | 18.8 s | 23.0 s |
    | 5 | 13 | 1.54 s | 19.2 s | 27.5 s |
    | 6 | 16 | 1.25 s | 19.5 s | 32.0 s |
    | 7 | 16 | 1.25 s | 19.5 s | 32.0 s |
    | 8 | 19 | 1.05 s | 19.7 s | 36.5 s |
    | 9 | 20 | 1.00 s | 19.8 s | 38.0 s |
    | 10 | 22 | 0.91 s | 19.8 s | 41.0 s |
    | 11 | 24 | 0.83 s | 19.9 s | 44.0 s |
    | 12 | 27 | 0.74 s | 20.0 s | 48.5 s |
    | 13 | 28 | 0.71 s | 20.0 s | 50.0 s |
    | 14 | 29 | 0.69 s | 20.1 s | 51.5 s |
    | 15 | 32 | 0.62 s | 20.1 s | 56.0 s |
    | 16 | 33 | 0.61 s | 20.1 s | 57.5 s |
    | 17 | 35 | 0.57 s | 20.2 s | 60.5 s |
    | 18 | 37 | 0.54 s | 20.2 s | 63.5 s |
    | 19 | 39 | 0.51 s | 20.2 s | 66.5 s |
    | 20 | 40 | 0.50 s | 20.2 s | 68.0 s |

    The closest is round 4 (18.8 s against 23 s). From round 5 on every round arrives in about 20 s while the limit
    grows by 1.5 s per enemy, so the fast-clear bonus gets easier with the round (a balance note, not changed).
- **Tests:**
  - New `Round.RollOver`, `Round.Breather`, `Round.RollOverPace` (see the test table). The same-seed waves check was
    added to `Run.SameSeedSameRun`.
  - **Changed, because the new round flow made them untrue** (what each checks is otherwise unchanged):
    - `Round.PhaseLoop`: no RoundClear phase; a skip clears round 1 and round 2 starts in Combat with no countdown;
      round 10's clear → Breather → its timer → round 11 (was: RoundClear timer → PreRound).
    - `Round.WaveDirector`: runs with RollOverRemaining 0 (a settings copy) so round 1's wave is checked alone; "round
      1 cleared" by its record instead of "phase is RoundClear"; the skip clears round 2 and round 3 starts (was: the
      wave stops and the counts reset).
    - `Round.ClearBonus`: the RoundClear summary panel is gone, so it checks the HUD pop-up and each round's record and
      payout; one round at a time (RollOverRemaining 0); "RoundClear from PreRound pays nothing" became "`gs.Round.Set`
      stops an open round without a bonus"; round 9 instead of 10 for "pays the same as round 1" (10 is a breather
      round now).
    - `UI.RoundHUD`: no ROUND CLEAR banner: checks that it never shows and that round 2's pop-up does; the enemies line
      has "· N alive"; round 3 rolls in during round 2 (no countdown); the stale-banner check uses ROUND 4 (two skips
      while the cards are up) instead of ROUND CLEAR; the overlap check covers pop-ups too; XPGain 0 after the forced
      level-up (the skips' vacuums would level up again and hold the queue).
    - `Profile.PermanentCash`: saved at the clear (nothing on the ground, so at once), counted by
      `GetRoundClearSaves` (was: saved when RoundClear ended).
    - `Run.GameOver`: round 1 cleared by a skip, round 2 starts when the round timer is unpaused (was: RoundClear, then
      PreRound); "no spawns after death" compares the spawned count with the one at death (the stopped round keeps its
      count now).
    - `Run.SeedAndStats` (comment only), `Progression.LevelUpRewards` ("RoundClear works the same" became "right after
      a round's clear"), `Economy.CashPickups`, `Economy.XPOrbs`, `Economy.KillCombo`, `Economy.HealthOrbs` (they
      entered RoundClear only for its vacuum; they call the vacuum directly now), `Round.RangedCap` (`StopWave` →
      `StopAll`).
- **PIE check** (hub → `gs.Market.Return`, god mode, `gs.KillAll` every few seconds): round 2 started while round 1
  still had enemies, round 3 while round 2 did; "Round 1 clear bonus: $50 … cleared in 13.6 s of 14.0 s", then "[round]
  Round 1 cleared: … $46 collected + $50 bonus, 40 XP" and "Saved slot GSProfile (RoundClear)". The HUD showed "ROUND 3 /
  COMBAT / 6 enemies left · 0 alive" and the fading "Round 2 clear +$50 / Round clear +$20 · No damage +$15 · Fast
  clear +$15 (11 s / 17 s)" pop-up under it.
  - Found and fixed there: the `[round]` line was written at the clear, before the vacuum landed ("$0 collected"); it
    now waits for the clear's save.
- **Decided here, to confirm:**
  - The phases: PreRound only before round 1; no RoundClear; Breather after rounds 10, 20, …
  - Roll-over needs at least one of the newest round's own enemies to have died, so a round with 3 or fewer enemies
    doesn't start the next one the moment it starts.
  - The breather waits for **every** open round (older leftovers too) to be cleared.
  - The safety guard (180 s) starts once the breather round is down to where it would have rolled over, and clears the
    leftovers when it runs out.
  - Waves come from a stream per round (run seed + round), not from the run stream.
  - Enemies with no round (`gs.Spawn`) count to the newest round; enemy scaling uses the enemy's own round.
  - The save at a clear waits for its vacuum to land (at most 3 s).
  - `gs.Round.Skip` in Combat clears every open round at once (each pays its bonus); `gs.Round.Set` stops them with
    none; `gs.Round.Pause` holds the roll-over.
  - The batch and interval fields are removed (not left unused).
  - The "ROUND N" banner keeps "GET READY" as its subtitle; the pop-up shares the banner queue in its own panel.
  - `CombatFallbackDuration`'s class default is now 180 (the asset's value) instead of 60.
- **CLAUDE.md:** the run line (no RoundClear phase), "saves at each round's clear", the suite time (about 4 minutes).
- **Full suite:** 70 tests, all passing, in 249.8 s.

### Prompt 60 — One hundred enemies
Up to 100 cops alive at once at 60 fps on this PC, bigger generated waves so the cap matters, and at most 20 corpses.
Nothing about how the cops behave or look up close changed.
- **How it was measured:** PIE in the town, god mode, the round paused in PreRound, editor throttling off, `t.MaxFPS 0`
  (PIE still caps at 120 fps, so frames under 8.33 ms read 8.33). Cops were added with `spawn_enemies` (60% Beat Cops,
  40% Patrol Officers), left 12–25 s to close in and attack, then `gs.Perf 5`. "With weapons" = Barbed Wire, Dynamite ×2
  and Molotov hitting the crowd, the player's Damage at 0.01 so nobody dies and every hit still runs, XPGain 0. Each change
  is a switch in data or a cvar (below), so before and after were measured in one editor session: switched off for
  "before", on for "after", new cops spawned after each switch.
- **Before and after** (ms; frame avg / p95 / game thread avg / render avg / GPU avg):

  | Alive | Weapons | Before | After |
  |---|---|---|---|
  | 30 | none | 8.34 / 8.33 / 5.05 / 3.34 / 2.97 | 8.33 / 8.33 / 3.75 / 3.13 / 3.01 |
  | 60 | none | 8.47 / 9.17 / 7.66 / 3.65 / 3.29 | 8.34 / 8.33 / 4.83 / 3.33 / 3.32 |
  | 100 | none | 10.70 / 12.86 / 10.69 / 3.80 / 3.62 | 8.37 / 8.39 / 6.71 / 3.75 / 3.64 |
  | 30 | three area weapons | 8.45 / 8.92 / 7.50 / 4.21 / 3.04 | 8.36 / 8.36 / 6.55 / 4.20 / 3.08 |
  | 60 | three area weapons | 11.29 / 13.41 / 11.28 / 5.63 / 3.42 | 9.45 / 11.04 / 9.44 / 5.74 / 3.45 |
  | 100 | three area weapons | 15.50 / 17.71 / 15.49 / 6.56 / 3.83 | 11.97 / 14.18 / 11.97 / 6.85 / 3.91 |

  - The worst case (100 with weapons) went from failing 60 fps at p95 (17.7 ms) to 12.0 ms average and 14.2 ms p95: 4.7
    ms of room on average. Every case is game-thread bound; the GPU stays under 4 ms.
  - A first session in a freshly started editor read higher for the same setup (100 none 13.85 ms, 100 with weapons
    21.66 ms, GPU 6.4 ms), probably shader warm-up; only one session's numbers are compared.
- **What cost the most** (`stat dumpave`, 100 cops with weapons, before): character movement 3.4 ms; the physics bodies
  of each cop's body mesh following its animation (`UpdateKinematicBonesToAnim`) 2.1 ms; animation update and evaluation
  ~2.5 ms; ~285 live RadialBurst hit effects (Barbed Wire's, 40 cops in the ring hit every 0.5 s) ~4 ms; StateTree 0.4 ms,
  sight perception 0.3 ms.
- **The steps**, each measured at 100 cops with the three weapons (frame avg):

  | Step | Frame avg | Gain |
  |---|---|---|
  | Before | 15.50 ms | |
  | 1. Body mesh without collision while alive (its physics bodies don't follow the animation; the capsule takes every hit already) | 12.43 ms | −3.1 ms (character movement 3.4 → 1.8 ms: no more sweeps against 100 meshes; kinematic bones 2.1 → 0) |
  | 2. No pose update off screen | 12.39 ms | ~0 (the crowd is on screen, and cops behind the camera still cast shadows into view) |
  | 3. Animation update rate optimization (far bodies animate less often) | 12.31 ms | −0.1 ms (the crowd is close) |
  | 4. No overlap events on the cops' capsules | 12.20 ms | −0.1 ms |
  | 5. Pooled one-shot effects (Niagara's component pool) | 11.97 ms | −0.2 ms |

  Stopped there: 100 holds 60 fps with room. Not touched, since nothing more was needed: StateTree tick rate,
  health-bar widgets, damage numbers and pickups.
- **The switches** (`UGSEnemyData`, "Performance", all on by default; `DA_Enemy_BeatCop` / `_PatrolOfficer` use the class
  defaults): `bMeshCollisionOnlyWhenDead`, `bSkipPoseWhenOffScreen`, `bAnimUpdateRateOptimizations`,
  `bNoCapsuleOverlapEvents`. Applied in `AGSEnemyCharacter::ApplyEnemyDataVisuals`.
  - Why nothing changes: the CharacterMesh profile already ignores Visibility, so the player's shots always hit the
    capsule; projectiles and enemy bolts now stop on the capsule instead of the mesh (the capsule surrounds the body).
    Gameplay reads no bone of a cop (melee is an overlap query in front, the muzzle is the gun mesh's socket). Nothing
    listens to a cop's overlap events.
  - At death (`OnDeathStarted`) the mesh gets its pose refreshed, the Ragdoll profile, its physics state rebuilt at the
    current pose and full pose updates again, then simulates as before (`Enemies.CorpseLimit` checks the ragdoll starts
    at the cop).
- **Pooled effects:** `Core/GSEffects.*` (`GSEffects::SpawnOneShot`) spawns one-shot effects from Niagara's pool
  (`ENCPoolMethod::AutoRelease`): Barbed Wire hits, projectile impacts (Floating Gun, Razor Cards, Lucky Coin, enemy
  bolts), Tomahawk hits, Dynamite and Powder Keg blasts. Same effect, no new component each time. Cvar
  `gs.Debug.NoFXPool 1` turns the pool off (for measuring).
- **The cap and the waves** (`DA_WaveSettings`; the asset uses the class defaults, which changed): `MaxAliveEnemies` 30 →
  **100**; `BudgetPerRound` 2.5 → **5** (budget 5 + 5 × round, placeholder); DT_Waves rounds 1–5 unchanged. Enemies per
  generated round (seed 6101; "up to N alive" = if nothing dies while the round arrives, plus the 3 left from the round
  before, capped; roll-over needs the newest round down to 3, so this is the most one round brings):

  | Round | Before: enemies (budget) | up to alive | After: enemies (budget) | ranged | up to alive |
  |---|---|---|---|---|---|
  | 6 | 15 (20) | 18 | 25 (35) | 40% | 28 |
  | 7 | 16 (22.5) | 19 | 31 (40) | 29% | 34 |
  | 8 | 19 (25) | 22 | 33 (45) | 36% | 36 |
  | 9 | 20 (27.5) | 23 | 36 (50) | 39% | 39 |
  | 10 | 23 (30) | 26 | 40 (55) | 38% | 43 |
  | 11 | 23 (32.5) | 26 | 44 (60) | 36% | 47 |
  | 12 | 26 (35) | 29 | 47 (65) | 38% | 50 |
  | 13 | 27 (37.5) | 30 (cap) | 50 (70) | 40% | 53 |
  | 14 | 29 (40) | 30 (cap) | 54 (75) | 39% | 57 |
  | 15 | 31 (42.5) | 30 (cap) | 58 (80) | 38% | 61 |
  | 16 | 33 (45) | 30 (cap) | 61 (85) | 39% | 64 |
  | 17 | 34 (47.5) | 30 (cap) | 65 (90) | 38% | 68 |
  | 18 | 36 (50) | 30 (cap) | 68 (95) | 40% | 71 |
  | 19 | 38 (52.5) | 30 (cap) | 73 (100) | 37% | 76 |
  | 20 | 40 (55) | 30 (cap) | 76 (105) | 38% | 79 |

  The old cap held every round from 13 back. Now one round alone reaches 100 from round 26 (97 cops; round 27 has 100,
  round 30 111). With overlapping rounds and slow kills the cap is reached earlier: `Round.AliveCap100` had 250 cops in
  three open rounds with 100 alive and 148 waiting.
- **Corpses:** `DA_WaveSettings.MaxCorpses` 20 (0 = no limit). `UGSWaveDirector::RegisterCorpse` (called from
  `AGSEnemyCharacter::OnDeathStarted` on the server, every enemy, wave or not) keeps them oldest first and destroys the
  oldest when there are more. `CorpseLifetime` (5 s) still removes them as before. `GetCorpseCount`,
  `GetCorpsesRemovedByLimit` (tests).
- **Cash, XP and health orbs:** unchanged rules (flat per cop); bigger waves simply drop more.
- **Test speed** (every check kept; durations from the full suite, before → after):

  | Test | Before | After | How |
  |---|---|---|---|
  | `Round.RollOverPace` | 29.4 s | 11.9 s | game time ×10 instead of ×4. A report on the real pacing, so the real timers stay; it still plays rounds 1–10 and logs everything it did |
  | `Enemies.DodgeableShots` | 18.0 s | 9.2 s | game time ×2 instead of ×1: flight times are game time, a frame (~0.02 s) is well inside the 0.08 s tolerance |
  | `Round.RollOver` | 14.1 s | 5.5 s | after checking the asset's values, a settings copy with a 5 s spawn window (max gap 0.5 s); its checks read the copy |
  | `Round.Breather` | 13.0 s | 6.4 s | the same copy plus a 2.5 s breather; the asset's 10 s is checked first, the breather's length against the copy |

  The other tests' durations didn't change (the next slowest: `Upgrades.FloatingGun` 9.9 s, `UI.RoundHUD` 9.7 s,
  `Upgrades.Molotov` 8.7 s, `Round.ClearBonus` 7.3 s, `Upgrades.Dynamite` 7.2 s, `Round.WaveDirector` 6.9 s).
  **Full suite: 251.3 s (70 tests, this build, before the speed-ups) → 213.9 s (73 tests).** It was 249.8 s in Prompt 59.
- **New tests:** `Round.AliveCap100`, `Round.BiggerWaves`, `Enemies.CorpseLimit` (see the test table). Every existing
  test passed unchanged with the new cap, budget and switches (the full suite ran once before the test changes: 70 of 70).
- **Pace report with the bigger waves** (`Round.RollOverPace`): round 10 starts 117.3 s after round 1's Combat with
  `gs.KillAll` once a second (was 109.1 s); no empty time between rounds 1–6 (the next round's first cop arrives 4.3–5.3 s
  before the clear); every round's enemies arrive in about 20 s, and every fast-clear limit is longer than that (round
  20: 76 cops, limit 122 s).
- **PIE check** (town, god mode, `gs.Round.Set 27`, no weapons): round 27's 101 cops arrived over 20 s; the log said
  "alive cap 100 reached; its spawns wait for room", the HUD read "101 enemies left · 100 alive", and `gs.Perf 5` with 100
  alive: frame 8.35 ms (the 120 fps cap), p95 8.34, game 6.59, render 3.73, GPU 4.76. The crowd around the robber looked
  as before (low-poly cops, caps, batons, wind-up glow, magenta bolts).
- **Decided here, to confirm:**
  - The corpse limit destroys the oldest corpse (it doesn't fade or freeze it).
  - The corpse limit lives in `DA_WaveSettings` and counts every enemy's corpse (`gs.Spawn` ones too).
  - Projectiles and enemy bolts stop on the cop's capsule instead of its body mesh while it is alive.
- **Full suite:** 73 tests, all passing, in 213.9 s.

### Prompt 59b — Fix: no aim laser on the Patrol Officer
Written to run between Prompts 59 and 60; it ran after Prompt 60, so it keeps its number and comes after 60 here.
- **The problem:** during its 0.4 s aim the Patrol Officer showed an orange beam from its gun along its facing: the aim
  laser left over from the instant-shot days, which read as the old instant shot.
- **The fix (data only, no source change in the game):** `DA_Enemy_PatrolOfficer.AimLaserMaterial` cleared (was
  `M_EnemyLaser`). `AGSEnemyCharacter` only shows the laser for enemy data that has the material, so no beam shows at any
  point of its aim or burst. The warning before a burst is now the 0.4 s body glow alone (`TelegraphAmount`, as before).
  Aim time (0.4 s), burst (4 shots), bolts and damage are unchanged.
- **Kept:** the aim-laser code (`AimLaserComponent`, `UpdateAimLaser`, `UGSEnemyData::AimLaserMaterial`) for the Sniper
  (Prompt 65), and `M_EnemyLaser`, which the spawn telegraph markers use (`DA_WaveSettings.TelegraphMaterial`).
- **Nothing else draws a line from a Patrol Officer:**
  - `GA_EnemyFire` calls `MulticastShotFired` with no hit, so no impact effect plays there.
  - `DA_PatrolCarbine` has no `MuzzleEffect` and no `ImpactEffect` set, so only the fire sound plays at the gun.
  - The 0.06 s tracer flash was removed in Prompt 58.
  - The only other line is the `gs.Debug.Weapons` debug draw (off unless that cvar is on).
  - What the player sees is the glow, the magenta bolts and their impact bursts where they land. There is no muzzle
    flash today (a known issue).
- **Tests changed:** only `Enemies.CopLooks`. Its "the aim laser shows" check became: no AimLaserMaterial on the data,
  no laser at rest, no laser in any frame of the aim and the burst (it waits until the ranged attack has ended and checks
  the burst fired its 4 bolts), and every body slot glows during the aim. No other test expected the Patrol Officer's
  laser: `Map.Boundary` checks the shot object query (`GetShotObjectQueryParams`), not the laser. Still 73 tests.
- **PIE check** (town, god mode, round paused, `gs.Spawn PatrolOfficer 3`, `slomo 0.1`): the officers kept 14–21 m away
  and fired; screenshots during an aim and a burst showed the glowing body and a magenta bolt in the air, and no beam.
- **Full suite:** 73 tests, all passing, in 214.3 s.

---

## Console commands and CVars

Non-Shipping cheats. They run on the server; clients forward them through `ServerExecDebugCommand`.

| Command / CVar | What it does |
|---|---|
| `gs.Spawn <Enemy> [Count=1]` | Spawn enemies around the player via the spawn finder (`BeatCop`, `PatrolOfficer` or a full `DA_Enemy_*` name). |
| `gs.KillAll` | Kill every living enemy through GE_Damage, credited to the player (they drop cash and XP orbs; no XP until the orbs are collected). |
| `gs.God [0\|1]` | Toggle/set damage immunity (one `State.Invulnerable` count). |
| `gs.GiveWeapon <Weapon>` | Equip `Rifle` / `Pistol` / `PatrolCarbine` with a full mag (replaces the equipped one if both slots are full). |
| `gs.SetAttr <Attr> <Value>` | Set a player attribute base value (Health, MaxHealth, MoveSpeed, Damage, FireRate, PickupRadius, XPGain, ReloadSpeed). |
| `gs.Upgrade.Grant <Name> [Count=1] [Rarity=Common]` | Add picks of an upgrade at a rarity (Common, Rare, Epic, Legendary or C / R / E / L; Count and Rarity in any order; whole-unit upgrades have no Rare) (`ReloadSpeed`, `Damage`, `MoveSpeed`, `FireRate`, `Magnet`, `BiggerMags`, `ExtraJump`, `DashCharge`, `Leech`, `LoadedDice`, `PiercingRounds`, `PowderKeg`, `FloatingShield`, `SecondWind`, `FloatingGun`, `RazorCards`, `BarbedWire`, `Dynamite`, `LuckyCoin`, `Molotov`, `Tomahawk` or `DA_Upgrade_*`). E.g. `gs.Upgrade.Grant ExtraJump 45`, `gs.Upgrade.Grant Damage 2 Epic`. |
| `gs.Upgrade.List` | List every upgrade by category (Stat Boosts, Abilities, Legendary abilities, Weapons) with this run's picks by rarity, total value, Common pick value, cap and description, plus ReloadSpeed / Damage / FireRate / MoveSpeed / PickupRadius / ExtraJumps / MagazineSizeMultiplier / HealthOnKill / CritChance / Pierce / PowderKeg / Shields / SecondWinds / FloatingGunLevel / FloatingGunPower, the magazine (ammo / size), the dash charges (max = base + ExtraDashCharges, ready now), the floating shields (ready / each recharge / hits blocked), the Floating Gun (picks, power, guns, shots and target), Razor Cards (picks, power, damage per card, cards a second, cards thrown, target), Barbed Wire (picks, power, radius, damage per tick, ticks, hits, enemies inside on the last tick), Dynamite (picks, power, damage per blast, blast radius, interval, lead factor, throws, blasts and how many caught their target, hits, hits of the last blast), Lucky Coin (picks, power, damage per hit, bounces, interval, flips, hits, hits of the last flip, target), Molotov (picks, power, damage per burn, patch radius, interval, lead factor, throws, patches lit and how many caught their target at the first burn, patches burning now, burns, hits), Tomahawk (picks, power, damage per hit, tomahawks a throw, interval, throws, tomahawks thrown, in the air, hits), Second Wind (left / used this run) and Powder Keg (stacks, chance per kill, blast damage and radius). The Stats line also shows the slide / dash speed ratio (MoveSpeed / base walk speed, at least 1), the current slide speed cap and dash distance. |
| `gs.Upgrade.Offer` | Queue a level-up offer without the level (opens at once if none is open) and print the open offer. Refused in GameOver. |
| `gs.Upgrade.Choose <0-2>` | Take a choice of the open offer; the next queued offer opens. |
| `gs.Upgrade.Reroll` | Reroll the open offer for one of this run's rerolls (refused with none left or no offer). |
| `gs.Molotov.Patches [Count=4]` | Light Molotov fire patches 4 m around the local player at the current Molotov power (they burn for the usual 4 s). For frame-time checks (Prompt 51). |
| `gs.HealthOrb.Spawn [Count=1]` | Drop health orbs around the local player (no drop roll). At full health they wait on the ground until the player is hurt. |
| `gs.XP.Add <N>` | Grant N raw XP to the local player (XPGain not applied; may level up several times). |
| `gs.XP.SetLevel <N>` | Reset to level 1, then add exactly the XP for level N (0 XP into it; no cap, only the int32 XP limit at about level 11,960). |
| `gs.Profile.AddCash <N>` | Add N to the permanent cash balance (local profile; saved at the next save point). |
| `gs.Profile.Reset` | Wipe the profile: delete the save file, cash 0. |
| `gs.Profile.Save` | Save the profile now. |
| `gs.Profile.Print` | Print slot, save version, cash and owned items to the log and screen. |
| `gs.Damage.Self <n>` | Damage the local player through GAS. |
| `gs.Perf [Seconds=5]` | Log avg/p95/max frame time, game/render/GPU time and enemies alive. |
| `gs.Round.Skip` | PreRound → Combat; in Combat, remove what is left of every open round so each clears now (bonus, vacuum, save) and the run moves on (next round or the breather); in the breather, start the next round; nothing in GameOver. |
| `gs.Round.Set <N>` | Stop every open round without a bonus (leftovers removed), then round N's 3 s countdown. |
| `gs.Round.Pause [0\|1]` | Freeze / resume the phase timer and the safety guard; while paused nothing rolls over. |
| `gs.Run.Seed [N]` | Town: print the run seed, or restart the run (stream + stats, round 1) with seed N. Hub: print / set the next run's seed (0 = random); the portal passes it on. |
| `gs.Run.Stats` | Print run stats to log and screen (works on clients), plus this round's stats and the last round-clear bonus. Since Prompt 48 also the damage dealt by source (rifle/pistol, Floating Gun, Razor Cards, Barbed Wire, Powder Keg, Dynamite (Prompt 49), Lucky Coin (50), Molotov (51), Tomahawk (52), other) and the health orbs dropped / collected / healed. |
| `gs.Market.Teleport` / `gs.Market.Return` | Debug shortcuts. Teleport (in a run) opens the hub map (`AGSGameMode::OpenHubMap`). Return (in the hub) starts a run like the portal (`AGSHubGameMode::StartRun`). |
| `gs.Editor.RemoveSublevel <ShortName>` | Editor only: remove a streaming sublevel from the open map (MCP can't); save the map afterwards. |
| `gs.Editor.BuildEnemyStateTree [melee\|ranged\|all]` | Editor only: regenerate ST_BeatCop / ST_PatrolOfficer from code. |
| `gs.Debug.Round 1` (cvar) | On-screen round, phase, time left, area, wave counts (alive in every round, the newest round's remaining / total), enemy HP/DMG multipliers, and since Prompt 59 the open rounds (each with its enemies left / alive and "ready"), the roll-over point, the breather rule, the safety guard and "PAUSED". |
| `gs.Debug.Spawns 1` (cvar) | Draw the spawn ring and chosen points (green ring, yellow fallback) for 8 s. |
| `gs.Debug.Boundary 1` (cvar) | Draw the map boundary's play area (walls + ceiling) every frame. |
| `gs.Debug.Weapons 1` (cvar) | Debug-draw shots and log enemy shots (`[shot]`). |
| `gs.Debug.DamageLog 1` (cvar) | Log every health loss (`[dmg] victim took N from X (health) at t=`). |
| `gs.Debug.NoFXPool 1` (cvar) | Prompt 60: spawn one-shot hit, impact and blast effects without Niagara's component pool (for measuring the pool's gain). |
| `gs.Debug.AutoPickUpgrades 1` (cvar) | Playtests from the console: every open upgrade offer is taken a frame later. It picks an unowned weapon while a slot is free, else an owned weapon, else the rarest card. An offer that is all Common with no weapon is rerolled first while rerolls last. Logs `[upgrade] auto-pick (debug): …`. |
| `gs.Wave.MaxAlive <N>` (cvar) | Override the alive cap (0 = DA_WaveSettings). |
| `gs.Cash.Lifetime <s>` (cvar) | Lifetime of uncollected cash pieces (0 = DA_CashSettings). |
| `GSFire`, `GSPress <InputTag>`, `GSAimAt X Y Z`, `GSJump` | Exec on the controller: drive fire, any ability input, the camera, or a jump tap from the console. |
| `GSAim 1\|0`, `GSSwapShoulder`, `GSRun <Seconds>` | Exec on the controller (Prompt 56): aim down sights on / off, swap the camera shoulder, hold the move input toward the camera's facing for that long (game time). For camera and animation checks from MCP. |

Launch arg: `-GSSeed=N` fixes the run seed.

MCP toolset `banditballer_toolset.toolsets.BanditBallerTools`:
- `spawn_enemies(type, count)` (PIE)
- `get_player_attributes()` (PIE): attributes plus Ammo, Cash (profile balance), Level, XP, XPForNextLevel, TotalXP,
  Combo, ComboMultiplier, ComboTimeLeft, God, Invulnerable, Dead.
- `list_enemies_alive()` (PIE)
- `simulate_upgrade_offers(count=10000, seed=1, owned="")` (no PIE needed): rolls DA_UpgradePool offers with its own stream (with the comma-separated `owned` upgrades counted as one pick each, e.g. three weapons for full slots) and
  returns JSON text with how often each upgrade and rarity appears.
- Editor tools (Prompt 56, no PIE needed): `export_asset_fbx(asset_path, file_path)` (a skeletal or static mesh to FBX,
  LOD 0, inside the project folder), `import_mesh_file(file_path, destination_path, asset_name)` (Unreal's default
  importer; replaces an existing asset in place, which is how placeholder meshes are updated), `describe_ref_pose(mesh_path,
  bones="", max_bones=0)` (a skeletal mesh's reference pose per bone) and `console_command(command)` (one console command
  in PIE, else in the editor world).

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
| CritChance | Chance that a hit from the player's weapons is critical; past 1 every hit crits and the rest is the triple-hit chance (TripleCritMultiplier 3) (GSCombat::RollCriticalHit, run stream; damage × DA_CombatSettings CritMultiplier 2). Constructor default 0, clamped to >= 0 (no upper clamp since Prompt 44). Raised by Loaded Dice. Not replicated yet. |
| Pierce | Targets a player shot passes through (UGSGameplayAbility_Fire::TraceShot): up to 1 + Pierce targets, each once, full damage; walls stop it. Constructor default 0, min 0, rounded. Raised by Piercing Rounds. Not replicated yet. |
| SecondWinds | Second Winds left: a hit that would take Health to 0 uses one instead (UGSSecondWindComponent on the PlayerState): Health goes to 30% of MaxHealth (whole points), 2 s of State.Invulnerable, one stack of the upgrade removed. Constructor default 0, min 0, rounded. Raised by Second Wind. Not replicated yet. |
| Shields | Floating shields: each blocks one whole hit in the damage path, then recharges for DA_CombatSettings ShieldCooldown (UGSShieldComponent on the PlayerState; drawn by AGSCharacter's UGSFloatingShieldsComponent). Constructor default 0, min 0, rounded. Raised by Floating Shield. Not replicated yet. |
| PowderKeg | Powder Keg stacks: each of the player's enemy kills explodes with chance stacks × DA_CombatSettings ChancePerStack (3%; past 100% the rest is the chance of a double-size blast, 6 m; run stream; AGSGameMode::RollPowderKegChance / ExplodePowderKeg). The blast is fixed: BlastDamage × Damage in BlastRadius. Constructor default 0, min 0, rounded. Not replicated yet. |
| FloatingGunLevel | Floating Gun picks: 1 = a gun over the right shoulder, 2+ = a second over the left (never more than two), each shooting the nearest enemy (preferring different ones) with pooled projectiles (UGSFloatingGunComponent on the character, AGSFloatingGun, DA_FloatingGunSettings). Constructor default 0, min 0. Raised by Floating Gun. Not replicated yet. |
| FloatingGunPower | Floating Gun weapon power (the total value of the Floating Gun picks: picks 1 and 2 add the rarity multiplier − 1, later picks the multiplier): each point adds DA_FloatingGunSettings DamageBonusPerPower / FireRateBonusPerPower (+10%) to both guns; fractions count. Constructor default 0, min 0. Not replicated yet. |
| RazorCardsLevel | Razor Cards picks: 1 or more = the player throws homing cards at the nearest enemy (UGSRazorCardsComponent on the character, DA_RazorCardsSettings). Constructor default 0, min 0, rounded. Not replicated yet. |
| RazorCardsPower | Razor Cards weapon power (the first pick adds the rarity multiplier − 1, later picks the multiplier): each point adds DA_RazorCardsSettings DamageBonusPerPower / FireRateBonusPerPower (+10%); fractions count. Constructor default 0, min 0. Not replicated yet. |
| BarbedWireLevel | Barbed Wire picks: 1 or more = a ring on the ground around the player hurts every enemy inside it every 0.5 s (UGSBarbedWireComponent on the character, DA_BarbedWireSettings). Constructor default 0, min 0, rounded. Not replicated yet. |
| BarbedWirePower | Barbed Wire weapon power (the first pick adds the rarity multiplier − 1, later picks the multiplier): each point adds DA_BarbedWireSettings DamageBonusPerPower (+10%) and RadiusBonusPerPower (+5%); fractions count. Constructor default 0, min 0. Not replicated yet. |
| DynamiteLevel | Dynamite picks: 1 or more = every 4 s a stick is lobbed at the enemy in range with the most neighbours and explodes where it lands (UGSDynamiteComponent on the character, DA_DynamiteSettings). Constructor default 0, min 0, rounded. Not replicated yet. |
| DynamitePower | Dynamite weapon power (the first pick adds the rarity multiplier − 1, later picks the multiplier): each point adds DA_DynamiteSettings DamageBonusPerPower (+10%) and RadiusBonusPerPower (+5%); fractions count. Constructor default 0, min 0. Not replicated yet. |
| LuckyCoinLevel | Lucky Coin picks: 1 or more = every 1.5 s a coin at the nearest enemy that bounces from enemy to enemy (UGSLuckyCoinComponent on the character, DA_LuckyCoinSettings). Constructor default 0, min 0, rounded. Not replicated yet. |
| LuckyCoinPower | Lucky Coin weapon power (the first pick adds the rarity multiplier − 1, later picks the multiplier): each point adds DA_LuckyCoinSettings DamageBonusPerPower (+10%; fractions count), and every PowerPerBounce (2) full points add a bounce. Constructor default 0, min 0. Not replicated yet. |
| MolotovLevel | Molotov picks: 1 or more = every 5 s a bottle at the nearest enemy that leaves a fire patch on the ground (UGSMolotovComponent on the character, DA_MolotovSettings). Constructor default 0, min 0, rounded. Not replicated yet. |
| MolotovPower | Molotov weapon power (the first pick adds the rarity multiplier − 1, later picks the multiplier): each point adds DA_MolotovSettings DamageBonusPerPower (+10% burn damage) and RadiusBonusPerPower (+5% patch radius); fractions count. Constructor default 0, min 0. Not replicated yet. |
| TomahawkLevel | Tomahawk picks: 1 or more = every 3 s a tomahawk at the nearest enemy that flies out 18 m and back (UGSTomahawkComponent on the character, DA_TomahawkSettings). Constructor default 0, min 0, rounded. Not replicated yet. |
| TomahawkPower | Tomahawk weapon power (the first pick adds the rarity multiplier − 1, later picks the multiplier): each point adds DA_TomahawkSettings DamageBonusPerPower (+10%; fractions count), and every PowerPerTomahawk (3) full points add a tomahawk. Constructor default 0, min 0. Not replicated yet. |
| IncomingDamage | Meta attribute: GE_Damage writes it, the set converts it to Health loss and records the instigator. |

**Gameplay tags** (native, `Abilities/GSGameplayTags.*`; none in config):
- State: `State.Dead`, `State.Sliding`, `State.Dashing`, `State.Firing`, `State.Reloading`,
  `State.Invulnerable`, `State.Attacking`, `State.SafeZone` (hub: blocks GA_Fire, GE_Damage ignores the target)
- Ability: `Ability.Fire`, `Ability.Reload`, `Ability.Slide`, `Ability.Dash`, `Ability.SwapWeapon`,
  `Ability.Attack.Melee`, `Ability.Attack.Ranged`
- Cooldown: `Cooldown.Slide`, `Cooldown.Dash`
- SetByCaller data: `Data.UpgradeValue` / `Data.UpgradeMultiplier` / `Data.UpgradePicks` (level-up upgrade effects: total value, 1 + total, picks), `Data.Damage`, `Data.Critical` (1 on a critical hit's damage spec, 2 on a triple hit), `Data.Heal` (health orbs), `Data.Duration` (timed state effects: Second Wind invulnerability), `Data.Cooldown`,
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
| `GE_Upgrade_ReloadSpeed` | `UGSUpgradeEffect_ReloadSpeed` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Multiply (Additive) 1 + total on ReloadSpeed (Data.UpgradeMultiplier) → +10% of base per Common pick (BaseValue on the DA_Upgrade_*). |
| `GE_Upgrade_Damage` | `UGSUpgradeEffect_Damage` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Multiply (Additive) 1 + total on Damage (Data.UpgradeMultiplier) → +10% of base per Common pick (BaseValue on the DA_Upgrade_*). |
| `GE_Upgrade_MoveSpeed` | `UGSUpgradeEffect_MoveSpeed` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Multiply (Additive) 1 + total on MoveSpeed (Data.UpgradeMultiplier) → +8% of base per Common pick (BaseValue on the DA_Upgrade_*). |
| `GE_Upgrade_ExtraJump` | `UGSUpgradeEffect_ExtraJump` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on ExtraJumps (Data.UpgradeValue; +1 per Common pick). |
| `GE_Upgrade_DashCharge` | `UGSUpgradeEffect_DashCharge` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on ExtraDashCharges (Data.UpgradeValue; +1 per Common pick). |
| `GE_Upgrade_FireRate` | `UGSUpgradeEffect_FireRate` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Multiply (Additive) 1 + total on FireRate (Data.UpgradeMultiplier) → +8% of base per Common pick (BaseValue on the DA_Upgrade_*). |
| `GE_Upgrade_Magnet` | `UGSUpgradeEffect_Magnet` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Multiply (Additive) 1 + total on PickupRadius (Data.UpgradeMultiplier) → +15% of base per Common pick (BaseValue on the DA_Upgrade_*). |
| `GE_Upgrade_BiggerMags` | `UGSUpgradeEffect_BiggerMags` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Multiply (Additive) 1 + total on MagazineSizeMultiplier → +15% per Common pick. |
| `GE_Upgrade_Leech` | `UGSUpgradeEffect_Leech` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on HealthOnKill (Data.UpgradeValue; +1 per Common pick). |
| `GE_Upgrade_PiercingRounds` | `UGSUpgradeEffect_PiercingRounds` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on Pierce (Data.UpgradeValue; +1 per Common pick). |
| `GE_Upgrade_PowderKeg` | `UGSUpgradeEffect_PowderKeg` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on PowderKeg (Data.UpgradeValue; +1 per Common pick). |
| `GE_Upgrade_FloatingShield` | `UGSUpgradeEffect_FloatingShield` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on Shields (Data.UpgradeValue; +1 per Common pick). |
| `GE_Upgrade_SecondWind` | `UGSUpgradeEffect_SecondWind` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on SecondWinds (Data.UpgradeValue; +1 per Common pick). A used Second Wind takes one pick back. |
| `GE_Upgrade_FloatingGun` | `UGSUpgradeEffect_FloatingGun` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive picks on FloatingGunLevel (Data.UpgradePicks) and total on FloatingGunPower (Data.UpgradeValue). |
| `GE_Upgrade_RazorCards` | `UGSUpgradeEffect_RazorCards` | Infinite, one per upgrade; Additive picks on RazorCardsLevel (Data.UpgradePicks) and total on RazorCardsPower (Data.UpgradeValue). |
| `GE_Upgrade_BarbedWire` | `UGSUpgradeEffect_BarbedWire` | Infinite, one per upgrade; Additive picks on BarbedWireLevel (Data.UpgradePicks) and total on BarbedWirePower (Data.UpgradeValue). |
| `GE_Upgrade_Dynamite` | `UGSUpgradeEffect_Dynamite` | Infinite, one per upgrade; Additive picks on DynamiteLevel (Data.UpgradePicks) and total on DynamitePower (Data.UpgradeValue). |
| `GE_Upgrade_LuckyCoin` | `UGSUpgradeEffect_LuckyCoin` | Infinite, one per upgrade; Additive picks on LuckyCoinLevel (Data.UpgradePicks) and total on LuckyCoinPower (Data.UpgradeValue). |
| `GE_Upgrade_Molotov` | `UGSUpgradeEffect_Molotov` | Infinite, one per upgrade; Additive picks on MolotovLevel (Data.UpgradePicks) and total on MolotovPower (Data.UpgradeValue). |
| `GE_Upgrade_Tomahawk` | `UGSUpgradeEffect_Tomahawk` | Infinite, one per upgrade; Additive picks on TomahawkLevel (Data.UpgradePicks) and total on TomahawkPower (Data.UpgradeValue). |
| (native, no asset) | `UGSTimedInvulnerabilityEffect` | Has duration (SetByCaller `Data.Duration`); grants `State.Invulnerable`. Second Wind applies it for 2 s (DA_CombatSettings `SecondWindInvulnerableEffect` can replace it). |
| `GE_Upgrade_LoadedDice` | `UGSUpgradeEffect_LoadedDice` | Infinite, one per upgrade (SetByCaller totals, Prompt 41); Additive total on CritChance (Data.UpgradeValue; +0.05 per Common pick) (MaxStacks 20 on the upgrade). |
| (native, no asset) | `UGSHealEffect` | Instant; Health + SetByCaller `Data.Heal`, clamped to MaxHealth. AGSGameMode::AwardHealthPickup applies it when a health orb is collected (DA_CashSettings `HealthOrbHealEffect` can replace it). |
| (native, no asset) | `UGSHealthOnKillEffect` | Instant; Health + the target's HealthOnKill (captured live). AGSGameMode applies it to the killer per enemy kill (`HealthOnKillEffect`). |

**Abilities:**
- Player (`DA_PlayerAbilitySet`): `GA_Fire`, `GA_Reload`, `GA_SwapWeapon`, `GA_Slide`, `GA_Dash`.
- Enemies (`DA_EnemyAbilities_*`): `GA_MeleeAttack`, `GA_EnemyFire`.

## Automation tests (73, all passing, about 214 s)

`Tests/GSGameplayTests.cpp`. Each test opens L_Town_Graybox in the editor if needed (`Map.Hub`: L_Hub_Graybox),
starts PIE, waits for the player, removes enemies, runs and ends PIE. Stop PIE before running. While working, run
the area's tests; run the full suite once per prompt before the commit (see **Workflow update — faster test runs**).
`UI.RoundHUD`, `Round.PhaseLoop`, `Round.ClearBonus`, `Map.TownLayout` and `Upgrades.DashCharge` run game time ×4 and
`Upgrades.ExtraJump` ×3 (`SpeedUpWorld`). `Economy.HealthOrbs` also runs ×4. `Upgrades.FireRateAndMagnet` measures its shots at ×1 (throttling off) and
runs its magnet part at ×4. `Upgrades.FloatingGun` and `Upgrades.FloatingGunLevels` run at ×4. `Weapons.HeldFire` runs at ×4, and ×16 for its slow-frame burst. `Upgrades.MoveSpeedSlideDash` measures at ×1 (throttling off) and `Upgrades.MoveSpeedCover` runs at ×3. `UI.RoundHUD` also shortens its HUD's banners. `Round.RollOver`, `Round.Breather` and `Round.ClearBonus` run at ×4 (Prompt 59); since Prompt 60 `Round.RollOver` and `Round.Breather` also use shorter round timers on a settings copy, `Round.RollOverPace` runs at ×10 and `Enemies.DodgeableShots` at ×2. `Round.AliveCap100` and `Enemies.CorpseLimit` run at ×4.

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
| `Round.PhaseLoop` | PreRound on start, timer → Combat; no market phases and no RoundClear in the enum, a Breather phase; Combat is untimed and round 1's wave runs; a skip clears round 1 and round 2 starts in Combat (no countdown), again → round 3; gs.Round.Set 10 → PreRound, open rounds stopped; round 10's clear → Breather (still round 10) for BreatherDuration; its timer → round 11 in Combat; gs.Round.Set 5; death → GameOver (no end time, a skip doesn't leave it), still GameOver in round 5 after the PreRound time. |
| `Player.ReturnToHubRestoresInput` | Die → run summary with UI input and the cursor → R once the input lock is over (the widget's key path) → returning → the hub map (AGSHubGameMode, no run) → game input back, no cursor, no summary, pawn not dead. |
| `Run.GameOver` | Return refused while alive; run time format; a level-up pick + 3 upgrade stacks, round 1 with 2 kills, a pickup and its clear bonus; round 2 wave running with enemies to come; an offer open at death (paused) → death: GameOver, still round 2, wave stopped (no telegraphs, nothing left to spawn), living enemies stay, untimed, a skip does nothing, offer closed and unpaused; debug XP after death levels up with no offer or pause; run time recorded, profile saved; summary shown with UI input + cursor, locked (R and the button do nothing); 3 s later: no spawns or new enemies, no round clear / bonus, run time frozen, same cash; killing the leftovers clears nothing; summary lines (rounds, level, kills, combo, cash earned, total cash, run time, seed, upgrades text + one icon per upgrade with stacks); held R (repeat) and Space do nothing; the "Return to Market" button → returning, second press / request refused, camera fades to black, the summary fades out before the hub opens; the hub: cash kept, no summary, game input, no cursor, unfaded, no upgrades. |
| `Spawn.LocationFinder` | 12 points: on NavMesh, reachable, in ring, spaced, inside the boundary; same seed → same points; no ring NavMesh (player 50 m up) → 2 flagged EnemySpawn fallbacks, on NavMesh, inside, spaced, and the fallback warning is logged. (No spawns in the Market District: `Map.Hub`.) |
| `Round.WaveDirector` | Round 1 alone (RollOverRemaining 0): its size matches DT_Waves, its first enemy telegraphs at once; killing the first arrivals while more are to come keeps round 1 open (still round 1); killing as they arrive clears it (its record: kills = wave), round 2 starts at that clear and the GameState counts are round 2's; a skip removes round 2's leftovers and telegraphs and clears it, round 3 starts. |
| `Round.WaveBudget` | Seeded purchase within budget, MinRound respected, alive cap with telegraphs, spawns resume. |
| `Round.DifficultyScaling` | Round 1 base stats; round 10 = base × curve, full health; GameState multipliers. |
| `Balance.TimeToKill` | Rifle/pistol shots-to-kill at rounds 1/5/10 vs both cops via the real fire path (`[ttk]` log). XPGain 0, so kills don't open upgrade offers. |
| `Run.SeedAndStats` | Same seed replays rolls; restart resets stats; kills per type; damage dealt/taken; rounds cleared. |
| `Run.SameSeedSameRun` | Rounds 1, 2, 5, 8 (generated) replayed with one seed, in one frame: the same enemies in spawn order and the same spawn point for each (all placed at once, player still), the same level-up offer, 3 rerolls, refused 4th and next offer; every enemy placed; another seed gives another run. Since Prompt 59: rounds 1–20's waves (per-round stream) are the same after 5,000 other run-stream draws and in a new run with the same seed, and differ with another seed. |
| `Run.NewRunResets` | A run with 2 level-ups (a reroll), 6 upgrade picks, air jumps, +20 MaxHealth, a kill and $77 → death → hub (cash kept) → Start Run → town: level 1, 0 XP, no upgrades / offers, rerolls back to 3, ExtraJumps 0 / JumpMaxCount 1, base Damage / MoveSpeed / MaxHealth at full health, round 1, no kills / cash earned / combo, cash kept; HUD level 1, no icons, cash. |
| `Economy.FlatDropsPerRound` | Melee and Patrol Officers (frozen) killed in round 1 and round 10 with no combo: round 10 has more HP, but the cash (CashValue, same pieces) and the single XP orb (XPValue) are the same. |
| `Economy.CashPickups` | Exact split; out-of-reach drop stays; radius collects into Cash/CashEarned/HUD + popup; pool reuse; RoundClear vacuum; short lifetime blinks, expires unpaid. Counts cash pickups only (the kills' XP orbs never expire). Ends the kill combo before each kill, so every drop is exactly CashValue. |
| `Economy.KillCombo` | DA_ComboSettings on the GameMode (2.5 s, x1/x1.25/x1.5/x2 steps, end on damage, no XP); step lookup at each boundary; kills 1–10 in a row: count +1, multiplier, cash drop = round(CashValue × multiplier), XP orb unchanged, window restarted, HUD count / multiplier / shown from 2, "10 COMBO" / "x2 CASH", best combo; a target dummy kill doesn't count; the window drains with game time and the HUD bar follows; a kill inside the window continues; it times out one window after the last kill (HUD hidden); pickups collected after the combo ended pay the kill-time multipliers; a level-up pause freezes the window (1 s real time), picking resumes it; god mode keeps the combo, health lost ends it (DamageTaken); override bEndComboOnDamage off / bApplyComboToXP on: XP orbs multiplied, damage keeps the combo; a new run ends it (Reset) and clears the best combo; gs.Run.Stats shows it. |
| `Economy.HealthOrbs` | DA_CashSettings: chance 20%, heal 15, no lifetime, M_HealthOrb look larger than the XP orb, a sound; the Magnet card mentions health orbs; chance 0: 10 player kills drop none; chance 1: 5 kills drop 5; an orb worth 15 with no expiry and the orb look; at full health, standing on it: not attracted or collected (1.5 s); hurt to 60: collected, Health 75 (+15), run stats 1 orb / 15 healed, green popup; `gs.HealthOrb.Spawn 2` at 75: both collected, Health = MaxHealth (only 25 healed, 40 in total); a spawned orb at full health waits; the RoundClear vacuum collects it anyway, nothing healed over MaxHealth; a kill that isn't the player's drops none; a shot kill + its Powder Keg blast kill drop 2; a new run clears orbs and the stats; a dead player collects nothing. Game time ×4. |
| `Economy.XPOrbs` | XPOrbLifetime 0; kill → no XP, one orb worth XPValue with the orb mesh / material, no expiry; cash expires (gs.Cash.Lifetime 1.5) but the orb stays, no blink; walking in collects it → XP = XPValue × XPGain 1.5, run stats XPEarned, "+N XP" popup at the collection point; out-of-reach orb waits, RoundClear vacuums it; gs.XP.Add is instant; a new run clears orbs on the ground; a dead player collects nothing. |
| `Progression.XPAndLevels` | DA_XPCurve BaseXP 100 / XPIncreasePerLevel 30: L2 = 100, L3 = 130, L10 = 340, L51 = 1,570, L1000 = 30,040, every step +30 and the totals equal the per-level sums to level 300 (logs the table for L2–10, 25, 50, 100); partial XP; 260 XP → level 3 with 30 carried over (HUD level, bar, "LV 3", "30 / 160 XP"), one offer open + one queued; kill gives no XP but one orb; collected orb XP = XPValue × XPGain (×1, ×2); run stats XP/level; SetLevel 10 → 0 XP; level 10 → 60 + 7 XP in one gain (past the old cap of 50) queues one offer per level (50), the remainder carries over, a next level is always there; new run resets. |
| `Progression.EndlessLevels` | Total XP to level 100 = 155,430; `gs.XP.Add` 100 → level 2 (measures the MaxHealth per level), then the rest in one `gs.XP.Add` → level 100, 0 XP, 3,070 for 101, total XP, run stats level 100; MaxHealth = base + 99 rewards and a full heal; HUD "LV 100", "0 / 3,070 XP", empty bar, MaxHealth; 99 offers (one open, 98 queued, paused; the first has a weapon card), taken one by one: each has 3 different cards and the queue goes down by one, 99 picks, no reroll used, unpaused; 1,535 XP → "1,535 / 3,070 XP", bar half; another 1,535 → level 101 ("LV 101", "0 / 3,100 XP"), one offer; `gs.XP.SetLevel 250` → level 250, 7,570 for 251, "LV 250"; a new run → level 1, "LV 1", "0 / 100 XP", base MaxHealth. |
| `Progression.LevelUpRewards` | DA_LevelRewards wiring; +10 per stack; damaged → two level-ups → MaxHealth +20, Health full, 2 stacks, HUD health/level/XP follow; same in RoundClear; new run removes stacks and clamps Health; dead player gets the stack but no heal. Takes each upgrade offer (choice 0) so the game unpauses. |
| `Progression.LevelUpFeedback` | DescribeRewards "+10" / "+20 Max Health"; kill → no popup, collected orb → "+XPValue XP" at the collection point; double level-up → banner "Level 3" + "+20 Max Health", label, XP bar pulse, one sound, player glow; non-kill XP pops above the player; level-up during the banner merges (level 4, "+30"); 3 upgrade offers taken; banner/pulse/glow end. |
| `UI.UpgradeChoice` | Choice widget from the PlayerController; level-up pauses (GameMode owns the pause) and the cards wait for the banner; cards match the offer (order; category, rolled rarity text and frame color, pick line, "Now:" line, a description); the pick gets the card's rarity; "Reroll (3 left)", cursor shown; key / click ignored during the 0.3 s lock; R, gamepad Y and the button reroll (label 2/1/0, new cards, lock again), greyed out at 0 and R does nothing; key 2 picks the 2nd card, last pick hides, unpauses, cursor off; 1000 XP → 1 open + 3 queued; D-pad selects, A picks; next offer swaps in at once (no banner wait), locked, still paused; click / numpad 1 / key 3 pick the rest; hidden and unpaused after the queue. |
| `UI.RoundHUD` | PreRound: "ROUND 1", "PRE-ROUND", countdown "Round 1 in 3…" → "… 2…" a second later, no enemy count, ROUND 1 banner, no icons; gs.Round.Set 2 restarts text + countdown and shows ROUND 2; a level-up hides it and queues it again (LEVEL UP!, offer open, paused); 3.5 s with the cards up: still waiting; the pick replays ROUND 2; icons: picked upgrade x1 → same again x2 (one icon) → another upgrade (two icons); Combat: "COMBAT", no countdown, "N enemies left · N alive", a kill updates it; round 3 rolls in during round 2 (still COMBAT, no countdown); round 2's clear shows the pop-up, never a ROUND CLEAR banner; a level-up during the pop-up, then two skips start round 5 with the cards up → the stale ROUND 4 is dropped, pop-ups 2–4 and ROUND 5 show one at a time after LEVEL UP!; no frame with a banner or pop-up over LEVEL UP! or an open offer; a new run clears the icons. |
| `Profile.PermanentCash` | Test slot; reset deletes the save; pickup → profile + HUD + CashEarned; spend / no overspend; no save on change; a round's clear pays its bonus into the profile and saves at the clear (one more round-clear save, bonus included, version, no owned items); new run resets CashEarned but keeps the balance; death saves; balance survives death + return to the hub (the hub's HUD shows it); quitting saves it and the next PIE session (new GameInstance) loads it (HUD too); `gs.Profile.Reset` through the console → 0 and no save file. |
| `Round.ClearBonus` | DA_RoundClearSettings ($20 / $15 / $15, 8 s + 1.5 s per enemy); one round at a time (RollOverRemaining 0); PreRound opens round 1's record (no payout, no pop-up); Combat records its start and wave size; round 1 killed fast with no damage → base + no damage + fast clear, kept in its record, kills = wave size, profile += bonus + cash collected, round 2 starts with no countdown; the HUD pop-up "Round 1 clear  +$50" with its three lines (not the big banner); after the vacuum round 1's cash ≥ kills × CashValue, XP = kills × XPValue, profile and CashEarned += cash + bonus; round 2's record from zero, 5 damage → round 2 only (round 1 already cleared), an impossible time limit → base only, pop-up "missed" lines; gs.Round.Set stops the open round 3 with no bonus or cash; round 9 (skip) pays the same as round 1; gs.Run.Stats shows the last bonus; a new run clears payout and records. |
| `Upgrades.StatUpgrades` | The 3 DA_Upgrade_* assets (Common, no cap, description shows +10% / +8%); ReloadSpeed ×5 via `gs.Upgrade.Grant` → 1.5, 5 GE stacks, real reload shorter (≥ ReloadTime / 1.5); Damage ×3 → 1.3; MoveSpeed ×2 → base × 1.16 and MaxWalkSpeed follows; MaxStacks 1 refuses a second pick; unknown name refused; new run removes all upgrades and restores the stats. |
| `Upgrades.FloatingShield` | DA_Upgrade_FloatingShield (Rare, no cap, "+1", GE class, in the pool); ShieldCooldown 10 s; Shields 0, no shields or meshes; no shield: a hit takes Health; `gs.Upgrade.Grant FloatingShield 2` → 2 shields, both ready, 2 meshes shown, no collision, on the orbit at chest height, 180° apart; a real dash's i-frames block a hit with no shield used; block 1: no Health lost, 1 of 2 ready, the first shield gone, 10 s cooldown, counted, no run-stats damage taken, the combo goes on, block flash shown; the blocked mesh hidden, the other shown; block 2 (2 s later): none ready, each shield on its own cooldown; a third hit takes Health (run stats 25 damage); shield 1 back 10 s after its block (return shown, visible again), shield 2 back 10 s after its own; a pick while one recharges: the new shield ready at once, 3 meshes 120° apart; a real Beat Cop's swing and a Patrol Officer's burst are blocked (no Health lost; 4 of 4 shots in the run); a new run → Shields 0, no shields, no meshes. Health orbs off. Game time ×4. |
| `Upgrades.FireRateAndMagnet` | DA_Upgrade_FireRate (+8%) and DA_Upgrade_Magnet (+15%): Common, no cap, their GE classes, in DA_UpgradePool; FireRate 1 and PickupRadius 300 at start; rifle held for 12 shots: on the exact schedule every frame, 0.100 s apart on average (within a frame over the burst); `gs.Upgrade.Grant FireRate 10` → FireRate 1.8, refire interval / 1.8 (0.0556 s), 16 held shots on that schedule, 1.8 times as fast; a cop's drop (cash + XP orb) lands 380–660 cm away: nothing flies in at radius 300; `gs.Upgrade.Grant Magnet 10` → PickupRadius 750, the cash and the orb fly in (cash += CashValue, XP += XPValue); a new run resets both. |
| `Upgrades.PowderKeg` | DA_CombatSettings: ChancePerStack 3%, blast 30 / 3 m, chain delay 0.1 s, damage effect, placeholder effect and sound; chance 0 at 0 stacks, 3% at 1, 30% at 10, 99% at 33, 100% at 34 and clamped above; DA_Upgrade_PowderKeg: Epic, MaxStacks 34 (the first count at 100%), card text "+3% chance per stack" matches ChancePerStack, GE class, in the pool; 0 stacks: 100 kill rolls never explode and draw nothing from the run stream, a real kill doesn't explode; 34 stacks (console; a 35th refused): 100 rolls all explode with no dice; A's kill explodes at once, B (20 HP, 2 m) dies, C (4.5 m) and D (4 m) untouched, the player (1 m, god off) unhurt and Leech healed both kills, combo +2, run-stats kills +2, cash and 2 XP orbs, B's chain blast pending; it fires and hits C for 30, D untouched; a kill that isn't the player's doesn't explode even at 100%; the blast is the same at 1 and 20 stacks (30 at 2.9 m, nothing at 3.1 m); Damage x1.1 → 33; 10 stacks: 1,000 kill rolls within 26–34% (314 in the run) and the same stream state replays them; a new run → 0. Health orbs off (override 0). Game time ×4. |
| `Upgrades.PiercingRounds` | DA_Upgrade_PiercingRounds (Epic, no cap, "+1", GE class, in the pool); Pierce 0 at start; TraceShot along 4 frozen cops in a line: Pierce 0 → the nearest, ending in it; Pierce 2 → cops 1–3 in order, ending in the third; Pierce 10 → all 4, each once, ending at the last; a line with nothing on it runs to its end; a spawned wall between cops 2 and 3 stops a Pierce 10 shot after 2, ending at the wall; a dead cop is passed without using the pierce; a real shot from a zero-spread copy of the rifle with Piercing x2 and Loaded Dice x10 at 3 cops placed on its exact line (+ a 4th): cops 1–3 each take one full hit (20 or 40), cop 4 none, 3 damage numbers, critical numbers = crits, exactly 3 crit rolls drawn from the run stream; a new run → Pierce 0. |
| `Upgrades.LoadedDice` | DA_Upgrade_LoadedDice (Rare, MaxStacks 20, "+5%", GE class, in the pool); DA_CombatSettings on the GameMode, CritMultiplier 2; CritChance 0: 100 rolls, no crit, run stream untouched; 10 stacks (console) → 0.5: 2000 rolls ~50% crits (±4%), multiplier 2 on crits and 1 otherwise, the stream advanced, and the same stream state replays the same crits; 20 stacks → 1: every roll a crit, stream untouched, a 21st stack refused; the attribute clamps to 0..1; a new run → 0; a real rifle hit on a frozen cop 4 m ahead deals 20 with a plain damage number, then with 20 stacks 40 with a critical one (one more critical number; crits are bigger and another colour). |
| `Upgrades.BiggerMagsAndLeech` | DA_Upgrade_BiggerMags (Common, +15%) and DA_Upgrade_Leech (Rare, +1): no cap, their GE classes, in DA_UpgradePool; multiplier 1, HealthOnKill 0, rifle magazine 30, HUD "30 / 30"; two taps → 28, one Bigger Mags stack (console) → ×1.15, magazine 35, still 28 rounds (no refill, not full), HUD "28 / 35"; a reload fills 35; 4 stacks → ×1.6: rifle exactly 48, pistol ceil(19.2) = 20, rifle still 35 rounds, HUD "35 / 48"; the pistol equips with 20 ("20 / 20"); Leech: damaged to 70, a kill without Leech heals nothing, x1 → +1 per kill, x5 → +5, a kill that isn't the player's heals nothing, at 98 a kill stops at MaxHealth 100; a new run resets both (multiplier 1, magazine 30, HealthOnKill 0). Game time ×4. |
| `Upgrades.DashCharge` | DA_DashSettings MaxCharges 1; DA_Upgrade_DashCharge (Rare, no cap, "+1", GE_Upgrade_DashCharge, in DA_UpgradePool); ExtraDashCharges 0, 1 max charge, icon READY; a dash uses it, a second press inside the recharge is refused, icon shows the countdown with no count; a pick (console) during the recharge: ExtraDashCharges 1, max 2 and one charge ready at once, icon "1/2" a frame later, the second dash works; charges come back one recharge (3 s) after the first dash and one more recharge later; `gs.Upgrade.Grant DashCharge 2` → 3 stacks, 4 charges all ready, icon "4/4", four dashes in a row, the 5th refused, icon "0/4"; a new run → ExtraDashCharges 0, 1 max charge, no stacks. Game time ×4. |
| `Upgrades.ExtraJump` | DA_Upgrade_ExtraJump (Rare, no cap, "+1"); no air jump without it; ×3 → ExtraJumps 3, JumpMaxCount 4; three air jumps (rising, falling, falling fast) each set AirJumpZVelocity, 4th refused, one puff/sound each; landing resets; coyote jump is the ground jump; late jump after walking off uses an air jump; slide-hop then air jump; jump during an air dash is queued and becomes an air jump at the dash end; jump during a ground dash becomes the ground jump; 25 more stacks (no cap) → ExtraJumps = stacks, JumpMaxCount = 1 + stacks; new run → 0 / 1. |
| `Upgrades.SecondWind` | DA_Upgrade_SecondWind (Legendary, no cap, "+1", GE class, in the pool); DA_CombatSettings 30% / 2 s; SecondWinds 0, can't save; `gs.Upgrade.Grant SecondWind 2` → 2, Owned 2; with a combo running, a 9999 hit from a cop: not dead, still in the run, Health = 30 (whole points), invulnerable for exactly 2 s (the effect's duration), SecondWinds 1, Owned 1, used once, damage taken += the old Health, combo ended; HUD: strong flash, one sound, "SECOND WIND / 1 left" banner, Health shown as 30, upgrade icon x1; a second killing blow while invulnerable does nothing; the invulnerability doesn't end early; a hit that doesn't kill uses none; a ready Floating Shield blocks a killing blow first (no Second Wind used); the last Second Wind saves again (Health 30, none left, Owned 0, "THAT WAS YOUR LAST", icon gone, two sounds); picked again → 1; a new run → 0, no uses; with none, a killing blow kills (GameOver). Health orbs off. Game time ×4; the HUD flash shortened on this HUD (it fades in real time). |
| `Upgrades.FloatingGun` | DA_Upgrade_FloatingGun (Epic, no cap, "+1", GE class, in the pool); DA_FloatingGunSettings on the GameMode (8 damage, 1.5 / s, 20 m, 60 m/s, GE_Damage, meshes); level 0: no gun; `gs.Upgrade.Grant FloatingGun` → level 1, one gun on the right: to the right of the player and above the top of the head in the camera's frame, no collision; a frozen cop 10 m ahead: 4 s of shooting with every frame on the exact schedule (shots = 1 + floor(elapsed / interval)), at least 6 shots, every hit 8; Damage ×1.5 and Loaded Dice 20 → a critical "24"; a cop at 25 m isn't targeted; one behind a wall isn't; the nearest of two is; hand-fired projectiles pass through the player and hit a cop behind (7 × Damage 2 = 14), stop at a wall, expire in open air; the pool reuses projectiles (created = prewarm); a safe zone stops it; its kill is the player's (kills, combo 1, XP orb, health orb at chance 1, cash, Leech +1, a Powder Keg blast at 100%); paused: no shots; 24 camera views (shoulder ×2, hip / ADS, pitch −70 to +50): the gun at least 0.9 × the clear radius from the crosshair (logged per view); dead: no shots; a new run → level 0, no projectiles, no gun. Health orbs off except where noted. Game time ×4. |
| `Upgrades.FloatingGunLevels` | DA_FloatingGunSettings +10% damage and fire rate per level; guns per level 0–6: 0, 1, 2, 2, 2, 2, 2; multipliers x1 at level 2, x1.1 at 3, x1.3 at 5 (added up), 1.95 shots/s at 5; DA_Upgrade_FloatingGun has 3 card texts: "Adds a floating gun…", "Adds a second floating gun…", "+10% floating gun damage and fire rate" (the number matches the settings, damage and fire rate bonus equal), 9 owned repeats the last; the real WBP_UpgradeCard shows the text for 0–3 owned; level 1: one gun; level 2: two guns, the first still right, the second left, mirrored at the same height above the head, no collision, no bonus; one cop in range: both shoot it; a second cop: different targets, each takes one gun's damage (about 12 a second over 4 s); one dies: both go to the other; the level 5 pick mid-burst: every frame of both guns on schedule before and after, the schedule switches to the level 5 interval; level 5: still two guns (and two gun actors), 1.95 shots/s and x1.3 each, every hit 10.4, about 40.6 a second from both; logs one gun's DPS next to the rifle's; a new run → level 0, no guns, no gun actors left. Health orbs off. Game time ×4. |
| `Upgrades.Offers` | DA_UpgradePool on the GameMode (21 upgrades since Prompt 52; the 20 other than Second Wind come up evenly, weights Stat Boost / Weapon 100/20/6/1, Ability 100/10/3/0.5, Legendary ability chance 0.8%, 3 choices, 3 rerolls), 4 rarities; per-card roll: 30,000 offers all 3 different, Legendary ability cards at ~0.8% and always Legendary, the 14 others even, each category's rarity split, no Rare whole-unit card; chance 1 / 0 for the Legendary ability; Rare-only weights → Rare, but Common for whole units; per-category weights; AppearanceWeight 0 never offered; same seed same offers (with rarities), 2 upgrades → offer of 2, MaxStacks excluded, empty pool → none; a pick gets the card's rolled rarity and value; level-up opens an offer after the MaxHealth reward and heal; double level-up queues 2; 3 rerolls (events 2/1/0, Rerolled) then refused; bad index refused; picks grant a stack, Closed + Opened, last pick only Closed; new run closes the offer, refills rerolls, drops picks; reroll with no offer refused; same run seed → same first offer (with rarities); simulator deterministic, no short offers, Epic and Legendary cards, every card in a category, no Rare Legendary ability, run stream untouched. |
| `Upgrades.PickValues` | DA_UpgradePool multipliers 1 / 1.5 / 2 / 3 and whole-unit values 1 / – / 2 / 3; all 21 DA_Upgrade_* (Razor Cards since Prompt 46, Barbed Wire since 47, "+10% Barbed Wire damage, +5% size", Dynamite since 49, "+10% Dynamite damage, +5% blast size", Lucky Coin since 50, "+10% Lucky Coin damage", Molotov since 51, "+10% Molotov burn damage, +5% patch size", Tomahawk since 52, "+10% Tomahawk damage"): category, Legendary ability, whole units, BaseValue (= the old per-pick value), AppearanceWeight 1, percent or not, in the pool; Floating Gun WeaponGivingPicks 2; card number and FormatValue; ParseRarity; for every stat boost and ability one pick of each rarity: the attribute = base × (1 + total) or base + total after each, a Common pick = exactly the old pick, whole units refuse Rare, picks and total tracked, one active effect; 6 shields, JumpMaxCount 7, Powder Keg 7.5 stacks = 22.5%, crit 37.5%; `gs.Upgrade.Grant Damage 2 Epic` / `Damage Legendary`, ExtraJump Rare refused; Floating Gun Rare → Legendary → Common → Epic: power 0.5 / 2.5 / 3.5 / 5.5, 4 picks, two guns, ×1.55; Second Wind Common / Legendary / Rare = 3 charges, a killing blow uses one (2 picks, total 2, one effect); a new run removes all. |
| `Upgrades.RazorCards` | DA_RazorCardsSettings (6 damage, 2 a second, 25 m, 30 m/s, homing, +10% per power, GE_Damage, a thin card) and DA_Upgrade_RazorCards (Weapon, first pick gives it, GE class, in the pool, card text, the +10% matches the settings); a hand-thrown homing card turns toward its cop, and when the cop dies it loses its target and flies on straight; granted: level 1, power 0, 0.5 s, ×1; no cards with no enemy, with one 27 m away (out of range) or behind a wall; a cop in range: about 4 cards in 2 s, every frame on the exact schedule, every card a hit for 6; power 1: on schedule at 1/2.2 s, 6.6 per card; power 3 (an Epic pick): 2.6 a second, 7.8 per card; a cop running past (17 m in 3 s, 4 m to the side): 8 of 8 cards hit, none hit the world or expired; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: no cards; a new run: level 0, no cards in flight, none thrown at a cop in range. Game time ×4. |
| `Upgrades.BarbedWire` | DA_BarbedWireSettings (3.5 m, 5 per tick, every 0.5 s, +10% damage / +5% radius per power, GE_Damage, ring segments, a hit effect) and DA_Upgrade_BarbedWire (Weapon, first pick gives it, GE class, in the pool, card texts, "+15% Barbed Wire damage, +7.5% size" for a Rare pick, both numbers match the settings); not owned: no ring; granted: level 1, power 0, radius 350, 5 per tick; the ring is shown with one instance per segment at the radius, at the feet, no collision; four tough frozen cops at 2 m, 3.3 m (inside), 3.8 m and 1.5 m but 3 m up (outside); 4 ticks on the exact schedule every frame, two hit per tick, 5 each, the outside ones untouched; power 1: radius 367.5 (3.8 m still outside), 5.5 per tick, the ring redrawn; power 3 (an Epic pick): radius 402.5, 6.5 per tick, the 3.8 m cop now inside (three per tick), the one above still untouched; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: no ticks; a new run: level 0, no ring, no ticks, a cop next to the player untouched. Game time ×4. |
| `Upgrades.Dynamite` | DA_DynamiteSettings (40 damage, 3 m blast, every 4 s, 20 m, 0.6 s flight, +10% damage / +5% radius per power, GE_Damage, stick mesh and material, an explosion) and DA_Upgrade_Dynamite (Weapon, first pick gives it, GE class, in the pool, card texts, "+15% Dynamite damage, +7.5% blast size" for a Rare pick, both numbers match the settings); granted: level 1, power 0, radius 300, 40 per blast; one cop 21.5 m away: no throw in 9 s; moved into range: one throw at once (nothing banked); a lone cop at 5 m and a group of three at 14 m: it picks the group's nearest member (two neighbours each, none for the lone one); a tie goes to the nearest; one tough cop: 40 per blast (logs 10 DPS), a group of five: all five hit every blast (logs 50 DPS); T 2 m ahead, A 2.5 m from T, B 3.4 m from T, the player 2 m from T with god mode off: the blast centered on T, the stick at T's feet, T and A take 40 per blast, B nothing, the player nothing, one damage number per hit, the damage counted as Dynamite in run stats, every frame on the exact 4 s schedule; power 1: x1.1, 315 cm, T 44, B still outside; power 3 (an Epic pick): 52, 345 cm, B hit; Loaded Dice x20: T takes 104 and a critical number for each of the three hit; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused with a stick in the air: the stick doesn't move, no throw or blast; safe zone: no throws in 9 s, then one at once; a new run with a stick in the air: no stick left, it never explodes, level 0, no throws, a cop in range untouched; dead: no throws in 9 s. Since Prompt 52b: LeadFactor 1 on the asset; the aim on its own (800 cm/s x 0.6 s = 480 cm ahead, flat, on the ground; LeadFactor 0.5 = 240 cm; 0 = the old point; vertical speed only = no lead; running off the edge of the ground = where it stands now); a standing target at LeadFactor 1 lands exactly where LeadFactor 0 does; a cop running straight down the lane at a steady 800 cm/s is inside the blast with LeadFactor 1 and outside it with 0 (one throw each, counted as a target hit / miss). Runs on a test copy with a 1 s interval (fixed waits x0.25). Game time x4. |
| `Upgrades.LuckyCoin` | DA_LuckyCoinSettings (10 damage, every 1.5 s, 20 m, 8 m bounce range, 3 bounces, +1 bounce per 2 full power: 3 3 3 4 4 5 at power 0 / 1 / 1.9 / 2 / 3.5 / 4, +10% damage per power, GE_Damage, coin mesh and material) and DA_Upgrade_LuckyCoin (Weapon, first pick gives it, GE class, in the pool, card texts, "+10% Lucky Coin damage", the number matches the settings); granted: level 1, 3 bounces, 10 per hit; no enemy: no flips in 2 s, then one at once when a cop appears (nothing banked); one cop: one hit a flip (logs 6.7 DPS); five tough cops in a row 4 m apart: every frame on the exact 1.5 s schedule, a flip hits the first four in order, each once (10 a flip each), the fifth never (out of bounces), the damage counted as Lucky Coin in run stats (logs 26.7 DPS); power 1: 11 per hit, still four hit; power 2: 4 bounces, all five hit, 12 each; power 3.5 (a Rare pick): 13.5 per hit, still 4 bounces; two cops 12 m apart: the flip stops after the first, the far one never hit; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: no flips; a new run with a coin in the air: no projectile left, level 0, no flips, a cop in range untouched. Since Prompt 52b it runs on a test copy that flips every 1.25 s. Game time x4. |
| `Upgrades.Molotov` | DA_MolotovSettings (every 5 s, 15 m, 0.6 s flight, 4 per burn every 0.5 s for 4 s, 2.5 m patch, +10% damage / +5% radius per power, GE_Damage, bottle and patch meshes and materials) and DA_Upgrade_Molotov (Weapon, first pick gives it, GE class, in the pool, card texts, "+15% Molotov burn damage, +7.5% patch size" for a Rare pick, both numbers match the settings); a cop 16 m away: no target, no throw; moved to 8 m: a throw at once (nothing banked), the patch at its feet, no collision, drawn at its radius; T 1.2 m in front of the player (in the fire, god mode off), A 1.5 m and B 2.4 m from T, C 2.7 m, D 3 m above: two patches, every frame on the exact 5 s schedule, 8 burns per patch, T / A / B take 32 a patch, C and D nothing, the player nothing, the damage counted as Molotov in run stats; power 1: 4.4 per burn, 262.5 cm, C still outside; power 3 (an Epic pick): 5.2, 287.5 cm, C burns, D still doesn't; paused: the patch doesn't age, no burns, no damage; safe zone: no throws, no burns; one cop and five cops in a patch at power 3 (logs DPS); its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); a new run with a patch burning: no patch or bottle left, level 0, no throws or burns, a cop in range untouched. Since Prompt 52b: LeadFactor 1 on the asset; a standing target is caught by the first burn and the landing isn't led; a cop running straight down the lane at a steady 800 cm/s is in the patch at its first burn with LeadFactor 1 and outside it with 0. Runs on a test copy: a throw every 2.75 s, a burn every 0.25 s for 2 s (still 8 burns a patch). Game time x4. |
| `Upgrades.Tomahawk` | DA_TomahawkSettings (12 damage, every 3 s, 18 m range, 18 m out, 1 tomahawk, +1 per 3 full power: 1 1 2 2 3 at power 0 / 2.9 / 3 / 5.9 / 6, +10% damage per power, GE_Damage, mesh and material) and DA_Upgrade_Tomahawk (Weapon, first pick gives it, GE class, in the pool, card texts, "+10% Tomahawk damage", the number matches the settings); granted: level 1, one tomahawk, 12 per hit; no enemy: no throws, then one at once when a cop appears (nothing banked); one cop: 24 a throw, two hits (logs 8 DPS); a line of five 3 m apart and one 2.5 m beside it: every frame on the exact 3 s schedule, thrown at the nearest, each in the line hit once out and once back (24 a throw), the one beside untouched, ten hits a throw, the damage counted as Tomahawk in run stats (logs 40 DPS); a wall across the lane 1.5 m past the second: the two in front hit out and back, the three behind untouched; power 1: 13.2 per hit, still one tomahawk, the far cop hit again once the wall is gone; power 3 (an Epic pick): 15.6 per hit, two tomahawks at the nearest and the next nearest, 20 hits a throw; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: the tomahawk in the air doesn't move, no throws; a new run with a tomahawk in the air: none left, level 0, no throws, a cop in range untouched. Since Prompt 52b it runs on a test copy that throws every 2.25 s. Game time x4. |
| `Upgrades.WeaponSlots` | MaxWeapons 3; the real first offer has a weapon card (any weapon); the limit step uses a pool copy without Razor Cards, Barbed Wire, Dynamite, Lucky Coin, Molotov and Tomahawk; test pool with two test weapons, MaxWeapons 1: none owned → both offered, one owned → the other never offered (CanOfferWeapon false) and the owned one still offered, MaxWeapons 2 → offered again; the first offer's weapon card in 400 of 400 offers, 3 different cards, at all three positions, Common and Rare both rolled; slots full → it is the owned weapon; MaxWeapons 0 → no weapon card, 3 cards; card labels NEW WEAPON / WEAPON UPGRADE / STAT BOOST; real run: first offer has the Floating Gun, 3 rerolls keep a weapon card, after the first pick not pending and in 200 later offers the share without a weapon card is within 10 points of the share expected from the weapons that can be offered at each offer (since Prompt 49, per offer since 52; it was "more than 5 of 20"), a new run → pending; a copy of the pool with a test weapon and MaxWeapons 1 (`SetUpgradePoolOverride`): one HUD slot, 30 offers + rerolls never offer the test weapon but offer the Floating Gun, the first one includes it; MaxWeapons 3: the test weapon is offered and picked, slots Floating Gun / test weapon / empty, no weapon in the upgrade row; real pool HUD: 3 empty slots without an icon, Damage in the upgrade row, two Epic Floating Gun picks → slot 1 shows it (icon, no count), the others empty, no Floating Gun in the upgrade row; a new run empties the slots; death → summary weapons line "Floating Gun", upgrades "Damage x1", one upgrade icon. |
| `Upgrades.MoveSpeedSlideDash` | Base Move Speed: ratio exactly 1, the scaled settings getters equal the plain values at 1; half Move Speed → ratio 1 (never below), ×2 → 2; a Move Speed pick raises it, a new run resets it; on a clear 34 m lane of the ring road, a slide from a full run and a dash at ×1 and ×2: each keeps its start ratio; slide start = min(run + SpeedBoost × r, MaxSpeed × r) and faster than running, end = EndSpeed × r; dash = Distance × r in its Duration (±0.02 / +0.06 s), exit = min(ExitSpeed, dash speed) × r = the running speed; ×1 = the old numbers exactly; ×2 = twice the distance in the same time (within 0.05 s), the slide ends at twice EndSpeed; ×33: a dash and a slide into the east boundary wall stop inside. |
| `Upgrades.MoveSpeedCover` | A crate, a low wall, a shack, a tower and the silo, each with a clear 6 m run-up; at Move Speed ×1, ×5 and ×33, a dash and a slide from a full run into each, the player's location sampled every frame in the piece's own frame: no frame overlaps the piece, the player stays on the ground, and if it ends behind the piece it went round it. Logs each path. Game time ×3. |
| `Checkpoint.WeaponPicksAndNewRun` | Weapon power after picks of each rarity for every weapon (Dynamite since Prompt 49, Lucky Coin since 50, Molotov since 51, Tomahawk since 52; e.g. Barbed Wire at power 6.5: ×1.65 damage, radius 463.75); a new run starts with 1 dash charge; every upgrade picked at every rarity it has (63 picks across 17 upgrades), a reroll used, Damage / air jumps / MaxHealth raised, two guns, the ring, Razor Cards and shields present; a new run: no picks, no offer, rerolls refilled, no weapons and the first offer's weapon card pending again, level 1, 1 dash charge, no shields / guns / ring / cards / Dynamite / Lucky Coin / Molotov / Tomahawk, nothing thrown left in play, no combo, no Powder Keg blasts, no damage dealt, HUD weapon slots empty and no upgrade icons. |
| `Checkpoint.WeaponLimitAllThree` | Real pool (since Prompt 49): three real weapons owned, 60 offers and their rerolls never offer an unowned real weapon, the owned ones still offered. Then all three M2c weapons owned, plus a fourth test weapon in a pool copy: HUD slots in pick order (Barbed Wire, Razor Cards, Floating Gun), no weapon in the upgrade row; slots full: `CanOfferWeapon` refuses the fourth; 60 offers and their rerolls never offer it, and owned weapons are still offered (31 times in the run); still three weapons; with MaxWeapons 4 the fourth is offered; the auto-pick policy (owned weapon with full slots, new weapon with a free slot, reroll when all Common, first without rerolls, else the rarest). |
| `Checkpoint.SevenWeapons` | Real DA_UpgradePool (Prompt 53): 21 upgrades, each once and in its category (7 Stat Boosts, 7 Abilities with Second Wind the only Legendary ability, 7 Weapons), every DA_Upgrade_* asset in the pool, MaxWeapons 3, three HUD slots; the roll for each of the 35 sets of three owned weapons (200 offers each, half of them first offers with the weapon card): the other four never rolled, each owned one still rolled; 8 real runs of 40 offers, taking weapon cards and keeping the rerolls for when the slots are full: never more than three weapons, every run fills its slots, no unowned weapon in the 293 offers and 24 rerolls after that; 48 new runs: every first offer and its 3 rerolls has a weapon card, and all seven weapons come up as first-offer weapons. |
| `Upgrades.PowderKegChainEnds` | 34 stacks (102%), 8 frozen 10 HP cops in a line 2.5 m apart. One kill starts a chain that runs down the line and ends: all 8 die, exactly 8 blasts, nothing pending. The player (no god mode, 6 m from the first) is never hurt. Run stats record the Powder Keg damage. Game time ×4. |
| `Art.PlayerMesh` | Prompt 56, in the town and then the hub: the player's mesh is `SK_LowPolyBody` on the same skeleton as before (`SK_Mannequin`), with `PA_Mannequin` and `ABP_Unarmed` running; six colour slots; bones `hand_r`, `head`, `spine_04`, `foot_l`; in the animated pose the head is 140–175 cm above the foot and `root` has scale 1 in the reference pose (the FBX unit-scale trap); two worn props (`SM_SkiMask` on `head`, `SM_LootBag` on `spine_04`), attached to the body, no collision, visible, the mask at the head and the bag more than 10 cm behind the chest. |
| `Enemies.CopLooks` | Prompt 57: a still Beat Cop and Patrol Officer 6 m in front of the player. Each has its display name ("Beat Cop", "Patrol Officer"), wears `SK_LowPolyBody` with `ABP_Unarmed`, and has a dynamic material on each of the 6 slots made from that slot's `SlotMaterials` entry, with `TelegraphAmount` 0 at rest. Props: the cap on `head` (above the head) for both, the baton on `hand_r` for the Beat Cop; each attached to the body, no collision, visible, near its bone. Gun: none for the Beat Cop, `SM_PatrolCarbine` in the weapon's socket for the Patrol Officer, with the muzzle near it. Neither wears the robber's top or trousers, and the two tops differ. The Beat Cop's melee wind-up and the Patrol Officer's aim set `TelegraphAmount` 1 on every slot, and the glow goes back to 0 when they end. Since Prompt 59b: `DA_Enemy_PatrolOfficer` has no AimLaserMaterial; no laser shows at rest or in any frame of its aim and its burst (which fires its 4 bolts), and its body glows during the aim. |
| `Enemies.DodgeableShots` | Prompt 58, 45 m up over the town centre (no cover in the way unless placed), the player held still (flying) and Patrol Officers with their brains stopped: `DA_PatrolCarbine` 2,800 cm/s with a bolt mesh, material and impact effect; damage 7, burst 4 and the 0.4 s aim unchanged; the Patrol Officer `bRanged`, the Beat Cop not. With a spreadless copy of the Patrol Officer at 15, 20 and 25 m: no damage on the frame the shot is fired, the shot is one visible bolt at 2,800 cm/s, the first hit lands (distance − 80 cm) / 2,800 s later (±0.08 s; logs each flight time and how far a walking player moves meanwhile), and all 4 shots hit for 28. With the real carbine (spread) at 20 m and the player moved sideways at 900 cm/s every frame: 4 shots fired, none hit, no Health lost. A wall at 10 m stops all 4 (hit world), none reach the player. A Beat Cop at 10 m on the line: all 4 fly through it and hit the player, the cop untouched. `State.Invulnerable` (the dash's i-frames) with 2 shields: no Health lost and no shield used. 2 shields: two shots blocked, one shield each, the other two take 14. Second Wind at 5 Health: the player survives at 30% with it used up (the later shots hit the invulnerability). Paused with a shot in flight: it doesn't move or hit for 0.3 s real time, then lands after the unpause. A new run with a shot in flight: no enemy shot left. Game time ×2 (×1 before Prompt 60). |
| `Round.RangedCap` | Prompt 58: `DA_WaveSettings` MaxRangedShare 0.4, generated waves from round 6; rounds 6–30 for 10 seeds (250 purchases): none above 40% ranged, and none leaves budget a non-ranged enemy could use (or hit the size cap); ranged enemies still bought in over 200 of them; logs the highest share, rounds 6–15 before (no cap) and after for one seed, and each DT_Waves row's share (all at most 40%, not capped); round 7's real wave from the director (queued + telegraphing = its total) is at most 40% ranged. |
| `Round.RollOver` | Prompt 59: DA_RoundSettings RollOverRemaining 3, window 20 s, gap 0.25–2 s (4 enemies 2 s, 13 enemies 20/13 s, 200 enemies 0.25 s). Round 5 killed as it arrives: round 6 starts exactly when round 5 is down to 3 (not before, not later), still Combat, round 5 still open; round 5's spawns one gap apart (±0.15 s). A hit while 5 and 6 are open counts to both; both pay their bonus once, at their own clear, without the no-damage part, with clear time from their own start and fast clear iff within their own limit; each round's kills = its wave and its cash = the sum of its enemies' CashValue (combo off); a save at each clear holding the balance. RollOverRemaining 0: rounds 3 and 4 never open together, round 4 starts at round 3's full clear. The alive cap (6) holds for two open rounds together and is reached. Death: GameOver, the open round closed without a clear, killing the leftovers pays nothing. Game time ×4; since Prompt 60 the rounds run on a settings copy with a 5 s spawn window (max gap 0.5 s) after the asset's values are checked. |
| `Round.Breather` | Prompt 59: a breather after every 10 rounds, 10 s; rounds 10 and 20 are breather rounds, 9 and 11 aren't. Round 9 rolls into 10, 10 never into 11; once rounds 9 and 10 are cleared → Breather, still round 10, BreatherDuration left, HUD "BREATHER" and "Round 11 in …", no enemy count. During it: nothing spawns or telegraphs, no stats row, clear or bonus, no cash once the vacuum has landed; it lasts BreatherDuration (game time); then round 11 in Combat with exactly one new record and rounds cleared unchanged. The safety guard (2 s on a settings copy): round 20 ready after a kill → guard running, no roll-over; it clears the leftovers → Breather. Game time ×4; since Prompt 60 a settings copy (5 s spawn window, 2.5 s breather) after the asset's 10 s is checked. |
| `Round.RollOverPace` | Prompt 59, a report: `gs.KillAll` once a game second from round 1's Combat until round 10 starts (logs the time); the empty time between rounds 1–6 (a round's clear to the next round's first enemy on the field), each less than the old 7.75 s; rounds 1–20: arrival time of the last enemy next to the fast-clear limit, flagging any shorter limit (logged). Game time ×10 (×4 before Prompt 60). |
| `Economy.HealthOrbsAllSources` | For each kind of player kill (own kill, Floating Gun, Razor Cards, Barbed Wire, a Powder Keg blast kill) in a fresh run: at drop chance 1 every kill drops an orb, at 0 none, and the damage is recorded under that source in run stats. Chance 0.3 in bulk: 60 own kills and a crowd killed by one Barbed Wire tick (5 Legendary picks) drop at about the same rate (23 of 60 and 16 of 50 in the run). Game time ×4. |
| `Round.AliveCap100` | Prompt 60: `DA_WaveSettings` MaxAliveEnemies 100 and the cap in effect is 100. From round 21 on a settings copy (any kill rolls over; enemies arrive fast), one kill in each new round opens the next until three rounds are open, then nothing dies: alive + telegraphing never above 100 in any frame, three rounds open at once, exactly 100 alive (GameState too) for 2 s of game time, and the open rounds still have enemies waiting (logged: 250 in three rounds, 148 waiting). Game time ×4. |
| `Round.BiggerWaves` | Prompt 60: `DA_WaveSettings` BudgetBase 5, BudgetPerRound 5, no curve, MaxAliveEnemies 100; DT_Waves rounds 1–5 still 4 / 6 / 8 / 10 / 13. Rounds 6–30 for 5 seeds: budget 5 + 5 × round, spent (nothing a Beat Cop could still buy left, or the size cap), at most 40% ranged, and bigger than the old budget's wave from the same round stream. Logs rounds 6–30 before / after: enemies, budget, ranged share and the most one round brings alive (capped). |
| `Enemies.CorpseLimit` | Prompt 60: `DA_WaveSettings` MaxCorpses 20. 30 still cops; while alive: no body-mesh collision, its bones skip the physics update, no pose off screen, animation rate optimization on, no capsule overlap events, the capsule blocks Visibility. Killed one a frame: the first corpse simulates with collision again and its pelvis body starts within 1 m of where the cop stood; never more than 20 corpses, 20 at the end, the limit removed 10, and those are exactly the 10 oldest; the rest go after CorpseLifetime. Game time ×4. |

All names start with `BanditBaller.`. Run them with `RunTestsByFilter("StartsWith:BanditBaller")`.

## Tuning values (current)

- **Player**: walk 900 cm/s, accel 6000, jump 800, air jump 800 (~1.87 m each), gravity ×1.75, coyote 0.12 s; camera arm 350 / FOV 90, ADS 180 /
  65. Slide: +600 boost, max 1600, end 600, 0.8 s, 1 s cooldown. Dash: 600 cm in 0.15 s, exit 900 cm/s, 0.25 s
  i-frames, 1 charge (+1 per Dash Charge pick), 3 s recharge each, one after another. Since Prompt 44b the slide's boost,
  max and end speed and the dash's distance and exit speed are × the speed ratio (MoveSpeed / 900, at least 1, taken
  when the slide or dash starts): at Move Speed ×2 the slide caps at 3,200 and ends at 1,200 cm/s, the dash covers
  12 m in the same 0.15 s and exits at 1,800 cm/s.
- **Weapons** (damage / RPM / mag / reload):
  - Rifle: 20 / 600 auto / 30 / 1.8 s, spread hip 2° and ADS 0.5°. Held fire is exactly 600 rounds a minute
    (0.100 s apart, Prompt 31b), at any frame rate.
  - Pistol: 45 / 300 semi / 12 / 1.2 s.
  - PatrolCarbine (enemy): 7 / 360 / 4-shot bursts, 3.5° spread, 60 m.
- **Enemies** (`Enemies/DA_Enemy_*`):

  | Enemy | HP | Speed | Combat | SpawnCost | MinRound | Cash |
  |---|---|---|---|---|---|---|
  | BeatCop | 60 | 520 (±15%) | 12 dmg melee, 0.55 s wind-up, range 170 | 1 | 1 | $10 |
  | PatrolOfficer | 45 | 430 (±12%) | range 15–25 m, backs off < 8 m, 0.4 s aim (body glow only, no laser since Prompt 59b), 4-shot bursts | 2 | 2 | $15 |

  Corpses stay 5 s.
- **Rounds** (DA_RoundSettings, since Prompt 59): a 3 s PreRound before round 1 only; the next round starts when the
  newest has `RollOverRemaining` 3 or fewer enemies left (after one of its own kills); each round's enemies arrive one
  at a time over `RoundSpawnWindow` 20 s (gap kept within 0.25–2 s: 4 enemies 2 s apart, 13 enemies 1.54 s, 40+ 0.5 s);
  after every `BreatherEveryRounds` 10th round a full clear, then a `BreatherDuration` 10 s breather; the 180 s
  `CombatFallbackDuration` guards only the wait before a breather; the save at a clear waits up to `ClearSaveMaxWait`
  3 s for its vacuum. GameOver (death) is untimed.
- **Run end:** run summary input lock 1 s, fade-in 0.4 s and fade-out 0.5 s (WBP_RunSummary class defaults); camera
  fade before the hub opens 0.5 s (`ReturnToHubFadeTime`, BP_GSGameMode).
- **Spawn ring** (DA_SpawnSettings): 12–25 m, 2 m spacing, 20 attempts, prefer off-screen. The user chose to keep
  12 m.
- **Town map** (L_Town_Graybox, Prompt 26b): 300 × 300 m, boundary 30000 × 30000 cm with a 75 m ceiling, NavMesh
  bounds Z −5 to 35 m, EnemySpawn fallback points 142 m out on the axes, landmarks 125 m out.
- **Wave table** (DT_Waves; the per-row interval and batch were removed in Prompt 59, spacing is the round settings'):

  | Round | Enemies | Spawn gap |
  |---|---|---|
  | 1 | 4 melee | 2.0 s |
  | 2 | 5 melee + 1 ranged | 2.0 s |
  | 3 | 6 + 2 | 2.0 s |
  | 4 | 7 + 3 | 2.0 s |
  | 5 | 9 + 4 | 1.54 s |
- **Generated waves** (DA_WaveSettings): Budget = 5 + 5 × round since Prompt 60 (was 5 + 2.5 × round; BudgetCurve empty),
  e.g. round 8 = 45, round 15 = 80, round 27 = 140 (about 100 cops). MaxRangedShare 0.4 (Prompt 58: at most 40% of a generated wave ranged, by count), MaxGeneratedEnemies 150,
  one at a time like every round (the batch fields were removed in Prompt 59), MaxAliveEnemies 100 since Prompt 60 (was 30; every open round together),
  MaxCorpses 20 (Prompt 60), telegraph 0.75 s (M_EnemyLaser). Pool: BeatCop, PatrolOfficer.
- **Difficulty curve** (DA_DifficultyCurve, linear, extrapolates past 30):

  | Round | 1 | 5 | 10 | 20 | 30 |
  |---|---|---|---|---|---|
  | Health ×| 1.0 | 1.4 | 2.0 | 3.5 | 5.0 |
  | Damage × | 1.0 | 1.15 | 1.35 | 1.8 | 2.3 |

  Measured time-to-kill: round 10 melee (120 HP) takes 6 rifle shots (0.50 s) or 3 pistol shots (0.40 s).
- **Cash** (DA_CashSettings):
  - Split: TargetPieceValue 4, 2–6 pieces, bills at $4+. A Beat Cop drops $4 + $3 + $3; a Patrol Officer drops
    $4 + $4 + $4 + $3.
  - Scatter 40–140 cm, toss 0.35 s / 80 cm high, rest 25 cm above the floor.
  - Lifetime 20 s, blink for the last 4 s.
  - Magnet 400 → 3000 cm/s (accel 3000), collected within 80 cm. Pool prewarm 48 (shared with XP orbs).
  - XP orbs: one per kill worth XPValue, same toss / magnet / vacuum, no lifetime (XPOrbLifetime 0), sphere ×0.32
    with M_XPOrb, pulse ±15% at 1.5 Hz.
  - Health orbs: 20% chance per player kill (run stream), heal 15 (never over MaxHealth), same toss / magnet /
    vacuum, ignored at full health (except by the vacuum), no lifetime, sphere ×0.42 with M_HealthOrb, pulse ±12%
    at 1 Hz. On average one in five kills drops one: a round 5 wave (13 enemies) drops about 2.6 orbs, ~39 health.
- **XP** (DA_XPCurve, since Prompt 45; no level cap): `BaseXP` 100 for level 2, then `XPIncreasePerLevel` +30 more
  for every level after that (XP from the previous level = 100 + 30 × (level − 2)):

  | Level | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 25 | 50 | 100 |
  |---|---|---|---|---|---|---|---|---|---|---|---|---|
  | XP from the previous level | 100 | 130 | 160 | 190 | 220 | 250 | 280 | 310 | 340 | 790 | 1,540 | 3,070 |
  | Total from level 1 | 100 | 230 | 390 | 580 | 800 | 1,050 | 1,330 | 1,640 | 1,980 | 10,680 | 40,180 | 155,430 |

  - The old curve (+25% per level, cap 50) needed 596 for level 10, ~16,900 for level 25 and 4.48 M for level 50.
  - XP per enemy (orb value): BeatCop 10, PatrolOfficer 15. DT_Waves rounds 1–5 drop 40 / 65 / 90 / 115 / 150 XP;
    generated waves about 8.3 XP per budget point (round 10: 19 enemies, 245 XP; round 20: 36–37 enemies, 455–460 XP).
  - **Levels per cleared round** (every enemy killed and every orb collected, XPGain 1, clearing every round from 1):
    round 1 0.4 (40 of the 100 for level 2), round 5 ~0.9 (150 XP at level 3–4, which needs 160–190), round 10 ~0.8
    (245 XP at level 7–8, 280–310), round 20 ~0.85 (460 XP at level 16, 550). Because both the XP per round and the XP
    per level grow linearly, a cleared round stays worth a little under one level all the way up.
- **Kill combo** (DA_ComboSettings): 2.5 s window, restarted by every kill. Cash x1 from 1 kill, x1.25 from 3,
  x1.5 from 6 and x2 from 10. Damage ends it; XP isn't multiplied.
  - Beat Cop drops: $10 / $13 / $15 / $20.
  - Patrol Officer drops: $15 / $19 / $23 / $30.
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
  pierced, Powder Keg +3% chance that a kill explodes, Floating Gun +1 level (1: a gun, 2: a second gun, 3+: +10%
  gun damage and fire rate) (Epic); Second Wind +1 (Legendary, used up when it saves the
  player). Nothing is capped since Prompt 44: Loaded Dice past 100% (20 Common picks) gives triple hits, Powder Keg past
  100% (34 Common picks = 102%) double-size blasts.
- **Second Wind** (DA_CombatSettings): back at `SecondWindHealthFraction` 30% of MaxHealth (rounded to whole points),
  `SecondWindInvulnerableTime` 2 s. HUD (WBP_HUD, HUD|Second Wind): full-screen flash (1, 0.85, 0.4) from 0.85 alpha
  over 0.7 s, `SecondWindSound` = `/Engine/VREditor/Sounds/UI/Teleport_Committed` (editor content), banner color
  (1, 0.7, 0.15) for the round banner's 1.8 s.
- **Floating Gun** (DA_FloatingGunSettings): 8 damage per hit × the player's Damage (crits ×2 with Loaded Dice),
  1.5 shots a second (0.667 s apart, exact schedule), range 20 m with a clear line, projectile 60 m/s with a 0.75 s
  lifetime (45 m) and an 8 cm sweep, pool prewarm 16. Rest point (−15, 45, 115) cm from the capsule center in the
  camera's yaw frame (19 cm above the head top); follow 14, turn 12; off the crosshair: radius 10% of the screen
  height, slide 600 cm/s, at most 150 cm. Look: engine cube (0.4, 0.1, 0.12) with M_FloatingGun, engine sphere ×0.12
  with M_FloatingGunProjectile, `VR_click1_Cue` (editor content), RadialBurst ×0.25 on impact. About 12 damage a
  second at Damage 1, so a round-1 Beat Cop (60 HP) takes 8 hits, about 5 s.
  - Levels: 1 gun at level 1, 2 from level 2 (mirrored over the left shoulder; never more). From level 3,
    `DamageBonusPerLevel` and `FireRateBonusPerLevel` +10% each per level, added up: level 5 = 10.4 per hit at 1.95
    shots/s, 20.3 DPS per gun, 40.6 for both. The rifle does 200 while firing, 125 with reloads.
- **Critical hits** (DA_CombatSettings): CritMultiplier 2.0, TripleCritMultiplier 3.0 (crit chance past 100%: the rest is
  the triple-hit chance). Damage numbers: normal 20 white, crit 28 gold "N!", triple 36 orange-red "N!!".
- **Powder Keg** (DA_CombatSettings): `ChancePerStack` 3% that a player kill explodes (10 stacks 30%, 34 stacks 102%; past 100% the rest is the chance of a double-size blast: 6 m, the same damage),
  rolled per kill on the run stream. The blast is always 30 damage × the player's Damage in 3 m, at any stack count;
  chain kills roll again and explode 0.1 s later; no line of sight; never hurts the player; no crits.
- **Floating Shield** (DA_CombatSettings): `ShieldCooldown` 10 s per shield, each on its own world timer (pauses with
  the game). Visuals on BP_GSCharacter's `FloatingShields` component: engine cylinder at (0.45, 0.45, 0.06) with
  M_FloatingShield, orbit 85 cm, 40 cm above the capsule center (chest height), 120°/s; block flash ×1.8 over 0.15 s,
  return pop 0.35 s.
- **Upgrade offers** (DA_UpgradePool, 21 upgrades since Prompt 52, seven of them weapons; placeholders): per card a 0.8% Legendary ability
  chance, otherwise an upgrade by AppearanceWeight (all 1) and a rarity from its category's weights: Stat Boosts and
  Weapons 100 / 20 / 6 / 1, Abilities 100 / 10 / 3 / 0.5 (whole units: Rare → Common). 3 choices, 3 rerolls per run.
  Pick values ×1 / ×1.5 / ×2 / ×3, whole units +1 / +2 / +3.
  - `simulate_upgrade_offers(10000, 1)` (Prompt 53, 21 upgrades): each weapon on 4.7–5.2% of cards (14.2–15.5% of offers); cards by category: Stat Boost 35.0%, Weapon 34.6% (in 74.2% of offers), Ability 29.7%, Legendary ability 0.77%. With three weapons owned (slots full): the three owned weapons on 5.9–6.2% of cards each, the other four on none; Weapon 18.2% of cards (48.1% of offers), Stat Boost 43.4%, Ability 37.6%. Earlier, Prompt 46 (16 upgrades): each of the 15 non-Legendary upgrades on 6.3–6.9% of cards
    (Razor Cards 6.6%), Second Wind on 0.77%. Cards by category: Stat Boost 46.3%, Ability 39.8%, Weapon 13.2% (in
    36.7% of offers; was 7.0% / 21.1% with one weapon), Legendary ability 0.77%.
  - By rarity: Common 84.8% of cards, Rare 10.0%, Epic 3.8%, Legendary 1.4%. Offers with a Rare card 27.0%, an Epic
    11.0%, a Legendary 4.2%. No short offers.
- **Razor Cards** (DA_RazorCardsSettings): 6 damage per card × the player's Damage (crits with Loaded Dice), 2 cards a
  second on an exact schedule, range 25 m from the throw point with a clear line, cards fly at 30 m/s and turn 720°/s
  toward their target (turning circle ~2.4 m), lifetime 1.25 s (37.5 m), sweep radius 10 cm. Thrown from 30 cm in front
  of and 40 cm above the capsule center. Each point of power: +10% damage and +10% fire rate. Look: the engine cube at
  (0.25, 0.18, 0.01) with MI_RazorCard (white), spinning 1,080°/s, `VR_click3_Cue` (editor content), RadialBurst ×0.2
  on impact.

  | | Damage per card | Cards / shots a second | DPS (Damage 1, no crits) |
  |---|---|---|---|
  | Razor Cards, power 0 (one Common pick) | 6 | 2 | **12** |
  | Razor Cards, power 1 / 3 (two Common picks / + an Epic) | 6.6 / 7.8 | 2.2 / 2.6 | 14.5 / **20.3** |
  | Floating Gun, one gun (one pick) | 8 | 1.5 | **12** |
  | Floating Gun, both guns (two picks, power 0) / power 3 (five Common picks) | 8 / 10.4 | 1.5 / 1.95 each | 24 / 40.6 |
  | Rifle (for scale) | 20 | 10 | 200 held, 125 with reloads |

  One pick of either weapon is 12 DPS. The Floating Gun's second pick doubles it (the second gun), while Razor Cards'
  second pick adds 10%; from there both grow 10% per power point, and the gun has twice the base. Razor Cards' edge is
  that its cards home in: they hit a running cop every time (8 of 8 in the test), where the gun's bullets lead the
  target once and can miss a turning one.
- **Barbed Wire** (DA_BarbedWireSettings): a ring of radius 3.5 m on the ground around the player (enemy centers within
  the radius flat and within 2 m up or down); every 0.5 s (exact schedule) 5 damage × the player's Damage to every enemy
  inside, each with its own crit roll. Each point of power: +10% damage, +5% radius (power 3: 6.5 per tick, 4.025 m;
  power 10: 10 per tick, 5.25 m). That is 10 DPS per enemy inside at power 0 (no crits), with no cap on how many: 4
  cops inside = 40 DPS. Look: 48 segments of the engine cube, 4 cm wire, 3 cm above the floor, MI_BarbedWire (grey);
  RadialBurst ×0.06 on each enemy hit.
- **Dynamite** (DA_DynamiteSettings): every 4 s (exact schedule) a stick lobbed at the enemy within 20 m with the most
  other enemies within the blast radius of it (nearest on a tie; no clear line needed); it lands 0.6 s later (arc 3 m
  high) on the ground where that enemy will be (`LeadFactor` 1: its flat velocity × the flight time; a Beat Cop at
  520 cm/s is led about 3.1 m) and blasts every enemy within 3 m of the body over that point for 40 × the player's
  Damage, each with its own crit roll. Each point of power: +10% damage, +5% blast radius (power 3: 52 per enemy, 3.45 m). Look: the engine
  cylinder (0.08, 0.08, 0.3) with MI_Dynamite (red), tumbling 720°/s, SimpleExplosion scaled to the radius.
- **Molotov** (DA_MolotovSettings): every 5 s (exact schedule) a bottle lobbed at the nearest enemy within 15 m; it lands
  0.6 s later on the ground where that enemy will be (`LeadFactor` 1, as Dynamite) and leaves a 2.5 m patch (flat, 2 m up
  or down) that burns every enemy in it for 4 × the player's Damage every 0.5 s for 4 s (8 burns). Each point of power:
  +10% burn damage, +5% patch radius.
- **Lead in the PIE check (Prompt 52b),** the first 10 throws at running Beat Cops: Dynamite caught its target 7 of 10
  times with LeadFactor 0 and 9 of 10 with 1; the Molotov's first burn caught it 0 of 10 times with 0 and 10 of 10 with 1.
- **Weapons at power 0, Damage 1, no crits** (power 5 and the Floating Gun per gun: the table under Prompt 53):

  | Weapon | One enemy | Five enemies |
  |---|---|---|
  | Floating Gun | 12 (24 with the second pick's gun) | 12 / 24 (one target at a time per gun) |
  | Razor Cards | 12 (never misses) | 12 |
  | Barbed Wire | 10 (inside 3.5 m) | 50 (all five inside) |
  | Dynamite | 10 (40 every 4 s) | 50 (all five within 3 m) |
  | Lucky Coin | 6.7 (10 every 1.5 s) | 26.7 (four of the five hit a flip, within 8 m of each other) |
  | Molotov | 6.4 (32 per patch every 5 s; 8 while it burns) | 32 (all five in the 2.5 m patch) |
  | Tomahawk | 8 (12 out + 12 back every 3 s) | 40 (a line of five; one tomahawk passes all five both ways) |
- **Combo curves**: there is no combo system yet.

## Known issues, TODOs and deviations

**Not built yet / stubs**
- The M2c upgrade rework is complete (Prompts 41–48), and so are the last four weapons (49–52) and the weapons checkpoint
  (53). M2d (Prompts 55–69) is next, then M3.
- M2d (designed in Prompt 54) is only started: the player is the robber (Prompt 56) and the enemies are cops in
  Blender models (Prompt 57) whose shots are dodgeable bolts with a 40% ranged cap (Prompt 58), and rounds roll over
  (Prompt 59), and up to 100 are alive at once (Prompt 60), but the code still has engine-shape placeholders for weapons,
  pickups and cover, only the two enemy types, and the 300 × 300 m town.
- One hundred enemies (Prompt 60):
  - The biggest single cost left with a crowd is the Barbed Wire hit effect: the RadialBurst placeholder has a ribbon
    trail built on the CPU, about 1.9 ms for the ~270 bursts alive with 40 cops in the ring. A cheaper hit effect (the
    art pass, M5) would remove it; it wasn't changed here because it changes the look.
  - The frame numbers are from PIE in the editor with a 120 fps cap (frames under 8.33 ms read 8.33). A packaged build
    has no editor overhead (Slate alone is ~2 ms of game thread in PIE).
  - The first measuring session (a freshly started editor) read much higher (100 cops with weapons 21.7 ms, GPU 6.4 ms);
    a restarted session read 15.5 ms with the same setup and GPU 3.8 ms. Only numbers from one session are compared.
  - From round 26 one round alone has about 100 cops; with overlapping rounds and slow kills the cap is reached earlier
    (`Round.AliveCap100`: 250 cops in three open rounds, 100 alive).
  - Waves are now about twice as big for the same flat cash and XP per cop, so income per round roughly doubles from
    round 6. Balance (and the M3 prices) comes from the M2d checkpoint (69).
  - Skipping the pose off screen gained nothing measurable (cops behind the camera still cast shadows into view, and most
    of a crowd is on screen); it is kept because it costs nothing and helps when the crowd is spread out.
- Roll-over rounds (Prompt 59):
  - The full suite went from 180 s to 250 s: the three new round tests take 54 s together (`Round.RollOverPace` 27 s,
    it plays rounds 1–9 at ×4).
  - Nothing yet fills the breather (a boss goes there later); it is 10 s of nothing spawning, with the countdown.
  - The safety guard is 180 s (the asset's value): a round before a breather that keeps one unreachable enemy holds
    the run for 3 minutes before it is cleared. Other rounds have no guard: their leftovers just stay while the next
    rounds roll on.
  - From round 5 on every round arrives in about 20 s while the fast-clear limit keeps growing (8 s + 1.5 s per enemy),
    so the fast-clear bonus gets easier with the round (table under Prompt 59).
  - Roll-over and its pop-ups were checked with `gs.KillAll`, not real shooting; how the stream feels in real play is
    the M2d checkpoint's (69).
  - `AGSGameState::OnWaveCountsChanged` and the per-round stats are server-only, as before (`TODO(co-op)`).
- Enemy shots (Prompt 58):
  - The bolt is an opaque emissive mesh with its tail modelled in; there is no particle trail, and the impact flash and
    the two click sounds are engine / editor placeholders (the sounds may not cook).
  - Shots aim at where the player is with no lead, so a player who keeps moving is rarely hit at range, and one who
    stands still takes every shot inside the spread (3.5° cone, ±1.2 m at 20 m). Balance with real play.
  - The Patrol Officer has no aim laser since Prompt 59b; its warning is the 0.4 s body glow. (The laser pointing along
    the facing instead of at the player moved to the Sniper's line in Next up.)
  - The carbine has no muzzle effect set (`DA_PatrolCarbine.MuzzleEffect` is empty), so a shot shows only the bolt and
    the fire sound; a placeholder muzzle flash could help the player see who fired.
  - Bolts pass through target dummies (they aren't players).
  - With the cap, generated waves have more enemies for the same budget (melee costs 1, ranged 2): round 10 has 22
    instead of 20, round 14 30 instead of 26. Wave sizes are revisited with the new types and the 100 cap (Prompt 60).
  - Not replicated (`TODO(co-op)`).
- Cop models (Prompt 57):
  - The baton and the carbine follow `hand_r` of the `ABP_Unarmed` pose (yaw 90 on the bone, like the player's rifle),
    so both stick out sideways from the hanging hand. There are no holding, aiming or swinging animations yet.
  - The wind-up glow is the old enemy materials' colour (6, 4.5, 0.5) at full strength on every body slot: the whole
    body turns bright yellow during a wind-up, which is very loud next to the low-poly colours. The props don't glow.
  - `M_Enemy_BeatCop` is now only the parent of `MI_CashCoin` / `MI_CashBill`; `M_Enemy_PatrolOfficer` is unused.
    Both are kept (renamed), not deleted.
  - `Art/Blender/Props.blend` still has the first, free-standing `SM_PoliceCap`; the exported cap now comes from
    `LowPolyBody.blend` (placed on the head). Export the cap from there.
  - The game thread rose by about 0.6 ms with 34 cops (props on every body); the frame stays at the 120 fps cap.
- Placeholder pipeline and the robber (Prompt 56):
  - The level-up glow (`M_LevelUpGlow`) still lacks the skeletal-mesh usage flag (known since Prompt 14), so on
    `SK_LowPolyBody` the overlay falls back to the default material (tests log "Overlay material with missing skel mesh
    usage flag"). It doesn't reach the worn props either.
  - The rifle and pistol are still the engine-cube placeholders (Prompt 68), held by the `ABP_Unarmed` hand pose:
    there are no weapon-holding, slide or aim animations yet.
  - Rigid one-bone parts leave small gaps or overlaps at bent knees, elbows and the neck. That's by design (no weight
    painting).
  - The ski mask and loot bag aren't part of the run summary or any gameplay; they are cosmetic components built in
    `OnConstruction`.
  - `AssetTools.exists` reported false for assets that exist in this session, and `AssetTools.delete` removed
    `_ImportTest/SK_Body_FbxFactory` from the registry but left its file until the editor closed. Use `find_assets` to
    check.
  - The two `.blend` files and every export are in git LFS; re-export after editing a .blend, or Unreal and the
    exports drift apart.
- Tomahawk (Prompt 52):
  - Placeholder look: a flattened grey engine cone spinning end over end, the RadialBurst template on hits, an editor
    click sound that may not cook. No trail, so it is hard to follow at 20 m/s.
  - It flies level at the throw height (40 cm above the capsule center): it passes over a crouched or short enemy only
    if they are lower than its sweep, and on a slope it can hit the ground and turn back early.
  - On the way back it flies through walls (so it always comes home) and homes on the player; a player who moves fast
    makes the return longer (it is gone after 4 s whatever happens).
  - With fewer targets than tomahawks, the extra ones go at the same targets again (they still pass anything on their
    line).
  - Not replicated (`TODO(co-op)`).
- Molotov (Prompt 51):
  - Placeholder look: a dark green cylinder for the bottle, a flat orange glowing disc for the fire (no flames, no
    particles), editor sounds (`VR_click3_Cue`, `VR_negative_Cue`) that may not cook. Big patches (high power) cover the
    floor in flat orange.
  - Since Prompt 52b the patch lands where the target will be (its velocity at the throw × the flight time). A cop
    that turns, stops or reaches the player during the 0.6 s flight can still be out of it. Cops walk through the
    fire rather than avoiding it (no AI reaction to fire).
  - Burns ignore walls (an enemy behind a thin wall inside the radius burns), like Barbed Wire.
  - The burn damage and radius are set when the bottle is thrown; a pick doesn't change a patch already burning.
  - Every burn shows a damage number per enemy: with a crowd in a patch that is many numbers every 0.5 s.
  - Not replicated (`TODO(co-op)`).
- Lucky Coin (Prompt 50):
  - Placeholder look: a small gold disc spinning, the RadialBurst template on impact, an editor click sound that may not
    cook. The bounce is a straight homing leg from the hit point, with no arc.
  - A coin hits the first living enemy it touches (like the other projectiles), so a leg can hit an enemy in front of
    its target; that enemy then counts as hit for the flip and the chain goes on from it.
  - A bounce needs a clear line from the enemy just hit; a coin that misses (a wall, or its 1 s flight runs out) ends
    the flip.
  - Not replicated (`TODO(co-op)`).
- Dynamite (Prompt 49):
  - Placeholder look: a red cylinder tumbling through the air, the `SimpleExplosion` template scaled to the blast, editor
    sounds (`VR_click3_Cue`, `VR_negative_Cue`) that may not cook.
  - Since Prompt 52b the stick lands where the target will be (its velocity at the throw × the flight time). A cop
    that turns or stops at the player during the 0.6 s flight can be missed, and one running straight at the player can
    have its lead land past the player (the blast then still covers the player's spot, but not always the cop).
  - The lead uses the velocity at the throw only, with no path prediction: a cop pathing round cover can leave the
    led spot.
  - The blast has no line-of-sight check (like Powder Keg): enemies behind a wall inside the radius are hit.
  - `Upgrades.Dynamite` took about 23 s; since 52b it runs on a test copy with a 1 s interval (about 7 s).
  - Not replicated (`TODO(co-op)`).
- M2b + M2c checkpoint (Prompt 48), seen in the PIE loops:
  - The PIE loops cleared rounds with god mode and `gs.KillAll`, as in Prompt 28, so health stayed full: the health
    orbs (14 dropped, all collected) healed nothing, and the round times aren't real kill speed.
  - The rifle's numbers in the loops are small because MCP can't hold fire (`Input.+key` releases after a few
    frames). They don't measure the rifle.
  - With both guns at power 2 the Floating Gun cleared rounds 5 and 6 (13 and 12 enemies) on its own in 39 s and 32 s,
    while the player stood still in god mode. It dealt 2,821 of loop 2's 5,518 damage (2,416 were `gs.KillAll`).
  - All 7 offers in the two loops had a weapon card. Only the first is guaranteed; the other six came at the normal
    48% chance (about 1.2% for six in a row). `Upgrades.WeaponSlots` checks that later offers are rolled as usual.
  - Loop 1's round 1 counted 5 kills for a wave of 4. It may have been a target dummy in the plaza hit by the
    rifle; not looked into.
  - Damage dealt by source is in `gs.Run.Stats`, the `[round]` line and the "Run over" line, but not in the run
    summary.
- Barbed Wire (Prompt 47):
  - Placeholder look: 48 grey cube segments in a circle (no torus in the engine content), no barbs, no animation. The
    hit effect is the RadialBurst template at ×0.06; even halved from ×0.12, 30 of them every 0.5 s are busy on screen.
    A proper small spark effect is still needed.
  - "Inside" is the enemy's center within the radius (flat), so a cop whose body overlaps the wire but whose center
    is just outside isn't hit. It also ignores walls: an enemy behind a wall inside the radius is hit.
  - Every tick hits everyone inside at once (30 damage numbers in one frame with a crowd).
  - Not replicated (`TODO(co-op)`).
- `M_XPOrb` got the "Used with Instanced Static Meshes" usage flag (Prompt 47) for the ring's material instance;
  without it the ring rendered with the default material.
- Endless levels (Prompt 45): XP amounts are int32 (`FGSProgressionState`, `AddXP`), so a run's total XP tops out at
  about 2.1 billion, around level 11,960; `gs.XP.SetLevel` above that uses the highest level that fits (with a
  warning). Far beyond real play. A single gain of many levels applies every level's reward and queues every offer
  (level 1 → 100 at once: 99 offers), which is instant at that size.
- Move Speed (Prompt 44b): at a high Move Speed a dash or slide into cover that is turned a few degrees skids round it and
  keeps going (×33: 127–184 m past a tower or shack); it never passes through. A ×33 dash covers 198 m, two-thirds of the
  town. No limit was added (44b asked only for a report); revisit with real play.
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
- Razor Cards (Prompt 46):
  - Placeholder look: a white unlit box spinning flat, an editor click sound that may not cook, the RadialBurst template
    on impact. In a still capture the small fast cards are hard to see (motion blur streaks); there is no trail.
  - Several cards in flight all home in on the target they were thrown at; when it dies, the rest fly on straight and
    usually expire (they don't pick a new target), so some damage is wasted on the last hits of a kill.
  - A card hits the first living enemy it touches, not only its target, like the gun's bullets.
  - The turning circle is ~2.4 m (30 m/s at 720°/s): a card thrown at a cop that is right next to the player and
    moving sideways can circle before it hits. Not seen in the tests.
  - The first pick's `[upgrade]` log line reads "+0% Razor Cards damage and fire rate" (a Common first pick adds no
    power; the card hides that line). The Floating Gun has the same.
  - It targets by distance only, like the Floating Gun, and isn't replicated (`TODO(co-op)`).
- Floating Gun (Prompts 38–39):
  - The guns split targets by "not the other gun's", not by side: with two cops the right gun may shoot the one on
    the left (seen in the test), so their shots can cross.
  - Since Prompt 39 the "+10%" in the level 3+ card text is typed by hand; retuning the bonuses needs the card text
    changed too (`Upgrades.FloatingGunLevels` fails until they match).
  - The player's Fire Rate upgrade doesn't speed up the guns (still an open question).
  - At level 2 with the left-shoulder camera, the left gun sits where the right gun sits with the right-shoulder camera
    (mirrored), so the same crosshair push applies; only the right gun's 24 views are measured in a test.
  - Placeholder look: a gold box for the gun, a glowing sphere for the projectile, an editor click sound that may not
    cook, and the RadialBurst template on impact. There is no muzzle flash or tracer.
  - It targets by distance only. It doesn't prefer the enemy the player is shooting; it picks the nearest again
    every 0.1 s, so it can switch targets mid-fight.
  - The clear-line check and the projectile ignore other enemies on the way, so a projectile can hit an enemy in front
    of its target (it hits the first one it touches).
  - Damage is set when the shot is fired, so a Damage pick doesn't change projectiles already in the air.
  - At steep downward pitches over the right shoulder the gun is pushed 10–20 cm away from the crosshair; the push
    can be seen as a small slide.
  - Gun, projectiles and sounds aren't replicated (`TODO(co-op)`).
- Every rarity tier now has upgrades. Epic has Piercing Rounds, Powder Keg and Floating Gun.
- Powder Keg (Prompt 35):
  - The explosion and sound are placeholders (the Niagara template `SimpleExplosion`; an editor sound that may not
    cook). The first blast of a session costs a ~140 ms frame while they load; warm chains peak at ~29 ms.
  - Effect and sound play only on the server (`TODO(co-op)`).
  - A blast that kills many enemies drops their cash and XP orbs together. In the PIE chain the orbs leveled the
    player up twice at once (the upgrade screen paused the game).
  - The card shows stacks x `DisplayScale` 3 (Prompt 42), set by hand to match ChancePerStack; `Upgrades.PowderKeg`
    fails until they match. No pick cap since Prompt 44.
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
    ranged (8 of 12, 9 of 13). The ranged share is decided (M2d item 7: at most 40% ranged, built in Prompt 58); the
    smaller round 6 is still open.
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
player in 9.4 s; the 3 Patrol Officers alone took 19.3 s.

**Placeholder art**:
- The player is the robber since Prompt 56: `SK_LowPolyBody` (Blender, low poly) with `SM_SkiMask` and `SM_LootBag`;
  see CLAUDE.md **Placeholder models**. Props ready for the cops: `SM_PoliceBaton`, `SM_PoliceCap`, `SM_PatrolCarbine`.
- Enemies (Prompt 57): `SK_LowPolyBody` with `MI_BeatCop_*` / `MI_PatrolOfficer_*` colours, `SM_PoliceCap` (both),
  `SM_PoliceBaton` (Beat Cop) and `SM_PatrolCarbine` (`DA_PatrolCarbine.Mesh`).
- Enemy shots (Prompt 58): `SM_EnemyBolt` (Blender) with `MI_EnemyBolt_Core` / `_Trail`, the Niagara
  `DirectionalBurstLightweight` template on impact, `VR_click2_Cue` / `VR_click1_Cue` (editor sounds).
- `MI_CashCoin` / `MI_CashBill` are instances of `M_Enemy_BeatCop` (changing that material changes the cash).
- The telegraph uses `M_EnemyLaser`.
- Level-up: `M_LevelUpGlow` overlay and the `CompileSuccess` editor sound.
- XP orb: `M_XPOrb` on the engine sphere.
- Health orb: `M_HealthOrb` (green copy of M_XPOrb) on the engine sphere, and the `VR_confirm_Cue` editor sound.
- Floating Gun: the engine cube with `M_FloatingGun` (gold copy of M_XPOrb), its projectile the engine sphere with
  `M_FloatingGunProjectile`, and the `VR_click1_Cue` editor sound.
- Air jump: the Niagara `RadialBurst` template and the `VR_teleport_Cue` editor sound (may not cook into a packaged
  build).

**Warnings**:
- `C4701` potentially uninitialized `BestLocation` in `AI/GSSpawnLocationFinder.cpp:262`.
- Every PIE test logs `LogCrowdFollowing: Unable to find RecastNavMesh instance…` (harmless so far).

**Leftover template code and content**: `Content/ThirdPerson`, `Variant_*` (reference only), and
`BanditBallerCharacter` / `GameMode` / `PlayerController`. `AGSPlayerController` derives from
`ABanditBallerPlayerController`.

**Environment gotchas**:
- Windows Smart App Control blocks freshly built unsigned DLLs: the editor shows "Bad Image … 0xc0e90002" and "The game
  module 'BanditBaller' could not be loaded". Check `Microsoft-Windows-CodeIntegrity/Operational` (events 3077 / 3118). It
  switched to On on 2026-11-22 and the user turned it off. A shipped build needs code signing (release milestone).
- The Claude desktop app is an MSIX package: files its child processes create under `%APPDATA%` / `%LOCALAPPDATA%`
  land in `%LOCALAPPDATA%\Packages\Claude_pzs8sxrjxfjjc\LocalCache` instead. Run uv installs and updates (and anything
  else that must write real AppData files, like the Blender add-on) in a normal PowerShell from the Start menu
  (Prompt 55).
- There is no Python on this machine (`python` isn't found). uv has its own managed Python 3.15.2 since Prompt 55 (used
  by uvx only; still no `python` on PATH). Use PowerShell, the Edit tool, or MCP's
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
- MCP calls now arrive within a frame or two (since the Prompt 56 session; earlier notes say 1–3 s). Wait with
  PowerShell `Start-Sleep` between a command and what should follow it (a run before a slide, a camera blend before a
  screenshot); `slomo 0.1` helps catch a move mid-way. Screenshots: `console_command("shot")` →
  `Saved/Screenshots/WindowsEditor/`.
- The `cd` in a Bash call moves the session's working directory for later calls; a folder can't be renamed while a
  shell sits in it ("in use").
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
38. ~~**Floating Gun, part 1** (Epic): the projectile and the first gun (right shoulder). Its kills count as the
    player's.~~ Done (see Prompt 38).
39. ~~**Floating Gun, part 2:** the second gun (left shoulder) and the +10% damage / fire rate picks from the 3rd on.~~
    Done (see Prompt 39).

The M2b checkpoint (planned as Prompt 40) moved to Prompt 48, after the rework.

**Next: M2c — upgrade rework** (see **Design update — upgrade rework (M2c)** for the design, the rules and the open
questions). Each prompt updates the tests that assume the old design.

40. ~~**M2c docs.**~~ Done (see **Design update — upgrade rework (M2c)**; docs only).
41. ~~**Upgrade data and per-pick values:** the three categories (and Legendary abilities), the rarity multipliers
    (×1 / ×1.5 / ×2 / ×3) and whole-unit values (+1 / +2 / +3, no Rare) in data; a pick of a given rarity applies that
    much; weapon power and the per-weapon steps (the Floating Gun's levels become steps); Second Wind as charges.~~
    Done (see Prompt 41): offered picks stay Common until 42; picks 1 and 2 of the Floating Gun give its guns.
42. ~~**Offer roll and cards:** a rarity rolled per card on the run stream with the category weights (100 / 20 / 6 / 1;
    Abilities 100 / 10 / 3 / 0.5); the 0.8% Legendary ability chance; cards show the rolled rarity and the pick's value;
    the simulator.~~ Done (see Prompt 42). MaxWeapons and the first level-up's weapon card moved to 43.
43. ~~**Weapon slots and HUD:** three weapon slots in the top left, like the upgrade icons, filled in pick order (icon
    only); MaxWeapons 3 and owned-only weapons after that; a weapon card in the first level-up of a run (moved from 42).~~
    Done (see Prompt 43).
44. ~~**Overflow effects and no pick caps:** every MaxStacks removed; Loaded Dice past 100% gives a chance of a
    triple-damage hit, Powder Keg past 100% a chance of a double-size blast.~~ Done (see Prompt 44).
44b. ~~**Fix: slide and dash scale with Move Speed** (the snag found in 44): SpeedBoost / MaxSpeed / EndSpeed and the dash's
    Distance / ExitSpeed × MoveSpeed ÷ base walk speed (at least 1), kept per slide or dash.~~ Done (see Prompt 44b).
45. ~~**Endless levels:** no level cap; the XP needed per level grows by a fixed amount.~~ Done (see Prompt 45): 100 for
    level 2, +30 per level after that.
46. ~~**Razor Cards** (weapon).~~ Done (see Prompt 46): homing playing cards at the nearest enemy.
47. ~~**Barbed Wire** (weapon).~~ Done (see Prompt 47): a damaging ring around the player.
48. ~~**M2b + M2c tests + checkpoint:** tests for the M2b and M2c rules and a PIE checkpoint, like Prompt 28. Replaces
    the old Prompt 40.~~ Done (see Prompt 48 and **M2b + M2c checkpoint**). **M2b and M2c are complete.**

**Then more weapons** (each follows the rules for every weapon):

49. ~~**Dynamite** (weapon).~~ Done (see Prompt 49): a stick lobbed at the biggest group, a 3 m blast where it lands.
50. ~~**Lucky Coin** (weapon).~~ Done (see Prompt 50): a coin that bounces from enemy to enemy, each once per flip.
51. ~~**Molotov** (weapon).~~ Done (see Prompt 51): a bottle that leaves a burning patch for 4 s.
52. ~~**Tomahawk** (weapon).~~ Done (see Prompt 52): a tomahawk that cuts through a line of enemies and comes back.
52b. ~~**Fix: Dynamite and Molotov lead their target**, plus tidying from 49–52 (PROGRESS.md repair, faster weapon tests,
    the unity-build naming rule).~~ Done (see Prompt 52b).
53. ~~**Weapons checkpoint.**~~ Done (see Prompt 53 and **Weapons checkpoint**). **The weapons block is complete.**

**Next: M2d — cops, city and round flow** (after the weapons checkpoint, before M3; see **Design update — cops, city
and round flow (M2d)** for the decisions and open questions). Each prompt updates the tests that assume the old design.

54. ~~**M2d docs.**~~ Done (see **Design update — cops, city and round flow (M2d)**; docs only).
55. ~~**Connect Blender:** a Blender MCP server for this project, checked from a session.~~ Done (see Prompt 55); both
    servers checked after the restart.
56. ~~**Placeholder pipeline and the robber:** Blender → Unreal for low-poly placeholders (export, import, materials in
    the low-poly look), and the player's robber model on today's skeleton and animations.~~ Done (see Prompt 56).
57. ~~**Rename the enemies and give them cop models.**~~ Done (see Prompt 57).
58. ~~**Dodgeable ranged shots and the ranged cap.**~~ Done (see Prompt 58).
59. ~~**Roll-over rounds and the breather.**~~ Done (see Prompt 59).
59b. ~~**Fix: no aim laser on the Patrol Officer.**~~ Done after Prompt 60 (see Prompt 59b).
60. ~~**One hundred enemies:** the alive cap to 100 at 60 fps on the user's PC (profile and optimize).~~ Done (see
    Prompt 60).
61. **Rusher** (unlocks in round 4).
62. **Shield** (round 6).
63. **Heavy** (round 10).
64. **Grenadier** (round 8).
65. **Sniper** (round 12). Note (from Prompt 59b): the aim-laser code (`AGSEnemyCharacter::UpdateAimLaser`) points the
    beam straight along the enemy's facing, from the muzzle; the Sniper's laser has to point at the player (and follow
    them, then lock), not along the facing.
66. **Drone** (round 14).
67. **The city map:** a 500 × 500 m city graybox with a 100 m ceiling replaces the town (NavMesh, boundary, spawns,
    tests).
68. **Remake the older placeholders in Blender:** the engine-shape placeholders (weapons, pickups, portal, cover) in
    the low-poly look.
69. **M2d tests + checkpoint:** tests for the M2d rules and a PIE checkpoint, like Prompt 53; its income numbers set
    the M3 prices.

**Then M3** (after the M2d checkpoint; not planned in detail yet; each item needs its own prompt). From the decisions so far:
- The two-layer save (playthrough data: cash, owned items, loadout, total cash earned; prestige data, empty until
  prestige), with an upgrade step from save version 1.
- Item types: usable items (consumables, bought again to restock, used up in a run) and gear (bought once, one per
  loadout slot). Other types (abilities, powerups) are decided in M3.
- The static shop in the hub (a fixed catalogue, `TrySpendCash`), owned items in the profile, the hub loadout, and
  owned things applied at the start of the next run.
- Prices set from the M2d checkpoint's income numbers.

Suggestions from the M2 checkpoint (not scheduled): look at the DT_Waves → budget hand-off (round 6 is smaller than
round 5), and replace `gs.KillAll` play with real shooting before balance work. (The ranged share is decided: M2d
item 7.)

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
  (M2b). The rarity weights are still placeholders. (Replaced by M2c: rarity is rolled per card.)
- **M2b upgrades** (after M2, before M3; the pool grows from 4 to 15). Every number is a starting value in data; no
  cap unless noted. (M2c replaces the rarities and caps below with rarity per card and no caps; the per-pick values
  become the Common values.)
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
- **Upgrade rarities and rerolls:** four tiers (Common, Rare, Epic, Legendary), rolled per card since M2c. The 3
  choices can be rerolled a limited number of times per run (3 to start). There is no skip.
- **Upgrade rework (M2c, Prompt 40).** Every number is a placeholder in data.
  - Categories: Stat Boosts (Damage, Fire Rate, Reload Speed, Move Speed, Magnet, Bigger Mags, Loaded Dice),
    Abilities (Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech, Powder Keg; Legendary abilities are
    a sub-group: Second Wind today, more later) and Weapons (automatic attacks: Floating Gun; Razor Cards and Barbed
    Wire next; Dynamite, Lucky Coin, Molotov and Tomahawk after the checkpoint).
  - Rarity is rolled for each card. A higher rarity gives a bigger pick: ×1, ×1.5, ×2, ×3. Whole-unit upgrades
    (Extra Jump, Dash Charge, Floating Shield, Piercing Rounds, Leech) give +1 / +2 / +3 and have no Rare version.
  - Rarity weights per card: Stat Boosts and Weapons 100 / 20 / 6 / 1; Abilities 100 / 10 / 3 / 0.5.
  - Legendary abilities have no rarities: each card has a fixed 0.8% chance to be one, and a pick adds one charge.
    Second Wind charges are used one at a time; the next can save the player once the last one's invulnerability
    has ended.
  - Weapons: at most `MaxWeapons` 3 different weapons per run; until then any weapon can be offered, after that only
    the owned ones, as upgrades. The rifle and pistol aren't part of this. The first level-up of a run always
    includes one weapon card.
  - Weapon picks: the first pick gives the weapon. Every pick adds weapon power equal to its rarity multiplier; the
    first pick adds the multiplier − 1, so a Common first pick is the plain weapon. Each point of power is one
    upgrade step, defined per weapon.
  - Every weapon: attacks on its own; damage × the player's Damage, can crit (Loaded Dice); Piercing Rounds doesn't
    apply; its kills are the player's; it doesn't act while the player is dead, in a safe zone or while the game is
    paused; exact schedule (as Prompt 31b); no collision; a new run removes it.
  - No pick caps. Loaded Dice past 100% becomes a chance of a triple-damage hit, Powder Keg past 100% a chance of a
    double-size blast.
  - Endless levels: no level cap; the XP needed per level grows by a fixed amount instead of 25%.
  - HUD: three weapon slots in the top left, like the upgrade icons, filled as weapons are picked. Icon only.
  - The roll (Prompt 42): per card the Legendary ability chance first, else an upgrade by AppearanceWeight across the
    three categories and a rarity from its category's weights. A whole-unit upgrade's Rare roll counts as Common.
- **Casino bets** risk cash only, never owned items.
- **XP pickups:** XP comes from pickups enemies drop, not instantly on the kill.
- **Map boundary:** every map has invisible walls around the play area and a ceiling, blocking pawns only.
- **Cops, city and round flow (M2d, Prompt 54).** Every number is a placeholder in data.
  - Theme: cops and robbers. The player is the robber, out to get all the cash; the enemies are law enforcement. The
    project and code names (BanditBaller, GS) stay.
  - Setting: runs take place in a city; the hub stays the black market and casino. A 500 × 500 m city graybox with
    a 100 m ceiling replaces the 300 × 300 m town (75 m ceiling) later in M2d.
  - Look: low poly (faceted, flat shaded, clean blocks of colour, no texture detail) with high-quality shaders and
    lighting (coloured rim light, glowing emissives with bloom, coloured shadows, soft gradient skies); reference
    RevenBlade. The art pass is still M5; placeholders follow the look until then.
  - Placeholder models are made in Blender, not from engine shapes: the robber and every enemy, very basic low poly
    with props (cap, baton, carbine, shield), on today's skeleton and animations (no new animations yet).
  - Names: MeleeBandit → Beat Cop (baton), RangedBandit → Patrol Officer (carbine); the code keeps the old names
    until Prompt 57.
  - Ranged enemies fire visible, dodgeable projectiles (about 28 m/s, aimed at where the player was when fired, no
    lead); the aim warning and the burst stay; no enemy uses hit-scan.
  - At most 40% of a generated wave is ranged (all ranged types together), for now.
  - Eight enemy types unlock by round (no wanted level, no agency tiers): Beat Cop 1, Patrol Officer 2, Rusher 4,
    Shield 6, Grenadier 8, Heavy 10, Sniper 12, Drone 14. Rusher: half health, ×1.5 speed, packs of 4–6, a quick weak
    hit. Shield: blocks the rifle, pistol and weapon projectiles from the front (area damage, Piercing Rounds and side
    or rear hits get through), breaks after enough damage into a normal cop, slow, melee bash. Grenadier: 12–20 m
    away, a grenade every 4 s with a 1.5 s red circle, doesn't hurt enemies. Heavy: ×8 health, slow, a ground slam
    with a 3 m circle 1 s before it lands, ×4 cash, always drops a health orb. Sniper: 30–45 m away on the ground, a
    laser follows for 1.5 s, locks and flashes 0.4 s, then one very fast visible heavy shot along the locked line
    (moving or dashing dodges it, cover blocks it). Drone: flies over cover at about 3 m, low health, hovers, beeps
    before shooting, dodgeable shots, explodes when shot down and hurts nearby enemies, never the player.
  - Alive cap 30 → 100 in M2d, holding 60 fps on the user's PC. Thousands at once (like Megabonk) is a later,
    undesigned block.
  - Roll-over rounds: rounds stay but feel continuous. The next round's enemies start arriving when only a few of the
    current round are left, with no countdown; a steady stream instead of a few batches; the round-clear bonus as a
    small pop-up while play goes on. Level-ups still pause for the card pick.
  - A 10-second breather every 10 rounds; a boss goes there later.
  - M2d ends with a checkpoint whose income numbers set the M3 prices.

Still open from M2: more DA_LevelRewards entries.

## Later — thousands of enemies

Not designed yet; noted in Prompt 60. The long-term goal is thousands of enemies at once (like Megabonk). Today every
enemy is a full character (CharacterMovement, a StateTree AI controller with sight, an ability system, a skeletal mesh
with animation), and 100 of them cost about 6.7 ms of game thread in PIE. Thousands need a lighter kind of enemy for the
common types (for example Mass entities with instanced or vertex-animated meshes and simple steering), with full
characters kept for the few special ones. Nothing about it is decided.

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

## Side project — Grave Shift (web zombie shooter)

Not part of BanditBaller and not an Unreal asset: a separate browser game asked for in a cloud session (no editor, no
MCP). It doesn't change **Next up**.
- **What:** a first-person, round-based zombie survival shooter that runs in a phone browser. One file,
  `web/grave-shift/index.html` (three.js r128 from cdnjs, everything else inline, procedural art and sound).
- **Built:** touch controls (left thumb moves, right thumb aims, FIRE / RELOAD / SWAP, a contextual buy button) plus
  keyboard and mouse; cash for hits (+10) and kills (+60, headshot +100, explosive +50); four wall guns with ammo
  re-buys; the "unclaimed freight" mystery crate ($950, nine weapons, four of them crate-only); four pay-to-raise
  shutters into new bays; four power-up drops; a health tonic and an overclock bench as late cash sinks; rounds with
  growing zombie count, health and speed; best round saved in the browser.
- **Checked:** a headless phone-sized Chromium run (landscape and portrait) through start, kills paying out, a wall
  buy, a shutter, the crate, the touch stick and fire button, pause and death. Not yet played on a real phone.
- **Known issues:** frame rate on a real phone is unmeasured; balance is first-pass; opening it needs a connection
  for three.js and the fonts.

### Grave Shift update — more guns, the bank, the locker and the break room
- **Guns (19 in all):** three more wall guns (Stubby Double $750, Tally M3 burst rifle $1,300, Longshot .308 $1,700),
  three more crate-only standards (Hornet PDW, Riot Auto-12, Carver BR) and four more crate-only specials whose damage
  keeps pace with the round: Nova Pistol (energy bolts with a small blast), Frost Jet (freezes a cone; frozen zombies
  take double damage), Rail Spike (pierces everything on its line) and Sinkhole (a vortex that pulls zombies in, then
  bursts). The crate now rolls from 18 weapons.
- **Bank:** every shift pays into a permanent bank when it ends (death, or "End shift" from pause):
  (30 x rounds survived + 2 x kills + 2 x headshots + 2% of cash earned) x Overtime x Double Time. New saves start
  with $250.
- **Main menu:** Start shift, Locker and Break room. The time card has "Main menu" next to "Play again".
- **Locker (permanent upgrades, bought with the bank):** Thick Skin, Head Start, Hollow Points, Quick Hands, Deep
  Pockets, Steel Toes, Overtime (each 4-5 levels) and Packed Heat (start with a second gun).
- **Break room:** blackjack (3:2, dealer stands on 17, double down), single-zero roulette (numbers, dozens, even-money
  bets, rebet) and a three-reel slot (about 94% returned). Bets use the bank only.
  - **Charms** (only won by gambling, three levels each): Lucky Star, Card Shark, Ace Up the Sleeve, Inside Man,
    House Edge, Hot Hand. Won by a natural blackjack, a straight-up roulette hit or three stars on the slots.
  - **One-shift buffs:** Contraband (start with a special weapon; three 7s, three BARs or a zero hit) and Double Time
    (double pay; three blackjack wins in a row).
- **Save:** in the browser (`localStorage`), and in the viewer's private store when opened as a claude.ai artifact
  (`db` + `user` capabilities); the newer save wins.
- **Checked:** the headless run covers the three games (payout maths enumerated: slots 93.8%, red 97.3%), every
  upgrade bought, all charms and both buffs applied to a shift, every weapon fired, the special weapons against a
  placed group, pay at death and at End shift, and the bank surviving a reload. The claude.ai store was empty when
  checked, so the cloud save itself is untested.
- **Known issues:** balance of pay, prices and specials is first-pass; a blackjack hand left mid-way stays open only
  until the page closes (the bet is already taken).

### Grave Shift update — bigger depot and eight perk machines
- **Map:** 37 × 29 cells (was 23 × 18; about 2.3 times the floor). Eight areas instead of four: the Loading dock (start),
  an open Rail yard in the middle, and the Scrapyard, Canteen, Cold store, Power house, Warehouse and Workshop in a ring
  around it. Twelve shutters ($750–$1,500): the ring connects each area to its neighbours and the yard connects to five
  of them, so there are several ways round. The yard and the scrapyard are open to the sky.
  - The map is now fully described by the `MAP` rows: walls, coloured containers (`R U N Y`), crates, spawns (32),
    lamps (`L`), shutters (`0-9 a b`) and a letter per area (`A-H`, looked up in `AREAS`). Wall guns, perk machines, the
    bench and the crate are placed with `wallSpot(column, row, wall)`.
  - Outdoor lamps stand on masts (with collision). The HUD shows the name of the area the player is in.
- **Where things are:** carbine and double in the dock; SMG in the yard; burst rifle in the canteen; shotgun in the
  workshop; sniper in the warehouse; AR in the cold store; the freight crate in the scrapyard; the overclock bench in
  the power house.
- **Perk machines (8, one per area, kept until the shift ends, no limit on how many):** Iron Gut Tonic $2,500 (+100
  health, canteen), Snap Load Soda $2,000 (reload and swap twice as fast, workshop), Hair Trigger Fizz $2,500 (+33% fire
  rate, warehouse), Long Haul Brew $2,000 (+15% speed, grabs slow less, scrapyard), Second Shift Shot $1,500 (get back
  up once at full health; used up; 3 per shift; used before the Ace charm; dock), Dead Eye Drops $1,500 (headshots ×1.4,
  45% less spread, yard), Pack Mule Malt $4,000 (third weapon slot, cold store) and Static Sour $2,000 (a reload shocks
  zombies within 4.2 m for half a zombie's health, 4 s cooldown, power house). Owned perks show as chips on the HUD.
- **Checked** (headless phone-sized Chromium): all eight machines bought through the buy button and their effects read
  back; all twelve shutters bought; every spawn reachable with the shutters open; zombies placed in the far corners
  walked through the shutters to the player; a restart clears perks, the third slot and the shutters; the earlier menu,
  casino and weapon tests still pass.
- **Known issues:** round 1 is slower to start (the dock is bigger, first contact after about 15 s standing still);
  perk prices and strengths are first-pass; still not played on a real phone.

### Grave Shift update — three jobs, thirty rounds each, a boss at the end
- **Jobs (levels):** a shift is now one job: 30 rounds on that job's map, then its boss in a separate arena. The menu has
  a **Job board** next to the Locker and the Break room.
  - Job 1 is free. Beating a job's boss opens the next job on the board, which is then bought from the bank
    ($6,000 for job 2, $15,000 for job 3). The player picks which owned job to work.
  - Each later job is harder and pays more:

    | Job | Place | Zombie health | Hit on the player | Speed | Shift pay | Boss | Boss bonus |
    |---|---|---|---|---|---|---|---|
    | 1 Freight handler | Depot 9 (the existing map) | ×1 | 45 | ×1 | ×1 | The Foreman | $3,000 |
    | 2 Night porter | Mercy General (hospital) | ×1.6 | 60 | ×1.06, runners earlier | ×1.6 | The Surgeon | $6,000 |
    | 3 Furnace hand | Crucible Steelworks | ×2.4 | 75 | ×1.12, runners earlier still | ×2.4 | The Smelter | $12,000 |

  - **Mercy General:** one long main corridor with seven rooms off it (reception, ambulance bay, pharmacy, laundry,
    operating wing, morgue, intensive care), tiled walls, beds for cover, patients in gowns and scrubs.
  - **Crucible Steelworks:** a casting floor that runs round a furnace block, with seven rooms in a ring outside it,
    brick walls, plate floors, orange light, workers in overalls and heat suits.
  - Every job has the same shop: 12 shutters, 7 wall guns, 8 perk machines, the freight crate and the overclock bench.
- **Boss fight:** after round 30 the player is moved to the arena with full health and full ammo. The boss has a health
  bar, a second phase at half health (faster, shorter cooldowns, more projectiles) and calls in zombies. Every attack
  shows a red mark on the floor first. The arena has a free ammo locker (40 s between refills).
  - The Foreman: hammer swipe, ground slam (6 m), thrown crates.
  - The Surgeon: fast; a dash down a marked lane, fans of scalpels.
  - The Smelter: slam and lobbed slag that both leave burning pools.
  - A boss takes at most 0.6% of its health per hit, so the round-scaled special weapons can't end it in seconds; it
    can't be frozen, pulled by the Sinkhole or killed by the Blast drop, and Instakill only doubles damage on it.
  - Winning pays the boss bonus on top of the shift pay and ends the shift with a "Boss down" time card.
- **Code:** `LEVELS` holds every job (map rows, area names and colours, shop spots, zombie outfits, difficulty, boss).
  `loadDef` swaps the loaded map (a job or its arena); everything map-specific hangs off one group that is disposed on a
  swap. The save has a `lv` block (owned, boss beaten, best round, selected job).
- **Checked** (headless phone-sized Chromium): job 1 from round 30 through the arena, every Foreman attack, a kill
  through the real fire path, the time card, the unlock and buying job 2; on jobs 2 and 3 every perk machine, wall gun,
  shutter, the bench and the crate, every spawn reachable, each boss's attacks, death in the arena and "Play again".
- **Known issues:** nobody has played 30 real rounds: round 30 is 243 zombies, so a full job is a long sitting, and the
  difficulty, prices, boss health and boss damage are first guesses. A shift can't be saved part-way. The three bosses
  share one body shape (colours, hat and tool differ).
