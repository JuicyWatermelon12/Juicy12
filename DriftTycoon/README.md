# Drift Tycoon (prototype)

An idle tycoon about a drift team, played in portrait on a phone. It is a separate project from BanditBaller and
only shares this repository for now; the folder can be moved out as it is.

You watch your cars drift a parking-lot course from a fixed, tilted 3D view. Style points from the cars turn into
cash, and cash buys parts, drivers, mechanics and venue upgrades, all of which show up in the view.

## Play

- Open `index.html` in a browser (it needs internet once, for Three.js and the fonts), or
- `node tools/build.js` writes `dist/drift-tycoon.html`, one file with the game's own code inlined.

Progress saves in the browser (`localStorage`). "Start over" is at the bottom of the Venue tab.

## What is built (first slice)

- **Venue:** the Parking Lot. One drift line, cones at the corners, pit tents, a team HQ.
- **Cars:** six fictional models (Kumo 180 → Shogun GT). Each has six parts: engine, turbo, tires, suspension,
  stance, angle kit. Parts multiply the car's style; stance lowers the car, the angle kit widens its drift angle,
  tires add smoke, and engine and turbo add wings, an intercooler and exhaust flames.
- **Drivers:** named, with a rarity (Common, Rare, Epic, Legendary) and three stats. Skill and flair raise style;
  control lowers the crash rate. Scouting pays for a random driver.
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
```

## Tests

```
node --test tests/game.test.js
```

`DT.debug` in the browser console exposes `state`, `world` and `ui` for poking at a running game.
