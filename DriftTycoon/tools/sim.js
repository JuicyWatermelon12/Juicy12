/* Pacing check: a simple player plays for an hour and we count how often they buy something.
   Run: node tools/sim.js [minutes=60] [seed=1] [--active=5]
   The player always saves for whatever pays itself back fastest, holds a tryout when one is ready and
   cheap next to their cash, and signs a prospect when they beat the weakest driver. */
require('../src/data.js');
require('../src/game.js');
const { data: D, game: G } = globalThis.DT;

const minutes = +process.argv[2] || 60, seed = +process.argv[3] || 1;
// --active=N: the player also goes Street Drifting, which pays N times the venue's income on top of it
const active = +((process.argv.find((a) => a.startsWith('--active=')) || '=0').split('=')[1]);
const rng = G.seeded(seed);
const clone = (s) => JSON.parse(JSON.stringify(s));
const potential = (d) => { const c = Object.assign({}, d, { level: G.levelCap(d) }); return G.driverMult(c); };

function candidates(s) {
  const out = [];
  const base = G.expectedIncome(s);
  const consider = (name, cost, apply) => {
    if (!isFinite(cost)) return;
    const t = clone(s);
    t.cash = Infinity;
    if (!apply(t)) return;
    const gain = G.expectedIncome(t) - base;
    if (gain > 1e-9) out.push({ name, cost, payback: cost / gain, apply });
  };
  s.cars.forEach((c) => {
    D.PARTS.forEach((p) => {
      if (c.parts[p.id] < G.partCap(s)) consider(G.model(c.model).name + ' ' + p.name, G.partCost(c, p.id), (t) => G.buyPart(t, c.id, p.id));
    });
    const next = D.MODELS[G.modelIndex(c.model) + 1];
    if (next) consider('trade to ' + next.name, next.cost, (t) => G.tradeCar(t, c.id, next.id));
  });
  D.VENUE.forEach((v) => consider(v.name, G.venueCost(s, v.id), (t) => G.buyVenue(t, v.id)));
  if (!G.mechTrainMaxed(s)) consider('mechanic training', G.mechTrainCost(s), (t) => G.trainMech(t));
  if (s.slots < G.maxPits(s) && s.cars.length >= s.slots) {
    const best = s.cars.reduce((m, c) => Math.max(m, G.modelIndex(c.model)), 0);
    const m = D.MODELS[Math.max(0, best - 1)];
    consider('pit + ' + m.name, G.slotCost(s) + m.cost, (t) => {
      if (!G.buySlot(t) || !G.buyCar(t, m.id)) return false;
      const car = t.cars[t.cars.length - 1];
      if (car.driver == null) {   // assume an average common turns up at the tryout
        t.drivers.push({ id: 9e6, name: 'x', rarity: 'common', skill: 19, control: 19, flair: 19, level: 1, xp: 0, trait: null });
        car.driver = 9e6;
      }
      return true;
    });
  }
  return out.sort((a, b) => a.payback - b.payback);
}

const s = G.newState(0);
let now = 0, buys = [], log = [], target = null;
const builtAt = {};
const buckets = new Array(Math.ceil(minutes / 5)).fill(0);
const note = (what) => { buys.push(now); buckets[Math.min(buckets.length - 1, Math.floor(now / 300))]++; log.push([now, what]); };

for (now = 0; now < minutes * 60; now++) {
  G.earn(s, G.expectedIncome(s) * (1 + active));
  G.awayXP(s, 1);
  // tryouts
  if (!s.prospects && G.tryoutWait(s, now * 1000) === 0 && (G.needsDriver(s) || G.tryoutCost(s) < s.cash * 0.25) && G.holdTryout(s, now * 1000, rng)) {
    const best = s.prospects.map((p, i) => [potential(p), i]).sort((a, b) => b[0] - a[0])[0];
    const weakest = s.drivers.slice().sort((a, b) => potential(a) - potential(b))[0];
    if (!G.rosterFull(s)) { G.signProspect(s, best[1]); note('tryout: signed'); }
    else if (best[0] > potential(weakest) * 1.15) { G.signProspect(s, best[1], weakest.id); note('tryout: replaced'); }
    else { G.dismissProspects(s); note('tryout: none'); }
    target = null;
  }
  // a fully built venue: every upgrade at its top level, every pit running the best car on sale there
  const bestCar = D.MODELS.filter((m) => G.modelUnlocked(s, m.id)).pop().id;
  const built = G.venueMaxed(s) && s.slots >= G.maxPits(s) && s.cars.length >= s.slots && s.cars.every((c) => c.model === bestCar && c.driver != null);
  if (built && !builtAt[s.place]) {
    builtAt[s.place] = now;
    console.log('\n== ' + G.place(s).name + ' fully built at ' + G.duration(now) + ': income $' + G.fmt(G.expectedIncome(s), 1) + '/s, style ' + G.fmt(G.incomeRate(s).style) +
      '/s, crowd cap ' + G.fmt(G.crowdCap(s)) + ', parts [' + s.cars.map((c) => D.PARTS.map((p) => c.parts[p.id]).join('')).join(' ') + ']');
  }
  if (built && G.nextPlace(s)) {   // save for the next venue, buy it and move at once
    if (s.cash >= G.nextPlace(s).cost && G.buyPlace(s) && G.movePlace(s)) {
      console.log('== moved to ' + G.place(s).name + ' at ' + G.duration(now) + ' (saved for ' + G.duration(now - builtAt[s.place - 1]) + '); income there $' +
        G.fmt(G.expectedIncome(s), 1) + '/s, crowd full: ' + G.incomeRate(s).full);
      note('move to ' + G.place(s).name);
      target = null;
    }
    continue;
  }
  if (!target) target = candidates(s)[0] || null;
  if (target && s.cash >= target.cost) {
    if (target.apply(s)) note(target.name);
    target = null;
  }
  if (s.mech.count < D.MECH_MAX && G.mechHireCost(s) < s.cash * 0.05) { G.hireMech(s); note('hire mechanic'); }
  // cheap venue upgrades are bought even when they do not pay yet (a grandstand before the crowd is full)
  D.VENUE.forEach((v) => { if (G.venueCost(s, v.id) < s.cash * 0.05 && G.buyVenue(s, v.id)) note(v.name); });
  if ((now + 1) % 600 === 0) {
    console.log(`\n${(now + 1) / 60} min: $${G.fmt(s.cash)}  income $${G.fmt(G.expectedIncome(s), 1)}/s  pits ${s.slots}  ` +
      `${G.place(s).name}  stands ${s.venue.stands} sponsors ${s.venue.sponsors} prep ${s.venue.prep}  mech ${s.mech.count}/L${s.mech.level}`);
    s.cars.forEach((c) => { const d = G.driverOf(s, c); console.log(`   ${G.model(c.model).name} [${D.PARTS.map((p) => c.parts[p.id]).join('')}]  ${d ? d.rarity + ' L' + d.level : 'no driver'}`); });
  }
}
console.log('\nPurchases in each 5 minutes:', buckets.join(' '));
const gaps = buys.slice(1).map((t, i) => t - buys[i]).sort((a, b) => a - b);
console.log(`Total ${buys.length}. Gap between purchases: median ${gaps[gaps.length >> 1]} s, longest ${gaps[gaps.length - 1]} s`);
if (process.argv.includes('--log')) log.forEach((l) => console.log(G.duration(l[0]).padStart(12), l[1]));
