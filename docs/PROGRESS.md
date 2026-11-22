# BanditBaller — Progress

Read this at the start of
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
- **Full suite:** 62 tests, all passing, in 181.9 s. every session. After finishing a prompt, add a short entry at the end of the current
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
as block **M2c**; see **Design update — upgrade rework (M2c)**.
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
**Next:** the weapons checkpoint (53). **Then M3** (shop, owned items, gear loadout, two-layer save); not planned in detail yet.

State at the time of writing: `main` after "Tomahawk" (Prompt 52), not pushed. The build is clean and all 62 automation tests pass in about 182 s. The game and the editor start in the hub (`L_Hub_Graybox`). Walk into the Start Run portal to
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
| W 52 | Tomahawk (weapon): every 3 s a tomahawk at the nearest enemy flies 18 m out and back, hitting every enemy on its line once each way (12 per hit); a wall turns it back; +1 tomahawk per 3 power; `AGSTomahawk`; pool of 21 | see git log |

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
- **Full suite:** 61 tests, all passing, in 173.7 s.---

## Console commands and CVars

Non-Shipping cheats. They run on the server; clients forward them through `ServerExecDebugCommand`.

| Command / CVar | What it does |
|---|---|
| `gs.Spawn <Enemy> [Count=1]` | Spawn enemies around the player via the spawn finder (`MeleeBandit`, `RangedBandit` or a full `DA_Enemy_*` name). |
| `gs.KillAll` | Kill every living enemy through GE_Damage, credited to the player (they drop cash and XP orbs; no XP until the orbs are collected). |
| `gs.God [0\|1]` | Toggle/set damage immunity (one `State.Invulnerable` count). |
| `gs.GiveWeapon <Weapon>` | Equip `Rifle` / `Pistol` / `BanditCarbine` with a full mag (replaces the equipped one if both slots are full). |
| `gs.SetAttr <Attr> <Value>` | Set a player attribute base value (Health, MaxHealth, MoveSpeed, Damage, FireRate, PickupRadius, XPGain, ReloadSpeed). |
| `gs.Upgrade.Grant <Name> [Count=1] [Rarity=Common]` | Add picks of an upgrade at a rarity (Common, Rare, Epic, Legendary or C / R / E / L; Count and Rarity in any order; whole-unit upgrades have no Rare) (`ReloadSpeed`, `Damage`, `MoveSpeed`, `FireRate`, `Magnet`, `BiggerMags`, `ExtraJump`, `DashCharge`, `Leech`, `LoadedDice`, `PiercingRounds`, `PowderKeg`, `FloatingShield`, `SecondWind`, `FloatingGun`, `RazorCards`, `BarbedWire`, `Dynamite`, `LuckyCoin`, `Molotov`, `Tomahawk` or `DA_Upgrade_*`). E.g. `gs.Upgrade.Grant ExtraJump 45`, `gs.Upgrade.Grant Damage 2 Epic`. |
| `gs.Upgrade.List` | List every upgrade by category (Stat Boosts, Abilities, Legendary abilities, Weapons) with this run's picks by rarity, total value, Common pick value, cap and description, plus ReloadSpeed / Damage / FireRate / MoveSpeed / PickupRadius / ExtraJumps / MagazineSizeMultiplier / HealthOnKill / CritChance / Pierce / PowderKeg / Shields / SecondWinds / FloatingGunLevel / FloatingGunPower, the magazine (ammo / size), the dash charges (max = base + ExtraDashCharges, ready now), the floating shields (ready / each recharge / hits blocked), the Floating Gun (picks, power, guns, shots and target), Razor Cards (picks, power, damage per card, cards a second, cards thrown, target), Barbed Wire (picks, power, radius, damage per tick, ticks, hits, enemies inside on the last tick), Dynamite (picks, power, damage per blast, blast radius, interval, throws, blasts, hits, hits of the last blast), Lucky Coin (picks, power, damage per hit, bounces, interval, flips, hits, hits of the last flip, target), Molotov (picks, power, damage per burn, patch radius, interval, throws, patches lit, patches burning now, burns, hits), Tomahawk (picks, power, damage per hit, tomahawks a throw, interval, throws, tomahawks thrown, in the air, hits), Second Wind (left / used this run) and Powder Keg (stacks, chance per kill, blast damage and radius). The Stats line also shows the slide / dash speed ratio (MoveSpeed / base walk speed, at least 1), the current slide speed cap and dash distance. |
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
| `gs.Round.Skip` | Advance to the next round phase (nothing in GameOver, after death). |
| `gs.Round.Set <N>` | Jump to round N and restart it from PreRound. |
| `gs.Round.Pause [0\|1]` | Freeze / resume the phase timer. |
| `gs.Run.Seed [N]` | Town: print the run seed, or restart the run (stream + stats, round 1) with seed N. Hub: print / set the next run's seed (0 = random); the portal passes it on. |
| `gs.Run.Stats` | Print run stats to log and screen (works on clients), plus this round's stats and the last round-clear bonus. Since Prompt 48 also the damage dealt by source (rifle/pistol, Floating Gun, Razor Cards, Barbed Wire, Powder Keg, Dynamite (Prompt 49), Lucky Coin (50), Molotov (51), Tomahawk (52), other) and the health orbs dropped / collected / healed. |
| `gs.Market.Teleport` / `gs.Market.Return` | Debug shortcuts. Teleport (in a run) opens the hub map (`AGSGameMode::OpenHubMap`). Return (in the hub) starts a run like the portal (`AGSHubGameMode::StartRun`). |
| `gs.Editor.RemoveSublevel <ShortName>` | Editor only: remove a streaming sublevel from the open map (MCP can't); save the map afterwards. |
| `gs.Editor.BuildEnemyStateTree [melee\|ranged\|all]` | Editor only: regenerate ST_MeleeBandit / ST_RangedBandit from code. |
| `gs.Debug.Round 1` (cvar) | On-screen round, phase, time left, area, wave counts, enemy HP/DMG multipliers. |
| `gs.Debug.Spawns 1` (cvar) | Draw the spawn ring and chosen points (green ring, yellow fallback) for 8 s. |
| `gs.Debug.Boundary 1` (cvar) | Draw the map boundary's play area (walls + ceiling) every frame. |
| `gs.Debug.Weapons 1` (cvar) | Debug-draw shots and log enemy shots (`[shot]`). |
| `gs.Debug.DamageLog 1` (cvar) | Log every health loss (`[dmg] victim took N from X (health) at t=`). |
| `gs.Debug.AutoPickUpgrades 1` (cvar) | Playtests from the console: every open upgrade offer is taken a frame later. It picks an unowned weapon while a slot is free, else an owned weapon, else the rarest card. An offer that is all Common with no weapon is rerolled first while rerolls last. Logs `[upgrade] auto-pick (debug): …`. |
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

## Automation tests (62, all passing, about 182 s)

`Tests/GSGameplayTests.cpp`. Each test opens L_Town_Graybox in the editor if needed (`Map.Hub`: L_Hub_Graybox),
starts PIE, waits for the player, removes enemies, runs and ends PIE. Stop PIE before running. While working, run
the area's tests; run the full suite once per prompt before the commit (see **Workflow update — faster test runs**).
`UI.RoundHUD`, `Round.PhaseLoop`, `Round.ClearBonus`, `Map.TownLayout` and `Upgrades.DashCharge` run game time ×4 and
`Upgrades.ExtraJump` ×3 (`SpeedUpWorld`). `Economy.HealthOrbs` also runs ×4. `Upgrades.FireRateAndMagnet` measures its shots at ×1 (throttling off) and
runs its magnet part at ×4. `Upgrades.FloatingGun` and `Upgrades.FloatingGunLevels` run at ×4. `Weapons.HeldFire` runs at ×4, and ×16 for its slow-frame burst. `Upgrades.MoveSpeedSlideDash` measures at ×1 (throttling off) and `Upgrades.MoveSpeedCover` runs at ×3. `UI.RoundHUD` also shortens its HUD's banners.

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
| `Progression.XPAndLevels` | DA_XPCurve BaseXP 100 / XPIncreasePerLevel 30: L2 = 100, L3 = 130, L10 = 340, L51 = 1,570, L1000 = 30,040, every step +30 and the totals equal the per-level sums to level 300 (logs the table for L2–10, 25, 50, 100); partial XP; 260 XP → level 3 with 30 carried over (HUD level, bar, "LV 3", "30 / 160 XP"), one offer open + one queued; kill gives no XP but one orb; collected orb XP = XPValue × XPGain (×1, ×2); run stats XP/level; SetLevel 10 → 0 XP; level 10 → 60 + 7 XP in one gain (past the old cap of 50) queues one offer per level (50), the remainder carries over, a next level is always there; new run resets. |
| `Progression.EndlessLevels` | Total XP to level 100 = 155,430; `gs.XP.Add` 100 → level 2 (measures the MaxHealth per level), then the rest in one `gs.XP.Add` → level 100, 0 XP, 3,070 for 101, total XP, run stats level 100; MaxHealth = base + 99 rewards and a full heal; HUD "LV 100", "0 / 3,070 XP", empty bar, MaxHealth; 99 offers (one open, 98 queued, paused; the first has a weapon card), taken one by one: each has 3 different cards and the queue goes down by one, 99 picks, no reroll used, unpaused; 1,535 XP → "1,535 / 3,070 XP", bar half; another 1,535 → level 101 ("LV 101", "0 / 3,100 XP"), one offer; `gs.XP.SetLevel 250` → level 250, 7,570 for 251, "LV 250"; a new run → level 1, "LV 1", "0 / 100 XP", base MaxHealth. |
| `Progression.LevelUpRewards` | DA_LevelRewards wiring; +10 per stack; damaged → two level-ups → MaxHealth +20, Health full, 2 stacks, HUD health/level/XP follow; same in RoundClear; new run removes stacks and clamps Health; dead player gets the stack but no heal. Takes each upgrade offer (choice 0) so the game unpauses. |
| `Progression.LevelUpFeedback` | DescribeRewards "+10" / "+20 Max Health"; kill → no popup, collected orb → "+XPValue XP" at the collection point; double level-up → banner "Level 3" + "+20 Max Health", label, XP bar pulse, one sound, player glow; non-kill XP pops above the player; level-up during the banner merges (level 4, "+30"); 3 upgrade offers taken; banner/pulse/glow end. |
| `UI.UpgradeChoice` | Choice widget from the PlayerController; level-up pauses (GameMode owns the pause) and the cards wait for the banner; cards match the offer (order; category, rolled rarity text and frame color, pick line, "Now:" line, a description); the pick gets the card's rarity; "Reroll (3 left)", cursor shown; key / click ignored during the 0.3 s lock; R, gamepad Y and the button reroll (label 2/1/0, new cards, lock again), greyed out at 0 and R does nothing; key 2 picks the 2nd card, last pick hides, unpauses, cursor off; 1000 XP → 1 open + 3 queued; D-pad selects, A picks; next offer swaps in at once (no banner wait), locked, still paused; click / numpad 1 / key 3 pick the rest; hidden and unpaused after the queue. |
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
| `Upgrades.FloatingGun` | DA_Upgrade_FloatingGun (Epic, no cap, "+1", GE class, in the pool); DA_FloatingGunSettings on the GameMode (8 damage, 1.5 / s, 20 m, 60 m/s, GE_Damage, meshes); level 0: no gun; `gs.Upgrade.Grant FloatingGun` → level 1, one gun on the right: to the right of the player and above the top of the head in the camera's frame, no collision; a frozen bandit 10 m ahead: 4 s of shooting with every frame on the exact schedule (shots = 1 + floor(elapsed / interval)), at least 6 shots, every hit 8; Damage ×1.5 and Loaded Dice 20 → a critical "24"; a bandit at 25 m isn't targeted; one behind a wall isn't; the nearest of two is; hand-fired projectiles pass through the player and hit a bandit behind (7 × Damage 2 = 14), stop at a wall, expire in open air; the pool reuses projectiles (created = prewarm); a safe zone stops it; its kill is the player's (kills, combo 1, XP orb, health orb at chance 1, cash, Leech +1, a Powder Keg blast at 100%); paused: no shots; 24 camera views (shoulder ×2, hip / ADS, pitch −70 to +50): the gun at least 0.9 × the clear radius from the crosshair (logged per view); dead: no shots; a new run → level 0, no projectiles, no gun. Health orbs off except where noted. Game time ×4. |
| `Upgrades.FloatingGunLevels` | DA_FloatingGunSettings +10% damage and fire rate per level; guns per level 0–6: 0, 1, 2, 2, 2, 2, 2; multipliers x1 at level 2, x1.1 at 3, x1.3 at 5 (added up), 1.95 shots/s at 5; DA_Upgrade_FloatingGun has 3 card texts: "Adds a floating gun…", "Adds a second floating gun…", "+10% floating gun damage and fire rate" (the number matches the settings, damage and fire rate bonus equal), 9 owned repeats the last; the real WBP_UpgradeCard shows the text for 0–3 owned; level 1: one gun; level 2: two guns, the first still right, the second left, mirrored at the same height above the head, no collision, no bonus; one bandit in range: both shoot it; a second bandit: different targets, each takes one gun's damage (about 12 a second over 4 s); one dies: both go to the other; the level 5 pick mid-burst: every frame of both guns on schedule before and after, the schedule switches to the level 5 interval; level 5: still two guns (and two gun actors), 1.95 shots/s and x1.3 each, every hit 10.4, about 40.6 a second from both; logs one gun's DPS next to the rifle's; a new run → level 0, no guns, no gun actors left. Health orbs off. Game time ×4. |
| `Upgrades.Offers` | DA_UpgradePool on the GameMode (21 upgrades since Prompt 52; the 20 other than Second Wind come up evenly, weights Stat Boost / Weapon 100/20/6/1, Ability 100/10/3/0.5, Legendary ability chance 0.8%, 3 choices, 3 rerolls), 4 rarities; per-card roll: 30,000 offers all 3 different, Legendary ability cards at ~0.8% and always Legendary, the 14 others even, each category's rarity split, no Rare whole-unit card; chance 1 / 0 for the Legendary ability; Rare-only weights → Rare, but Common for whole units; per-category weights; AppearanceWeight 0 never offered; same seed same offers (with rarities), 2 upgrades → offer of 2, MaxStacks excluded, empty pool → none; a pick gets the card's rolled rarity and value; level-up opens an offer after the MaxHealth reward and heal; double level-up queues 2; 3 rerolls (events 2/1/0, Rerolled) then refused; bad index refused; picks grant a stack, Closed + Opened, last pick only Closed; new run closes the offer, refills rerolls, drops picks; reroll with no offer refused; same run seed → same first offer (with rarities); simulator deterministic, no short offers, Epic and Legendary cards, every card in a category, no Rare Legendary ability, run stream untouched. |
| `Upgrades.PickValues` | DA_UpgradePool multipliers 1 / 1.5 / 2 / 3 and whole-unit values 1 / – / 2 / 3; all 21 DA_Upgrade_* (Razor Cards since Prompt 46, Barbed Wire since 47, "+10% Barbed Wire damage, +5% size", Dynamite since 49, "+10% Dynamite damage, +5% blast size", Lucky Coin since 50, "+10% Lucky Coin damage", Molotov since 51, "+10% Molotov burn damage, +5% patch size", Tomahawk since 52, "+10% Tomahawk damage"): category, Legendary ability, whole units, BaseValue (= the old per-pick value), AppearanceWeight 1, percent or not, in the pool; Floating Gun WeaponGivingPicks 2; card number and FormatValue; ParseRarity; for every stat boost and ability one pick of each rarity: the attribute = base × (1 + total) or base + total after each, a Common pick = exactly the old pick, whole units refuse Rare, picks and total tracked, one active effect; 6 shields, JumpMaxCount 7, Powder Keg 7.5 stacks = 22.5%, crit 37.5%; `gs.Upgrade.Grant Damage 2 Epic` / `Damage Legendary`, ExtraJump Rare refused; Floating Gun Rare → Legendary → Common → Epic: power 0.5 / 2.5 / 3.5 / 5.5, 4 picks, two guns, ×1.55; Second Wind Common / Legendary / Rare = 3 charges, a killing blow uses one (2 picks, total 2, one effect); a new run removes all. |
| `Upgrades.RazorCards` | DA_RazorCardsSettings (6 damage, 2 a second, 25 m, 30 m/s, homing, +10% per power, GE_Damage, a thin card) and DA_Upgrade_RazorCards (Weapon, first pick gives it, GE class, in the pool, card text, the +10% matches the settings); a hand-thrown homing card turns toward its bandit, and when the bandit dies it loses its target and flies on straight; granted: level 1, power 0, 0.5 s, ×1; no cards with no enemy, with one 27 m away (out of range) or behind a wall; a bandit in range: about 4 cards in 2 s, every frame on the exact schedule, every card a hit for 6; power 1: on schedule at 1/2.2 s, 6.6 per card; power 3 (an Epic pick): 2.6 a second, 7.8 per card; a bandit running past (17 m in 3 s, 4 m to the side): 8 of 8 cards hit, none hit the world or expired; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: no cards; a new run: level 0, no cards in flight, none thrown at a bandit in range. Game time ×4. |
| `Upgrades.BarbedWire` | DA_BarbedWireSettings (3.5 m, 5 per tick, every 0.5 s, +10% damage / +5% radius per power, GE_Damage, ring segments, a hit effect) and DA_Upgrade_BarbedWire (Weapon, first pick gives it, GE class, in the pool, card texts, "+15% Barbed Wire damage, +7.5% size" for a Rare pick, both numbers match the settings); not owned: no ring; granted: level 1, power 0, radius 350, 5 per tick; the ring is shown with one instance per segment at the radius, at the feet, no collision; four tough frozen bandits at 2 m, 3.3 m (inside), 3.8 m and 1.5 m but 3 m up (outside); 4 ticks on the exact schedule every frame, two hit per tick, 5 each, the outside ones untouched; power 1: radius 367.5 (3.8 m still outside), 5.5 per tick, the ring redrawn; power 3 (an Epic pick): radius 402.5, 6.5 per tick, the 3.8 m bandit now inside (three per tick), the one above still untouched; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: no ticks; a new run: level 0, no ring, no ticks, a bandit next to the player untouched. Game time ×4. |
| `Upgrades.Dynamite` | DA_DynamiteSettings (40 damage, 3 m blast, every 4 s, 20 m, 0.6 s flight, +10% damage / +5% radius per power, GE_Damage, stick mesh and material, an explosion) and DA_Upgrade_Dynamite (Weapon, first pick gives it, GE class, in the pool, card texts, "+15% Dynamite damage, +7.5% blast size" for a Rare pick, both numbers match the settings); granted: level 1, power 0, radius 300, 40 per blast; one bandit 21.5 m away: no throw in 9 s; moved into range: one throw at once (nothing banked); a lone bandit at 5 m and a group of three at 14 m: it picks the group's nearest member (two neighbours each, none for the lone one); a tie goes to the nearest; one tough bandit: 40 per blast (logs 10 DPS), a group of five: all five hit every blast (logs 50 DPS); T 2 m ahead, A 2.5 m from T, B 3.4 m from T, the player 2 m from T with god mode off: the blast centered on T, the stick at T's feet, T and A take 40 per blast, B nothing, the player nothing, one damage number per hit, the damage counted as Dynamite in run stats, every frame on the exact 4 s schedule; power 1: x1.1, 315 cm, T 44, B still outside; power 3 (an Epic pick): 52, 345 cm, B hit; Loaded Dice x20: T takes 104 and a critical number for each of the three hit; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused with a stick in the air: the stick doesn't move, no throw or blast; safe zone: no throws in 9 s, then one at once; a new run with a stick in the air: no stick left, it never explodes, level 0, no throws, a bandit in range untouched; dead: no throws in 9 s. Game time x4. |
| `Upgrades.LuckyCoin` | DA_LuckyCoinSettings (10 damage, every 1.5 s, 20 m, 8 m bounce range, 3 bounces, +1 bounce per 2 full power: 3 3 3 4 4 5 at power 0 / 1 / 1.9 / 2 / 3.5 / 4, +10% damage per power, GE_Damage, coin mesh and material) and DA_Upgrade_LuckyCoin (Weapon, first pick gives it, GE class, in the pool, card texts, "+10% Lucky Coin damage", the number matches the settings); granted: level 1, 3 bounces, 10 per hit; no enemy: no flips in 2 s, then one at once when a bandit appears (nothing banked); one bandit: one hit a flip (logs 6.7 DPS); five tough bandits in a row 4 m apart: every frame on the exact 1.5 s schedule, a flip hits the first four in order, each once (10 a flip each), the fifth never (out of bounces), the damage counted as Lucky Coin in run stats (logs 26.7 DPS); power 1: 11 per hit, still four hit; power 2: 4 bounces, all five hit, 12 each; power 3.5 (a Rare pick): 13.5 per hit, still 4 bounces; two bandits 12 m apart: the flip stops after the first, the far one never hit; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: no flips; a new run with a coin in the air: no projectile left, level 0, no flips, a bandit in range untouched. Game time x4. |
| `Upgrades.Molotov` | DA_MolotovSettings (every 5 s, 15 m, 0.6 s flight, 4 per burn every 0.5 s for 4 s, 2.5 m patch, +10% damage / +5% radius per power, GE_Damage, bottle and patch meshes and materials) and DA_Upgrade_Molotov (Weapon, first pick gives it, GE class, in the pool, card texts, "+15% Molotov burn damage, +7.5% patch size" for a Rare pick, both numbers match the settings); a bandit 16 m away: no target, no throw; moved to 8 m: a throw at once (nothing banked), the patch at its feet, no collision, drawn at its radius; T 1.2 m in front of the player (in the fire, god mode off), A 1.5 m and B 2.4 m from T, C 2.7 m, D 3 m above: two patches, every frame on the exact 5 s schedule, 8 burns per patch, T / A / B take 32 a patch, C and D nothing, the player nothing, the damage counted as Molotov in run stats; power 1: 4.4 per burn, 262.5 cm, C still outside; power 3 (an Epic pick): 5.2, 287.5 cm, C burns, D still doesn't; paused: the patch doesn't age, no burns, no damage; safe zone: no throws, no burns; one bandit and five bandits in a patch at power 3 (logs DPS); its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); a new run with a patch burning: no patch or bottle left, level 0, no throws or burns, a bandit in range untouched. Game time x4. |
| `Upgrades.Tomahawk` | DA_TomahawkSettings (12 damage, every 3 s, 18 m range, 18 m out, 1 tomahawk, +1 per 3 full power: 1 1 2 2 3 at power 0 / 2.9 / 3 / 5.9 / 6, +10% damage per power, GE_Damage, mesh and material) and DA_Upgrade_Tomahawk (Weapon, first pick gives it, GE class, in the pool, card texts, "+10% Tomahawk damage", the number matches the settings); granted: level 1, one tomahawk, 12 per hit; no enemy: no throws, then one at once when a bandit appears (nothing banked); one bandit: 24 a throw, two hits (logs 8 DPS); a line of five 3 m apart and one 2.5 m beside it: every frame on the exact 3 s schedule, thrown at the nearest, each in the line hit once out and once back (24 a throw), the one beside untouched, ten hits a throw, the damage counted as Tomahawk in run stats (logs 40 DPS); a wall across the lane 1.5 m past the second: the two in front hit out and back, the three behind untouched; power 1: 13.2 per hit, still one tomahawk, the far bandit hit again once the wall is gone; power 3 (an Epic pick): 15.6 per hit, two tomahawks at the nearest and the next nearest, 20 hits a throw; its kill is the player's (run-stats kill, combo, XP orb, health orb, cash, Leech); paused: the tomahawk in the air doesn't move, no throws; a new run with a tomahawk in the air: none left, level 0, no throws, a bandit in range untouched. Game time x4. |
| `Upgrades.WeaponSlots` | MaxWeapons 3; the real first offer has a weapon card (any weapon); the limit step uses a pool copy without Razor Cards, Barbed Wire, Dynamite, Lucky Coin, Molotov and Tomahawk; test pool with two test weapons, MaxWeapons 1: none owned → both offered, one owned → the other never offered (CanOfferWeapon false) and the owned one still offered, MaxWeapons 2 → offered again; the first offer's weapon card in 400 of 400 offers, 3 different cards, at all three positions, Common and Rare both rolled; slots full → it is the owned weapon; MaxWeapons 0 → no weapon card, 3 cards; card labels NEW WEAPON / WEAPON UPGRADE / STAT BOOST; real run: first offer has the Floating Gun, 3 rerolls keep a weapon card, after the first pick not pending and in 200 later offers the share without a weapon card is within 10 points of the share expected from the weapons that can be offered at each offer (since Prompt 49, per offer since 52; it was "more than 5 of 20"), a new run → pending; a copy of the pool with a test weapon and MaxWeapons 1 (`SetUpgradePoolOverride`): one HUD slot, 30 offers + rerolls never offer the test weapon but offer the Floating Gun, the first one includes it; MaxWeapons 3: the test weapon is offered and picked, slots Floating Gun / test weapon / empty, no weapon in the upgrade row; real pool HUD: 3 empty slots without an icon, Damage in the upgrade row, two Epic Floating Gun picks → slot 1 shows it (icon, no count), the others empty, no Floating Gun in the upgrade row; a new run empties the slots; death → summary weapons line "Floating Gun", upgrades "Damage x1", one upgrade icon. |
| `Upgrades.MoveSpeedSlideDash` | Base Move Speed: ratio exactly 1, the scaled settings getters equal the plain values at 1; half Move Speed → ratio 1 (never below), ×2 → 2; a Move Speed pick raises it, a new run resets it; on a clear 34 m lane of the ring road, a slide from a full run and a dash at ×1 and ×2: each keeps its start ratio; slide start = min(run + SpeedBoost × r, MaxSpeed × r) and faster than running, end = EndSpeed × r; dash = Distance × r in its Duration (±0.02 / +0.06 s), exit = min(ExitSpeed, dash speed) × r = the running speed; ×1 = the old numbers exactly; ×2 = twice the distance in the same time (within 0.05 s), the slide ends at twice EndSpeed; ×33: a dash and a slide into the east boundary wall stop inside. |
| `Upgrades.MoveSpeedCover` | A crate, a low wall, a shack, a tower and the silo, each with a clear 6 m run-up; at Move Speed ×1, ×5 and ×33, a dash and a slide from a full run into each, the player's location sampled every frame in the piece's own frame: no frame overlaps the piece, the player stays on the ground, and if it ends behind the piece it went round it. Logs each path. Game time ×3. |
| `Checkpoint.WeaponPicksAndNewRun` | Weapon power after picks of each rarity for every weapon (Dynamite since Prompt 49, Lucky Coin since 50, Molotov since 51, Tomahawk since 52; e.g. Barbed Wire at power 6.5: ×1.65 damage, radius 463.75); a new run starts with 1 dash charge; every upgrade picked at every rarity it has (63 picks across 17 upgrades), a reroll used, Damage / air jumps / MaxHealth raised, two guns, the ring, Razor Cards and shields present; a new run: no picks, no offer, rerolls refilled, no weapons and the first offer's weapon card pending again, level 1, 1 dash charge, no shields / guns / ring / cards / Dynamite / Lucky Coin / Molotov / Tomahawk, nothing thrown left in play, no combo, no Powder Keg blasts, no damage dealt, HUD weapon slots empty and no upgrade icons. |
| `Checkpoint.WeaponLimitAllThree` | Real pool (since Prompt 49): three real weapons owned, 60 offers and their rerolls never offer an unowned real weapon, the owned ones still offered. Then all three M2c weapons owned, plus a fourth test weapon in a pool copy: HUD slots in pick order (Barbed Wire, Razor Cards, Floating Gun), no weapon in the upgrade row; slots full: `CanOfferWeapon` refuses the fourth; 60 offers and their rerolls never offer it, and owned weapons are still offered (31 times in the run); still three weapons; with MaxWeapons 4 the fourth is offered; the auto-pick policy (owned weapon with full slots, new weapon with a free slot, reroll when all Common, first without rerolls, else the rarest). |
| `Upgrades.PowderKegChainEnds` | 34 stacks (102%), 8 frozen 10 HP bandits in a line 2.5 m apart. One kill starts a chain that runs down the line and ends: all 8 die, exactly 8 blasts, nothing pending. The player (no god mode, 6 m from the first) is never hurt. Run stats record the Powder Keg damage. Game time ×4. |
| `Economy.HealthOrbsAllSources` | For each kind of player kill (own kill, Floating Gun, Razor Cards, Barbed Wire, a Powder Keg blast kill) in a fresh run: at drop chance 1 every kill drops an orb, at 0 none, and the damage is recorded under that source in run stats. Chance 0.3 in bulk: 60 own kills and a crowd killed by one Barbed Wire tick (5 Legendary picks) drop at about the same rate (23 of 60 and 16 of 50 in the run). Game time ×4. |

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
- **XP** (DA_XPCurve, since Prompt 45; no level cap): `BaseXP` 100 for level 2, then `XPIncreasePerLevel` +30 more
  for every level after that (XP from the previous level = 100 + 30 × (level − 2)):

  | Level | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 25 | 50 | 100 |
  |---|---|---|---|---|---|---|---|---|---|---|---|---|
  | XP from the previous level | 100 | 130 | 160 | 190 | 220 | 250 | 280 | 310 | 340 | 790 | 1,540 | 3,070 |
  | Total from level 1 | 100 | 230 | 390 | 580 | 800 | 1,050 | 1,330 | 1,640 | 1,980 | 10,680 | 40,180 | 155,430 |

  - The old curve (+25% per level, cap 50) needed 596 for level 10, ~16,900 for level 25 and 4.48 M for level 50.
  - XP per enemy (orb value): MeleeBandit 10, RangedBandit 15. DT_Waves rounds 1–5 drop 40 / 65 / 90 / 115 / 150 XP;
    generated waves about 8.3 XP per budget point (round 10: 19 enemies, 245 XP; round 20: 36–37 enemies, 455–460 XP).
  - **Levels per cleared round** (every enemy killed and every orb collected, XPGain 1, clearing every round from 1):
    round 1 0.4 (40 of the 100 for level 2), round 5 ~0.9 (150 XP at level 3–4, which needs 160–190), round 10 ~0.8
    (245 XP at level 7–8, 280–310), round 20 ~0.85 (460 XP at level 16, 550). Because both the XP per round and the XP
    per level grow linearly, a cleared round stays worth a little under one level all the way up.
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
  second at Damage 1, so a round-1 melee bandit (60 HP) takes 8 hits, about 5 s.
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
- **Upgrade offers** (DA_UpgradePool, 17 upgrades since Prompt 47, three of them weapons; placeholders): per card a 0.8% Legendary ability
  chance, otherwise an upgrade by AppearanceWeight (all 1) and a rarity from its category's weights: Stat Boosts and
  Weapons 100 / 20 / 6 / 1, Abilities 100 / 10 / 3 / 0.5 (whole units: Rare → Common). 3 choices, 3 rerolls per run.
  Pick values ×1 / ×1.5 / ×2 / ×3, whole units +1 / +2 / +3.
  - `simulate_upgrade_offers(10000, 1)` (Prompt 46): each of the 15 non-Legendary upgrades on 6.3–6.9% of cards
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
  that its cards home in: they hit a running bandit every time (8 of 8 in the test), where the gun's bullets lead the
  target once and can miss a turning one.
- **Barbed Wire** (DA_BarbedWireSettings): a ring of radius 3.5 m on the ground around the player (enemy centers within
  the radius flat and within 2 m up or down); every 0.5 s (exact schedule) 5 damage × the player's Damage to every enemy
  inside, each with its own crit roll. Each point of power: +10% damage, +5% radius (power 3: 6.5 per tick, 4.025 m;
  power 10: 10 per tick, 5.25 m). That is 10 DPS per enemy inside at power 0 (no crits), with no cap on how many: 4
  bandits inside = 40 DPS. Look: 48 segments of the engine cube, 4 cm wire, 3 cm above the floor, MI_BarbedWire (grey);
  RadialBurst ×0.06 on each enemy hit.
- **Dynamite** (DA_DynamiteSettings): every 4 s (exact schedule) a stick lobbed at the enemy within 20 m with the most
  other enemies within the blast radius of it (nearest on a tie; no clear line needed); it lands at that enemy's feet
  0.6 s later (arc 3 m high) and blasts every enemy within 3 m of where it stood for 40 × the player's Damage, each with
  its own crit roll. Each point of power: +10% damage, +5% blast radius (power 3: 52 per enemy, 3.45 m). Look: the engine
  cylinder (0.08, 0.08, 0.3) with MI_Dynamite (red), tumbling 720°/s, SimpleExplosion scaled to the radius.
- **Weapons at power 0, Damage 1, no crits:**

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
- The M2c upgrade rework is complete (Prompts 41–48), and so are the last four weapons (49–52). The weapons checkpoint
  (53) is next.
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
  - The patch lands where the target stood when the bottle was thrown; a walking bandit can be out of it by then, and
    bandits walk through it rather than avoiding it (no AI reaction to fire).
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
  - The stick lands where the target stood when it was thrown. A bandit walking at full speed covers about 3 m in the
    0.6 s flight, so against a running crowd the blast often hits the bandits around the spot rather than the target.
  - The blast has no line-of-sight check (like Powder Keg): enemies behind a wall inside the radius are hit.
  - `Upgrades.Dynamite` is the slowest test (about 23 s): the 4 s interval at ×4 and several waits longer than an
    interval.
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
  - "Inside" is the enemy's center within the radius (flat), so a bandit whose body overlaps the wire but whose center
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
  - The turning circle is ~2.4 m (30 m/s at 720°/s): a card thrown at a bandit that is right next to the player and
    moving sideways can circle before it hits. Not seen in the tests.
  - The first pick's `[upgrade]` log line reads "+0% Razor Cards damage and fire rate" (a Common first pick adds no
    power; the card hides that line). The Floating Gun has the same.
  - It targets by distance only, like the Floating Gun, and isn't replicated (`TODO(co-op)`).
- Floating Gun (Prompts 38–39):
  - The guns split targets by "not the other gun's", not by side: with two bandits the right gun may shoot the one on
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
53. **Weapons checkpoint.**

**Then M3** (after the weapons checkpoint; not planned in detail yet; each item needs its own prompt). From the decisions so far:
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
