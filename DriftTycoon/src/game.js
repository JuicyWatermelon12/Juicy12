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

  const emptyOwned = () => ({ paint: [], rims: [], kit: [], glow: [] });
  function addCar(state, modelId) {
    const used = state.cars.map((c) => c.paint);
    const paint = D.FACTORY_PAINTS.find((p) => !used.includes(p)) || D.FACTORY_PAINTS[state.cars.length % D.FACTORY_PAINTS.length];
    // `color` is the paint's colour, kept on the car so everything that draws it has it to hand
    const car = { id: state.nextId++, model: modelId, parts: emptyParts(), driver: null, paint, color: byId(D.PAINTS, paint).color, rims: 'steel', kit: 'stock', glow: 'none',
      owned: emptyOwned() };   // the paid cosmetics bought for this car
    state.cars.push(car);
    return car;
  }

  function newState(now) {
    const s = {
      v: SAVE_VERSION, cash: D.START_CASH, totalEarned: 0, slots: 1, nextId: 1,
      cars: [], drivers: [],
      tryouts: 0, nextTryoutAt: 0, prospects: null,
      mech: { count: 1, level: 0 },
      levels: [zeroLevels()],   // upgrade levels of every venue owned, by index into D.VENUES
      venue: null,               // the levels of the venue the team is at: always the same object as levels[place]
      place: 0, placeOwned: 0,   // the venue the team is at, and the furthest venue it has bought (indexes into D.VENUES)
      street: { runs: 0, cred: 0, best: {} },   // Drift mode: runs, street cred, and the best { score, dist } on each map
      fx: [],   // tap-bubble effects running at the venue: { id, left }. Not carried across a reload
      settings: { steer: 1, testing: true },   // the player's settings: Drift steering sensitivity (1 = default), testing tools shown
      lastSeen: now || 0,
    };
    s.venue = s.levels[0];
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
  /* ---------- Venues ---------- */
  const place = (state) => D.VENUES[state.place];
  const bestPlace = (state) => D.VENUES[state.placeOwned];                    // the best venue owned: it decides what is on sale
  const nextPlace = (state) => D.VENUES[state.placeOwned + 1] || null;        // the next venue to buy
  const ownsPlace = (state, index) => index >= 0 && index <= state.placeOwned;
  /* The venue that puts a car model on sale, as an index into D.VENUES (-1: none yet). */
  const modelPlace = (modelId) => D.VENUES.findIndex((v) => v.cars.includes(modelId));
  const modelUnlocked = (state, modelId) => { const i = modelPlace(modelId); return i >= 0 && i <= state.placeOwned; };
  const maxPits = (state) => Math.min(MAX_SLOTS, bestPlace(state).pits);
  const partCap = (state) => Math.min(D.PART_MAX, bestPlace(state).stage);   // highest part stage on sale
  const levelsMaxed = (levels) => !!levels && D.VENUE.every((v) => (levels[v.id] || 0) >= v.max);
  const venueMaxed = (state) => levelsMaxed(state.venue);
  /* Upgrade levels bought and in all, for a venue's levels. */
  const venueProgress = (levels) => ({ done: D.VENUE.reduce((t, v) => t + Math.min(v.max, (levels && levels[v.id]) || 0), 0), total: D.VENUE.reduce((t, v) => t + v.max, 0) });
  /* What a venue's upgrade lines of one effect multiply together: crowd, cash or style. */
  const lineMult = (levels, effect) => D.VENUE.reduce((m, v) => (v.effect === effect ? m * Math.pow(v.per, (levels && levels[v.id]) || 0) : m), 1);
  const mediaLevel = (state) => D.VENUE.reduce((t, v) => t + (v.effect === 'media' ? v.per * (state.venue[v.id] || 0) : 0), 0);

  const prepMult = (state) => place(state).style * lineMult(state.venue, 'style');
  const sponsorMult = (state) => lineMult(state.venue, 'cash');
  const traitSum = (state, key) => seatedDrivers(state).reduce((t, d) => t + (trait(d)[key] || 0), 0);
  const cashMult = (state) => D.VENUE_CASH * (place(state).cash || 1) * sponsorMult(state) * (1 + traitSum(state, 'cash'));
  /* How much the crowd likes this car's looks: 1 + the bonus of the best tier the car owns in each category
     (paint, rims, body kit, underglow), whatever it is wearing. */
  const LOOKS = { paint: D.PAINTS, rims: D.RIMS, kit: D.KITS, glow: D.GLOWS };
  function looksBonus(car, kind) {
    const ids = ((car.owned && car.owned[kind]) || []).concat(car[kind]);
    return ids.reduce((m, id) => { const c = byId(LOOKS[kind], id); return Math.max(m, (c && c.style) || 0); }, 0);
  }
  /* The best tier of this kind the car owns (null if only free items). */
  function bestTier(car, kind) {
    const b = looksBonus(car, kind);
    return b > 0 ? D.COSMETIC_TIERS[kind].find((t) => t.style === b) || null : null;
  }
  const looksMult = (car) => 1 + Object.keys(LOOKS).reduce((t, k) => t + looksBonus(car, k), 0);
  const cosmeticTier = (kind, tierId) => (D.COSMETIC_TIERS[kind] || []).find((t) => t.id === tierId);
  const tierOwned = (car, kind, tierId) => !!car && LOOKS[kind].some((c) => c.tier === tierId && car.owned[kind].includes(c.id));
  const crowdCap = (state) => place(state).crowd * lineMult(state.venue, 'crowd') * (1 + traitSum(state, 'crowd'));

  /* Style points a second this car scores while it is drifting. 0 without a driver. */
  function carStyle(state, car) {
    const d = driverOf(state, car);
    if (!d) return 0;
    const t = trait(d);
    let v = model(car.model).base * partsMult(car) * driverMult(d) * prepMult(state) * (t.style || 1) * looksMult(car);
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
    const b = boost(state), cap = crowdCap(state) * b.crowd;   // live venue income includes tap-bubble effects
    const cheered = Math.min(style, cap);
    return { style, cap, cheered, cash: cheered * cashMult(state) * b.cash, full: style > cap };
  }

  /* Long-run income, allowing for crashes. Used for away earnings and Street Drifting payouts.
     Tap-bubble effects are left out: they only lift what the venue earns while you watch it. */
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
  const slotCost = (state) => (state.slots >= maxPits(state) ? Infinity : D.SLOT_COSTS[state.slots]);
  function venueCost(state, id) {
    const v = byId(D.VENUE, id);
    if (state.venue[id] >= D.VENUE_MAX[id]) return Infinity;
    return Math.ceil(v.cost * place(state).scale * Math.pow(v.growth, state.venue[id]));
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
    if (!car || !part(partId) || car.parts[partId] >= partCap(state)) return false;
    if (!spend(state, partCost(car, partId))) return false;
    car.parts[partId]++;
    return true;
  }

  function buySlot(state) {
    if (state.slots >= maxPits(state) || !spend(state, slotCost(state))) return false;
    state.slots++;
    return true;
  }

  function buyCar(state, modelId) {
    const m = model(modelId);
    if (!m || !modelUnlocked(state, modelId) || state.cars.length >= state.slots || !spend(state, m.cost)) return false;
    const car = addCar(state, modelId);
    const bench = benchDrivers(state).sort((a, b) => driverMult(b) - driverMult(a))[0];
    if (bench) car.driver = bench.id;
    return true;
  }

  /* Swap a car for a better model. Its parts and driver move across. */
  function tradeCar(state, carId, modelId) {
    const car = getCar(state, carId);
    const m = model(modelId);
    if (!car || !m || !modelUnlocked(state, modelId) || modelIndex(modelId) <= modelIndex(car.model) || !spend(state, m.cost)) return false;
    car.model = modelId;
    car.owned.kit = []; car.kit = 'stock';   // a kit is made for one model; paint, rims and underglow stay on the car
    return true;
  }

  function buyVenue(state, id) {
    if (!byId(D.VENUE, id) || !spend(state, venueCost(state, id))) return false;
    state.venue[id]++;
    return true;
  }

  const zeroLevels = () => { const l = {}; D.VENUE.forEach((v) => (l[v.id] = 0)); return l; };
  /* The next venue goes on sale only once every upgrade line at the best venue owned is at its top level. */
  const placeLocked = (state) => !levelsMaxed(state.levels[state.placeOwned]);
  /* Buy the next venue. It is kept for good; the team stays where it is until it moves. */
  function buyPlace(state) {
    const v = nextPlace(state);
    if (!v || placeLocked(state) || !spend(state, v.cost)) return false;
    state.placeOwned++;
    state.levels[state.placeOwned] = state.levels[state.placeOwned] || zeroLevels();
    return true;
  }
  /* Move the team to a venue it owns (the newest one if none is named). Every venue keeps its own
     grandstands, sponsors and track prep; a new venue starts with none. */
  function movePlace(state, index) {
    if (index == null) index = state.placeOwned;
    if (!ownsPlace(state, index) || index === state.place) return false;
    state.levels[index] = state.levels[index] || zeroLevels();
    state.place = index;
    state.venue = state.levels[index];
    return true;
  }

  /* ---------- Tap bubbles ---------- */

  const bubble = (id) => byId(D.BUBBLES, id);
  /* What the running effects add up to. */
  function boost(state) {
    const out = { crowd: 1, cash: 1, train: false };
    (state.fx || []).forEach((f) => { const b = bubble(f.id); if (b) { out.crowd *= b.crowd || 1; out.cash *= b.boost || 1; out.train = out.train || !!b.train; } });
    return out;
  }
  const fxLeft = (state, id) => { const f = state.fx.find((x) => x.id === id); return f ? f.left : 0; };
  /* Seconds of watching until the next bubble. Media upgrades at the venue shorten it. */
  const bubbleGap = (rng, state) => (D.BUBBLE_GAP[0] + (rng || Math.random)() * (D.BUBBLE_GAP[1] - D.BUBBLE_GAP[0])) * (state ? 1 - mediaLevel(state) : 1);
  /* What bubbles pay, times: media upgrades raise it. */
  const bubblePay = (state) => 1 + 2 * mediaLevel(state);
  /* Which bubble comes next. One whose effect is still running does not come again yet. */
  function rollBubble(state, rng) {
    const list = D.BUBBLES.filter((b) => !b.time || fxLeft(state, b.id) <= 0);
    let x = (rng || Math.random)() * list.reduce((t, b) => t + b.weight, 0);
    for (const b of list) if ((x -= b.weight) < 0) return b.id;
    return list[0].id;
  }
  /* The player tapped a bubble: pay its cash now, or start its effect. Returns the cash paid. */
  function popBubble(state, id) {
    const b = bubble(id);
    if (!b) return 0;
    if (b.time) {
      const f = state.fx.find((x) => x.id === id);
      if (f) f.left = b.time; else state.fx.push({ id, left: b.time });
    }
    const cash = (b.cash || 0) * expectedIncome(state) * bubblePay(state);
    earn(state, cash);
    return cash;
  }
  /* Run the effects' clocks. Returns the ids that just ended. */
  function tickFx(state, dt) {
    const ended = [];
    state.fx.forEach((f) => { f.left -= dt; if (f.left <= 0) ended.push(f.id); });
    if (ended.length) state.fx = state.fx.filter((f) => f.left > 0);
    return ended;
  }

  /* ---------- Street cred ---------- */

  /* The rank a cred total has reached, and how far it is to the next. */
  function rankOf(cred) {
    let index = 0;
    D.RANKS.forEach((r, i) => { if (cred >= r.cred) index = i; });
    const next = D.RANKS[index + 1] || null, base = D.RANKS[index].cred;
    return { index, name: D.RANKS[index].name, cred, next, progress: next ? (cred - base) / (next.cred - base) : 1 };
  }
  const rank = (state) => rankOf(state.street.cred);
  /* Cred for one run: distance, points, and a bonus for each record it set. */
  function credFor(dist, score, bests) {
    return Math.floor(dist / 100) * D.CRED_PER_100M + Math.floor(score / 1000) * D.CRED_PER_1000_POINTS + (bests || 0) * D.CRED_NEW_BEST;
  }
  /* Add cred. Returns the rank before and after, and what the new ranks put on sale. */
  function addCred(state, amount) {
    const before = rank(state);
    state.street.cred += Math.max(0, amount);
    const after = rank(state), unlocked = [];
    if (after.index > before.index) {
      /* A rank puts whole tiers on sale: each entry is a tier and its items. */
      Object.keys(D.COSMETIC_TIERS).forEach((kind) => D.COSMETIC_TIERS[kind].forEach((t) => {
        if (t.rank > before.index && t.rank <= after.index) unlocked.push({ kind, tier: t, items: LOOKS[kind].filter((c) => c.tier === t.id) });
      }));
    }
    return { before, after, unlocked };
  }

  /* ---------- Cosmetics: paint, rims, body kits, underglow. Looks only. ---------- */

  const COSMETICS = LOOKS;
  const cosmetic = (kind, id) => (COSMETICS[kind] ? byId(COSMETICS[kind], id) : undefined);
  /* Set price: it depends on the item (and, for a kit, the car model), never on the car's upgrades. */
  function cosmeticCost(kind, id, modelId) {
    const c = cosmetic(kind, id);
    return c ? c.cost * (kind === 'kit' ? model(modelId).kitScale : 1) : Infinity;
  }
  /* Cosmetics are bought per car: each car owns only what was bought for it (free items are everyone's). */
  function ownsCosmetic(car, kind, id) {
    const c = cosmetic(kind, id);
    if (!c || !car) return false;
    return c.cost === 0 || car.owned[kind].includes(id);
  }
  const cosmeticUnlocked = (state, kind, id) => { const c = cosmetic(kind, id); return !!c && rank(state).index >= c.rank; };
  /* Put an owned cosmetic on a car. */
  function applyCosmetic(state, carId, kind, id) {
    const car = getCar(state, carId), c = cosmetic(kind, id);
    if (!car || !c || !ownsCosmetic(car, kind, id)) return false;
    if (kind === 'paint') { car.paint = id; car.color = c.color; } else car[kind] = id;
    return true;
  }
  /* Buy a cosmetic (for this car's model, if it is a kit) and put it on the car. */
  function buyCosmetic(state, carId, kind, id) {
    const car = getCar(state, carId);
    if (!car || !cosmetic(kind, id) || ownsCosmetic(car, kind, id) || !cosmeticUnlocked(state, kind, id)) return false;
    if (!spend(state, cosmeticCost(kind, id, car.model))) return false;
    car.owned[kind].push(id);
    return applyCosmetic(state, carId, kind, id);
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

  function rollDriver(state, rng, taken, rarityId) {
    rng = rng || Math.random;
    const r = rarityId ? rarity(rarityId) : rollRarity(rng);
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

  /* ---------- Testing shortcuts (Menu → Testing). They skip the normal prices and limits on purpose. ---------- */

  /* Every pit bought, each running the best car on sale, and a fresh driver in every empty seat. */
  function testFillPits(state, rng) {
    const best = D.MODELS.filter((m) => modelUnlocked(state, m.id)).pop().id;
    state.slots = Math.max(state.slots, maxPits(state));
    state.cars.forEach((c) => { if (modelIndex(c.model) < modelIndex(best)) { c.model = best; c.owned.kit = []; c.kit = 'stock'; } });
    while (state.cars.length < state.slots) addCar(state, best);
    state.cars.forEach((c) => {
      if (c.driver != null) return;
      const bench = benchDrivers(state)[0];
      if (bench) { c.driver = bench.id; return; }
      const d = rollDriver(state, rng);
      state.drivers.push(d);
      c.driver = d.id;
    });
    return true;
  }
  /* Own every venue, whatever the gate says. A venue not owned yet starts with no upgrades. */
  function testUnlockVenues(state) {
    while (state.placeOwned < D.VENUES.length - 1) { state.placeOwned++; state.levels[state.placeOwned] = state.levels[state.placeOwned] || zeroLevels(); }
    return true;
  }
  /* A free tryout, at once, where all three prospects are Legendary. */
  function testLegendaryTryout(state, now, rng) {
    if (state.prospects) return false;
    state.prospects = [];
    for (let i = 0; i < D.TRYOUT_SIZE; i++) state.prospects.push(rollDriver(state, rng, state.prospects.map((p) => p.name), 'legendary'));
    return true;
  }
  /* What an hour away would pay, as the away screen shows it. Pays it, and the drivers' seat time. */
  function testSkipTime(state, seconds) {
    const paid = Math.min(seconds, D.OFFLINE_CAP), info = { seconds, paidSeconds: paid, cash: expectedIncome(state) * paid };
    earn(state, info.cash);
    info.levelUps = awayXP(state, paid).length;
    return info;
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

  /* Let a driver go for good. A seated driver's car gets the best reserve driver, or stands empty.
     The last driver can't be let go. No experience is passed on. */
  function releaseDriver(state, driverId) {
    const d = getDriver(state, driverId);
    if (!d || state.drivers.length <= 1) return false;
    const seat = carOfDriver(state, d.id);
    state.drivers = state.drivers.filter((x) => x !== d);
    if (seat) {
      const bench = benchDrivers(state).sort((a, b) => driverMult(b) - driverMult(a))[0];
      seat.driver = bench ? bench.id : null;
    }
    return { driver: d, car: seat, replacement: seat && seat.driver != null ? getDriver(state, seat.driver) : null };
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
  /* Drift steering sensitivity, kept inside the allowed range. */
  const steerSensOf = (v) => Math.min(D.TOUGE.STEER_SENS[1], Math.max(D.TOUGE.STEER_SENS[0], +v || 1));
  const steerSens = (state) => steerSensOf(state.settings && state.settings.steer);
  function setSteerSens(state, v) { state.settings = state.settings || {}; state.settings.steer = steerSensOf(v); return state.settings.steer; }

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
    s.slots = Math.max(1, Math.floor(num(raw.slots, 1)));   // capped to the venue's pits below, once the venue is known
    s.lastSeen = num(raw.lastSeen, now || 0);
    s.settings.steer = steerSensOf(num(raw.settings && raw.settings.steer, 1));
    s.settings.testing = !(raw.settings && raw.settings.testing === false);
    s.tryouts = Math.floor(num(raw.tryouts, 0));
    s.nextTryoutAt = num(raw.nextTryoutAt, 0);
    s.mech.count = Math.min(D.MECH_MAX, Math.max(1, Math.floor(num(raw.mech && raw.mech.count, 1))));
    s.mech.level = Math.floor(num(raw.mech && raw.mech.level, 0));
    s.placeOwned = Math.min(D.VENUES.length - 1, Math.floor(num(raw.placeOwned, 0)));
    s.place = Math.min(s.placeOwned, Math.floor(num(raw.place, 0)));
    s.slots = Math.min(maxPits(s), s.slots);
    s.levels = [];
    for (let i = 0; i <= s.placeOwned; i++) {   // older saves only kept the levels of the venue the team was at
      const from = (Array.isArray(raw.levels) && raw.levels[i]) || (i === s.place && raw.venue) || {};
      s.levels[i] = zeroLevels();
      D.VENUE.forEach((v) => (s.levels[i][v.id] = Math.min(D.VENUE_MAX[v.id], Math.floor(num(from[v.id], 0)))));
    }
    s.venue = s.levels[s.place];
    const st = raw.street || {};
    s.street.runs = Math.floor(num(st.runs, 0)); s.street.cred = num(st.cred, 0);
    Object.keys(st.best || {}).forEach((k) => (s.street.best[k] = { score: num(st.best[k] && st.best[k].score, 0), dist: num(st.best[k] && st.best[k].dist, 0) }));
    if (!st.best && (st.bestScore || st.bestDist)) s.street.best.pass = { score: num(st.bestScore, 0), dist: num(st.bestDist, 0) };
    const o = raw.owned || {};
    const paid = (list, all) => (Array.isArray(list) ? Array.from(new Set(list.filter((id) => byId(all, id) && byId(all, id).cost > 0))) : []);
    s.drivers = raw.drivers.filter(isDriver).map((d) => cleanDriver(d, num));
    s.cars = raw.cars.filter((c) => c && model(c.model)).slice(0, s.slots).map((c, i) => {
      const parts = emptyParts();
      D.PARTS.forEach((p) => {
        const lv = Math.floor(num(c.parts && c.parts[p.id], 0));
        parts[p.id] = Math.min(D.PART_MAX, lv);
      });
      const driver = s.drivers.some((d) => d.id === c.driver) ? c.driver : null;
      const paint = byId(D.PAINTS, c.paint) || D.PAINTS.find((p) => p.color === c.color) || byId(D.PAINTS, D.FACTORY_PAINTS[i % D.FACTORY_PAINTS.length]);
      return { id: c.id, model: c.model, parts, driver, paint: paint.id, color: paint.color,
        rims: byId(D.RIMS, c.rims) ? c.rims : 'steel', kit: byId(D.KITS, c.kit) ? c.kit : 'stock', glow: byId(D.GLOWS, c.glow) ? c.glow : 'none',
        // what was bought for this car. Saves from when cosmetics belonged to the team (raw.owned): every car
        // keeps the team's paints, rims and underglows, and the kits bought for its model
        owned: {
          paint: paid(c.owned ? c.owned.paint : o.paints, D.PAINTS), rims: paid(c.owned ? c.owned.rims : o.rims, D.RIMS),
          glow: paid(c.owned ? c.owned.glow : o.glows, D.GLOWS), kit: paid(c.owned ? c.owned.kit : o.kits && o.kits[c.model], D.KITS),
        } };
    });
    s.cars.forEach((c) => {   // nothing worn that the car does not own
      if (!ownsCosmetic(c, 'paint', c.paint)) { c.paint = D.FACTORY_PAINTS[0]; c.color = byId(D.PAINTS, c.paint).color; }
      if (!ownsCosmetic(c, 'rims', c.rims)) c.rims = 'steel';
      if (!ownsCosmetic(c, 'kit', c.kit)) c.kit = 'stock';
      if (!ownsCosmetic(c, 'glow', c.glow)) c.glow = 'none';
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
    SAVE_VERSION, MAX_SLOTS, steerSens, setSteerSens, testFillPits, testUnlockVenues, testLegendaryTryout, testSkipTime, seeded, model, part, rarity, trait, modelIndex, newState, load, serialize,
    getCar, getDriver, driverOf, carOfDriver, benchDrivers, seatedDrivers,
    driverStats, driverMult, levelCap, xpForLevel, xpToNext, totalXP, addXP, tickXP, awayXP,
    partsMult, looksMult, prepMult, sponsorMult, cashMult, crowdCap, carStyle, carCrashRate, repairTime, carRepairTime, carUptime,
    incomeRate, expectedIncome, offlineEarnings,
    partCost, slotCost, venueCost, tryoutCost, mechHireCost, mechTrainCost, mechTrainMaxed,
    earn, buyPart, buySlot, buyCar, tradeCar, buyVenue, rollDriver, assignDriver, hireMech, trainMech,
    bubble, boost, fxLeft, bubbleGap, rollBubble, popBubble, tickFx,
    place, bestPlace, nextPlace, ownsPlace, partCap, modelPlace, modelUnlocked, maxPits, venueMaxed, levelsMaxed, venueProgress, lineMult, mediaLevel, bubblePay,
    placeLocked, buyPlace, movePlace,
    rankOf, rank, credFor, addCred, cosmetic, cosmeticCost, ownsCosmetic, looksBonus, bestTier, cosmeticTier, tierOwned, releaseDriver, cosmeticUnlocked, applyCosmetic, buyCosmetic,
    rosterCap, rosterFull, needsDriver, tryoutWait, holdTryout, mentored, signProspect, dismissProspects,
    fmt, money, duration, clock,
  };
})(typeof window !== 'undefined' ? window : globalThis);
