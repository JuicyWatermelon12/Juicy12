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

- **Venue:** the Parking Lot. One drift line, cones at the corners, pit tents, a team HQ.
- **Cars:** six fictional models (Kumo 180 → Shogun GT). Each pit's car has six parts: engine, turbo, tires,
  suspension, stance, angle kit. A part has five big stages (Stock → Street → Sport → Race → Pro → Works), each
  40 times the price of the last, so upgrades are occasional and each one shows: stance lowers the car, the angle
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
- **Away earnings:** up to two hours at the team's long-run rate, crashes included.

Not built yet: the drive-it-yourself drift mini-game, more venues, prestige, sound.

## Layout

```
index.html        page shell; loads Three.js r128 from cdnjs and the files below
src/data.js       every tuning number (parts, cars, rarities, costs, crash and repair times)
src/game.js       rules: income, costs, purchases, drivers, save and load. No DOM, no Three.js
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

`node tools/sim.js [minutes] [seed]` plays a simple player and prints how many purchases they make in each five
minutes. With the current numbers that is about 155 in the first hour (about 600 before parts became stages),
dropping to a few every five minutes after two hours. The rule that keeps it from snowballing: every price ladder
grows faster than what it buys (a test checks the sum).

`DT.debug` in the browser console exposes `state`, `world` and `ui` for poking at a running game.
