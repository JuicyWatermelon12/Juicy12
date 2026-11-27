/* Run with: node --test tests/game.test.js */
const test = require('node:test');
const assert = require('node:assert/strict');
require('../src/data.js');
require('../src/game.js');
const { data: D, game: G } = globalThis.DT;

const rich = () => { const s = G.newState(1000); s.cash = 1e18; return s; };
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
  assert.equal(car.parts.tires, D.PART_MAX);
  s.cash = 0;
  assert.equal(G.buyPart(s, car.id, 'stance'), false);
  assert.equal(car.parts.stance, 0);
  // a whole car is at most 6 x PART_MAX purchases
  assert.equal(D.PARTS.length * D.PART_MAX, 30);
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
  const car = s.cars[0];
  for (const p of D.PARTS) for (let i = 0; i < 3; i++) G.buyPart(s, car.id, p.id);
  let inc = G.incomeRate(s);
  assert.ok(inc.full, 'style is above the crowd capacity');
  assert.equal(inc.cheered, D.CROWD_BASE);
  assert.equal(inc.cash, D.CROWD_BASE);
  G.buyVenue(s, 'stands');
  near(G.crowdCap(s), D.CROWD_BASE * D.CROWD_GROWTH);
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
  assert.equal(s.slots, G.MAX_SLOTS);
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
require('../src/tougecore.js');
const C = globalThis.DT.touge, T = D.TOUGE;
const HZ = 120;
const drive = (run, steerFn, seconds) => {
  const ev = [];
  for (let i = 0; i < seconds * HZ && !run.done; i++) ev.push(...C.step(run, steerFn(run), 1 / HZ));
  return ev;
};
const straight = (halfW) => ({ pts: Array.from({ length: 40000 }, (_, i) => ({ x: 0, z: i, h: 0, k: 0 })), ds: 1, n: 40000, halfW, gen: null });
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
      assert.ok(Math.abs(r.pts[i].k) <= 1 / T.RADIUS_MIN[1] + 1e-9);
    }
    assert.ok(r.pts.slice(0, T.START_STRAIGHT - 6).every((p) => p.k === 0), 'it starts with a straight');
  }
});

test('street road: bends get tighter and straights shorter over the first kilometres, then hold', () => {
  assert.equal(C.difficulty(0), 0);
  assert.equal(C.difficulty(T.RAMP_KM * 1000), 1);
  assert.equal(C.difficulty(T.RAMP_KM * 5000), 1);
  let early = 0, late = 0, later = 0;
  const bend = (pts) => pts.reduce((t, p) => t + Math.abs(p.k), 0) / pts.length;
  for (let seed = 1; seed <= 12; seed++) {
    const r = C.extendRoad(C.newRoad(G.seeded(seed)), 16000);
    early += bend(r.pts.slice(100, 1500)); late += bend(r.pts.slice(8000, 11000)); later += bend(r.pts.slice(13000, 16000));
  }
  assert.ok(late > early * 1.25, 'more turning per metre later on: ' + (late / early).toFixed(2));
  assert.ok(Math.abs(later / late - 1) < 0.1, 'and no harder after the ramp');
  assert.ok(T.RADIUS_MIN[1] < T.RADIUS_MIN[0] && T.STRAIGHT_MAX[1] < T.STRAIGHT_MAX[0]);
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

test('street distance: each kilometre raises the multiplier cap and the speed; a kilometre with no touches pays a bonus', () => {
  const { setup } = stock();
  let run = go(straight(500), setup);
  assert.equal(C.multCap(run), T.COMBO_MAX);
  let ev = drive(run, (r) => Math.cos(r.t * 3.2) * 0.6, 200);
  const kms = ev.filter((e) => e === 'km').length;
  assert.ok(kms >= 2 && run.km === kms && run.dist >= kms * 1000);
  assert.equal(C.multCap(run), T.COMBO_MAX + kms * T.COMBO_MAX_PER_KM);
  assert.equal(run.cleanKm, kms, 'no touches, so every marker paid');
  assert.ok(run.cleanBonus > 0);
  assert.ok(run.bestMult > T.COMBO_MAX, 'the multiplier went past the first cap: x' + run.bestMult.toFixed(1));
  // the bonus is CLEAN_KM_BONUS of the points scored in that kilometre
  run = go(straight(500), setup);
  let before = 0;
  for (let i = 0; i < HZ * 200 && run.km < 1; i++) { before = run.score; C.step(run, Math.cos(run.t * 3.2) * 0.6, 1 / HZ); }
  assert.ok(Math.abs(run.lastKmBonus / before - T.CLEAN_KM_BONUS) < 0.01);
  // after a touch, markers pay nothing
  run = go(straight(T.HALF_WIDTH), setup);
  drive(run, () => 1, 4);
  assert.equal(run.hits, 1);
  ev = drive(run, (r) => C.drifter(r, 0.5), 100);
  assert.ok(run.km >= 1 && run.cleanKm === 0 && run.cleanBonus === 0);
  // top speed: +SPEED_PER_KM a kilometre, up to the cap
  const fast = go(straight(500), setup);
  drive(fast, () => 0, 30);
  const v0 = fast.speed;
  drive(fast, () => 0, 700);
  assert.ok(fast.dist > T.SPEED_RAMP_MAX / T.SPEED_PER_KM * 1000);
  assert.ok(Math.abs(fast.speed / setup.top - (1 + T.SPEED_RAMP_MAX)) < 0.01 && v0 < fast.speed);
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
      assert.ok(run.done && run.hits === T.MAX_HITS, 'every run ends on the third touch');
      t += run.t; score += run.score; dist += run.dist;
    }
    rate[build] = score / t;
    console.log(`   ${D.PART_STAGES[build].padEnd(6)} car: lasts ${(t / 10).toFixed(0)} s, ${(dist / 10000).toFixed(1)} km, ${(score / 10).toFixed(0)} points, ${rate[build].toFixed(0)} points a second (averages over 10 roads)`);
    assert.ok(dist / 10 > 1000, 'the road is drivable');
  }
  assert.ok(rate[3] > rate[0] * 1.3 && rate[5] > rate[3], 'upgrades pay on the street');
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
  near(res.seconds, 7000 / T.POINTS_PER_SECOND);
  near(res.cash, income * 7000 / T.POINTS_PER_SECOND);
  assert.ok(res.out);
  const cash = s.cash, xp = G.totalXP(d);
  const got = C.collect(s, car, run);
  near(s.cash - cash, got.cash);
  near(G.totalXP(d) - xp, 50 * T.XP_MULT);
  assert.equal(got.levels, d.level - 1);
  assert.ok(got.bestScore && got.bestDist);
  assert.deepEqual(s.street, { runs: 1, bestScore: 7000, bestDist: 1234 });
  run.score = 5000; run.dist = 2000;
  const next = C.collect(s, car, run);
  assert.ok(!next.bestScore && next.bestDist);
  assert.deepEqual(s.street, { runs: 2, bestScore: 7000, bestDist: 2000 });
  assert.deepEqual(G.load(G.serialize(s), 0).street, s.street);
});

test('street pays better than waiting: a test driver in a stock car earns several times the venue over the same time', () => {
  const { s, car, setup } = stock();
  let seconds = 0, time = 0;
  for (let seed = 1; seed <= 10; seed++) {
    const run = go(C.newRoad(G.seeded(seed)), setup);
    drive(run, (r) => C.drifter(r, 0.7), 90);   // the first minute and a half, where most real runs will end
    seconds += C.payout(s, run).seconds; time += run.t;
  }
  const ratio = seconds / time;
  console.log(`   a stock-car run is worth ${ratio.toFixed(1)} times the venue's income over the same time`);
  assert.ok(ratio > 2.5 && ratio < 8);
});
