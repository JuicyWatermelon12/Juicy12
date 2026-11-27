# Drift Tycoon (prototype)

An idle tycoon about a drift team, played in portrait on a phone. It is a separate project from BanditBaller and
only shares this repository for now; the folder can be moved out as it is.

You watch your cars drift a parking-lot course from a fixed, tilted 3D view. Style points from the cars turn into
cash, and cash buys parts, drivers, mechanics and venue upgrades, all of which show up in the view.

## Play

- Open `index.html` in a browser (it needs internet once, for Three.js and the fonts), or
- `node tools/build.js` writes `dist/drift-tycoon.html`, one file with the game's own code inlined.

Progress saves in the browser (`localStorage`). The menu button (top right) has "Reset all progress".

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
- **Crashes and mechanics:** a drifting car can spin out. A free mechanic runs to it, repairs it and it rejoins
  when the track around it is clear. More mechanics repair more cars at once; training shortens repairs.
- **Three limits to upgrade:** pits (how many cars run), the cars themselves (style a second), and grandstands
  (how much style the crowd can cheer for, which caps income). Sponsors multiply cash; track prep multiplies style.
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
  - **Your build matters:** engine and turbo raise top speed, the angle kit adds steering lock and drift angle,
    suspension keeps more speed after a touch, stance and tires raise the score, and so do the driver's skill and
    flair. Steering scales with speed, so fast cars can still make the bends.

- **Street cred:** a rank earned only in Drift mode (Street or Track): 1 cred per 100 m, 1 per 1,000 points, and 10 for each
  record a run sets (best score, best distance on that map). Twelve ranks, Unknown to Myth. It never goes down, is never
  spent, and gives no bonuses; it puts cosmetics on sale.
- **Cosmetics** (Garage → a car → Customise): paint, rims and body kits, bought with cash at set
  prices that never change; items at higher ranks cost more. The crowd pays for looks: each paid item adds style
  to the car wearing it at the venue (paint up to +18%, rims up to +20%, kits +8% / +16% / +30%, added together).
  They change nothing about how the car drives or scores on the street. Paint and rims belong to the team and stay on the car
  when it is traded up. Body kits (Street, Drift, Pro) are bought per car model. The kit decides the car's wing,
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
  - top levels for its upgrades: grandstands 5, sponsors 6, track prep 5.
  - **a distance bar** (`bar`): the average Drift run that keeps the intended pace there. It grows with every
    venue, because the player is expected to get better: 2 km at the Parking Lot, 3 km at the Docks, 4 km at the
    Airfield (the first three are the hook, on a quicker curve), then 1 km more per venue: 5 km at the Pro
    Circuit up to 8 km on the Aircraft Carrier. Each venue's cash rate (`cash`: x1.7, x1, x0.7, x0.7, x0.45,
    x0.45, x0.52) sets its pace; a player below the bar still progresses, only slower.
  The Docks cost $40M, the Airfield $6B, the Pro Circuit $700B, the Stadium $60T, Neon City $2Q and the Carrier
  $80Q. For a player at each bar (from moving in to moving on): the Parking Lot about 15 to 20 minutes, the Docks
  30 to 45, the Airfield about an hour, the Pro Circuit about 2 hours, the Stadium about 3½, Neon City about 6,
  and the Aircraft Carrier about 10 hours to build out: about 24 hours in all. Each venue after the hook takes
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
