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

- **Cars:** six fictional models (Kumo 180 → Shogun GT). Each pit's car has six parts: engine, turbo, tires,
  suspension, stance, angle kit. A part has five big stages (Stock → Street → Sport → Race → Pro → Works), each
  50 times the price of the last, so upgrades are occasional and each one shows: stance lowers the car, the angle
  kit widens its drift angle, tires add smoke, engine and turbo add wings, an intercooler and exhaust flames.
  Trading a car up to the next model keeps its parts and driver.
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
    Laps are 1.07 km (Parking Lot), 1.22 km (Docks) and 0.85 km (Airfield).
  Scoring, payout, street cred and the rules below are the same on both. Best score and distance are kept per map.
  - **Score:** speed x drift angle x a multiplier that builds while the drift is unbroken and resets after a
    straight or a barrier touch. The multiplier's cap starts at x5 and rises by 1 every kilometre.
  - **The road:** a street is different every run: bends tighten, straights shorten and top speed rises over the
    first six kilometres. A track keeps its layout and only the top speed rises.
  - **The end:** touching a barrier ends the run (`MAX_HITS` in `src/data.js`, now 1). "End run" banks it early. Either way you keep the score.
  - **Clean kilometres:** each kilometre marker passed with no touches so far adds 20% of that kilometre's points.
  - **Payout:** every 60 points pays one second of the team's income, and the driver earns double seat time.
    A test driver in a stock car earns about 3.4 times the venue's income over the same time; built cars score
    about twice as fast.
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

- **Venues:** a ladder: Parking Lot → Docks → Airfield (more to come). Every venue bought is kept. The team
  drifts for the crowd at one venue at a time and can move between owned venues freely (Venue tab → Your venues).
  Cars, parts, drivers, mechanics and cosmetics go with the team; each venue keeps its own grandstands, sponsors
  and track prep, which start from zero at a new venue. What is on sale (pits, cars, part stages) follows the best
  venue owned, wherever the team is. Each venue has its own drift line (`src/tracks.js`: straights and arcs, with
  hairpins, kinks and esses) and scenery, and its track can be driven in Drift mode. A venue sets:
  - how many pits it has (3, 4, 5) and which car it puts on sale (Kaze Tourer at the Docks, Rotor Seven at the
    Airfield);
  - the highest part stage on sale (Sport, Race, Pro), so parts last the whole ladder;
  - a style multiplier (x1, x4, x16), a bigger crowd, and dearer venue upgrades;
  - top levels for its upgrades: grandstands 5, sponsors 6, track prep 5.
  The Docks cost $40M and the Airfield $6B. Early progress is quick and it slows with each venue: the simulated
  active player builds the Parking Lot in about 15 minutes and saves about 10 more for the Docks; the Docks take
  about 35 to 45 minutes to build and 20 to 25 to save past; the Airfield takes 45 to 80.

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

Prices are set for a team that drifts in Drift mode. `node tools/sim.js [minutes] [seed] [--active=5]` plays a
simple player and prints how many purchases they make in each five minutes; `--active=5` adds Drift mode
income worth five times the venue's. With it the player makes about 160 to 175 purchases in the first hour; a
team that only idles makes about 50 to 80. The rule that keeps the economy from snowballing: every price ladder
grows faster than what it buys (a test checks the sum).

Saves from earlier prototype versions are not carried over: a save with another version number starts a new team.
Saves from before venues were kept (same version number) load as they are: the team keeps its venue's levels,
and earlier venues come back owned with no upgrades.

`DT.debug` in the browser console exposes `state`, `world` and `ui` for poking at a running game.
