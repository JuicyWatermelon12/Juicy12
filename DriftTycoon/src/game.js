/* Drift Tycoon — game rules. Pure logic: no DOM, no Three.js, so it runs in Node tests too. */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});
  const D = DT.data;
  const SAVE_VERSION = 3;
  const MAX_SLOTS = D.SLOT_COSTS.length;

  const byId = (list, id) => list.find((x) => x.id === id);
  const model = (id) => byId(D.MODELS, id);
  const part = (id) => byId(D.PARTS, id);
  const rarity = (id) => byId(D.RARITIES, id);
  const modelIndex = (id) => D.MODELS.findIndex((m) => m.id === id);
  const NO_TRAIT = {};
  const trait = (driver) => (driver && driver.trait && byId(D.TRAITS, driver.trait)) || NO_TRAIT;

  /* Small seedable generator, for tests and for anything that should replay. */
  function seeded(seed) {
    let a = seed >>> 0;
    return function () {
      a = (a + 0x6d2b79f5) >>> 0;
      let t = a;
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  function emptyParts() {
    const p = {};
    D.PARTS.forEach((x) => (p[x.id] = 0));
    return p;
  }

  function addCar(state, modelId) {
    const used = state.cars.map((c) => c.color);
    const color = D.CAR_COLORS.find((c) => !used.includes(c)) || D.CAR_COLORS[state.cars.length % D.CAR_COLORS.length];
    const car = { id: state.nextId++, model: modelId, parts: emptyParts(), driver: null, color };
    state.cars.push(car);
    return car;
  }

  function newState(now) {
    const s = {
      v: SAVE_VERSION, cash: D.START_CASH, totalEarned: 0, slots: 1, nextId: 1,
      cars: [], drivers: [],
      tryouts: 0, nextTryoutAt: 0, prospects: null,
      mech: { count: 1, level: 0 },
      venue: { stands: 0, sponsors: 0, prep: 0 },
      street: { runs: 0, bestScore: 0, bestDist: 0 },   // Street Drifting records
      lastSeen: now || 0,
    };
    const rookie = { id: s.nextId++, name: 'Sam "Rookie" Okada', rarity: 'common', skill: 12, control: 16, flair: 10,
      helmet: '#ffffff', level: 1, xp: 0, trait: null };
    s.drivers.push(rookie);
    addCar(s, 'kumo').driver = rookie.id;
    return s;
  }

  /* ---------- Drivers ---------- */

  const getCar = (state, id) => byId(state.cars, id);
  const getDriver = (state, id) => (id == null ? null : byId(state.drivers, id) || null);
  const driverOf = (state, car) => getDriver(state, car.driver);
  const carOfDriver = (state, driverId) => state.cars.find((c) => c.driver === driverId) || null;
  const benchDrivers = (state) => state.drivers.filter((d) => !carOfDriver(state, d.id));
  const seatedDrivers = (state) => state.cars.map((c) => driverOf(state, c)).filter(Boolean);

  /* Stats as they are today: the rolled base stats grown by the driver's level. */
  function driverStats(d) {
    const m = 1 + D.LEVEL_STAT * (d.level - 1);
    return { skill: Math.round(d.skill * m), control: Math.round(d.control * m), flair: Math.round(d.flair * m) };
  }
  function driverMult(d) {
    const s = driverStats(d);
    return (1 + s.skill / D.SKILL_DIV) * (1 + s.flair / D.FLAIR_DIV);
  }
  const levelCap = (d) => rarity(d.rarity).cap;
  const xpForLevel = (level) => D.XP_BASE * Math.pow(D.XP_GROWTH, level - 1);   // from `level` to the next
  const xpToNext = (d) => (d.level >= levelCap(d) ? 0 : xpForLevel(d.level));
  function totalXP(d) {
    let t = d.xp;
    for (let l = 1; l < d.level; l++) t += xpForLevel(l);
    return t;
  }
  /* Returns how many levels were gained. Experience past the cap is dropped. */
  function addXP(d, amount) {
    if (!(amount > 0)) return 0;
    let gained = 0;
    d.xp += amount;
    while (d.level < levelCap(d) && d.xp >= xpForLevel(d.level)) { d.xp -= xpForLevel(d.level); d.level++; gained++; }
    if (d.level >= levelCap(d)) d.xp = 0;
    return gained;
  }

  /* ---------- Derived numbers ---------- */

  function partsMult(car) {
    return D.PARTS.reduce((m, p) => m * (1 + (car.parts[p.id] || 0) * p.per), 1);
  }
  const prepMult = (state) => Math.pow(D.PREP_MULT, state.venue.prep);
  const sponsorMult = (state) => Math.pow(D.SPONSOR_MULT, state.venue.sponsors);
  const traitSum = (state, key) => seatedDrivers(state).reduce((t, d) => t + (trait(d)[key] || 0), 0);
  const cashMult = (state) => sponsorMult(state) * (1 + traitSum(state, 'cash'));
  const crowdCap = (state) => D.CROWD_BASE * Math.pow(D.CROWD_GROWTH, state.venue.stands) * (1 + traitSum(state, 'crowd'));

  /* Style points a second this car scores while it is drifting. 0 without a driver. */
  function carStyle(state, car) {
    const d = driverOf(state, car);
    if (!d) return 0;
    const t = trait(d);
    let v = model(car.model).base * partsMult(car) * driverMult(d) * prepMult(state) * (t.style || 1);
    if (t.perCar) v *= 1 + t.perCar * (seatedDrivers(state).length - 1);
    return v;
  }

  /* Crashes a second while drifting. */
  function carCrashRate(state, car) {
    const d = driverOf(state, car);
    if (!d) return 0;
    let r = D.BASE_CRASH / (1 + driverStats(d).control / D.CONTROL_HALF) * (trait(d).crash || 1);
    D.PARTS.forEach((p) => (r *= Math.pow(p.crash, car.parts[p.id] || 0)));
    return r;
  }

  const repairTime = (state, level) =>
    Math.max(D.REPAIR_MIN, D.REPAIR_BASE * Math.pow(D.REPAIR_STEP, level == null ? state.mech.level : level));
  const carRepairTime = (state, car) => repairTime(state) * (trait(driverOf(state, car)).repair || 1);

  /* Share of the time a car spends drifting, given its crash rate and the repair time. */
  function carUptime(state, car) {
    const r = carCrashRate(state, car);
    return 1 / (1 + r * (carRepairTime(state, car) + D.REJOIN_OVERHEAD));
  }

  /* Income for the cars that are drifting right now. Style above the crowd's capacity is wasted. */
  function incomeRate(state, runningIds) {
    let style = 0;
    state.cars.forEach((c) => {
      if (!runningIds || runningIds.includes(c.id)) style += carStyle(state, c);
    });
    const cap = crowdCap(state);
    const cheered = Math.min(style, cap);
    return { style, cap, cheered, cash: cheered * cashMult(state), full: style > cap };
  }

  /* Long-run income, allowing for crashes. Used for away earnings. */
  function expectedIncome(state) {
    let style = 0;
    state.cars.forEach((c) => (style += carStyle(state, c) * carUptime(state, c)));
    return Math.min(style, crowdCap(state)) * cashMult(state);
  }

  function offlineEarnings(state, now) {
    const seconds = Math.max(0, (now - (state.lastSeen || now)) / 1000);
    if (seconds < D.OFFLINE_MIN) return { seconds, paidSeconds: 0, cash: 0 };
    const paidSeconds = Math.min(seconds, D.OFFLINE_CAP);
    return { seconds, paidSeconds, cash: expectedIncome(state) * paidSeconds };
  }

  /* Seat time for the drivers whose cars are drifting. Returns the drivers who levelled up. */
  function tickXP(state, runningIds, dt) {
    const ups = [];
    state.cars.forEach((c) => {
      if (runningIds && !runningIds.includes(c.id)) return;
      const d = driverOf(state, c);
      if (d && addXP(d, dt * (trait(d).xp || 1))) ups.push(d);
    });
    return ups;
  }
  /* Seat time for a stretch nobody watched: each seated driver drifts for their car's uptime. */
  function awayXP(state, seconds) {
    const ups = [];
    state.cars.forEach((c) => {
      const d = driverOf(state, c);
      if (d && addXP(d, seconds * carUptime(state, c) * (trait(d).xp || 1))) ups.push(d);
    });
    return ups;
  }

  /* ---------- Costs ---------- */

  function partCost(car, partId) {
    const p = part(partId);
    return Math.ceil(p.cost * Math.pow(D.PART_GROWTH, car.parts[partId] || 0));
  }
  const slotCost = (state) => (state.slots >= MAX_SLOTS ? Infinity : D.SLOT_COSTS[state.slots]);
  function venueCost(state, id) {
    const v = byId(D.VENUE, id);
    return Math.ceil(v.cost * Math.pow(v.growth, state.venue[id]));
  }
  const tryoutCost = (state) => Math.ceil(D.TRYOUT_COST * Math.pow(D.TRYOUT_GROWTH, state.tryouts));
  const mechHireCost = (state) =>
    state.mech.count >= D.MECH_MAX ? Infinity : Math.ceil(D.MECH_HIRE * Math.pow(D.MECH_HIRE_GROWTH, state.mech.count - 1));
  const mechTrainCost = (state) => Math.ceil(D.MECH_TRAIN * Math.pow(D.MECH_TRAIN_GROWTH, state.mech.level));
  const mechTrainMaxed = (state) => repairTime(state) <= D.REPAIR_MIN;

  /* ---------- Actions. Each returns true (or a result) when it happened. ---------- */

  function spend(state, cost) {
    if (!(cost >= 0) || !isFinite(cost) || state.cash < cost) return false;
    state.cash -= cost;
    return true;
  }
  function earn(state, amount) {
    if (!(amount > 0)) return;
    state.cash += amount;
    state.totalEarned += amount;
  }

  function buyPart(state, carId, partId) {
    const car = getCar(state, carId);
    if (!car || !part(partId) || car.parts[partId] >= D.PART_MAX) return false;
    if (!spend(state, partCost(car, partId))) return false;
    car.parts[partId]++;
    return true;
  }

  function buySlot(state) {
    if (state.slots >= MAX_SLOTS || !spend(state, slotCost(state))) return false;
    state.slots++;
    return true;
  }

  function buyCar(state, modelId) {
    const m = model(modelId);
    if (!m || state.cars.length >= state.slots || !spend(state, m.cost)) return false;
    const car = addCar(state, modelId);
    const bench = benchDrivers(state).sort((a, b) => driverMult(b) - driverMult(a))[0];
    if (bench) car.driver = bench.id;
    return true;
  }

  /* Swap a car for a better model. Its parts and driver move across. */
  function tradeCar(state, carId, modelId) {
    const car = getCar(state, carId);
    const m = model(modelId);
    if (!car || !m || modelIndex(modelId) <= modelIndex(car.model) || !spend(state, m.cost)) return false;
    car.model = modelId;
    return true;
  }

  function buyVenue(state, id) {
    if (!byId(D.VENUE, id) || !spend(state, venueCost(state, id))) return false;
    state.venue[id]++;
    return true;
  }

  /* ---------- Tryouts ---------- */

  function rollRarity(rng) {
    const total = D.RARITIES.reduce((t, r) => t + r.weight, 0);
    let x = rng() * total;
    for (const r of D.RARITIES) {
      if ((x -= r.weight) < 0) return r;
    }
    return D.RARITIES[0];
  }

  function rollDriver(state, rng, taken) {
    rng = rng || Math.random;
    const r = rollRarity(rng);
    const stat = () => Math.round(r.min + rng() * (r.max - r.min));
    const pick = (list) => list[Math.floor(rng() * list.length)];
    const names = state.drivers.map((d) => d.name).concat(taken || []);
    let name = '';
    for (let tries = 0; tries < 30; tries++) {
      const nick = rng() < 0.4 ? ' "' + pick(D.NICKNAMES) + '" ' : ' ';
      name = pick(D.FIRST_NAMES) + nick + pick(D.LAST_NAMES);
      if (!names.includes(name)) break;
    }
    const d = { id: state.nextId++, name, rarity: r.id, skill: stat(), control: stat(), flair: stat(), helmet: pick(D.HELMETS),
      level: 1, xp: 0, trait: null };
    if (rng() < r.trait) d.trait = pick(D.TRAITS).id;
    return d;
  }

  const rosterCap = (state) => state.slots + D.RESERVE_SEATS;
  const rosterFull = (state) => state.drivers.length >= rosterCap(state);
  /* A car with nobody to drive it: prospects come at once. */
  const needsDriver = (state) => state.cars.some((c) => c.driver == null) && benchDrivers(state).length === 0;
  /* Seconds until the next tryout can be held. 0 = ready (or prospects are already waiting). */
  function tryoutWait(state, now) {
    if (state.prospects || needsDriver(state)) return 0;
    return Math.max(0, (state.nextTryoutAt - now) / 1000);
  }

  /* Pay for a tryout: prospects show up and wait until one is signed or they are sent home. */
  function holdTryout(state, now, rng) {
    if (state.prospects || tryoutWait(state, now) > 0 || !spend(state, tryoutCost(state))) return false;
    state.tryouts++;
    state.prospects = [];
    for (let i = 0; i < D.TRYOUT_SIZE; i++) state.prospects.push(rollDriver(state, rng, state.prospects.map((p) => p.name)));
    state.nextTryoutAt = now + D.TRYOUT_COOLDOWN * 1000;
    return true;
  }

  /* What a prospect would look like after being mentored by `retiree` (no changes made). */
  function mentored(prospect, retiree) {
    const copy = Object.assign({}, prospect);
    if (retiree) addXP(copy, totalXP(retiree) * D.MENTOR_SHARE);
    return copy;
  }

  /* Sign one prospect; the others go home. With a full roster somebody has to make way: they retire,
     pass on part of their experience to the newcomer, and the newcomer takes their seat. */
  function signProspect(state, index, retireId) {
    const p = state.prospects && state.prospects[index];
    if (!p) return null;
    const retiree = getDriver(state, retireId);
    if (rosterFull(state) && !retiree) return null;
    const signed = mentored(p, retiree);
    if (retiree) {
      const seat = carOfDriver(state, retiree.id);
      state.drivers = state.drivers.filter((d) => d !== retiree);
      if (seat) seat.driver = signed.id;
    }
    state.drivers.push(signed);
    if (!carOfDriver(state, signed.id)) {
      const empty = state.cars.find((c) => c.driver == null);
      if (empty) empty.driver = signed.id;
    }
    state.prospects = null;
    return { driver: signed, retired: retiree, levels: signed.level - p.level };
  }

  function dismissProspects(state) {
    if (!state.prospects) return false;
    state.prospects = null;
    return true;
  }

  /* Put a driver in a car. If they were driving another car, the two drivers swap seats. */
  function assignDriver(state, carId, driverId) {
    const car = getCar(state, carId);
    if (!car || !getDriver(state, driverId)) return false;
    const from = carOfDriver(state, driverId);
    if (from === car) return true;
    if (from) from.driver = car.driver;
    car.driver = driverId;
    return true;
  }

  function hireMech(state) {
    if (state.mech.count >= D.MECH_MAX || !spend(state, mechHireCost(state))) return false;
    state.mech.count++;
    return true;
  }
  function trainMech(state) {
    if (mechTrainMaxed(state) || !spend(state, mechTrainCost(state))) return false;
    state.mech.level++;
    return true;
  }

  /* ---------- Save / load ---------- */

  const serialize = (state) => JSON.stringify(state);

  function cleanDriver(d, num) {
    const out = { id: d.id, name: d.name, rarity: d.rarity, skill: num(d.skill, 10), control: num(d.control, 10),
      flair: num(d.flair, 10), helmet: d.helmet || '#ffffff', level: 1, xp: num(d.xp, 0),
      trait: byId(D.TRAITS, d.trait) ? d.trait : null };
    out.level = Math.min(levelCap(out), Math.max(1, Math.floor(num(d.level, 1))));
    return out;
  }

  function load(json, now) {
    let raw;
    try { raw = typeof json === 'string' ? JSON.parse(json) : json; } catch (e) { raw = null; }
    if (!raw || typeof raw !== 'object' || !Array.isArray(raw.cars) || !Array.isArray(raw.drivers)) return newState(now);
    if (raw.v !== SAVE_VERSION) return newState(now);   // saves from earlier prototypes are not carried over
    const s = newState(now);
    const num = (v, d) => (typeof v === 'number' && isFinite(v) && v >= 0 ? v : d);
    const isDriver = (d) => d && rarity(d.rarity) && typeof d.name === 'string';
    s.cash = num(raw.cash, s.cash);
    s.totalEarned = num(raw.totalEarned, 0);
    s.slots = Math.min(MAX_SLOTS, Math.max(1, Math.floor(num(raw.slots, 1))));
    s.lastSeen = num(raw.lastSeen, now || 0);
    s.tryouts = Math.floor(num(raw.tryouts, 0));
    s.nextTryoutAt = num(raw.nextTryoutAt, 0);
    s.mech.count = Math.min(D.MECH_MAX, Math.max(1, Math.floor(num(raw.mech && raw.mech.count, 1))));
    s.mech.level = Math.floor(num(raw.mech && raw.mech.level, 0));
    D.VENUE.forEach((v) => (s.venue[v.id] = Math.floor(num(raw.venue && raw.venue[v.id], 0))));
    Object.keys(s.street).forEach((k) => (s.street[k] = num(raw.street && raw.street[k], 0)));
    s.drivers = raw.drivers.filter(isDriver).map((d) => cleanDriver(d, num));
    s.cars = raw.cars.filter((c) => c && model(c.model)).slice(0, s.slots).map((c, i) => {
      const parts = emptyParts();
      D.PARTS.forEach((p) => {
        const lv = Math.floor(num(c.parts && c.parts[p.id], 0));
        parts[p.id] = Math.min(D.PART_MAX, lv);
      });
      const driver = s.drivers.some((d) => d.id === c.driver) ? c.driver : null;
      return { id: c.id, model: c.model, parts, driver, color: c.color || D.CAR_COLORS[i % D.CAR_COLORS.length] };
    });
    // The roster has a size now: seated drivers stay, then the best of the bench while there is room.
    const cap = rosterCap(s);
    if (s.drivers.length > cap) {
      const seated = s.drivers.filter((d) => carOfDriver(s, d.id));
      const bench = s.drivers.filter((d) => !carOfDriver(s, d.id)).sort((a, b) => driverMult(b) - driverMult(a));
      s.drivers = seated.concat(bench.slice(0, Math.max(0, cap - seated.length)));
    }
    s.prospects = Array.isArray(raw.prospects) && raw.prospects.length && raw.prospects.every(isDriver)
      ? raw.prospects.map((d) => cleanDriver(d, num)) : null;
    const ids = s.cars.map((c) => c.id || 0).concat(s.drivers.map((d) => d.id || 0), (s.prospects || []).map((d) => d.id || 0));
    s.nextId = Math.max(Math.floor(num(raw.nextId, 1)), Math.max(0, ...ids) + 1);
    return s;
  }

  /* ---------- Formatting ---------- */

  const SUFFIX = ['', 'K', 'M', 'B', 'T', 'Qa', 'Qi', 'Sx', 'Sp', 'Oc', 'No', 'Dc'];
  function fmt(n, smallDecimals) {
    if (!isFinite(n)) return '—';
    const neg = n < 0;
    n = Math.abs(n);
    let out;
    if (n < 1000) {
      const dec = smallDecimals && n < 100 ? smallDecimals : 0;
      out = dec ? n.toFixed(dec) : String(Math.floor(n + 1e-9));
    } else {
      let i = Math.min(SUFFIX.length - 1, Math.floor(Math.log10(n) / 3));
      let v = n / Math.pow(1000, i);
      if (v >= 999.5 && i < SUFFIX.length - 1) { v /= 1000; i++; }
      out = (v >= 100 ? v.toFixed(0) : v >= 10 ? v.toFixed(1) : v.toFixed(2)) + SUFFIX[i];
    }
    return (neg ? '-' : '') + out;
  }
  const money = (n, smallDecimals) => '$' + fmt(n, smallDecimals);
  function duration(seconds) {
    seconds = Math.floor(seconds);
    const h = Math.floor(seconds / 3600), m = Math.floor((seconds % 3600) / 60), s = seconds % 60;
    if (h) return h + ' h ' + m + ' min';
    if (m) return m + ' min ' + s + ' s';
    return s + ' s';
  }
  const clock = (seconds) => { seconds = Math.ceil(seconds); return Math.floor(seconds / 60) + ':' + String(seconds % 60).padStart(2, '0'); };

  DT.game = {
    SAVE_VERSION, MAX_SLOTS, seeded, model, part, rarity, trait, modelIndex, newState, load, serialize,
    getCar, getDriver, driverOf, carOfDriver, benchDrivers, seatedDrivers,
    driverStats, driverMult, levelCap, xpForLevel, xpToNext, totalXP, addXP, tickXP, awayXP,
    partsMult, prepMult, sponsorMult, cashMult, crowdCap, carStyle, carCrashRate, repairTime, carRepairTime, carUptime,
    incomeRate, expectedIncome, offlineEarnings,
    partCost, slotCost, venueCost, tryoutCost, mechHireCost, mechTrainCost, mechTrainMaxed,
    earn, buyPart, buySlot, buyCar, tradeCar, buyVenue, rollDriver, assignDriver, hireMech, trainMech,
    rosterCap, rosterFull, needsDriver, tryoutWait, holdTryout, mentored, signProspect, dismissProspects,
    fmt, money, duration, clock,
  };
})(typeof window !== 'undefined' ? window : globalThis);
