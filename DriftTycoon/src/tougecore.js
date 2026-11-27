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
  /* How hard the street is at a distance: 0 at the start, 1 at RAMP_KM, 2 from HARD_KM on. */
  const difficulty = (metres) => {
    const km = metres / 1000;
    return km <= T.RAMP_KM ? clamp(km / T.RAMP_KM, 0, 1) : 1 + clamp((km - T.RAMP_KM) / (T.HARD_KM - T.RAMP_KM), 0, 1);
  };
  const ramp = (v, d) => (d <= 1 ? v[0] + (v[1] - v[0]) * d : v[1] + (v[2] - v[1]) * (d - 1));

  /* An endless winding road. It always makes progress downhill, so it can never cross itself.
     Points are made on demand by extendRoad; the same seed always gives the same road. */
  function newRoad(rng) {
    const road = { pts: [], n: 0, ds: T.STEP, halfW: T.HALF_WIDTH, speed: { top: 1, perKm: T.SPEED_PER_KM },
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

  /* A venue's drift line as a road to drive: the layout scaled up, lapped for ever. Flat, clockwise. */
  function trackRoad(layoutId) {
    const scale = D.TRACK_SCALE, lap = Math.round(DT.tracks.trace(layoutId).length * scale / T.STEP);
    const s = DT.tracks.sample(layoutId, lap), base = [];
    for (let i = 0; i < lap; i++) {
      let k = 0;
      for (let j = -SMOOTH; j <= SMOOTH; j++) k += s.pts[(i + j + lap) % lap].k;   // ease in and out of bends, as on the street
      // headings: the layout's 0 is +x and grows clockwise; a road's 0 is +z and its direction is (sin h, cos h)
      base.push({ x: s.pts[i].x * scale, z: s.pts[i].z * scale, h: Math.PI / 2 - s.pts[i].th, k: -k / (2 * SMOOTH + 1) / scale });
    }
    const road = { pts: [], n: 0, ds: s.length * scale / lap, halfW: T.HALF_WIDTH, loop: { lap, base, id: layoutId },
      speed: { top: T.TRACK_SPEED[layoutId] || 1, perKm: T.TRACK_SPEED_PER_KM } };
    extendRoad(road, lap + 300);
    return road;
  }

  /* Make sure the road has at least `count` points. */
  function extendRoad(road, count) {
    if (road.loop) {   // another lap is the same points again, with the heading one full turn on
      const lp = road.loop;
      while (road.n < count) {
        const b = lp.base[road.n % lp.lap];
        road.pts.push({ x: b.x, z: b.z, h: b.h - 2 * Math.PI * Math.floor(road.n / lp.lap), k: b.k });
        road.n++;
      }
      return road;
    }
    if (!road.gen) return road;
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
  /* A part stage as far as handling goes: stages past HANDLING_STAGES count for only HANDLING_EXTRA each. */
  const handling = (stage) => Math.min(stage, T.HANDLING_STAGES) + Math.max(0, stage - T.HANDLING_STAGES) * T.HANDLING_EXTRA;
  function carSetup(state, car) {
    const parts = car.parts, d = G.driverOf(state, car), s = d ? G.driverStats(d) : { skill: 0, control: 0, flair: 0 };
    const p = { engine: handling(parts.engine), turbo: handling(parts.turbo), angle: handling(parts.angle), suspension: handling(parts.suspension),
      stance: parts.stance, tires: parts.tires };
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
      dist: 0, km: 0, kmMarks: [], drifting: false, done: false };
  }

  const multCap = (run) => T.COMBO_MAX + T.COMBO_MAX_PER_KM * run.km;
  /* Put a run `km` kilometres down its road, as if it had driven there scoring nothing (testing). */
  function skipAhead(run, km) {
    const idx = T.START_OFFSET + Math.round(km * 1000 / run.road.ds);
    extendRoad(run.road, idx + 400);
    const p = run.road.pts[idx];
    Object.assign(run, { idx, x: p.x, z: p.z, h: p.h, va: p.h, dist: (idx - T.START_OFFSET) * run.road.ds, km: Math.floor(km), kmMarks: new Array(Math.floor(km)).fill(0) });
    return run;
  }
  const hitsLeft = (run) => Math.max(0, T.MAX_HITS - run.hits);
  /* Laps of a track are counted at its start line, a little ahead of where the car sets off; the first crossing starts lap 1. */
  const lapsDone = (run) => (run.road.loop ? Math.max(0, Math.floor((run.idx - T.START_OFFSET - T.LAP_LINE) / run.road.loop.lap)) : 0);

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
     Returns what happened: 'hit' (touched a barrier), 'km' (passed a kilometre marker), 'out' (the last allowed touch: run over). */
  function step(run, input, dt) {
    const ev = [];
    if (run.done) return ev;
    if (run.hold > 0) { run.hold -= dt; return ev; }
    const s = run.setup;
    extendRoad(run.road, run.idx + 320);
    run.t += dt;
    run.steer += (clamp(input || 0, -1, 1) - run.steer) * (1 - Math.exp(-T.STEER_RESPONSE * dt));
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
    const sp = run.road.speed;   // the road gets faster the further along it goes, up to HARD_KM
    const top = s.top * sp.top * (1 + sp.perKm * clamp(run.dist / 1000, 0, T.HARD_KM));
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

    // kilometre markers: the multiplier cap and the pay rate rise
    const km = Math.floor(run.dist / 1000);
    if (km > run.km) {
      while (run.kmMarks.length < km) run.kmMarks.push(run.score);   // the score when each marker was passed
      run.km = km;
      ev.push('km');
    }
    return ev;
  }

  /* The pay rate of a kilometre: index 0 is the first kilometre. It holds at the last entry. */
  const payRate = (km) => T.PAY_PER_KM[clamp(km, 0, T.PAY_PER_KM.length - 1)];

  /* What a run is worth: the points of each kilometre buy seconds of the team's long-run income at that
     kilometre's pay rate. `perKm` lists each kilometre driven (the last one may be part of one). */
  /* Seconds of team income the run is worth so far. */
  function paySeconds(run) {
    let seconds = 0, from = 0;
    for (let k = 0; k <= run.kmMarks.length; k++) {
      const to = k < run.kmMarks.length ? run.kmMarks[k] : run.score;
      seconds += (to - from) * payRate(k) / T.POINTS_PER_SECOND; from = to;
    }
    return seconds;
  }
  function payout(state, run) {
    const income = G.expectedIncome(state), perKm = [];
    let seconds = 0, from = 0;
    for (let k = 0; k <= run.kmMarks.length; k++) {
      const to = k < run.kmMarks.length ? run.kmMarks[k] : run.score, rate = payRate(k), s = (to - from) * rate / T.POINTS_PER_SECOND;
      if (k < run.kmMarks.length || to > from) perKm.push({ km: k + 1, points: to - from, rate, cash: s * income });
      seconds += s; from = to;
    }
    return { score: Math.floor(run.score), seconds, cash: seconds * income, xp: run.t * T.XP_MULT, perKm, rate: payRate(run.km),
      bestMult: run.bestMult, hits: run.hits, out: run.hits >= T.MAX_HITS, time: run.t, dist: run.dist, laps: lapsDone(run) };
  }

  /* Pay the run out: cash to the team, seat time to the driver, the team's records and street cred. */
  function collect(state, car, run) {
    const res = payout(state, run), rec = state.street;
    G.earn(state, res.cash);
    const d = G.driverOf(state, car);
    res.driver = d;
    res.levels = d ? G.addXP(d, res.xp * (G.trait(d).xp || 1)) : 0;
    const best = (rec.best[run.map || 'pass'] = rec.best[run.map || 'pass'] || { score: 0, dist: 0 });   // records are kept per map
    res.map = run.map || 'pass';
    res.bestScore = res.score > best.score; res.bestDist = res.dist > best.dist;
    rec.runs++; best.score = Math.max(best.score, res.score); best.dist = Math.max(best.dist, res.dist);
    res.cred = G.credFor(res.dist, res.score, (res.bestScore ? 1 : 0) + (res.bestDist ? 1 : 0));
    res.rank = G.addCred(state, res.cred);   // { before, after, unlocked }
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

  /* A careful driver: it aims the direction of travel (not the nose) at the road ahead, so it holds a line
     at speed. Used to check that the hardest stretch of road can still be driven. */
  function steady(run) {
    const pts = run.road.pts, a = pts[Math.min(pts.length - 1, run.idx + Math.round(6 + run.speed * 0.5))];
    const want = Math.atan2(a.x - run.x, a.z - run.z);
    const nose = want + clamp(wrapPi(want - run.va) * 1.2, -0.9, 0.9);
    return clamp(-wrapPi(nose - run.h) * 2.5, -1, 1);
  }

  DT.touge = { handling, newRoad, trackRoad, lapsDone, extendRoad, difficulty, carSetup, newRun, skipAhead, step, payRate, paySeconds, payout, collect, multCap, hitsLeft, autopilot, drifter, steady, wrapPi };
})(typeof window !== 'undefined' ? window : globalThis);
