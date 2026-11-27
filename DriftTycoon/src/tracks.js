/* Drift Tycoon — the drift line of each venue. Pure geometry (no DOM, no Three.js).
   Each layout is drawn like a turtle would: go straight, or turn along an arc. Units are the venue's
   own (the lot is 34 x 72); x is across the lot, z runs down it towards the camera. Laps are clockwise.
   The same layouts are used by the venue view and, scaled up, by Track drifting. */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});
  const RAD = Math.PI / 180;

  const S = (len) => ['s', len];
  const R = (r, deg) => ['a', r, deg];    // turn right (clockwise)
  const L = (r, deg) => ['a', r, -deg];   // turn left
  /* A bulge to one side and back onto the line: `dir` 1 bulges right, -1 left. It is 4 r sin(a) long. */
  const bump = (r, a, dir) => [['a', r, dir * a], ['a', r, -dir * a], ['a', r, -dir * a], ['a', r, dir * a]];
  const bumpLen = (r, a) => 4 * r * Math.sin(a * RAD);
  /* A side of `len` with bulges in it, the rest of the length shared out as straights between them. */
  function side(len, bumps) {
    const used = bumps.reduce((t, b) => t + bumpLen(b[0], b[1]), 0), gap = (len - used) / (bumps.length + 1);
    let out = [];
    bumps.forEach((b) => { if (gap > 1e-6) out.push(S(gap)); out = out.concat(bump(b[0], b[1], b[2])); });
    if (gap > 1e-6) out.push(S(gap));
    return out;
  }

  const LAYOUTS = {
    /* Parking Lot: a box with a hairpin tongue cut into the left side and two kinks down the right. */
    lot: { start: [-8, -29], steps: [].concat(
      [S(16), R(5, 90)], side(48, [[7, 40, 1], [7, 40, 1]]), [R(5, 90), S(16), R(5, 90)],
      [S(15.5), R(4.25, 90), S(10.75), L(4.25, 180), S(10.75), R(4.25, 90), S(15.5), R(5, 90)]) },
    /* Docks: an S. A hairpin tongue from the right side and another from the left, past each other. */
    docks: { start: [-8, -29], steps: [].concat(
      [S(16), R(5, 90), S(24.75), R(4.25, 90), S(10.75), L(4.25, 180), S(10.75), R(4.25, 90), S(6.25), R(5, 90)],
      [S(16), R(5, 90), S(24.75), R(4.25, 90), S(10.75), L(4.25, 180), S(10.75), R(4.25, 90), S(6.25), R(5, 90)]) },
    /* Airfield: fast and flowing. Both long sides are a run of esses, with wide corners at the ends. */
    airfield: { start: [-5, -29], steps: [].concat(
      [S(10), R(6.5, 90)], side(45, [[7.5, 30, 1], [7.5, 30, -1], [7.5, 30, 1]]), [R(6.5, 90), S(10), R(6.5, 90)],
      side(45, [[7.5, 30, 1], [7.5, 30, -1], [7.5, 30, 1]]), [R(6.5, 90)]) },
    /* Pro Circuit: a deep C cut into the infield from the right (two lefts among the rights), a kink on the
       bottom straight and esses down the left. */
    pro: { start: [-4, -29], steps: [].concat(
      [S(10), R(6, 90), S(18), R(4.5, 90), S(3), L(4.5, 90), S(8), L(4.5, 90), S(3), R(4.5, 90), S(2), R(6, 90)],
      side(12, [[5, 20, 1]]), [R(6, 90)], side(46, [[8, 25, 1], [8, 25, -1]]), [R(6, 90), S(2)]) },
    /* Stadium: a chicane on the start straight, a slalom down the right, a hairpin tongue up from the bottom
       and a long sweep up the left. */
    stadium: { start: [-9, -29], steps: [].concat(
      side(17, [[5, 30, 1]]), [R(4.5, 90)], side(49, [[6, 25, 1], [6, 25, -1], [6, 25, 1]]), [R(4.5, 90)],
      [R(4.25, 90), S(16), L(4.25, 180), S(16), R(4.25, 90)], [R(4.5, 90)], side(49, [[9, 25, 1]]), [R(4.5, 90)]) },
  };

  /* Walk a layout in small steps. Returns points { x, z, th, k, s }: th is the heading (0 = +x, growing
     clockwise), k the curvature (positive = turning right), s the distance from the start. */
  function trace(id, fine) {
    const lay = LAYOUTS[id], ds = fine || 0.05, pts = [];
    let x = lay.start[0], z = lay.start[1], th = 0, s = 0;
    lay.steps.forEach((st) => {
      const len = st[0] === 's' ? st[1] : st[1] * Math.abs(st[2]) * RAD, k = st[0] === 's' ? 0 : Math.sign(st[2]) / st[1];
      const n = Math.max(1, Math.round(len / ds)), d = len / n;
      for (let i = 0; i < n; i++) {
        pts.push({ x, z, th, k, s });
        // exact step along a straight or an arc
        if (k === 0) { x += Math.cos(th) * d; z += Math.sin(th) * d; }
        else { const t2 = th + k * d; x += (Math.sin(t2) - Math.sin(th)) / k; z -= (Math.cos(t2) - Math.cos(th)) / k; th = t2; }
        s += d;
      }
    });
    return { pts, length: s, end: { x, z, th } };
  }

  /* `count` points spaced evenly round the lap. */
  function sample(id, count) {
    const t = trace(id), out = [];
    let j = 0;
    for (let i = 0; i < count; i++) {
      const s = (i / count) * t.length;
      while (j < t.pts.length - 1 && t.pts[j + 1].s <= s) j++;
      const a = t.pts[j], d = s - a.s;
      let x, z, th = a.th;
      if (a.k === 0) { x = a.x + Math.cos(th) * d; z = a.z + Math.sin(th) * d; }
      else { const t2 = th + a.k * d; x = a.x + (Math.sin(t2) - Math.sin(th)) / a.k; z = a.z - (Math.cos(t2) - Math.cos(th)) / a.k; th = t2; }
      out.push({ x, z, th, k: a.k, s });
    }
    return { pts: out, length: t.length };
  }

  DT.tracks = { LAYOUTS, trace, sample, ids: Object.keys(LAYOUTS) };
})(typeof window !== 'undefined' ? window : globalThis);
