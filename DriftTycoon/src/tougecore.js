/* Drift Tycoon — Touge practice rules: the road, the car's handling, scoring and the payout.
   Pure logic (no DOM, no Three.js). The road runs down a slope: +z is downhill, x is across it.
   A heading of 0 points straight downhill; the direction of travel is (sin h, cos h). */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});
  const D = DT.data, G = DT.game, T = D.TOUGE;
  const TAU = Math.PI * 2;
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const wrapPi = (a) => { a = (a + Math.PI) % TAU; if (a < 0) a += TAU; return a - Math.PI; };

  /* A winding road that always makes progress downhill, so it can never cross itself. */
  function buildRoad(rng) {
    const ds = T.STEP, kap = [];
    const push = (len, k) => { for (let i = Math.max(1, Math.round(len / ds)); i > 0; i--) kap.push(k); };
    // a bend that turns the road by exactly `turn`, no tighter than radius r
    const arc = (turn, r) => { const steps = Math.max(1, Math.ceil(Math.abs(turn) * r / ds)); push(steps * ds, turn / (steps * ds)); };
    push(T.START_STRAIGHT, 0);
    let side = rng() < 0.5 ? -1 : 1, cur = 0;
    while (kap.length * ds < T.LENGTH) {
      const progress = kap.length * ds / T.LENGTH;
      const target = side * (T.TURN_MIN + rng() * (T.TURN_MAX - T.TURN_MIN));
      const r = T.RADIUS_MIN + rng() * (T.RADIUS_MAX - T.RADIUS_MIN) * (1 - 0.5 * progress);   // tighter towards the bottom
      arc(target - cur, r);
      cur = target;
      push(T.STRAIGHT_MIN + rng() * (T.STRAIGHT_MAX - T.STRAIGHT_MIN), 0);
      if (rng() < 0.85) side = -side;
    }
    arc(-cur, 22);   // straighten out for the finish
    push(T.FINISH_STRAIGHT, 0);
    const n = kap.length, pts = [];
    let x = 0, z = 0, h = 0;
    for (let i = 0; i < n; i++) {
      let k = 0, c = 0;
      for (let j = -5; j <= 5; j++) if (i + j >= 0 && i + j < n) { k += kap[i + j]; c++; }   // ease in and out of bends
      k /= c;
      pts.push({ x, z, h, k });
      h += k * ds;
      x += Math.sin(h) * ds; z += Math.cos(h) * ds;
    }
    return { pts, ds, n, length: n * ds, halfW: T.HALF_WIDTH };
  }

  /* How this car and driver handle on the pass. */
  function carSetup(state, car) {
    const p = car.parts, d = G.driverOf(state, car), s = d ? G.driverStats(d) : { skill: 0, control: 0, flair: 0 };
    return {
      top: T.TOP_SPEED + T.TOP_PER_STAGE * (p.engine + p.turbo),
      accel: T.ACCEL,
      steer: T.STEER + T.STEER_PER_ANGLE * p.angle,
      grip: T.GRIP,
      maxSlip: T.MAX_SLIP + T.SLIP_PER_ANGLE * p.angle,
      hitKeep: Math.min(0.85, T.HIT_KEEP + T.HIT_KEEP_PER_SUSP * p.suspension),
      bonus: (1 + T.BONUS_PER_STANCE * p.stance + T.BONUS_PER_TIRES * p.tires) * (1 + s.skill / T.SKILL_DIV) * (1 + s.flair / T.FLAIR_DIV),
    };
  }

  function newRun(road, setup) {
    const at = Math.min(road.n - 1, T.START_OFFSET), p = road.pts[at];
    return { road, setup, x: p.x, z: p.z, h: p.h, va: p.h, speed: T.START_SPEED, steer: 0, idx: at, slip: 0, lateral: 0,
      comboT: 0, grace: 0, mult: 1, bestMult: 1, score: 0, hits: 0, hitT: 0, t: 0, hold: T.COUNTDOWN,
      drifting: false, done: false, finished: false };
  }

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
     Returns what happened: 'hit' (touched a barrier), 'finish'. */
  function step(run, input, dt) {
    const ev = [];
    if (run.done) return ev;
    if (run.hold > 0) { run.hold -= dt; return ev; }
    const s = run.setup;
    run.t += dt;
    run.steer += (clamp(input || 0, -1, 1) - run.steer) * (1 - Math.exp(-12 * dt));
    // the nose turns with the wheel; steering right swings the heading towards -x
    run.h -= run.steer * s.steer * (0.4 + 0.6 * Math.min(1, run.speed / 18)) * dt;
    let p = run.road.pts[run.idx];
    run.h = p.h + clamp(wrapPi(run.h - p.h), -1.75, 1.75);   // no spinning round to face uphill
    // the direction of travel chases the nose; the gap between them is the drift angle
    let slip = wrapPi(run.h - run.va);
    run.va += slip * (1 - Math.exp(-s.grip * dt));
    slip = wrapPi(run.h - run.va);
    if (Math.abs(slip) > s.maxSlip) { slip = Math.sign(slip) * s.maxSlip; run.va = run.h - slip; }
    run.slip = slip;
    const target = s.top * (1 - T.SLIDE_DRAG * Math.min(1, Math.abs(slip)));
    run.speed += (target - run.speed) * (1 - Math.exp(-s.accel * dt));
    run.x += Math.sin(run.va) * run.speed * dt;
    run.z += Math.cos(run.va) * run.speed * dt;

    // barriers on both sides of the road
    p = locate(run);
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
    }

    // score: speed x angle x multiplier while the car is properly sideways
    run.drifting = Math.abs(run.slip) > T.DRIFT_MIN && run.speed > T.MIN_SPEED && run.hitT <= 0;
    if (run.drifting) {
      run.grace = 0;
      run.comboT += dt;
      run.mult = Math.min(T.COMBO_MAX, 1 + run.comboT / T.COMBO_TIME);
      run.bestMult = Math.max(run.bestMult, run.mult);
      run.score += run.speed * Math.min(1, Math.abs(run.slip)) * T.SCORE_RATE * run.mult * s.bonus * dt;
    } else {
      run.grace += dt;
      if (run.grace > T.COMBO_GRACE) { run.comboT = 0; run.mult = 1; }
    }

    if (run.idx >= run.road.n - T.FINISH_MARGIN) { run.done = true; run.finished = true; ev.push('finish'); }
    return ev;
  }

  /* What a run is worth: points buy seconds of the team's long-run income. */
  function payout(state, run) {
    const clean = run.finished && run.hits === 0;
    const seconds = run.score / T.POINTS_PER_SECOND * (clean ? T.CLEAN_BONUS : 1);
    return { score: Math.floor(run.score), seconds, cash: seconds * G.expectedIncome(state), clean, xp: run.t * T.XP_MULT,
      bestMult: run.bestMult, hits: run.hits, finished: run.finished, time: run.t };
  }

  /* Pay the run out: cash to the team, seat time to the driver. */
  function collect(state, car, run) {
    const res = payout(state, run);
    G.earn(state, res.cash);
    const d = G.driverOf(state, car);
    res.driver = d;
    res.levels = d ? G.addXP(d, res.xp * (G.trait(d).xp || 1)) : 0;
    return res;
  }

  /* A simple driver for tests and tuning: steers at a point a little way down the road. */
  function autopilot(run, look) {
    const pts = run.road.pts, a = pts[Math.min(pts.length - 1, run.idx + (look || Math.round(7 + run.speed * 0.4)))];
    const want = Math.atan2(a.x - run.x, a.z - run.z);
    return clamp(-wrapPi(want - run.h) * 2.2, -1, 1);
  }

  DT.touge = { buildRoad, carSetup, newRun, step, payout, collect, autopilot, wrapPi };
})(typeof window !== 'undefined' ? window : globalThis);
