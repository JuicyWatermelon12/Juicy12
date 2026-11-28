# Drift Tycoon (prototype)

An idle tycoon about a drift team, played in portrait on a phone. It is a separate project from BanditBaller and
only shares this repository for now; the folder can be moved out as it is.

You watch your cars drift a parking-lot course from a fixed, tilted 3D view. Style points from the cars turn into
cash, and cash buys parts, drivers, mechanics and venue upgrades, all of which show up in the view.

## Play

- Open `index.html` in a browser (it needs internet once, for Three.js and the fonts), or
- `node tools/build.js` writes `dist/drift-tycoon.html`, one file with the game's own code inlined.

Progress saves in the browser (`localStorage`). The menu button (top right) has "Reset all progress", the Drift
steering setting, and testing tools (they can be hidden from the menu): add $500T, max street cred, max the current
venue, fill all pits with the best car and a driver each, unlock all venues, a free Legendary tryout, skip an hour
of away earnings, and start a Drift run at 5 km.

## What is built (first slice)

- **Cars:** eight fictional models (Kumo 180 → Kaiju X). Each pit's car has six parts: engine, turbo, tires,
  suspension, stance, angle kit. A part has big stages (Stock → Street → Sport → Race → Pro → Works →
  Prototype → Unlimited → Legend; one more goes on sale at each venue), each 50 times the price of the last, so upgrades are occasional and each one shows: stance lowers the car, the angle
  kit widens its drift angle, tires add smoke, engine and turbo add wings, an intercooler and exhaust flames.
  Trading a car up to the next model keeps its parts and driver. In Drift mode, stages past Works change
  handling by only 0.4 of a stage each (`HANDLING_EXTRA`), so the latest builds stay drivable; they still add
  their full style at the venue and their score bonus.
- **Drivers:** a small team, one driver per pit plus one reserve. Each has a rarity (Common, Rare, Epic,
  Legendary), three stats and sometimes a trait (Showboat, Safe Hands, Tandem Ace, …). Skill and flair raise
  style; control lowers the crash rate.
  - **Seat time:** drivers level up while their car drifts, and each level grows their stats. Rarity sets how far
    they can go (level 10, 15, 20 or 25).
  - **Tryouts:** hiring is a tryout day: pay, three prospects show up, sign one or send them home. There is a
    three-minute wait between tryouts (none while a car has nobody to drive it).
  - **Mentoring:** with a full team, a signing replaces a driver. They retire and pass half their experience to
    the newcomer, who takes their seat.
  - **Letting a driver go:** the × on a driver's tile in the Team tab, then one confirmation. The driver leaves for
    good and passes no experience on. If they had a seat, the best reserve driver takes it, otherwise the car stands
    empty. The last driver can't be let go, so the team is never empty.
  - **Rarity at a glance:** wherever a driver is picked, and on each car in the Garage, a small square in the
    rarity colour and the driver's level sit beside the name.
- **Crashes and mechanics:** a drifting car can spin out. A free mechanic runs to it, repairs it and it rejoins
  when the track around it is clear. More mechanics repair more cars at once; training shortens repairs.
- **Three limits to upgrade:** pits (how many cars run), the cars themselves (style a second), and the crowd
  (how much style it can cheer for, which caps income). The venue's upgrade lines raise the crowd, cash and style.
- **Venue upgrades** (Venue tab): seven lines, 50 levels at every venue, and every level shows on the venue.

  | Line | Levels | Does | You see |
  |---|---|---|---|
  | Grandstands | 10 | crowd x1.41 a level | a stand module each level, then roofs on half, then all |
  | Sponsors | 10 | cash x1.09 a level | far-fence banners, the start arch, near-fence banners, flags, a turning sponsor pylon, the blimp, a hot-air balloon |
  | Track prep | 10 | style x1.054 a level | tire walls, light towers, kerbs, the BIG ANGLE board, more towers, barriers, marshal posts, catch fencing, apex flags, a race control tower |
  | Lighting | 5 | crowd x1.5 a level | lamp posts, string lights over the pit lane, floodlight towers, glowing kerbs, a sweeping light show |
  | Track surface | 5 | style x1.08 a level | fresh tarmac, painted run-off, drift-line markings, logos painted on the track, glowing clipping points |
  | Fan zone | 5 | cash x1.1 a level | food stalls, merch tents, a big LIVE screen, a stage, fireworks |
  | Media | 5 | tap bubbles come 8% sooner and pay 16% more a level | a camera tower, a second one and a cameraman, the commentary box, a drone over the leading car, a TV helicopter |

  A fully built venue earns what it did with the old three lines (crowd x243, cash x3.8, style x2.5). Prices are
  set so purchases spread through a venue's life: in the simulator about half are bought by a quarter to a half
  of the time there, and the last at 60 to 85%. **The next venue goes on sale only once every line at the best
  venue owned is at its top level** (the Venue tab shows how far along it is). Saves from before keep their
  levels, so a venue that was maxed is now part built.

  **Unique top pieces:** at every venue three lines have a top level of their own instead of the shared one
  (Grandstands keep their roofs and add it):

  | Venue | Unique top pieces |
  |---|---|
  | Parking Lot | waving tube men (Sponsors), a ferris wheel by the road (Fan zone), a TV news van with a turning dish (Media) |
  | Docks | a giant crane swinging a sponsor banner (Sponsors), a container-stack grandstand full of fans (Grandstands), searchlights sweeping from the quayside (Lighting) |
  | Airfield | a plane towing a sponsor banner (Sponsors), a stunt biplane looping and trailing smoke (Media), chasing runway approach lights (Lighting) |
  | Pro Circuit | a sponsor bridge across the track (Sponsors), the winners' podium spraying champagne (Track prep), a cable cam following the leading car (Media) |
  | Stadium | a giant fans' flag across the far stands (Grandstands), strobes flashing along the stands (Lighting), confetti cannons (Fan zone) |
  | Neon City | rooftop lasers (Lighting), a turning hologram billboard (Sponsors), a news helicopter whose spotlight follows the leader (Media) |
  | Aircraft Carrier | a jet formation flying over (Media), landing lights chasing down the angled deck (Lighting), signal flags strung from the island (Fan zone) |
- **Away earnings:** up to four hours at the team's long-run rate, crashes included.
- **Drift mode** (the Drift button; the code calls it "touge"): the main way to earn. Drive one of your own cars.
  The car drives itself; you slide one finger left and right to steer (arrow keys or A / D on a keyboard). The
  car's direction of travel lags behind its nose, so it slides through every bend. The menu has two kinds of map:
  - **Street:** street maps (`STREET_MAPS` in `src/data.js`). One so far, the Mountain Pass: an endless mountain
    road at dusk, generated as you go.
  - **Track:** the drift track of any venue you own, at full size (`TRACK_SCALE`, 5.5 times the venue view), flat
    and in daylight, with the venue's own scenery. It is lapped for ever; laps are counted at the start line.
    Laps are 1.07 km (Parking Lot), 1.22 km (Docks), 0.85 km (Airfield), 0.94 km (Pro Circuit), 1.12 km
    (Stadium), 0.96 km (Neon City, at night with headlights) and 0.83 km (Aircraft Carrier).
  Scoring, payout, street cred and the rules below are the same on both. Best score and distance are kept per map.
  - **Score:** speed x drift angle x a multiplier that builds while the drift is unbroken and resets after a
    straight or a barrier touch. The multiplier's cap starts at x5 and rises by 1 every kilometre.
  - **The road:** a street is different every run. Bends tighten, straights shorten and top speed rises 5% a
    kilometre over the first six kilometres, then it keeps getting harder (tighter still, up to +50% speed) until
    10 km, where it holds. A track keeps its layout, so it is made harder with speed: 7% more top speed a
    kilometre up to 10 km, and the Airfield track starts 15% faster (`TRACK_SPEED`). The test drivers get about
    three quarters as far on a track as on the street, which leaves room for learning the layout.
  - **The end:** touching a barrier ends the run (`MAX_HITS` in `src/data.js`, now 1). "End run" banks it early. Either way you keep the score.
  - **Payout: the money is far down the road.** Points pay in seconds of the team's income, and each
    kilometre's points pay at that kilometre's rate (`PAY_PER_KM`): x1 in the first kilometre, then x5, x11, x15,
    x21, x23, x25, x27, x29, and x31 from the tenth kilometre on (at x1, 330 points pay one second). Against the
    flat pay runs had before (60 points a second, +20% per clean kilometre), a run that ends at 1 km pays about
    15%, 2 km about half, 3 km the same, 5 km about twice and 8 km about three times as much. The run's cash and
    the current rate show under "End run", each kilometre marker announces the new rate, and the result card
    shows what each kilometre paid. The driver earns double seat time.
  - **Steering:** a slide of about a quarter of the screen width (`STEER_DRAG`, up to 520 px) gives full lock, and
    the wheel follows the finger in about an eighth of a second (`STEER_RESPONSE`). Menu → Drift steering sets the
    sensitivity from 60% (smoother, longer slides) to 150% (quicker); it is saved and survives a progress reset.
  - **Headlights:** on the street and on night tracks each headlight throws its own beam on the road. Light on
    the ground (beams, underglow) is drawn from vertex colours, not canvas textures: some browsers add noise to
    canvas pixels, which showed as coloured spots in the light.
  - **Your build matters:** engine and turbo raise top speed, the angle kit adds steering lock and drift angle,
    suspension keeps more speed after a touch, stance and tires raise the score, and so do the driver's skill and
    flair. Steering scales with speed, so fast cars can still make the bends.

- **Street cred:** a rank earned only in Drift mode (Street or Track): 1 cred per 100 m, 1 per 1,000 points, and 10 for each
  record a run sets (best score, best distance on that map). Twenty ranks, Unknown to The GOAT: the first twelve
  (to Myth, 3,600 cred) come during the hook venues, and the eight late ones (Icon 5,500 to The GOAT 46,000) run
  from about the end of the Pro Circuit to late on the Aircraft Carrier. It never goes down, is never spent, and
  gives no bonuses; it puts cosmetics on sale.
- **Cosmetics** (Garage → a car → Customise): paint, rims, body kits and underglow, bought with cash at set
  prices that never change. Paid items come in **tiers** (`COSMETIC_TIERS` in `data.js`): every item in a tier
  costs the same, goes on sale at the same street cred rank (a rank puts the whole tier on sale) and gives the same
  style bonus at the venue. In each category a car gets the bonus of the **best tier it owns, whatever it is
  wearing**, so the look is free to choose; the four categories add up. A car with a top-tier item in every
  category has +166% style.

  | Category | Tiers (bonus, rank, price each) |
  |---|---|
  | Paint | Street +6% (rank 1, $25K): white, black, gunmetal, forest, midnight, burnt orange · Custom +18% (rank 6, $20M): purple, teal, gold, magenta, silver, blood red · Show +27% (rank 13, $2T): Matte Black, Pearl Frost, Mirror Chrome · Legend +36% (rank 17, $10Q): Violet Shift, Gold Chrome, Inferno Shift |
  | Rims | Sport +8% (rank 2, $100K): five-spoke, six-spoke, bronze mesh, deep dish · Pro +20% (rank 7, $15M): gold five-spoke, neon six-spoke · Show +30% (rank 13, $3T): carbon, chrome dish, gold mesh · Legend +38% (rank 17, $10Q): neon pink twin-five, forged titanium |
  | Body kits | Street +12% (rank 3, $150K × the model's kitScale): Street, Drift · Pro +35% (rank 10, $100M ×): Pro, Widebody · Top +70% (rank 17, $100B ×): Time Attack, Kaido Racer |
  | Underglow | Underglow +12% (rank 12, $400B): cyan, pink, violet · Show +22% (rank 16, $1Q): gold, rainbow |

  - The late paints have finishes (matte, pearl, chrome) and two shift between colours as the car moves.
  - Widebody has bolt-on overfenders, a deep splitter, finned skirts, a ducktail and a tall wing; Time Attack a huge
    two-element wing on tall end plates, canards, a vented bonnet and a finned diffuser; Kaido Racer a long chin
    spoiler, boxy flares, pipes up past the roof and an upright wing.
  - Underglow is a soft light under the car; the rainbow cycles through every colour.
  - Saves from before the tiers keep everything each car owned; its bonus becomes that of the best tiers among them.
  They change nothing about how the car drives or scores in Drift mode. **Everything is bought per car:** each
  car owns only what was bought for it, so another car has to buy its own. Paint, rims and underglow stay on the
  car when it is traded up; a body kit is made for one model (its price depends on the model), so a trade-up
  needs a new kit. Saves from when cosmetics belonged to the team give every car what the team owned. The kit decides the car's wing,
  lip, skirts, arches and intercooler; parts no longer add those (stance height, smoke and exhaust flames still
  come from parts).

- **Venues:** a ladder: Parking Lot → Docks → Airfield → Pro Circuit → Stadium → Neon City → Aircraft Carrier
  (more to come). Every venue bought is kept. The team
  drifts for the crowd at one venue at a time and can move between owned venues freely (Venue tab → Your venues).
  Cars, parts, drivers, mechanics and cosmetics go with the team; each venue keeps its own grandstands, sponsors
  and track prep, which start from zero at a new venue. What is on sale (pits, cars, part stages) follows the best
  venue owned, wherever the team is. Each venue has its own drift line (`src/tracks.js`: straights and arcs, with
  hairpins, kinks and esses) and scenery, and its track can be driven in Drift mode. A venue sets:
  - how many pits it has (3, 4, 5, then 6 from the Pro Circuit on) and which car it puts on sale (Kaze Tourer
    at the Docks, Rotor Seven at the Airfield, Skyward R at the Pro Circuit, Shogun GT at the Stadium, Raijin RS
    in Neon City, Kaiju X on the Aircraft Carrier);
  - the highest part stage on sale (Sport, Race, Pro, Works, Prototype, Unlimited, Legend), so parts last the
    whole ladder;
  - a style multiplier (x4 per venue, up to x4096), a bigger crowd, and dearer venue upgrades;
  - the price scale of its upgrades (`scale`).
  - **a distance bar** (`bar`): the average Drift run that keeps the intended pace there. It grows with every
    venue, because the player is expected to get better: 2 km at the Parking Lot, 3 km at the Docks, 4 km at the
    Airfield (the first three are the hook, on a quicker curve), then 1 km more per venue: 5 km at the Pro
    Circuit up to 8 km on the Aircraft Carrier. Each venue's cash rate (`cash`: x1.7, x1.15, x1.3, x1.1, x0.75,
    x0.78, x0.45) sets its pace; a player below the bar still progresses, only slower.
  The Docks cost $40M, the Airfield $6B, the Pro Circuit $700B, the Stadium $60T, Neon City $2Q and the Carrier
  $80Q. For a player at each bar (from moving in to moving on): the Parking Lot about 15 to 20 minutes, the Docks
  35 to 45, the Airfield about an hour, the Pro Circuit about 2¼ hours, the Stadium about 3¾, Neon City about 6½,
  and the Aircraft Carrier about 10 hours to build out: about 25 hours in all (with every venue built out before
  the next one). Each venue after the hook takes
  about 1.75 times the one before.
  - **Scenery:** the Pro Circuit has pit garages and race control under a main grandstand, a timing tower, and
    the circuit running past with kerbs, gravel and tire walls. The Stadium sits on a football pitch (its lines
    still show) inside a full bowl of stands with floodlights and a big screen. Neon City is the first venue at
    night: towers with lit windows, neon signs, wet streets and street lamps, under dim cool light. The Aircraft
    Carrier is a flight deck on a grey hull out at sea, with the island tower, parked jets, nets and an escort
    ship. Each has its own Track-mode surroundings too.

- **Tap bubbles:** while you watch the venue a bubble floats up every 30 to 60 seconds and goes after 10 seconds
  if ignored. None appear while you are away or in Drift mode.
  - **Perfect drift** (most common, over a car): pays 20 seconds of team income at once.
  - **Crowd wave** (over the stands): for 30 seconds the crowd can cheer twice as much style and cash is x1.25.
  - **Drift meet** (rare): for 60 seconds the crowd can cheer twice as much, cash is doubled and the cars close up
    into a tandem train; afterwards they ease apart again.
  Effects show as timers under the money bar. They lift only what the venue earns while you watch: away earnings
  and Drift mode payouts ignore them.

Planned next: more venues. Not built yet: sound.

## Layout

```
index.html        page shell; loads Three.js r128 from cdnjs and the files below
src/data.js       every tuning number (parts, cars, rarities, costs, crash and repair times)
src/game.js       rules: income, costs, purchases, drivers, save and load. No DOM, no Three.js
src/tracks.js     venue track layouts, drawn from straights and arcs. No DOM, no Three.js
src/tougecore.js  Drift mode rules: the endless street, lapped tracks, handling, scoring, payout. No DOM, no Three.js
src/touge.js      Drift mode in 3D: the street built in chunks ahead of the car, a venue track built once,
                  chase camera, steering, HUD
src/world.js      the 3D venue: track, cars, smoke, tire marks, crashes, mechanics, crowd, camera
src/ui.js         money bar, tabs, upgrade sheets, pop-ups
src/main.js       boot, save, frame loop
tests/            node tests for the rules
tools/build.js    packs everything into dist/drift-tycoon.html
tools/sim.js      pacing check: a simple player plays for an hour and counts how often they buy
```

## Tests

```
node --test tests/game.test.js
```

## Pacing

Prices are set for a team that drifts in Drift mode. `node tools/sim.js [minutes] [seed] [--bar | --active=N]`
plays a simple player and prints how many purchases they make in each five minutes. `--bar` is the reference:
the player drifts with an average run of each venue's distance bar, and the sim works out what that is worth
from the pay rates and a model of points per kilometre measured from the test drivers. `--active=N` instead adds
a flat N times the venue's income (5 was the reference before pay rose with distance). The rule that keeps the economy from snowballing: every price ladder
grows faster than what it buys (a test checks the sum).

Saves from earlier prototype versions are not carried over: a save with another version number starts a new team.
Saves from before venues were kept (same version number) load as they are: the team keeps its venue's levels,
and earlier venues come back owned with no upgrades.

`DT.debug` in the browser console exposes `state`, `world` and `ui` for poking at a running game.
