/* Drift Tycoon — game rules. Pure logic: no DOM, no Three.js, so it runs in Node tests too. */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});
  const D = DT.data;
  const SAVE_VERSION = 1;
  const MAX_SLOTS = D.SLOT_COSTS.length;

  const byId = (list, id) => list.find((x) => x.id === id);
  const model = (id) => byId(D.MODELS, id);
  const part = (id) => byId(D.PARTS, id);
  const rarity = (id) => byId(D.RARITIES, id);
  const modelIndex = (id) => D.MODELS.findIndex((m) => m.id === id);

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
      cars: [], drivers: [], scouted: 0,
      mech: { count: 1, level: 0 },
      venue: { stands: 0, sponsors: 0, prep: 0 },
      lastSeen: now || 0,
    };
    const rookie = { id: s.nextId++, name: 'Sam "Rookie" Okada', rarity: 'common', skill: 12, control: 16, flair: 10,
      helmet: '#ffffff' };
    s.drivers.push(rookie);
    addCar(s, 'kumo').driver = rookie.id;
    return s;
  }

  /* ---------- Derived numbers ---------- */

  const getCar = (state, id) => byId(state.cars, id);
  const getDriver = (state, id) => (id == null ? null : byId(state.drivers, id) || null);
  const driverOf = (state, car) => getDriver(state, car.driver);
  const carOfDriver = (state, driverId) => state.cars.find((c) => c.driver === driverId) || null;

  function partsMult(car) {
    return D.PARTS.reduce((m, p) => m * (1 + (car.parts[p.id] || 0) * p.per), 1);
  }
  const prepMult = (state) => 1 + D.PREP_PER * state.venue.prep;
  const sponsorMult = (state) => 1 + D.SPONSOR_PER * state.venue.sponsors;
  const crowdCap = (state) => D.CROWD_BASE * Math.pow(D.CROWD_GROWTH, state.venue.stands);

  function driverMult(driver) {
    return (1 + driver.skill / D.SKILL_DIV) * (1 + driver.flair / D.FLAIR_DIV);
  }

  /* Style points a second this car scores while it is drifting. 0 without a driver. */
  function carStyle(state, car) {
    const d = driverOf(state, car);
    if (!d) return 0;
    return model(car.model).base * partsMult(car) * driverMult(d) * prepMult(state);
  }

  /* Crashes a second while drifting. */
  function carCrashRate(state, car) {
    const d = driverOf(state, car);
    if (!d) return 0;
    let r = D.BASE_CRASH * (1 - d.control / D.CONTROL_DIV);
    D.PARTS.forEach((p) => (r *= Math.pow(p.crash, car.parts[p.id] || 0)));
    return r;
  }

  const repairTime = (state, level) =>
    Math.max(D.REPAIR_MIN, D.REPAIR_BASE * Math.pow(D.REPAIR_STEP, level == null ? state.mech.level : level));

  /* Share of the time a car spends drifting, given its crash rate and the repair time. */
  function carUptime(state, car) {
    const r = carCrashRate(state, car);
    return 1 / (1 + r * (repairTime(state) + D.REJOIN_OVERHEAD));
  }

  /* Income for the cars that are drifting right now. Style above the crowd's capacity is wasted. */
  function incomeRate(state, runningIds) {
    let style = 0;
    state.cars.forEach((c) => {
      if (!runningIds || runningIds.includes(c.id)) style += carStyle(state, c);
    });
    const cap = crowdCap(state);
    const cheered = Math.min(style, cap);
    return { style, cap, cheered, cash: cheered * sponsorMult(state), full: style > cap };
  }

  /* Long-run income, allowing for crashes. Used for away earnings. */
  function expectedIncome(state) {
    let style = 0;
    state.cars.forEach((c) => (style += carStyle(state, c) * carUptime(state, c)));
    return Math.min(style, crowdCap(state)) * sponsorMult(state);
  }

  function offlineEarnings(state, now) {
    const seconds = Math.max(0, (now - (state.lastSeen || now)) / 1000);
    if (seconds < D.OFFLINE_MIN) return { seconds, paidSeconds: 0, cash: 0 };
    const paidSeconds = Math.min(seconds, D.OFFLINE_CAP);
    return { seconds, paidSeconds, cash: expectedIncome(state) * paidSeconds };
  }

  /* ---------- Costs ---------- */

  function partCost(car, partId) {
    const p = part(partId);
    return Math.ceil(p.cost * (model(car.model).base / 2) * Math.pow(D.PART_GROWTH, car.parts[partId] || 0));
  }
  const slotCost = (state) => (state.slots >= MAX_SLOTS ? Infinity : D.SLOT_COSTS[state.slots]);
  function venueCost(state, id) {
    const v = byId(D.VENUE, id);
    return Math.ceil(v.cost * Math.pow(v.growth, state.venue[id]));
  }
  const scoutCost = (state) => Math.ceil(D.SCOUT_COST * Math.pow(D.SCOUT_GROWTH, state.scouted));
  const mechHireCost = (state) =>
    state.mech.count >= D.MECH_MAX ? Infinity : Math.ceil(D.MECH_HIRE * Math.pow(D.MECH_HIRE_GROWTH, state.mech.count - 1));
  const mechTrainCost = (state) => Math.ceil(D.MECH_TRAIN * Math.pow(D.MECH_TRAIN_GROWTH, state.mech.level));
  const mechTrainMaxed = (state) => repairTime(state) <= D.REPAIR_MIN;

  /* ---------- Actions. Each returns true when it happened. ---------- */

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

  const benchDrivers = (state) => state.drivers.filter((d) => !carOfDriver(state, d.id));

  function buyCar(state, modelId) {
    const m = model(modelId);
    if (!m || state.cars.length >= state.slots || !spend(state, m.cost)) return false;
    const car = addCar(state, modelId);
    const bench = benchDrivers(state).sort((a, b) => b.skill - a.skill)[0];
    if (bench) car.driver = bench.id;
    return true;
  }

  /* Swap a car for a better model. Parts keep half their levels (rounded down). */
  function tradeCar(state, carId, modelId) {
    const car = getCar(state, carId);
    const m = model(modelId);
    if (!car || !m || modelIndex(modelId) <= modelIndex(car.model) || !spend(state, m.cost)) return false;
    car.model = modelId;
    D.PARTS.forEach((p) => (car.parts[p.id] = Math.floor((car.parts[p.id] || 0) / 2)));
    return true;
  }

  function buyVenue(state, id) {
    if (!byId(D.VENUE, id) || !spend(state, venueCost(state, id))) return false;
    state.venue[id]++;
    return true;
  }

  function rollRarity(rng) {
    const total = D.RARITIES.reduce((t, r) => t + r.weight, 0);
    let x = rng() * total;
    for (const r of D.RARITIES) {
      if ((x -= r.weight) < 0) return r;
    }
    return D.RARITIES[0];
  }

  function rollDriver(state, rng) {
    rng = rng || Math.random;
    const r = rollRarity(rng);
    const stat = () => Math.round(r.min + rng() * (r.max - r.min));
    const pick = (list) => list[Math.floor(rng() * list.length)];
    let name = '';
    for (let tries = 0; tries < 30; tries++) {
      const nick = rng() < 0.4 ? ' "' + pick(D.NICKNAMES) + '" ' : ' ';
      name = pick(D.FIRST_NAMES) + nick + pick(D.LAST_NAMES);
      if (!state.drivers.some((d) => d.name === name)) break;
    }
    return { id: state.nextId++, name, rarity: r.id, skill: stat(), control: stat(), flair: stat(), helmet: pick(D.HELMETS) };
  }

  /* Pay for a random driver. They take the first car with an empty seat, else the bench. */
  function scout(state, rng) {
    if (!spend(state, scoutCost(state))) return null;
    state.scouted++;
    const d = rollDriver(state, rng);
    state.drivers.push(d);
    const empty = state.cars.find((c) => c.driver == null);
    if (empty) empty.driver = d.id;
    return d;
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

  function load(json, now) {
    let raw;
    try { raw = typeof json === 'string' ? JSON.parse(json) : json; } catch (e) { raw = null; }
    if (!raw || typeof raw !== 'object' || !Array.isArray(raw.cars) || !Array.isArray(raw.drivers)) return newState(now);
    const s = newState(now);
    const num = (v, d) => (typeof v === 'number' && isFinite(v) && v >= 0 ? v : d);
    s.cash = num(raw.cash, s.cash);
    s.totalEarned = num(raw.totalEarned, 0);
    s.slots = Math.min(MAX_SLOTS, Math.max(1, Math.floor(num(raw.slots, 1))));
    s.scouted = Math.floor(num(raw.scouted, 0));
    s.lastSeen = num(raw.lastSeen, now || 0);
    s.mech.count = Math.min(D.MECH_MAX, Math.max(1, Math.floor(num(raw.mech && raw.mech.count, 1))));
    s.mech.level = Math.floor(num(raw.mech && raw.mech.level, 0));
    D.VENUE.forEach((v) => (s.venue[v.id] = Math.floor(num(raw.venue && raw.venue[v.id], 0))));
    s.drivers = raw.drivers.filter((d) => d && rarity(d.rarity) && typeof d.name === 'string').map((d) => ({
      id: d.id, name: d.name, rarity: d.rarity, skill: num(d.skill, 10), control: num(d.control, 10),
      flair: num(d.flair, 10), helmet: d.helmet || '#ffffff',
    }));
    s.cars = raw.cars.filter((c) => c && model(c.model)).slice(0, s.slots).map((c, i) => {
      const parts = emptyParts();
      D.PARTS.forEach((p) => (parts[p.id] = Math.min(D.PART_MAX, Math.floor(num(c.parts && c.parts[p.id], 0)))));
      const driver = s.drivers.some((d) => d.id === c.driver) ? c.driver : null;
      return { id: c.id, model: c.model, parts, driver, color: c.color || D.CAR_COLORS[i % D.CAR_COLORS.length] };
    });
    const maxId = Math.max(0, ...s.cars.map((c) => c.id || 0), ...s.drivers.map((d) => d.id || 0));
    s.nextId = Math.max(Math.floor(num(raw.nextId, 1)), maxId + 1);
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

  DT.game = {
    SAVE_VERSION, MAX_SLOTS, seeded, model, part, rarity, modelIndex, newState, load, serialize,
    getCar, getDriver, driverOf, carOfDriver, benchDrivers,
    partsMult, prepMult, sponsorMult, crowdCap, driverMult, carStyle, carCrashRate, repairTime, carUptime,
    incomeRate, expectedIncome, offlineEarnings,
    partCost, slotCost, venueCost, scoutCost, mechHireCost, mechTrainCost, mechTrainMaxed,
    earn, buyPart, buySlot, buyCar, tradeCar, buyVenue, rollDriver, scout, assignDriver, hireMech, trainMech,
    fmt, money, duration,
  };
})(typeof window !== 'undefined' ? window : globalThis);
