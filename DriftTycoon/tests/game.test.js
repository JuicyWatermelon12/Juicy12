/* Run with: node --test DriftTycoon/tests */
const test = require('node:test');
const assert = require('node:assert/strict');
require('../src/data.js');
require('../src/game.js');
const { data: D, game: G } = globalThis.DT;

const rich = () => { const s = G.newState(1000); s.cash = 1e15; return s; };

test('a new team has one Kumo, a rookie driver, one pit and one mechanic', () => {
  const s = G.newState(0);
  assert.equal(s.cars.length, 1);
  assert.equal(s.cars[0].model, 'kumo');
  assert.equal(G.driverOf(s, s.cars[0]).rarity, 'common');
  assert.equal(s.slots, 1);
  assert.equal(s.mech.count, 1);
  assert.equal(s.cash, D.START_CASH);
  assert.ok(G.carStyle(s, s.cars[0]) > 0);
});

test('a car with no driver scores nothing and never crashes', () => {
  const s = G.newState(0);
  s.cars[0].driver = null;
  assert.equal(G.carStyle(s, s.cars[0]), 0);
  assert.equal(G.carCrashRate(s, s.cars[0]), 0);
  assert.equal(G.incomeRate(s).cash, 0);
});

test('parts: each level adds its share, costs grow, purchases stop at the max and without cash', () => {
  const s = rich();
  const car = s.cars[0];
  const base = G.carStyle(s, car);
  const cost0 = G.partCost(car, 'engine');
  assert.ok(G.buyPart(s, car.id, 'engine'));
  assert.ok(Math.abs(G.carStyle(s, car) / base - 1.25) < 1e-9);
  assert.ok(G.partCost(car, 'engine') > cost0);
  assert.ok(G.buyPart(s, car.id, 'turbo'));
  assert.ok(Math.abs(G.carStyle(s, car) / base - 1.25 * 1.3) < 1e-9, 'parts multiply each other');
  for (let i = 0; i < 100; i++) G.buyPart(s, car.id, 'tires');
  assert.equal(car.parts.tires, D.PART_MAX);
  s.cash = 0;
  assert.equal(G.buyPart(s, car.id, 'stance'), false);
  assert.equal(car.parts.stance, 0);
});

test('suspension and tires lower the crash rate, turbo raises it, control lowers it', () => {
  const s = rich();
  const car = s.cars[0];
  const r0 = G.carCrashRate(s, car);
  G.buyPart(s, car.id, 'suspension');
  const r1 = G.carCrashRate(s, car);
  assert.ok(r1 < r0);
  G.buyPart(s, car.id, 'turbo');
  assert.ok(G.carCrashRate(s, car) > r1);
  G.driverOf(s, car).control = 100;
  assert.ok(G.carCrashRate(s, car) < r1 * 0.5);
});

test('income is capped by the crowd; grandstands raise the cap; sponsors multiply cash', () => {
  const s = rich();
  const car = s.cars[0];
  for (let i = 0; i < 10; i++) { G.buyPart(s, car.id, 'engine'); G.buyPart(s, car.id, 'turbo'); }
  let inc = G.incomeRate(s);
  assert.ok(inc.full, 'style is above the crowd capacity');
  assert.equal(inc.cheered, D.CROWD_BASE);
  assert.equal(inc.cash, D.CROWD_BASE);
  G.buyVenue(s, 'stands');
  assert.ok(Math.abs(G.crowdCap(s) - D.CROWD_BASE * D.CROWD_GROWTH) < 1e-9);
  const before = G.incomeRate(s).cash;
  G.buyVenue(s, 'sponsors');
  assert.ok(Math.abs(G.incomeRate(s).cash / before - (1 + D.SPONSOR_PER)) < 1e-9);
  const style = G.carStyle(s, car);
  G.buyVenue(s, 'prep');
  assert.ok(Math.abs(G.carStyle(s, car) / style - (1 + D.PREP_PER)) < 1e-9);
});

test('only running cars earn', () => {
  const s = rich();
  G.buySlot(s);
  G.buyCar(s, 'kumo');
  G.scout(s, G.seeded(3));
  for (let i = 0; i < 5; i++) G.buyVenue(s, 'stands');
  const all = G.incomeRate(s).style;
  const one = G.incomeRate(s, [s.cars[0].id]).style;
  assert.ok(one > 0 && one < all);
  assert.equal(G.incomeRate(s, []).cash, 0);
});

test('pits limit how many cars the team can own', () => {
  const s = rich();
  assert.equal(G.buyCar(s, 'kumo'), false, 'the one pit is taken');
  assert.ok(G.buySlot(s));
  assert.ok(G.buyCar(s, 'hachi'));
  assert.equal(s.cars.length, 2);
  assert.notEqual(s.cars[0].color, s.cars[1].color);
  for (let i = 0; i < 10; i++) G.buySlot(s);
  assert.equal(s.slots, G.MAX_SLOTS);
  assert.equal(G.slotCost(s), Infinity);
});

test('scouting: same seed same driver, stats inside the rarity range, price rises, new driver takes an empty seat', () => {
  const a = rich(), b = rich();
  const da = G.scout(a, G.seeded(42)), db = G.scout(b, G.seeded(42));
  assert.deepEqual(da, db);
  const s = rich();
  G.buySlot(s); G.buyCar(s, 'kumo');
  assert.equal(s.cars[1].driver, null);
  const c0 = G.scoutCost(s);
  const d = G.scout(s, G.seeded(7));
  assert.equal(s.cars[1].driver, d.id);
  assert.ok(G.scoutCost(s) > c0);
  const rng = G.seeded(99);
  const counts = {};
  for (let i = 0; i < 4000; i++) {
    const x = G.rollDriver(s, rng);
    const r = G.rarity(x.rarity);
    counts[r.id] = (counts[r.id] || 0) + 1;
    for (const k of ['skill', 'control', 'flair']) assert.ok(x[k] >= r.min && x[k] <= r.max);
  }
  assert.ok(counts.common > counts.rare && counts.rare > counts.epic && counts.epic > counts.legendary);
  assert.ok(Math.abs(counts.legendary / 4000 - 0.02) < 0.012);
});

test('assigning a driver who already drives swaps the two seats', () => {
  const s = rich();
  G.buySlot(s); G.buyCar(s, 'kumo');
  const d2 = G.scout(s, G.seeded(1));
  const [c1, c2] = s.cars;
  const d1 = c1.driver;
  assert.equal(c2.driver, d2.id);
  assert.ok(G.assignDriver(s, c1.id, d2.id));
  assert.equal(c1.driver, d2.id);
  assert.equal(c2.driver, d1);
  const d3 = G.scout(s, G.seeded(2));
  assert.equal(G.benchDrivers(s).length, 1);
  G.assignDriver(s, c1.id, d3.id);
  assert.deepEqual(G.benchDrivers(s).map((d) => d.id), [d2.id]);
});

test('trading up keeps half the part levels and needs a better model', () => {
  const s = rich();
  const car = s.cars[0];
  for (let i = 0; i < 7; i++) G.buyPart(s, car.id, 'engine');
  assert.equal(G.tradeCar(s, car.id, 'kumo'), false);
  assert.ok(G.tradeCar(s, car.id, 'kaze'));
  assert.equal(car.model, 'kaze');
  assert.equal(car.parts.engine, 3);
  assert.equal(G.tradeCar(s, car.id, 'hachi'), false, 'no trading down');
});

test('mechanics: hiring stops at the max, training shortens repairs down to the minimum', () => {
  const s = rich();
  const t0 = G.repairTime(s);
  assert.equal(t0, D.REPAIR_BASE);
  assert.ok(G.trainMech(s));
  assert.ok(Math.abs(G.repairTime(s) - t0 * D.REPAIR_STEP) < 1e-9);
  for (let i = 0; i < 60; i++) G.trainMech(s);
  assert.equal(G.repairTime(s), D.REPAIR_MIN);
  assert.ok(G.mechTrainMaxed(s));
  for (let i = 0; i < 10; i++) G.hireMech(s);
  assert.equal(s.mech.count, D.MECH_MAX);
  assert.equal(G.mechHireCost(s), Infinity);
});

test('uptime rises with faster repairs, and expected income sits below the all-running income', () => {
  const s = rich();
  const car = s.cars[0];
  for (let i = 0; i < 8; i++) G.buyVenue(s, 'stands');
  const u0 = G.carUptime(s, car);
  assert.ok(u0 > 0.5 && u0 < 1);
  G.trainMech(s); G.trainMech(s);
  assert.ok(G.carUptime(s, car) > u0);
  assert.ok(G.expectedIncome(s) < G.incomeRate(s).cash);
});

test('away earnings: nothing under the minimum, expected income after it, capped', () => {
  const t0 = 5000;
  const s = G.newState(t0);
  assert.equal(G.offlineEarnings(s, t0 + 10 * 1000).cash, 0);
  const hour = G.offlineEarnings(s, t0 + 3600 * 1000);
  assert.ok(Math.abs(hour.cash - G.expectedIncome(s) * 3600) < 1e-6);
  const week = G.offlineEarnings(s, t0 + 7 * 24 * 3600 * 1000);
  assert.equal(week.paidSeconds, D.OFFLINE_CAP);
  assert.ok(Math.abs(week.cash - G.expectedIncome(s) * D.OFFLINE_CAP) < 1e-6);
});

test('save and load round-trips; broken saves give a new team', () => {
  const s = rich();
  G.buySlot(s); G.buyCar(s, 'hachi'); G.scout(s, G.seeded(5)); G.buyVenue(s, 'prep'); G.hireMech(s);
  G.buyPart(s, s.cars[1].id, 'angle');
  s.lastSeen = 123456;
  const back = G.load(G.serialize(s), 999);
  assert.deepEqual(back, s);
  assert.equal(G.load('not json', 5).cars.length, 1);
  assert.equal(G.load('{"cars":3}', 5).cash, D.START_CASH);
  const odd = JSON.parse(G.serialize(s));
  odd.cars[0].model = 'nope'; odd.cars[1].driver = 9999; odd.cash = -5;
  const fixed = G.load(odd, 0);
  assert.equal(fixed.cars.length, 1);
  assert.equal(fixed.cars[0].driver, null);
  assert.equal(fixed.cash, D.START_CASH);
});

test('number formatting', () => {
  assert.equal(G.fmt(0), '0');
  assert.equal(G.fmt(12.34, 1), '12.3');
  assert.equal(G.fmt(999), '999');
  assert.equal(G.fmt(1000), '1.00K');
  assert.equal(G.fmt(15300), '15.3K');
  assert.equal(G.fmt(999999), '1.00M');
  assert.equal(G.fmt(2.5e9), '2.50B');
  assert.equal(G.money(1234567), '$1.23M');
  assert.equal(G.duration(75), '1 min 15 s');
  assert.equal(G.duration(7260), '2 h 1 min');
});

test('every cost is a positive whole number and each model costs and scores more than the last', () => {
  const s = G.newState(0);
  for (const p of D.PARTS) assert.ok(Number.isInteger(G.partCost(s.cars[0], p.id)) && G.partCost(s.cars[0], p.id) > 0);
  for (const v of D.VENUE) assert.ok(G.venueCost(s, v.id) > 0);
  for (let i = 1; i < D.MODELS.length; i++) {
    assert.ok(D.MODELS[i].cost > D.MODELS[i - 1].cost);
    assert.ok(D.MODELS[i].base > D.MODELS[i - 1].base);
  }
});
