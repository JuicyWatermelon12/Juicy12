/* Run with: node --test tests/game.test.js */
const test = require('node:test');
const assert = require('node:assert/strict');
require('../src/data.js');
require('../src/game.js');
const { data: D, game: G } = globalThis.DT;

/* A team with money to burn, already at the last venue so nothing there is locked. */
const LAST = D.VENUES.length - 1;
const own = (s, upTo) => { while (s.placeOwned < upTo) { s.placeOwned++; s.levels[s.placeOwned] = { stands: 0, sponsors: 0, prep: 0 }; } return s; };
const rich = () => { const s = own(G.newState(1000), LAST); s.cash = 1e18; G.movePlace(s, LAST); return s; };
const near = (a, b, eps = 1e-9) => assert.ok(Math.abs(a - b) < eps, a + ' vs ' + b);
const mk = (s, fields) => Object.assign({ id: s.nextId++, name: 'T' + s.nextId, rarity: 'common', skill: 20, control: 20, flair: 20,
  helmet: '#fff', level: 1, xp: 0, trait: null }, fields);
/* Hold a tryout with no cooldown in the way. */
const tryout = (s, seed) => { s.cash = 1e18; s.nextTryoutAt = 0; assert.ok(G.holdTryout(s, 1e6, G.seeded(seed))); return s.prospects; };

test('a new team has one Kumo, a level 1 rookie, one pit and one mechanic', () => {
  const s = G.newState(0);
  assert.equal(s.cars.length, 1);
  assert.equal(s.cars[0].model, 'kumo');
  const d = G.driverOf(s, s.cars[0]);
  assert.equal(d.rarity, 'common');
  assert.equal(d.level, 1);
  assert.equal(s.slots, 1);
  assert.equal(s.mech.count, 1);
  assert.equal(s.cash, D.START_CASH);
  assert.equal(s.prospects, null);
  assert.ok(G.carStyle(s, s.cars[0]) > 0);
});

test('a car with no driver scores nothing and never crashes', () => {
  const s = G.newState(0);
  s.cars[0].driver = null;
  assert.equal(G.carStyle(s, s.cars[0]), 0);
  assert.equal(G.carCrashRate(s, s.cars[0]), 0);
  assert.equal(G.incomeRate(s).cash, 0);
});

test('parts: a few big stages that multiply each other, steep prices, a hard top stage', () => {
  const s = rich();
  const car = s.cars[0];
  assert.equal(D.PART_STAGES.length, D.PART_MAX + 1);
  const base = G.carStyle(s, car);
  const cost0 = G.partCost(car, 'engine');
  assert.ok(G.buyPart(s, car.id, 'engine'));
  near(G.carStyle(s, car) / base, 1 + G.part('engine').per);
  near(G.partCost(car, 'engine'), cost0 * D.PART_GROWTH);
  assert.ok(G.buyPart(s, car.id, 'turbo'));
  near(G.carStyle(s, car) / base, (1 + G.part('engine').per) * (1 + G.part('turbo').per));
  for (let i = 0; i < 100; i++) G.buyPart(s, car.id, 'tires');
  assert.equal(car.parts.tires, G.partCap(s), 'up to the top stage on sale at this venue');
  s.cash = 0;
  assert.equal(G.buyPart(s, car.id, 'stance'), false);
  assert.equal(car.parts.stance, 0);
  // every venue after the first puts exactly one more stage on sale, and the last venue sells the top stage
  for (let i = 1; i < D.VENUES.length; i++) assert.equal(D.VENUES[i].stage, D.VENUES[i - 1].stage + 1, D.VENUES[i].id);
  assert.equal(D.VENUES[D.VENUES.length - 1].stage, D.PART_MAX);
});

test('part prices do not depend on the car, and trading up keeps parts and driver', () => {
  const s = rich();
  const car = s.cars[0];
  for (let i = 0; i < 3; i++) G.buyPart(s, car.id, 'engine');
  G.buyPart(s, car.id, 'angle');
  const cost = G.partCost(car, 'engine'), driver = car.driver, style = G.carStyle(s, car);
  assert.equal(G.tradeCar(s, car.id, 'kumo'), false);
  assert.ok(G.tradeCar(s, car.id, 'kaze'));
  assert.equal(car.model, 'kaze');
  assert.equal(car.parts.engine, 3);
  assert.equal(car.parts.angle, 1);
  assert.equal(car.driver, driver);
  assert.equal(G.partCost(car, 'engine'), cost);
  near(G.carStyle(s, car) / style, G.model('kaze').base / G.model('kumo').base);
  assert.equal(G.tradeCar(s, car.id, 'hachi'), false, 'no trading down');
});

test('suspension and tires lower the crash rate, turbo raises it, control lowers it', () => {
  const s = rich();
  const car = s.cars[0];
  const r0 = G.carCrashRate(s, car);
  G.buyPart(s, car.id, 'suspension');
  const r1 = G.carCrashRate(s, car);
  near(r1 / r0, G.part('suspension').crash);
  G.buyPart(s, car.id, 'turbo');
  assert.ok(G.carCrashRate(s, car) > r1);
  const d = G.driverOf(s, car);
  d.control = D.CONTROL_HALF; d.level = 1;
  const half = G.carCrashRate(s, car);
  d.control = 0;
  near(half / G.carCrashRate(s, car), 0.5);
});

test('income is capped by the crowd; grandstands, sponsors and track prep each multiply', () => {
  const s = rich();
  G.movePlace(s, 0);
  const car = s.cars[0];
  for (const p of D.PARTS) car.parts[p.id] = 3;
  let inc = G.incomeRate(s);
  assert.ok(inc.full, 'style is above the crowd capacity');
  const crowd = G.place(s).crowd;
  assert.equal(inc.cheered, crowd);
  assert.equal(inc.cash, crowd * D.VENUE_CASH * G.place(s).cash);
  G.buyVenue(s, 'stands');
  near(G.crowdCap(s), crowd * D.CROWD_GROWTH);
  const before = G.incomeRate(s).cash;
  G.buyVenue(s, 'sponsors'); G.buyVenue(s, 'sponsors');
  near(G.incomeRate(s).cash / before, D.SPONSOR_MULT * D.SPONSOR_MULT);
  const style = G.carStyle(s, car);
  G.buyVenue(s, 'prep');
  near(G.carStyle(s, car) / style, D.PREP_MULT);
});

test('only running cars earn and gain experience', () => {
  const s = rich();
  G.buySlot(s);
  G.buyCar(s, 'kumo');
  const d2 = mk(s, {});
  s.drivers.push(d2); s.cars[1].driver = d2.id;
  for (let i = 0; i < 5; i++) G.buyVenue(s, 'stands');
  const all = G.incomeRate(s).style;
  const one = G.incomeRate(s, [s.cars[0].id]).style;
  assert.ok(one > 0 && one < all);
  assert.equal(G.incomeRate(s, []).cash, 0);
  G.tickXP(s, [s.cars[0].id], 10);
  assert.equal(G.driverOf(s, s.cars[0]).xp, 10);
  assert.equal(d2.xp, 0);
});

test('pits limit how many cars the team can own', () => {
  const s = rich();
  assert.equal(G.buyCar(s, 'kumo'), false, 'the one pit is taken');
  assert.ok(G.buySlot(s));
  assert.ok(G.buyCar(s, 'hachi'));
  assert.equal(s.cars.length, 2);
  assert.notEqual(s.cars[0].color, s.cars[1].color);
  for (let i = 0; i < 10; i++) G.buySlot(s);
  assert.equal(s.slots, G.place(s).pits, 'no more pits than the venue has');
  assert.equal(G.slotCost(s), Infinity);
});

test('driver levels: seat time adds levels, each level grows the stats, rarity sets the cap', () => {
  const s = G.newState(0);
  const d = mk(s, { skill: 20, control: 30, flair: 10 });
  assert.equal(G.xpToNext(d), D.XP_BASE);
  assert.equal(G.addXP(d, D.XP_BASE - 1), 0);
  assert.equal(G.addXP(d, 1), 1);
  assert.equal(d.level, 2);
  assert.equal(d.xp, 0);
  near(G.xpToNext(d), D.XP_BASE * D.XP_GROWTH);
  assert.deepEqual(G.driverStats(d), { skill: Math.round(20 * (1 + D.LEVEL_STAT)), control: Math.round(30 * (1 + D.LEVEL_STAT)), flair: Math.round(10 * (1 + D.LEVEL_STAT)) });
  const m2 = G.driverMult(d);
  assert.ok(G.addXP(d, 1e12) > 0);
  assert.equal(d.level, G.rarity('common').cap);
  assert.equal(d.xp, 0, 'experience past the cap is dropped');
  assert.equal(G.xpToNext(d), 0);
  assert.ok(G.driverMult(d) > m2);
  const caps = D.RARITIES.map((r) => r.cap);
  assert.deepEqual(caps, caps.slice().sort((a, b) => a - b), 'rarer drivers can grow further');
  // a maxed common is worth more than a fresh one, so time with a driver is never wasted
  assert.ok(G.driverMult(d) > G.driverMult(mk(s, { skill: 20, control: 30, flair: 10 })) * 1.3);
  near(G.totalXP(mk(s, { level: 3, xp: 5 })), D.XP_BASE + D.XP_BASE * D.XP_GROWTH + 5);
});

test('tryouts: three prospects, a wait between tryouts, a rising fee, and prospects wait until decided', () => {
  const s = rich();
  G.buySlot(s);   // room on the team
  const now = 5e6, fee = G.tryoutCost(s);
  assert.equal(G.tryoutWait(s, now), 0);
  assert.ok(G.holdTryout(s, now, G.seeded(42)));
  assert.equal(s.prospects.length, D.TRYOUT_SIZE);
  assert.equal(new Set(s.prospects.map((p) => p.name)).size, D.TRYOUT_SIZE);
  assert.equal(s.drivers.length, 1, 'nobody joins until one is signed');
  assert.equal(G.holdTryout(s, now, G.seeded(1)), false, 'not while prospects are waiting');
  near(G.tryoutCost(s), Math.ceil(fee * D.TRYOUT_GROWTH), 1);
  const res = G.signProspect(s, 1);
  assert.equal(s.prospects, null, 'the others go home');
  assert.equal(s.drivers.length, 2);
  assert.equal(res.retired, null);
  near(G.tryoutWait(s, now), D.TRYOUT_COOLDOWN);
  assert.equal(G.holdTryout(s, now + 1000, G.seeded(1)), false, 'still waiting');
  assert.equal(G.tryoutWait(s, now + D.TRYOUT_COOLDOWN * 1000), 0);
  assert.ok(G.holdTryout(s, now + D.TRYOUT_COOLDOWN * 1000, G.seeded(1)));
  assert.ok(G.dismissProspects(s));
  assert.equal(s.prospects, null);
  assert.equal(s.drivers.length, 2);
  // same seed, same prospects
  const a = rich(), b = rich();
  G.holdTryout(a, now, G.seeded(9)); G.holdTryout(b, now, G.seeded(9));
  assert.deepEqual(a.prospects, b.prospects);
});

test('the roster is one driver per pit plus a reserve; a full team means someone retires and mentors', () => {
  const s = rich();
  assert.equal(G.rosterCap(s), 1 + D.RESERVE_SEATS);
  tryout(s, 3);
  assert.ok(G.signProspect(s, 0));
  assert.ok(G.rosterFull(s));
  const veteran = s.drivers[0], car = s.cars[0];
  G.addXP(veteran, 2000);
  const vetXP = G.totalXP(veteran);
  const prospects = tryout(s, 4);
  assert.equal(G.signProspect(s, 0), null, 'a full team needs a retiree');
  assert.equal(s.drivers.length, 2);
  const preview = G.mentored(prospects[0], veteran);
  const res = G.signProspect(s, 0, veteran.id);
  assert.equal(res.retired, veteran);
  assert.equal(s.drivers.length, 2, 'the team does not grow');
  assert.ok(!s.drivers.includes(veteran));
  assert.equal(car.driver, res.driver.id, 'the newcomer takes the seat');
  assert.equal(res.driver.level, preview.level);
  near(G.totalXP(res.driver), Math.min(vetXP * D.MENTOR_SHARE, G.totalXP(Object.assign({}, res.driver, { level: G.levelCap(res.driver), xp: 0 }))), 1e-6);
  assert.ok(res.driver.level > 1 && res.levels === res.driver.level - 1);
  // forty tryouts later the team is still the same size
  for (let i = 0; i < 40; i++) { tryout(s, 100 + i); G.signProspect(s, 0, s.drivers[0].id); }
  assert.equal(s.drivers.length, G.rosterCap(s));
});

test('a car with nobody to drive it skips the tryout wait; a new signing takes the empty seat', () => {
  const s = rich();
  G.buySlot(s);
  s.nextTryoutAt = 9e9;
  assert.ok(G.tryoutWait(s, 0) > 0);
  G.buyCar(s, 'kumo');
  assert.equal(s.cars[1].driver, null);
  assert.ok(G.needsDriver(s));
  assert.equal(G.tryoutWait(s, 0), 0);
  assert.ok(G.holdTryout(s, 0, G.seeded(5)));
  const res = G.signProspect(s, 2);
  assert.equal(s.cars[1].driver, res.driver.id);
  assert.equal(G.needsDriver(s), false);
});

test('rolled drivers: stats inside the rarity range, rarer is rarer, epics always have a trait', () => {
  const s = rich();
  const rng = G.seeded(99);
  const counts = {}, traits = {};
  for (let i = 0; i < 4000; i++) {
    const x = G.rollDriver(s, rng);
    const r = G.rarity(x.rarity);
    counts[r.id] = (counts[r.id] || 0) + 1;
    if (x.trait) { traits[r.id] = (traits[r.id] || 0) + 1; assert.ok(G.trait(x).name); }
    assert.equal(x.level, 1);
    for (const k of ['skill', 'control', 'flair']) assert.ok(x[k] >= r.min && x[k] <= r.max);
  }
  assert.ok(counts.common > counts.rare && counts.rare > counts.epic && counts.epic > counts.legendary);
  assert.ok(Math.abs(counts.legendary / 4000 - 0.02) < 0.012);
  assert.equal(traits.epic, counts.epic);
  assert.ok(Math.abs(traits.common / counts.common - G.rarity('common').trait) < 0.05);
});

test('traits change exactly what they say', () => {
  const s = rich();
  for (let i = 0; i < 2; i++) { G.buySlot(s); G.buyCar(s, 'kumo'); const d = mk(s, {}); s.drivers.push(d); s.cars[i + 1].driver = d.id; }
  const car = s.cars[0], d = G.driverOf(s, car);
  const base = { style: G.carStyle(s, car), crash: G.carCrashRate(s, car), cap: G.crowdCap(s), cash: G.cashMult(s), repair: G.carRepairTime(s, car) };
  const T = (id) => D.TRAITS.find((t) => t.id === id);
  d.trait = 'showboat';
  near(G.carStyle(s, car) / base.style, T('showboat').style); near(G.carCrashRate(s, car) / base.crash, T('showboat').crash);
  d.trait = 'steady';
  near(G.carCrashRate(s, car) / base.crash, T('steady').crash); near(G.carStyle(s, car), base.style);
  d.trait = 'tandem';
  near(G.carStyle(s, car) / base.style, 1 + T('tandem').perCar * 2, 1e-9);
  d.trait = 'fanfav';
  near(G.crowdCap(s) / base.cap, 1 + T('fanfav').crowd);
  d.trait = 'sponsor';
  near(G.cashMult(s) / base.cash, 1 + T('sponsor').cash);
  d.trait = 'gearhead';
  near(G.carRepairTime(s, car) / base.repair, T('gearhead').repair);
  assert.ok(G.carUptime(s, car) > G.carUptime(s, s.cars[1]));
  d.trait = 'learner';
  G.tickXP(s, null, 10);
  near(d.xp, 10 * T('learner').xp);
  assert.equal(G.driverOf(s, s.cars[1]).xp, 10);
  // a reserve driver's trait does nothing
  d.trait = 'fanfav'; car.driver = null;
  near(G.crowdCap(s), base.cap);
});

test('assigning a driver who already drives swaps the two seats', () => {
  const s = rich();
  G.buySlot(s); G.buyCar(s, 'kumo');
  const d2 = mk(s, {}), d3 = mk(s, {});
  s.drivers.push(d2, d3);
  const [c1, c2] = s.cars;
  const d1 = c1.driver;
  c2.driver = d2.id;
  assert.ok(G.assignDriver(s, c1.id, d2.id));
  assert.equal(c1.driver, d2.id);
  assert.equal(c2.driver, d1);
  assert.deepEqual(G.benchDrivers(s), [d3]);
  G.assignDriver(s, c1.id, d3.id);
  assert.deepEqual(G.benchDrivers(s), [d2]);
});

test('mechanics: hiring stops at the max, training shortens repairs down to the minimum', () => {
  const s = rich();
  const t0 = G.repairTime(s);
  assert.equal(t0, D.REPAIR_BASE);
  assert.ok(G.trainMech(s));
  near(G.repairTime(s), t0 * D.REPAIR_STEP);
  for (let i = 0; i < 60; i++) G.trainMech(s);
  assert.equal(G.repairTime(s), D.REPAIR_MIN);
  assert.ok(G.mechTrainMaxed(s));
  assert.ok(s.mech.level <= 8, 'a handful of training levels, not dozens');
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

test('away: nothing under the minimum, expected income after it, capped; seated drivers gain experience', () => {
  const t0 = 5000;
  const s = G.newState(t0);
  assert.equal(G.offlineEarnings(s, t0 + 10 * 1000).cash, 0);
  const hour = G.offlineEarnings(s, t0 + 3600 * 1000);
  near(hour.cash, G.expectedIncome(s) * 3600, 1e-6);
  const week = G.offlineEarnings(s, t0 + 7 * 24 * 3600 * 1000);
  assert.equal(week.paidSeconds, D.OFFLINE_CAP);
  near(week.cash, G.expectedIncome(s) * D.OFFLINE_CAP, 1e-6);
  const d = G.driverOf(s, s.cars[0]), up = G.carUptime(s, s.cars[0]);
  assert.deepEqual(G.awayXP(s, 50), [], 'not enough for a level yet');
  assert.deepEqual(G.awayXP(s, 150), [d]);
  assert.ok(d.level > 1);
  near(G.totalXP(d), 50 * up + 150 * up, 1e-6);
});

test('prices outrun what they buy, so the economy cannot snowball', () => {
  // each line's (log gain) / (log price growth), summed, must stay under 1
  const steps = (xs) => xs.slice(1).map((x, i) => x / xs[i]);
  const avg = (xs) => xs.reduce((a, b) => a + b, 0) / xs.length;
  const modelGain = Math.log(avg(steps(D.MODELS.map((m) => m.base)))) / Math.log(avg(steps(D.MODELS.map((m) => m.cost))));
  const partsGain = D.PARTS.reduce((t, p) => t + Math.log(1 + p.per * D.PART_MAX), 0) / (D.PART_MAX * Math.log(D.PART_GROWTH));
  const venue = (id) => D.VENUE.find((v) => v.id === id).growth;
  const venueGain = Math.log(D.SPONSOR_MULT) / Math.log(venue('sponsors')) + Math.log(D.PREP_MULT) / Math.log(venue('prep'));
  assert.ok(modelGain + partsGain + venueGain < 0.85, 'sum ' + (modelGain + partsGain + venueGain).toFixed(2));
  assert.ok(venue('stands') > D.CROWD_GROWTH);
});

test('save and load round-trips, prospects included; broken saves give a new team', () => {
  const s = rich();
  G.buySlot(s); G.buyCar(s, 'hachi'); G.buyVenue(s, 'prep'); G.hireMech(s);
  tryout(s, 5); G.signProspect(s, 0);
  G.buyPart(s, s.cars[1].id, 'angle');
  G.addXP(s.drivers[0], 500);
  tryout(s, 6);
  s.lastSeen = 123456;
  const back = G.load(G.serialize(s), 999);
  assert.deepEqual(back, s);
  assert.equal(G.load('not json', 5).cars.length, 1);
  assert.equal(G.load('{"cars":3}', 5).cash, D.START_CASH);
  const odd = JSON.parse(G.serialize(s));
  odd.cars[0].model = 'nope'; odd.cars[1].driver = 9999; odd.cash = -5; odd.drivers[0].level = 999; odd.drivers[0].trait = 'nope';
  const fixed = G.load(odd, 0);
  assert.equal(fixed.cars.length, 1);
  assert.equal(fixed.cars[0].driver, null);
  assert.equal(fixed.cash, D.START_CASH);
  assert.equal(fixed.drivers[0].level, G.levelCap(fixed.drivers[0]));
  assert.equal(fixed.drivers[0].trait, null);
});

test('saves from earlier prototypes are not carried over', () => {
  const s = rich();
  const old = JSON.parse(G.serialize(s));
  old.v = 2;
  const back = G.load(JSON.stringify(old), 5);
  assert.equal(back.cash, D.START_CASH);
  assert.equal(back.v, G.SAVE_VERSION);
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
  assert.equal(G.clock(134.2), '2:15');
  assert.equal(G.clock(5), '0:05');
});

test('every cost is a positive whole number and each model costs and scores more than the last', () => {
  const s = G.newState(0);
  for (const p of D.PARTS) assert.ok(Number.isInteger(G.partCost(s.cars[0], p.id)) && G.partCost(s.cars[0], p.id) > 0);
  for (const v of D.VENUE) assert.ok(G.venueCost(s, v.id) > 0);
  assert.ok(G.tryoutCost(s) > 0);
  for (let i = 1; i < D.MODELS.length; i++) {
    assert.ok(D.MODELS[i].cost > D.MODELS[i - 1].cost);
    assert.ok(D.MODELS[i].base > D.MODELS[i - 1].base);
  }
});

/* ---------- Street Drifting (code name: touge) ---------- */
require('../src/tracks.js');
require('../src/tougecore.js');
const C = globalThis.DT.touge, T = D.TOUGE;
const HZ = 120;
const drive = (run, steerFn, seconds) => {
  const ev = [];
  for (let i = 0; i < seconds * HZ && !run.done; i++) ev.push(...C.step(run, steerFn(run), 1 / HZ));
  return ev;
};
const straight = (halfW) => ({ pts: Array.from({ length: 40000 }, (_, i) => ({ x: 0, z: i, h: 0, k: 0 })), ds: 1, n: 40000, halfW, gen: null,
  speed: { top: 1, perKm: T.SPEED_PER_KM } });
const go = (road, setup) => { const run = C.newRun(road, setup); run.hold = 0; return run; };
const stock = () => { const s = G.newState(0); return { s, car: s.cars[0], setup: C.carSetup(s, s.cars[0]) }; };

test('street road: endless, made on demand, same seed same road, always heading downhill', () => {
  const a = C.newRoad(G.seeded(7)), b = C.newRoad(G.seeded(7)), c = C.newRoad(G.seeded(8));
  C.extendRoad(a, 5000); C.extendRoad(c, 5000);
  for (let n = 400; n <= 5000; n += 137) C.extendRoad(b, n);   // grown in many small steps
  C.extendRoad(b, 5000);
  assert.equal(a.n, 5000);
  assert.deepEqual(a.pts, b.pts, 'how the road is asked for does not change it');
  assert.notDeepEqual(a.pts.slice(0, 600).map((p) => p.x), c.pts.slice(0, 600).map((p) => p.x));
  for (let seed = 1; seed <= 12; seed++) {
    const r = C.extendRoad(C.newRoad(G.seeded(seed)), 12000);
    for (let i = 1; i < r.n; i++) {
      assert.ok(r.pts[i].z > r.pts[i - 1].z, 'z always grows, so the road cannot cross itself');
      assert.ok(Math.abs(r.pts[i].h) < Math.PI / 2 - 0.1);
      assert.ok(Math.abs(r.pts[i].k) <= 1 / T.RADIUS_MIN[2] + 1e-9);
    }
    assert.ok(r.pts.slice(0, T.START_STRAIGHT - 6).every((p) => p.k === 0), 'it starts with a straight');
  }
});

test('street road: bends get tighter and straights shorter to RAMP_KM, harder still to HARD_KM, then hold', () => {
  assert.equal(C.difficulty(0), 0);
  assert.equal(C.difficulty(T.RAMP_KM * 1000), 1);
  assert.equal(C.difficulty(T.HARD_KM * 1000), 2);
  assert.equal(C.difficulty(T.HARD_KM * 5000), 2);
  let early = 0, mid = 0, late = 0, later = 0;
  const bend = (pts) => pts.reduce((t, p) => t + Math.abs(p.k), 0) / pts.length;
  for (let seed = 1; seed <= 12; seed++) {
    const r = C.extendRoad(C.newRoad(G.seeded(seed)), 20000);
    early += bend(r.pts.slice(100, 1500)); mid += bend(r.pts.slice(5000, 6500)); late += bend(r.pts.slice(11000, 15000)); later += bend(r.pts.slice(16000, 20000));
  }
  assert.ok(mid > early * 1.25, 'more turning per metre by RAMP_KM: ' + (mid / early).toFixed(2));
  assert.ok(late > mid * 1.05, 'and more again past it: ' + (late / mid).toFixed(2));
  assert.ok(Math.abs(later / late - 1) < 0.1, 'and no harder after HARD_KM');
  for (const k of ['RADIUS_MIN', 'RADIUS_MAX', 'STRAIGHT_MIN', 'STRAIGHT_MAX']) assert.ok(T[k][2] <= T[k][1] && T[k][1] < T[k][0], k);
  for (const k of ['TURN_MIN', 'TURN_MAX']) assert.ok(T[k][2] > T[k][1] && T[k][1] > T[k][0], k);
});

test('the hardest street is still drivable: a careful driver gets to 12 km on every road, in a stock car and a built one', () => {
  const s = rich(), car = s.cars[0];
  for (const build of [0, D.PART_MAX]) {
    for (const p of D.PARTS) car.parts[p.id] = build;
    for (let seed = 1; seed <= 6; seed++) {
      const run = go(C.newRoad(G.seeded(seed)), C.carSetup(s, car));
      for (let i = 0; i < HZ * 1200 && !run.done && run.dist < 12000; i++) C.step(run, C.steady(run), 1 / HZ);
      assert.ok(!run.done && run.dist >= 12000, 'build ' + build + ', road ' + seed + ' ended at ' + (run.dist / 1000).toFixed(1) + ' km');
    }
  }
});

test('street handling: the countdown holds the car, steering right turns right, the nose leads the slide', () => {
  const { setup } = stock();
  const run = C.newRun(straight(500), setup);
  const z0 = run.z;
  assert.equal(z0, T.START_OFFSET);
  C.step(run, 1, 1);
  assert.equal(run.z, z0, 'held during the countdown');
  drive(run, () => 0, 5);
  assert.ok(run.z > z0 + 5 && Math.abs(run.x) < 1e-9 && run.slip === 0);
  assert.equal(run.score, 0, 'driving straight scores nothing');
  drive(run, () => 1, 0.75);
  assert.ok(run.h < 0 && run.x < 0, 'right is towards -x when heading downhill (+z)');
  assert.ok(run.slip < 0 && Math.abs(run.slip) > T.DRIFT_MIN, 'the nose is ahead of the direction of travel');
  assert.ok(Math.abs(run.slip) <= setup.maxSlip + 1e-9);
  assert.ok(run.drifting && run.score > 0 && run.mult > 1);
  assert.ok(run.speed < setup.top, 'sliding costs speed');
});

test('street handling: the tightest turn is the same at any speed, so fast cars can still make the bends', () => {
  const { s, car } = stock();
  const radius = (parts) => {
    Object.assign(car.parts, parts);
    const run = go(straight(1e6), C.carSetup(s, car));
    drive(run, () => 0, 8);
    const h0 = run.h, t0 = run.t;
    drive(run, () => 1, 0.5);
    return run.speed / (Math.abs(run.h - h0) / (run.t - t0));
  };
  const slow = radius({ engine: 0, turbo: 0 }), fast = radius({ engine: 5, turbo: 5 });
  assert.ok(Math.abs(fast / slow - 1) < 0.2, slow.toFixed(1) + ' m vs ' + fast.toFixed(1) + ' m');
  assert.ok(slow < T.RADIUS_MIN[1], 'tighter than the tightest bend the road makes');
});

test('street scoring: the multiplier builds with unbroken drifting and resets after a straight', () => {
  const { setup } = stock();
  const run = go(straight(500), setup);
  drive(run, (r) => Math.cos(r.t * 3.2), 6);
  assert.equal(run.hits, 0);
  assert.ok(run.bestMult > 1.5, 'weaving down a straight keeps a drift going: x' + run.bestMult.toFixed(2));
  const score = run.score;
  drive(run, () => 0, 3);
  assert.equal(run.mult, 1, 'straightening up resets the multiplier');
  assert.ok(run.score - score < score, 'and stops the scoring');
});

test('street barriers: a touch resets the multiplier and keeps the car on the road; the third ends the run', () => {
  const { setup } = stock();
  const road = straight(T.HALF_WIDTH), run = go(road, setup);
  assert.equal(C.hitsLeft(run), T.MAX_HITS);
  drive(run, (r) => Math.cos(r.t * 3.2), 2);
  assert.ok(run.mult > 1);
  const limit = road.halfW - T.CAR_HALF;
  for (let hit = 1; hit <= T.MAX_HITS; hit++) {
    const dir = hit % 2 ? 1 : -1;
    let ev = [];
    for (let i = 0; i < HZ * 6 && !ev.includes('hit'); i++) { ev = C.step(run, dir, 1 / HZ); assert.ok(Math.abs(run.lateral) <= limit + 1e-6); }
    assert.ok(ev.includes('hit'));
    assert.equal(run.hits, hit);
    assert.equal(run.mult, 1);
    assert.equal(C.hitsLeft(run), T.MAX_HITS - hit);
    assert.equal(run.done, hit === T.MAX_HITS);
    assert.equal(ev.includes('out'), hit === T.MAX_HITS);
  }
  const frozen = { x: run.x, z: run.z, score: run.score, t: run.t };
  assert.deepEqual(C.step(run, 1, 1), [], 'nothing happens after the run is over');
  assert.deepEqual({ x: run.x, z: run.z, score: run.score, t: run.t }, frozen);
});

test('street distance: each kilometre raises the multiplier cap, the pay rate and the speed', () => {
  const { setup } = stock();
  let run = go(straight(500), setup);
  assert.equal(C.multCap(run), T.COMBO_MAX);
  let ev = drive(run, (r) => Math.cos(r.t * 3.2) * 0.6, 200);
  const kms = ev.filter((e) => e === 'km').length;
  assert.ok(kms >= 2 && run.km === kms && run.dist >= kms * 1000);
  assert.equal(C.multCap(run), T.COMBO_MAX + kms * T.COMBO_MAX_PER_KM);
  assert.ok(run.bestMult > T.COMBO_MAX, 'the multiplier went past the first cap: x' + run.bestMult.toFixed(1));
  assert.equal(run.kmMarks.length, kms, 'the score is noted at every marker');
  for (let k = 1; k < kms; k++) assert.ok(run.kmMarks[k] > run.kmMarks[k - 1]);
  // top speed: +SPEED_PER_KM a kilometre, up to HARD_KM
  const fast = go(straight(500), setup);
  drive(fast, () => 0, 30);
  const v0 = fast.speed;
  drive(fast, () => 0, 900);
  assert.ok(fast.dist > T.HARD_KM * 1000);
  assert.ok(Math.abs(fast.speed / setup.top - (1 + T.SPEED_PER_KM * T.HARD_KM)) < 0.01 && v0 < fast.speed);
  // a venue track: its own starting speed and a faster rise per kilometre, to the same HARD_KM
  for (const id of DT.tracks.ids) {
    const r = C.trackRoad(id);
    assert.deepEqual(r.speed, { top: T.TRACK_SPEED[id], perKm: T.TRACK_SPEED_PER_KM });
    assert.ok(T.TRACK_SPEED[id] > 0.9 && T.TRACK_SPEED_PER_KM > T.SPEED_PER_KM, id + ' speeds up faster than the street');
  }
});

test('drift pay rises with every kilometre: short runs pay little, long runs a lot', () => {
  // the rates: x1 in the first kilometre, rising every kilometre, holding from the last entry
  assert.equal(C.payRate(0), 1);
  for (let k = 1; k < T.PAY_PER_KM.length; k++) assert.ok(C.payRate(k) > C.payRate(k - 1), 'km ' + (k + 1));
  assert.equal(C.payRate(T.PAY_PER_KM.length + 5), T.PAY_PER_KM[T.PAY_PER_KM.length - 1]);
  assert.equal(T.PAY_PER_KM.length, T.HARD_KM, 'pay stops rising where the road stops getting harder');
  // each kilometre's points pay at its own rate
  const { s, car, setup } = stock();
  const run = go(straight(500), setup);
  Object.assign(run, { score: 9000, kmMarks: [2000, 5000], km: 2, dist: 2500 });
  const res = C.payout(s, run), inc = G.expectedIncome(s);
  near(res.seconds, (2000 * C.payRate(0) + 3000 * C.payRate(1) + 4000 * C.payRate(2)) / T.POINTS_PER_SECOND);
  near(C.paySeconds(run), res.seconds);
  assert.deepEqual(res.perKm.map((k) => [k.km, k.points, k.rate]), [[1, 2000, 1], [2, 3000, C.payRate(1)], [3, 4000, C.payRate(2)]]);
  near(res.perKm.reduce((t, k) => t + k.cash, 0), res.cash);
  near(res.cash, res.seconds * inc);
  assert.equal(res.rate, C.payRate(2));
  // against the old flat pay (60 points a second, +20% for every kilometre finished), on real runs by the test
  // drivers: a run that ends at 1 km pays about 15%, at 2 km about half, at 3 km about the same, at 4 km more
  const ratios = {};
  for (const keen of [0.25, 0.4, 0.55]) {
    for (let seed = 1; seed <= 8; seed++) {
      const r = go(C.newRoad(G.seeded(seed)), setup);
      for (const km of [1, 2, 3]) {
        for (let i = 0; i < HZ * 400 && !r.done && r.dist < km * 1000; i++) C.step(r, C.drifter(r, keen), 1 / HZ);
        if (r.done) break;
        const old = (r.score + 0.2 * r.kmMarks[r.kmMarks.length - 1]) / 60;
        (ratios[km] = ratios[km] || []).push(C.paySeconds(r) / old);
      }
    }
  }
  const median = (a) => a.slice().sort((x, y) => x - y)[a.length >> 1];
  console.log('   pay against the old flat pay, for runs ending at 1 / 2 / 3 km: ' + [1, 2, 3].map((k) => median(ratios[k]).toFixed(2) + ' (' + ratios[k].length + ' runs)').join(' / '));
  near(median(ratios[1]), 0.15, 0.03);
  near(median(ratios[2]), 0.5, 0.08);
  near(median(ratios[3]), 1, 0.12);
});

test('street runs: test drivers get a good way down every road, and a built car scores faster than a stock one', () => {
  const s = rich(), car = s.cars[0];
  const rate = {};
  for (const build of [0, 3, 5]) {
    for (const p of D.PARTS) car.parts[p.id] = build;
    const setup = C.carSetup(s, car);
    let t = 0, score = 0, dist = 0;
    for (let seed = 1; seed <= 10; seed++) {
      const run = go(C.newRoad(G.seeded(seed)), setup);
      drive(run, (r) => C.drifter(r, 0.7), 900);
      assert.ok(run.done && run.hits === T.MAX_HITS, 'every run ends on the last allowed touch');
      t += run.t; score += run.score; dist += run.dist;
    }
    rate[build] = score / t;
    console.log(`   ${D.PART_STAGES[build].padEnd(6)} car: lasts ${(t / 10).toFixed(0)} s, ${(dist / 10000).toFixed(1)} km, ${(score / 10).toFixed(0)} points, ${rate[build].toFixed(0)} points a second (averages over 10 roads)`);
    assert.ok(dist / 10 > 1000, 'the road is drivable');
  }
  assert.ok(rate[3] > rate[0] * 1.3 && rate[5] > rate[3], 'upgrades pay on the street');
});

test('the latest part stages change handling only a little, so built cars stay drivable', () => {
  assert.equal(C.handling(T.HANDLING_STAGES), T.HANDLING_STAGES);
  near(C.handling(T.HANDLING_STAGES + 1), T.HANDLING_STAGES + T.HANDLING_EXTRA);
  for (let st = 0; st <= T.HANDLING_STAGES; st++) assert.equal(C.handling(st), st);
  const s = rich(), car = s.cars[0];
  for (const p of D.PARTS) car.parts[p.id] = T.HANDLING_STAGES;
  const works = C.carSetup(s, car);
  for (const p of D.PARTS) car.parts[p.id] = D.PART_MAX;
  const top = C.carSetup(s, car), extra = (D.PART_MAX - T.HANDLING_STAGES) * T.HANDLING_EXTRA;
  near(top.top - works.top, 2 * extra * T.TOP_PER_STAGE, 1e-9);
  near(top.lock - works.lock, extra * T.LOCK_PER_ANGLE, 1e-9);
  assert.ok(top.bonus > works.bonus, 'stance and tires still add their full score bonus');
});

test('street setup: parts and the driver change the car on the road', () => {
  const s = rich(), car = s.cars[0];
  const a = C.carSetup(s, car);
  car.parts.engine = 2; car.parts.turbo = 1; car.parts.angle = 3; car.parts.suspension = 2; car.parts.stance = 1; car.parts.tires = 1;
  const b = C.carSetup(s, car);
  near(b.top - a.top, 3 * T.TOP_PER_STAGE);
  near(b.maxSlip - a.maxSlip, 3 * T.SLIP_PER_ANGLE);
  assert.ok(b.lock > a.lock && b.hitKeep > a.hitKeep);
  near(b.bonus / a.bonus, 1 + T.BONUS_PER_STANCE + T.BONUS_PER_TIRES);
  G.driverOf(s, car).skill += 40;
  assert.ok(C.carSetup(s, car).bonus > b.bonus);
});

test('street payout: points buy seconds of team income, the driver gets double seat time, records are kept', () => {
  const s = G.newState(0), car = s.cars[0], d = G.driverOf(s, car);
  const run = C.newRun(C.newRoad(G.seeded(1)), C.carSetup(s, car));
  Object.assign(run, { score: 7000, t: 50, hits: 3, dist: 1234, done: true });
  const income = G.expectedIncome(s);
  const res = C.payout(s, run);
  near(res.seconds, 7000 * C.payRate(0) / T.POINTS_PER_SECOND, 1e-9);   // never reached a marker: all of it at the first kilometre's rate
  near(res.cash, income * res.seconds);
  assert.ok(res.out);
  const cash = s.cash, xp = G.totalXP(d);
  const got = C.collect(s, car, run);
  near(s.cash - cash, got.cash);
  near(G.totalXP(d) - xp, 50 * T.XP_MULT);
  assert.equal(got.levels, d.level - 1);
  assert.ok(got.bestScore && got.bestDist);
  assert.equal(got.cred, 12 * D.CRED_PER_100M + 7 * D.CRED_PER_1000_POINTS + 2 * D.CRED_NEW_BEST, 'distance + points + two records');
  assert.deepEqual(s.street, { runs: 1, cred: got.cred, best: { pass: { score: 7000, dist: 1234 } } });
  run.score = 5000; run.dist = 2000;
  const next = C.collect(s, car, run);
  assert.ok(!next.bestScore && next.bestDist);
  assert.equal(next.cred, 20 + 5 + D.CRED_NEW_BEST);
  assert.deepEqual(s.street, { runs: 2, cred: got.cred + next.cred, best: { pass: { score: 7000, dist: 2000 } } });
  // records are kept per map: the first run on another map sets both of its records
  run.map = 'track:lot'; run.score = 100; run.dist = 300;
  const other = C.collect(s, car, run);
  assert.ok(other.bestScore && other.bestDist);
  assert.deepEqual(s.street.best, { pass: { score: 7000, dist: 2000 }, 'track:lot': { score: 100, dist: 300 } });
  assert.deepEqual(G.load(G.serialize(s), 0).street, s.street);
});

test('venue pace: each venue asks for longer runs, and its cash rate makes a run of that length keep the intended pace', () => {
  for (let i = 0; i < D.VENUES.length; i++) {
    const v = D.VENUES[i];
    assert.ok(v.bar > 0 && v.cash > 0, v.id);
    if (i) assert.ok(v.bar > D.VENUES[i - 1].bar, v.id + ' asks for longer runs than the venue before');
  }
  const { s, car, setup } = stock();
  let seconds = 0, time = 0;
  for (let seed = 1; seed <= 10; seed++) {
    const run = go(C.newRoad(G.seeded(seed)), setup);
    drive(run, (r) => C.drifter(r, 0.7), 90);   // the first minute and a half
    seconds += C.payout(s, run).seconds; time += run.t;
  }
  console.log(`   a stock-car test driver's first minute and a half is worth ${(seconds / time).toFixed(1)} times the venue's income`);
  assert.ok(seconds / time > 0.3 && seconds / time < 5);
});

/* ---------- Street cred and cosmetics ---------- */

test('street cred: ranks only go up, each needs more than the last, and a new rank reports what it puts on sale', () => {
  const creds = D.RANKS.map((r) => r.cred);
  assert.equal(creds[0], 0);
  assert.deepEqual(creds, creds.slice().sort((a, b) => a - b));
  assert.equal(new Set(creds).size, creds.length);
  assert.equal(G.rankOf(0).index, 0);
  assert.equal(G.rankOf(D.RANKS[1].cred - 1).index, 0);
  assert.equal(G.rankOf(D.RANKS[1].cred).index, 1);
  near(G.rankOf((D.RANKS[1].cred + D.RANKS[2].cred) / 2).progress, 0.5);
  const top = G.rankOf(1e9);
  assert.equal(top.index, D.RANKS.length - 1);
  assert.equal(top.next, null);
  const s = G.newState(0);
  let r = G.addCred(s, D.RANKS[1].cred - 1);
  assert.equal(r.after.index, 0);
  assert.deepEqual(r.unlocked, []);
  r = G.addCred(s, D.RANKS[3].cred);
  assert.equal(r.before.index, 0);
  assert.equal(r.after.index, 3);
  const want = [].concat(D.PAINTS, D.RIMS, D.KITS).filter((c) => c.rank >= 1 && c.rank <= 3).map((c) => c.id).sort();
  assert.deepEqual(r.unlocked.map((u) => u.item.id).sort(), want);
  assert.ok(want.length > 0);
  G.addCred(s, -50);
  assert.equal(G.rank(s).index, 3, 'cred never goes down');
  // idling earns none
  const cred = s.street.cred;
  G.earn(s, 1e9); G.awayXP(s, 3600);
  assert.equal(s.street.cred, cred);
});

test('cosmetics data: set prices, dearer at higher ranks, every rank needed exists, new cars use free paint', () => {
  for (const list of [D.PAINTS, D.RIMS, D.KITS]) {
    assert.equal(new Set(list.map((c) => c.id)).size, list.length);
    for (const c of list) assert.ok(c.cost >= 0 && c.rank >= 0 && c.rank < D.RANKS.length, c.id);
    const paid = list.filter((c) => c.cost > 0);
    for (let i = 1; i < paid.length; i++) assert.ok(paid[i].rank >= paid[i - 1].rank && paid[i].cost >= paid[i - 1].cost, 'later unlocks cost more: ' + paid[i].id);
  }
  for (const id of D.FACTORY_PAINTS) assert.equal(G.cosmetic('paint', id).cost, 0);
  const s = rich();
  for (let i = 0; i < 4; i++) { G.buySlot(s); G.buyCar(s, 'kumo'); }
  assert.equal(new Set(s.cars.map((c) => c.paint)).size, 5, 'five cars, five different factory paints');
  for (const c of s.cars) { assert.equal(c.color, G.cosmetic('paint', c.paint).color); assert.equal(c.rims, 'steel'); assert.equal(c.kit, 'stock'); }
});

test('cosmetics: locked until the rank, bought once with cash; looks add venue style and nothing else', () => {
  const s = G.newState(0), car = s.cars[0];
  const before = { style: G.carStyle(s, car), crash: G.carCrashRate(s, car), setup: C.carSetup(s, car), income: G.expectedIncome(s) };
  s.cash = 1e12;
  assert.ok(!G.cosmeticUnlocked(s, 'paint', 'midnight'));
  assert.equal(G.buyCosmetic(s, car.id, 'paint', 'midnight'), false, 'rank too low');
  assert.equal(G.applyCosmetic(s, car.id, 'paint', 'midnight'), false, 'not owned');
  G.addCred(s, D.RANKS[D.RANKS.length - 1].cred);
  s.cash = 100;
  assert.equal(G.buyCosmetic(s, car.id, 'paint', 'midnight'), false, 'not enough cash');
  s.cash = 1e12;
  const cost = G.cosmeticCost('paint', 'midnight');
  assert.ok(G.buyCosmetic(s, car.id, 'paint', 'midnight'));
  near(1e12 - s.cash, cost);
  assert.equal(car.paint, 'midnight');
  assert.equal(car.color, G.cosmetic('paint', 'midnight').color);
  assert.equal(G.buyCosmetic(s, car.id, 'paint', 'midnight'), false, 'already owned');
  assert.ok(G.buyCosmetic(s, car.id, 'rims', 'bronze') && G.buyCosmetic(s, car.id, 'kit', 'drift'));
  assert.equal(car.rims, 'bronze'); assert.equal(car.kit, 'drift');
  // upgrades do not change a cosmetic's price
  for (const p of D.PARTS) car.parts[p.id] = D.PART_MAX;
  assert.equal(G.cosmeticCost('paint', 'midnight'), cost);
  assert.equal(G.cosmeticCost('kit', 'pro', car.model), G.cosmetic('kit', 'pro').cost * G.model(car.model).kitScale);
  for (const p of D.PARTS) car.parts[p.id] = 0;
  // looks: the bonuses of paint, rims and kit add up on the style this car scores at the venue...
  const looks = 1 + G.cosmetic('paint', 'midnight').style + G.cosmetic('rims', 'bronze').style + G.cosmetic('kit', 'drift').style;
  near(G.looksMult(car), looks);
  near(G.carStyle(s, car) / before.style, looks);
  assert.ok(looks > 1.2);
  // ...and change nothing else: crash rate and how the car drives and scores on the street stay the same
  assert.deepEqual({ crash: G.carCrashRate(s, car), setup: C.carSetup(s, car) }, { crash: before.crash, setup: before.setup });
  G.applyCosmetic(s, car.id, 'paint', 'red'); G.applyCosmetic(s, car.id, 'rims', 'steel'); G.applyCosmetic(s, car.id, 'kit', 'stock');
  near(G.looksMult(car), 1);
  near(G.carStyle(s, car), before.style);
  for (const list of [D.PAINTS, D.RIMS, D.KITS]) {
    const paid = list.filter((c) => c.cost > 0);
    assert.ok(list.filter((c) => c.cost === 0).every((c) => !c.style), 'free items give nothing');
    for (let i = 1; i < paid.length; i++) assert.ok(paid[i].style >= paid[i - 1].style && paid[0].style > 0, 'dearer items give at least as much');
  }
});

test('cosmetics: paint and rims belong to the team and survive a trade-up; kits are per model', () => {
  const s = rich();
  G.addCred(s, 1e6);
  G.buySlot(s); G.buyCar(s, 'kumo');
  const [a, b] = s.cars;
  G.buyCosmetic(s, a.id, 'paint', 'black'); G.buyCosmetic(s, a.id, 'rims', 'six'); G.buyCosmetic(s, a.id, 'kit', 'street');
  s.cash = 1e9;   // small enough for exact sums
  const cash = s.cash;
  assert.ok(G.applyCosmetic(s, b.id, 'paint', 'black') && G.applyCosmetic(s, b.id, 'rims', 'six'), 'the other car can wear them');
  assert.ok(G.applyCosmetic(s, b.id, 'kit', 'street'), 'same model, same kit');
  assert.equal(s.cash, cash, 'applying is free');
  assert.ok(G.applyCosmetic(s, b.id, 'paint', 'blue') && G.applyCosmetic(s, b.id, 'kit', 'stock'));
  G.tradeCar(s, a.id, 'hachi');
  assert.equal(a.paint, 'black'); assert.equal(a.rims, 'six');
  assert.equal(a.kit, 'stock', 'the Kumo kit does not fit the Hachi');
  assert.ok(!G.ownsCosmetic(s, 'kit', 'street', 'hachi') && G.ownsCosmetic(s, 'kit', 'street', 'kumo'));
  assert.ok(G.buyCosmetic(s, a.id, 'kit', 'street'));
  near(cash - s.cash, G.model('hachi').cost + G.cosmeticCost('kit', 'street', 'hachi'));
  assert.ok(G.cosmeticCost('kit', 'street', 'hachi') > G.cosmeticCost('kit', 'street', 'kumo'));
  // saved and loaded; anything not owned falls back
  const back = G.load(G.serialize(s), 0);
  assert.deepEqual(back.owned, s.owned);
  assert.deepEqual(back.cars.map((c) => [c.paint, c.rims, c.kit]), s.cars.map((c) => [c.paint, c.rims, c.kit]));
  const raw = JSON.parse(G.serialize(s));
  raw.cars[0].paint = 'blood'; raw.cars[0].rims = 'neon'; raw.cars[0].kit = 'pro'; raw.owned.paints.push('nope');
  const fixed = G.load(raw, 0);
  assert.deepEqual([fixed.cars[0].paint, fixed.cars[0].rims, fixed.cars[0].kit], [D.FACTORY_PAINTS[0], 'steel', 'stock']);
  assert.ok(!fixed.owned.paints.includes('nope'));
});

/* ---------- Venues ---------- */

test('venue ladder: each venue costs more, has at least as many pits, more style, a new car and a higher part stage', () => {
  const V = D.VENUES;
  assert.equal(V[0].cost, 0);
  assert.equal(V[0].style, 1);
  for (let i = 1; i < V.length; i++) {
    assert.ok(V[i].cost > V[i - 1].cost * 10 && V[i].pits >= V[i - 1].pits && V[i].style > V[i - 1].style && V[i].crowd > V[i - 1].crowd, V[i].id);
    assert.ok(V[i].scale > V[i - 1].scale && V[i].stage > V[i - 1].stage && V[i].cars.length > 0, V[i].id);
  }
  assert.ok(V[LAST].stage <= D.PART_MAX && V[LAST].pits <= G.MAX_SLOTS);
  // every car model is put on sale by exactly one venue, in order
  const listed = V.flatMap((v) => v.cars);
  assert.equal(new Set(listed).size, listed.length);
  assert.deepEqual(listed, D.MODELS.slice(0, listed.length).map((m) => m.id));
  assert.equal(G.modelPlace('kumo'), 0);
  assert.equal(G.modelPlace(V[1].cars[0]), 1);
});

test('a venue limits pits, car models, part stages and upgrade levels', () => {
  const s = G.newState(0), car = s.cars[0], lot = D.VENUES[0], next = D.VENUES[1];
  s.cash = 1e18;
  for (let i = 0; i < 10; i++) G.buySlot(s);
  assert.equal(s.slots, lot.pits);
  assert.equal(G.rosterCap(s), lot.pits + D.RESERVE_SEATS);
  assert.ok(G.modelUnlocked(s, 'hachi') && !G.modelUnlocked(s, next.cars[0]));
  assert.equal(G.buyCar(s, next.cars[0]), false, 'not on sale here');
  assert.equal(G.tradeCar(s, car.id, next.cars[0]), false);
  assert.ok(G.tradeCar(s, car.id, 'hachi'));
  for (let i = 0; i < 10; i++) G.buyPart(s, car.id, 'engine');
  assert.equal(car.parts.engine, lot.stage);
  assert.equal(G.partCap(s), lot.stage);
  for (const v of D.VENUE) {
    for (let i = 0; i < 30; i++) G.buyVenue(s, v.id);
    assert.equal(s.venue[v.id], D.VENUE_MAX[v.id]);
    assert.equal(G.venueCost(s, v.id), Infinity);
  }
  assert.ok(G.venueMaxed(s));
});

test('venues are kept: buying adds one, each keeps its own upgrade levels, and the team moves between them', () => {
  const s = G.newState(0), car = s.cars[0], lot = D.VENUES[0], next = D.VENUES[1];
  assert.equal(G.movePlace(s), false, 'nowhere else to go yet');
  assert.equal(G.movePlace(s, 1), false, 'not bought yet');
  s.cash = next.cost - 1;
  assert.equal(G.buyPlace(s), false);
  s.cash = next.cost + 5e7;
  car.parts.engine = 2; Object.assign(s.venue, { stands: 3, sponsors: 4, prep: 2 }); s.mech.level = 2;
  G.addCred(s, 500); G.buyCosmetic(s, car.id, 'paint', 'black');
  const cash = s.cash, driver = car.driver, style = G.carStyle(s, car), cred = s.street.cred, lotIncome = G.expectedIncome(s);
  assert.ok(G.buyPlace(s));
  near(cash - s.cash, next.cost);
  assert.equal(s.place, 0, 'bought, but the team has not moved');
  assert.equal(s.placeOwned, 1);
  assert.ok(G.ownsPlace(s, 0) && G.ownsPlace(s, 1) && !G.ownsPlace(s, 2));
  // owning it already puts its car, pits and part stage on sale
  assert.ok(G.modelUnlocked(s, next.cars[0]) && G.partCap(s) === next.stage && G.maxPits(s) === next.pits);
  assert.equal(G.nextPlace(s), D.VENUES[2] || null);
  // move there: its levels start at zero, the old venue keeps its own
  assert.ok(G.movePlace(s));
  assert.equal(G.place(s), next);
  assert.deepEqual(s.venue, { stands: 0, sponsors: 0, prep: 0 });
  assert.equal(s.venue, s.levels[1]);
  assert.deepEqual(s.levels[0], { stands: 3, sponsors: 4, prep: 2 });
  assert.deepEqual([car.parts.engine, car.driver, car.paint, s.mech.level, s.street.cred], [2, driver, 'black', 2, cred]);
  near(G.carStyle(s, car) / style, next.style / Math.pow(D.PREP_MULT, 2), 1e-9);
  near(G.crowdCap(s), next.crowd);
  assert.equal(G.venueCost(s, 'stands'), Math.ceil(D.VENUE[0].cost * next.scale));
  s.cash = 1e12; G.buyVenue(s, 'prep');
  // and back again: the Parking Lot is as it was left, the Docks keep what was just bought
  assert.equal(G.movePlace(s, 1), false, 'already there');
  assert.ok(G.movePlace(s, 0));
  assert.equal(G.place(s), lot);
  assert.deepEqual(s.venue, { stands: 3, sponsors: 4, prep: 2 });
  near(G.expectedIncome(s), lotIncome, 1e-9);
  assert.equal(G.venueCost(s, 'stands'), Math.ceil(D.VENUE[0].cost * lot.scale * Math.pow(D.VENUE[0].growth, 3)));
  assert.deepEqual(s.levels[1], { stands: 0, sponsors: 0, prep: 1 });
  assert.ok(G.modelUnlocked(s, next.cars[0]), 'what is on sale follows the best venue owned, not the one the team is at');
  // saved and loaded
  G.movePlace(s, 1);
  const back = G.load(G.serialize(s), 0);
  assert.deepEqual([back.place, back.placeOwned, back.levels], [1, 1, s.levels]);
  assert.equal(back.venue, back.levels[1]);
  // a save from before venues were kept: only the current venue's levels were stored
  const old = JSON.parse(G.serialize(s));
  delete old.levels; old.venue = { stands: 2, sponsors: 1, prep: 1 }; old.street = { runs: 4, cred: 77, bestScore: 900, bestDist: 450 };
  const conv = G.load(old, 0);
  assert.deepEqual(conv.levels, [{ stands: 0, sponsors: 0, prep: 0 }, { stands: 2, sponsors: 1, prep: 1 }]);
  assert.deepEqual(conv.street, { runs: 4, cred: 77, best: { pass: { score: 900, dist: 450 } } });
  // with every venue owned there is nothing more to buy
  own(s, LAST); s.cash = 1e30;
  assert.equal(G.nextPlace(s), null);
  assert.equal(G.buyPlace(s), false);
});

/* ---------- Tap bubbles ---------- */

test('bubbles: the gap is inside its range, kinds come by weight, and a running effect does not come again', () => {
  const rng = G.seeded(3), s = G.newState(0), counts = {};
  for (let i = 0; i < 500; i++) { const g = G.bubbleGap(rng); assert.ok(g >= D.BUBBLE_GAP[0] && g <= D.BUBBLE_GAP[1]); }
  for (let i = 0; i < 6000; i++) { const id = G.rollBubble(s, rng); counts[id] = (counts[id] || 0) + 1; }
  const total = D.BUBBLES.reduce((t, b) => t + b.weight, 0);
  for (const b of D.BUBBLES) assert.ok(Math.abs(counts[b.id] / 6000 - b.weight / total) < 0.03, b.id);
  assert.ok(counts.meet < counts.wave && counts.wave < counts.perfect, 'the meet is the rare one');
  G.popBubble(s, 'meet');
  for (let i = 0; i < 500; i++) assert.notEqual(G.rollBubble(s, rng), 'meet');
});

test('bubbles: a perfect drift pays cash at once; a wave and a meet lift the venue for a while and then end', () => {
  const s = G.newState(0), car = s.cars[0];
  for (const p of D.PARTS) car.parts[p.id] = 3;   // more style than the crowd can cheer
  const base = G.incomeRate(s), income = G.expectedIncome(s), cash = s.cash;
  assert.ok(base.full);
  assert.deepEqual(G.boost(s), { crowd: 1, cash: 1, train: false });
  near(G.popBubble(s, 'perfect'), G.bubble('perfect').cash * income);
  near(s.cash - cash, G.bubble('perfect').cash * income);
  assert.deepEqual(s.fx, []);
  const wave = G.bubble('wave'), meet = G.bubble('meet');
  assert.equal(G.popBubble(s, 'wave'), 0);
  near(G.incomeRate(s).cap, base.cap * wave.crowd);
  near(G.incomeRate(s).cash, Math.min(base.style, base.cap * wave.crowd) * G.cashMult(s) * wave.boost);
  assert.equal(G.boost(s).train, false);
  G.popBubble(s, 'meet');
  assert.deepEqual(G.boost(s), { crowd: wave.crowd * meet.crowd, cash: wave.boost * meet.boost, train: true });
  near(G.expectedIncome(s), income, 1e-9);   // away earnings and Street Drifting payouts ignore the effects
  assert.deepEqual(G.tickFx(s, wave.time - 1), []);
  near(G.fxLeft(s, 'wave'), 1);
  G.popBubble(s, 'wave');
  near(G.fxLeft(s, 'wave'), wave.time, 1e-9);   // tapping again restarts the clock; it does not stack
  assert.equal(s.fx.length, 2);
  assert.deepEqual(G.tickFx(s, wave.time + 0.5), ['wave']);   // the meet has half a second left
  assert.ok(G.boost(s).train);
  assert.deepEqual(G.tickFx(s, 1), ['meet']);
  assert.equal(G.boost(s).train, false);
  assert.deepEqual(s.fx, []);
  near(G.incomeRate(s).cash, base.cash);
  // effects are not carried across a reload
  G.popBubble(s, 'meet');
  assert.deepEqual(G.load(G.serialize(s), 0).fx, []);
});

/* ---------- Track layouts and Track drifting ---------- */
const TR = globalThis.DT.tracks;

test('track layouts: every venue has one; it closes, fits the lot, and keeps its distance from itself', () => {
  for (const v of D.VENUES) assert.ok(TR.LAYOUTS[v.id], v.id + ' has a layout');
  for (const id of TR.ids) {
    const t = TR.trace(id), st = TR.LAYOUTS[id].start;
    assert.ok(Math.hypot(t.end.x - st[0], t.end.z - st[1]) < 1e-6, id + ' ends where it starts');
    near(t.end.th, 2 * Math.PI, 1e-9);   // one full clockwise turn
    const s = TR.sample(id, 500).pts, n = s.length;
    let tight = Infinity, turns = 0;
    for (let i = 0; i < n; i++) {
      assert.ok(Math.abs(s[i].x) <= 14 && Math.abs(s[i].z) <= 30, id + ' stays on the asphalt');
      if (s[i].k) tight = Math.min(tight, 1 / Math.abs(s[i].k));
      if (Math.sign(s[i].k) !== Math.sign(s[(i + 1) % n].k) && s[(i + 1) % n].k !== 0) turns++;
      for (let j = i + 1; j < n; j++) {
        const along = Math.min(j - i, n - (j - i)) * t.length / n;
        if (along >= 16) assert.ok(Math.hypot(s[i].x - s[j].x, s[i].z - s[j].z) >= 6.5, id + ' sections stay apart');
      }
    }
    assert.ok(tight >= 4.2, id + ' has no corner tighter than a car can drive');
    assert.ok(s.some((p) => p.k > 0) && s.some((p) => p.k < 0), id + ' turns both ways');
    assert.ok(turns >= 8, id + ' has plenty of corners: ' + turns);
  }
});

test('track drifting: a venue track is an endless lapped road, and test drivers get round it', () => {
  const { s, car, setup } = stock();
  for (const id of TR.ids) {
    const road = C.trackRoad(id), lap = road.loop.lap;
    near(lap * road.ds, TR.trace(id).length * D.TRACK_SCALE, 1e-6);
    C.extendRoad(road, lap * 3 + 50);
    for (let i = 0; i < lap + 50; i++) {   // the next lap is the same place, one full turn on
      const a = road.pts[i], b = road.pts[i + lap];
      assert.ok(a.x === b.x && a.z === b.z && a.k === b.k);
      near(a.h - b.h, 2 * Math.PI, 1e-9);
    }
    for (let i = 1; i < lap * 2; i++) {    // evenly spaced, no jumps, heading matches the direction of travel
      const a = road.pts[i - 1], b = road.pts[i], d = Math.hypot(b.x - a.x, b.z - a.z);
      assert.ok(Math.abs(d - road.ds) < 0.02, id + ' step ' + i);
      assert.ok(Math.abs(C.wrapPi(Math.atan2(b.x - a.x, b.z - a.z) - a.h)) < 0.08, id + ' heading ' + i);
    }
    assert.ok(Math.max(...road.loop.base.map((p) => Math.abs(p.k))) < 1 / 20, id + ' has no bend tighter than about 23 m to drive');
    const run = go(road, setup);
    assert.equal(C.lapsDone(run), 0);
    drive(run, (r) => C.autopilot(r), 400);
    console.log(`   ${id.padEnd(8)} lap ${lap} m: the road-following driver did ${C.lapsDone(run)} laps, ${(run.dist / 1000).toFixed(1)} km`);
    assert.ok(C.lapsDone(run) >= 2, id + ' can be lapped');
    assert.equal(C.payout(s, run).laps, C.lapsDone(run));
    run.map = 'track:' + id;
    C.collect(s, car, run);
    assert.ok(s.street.best['track:' + id].dist > 0);
  }
});
