/* Drift Tycoon — the 3D venue: track, cars, smoke, crashes, mechanics, crowd. Needs THREE (r128). */
(function (root) {
  'use strict';
  const DT = root.DT, THREE = root.THREE, D = DT.data, G = DT.game;
  const V3 = THREE.Vector3;

  const SKY = 0xa9def2, GRASS = 0x7cc45c;
  const LOT = { hx: 17, hz: 36 };          // half size of the asphalt
  const PIT_X = LOT.hx + 5.2;              // centre line of the pit tents
  const CAR_SCALE = 1.5;                   // cars and people are drawn big, toy-style, so they read on a phone
  const BOUNDS = { x0: -LOT.hx + 3, x1: LOT.hx - 3, z0: -LOT.hz + 3, z1: LOT.hz - 3 }; // where a spinning car may end up
  const TRACK = [[-11, -31], [0, -37], [11, -31], [15, -16], [6, -3], [15, 10], [13, 27], [0, 37], [-12, 29],
    [-16, 12], [-6, 0], [-15, -16]].map((p) => [p[0] * 0.78, p[1] * 0.78]);
  const YAW = 10 * Math.PI / 180, EL = 52 * Math.PI / 180;
  const TAU = Math.PI * 2;
  const CROWD_COLORS = [0xff5a36, 0x19b3f5, 0xffd23f, 0x7ed957, 0xb56bff, 0xff6fb5, 0xffffff, 0x2a3142, 0xff9f1c];
  const BANNERS = [['KUMO TIRES', '#ff5a36'], ['APEX OIL', '#1877d4'], ['TOFU EXPRESS', '#22b573'], ['SLIDEWAYS', '#8a4dff']];

  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const lerp = (a, b, t) => a + (b - a) * t;
  const rand = (a, b) => a + Math.random() * (b - a);
  const wrapPi = (a) => { a = (a + Math.PI) % TAU; if (a < 0) a += TAU; return a - Math.PI; };
  const ease = (t) => t * t * (3 - 2 * t);

  /* ---------- Small building helpers ---------- */

  const matCache = {};
  function mat(color, extra) {
    const key = color + (extra ? JSON.stringify(extra) : '');
    if (!matCache[key]) matCache[key] = new THREE.MeshLambertMaterial(Object.assign({ color }, extra || {}));
    return matCache[key];
  }
  const UNIT_BOX = new THREE.BoxGeometry(1, 1, 1);
  const WHITE = new THREE.MeshLambertMaterial({ color: 0xffffff });
  const tmpObj = new THREE.Object3D(), tmpCol = new THREE.Color();

  function mesh(geo, material, x, y, z, parent, shadow) {
    const m = new THREE.Mesh(geo, material);
    m.position.set(x, y, z);
    if (shadow) m.castShadow = true;
    if (parent) parent.add(m);
    return m;
  }
  const box = (w, h, d, color, x, y, z, parent, shadow) =>
    mesh(new THREE.BoxGeometry(w, h, d), mat(color), x, y, z, parent, shadow);

  /* Many coloured copies of one shape in a single draw call. */
  function Batch(geo, material) { this.geo = geo || UNIT_BOX; this.material = material || WHITE; this.items = []; }
  Batch.prototype.add = function (w, h, d, color, x, y, z, ry) { this.items.push([w, h, d, color, x, y, z, ry || 0]); return this; };
  Batch.prototype.build = function (parent, shadow) {
    if (!this.items.length) return null;
    const m = new THREE.InstancedMesh(this.geo, this.material, this.items.length);
    this.items.forEach((it, i) => {
      tmpObj.position.set(it[4], it[5], it[6]);
      tmpObj.rotation.set(0, it[7], 0);
      tmpObj.scale.set(it[0], it[1], it[2]);
      tmpObj.updateMatrix();
      m.setMatrixAt(i, tmpObj.matrix);
      m.setColorAt(i, tmpCol.set(it[3]));
    });
    m.frustumCulled = false;
    m.castShadow = shadow !== false;
    m.receiveShadow = true;
    parent.add(m);
    return m;
  };

  function textTexture(text, bg, fg, w, h, size) {
    const c = document.createElement('canvas');
    c.width = w; c.height = h;
    const g = c.getContext('2d');
    g.fillStyle = bg; g.fillRect(0, 0, w, h);
    g.fillStyle = fg; g.textAlign = 'center'; g.textBaseline = 'middle';
    g.font = '700 ' + size + 'px Fredoka, "Arial Rounded MT Bold", Arial, sans-serif';
    g.fillText(text, w / 2, h / 2 + size * 0.04);
    const t = new THREE.CanvasTexture(c);
    t.anisotropy = 4;
    return t;
  }

  function disposeGroup(group) {
    group.traverse((o) => {
      if (o.geometry && o.geometry !== UNIT_BOX && !o.geometry.userData.shared) o.geometry.dispose();
      if (o.material && o.material.map) { o.material.map.dispose(); o.material.dispose(); }
    });
    while (group.children.length) group.remove(group.children[0]);
  }

  /* ---------- The drift line ---------- */

  function buildPath() {
    const curve = new THREE.CatmullRomCurve3(TRACK.map((p) => new V3(p[0], 0, p[1])), true, 'centripetal');
    curve.arcLengthDivisions = 2400;
    const N = 720;
    const pts = curve.getSpacedPoints(N);
    pts.pop();
    const L = curve.getLength(), ds = L / N;
    const ang = new Float32Array(N), raw = new Float32Array(N), kap = new Float32Array(N);
    for (let i = 0; i < N; i++) {
      const a = pts[(i + 1) % N], b = pts[(i - 1 + N) % N];
      ang[i] = Math.atan2(-(a.z - b.z), a.x - b.x);
    }
    for (let i = 0; i < N; i++) raw[i] = wrapPi(ang[(i + 1) % N] - ang[(i - 1 + N) % N]) / (2 * ds);
    for (let i = 0; i < N; i++) {
      let sum = 0;
      for (let k = -6; k <= 6; k++) sum += raw[(i + k + N) % N];
      kap[i] = sum / 13;
    }
    return { pts, ang, kap, L, ds, N };
  }

  function samplePath(P, s, out) {
    const u = (((s % P.L) + P.L) % P.L) / P.ds;
    const i = Math.floor(u) % P.N, j = (i + 1) % P.N, f = u - Math.floor(u);
    out.x = lerp(P.pts[i].x, P.pts[j].x, f);
    out.z = lerp(P.pts[i].z, P.pts[j].z, f);
    out.ang = P.ang[i] + wrapPi(P.ang[j] - P.ang[i]) * f;
    out.kap = lerp(P.kap[i], P.kap[j], f);
    return out;
  }

  function nearestS(P, x, z) {
    let best = 0, bd = Infinity;
    for (let i = 0; i < P.N; i++) {
      const dx = P.pts[i].x - x, dz = P.pts[i].z - z, d = dx * dx + dz * dz;
      if (d < bd) { bd = d; best = i; }
    }
    return best * P.ds;
  }

  /* Corners: the points of tightest turn, used for cones, tire walls and kerbs. */
  function findCorners(P) {
    const out = [];
    for (let i = 0; i < P.N; i++) {
      const k = Math.abs(P.kap[i]);
      if (k < 0.045) continue;
      let peak = true;
      for (let d = -40; d <= 40 && peak; d++) if (Math.abs(P.kap[(i + d + P.N) % P.N]) > k) peak = false;
      if (peak) out.push(i);
    }
    return out;
  }

  /* ---------- Textures ---------- */

  function asphaltTexture(P) {
    const S = 16, w = LOT.hx * 2 * S, h = LOT.hz * 2 * S;
    const c = document.createElement('canvas');
    c.width = w; c.height = h;
    const g = c.getContext('2d');
    const X = (x) => (x + LOT.hx) * S, Z = (z) => (z + LOT.hz) * S;
    g.fillStyle = '#414a5c'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 9; i++) {   // patched repairs
      g.fillStyle = i % 2 ? 'rgba(255,255,255,0.035)' : 'rgba(0,0,0,0.06)';
      g.fillRect(rand(0, w), rand(0, h), rand(60, 220), rand(60, 260));
    }
    for (let i = 0; i < 7000; i++) {
      g.fillStyle = Math.random() < 0.5 ? 'rgba(255,255,255,0.05)' : 'rgba(0,0,0,0.08)';
      g.fillRect(Math.random() * w, Math.random() * h, 2, 2);
    }
    g.strokeStyle = 'rgba(255,255,255,0.5)'; g.lineWidth = 3;   // parking bays down the left side
    const bx0 = -LOT.hx + 0.6, bx1 = -LOT.hx + 5, bz = Math.floor((LOT.hz - 3) / 3.2) * 3.2;
    for (let z = -bz; z <= bz + 0.01; z += 3.2) {
      g.beginPath(); g.moveTo(X(bx0), Z(z)); g.lineTo(X(bx1), Z(z)); g.stroke();
    }
    g.beginPath(); g.moveTo(X(bx1), Z(-bz)); g.lineTo(X(bx1), Z(bz)); g.stroke();
    g.strokeStyle = 'rgba(255,205,60,0.75)'; g.lineWidth = 4;   // yellow edge line
    g.strokeRect(X(-LOT.hx + 0.6), Z(-LOT.hz + 0.6), (LOT.hx * 2 - 1.2) * S, (LOT.hz * 2 - 1.2) * S);
    const line = (width, style, jitter) => {                    // rubber laid down on the drift line
      g.strokeStyle = style; g.lineWidth = width * S; g.lineJoin = 'round';
      g.beginPath();
      P.pts.forEach((p, i) => {
        const jx = jitter ? Math.sin(i * 0.11 + jitter) * jitter * S : 0;
        if (i) g.lineTo(X(p.x) + jx, Z(p.z)); else g.moveTo(X(p.x) + jx, Z(p.z));
      });
      g.closePath(); g.stroke();
    };
    line(7.4, 'rgba(18,20,28,0.16)');
    line(4.4, 'rgba(18,20,28,0.2)');
    line(0.5, 'rgba(10,12,18,0.3)', 1.3);
    line(0.4, 'rgba(10,12,18,0.3)', -1.1);
    const t = new THREE.CanvasTexture(c);
    t.anisotropy = 8;
    return t;
  }

  /* ---------- Car models ---------- */

  function taperedBox(len, h, wid, tf, tr, squeeze) {
    const geo = new THREE.BoxGeometry(len, h, wid);
    const p = geo.attributes.position;
    for (let i = 0; i < p.count; i++) {
      if (p.getY(i) > 0) {
        p.setX(i, p.getX(i) > 0 ? p.getX(i) - tf : p.getX(i) + tr);
        p.setZ(i, p.getZ(i) * squeeze);
      }
    }
    geo.computeVertexNormals();
    return geo;
  }

  function carSignature(c) {
    return [c.model, c.color, c.parts.engine >= 2 ? 1 : 0, c.parts.engine >= 4 ? 1 : 0, c.parts.turbo >= 2 ? 1 : 0].join('|');
  }

  /* Local axes: +X is forward, +Y up. */
  function makeCarMesh(c) {
    const b = G.model(c.model).body;
    const g = new THREE.Group(), body = new THREE.Group();
    g.add(body);
    const paint = c.color;
    mesh(new THREE.BoxGeometry(b.len, 0.6, b.wid), mat(paint), 0, 0.62, 0, body, true);
    box(b.len + 0.14, 0.2, b.wid + 0.06, 0x2a3142, 0, 0.3, 0, body);
    const cabW = b.wid * 0.9;
    mesh(taperedBox(b.cabLen, b.cabH, cabW, b.tf, b.tr, 0.84), mat(0x22304a), b.cabOff, 0.92 + b.cabH / 2, 0, body, true);
    const roofLen = Math.max(0.55, b.cabLen - b.tf - b.tr + 0.12);
    box(roofLen, 0.07, cabW * 0.84 + 0.04, paint, b.cabOff + (b.tr - b.tf) / 2, 0.92 + b.cabH + 0.03, 0, body);
    const lamp = mat(0xfff1a8, { emissive: 0x9a8a4a }), tail = mat(0xff3b30, { emissive: 0x701010 });
    [-1, 1].forEach((s) => {
      mesh(new THREE.BoxGeometry(0.08, 0.16, 0.42), lamp, b.len / 2 + 0.01, 0.7, s * (b.wid / 2 - 0.32), body);
      mesh(new THREE.BoxGeometry(0.08, 0.14, 0.5), tail, -b.len / 2 - 0.01, 0.74, s * (b.wid / 2 - 0.34), body);
    });
    const wing = Math.max(b.wing, c.parts.engine >= 4 ? 2 : c.parts.engine >= 2 ? 1 : 0);
    if (wing === 1) box(0.36, 0.07, b.wid * 0.92, paint, -b.len / 2 + 0.24, 0.98, 0, body);
    if (wing === 2) {
      box(0.6, 0.08, b.wid + 0.1, 0x1b2130, -b.len / 2 + 0.3, 1.5, 0, body, true);
      [-1, 1].forEach((s) => box(0.12, 0.56, 0.08, 0x1b2130, -b.len / 2 + 0.34, 1.2, s * b.wid * 0.3, body));
    }
    if (c.parts.turbo >= 2) box(0.1, 0.2, b.wid * 0.6, 0xcfd6e2, b.len / 2 + 0.08, 0.36, 0, body);
    const flame = mesh(new THREE.ConeGeometry(0.2, 0.9, 6), new THREE.MeshBasicMaterial({ color: 0xffa21f }),
      -b.len / 2 - 0.5, 0.36, -b.wid * 0.25, body);
    flame.rotation.z = Math.PI / 2;
    flame.visible = false;

    const tireGeo = new THREE.CylinderGeometry(0.4, 0.4, 0.3, 12), rimGeo = new THREE.CylinderGeometry(0.22, 0.22, 0.32, 8);
    const wheels = [];
    [1, -1].forEach((fx) => [1, -1].forEach((side) => {
      const pivot = new THREE.Group(), camber = new THREE.Group();
      pivot.position.set(fx * b.len * 0.31, 0.4, side * (b.wid / 2 - 0.02));
      pivot.add(camber);
      const tire = mesh(tireGeo, mat(0x15181f), 0, 0, 0, camber);
      const rim = mesh(rimGeo, mat(0xdfe5ee), 0, 0, 0, camber);
      tire.rotation.x = rim.rotation.x = Math.PI / 2;
      g.add(pivot);
      wheels.push({ pivot, camber, front: fx > 0, side });
    }));
    return { g, body, wheels, flame, len: b.len, wid: b.wid };
  }

  function makeMechanic(color) {
    const g = new THREE.Group();
    box(0.5, 0.5, 0.36, 0x2a3142, 0, 0.25, 0, g);
    box(0.64, 0.72, 0.44, color, 0, 0.86, 0, g, true);
    mesh(new THREE.SphereGeometry(0.27, 8, 6), mat(0xf2c9a0), 0, 1.46, 0, g);
    box(0.5, 0.14, 0.56, color, 0.04, 1.68, 0, g);
    g.scale.setScalar(2);
    return g;
  }

  /* ---------- The world ---------- */

  function World(canvas, state, hooks) {
    hooks = hooks || {};
    const P = buildPath();
    const corners = findCorners(P);
    const tmp = { x: 0, z: 0, ang: 0, kap: 0 }, tmp2 = { x: 0, z: 0, ang: 0, kap: 0 };
    const v = new V3();

    const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: 'high-performance' });
    const small = Math.min(root.innerWidth, root.innerHeight) < 700;
    renderer.setPixelRatio(Math.min(root.devicePixelRatio || 1, 2));
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(SKY);
    scene.fog = new THREE.Fog(SKY, 200, 700);
    const camera = new THREE.PerspectiveCamera(30, 1, 5, 2500);

    scene.add(new THREE.HemisphereLight(0xffffff, 0x9bb07a, 0.72));
    const sun = new THREE.DirectionalLight(0xfff1d8, 0.62);
    sun.position.set(-55, 90, 40);
    sun.castShadow = true;
    sun.shadow.mapSize.set(small ? 1024 : 2048, small ? 1024 : 2048);
    Object.assign(sun.shadow.camera, { left: -70, right: 70, top: 80, bottom: -80, near: 10, far: 260 });
    sun.shadow.bias = -0.0006;
    sun.shadow.normalBias = 0.03;
    scene.add(sun);

    /* ----- Camera: fit the venue into the part of the screen the UI leaves free ----- */
    const fx0 = -LOT.hx - 8.5, fx1 = LOT.hx + 11, fz0 = -LOT.hz - 9, fz1 = LOT.hz + 1.5;
    const FIT = [[fx0, 0, fz0], [fx1, 0, fz0], [fx0, 0, fz1], [fx1, 0, fz1], [fx0, 7, fz0], [fx1, 7, fz0], [2, 10, -LOT.hz - 19]]
      .map((p) => new V3(p[0], p[1], p[2]));
    const camDir = new V3(Math.sin(YAW) * Math.cos(EL), Math.sin(EL), Math.cos(YAW) * Math.cos(EL));
    const target = new V3(1, 0, -3);
    const inset = { top: 0, bottom: 0, topT: 0, bottomT: 0 };
    let size = { w: 1, h: 1 }, needFit = true;

    function placeCamera(dist) {
      camera.position.copy(target).addScaledVector(camDir, dist);
      camera.lookAt(target);
      camera.updateMatrixWorld();
    }
    function fit() {
      const w = size.w, h = size.h;
      const uw = w, uh = Math.max(140, h - inset.top - inset.bottom);
      camera.aspect = uw / uh;
      camera.clearViewOffset();
      let lo = 30, hi = 1200, x0 = 0, x1 = 0, y0 = 0, y1 = 0;
      const fits = (dist) => {
        placeCamera(dist);
        x0 = y0 = Infinity; x1 = y1 = -Infinity;
        let ok = true;
        for (const p of FIT) {
          v.copy(p).project(camera);
          if (Math.abs(v.z) > 1) ok = false;
          x0 = Math.min(x0, v.x); x1 = Math.max(x1, v.x);
          y0 = Math.min(y0, v.y); y1 = Math.max(y1, v.y);
        }
        return ok && x1 - x0 <= 1.94 && y1 - y0 <= 1.92;   // only the spans matter: the picture is re-centred below
      };
      for (let i = 0; i < 24; i++) { const mid = (lo + hi) / 2; if (fits(mid)) hi = mid; else lo = mid; }
      fits(hi);
      const cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;   // slide the picture so it sits in the middle of the free area
      camera.setViewOffset(uw, uh, cx * uw / 2, -inset.top - cy * uh / 2, w, h);
      scene.fog.near = hi + 90; scene.fog.far = hi + 560;
      needFit = false;
    }
    function resize() {
      const w = canvas.clientWidth || 1, h = canvas.clientHeight || 1;
      if (w === size.w && h === size.h) return;
      size = { w, h };
      renderer.setSize(w, h, false);
      needFit = true;
    }
    function setInsets(top, bottom, instant) {
      inset.topT = top; inset.bottomT = bottom;
      if (instant) { inset.top = top; inset.bottom = bottom; needFit = true; }
    }
    function toScreen(x, y, z) {
      v.set(x, y, z).project(camera);
      return { x: (v.x + 1) / 2 * size.w, y: (1 - v.y) / 2 * size.h };
    }

    /* ----- Static scenery ----- */
    (function buildStatic() {
      const ground = mesh(new THREE.PlaneGeometry(2600, 2600), mat(GRASS), 0, -0.05, 0, scene);
      ground.rotation.x = -Math.PI / 2;
      ground.receiveShadow = true;
      const rim = box(LOT.hx * 2 + 2.4, 0.3, LOT.hz * 2 + 2.4, 0xb9c0cc, 0, -0.15, 0, scene);
      rim.receiveShadow = true;
      const lot = mesh(new THREE.PlaneGeometry(LOT.hx * 2, LOT.hz * 2), new THREE.MeshLambertMaterial({ map: asphaltTexture(P) }),
        0, 0.03, 0, scene);
      lot.rotation.x = -Math.PI / 2;
      lot.receiveShadow = true;
      const pad = box(10.4, 0.3, LOT.hz * 2 - 2, 0xc9cfda, PIT_X + 0.2, -0.12, 0, scene);   // pit lane
      pad.receiveShadow = true;

      const b = new Batch();
      for (let i = 0, x = -LOT.hx + 1.5; x < LOT.hx; x += 3, i++) b.add(2.9, 1, 0.6, i % 2 ? 0xe5484d : 0xf1f3f6, x, 0.5, -LOT.hz - 0.7);
      for (let i = 0, z = -LOT.hz + 1.5; z < LOT.hz; z += 3, i++) b.add(0.6, 1, 2.9, i % 2 ? 0xf1f3f6 : 0xe5484d, -LOT.hx - 0.7, 0.5, z);
      b.add(0.5, 0.8, LOT.hz * 2 - 2, 0xdfe4ec, PIT_X + 5.6, 0.4, 0);   // wall behind the pits
      // Team HQ behind the top stands
      const hqz = -LOT.hz - 22;
      b.add(30, 8, 11, 0xeef1f6, 2, 4, hqz).add(30.6, 0.6, 11.6, 0x2a3142, 2, 8.3, hqz).add(30.1, 1.2, 11.1, 0xff6b2c, 2, 6.2, hqz);
      [-8, 2, 12].forEach((x) => b.add(7, 4.6, 0.3, 0x39445a, x, 2.3, hqz + 5.6));
      // Road on the left
      const roadX = -LOT.hx - 26;
      b.add(9, 0.06, 700, 0x4b5366, roadX, -0.01, -60);
      for (let z = -400; z < 280; z += 9) b.add(0.3, 0.07, 3.4, 0xf1f3f6, roadX, 0, z);
      // Tree trunks
      const crowns = new Batch(new THREE.IcosahedronGeometry(1, 0), WHITE);
      const greens = [0x3fa84f, 0x4dbb5e, 0x2f9146, 0x66c35a];
      let placed = 0;
      for (let tries = 0; tries < 900 && placed < 90; tries++) {
        const x = rand(-150, 130), z = rand(-230, 110);
        if (x > -LOT.hx - 17 && x < LOT.hx + 16 && z > -LOT.hz - 34 && z < LOT.hz + 6) continue;
        if (Math.abs(x - roadX) < 8) continue;
        const sc = rand(2.2, 3.8);
        b.add(0.7, sc * 1.1, 0.7, 0x8a5a3c, x, sc * 0.55, z);
        crowns.add(sc, sc * 1.15, sc, greens[placed % 4], x, sc * 1.6, z, rand(0, 3));
        if (placed % 3 === 0) crowns.add(1.3, 1, 1.3, greens[(placed + 1) % 4], x + rand(-6, 6), 0.6, z + rand(-6, 6), rand(0, 3));
        placed++;
      }
      b.build(scene);
      crowns.build(scene);
      const sign = mesh(new THREE.PlaneGeometry(12, 2), new THREE.MeshBasicMaterial({ map: textTexture('DRIFT HQ', '#2a3142', '#ffcf33', 512, 96, 64) }),
        2, 6.2, -LOT.hz - 22 + 5.65, scene);
      sign.renderOrder = 1;
    })();

    /* ----- Venue pieces that change with upgrades ----- */
    const venueGroup = new THREE.Group();
    scene.add(venueGroup);
    let venueSig = '', blimp = null;
    const crowd = { mesh: null, n: 0, baseY: null, phase: null };
    const CROWD_MAX = 320;
    crowd.mesh = new THREE.InstancedMesh(UNIT_BOX, WHITE, CROWD_MAX);
    crowd.mesh.frustumCulled = false;
    crowd.mesh.castShadow = true;
    crowd.baseY = new Float32Array(CROWD_MAX);
    crowd.phase = new Float32Array(CROWD_MAX);
    for (let i = 0; i < CROWD_MAX; i++) crowd.mesh.setColorAt(i, tmpCol.set(CROWD_COLORS[i % CROWD_COLORS.length]));
    crowd.mesh.count = 0;
    scene.add(crowd.mesh);

    const tentZ = (i) => -29 + i * 11.5;
    const STAND_MODULES = [['L', -6.5], ['L', 6.5], ['T', -6.5], ['L', -19.5], ['L', 19.5], ['T', 6.5], ['L', -32.5], ['L', 32.5]];

    function addPerson(x, y, z, ry) {
      if (crowd.n >= CROWD_MAX) return;
      tmpObj.position.set(x, y + 0.75, z);
      tmpObj.rotation.set(0, ry, 0);
      tmpObj.scale.set(0.8, 1.5, 0.6);
      tmpObj.updateMatrix();
      crowd.mesh.setMatrixAt(crowd.n, tmpObj.matrix);
      crowd.baseY[crowd.n] = y + 0.75;
      crowd.phase[crowd.n] = rand(0, TAU);
      crowd.n++;
    }

    function buildVenue() {
      disposeGroup(venueGroup);
      blimp = null;
      crowd.n = 0;
      const b = new Batch();
      const lvl = state.venue;

      // Pits: one tent per unlocked pit, in its car's colour
      for (let i = 0; i < state.slots; i++) {
        const z = tentZ(i), car = state.cars[i];
        [[-3.6, -3.3], [3.6, -3.3], [-3.6, 3.3], [3.6, 3.3]].forEach((p) => b.add(0.26, 4, 0.26, 0xdfe4ec, PIT_X + p[0], 2, z + p[1]));
        b.add(7.8, 0.4, 7.2, car ? car.color : 0xf1f3f6, PIT_X, 4.2, z);
        b.add(5.4, 0.34, 5, 0xffffff, PIT_X, 4.56, z);
        b.add(0.8, 1.2, 1.7, 0xe5484d, PIT_X + 4.6, 0.6, z - 1.8);
        b.add(1.1, 0.6, 1.1, 0x15181f, PIT_X + 4.6, 0.3, z + 1.6).add(1.1, 0.6, 1.1, 0x15181f, PIT_X + 4.6, 0.92, z + 1.6);
      }

      // Grandstands: a module per level, then roofs
      if (lvl.stands === 0) {
        for (let i = 0; i < 10; i++) addPerson(-LOT.hx - 2.2 + rand(-0.5, 0.5), 0, -16 + i * 3.4 + rand(-0.8, 0.8), Math.PI / 2);
      }
      STAND_MODULES.forEach((mod, j) => {
        if (lvl.stands <= j) return;
        const left = mod[0] === 'L', ry = left ? Math.PI / 2 : 0;
        const place = (lx, lz) => (left ? [-LOT.hx - 2.4 + lz, mod[1] - lx] : [mod[1] + lx, -LOT.hz - 2.4 + lz]);
        for (let k = 0; k < 3; k++) {
          const p = place(0, -k * 1.8 - 0.9);
          b.add(12, 0.9 * (k + 1), 1.8, k % 2 ? 0xcfd5df : 0xe1e6ee, p[0], 0.45 * (k + 1), p[1], ry);
          for (let n = -4; n <= 4; n++) {
            const q = place(n * 1.3 + rand(-0.15, 0.15), -k * 1.8 - 0.9);
            addPerson(q[0], 0.9 * (k + 1), q[1], ry);
          }
        }
        if (lvl.stands >= STAND_MODULES.length + 1 + j) {
          const r = place(0, -2.8), c = j % 2 ? 0x19b3f5 : 0xff6b2c;
          b.add(12.6, 0.3, 6.2, c, r[0], 6.2, r[1], ry);
          [-5.8, 5.8].forEach((lx) => { const q = place(lx, -5.6); b.add(0.3, 6.2, 0.3, 0xdfe4ec, q[0], 3.1, q[1], ry); });
        }
      });
      crowd.mesh.count = crowd.n;
      crowd.mesh.instanceMatrix.needsUpdate = true;

      // Sponsors: banners, start arch, flags, blimp
      for (let i = 0; i < Math.min(4, lvl.sponsors); i++) {
        const tex = textTexture(BANNERS[i][0], BANNERS[i][1], '#ffffff', 512, 96, 58);
        mesh(new THREE.PlaneGeometry(7.8, 1.5), new THREE.MeshBasicMaterial({ map: tex }), -12.3 + i * 8.2, 1.9, -LOT.hz - 0.35, venueGroup);
      }
      if (lvl.sponsors >= 3) {
        const tube = mat(0xff6b2c);
        const az = -12.5, ax = 11.2;
        [ax - 5.6, ax + 5.6].forEach((x) => mesh(new THREE.CylinderGeometry(0.75, 0.9, 7.4, 10), tube, x, 3.7, az, venueGroup, true));
        const beam = mesh(new THREE.CylinderGeometry(0.8, 0.8, 12.6, 10), tube, ax, 7.6, az, venueGroup, true);
        beam.rotation.z = Math.PI / 2;
        mesh(new THREE.PlaneGeometry(7, 1.1), new THREE.MeshBasicMaterial({ map: textTexture('SEND IT', '#ffffff', '#ff6b2c', 384, 64, 48) }),
          ax, 7.6, az + 0.85, venueGroup);
      }
      if (lvl.sponsors >= 5) {
        for (let i = 0; i < 7; i++) {
          const z = -LOT.hz + 4 + i * 11, x = PIT_X + 6;
          b.add(0.16, 7, 0.16, 0xdfe4ec, x, 3.5, z).add(0.1, 3.8, 1.4, CROWD_COLORS[i % 6], x, 5, z + 0.75);
        }
      }
      if (lvl.sponsors >= 6) {
        blimp = new THREE.Group();
        const hull = mesh(new THREE.SphereGeometry(1, 14, 10), mat(0xf6f8fc), 0, 0, 0, blimp, true);
        hull.scale.set(8, 2.8, 2.8);
        const band = mesh(new THREE.SphereGeometry(1, 14, 10), mat(0xff6b2c), 0, 0, 0, blimp);
        band.scale.set(3, 2.86, 2.86);
        box(2.6, 0.9, 1.1, 0x2a3142, 0, -3, 0, blimp);
        box(2.2, 2.6, 0.2, 0xff6b2c, -7, 1, 0, blimp);
        box(2.2, 0.2, 2.6, 0xff6b2c, -7, 0, 0, blimp);
        blimp.scale.setScalar(0.6);
        venueGroup.add(blimp);
      }

      // Track prep: cones always, then tire walls, lights, kerbs, a board
      const cones = new Batch(new THREE.ConeGeometry(0.5, 1, 8), WHITE);
      corners.forEach((ci, n) => {
        const p = P.pts[ci], a = P.ang[ci], sg = Math.sign(P.kap[ci]);
        const ix = -Math.sin(a) * sg, iz = -Math.cos(a) * sg;   // towards the inside of the corner
        cones.add(1.1, 1.5, 1.1, 0xff7a1a, p.x + ix * 4.2, 0.78, p.z + iz * 4.2);
        b.add(1.2, 0.12, 1.2, 0xff7a1a, p.x + ix * 4.2, 0.08, p.z + iz * 4.2);
        if (lvl.prep >= 1) {
          for (let k = -2; k <= 2; k++) {
            const q = P.pts[(ci + k * 9 + P.N) % P.N], qa = P.ang[(ci + k * 9 + P.N) % P.N];
            const ox = Math.sin(qa) * sg, oz = Math.cos(qa) * sg;
            const wx = clamp(q.x + ox * 7.4, -LOT.hx + 1, LOT.hx - 1), wz = clamp(q.z + oz * 7.4, -LOT.hz + 1, LOT.hz - 1);
            if (P.pts.some((t) => Math.hypot(t.x - wx, t.z - wz) < 6)) continue;   // never on another part of the drift line
            b.add(1.4, 0.6, 1.4, 0x15181f, wx, 0.33, wz, qa).add(1.4, 0.6, 1.4, (k + n) % 2 ? 0xf1f3f6 : 0xe5484d, wx, 0.95, wz, qa);
          }
        }
        if (lvl.prep >= 3) {
          for (let k = -5; k <= 5; k++) {
            const idx = (ci + k * 5 + P.N) % P.N, q = P.pts[idx], qa = P.ang[idx];
            b.add(1.5, 0.1, 0.9, k % 2 ? 0xf1f3f6 : 0xe5484d, q.x - Math.sin(qa) * sg * 3.5, 0.08, q.z - Math.cos(qa) * sg * 3.5, qa);
          }
        }
      });
      cones.build(venueGroup);
      const tx = LOT.hx - 0.9, tz = LOT.hz - 0.9;
      const towers = lvl.prep >= 5 ? [[-tx, -tz], [tx, -tz], [-tx, tz], [tx, tz], [-tx, 0], [tx, 0]]
        : lvl.prep >= 2 ? [[-tx, -tz], [tx, -tz], [-tx, tz], [tx, tz]] : [];
      towers.forEach((t) => {
        b.add(0.4, 12, 0.4, 0x8d99ad, t[0], 6, t[1]);
        mesh(new THREE.BoxGeometry(2.6, 1, 0.7), mat(0xfff6c9, { emissive: 0xbfb27a }), t[0], 12.3, t[1], venueGroup).rotation.y = t[0] < 0 ? 0.6 : -0.6;
      });
      if (lvl.prep >= 4) {
        const bx = -LOT.hx + 5, bzz = -LOT.hz + 1.6;
        b.add(0.3, 5, 0.3, 0x8d99ad, bx - 3, 2.5, bzz).add(0.3, 5, 0.3, 0x8d99ad, bx + 3, 2.5, bzz).add(7.4, 3.4, 0.4, 0x15181f, bx, 6.4, bzz);
        mesh(new THREE.PlaneGeometry(7, 3), new THREE.MeshBasicMaterial({ map: textTexture('BIG ANGLE', '#15181f', '#ffcf33', 384, 160, 62) }),
          bx, 6.4, bzz + 0.22, venueGroup);
      }
      b.build(venueGroup);
    }

    /* ----- Smoke and tire marks ----- */
    const SMOKE_N = 560;
    const smoke = {
      mesh: new THREE.InstancedMesh(new THREE.IcosahedronGeometry(1, 0),
        new THREE.MeshLambertMaterial({ color: 0xffffff, emissive: 0x666666, transparent: true, opacity: 0.72, depthWrite: false }), SMOKE_N),
      p: new Float32Array(SMOKE_N * 9), next: 0,   // x y z vx vy vz age life size
    };
    smoke.mesh.frustumCulled = false;
    smoke.mesh.renderOrder = 3;
    for (let i = 0; i < SMOKE_N; i++) { smoke.mesh.setColorAt(i, tmpCol.set(0xffffff)); smoke.p[i * 9 + 6] = 1; smoke.p[i * 9 + 7] = 0; }
    smoke.mesh.instanceMatrix.array.fill(0);
    scene.add(smoke.mesh);
    function puff(x, y, z, vx, vy, vz, sizeV, life, color) {
      const i = smoke.next; smoke.next = (i + 1) % SMOKE_N;
      const o = i * 9, p = smoke.p;
      p[o] = x; p[o + 1] = y; p[o + 2] = z; p[o + 3] = vx; p[o + 4] = vy; p[o + 5] = vz; p[o + 6] = 0; p[o + 7] = life; p[o + 8] = sizeV;
      smoke.mesh.setColorAt(i, tmpCol.set(color));
      smoke.mesh.instanceColor.needsUpdate = true;
    }
    function updateSmoke(dt) {
      const p = smoke.p, a = smoke.mesh.instanceMatrix.array, damp = Math.exp(-1.8 * dt);
      for (let i = 0; i < SMOKE_N; i++) {
        const o = i * 9, m = i * 16;
        let sc = 0;
        if (p[o + 6] < p[o + 7]) {
          p[o + 6] += dt;
          p[o] += p[o + 3] * dt; p[o + 1] += p[o + 4] * dt; p[o + 2] += p[o + 5] * dt;
          p[o + 3] *= damp; p[o + 5] *= damp;
          const t = p[o + 6] / p[o + 7];
          sc = t >= 1 ? 0 : p[o + 8] * (0.45 + 1.5 * t) * (t > 0.6 ? (1 - t) / 0.4 : 1);
        }
        a[m] = sc; a[m + 5] = sc; a[m + 10] = sc; a[m + 12] = p[o]; a[m + 13] = p[o + 1]; a[m + 14] = p[o + 2]; a[m + 15] = 1;
      }
      smoke.mesh.instanceMatrix.needsUpdate = true;
    }

    const MARK_N = 1300;
    const markGeo = new THREE.PlaneGeometry(1, 1);
    markGeo.rotateX(-Math.PI / 2);
    const marks = {
      mesh: new THREE.InstancedMesh(markGeo,
        new THREE.MeshBasicMaterial({ color: 0x10131a, transparent: true, opacity: 0.4, depthWrite: false }), MARK_N),
      next: 0,
    };
    marks.mesh.frustumCulled = false;
    marks.mesh.renderOrder = 1;
    marks.mesh.instanceMatrix.array.fill(0);
    scene.add(marks.mesh);
    function mark(x, z, ang, len) {
      tmpObj.position.set(x, 0.06, z);
      tmpObj.rotation.set(0, ang, 0);
      tmpObj.scale.set(len, 1, 0.46);
      tmpObj.updateMatrix();
      marks.mesh.setMatrixAt(marks.next, tmpObj.matrix);
      marks.next = (marks.next + 1) % MARK_N;
      marks.mesh.instanceMatrix.needsUpdate = true;
    }

    /* ----- Cars ----- */
    const cars = new Map();
    let time = 0, speed0 = 15;

    function tentSpot(i) { return { x: PIT_X, z: tentZ(i), h: Math.PI }; }

    function ensureCar(sc, index) {
      let c = cars.get(sc.id);
      const sig = carSignature(sc);
      if (!c) {
        const spot = tentSpot(index);
        c = { id: sc.id, state: 'parked', s: 0, slip: 0, x: spot.x, z: spot.z, heading: spot.h, vx: 0, vz: 0, spin: 0, t: 0,
          accrued: 0, popT: rand(0.6, 1.6), grace: 4, markD: 0, flameT: 0, lastSign: 0, repairT: 0, repairDur: 1, mech: null,
          from: null, to: null, dur: 1, m: null, sig: '' };
        cars.set(sc.id, c);
      }
      if (c.sig !== sig) {
        if (c.m) { scene.remove(c.m.g); disposeGroup(c.m.g); }
        c.m = makeCarMesh(sc);
        c.m.g.scale.setScalar(CAR_SCALE);
        c.m.len *= CAR_SCALE; c.m.wid *= CAR_SCALE;
        scene.add(c.m.g);
        c.sig = sig;
      }
      c.sc = sc;
      c.index = index;
      c.lane = ((index % 3) - 1) * 1.1;
      return c;
    }

    function park(c) {
      const spot = tentSpot(c.index);
      c.state = 'parked'; c.x = spot.x; c.z = spot.z; c.heading = spot.h; c.slip = 0;
    }

    function syncCars() {
      const seen = new Set();
      state.cars.forEach((sc, i) => {
        const c = ensureCar(sc, i);
        seen.add(sc.id);
        if (sc.driver == null && (c.state === 'run' || c.state === 'queue' || c.state === 'merge')) park(c);
        if (sc.driver == null && c.state === 'parked') park(c);
        if (sc.driver != null && c.state === 'parked') { c.state = 'queue'; c.to = null; }
      });
      cars.forEach((c, id) => {
        if (seen.has(id)) return;
        if (c.mech) { c.mech.state = 'back'; c.mech.car = null; }
        scene.remove(c.m.g); disposeGroup(c.m.g);
        cars.delete(id);
      });
    }

    function arcGap(a, b) { let d = Math.abs(a - b) % P.L; return d > P.L / 2 ? P.L - d : d; }

    /* Is the stretch of track around `s` free of cars for the next `dur` seconds? */
    function gapClear(self, s, dur) {
      const back = speed0 * dur + 12;
      let ok = true;
      cars.forEach((o) => {
        if (o === self || !ok) return;
        if (o.state === 'run') {
          const ahead = (((s - o.s) % P.L) + P.L) % P.L;   // how far `s` is in front of the other car
          if (ahead < back || P.L - ahead < 11) ok = false;
        } else if (o.state === 'merge' && o.to && arcGap(o.to.s, s) < 12) ok = false;
      });
      return ok;
    }

    function startCrash(c) {
      samplePath(P, c.s, tmp);
      const sp = speed0 * 0.75, sg = Math.sign(tmp.kap) || 1;
      const tx = Math.cos(tmp.ang), tz = -Math.sin(tmp.ang);
      const ox = Math.sin(tmp.ang) * sg, oz = Math.cos(tmp.ang) * sg;   // outside of the corner
      c.vx = tx * sp * 0.7 + ox * sp * 0.55; c.vz = tz * sp * 0.7 + oz * sp * 0.55;
      c.spin = (Math.random() < 0.5 ? -1 : 1) * rand(9, 13);
      c.state = 'spin'; c.t = 0;
      for (let i = 0; i < 16; i++) puff(c.x + rand(-1, 1), rand(0.5, 1.5), c.z + rand(-1, 1), rand(-4, 4), rand(1, 4), rand(-4, 4), rand(0.7, 1.2), rand(0.7, 1.3), 0x4a4f5c);
      if (hooks.crash) hooks.crash(c.sc, c.x, c.z);
    }

    function wheelWorld(c, lx, lz, out) {
      const ch = Math.cos(c.heading), sh = Math.sin(c.heading);
      out.x = c.x + lx * ch + lz * sh;
      out.z = c.z - lx * sh + lz * ch;
      return out;
    }

    function updateCar(c, dt) {
      const sc = c.sc, parts = sc.parts, m = c.m;
      const maxSlip = (26 + parts.angle * 6) * Math.PI / 180;
      let steer = 0, smokeAmt = 0;

      if (c.state === 'run') {
        samplePath(P, c.s, tmp);
        samplePath(P, c.s + 5.5, tmp2);
        const v0 = speed0 * (1 - 0.22 * Math.min(1, Math.abs(tmp.kap) * 14));
        c.s = (c.s + v0 * dt) % P.L;
        const want = maxSlip * Math.tanh(tmp2.kap / 0.05);
        c.slip += (want - c.slip) * (1 - Math.exp(-dt * 5.5));
        const lx = -Math.sin(tmp.ang), lz = -Math.cos(tmp.ang);
        c.x = tmp.x + lx * c.lane; c.z = tmp.z + lz * c.lane;
        c.heading = tmp.ang + c.slip;
        steer = -c.slip * 0.85;
        smokeAmt = clamp((Math.abs(c.slip) - 0.1) / 0.45, 0, 1);
        // tire marks from the rear wheels
        c.markD += v0 * dt;
        if (smokeAmt > 0.15 && c.markD > 0.62) {
          c.markD = 0;
          [-1, 1].forEach((side) => { wheelWorld(c, -m.len * 0.31, side * m.wid * 0.46, tmp2); mark(tmp2.x, tmp2.z, tmp.ang, 0.85); });
        }
        // turbo flames when the car flicks the other way
        const sign = Math.sign(want);
        if (parts.turbo >= 2 && sign !== 0 && sign !== c.lastSign) c.flameT = 0.16;
        if (sign !== 0) c.lastSign = sign;
        // crash roll: only mid-drift, after a grace period
        c.grace -= dt;
        if (c.grace <= 0 && smokeAmt > 0.4 && Math.random() < G.carCrashRate(state, sc) * 1.5 * dt) startCrash(c);
        // earnings pop-ups
        c.popT -= dt;
        if (c.popT <= 0 && c.accrued >= 0.1) {
          if (hooks.popup) hooks.popup(c.x, 2.6, c.z, '+' + G.money(c.accrued, 1), 'cash');
          c.accrued = 0; c.popT = rand(1.5, 2.4);
        }
      } else if (c.state === 'spin') {
        c.t += dt;
        c.x += c.vx * dt; c.z += c.vz * dt;
        if (c.x < BOUNDS.x0 || c.x > BOUNDS.x1) { c.x = clamp(c.x, BOUNDS.x0, BOUNDS.x1); c.vx *= -0.4; }
        if (c.z < BOUNDS.z0 || c.z > BOUNDS.z1) { c.z = clamp(c.z, BOUNDS.z0, BOUNDS.z1); c.vz *= -0.4; }
        const d = Math.exp(-3.2 * dt);
        c.vx *= d; c.vz *= d;
        c.heading += c.spin * dt; c.spin *= Math.exp(-2.4 * dt);
        c.slip *= d;
        if (c.t < 0.6 && Math.random() < 40 * dt) puff(c.x, 0.5, c.z, rand(-2, 2), rand(1, 3), rand(-2, 2), rand(0.6, 1), rand(0.6, 1.1), 0xdfe3ea);
        if (c.t > 1.2) { c.state = 'wait'; c.t = 0; c.waitSince = time; }
      } else if (c.state === 'wait' || c.state === 'repair') {
        c.t += dt;
        if (c.t > 0.3) { c.t = 0; wheelWorld(c, m.len * 0.35, 0, tmp2); puff(tmp2.x, 1.1, tmp2.z, rand(-0.4, 0.4), rand(1.5, 2.6), rand(-0.4, 0.4), rand(0.4, 0.7), rand(0.9, 1.5), 0x3b404c); }
        if (c.state === 'repair') {
          c.repairT += dt;
          if (c.repairT >= c.repairDur) {
            if (c.mech) { c.mech.state = 'back'; c.mech.car = null; c.mech = null; }
            c.state = 'queue'; c.to = null;
            if (hooks.repaired) hooks.repaired(sc);
            if (hooks.popup) hooks.popup(c.x, 2.6, c.z, 'Fixed!', 'good');
          }
        }
      } else if (c.state === 'queue') {
        if (sc.driver == null) { park(c); }
        else {
          const s = (nearestS(P, c.x, c.z) + 7) % P.L;
          samplePath(P, s, tmp);
          const lx = -Math.sin(tmp.ang), lz = -Math.cos(tmp.ang);
          const tx = tmp.x + lx * c.lane, tz = tmp.z + lz * c.lane;
          const dur = clamp(Math.hypot(tx - c.x, tz - c.z) / 20, 0.6, 2.4);
          if (gapClear(c, s, dur)) {
            c.state = 'merge'; c.t = 0; c.dur = dur;
            c.from = { x: c.x, z: c.z, h: c.heading };
            c.to = { s, x: tx, z: tz, h: tmp.ang };
          }
        }
      } else if (c.state === 'merge') {
        c.t += dt;
        const k = ease(Math.min(1, c.t / c.dur));
        const dirH = Math.atan2(-(c.to.z - c.from.z), c.to.x - c.from.x);
        c.x = lerp(c.from.x, c.to.x, k); c.z = lerp(c.from.z, c.to.z, k);
        const aim = k < 0.7 ? dirH : c.to.h;
        c.heading += wrapPi(aim - c.heading) * (1 - Math.exp(-dt * 9));
        smokeAmt = 0.25;
        if (c.t >= c.dur) { c.state = 'run'; c.s = c.to.s; c.heading = c.to.h; c.slip = 0; c.grace = 5; c.to = null; }
      }

      // smoke from the rear tires
      if (smokeAmt > 0) {
        const rate = smokeAmt * Math.min(34, 18 + parts.tires * 3.2);
        const col = parts.tires >= 4 ? tmpCol.set(0xffffff).lerp(new THREE.Color(sc.color), 0.4).getHex() : 0xffffff;
        const big = 1 + parts.tires * 0.06;
        [-1, 1].forEach((side) => {
          if (Math.random() < rate * dt) {
            wheelWorld(c, -m.len * 0.33, side * m.wid * 0.46, tmp2);
            puff(tmp2.x, 0.6, tmp2.z, -Math.cos(c.heading) * 2 + rand(-1, 1), rand(1.1, 2.3), Math.sin(c.heading) * 2 + rand(-1, 1),
              rand(0.7, 1.15) * big, rand(0.75, 1.25), col);
          }
        });
      }

      // pose
      m.g.position.set(c.x, 0, c.z);
      m.g.rotation.y = c.heading;
      const drop = Math.min(0.17, parts.stance * 0.034), camber = Math.min(0.22, parts.stance * 0.044);
      m.body.position.y = -drop;
      m.body.rotation.x = clamp(c.slip * 0.14, -0.09, 0.09);
      m.wheels.forEach((w) => {
        w.pivot.position.y = 0.4;
        w.pivot.rotation.y = w.front ? steer : 0;
        w.camber.rotation.x = -w.side * camber;
      });
      c.flameT -= dt;
      m.flame.visible = c.flameT > 0;
    }

    /* ----- Mechanics ----- */
    const mechs = [];
    const MECH_COLORS = [0xff8a1f, 0x19b3f5, 0x7ed957, 0xffd23f, 0xb56bff];
    function mechHome(i) { return { x: PIT_X - 4.4, z: tentZ(0) + 4.4 + i * 2.6 }; }
    function syncMechs() {
      while (mechs.length > state.mech.count) {   // only after a reset
        const mm = mechs.pop();
        if (mm.car) { mm.car.mech = null; if (mm.car.state === 'repair') mm.car.state = 'wait'; }
        scene.remove(mm.g);
      }
      while (mechs.length < state.mech.count) {
        const i = mechs.length, h = mechHome(i);
        const mm = { g: makeMechanic(MECH_COLORS[i % MECH_COLORS.length]), x: h.x, z: h.z, state: 'idle', car: null, hop: rand(0, 6), i };
        scene.add(mm.g);
        mechs.push(mm);
      }
    }
    function updateMechs(dt) {
      // hand waiting cars to free mechanics, oldest crash first
      const waiting = [];
      cars.forEach((c) => { if (c.state === 'wait' && !c.mech) waiting.push(c); });
      waiting.sort((a, b) => a.waitSince - b.waitSince);
      mechs.forEach((mm) => {
        if ((mm.state === 'idle' || mm.state === 'back') && waiting.length) {
          const c = waiting.shift();
          mm.car = c; c.mech = mm; mm.state = 'go';
        }
      });
      mechs.forEach((mm) => {
        let tx = mm.x, tz = mm.z, moving = false;
        if (mm.state === 'go' && mm.car) { tx = mm.car.x; tz = mm.car.z; }
        else if (mm.state === 'back' || mm.state === 'go') { const h = mechHome(mm.i); tx = h.x; tz = h.z; if (mm.state === 'go') mm.state = 'back'; }
        if (mm.state === 'go' || mm.state === 'back') {
          const dx = tx - mm.x, dz = tz - mm.z, d = Math.hypot(dx, dz), stop = mm.state === 'go' ? 3.4 : 0.2;
          if (d > stop + 0.05) {   // the margin matters: stepping to exactly `stop` can leave d a hair above it for ever
            const step = Math.min(d - stop, 13 * dt);
            mm.x += dx / d * step; mm.z += dz / d * step;
            mm.g.rotation.y = Math.atan2(-dz, dx);
            moving = true;
          } else if (mm.state === 'go') {
            mm.state = 'work';
            mm.car.state = 'repair'; mm.car.repairT = 0; mm.car.repairDur = G.carRepairTime(state, mm.car.sc);
          } else { mm.state = 'idle'; mm.g.rotation.y = Math.PI; }
        }
        mm.hop += dt * (moving ? 16 : mm.state === 'work' ? 11 : 0);
        const hopY = moving ? Math.abs(Math.sin(mm.hop)) * 0.45 : mm.state === 'work' ? Math.abs(Math.sin(mm.hop)) * 0.22 : 0;
        mm.g.position.set(mm.x, hopY, mm.z);
      });
    }

    /* ----- Frame ----- */
    const world = { hype: 0.5 };

    function sync() {
      const sig = [state.slots, state.venue.stands, state.venue.sponsors, state.venue.prep].join('|') + '|' + state.cars.map((c) => c.color).join(',');
      if (sig !== venueSig) { venueSig = sig; buildVenue(); }
      syncCars();
      syncMechs();
    }

    function update(dt) {
      time += dt;
      resize();
      const k = 1 - Math.exp(-dt * 9);
      if (Math.abs(inset.topT - inset.top) > 0.5 || Math.abs(inset.bottomT - inset.bottom) > 0.5) {
        inset.top = lerp(inset.top, inset.topT, k); inset.bottom = lerp(inset.bottom, inset.bottomT, k);
        needFit = true;
      }
      if (needFit) fit();

      // one pace for the whole field, so cars keep their gaps; engines and turbos speed everyone up a little
      let lv = 0, n = 0;
      cars.forEach((c) => { if (c.state === 'run') { lv += c.sc.parts.engine + c.sc.parts.turbo; n++; } });
      speed0 = lerp(speed0, 15 + Math.min(7, n ? (lv / n) * 0.7 : 0), 1 - Math.exp(-dt));

      cars.forEach((c) => updateCar(c, dt));
      updateMechs(dt);
      updateSmoke(dt);

      // the crowd hops; harder when more style is on show
      const a = crowd.mesh.instanceMatrix.array, amp = 0.12 + 0.4 * world.hype;
      for (let i = 0; i < crowd.n; i++) a[i * 16 + 13] = crowd.baseY[i] + Math.max(0, Math.sin(time * 7 + crowd.phase[i])) * amp;
      if (crowd.n) crowd.mesh.instanceMatrix.needsUpdate = true;

      if (blimp) {
        const t = time * 0.07;
        blimp.position.set(Math.cos(t) * 30 - 4, 24, Math.sin(t) * 30 - 22);
        blimp.rotation.y = -t - Math.PI / 2;
      }
    }

    const indicatorList = [];
    function indicators() {
      indicatorList.length = 0;
      cars.forEach((c) => {
        if (c.state !== 'wait' && c.state !== 'repair') return;
        const p = toScreen(c.x, 4.6, c.z);
        indicatorList.push({ id: c.id, x: p.x, y: p.y, repairing: c.state === 'repair', p: c.state === 'repair' ? c.repairT / c.repairDur : 0 });
      });
      return indicatorList;
    }

    function runningIds() {
      const out = [];
      cars.forEach((c) => { if (c.state === 'run') out.push(c.id); });
      return out;
    }
    function status(id) { const c = cars.get(id); return c ? c.state : 'parked'; }
    function addEarnings(id, amount) { const c = cars.get(id); if (c) c.accrued += amount; }
    function pos(id) { const c = cars.get(id); return c ? { x: c.x, z: c.z } : null; }

    resize();
    sync();
    Object.assign(world, { sync, update, render: () => renderer.render(scene, camera), setInsets, toScreen, indicators,
      runningIds, status, addEarnings, pos, path: P, _cars: cars, _mechs: mechs, _crash: (id) => { const c = cars.get(id); if (c && c.state === 'run') startCrash(c); } });
    return world;
  }

  DT.World = World;
})(window);
