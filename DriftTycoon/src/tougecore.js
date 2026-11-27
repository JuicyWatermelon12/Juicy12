/* Drift Tycoon — Street Drifting rules (code name "touge"): the endless road, the car's handling,
   scoring and the payout. Pure logic (no DOM, no Three.js).
   The road runs down a slope: +z is downhill, x is across it. A heading of 0 points straight downhill;
   the direction of travel is (sin h, cos h). */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});
  const D = DT.data, G = DT.game, T = D.TOUGE;
  const TAU = Math.PI * 2, SMOOTH = 5;
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const wrapPi = (a) => { a = (a + Math.PI) % TAU; if (a < 0) a += TAU; return a - Math.PI; };
  /* How hard the road is at a distance: 0 at the start, 1 from RAMP_KM on. */
  const difficulty = (metres) => clamp(metres / (T.RAMP_KM * 1000), 0, 1);
  const ramp = (pair, d) => pair[0] + (pair[1] - pair[0]) * d;

  /* An endless winding road. It always makes progress downhill, so it can never cross itself.
     Points are made on demand by extendRoad; the same seed always gives the same road. */
  function newRoad(rng) {
    const road = { pts: [], n: 0, ds: T.STEP, halfW: T.HALF_WIDTH,
      gen: { rng, kap: [], cur: 0, side: rng() < 0.5 ? -1 : 1, x: 0, z: 0, h: 0 } };
    for (let i = Math.round(T.START_STRAIGHT / T.STEP); i > 0; i--) road.gen.kap.push(0);
    extendRoad(road, 300);
    return road;
  }

  /* Plan one more bend and the straight after it. */
  function planBend(road) {
    const g = road.gen, ds = road.ds, d = difficulty(g.kap.length * ds - T.START_STRAIGHT);
    const push = (len, k) => { for (let i = Math.max(1, Math.round(len / ds)); i > 0; i--) g.kap.push(k); };
    const target = g.side * (ramp(T.TURN_MIN, d) + g.rng() * (ramp(T.TURN_MAX, d) - ramp(T.TURN_MIN, d)));
    const r = ramp(T.RADIUS_MIN, d) + g.rng() * (ramp(T.RADIUS_MAX, d) - ramp(T.RADIUS_MIN, d));
    const turn = target - g.cur, steps = Math.max(1, Math.ceil(Math.abs(turn) * r / ds));
    push(steps * ds, turn / (steps * ds));   // turns the road by exactly `turn`, no tighter than radius r
    g.cur = target;
    push(ramp(T.STRAIGHT_MIN, d) + g.rng() * (ramp(T.STRAIGHT_MAX, d) - ramp(T.STRAIGHT_MIN, d)), 0);
    if (g.rng() < 0.85) g.side = -g.side;
  }

  /* Make sure the road has at least `count` points. */
  function extendRoad(road, count) {
    const g = road.gen, ds = road.ds;
    while (road.n < count) {
      const i = road.n;
      while (g.kap.length <= i + SMOOTH) planBend(road);
      let k = 0, c = 0;
      for (let j = -SMOOTH; j <= SMOOTH; j++) if (i + j >= 0) { k += g.kap[i + j]; c++; }   // ease in and out of bends
      k /= c;
      road.pts.push({ x: g.x, z: g.z, h: g.h, k });
      g.h += k * ds;
      g.x += Math.sin(g.h) * ds; g.z += Math.cos(g.h) * ds;
      road.n++;
    }
    return road;
  }

  /* How this car and driver handle on the road. */
  function carSetup(state, car) {
    const p = car.parts, d = G.driverOf(state, car), s = d ? G.driverStats(d) : { skill: 0, control: 0, flair: 0 };
    return {
      top: T.TOP_SPEED + T.TOP_PER_STAGE * (p.engine + p.turbo),
      accel: T.ACCEL,
      lock: T.LOCK + T.LOCK_PER_ANGLE * p.angle,
      grip: T.GRIP,
      maxSlip: T.MAX_SLIP + T.SLIP_PER_ANGLE * p.angle,
      hitKeep: Math.min(0.85, T.HIT_KEEP + T.HIT_KEEP_PER_SUSP * p.suspension),
      bonus: (1 + T.BONUS_PER_STANCE * p.stance + T.BONUS_PER_TIRES * p.tires) * (1 + s.skill / T.SKILL_DIV) * (1 + s.flair / T.FLAIR_DIV),
    };
  }

  function newRun(road, setup) {
    extendRoad(road, T.START_OFFSET + 300);
    const p = road.pts[T.START_OFFSET];
    return { road, setup, x: p.x, z: p.z, h: p.h, va: p.h, speed: T.START_SPEED, steer: 0, idx: T.START_OFFSET, slip: 0, lateral: 0,
      comboT: 0, grace: 0, mult: 1, bestMult: 1, score: 0, hits: 0, hitT: 0, t: 0, hold: T.COUNTDOWN,
      dist: 0, km: 0, kmScore: 0, cleanKm: 0, cleanBonus: 0, drifting: false, done: false };
  }

  const multCap = (run) => T.COMBO_MAX + T.COMBO_MAX_PER_KM * run.km;
  const hitsLeft = (run) => Math.max(0, T.MAX_HITS - run.hits);

  /* Move the nearest-road-point index along with the car (it only ever looks a little way around). */
  function locate(run) {
    const pts = run.road.pts;
    let best = run.idx, bd = Infinity;
    for (let i = Math.max(0, run.idx - 6); i <= Math.min(pts.length - 1, run.idx + 40); i++) {
      const dx = pts[i].x - run.x, dz = pts[i].z - run.z, d = dx * dx + dz * dz;
      if (d < bd) { bd = d; best = i; }
    }
    run.idx = best;
    return pts[best];
  }

  /* One step of driving. `input` is the steering from -1 (full left) to 1 (full right), as the driver sees it.
     Returns what happened: 'hit' (touched a barrier), 'km' (passed a kilometre marker), 'out' (third touch: run over). */
  function step(run, input, dt) {
    const ev = [];
    if (run.done) return ev;
    if (run.hold > 0) { run.hold -= dt; return ev; }
    const s = run.setup;
    extendRoad(run.road, run.idx + 320);
    run.t += dt;
    run.steer += (clamp(input || 0, -1, 1) - run.steer) * (1 - Math.exp(-12 * dt));
    // the nose turns with the wheel; steering right swings the heading towards -x
    run.h -= run.steer * s.lock * run.speed * dt;
    let p = run.road.pts[run.idx];
    run.h = p.h + clamp(wrapPi(run.h - p.h), -1.75, 1.75);   // no spinning round to face uphill
    // the direction of travel chases the nose; the gap between them is the drift angle
    let slip = wrapPi(run.h - run.va);
    run.va += slip * (1 - Math.exp(-s.grip * dt));
    slip = wrapPi(run.h - run.va);
    if (Math.abs(slip) > s.maxSlip) { slip = Math.sign(slip) * s.maxSlip; run.va = run.h - slip; }
    run.slip = slip;
    const top = s.top * (1 + Math.min(T.SPEED_RAMP_MAX, T.SPEED_PER_KM * run.dist / 1000));   // the road gets faster the further down it goes
    const target = top * (1 - T.SLIDE_DRAG * Math.min(1, Math.abs(slip)));
    run.speed += (target - run.speed) * (1 - Math.exp(-s.accel * dt));
    run.x += Math.sin(run.va) * run.speed * dt;
    run.z += Math.cos(run.va) * run.speed * dt;

    // barriers on both sides of the road
    p = locate(run);
    run.dist = (run.idx - T.START_OFFSET) * run.road.ds;
    const nx = Math.cos(p.h), nz = -Math.sin(p.h);
    const d = (run.x - p.x) * nx + (run.z - p.z) * nz, limit = run.road.halfW - T.CAR_HALF;
    run.lateral = d;
    run.hitT -= dt;
    if (Math.abs(d) > limit) {
      const sg = Math.sign(d), over = Math.abs(d) - limit;
      run.x -= nx * over * sg; run.z -= nz * over * sg;
      run.lateral = sg * limit;
      let vx = Math.sin(run.va) * run.speed, vz = Math.cos(run.va) * run.speed;
      const vn = (vx * nx + vz * nz) * sg;
      if (vn > 0) { vx -= nx * sg * vn * (1 + T.BOUNCE); vz -= nz * sg * vn * (1 + T.BOUNCE); }
      if (run.hitT <= 0) {
        run.speed = Math.hypot(vx, vz) * s.hitKeep;
        run.hits++;
        ev.push('hit');
      }
      run.va = Math.atan2(vx, vz);
      run.h += wrapPi(p.h - run.h) * 0.5;
      run.hitT = 0.5;
      run.comboT = 0; run.mult = 1; run.grace = 0;
      if (run.hits >= T.MAX_HITS) { run.done = true; ev.push('out'); return ev; }
    }

    // score: speed x angle x multiplier while the car is properly sideways
    run.drifting = Math.abs(run.slip) > T.DRIFT_MIN && run.speed > T.MIN_SPEED && run.hitT <= 0;
    if (run.drifting) {
      run.grace = 0;
      run.comboT += dt;
      run.mult = Math.min(multCap(run), 1 + run.comboT / T.COMBO_TIME);
      run.bestMult = Math.max(run.bestMult, run.mult);
      run.score += run.speed * Math.min(1, Math.abs(run.slip)) * T.SCORE_RATE * run.mult * s.bonus * dt;
    } else {
      run.grace += dt;
      if (run.grace > T.COMBO_GRACE) { run.comboT = 0; run.mult = 1; }
    }

    // kilometre markers: the multiplier cap rises, and a kilometre with no touches so far pays a bonus
    const km = Math.floor(run.dist / 1000);
    if (km > run.km) {
      run.lastKmBonus = 0;
      if (run.hits === 0) {
        run.lastKmBonus = (run.score - run.kmScore) * T.CLEAN_KM_BONUS;
        run.score += run.lastKmBonus; run.cleanBonus += run.lastKmBonus; run.cleanKm++;
      }
      run.km = km; run.kmScore = run.score;
      ev.push('km');
    }
    return ev;
  }

  /* What a run is worth: points buy seconds of the team's long-run income. */
  function payout(state, run) {
    const seconds = run.score / T.POINTS_PER_SECOND;
    return { score: Math.floor(run.score), seconds, cash: seconds * G.expectedIncome(state), xp: run.t * T.XP_MULT,
      bestMult: run.bestMult, hits: run.hits, out: run.hits >= T.MAX_HITS, time: run.t, dist: run.dist,
      cleanKm: run.cleanKm, cleanBonus: Math.floor(run.cleanBonus) };
  }

  /* Pay the run out: cash to the team, seat time to the driver, and the team's records. */
  function collect(state, car, run) {
    const res = payout(state, run), rec = state.street;
    G.earn(state, res.cash);
    const d = G.driverOf(state, car);
    res.driver = d;
    res.levels = d ? G.addXP(d, res.xp * (G.trait(d).xp || 1)) : 0;
    res.bestScore = res.score > rec.bestScore; res.bestDist = res.dist > rec.bestDist;
    rec.runs++; rec.bestScore = Math.max(rec.bestScore, res.score); rec.bestDist = Math.max(rec.bestDist, res.dist);
    return res;
  }

  /* Simple drivers for tests and tuning. `autopilot` follows the road; `drifter` also keeps the car sideways. */
  function autopilot(run, look) {
    const pts = run.road.pts, a = pts[Math.min(pts.length - 1, run.idx + (look || Math.round(7 + run.speed * 0.4)))];
    const want = Math.atan2(a.x - run.x, a.z - run.z);
    return clamp(-wrapPi(want - run.h) * 2.2, -1, 1);
  }
  function drifter(run, keen) {
    const p = run.road.pts[run.idx], room = 1 - Math.abs(run.lateral) / (run.road.halfW - T.CAR_HALF);
    const straight = Math.abs(p.k) < 0.012 ? 1 : 0.25;   // weave on the straights, lean on the bends
    return clamp(autopilot(run) + Math.sin(run.t * 3.1) * (keen == null ? 0.7 : keen) * straight * clamp(room * 2, 0, 1), -1, 1);
  }

  DT.touge = { newRoad, extendRoad, difficulty, carSetup, newRun, step, payout, collect, multCap, hitsLeft, autopilot, drifter, wrapPi };
})(typeof window !== 'undefined' ? window : globalThis);
