/* Drift Tycoon — Drift mode (code name "touge"), the 3D side: chase camera, smoke, steering and HUD.
   The rules are in tougecore.js. Two kinds of map:
   - Street: an endless mountain road at dusk, built in chunks a little ahead of the car and dropped behind it.
   - Track: a venue's own drift line at full size, flat and in daylight, built once and lapped.
   It draws with the venue's renderer while the venue is paused. */
(function (root) {
  'use strict';
  const DT = root.DT, THREE = root.THREE, D = DT.data, G = DT.game, C = DT.touge, K = DT.kit, T = D.TOUGE;
  const V3 = THREE.Vector3;
  const SKY = 0x262c5c, FOG = 0x3a4180, GROUND = 0x2e5a4d;
  const CHUNK = 120, AHEAD = 3, CLEAR = 450;   // metres of road per chunk, chunks kept ahead, how far along the road trees check for tarmac
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const rand = (a, b) => a + Math.random() * (b - a);
  const $ = (id) => document.getElementById(id);
  const PAD = { hx: 18.5, hz: 37.5 };          // a venue's asphalt, in venue units (the lot's footprint and a margin)
  const pick = (list) => list[Math.floor(Math.random() * list.length)];
  /* What a venue's track looks like to drive: barrier colours, gate colour. Ground and sky come from the venue's theme. */
  const TRACK_LOOK = {
    lot: { wall: [0xe5484d, 0xf1f3f6], gate: 0xff6b2c },
    docks: { wall: [0xffc233, 0x2a3142], gate: 0x2f8fff },
    airfield: { wall: [0xff6b2c, 0xf1f3f6], gate: 0x22b573 },
  };
  const CROWD = [0xff5a36, 0x19b3f5, 0xffd23f, 0x7ed957, 0xb56bff, 0xff6fb5, 0xffffff, 0x2a3142, 0xff9f1c];

  function roadTexture(track) {
    const c = document.createElement('canvas');
    c.width = 256; c.height = 256;
    const g = c.getContext('2d');
    g.fillStyle = track ? '#2d3242' : '#353b4f'; g.fillRect(0, 0, 256, 256);
    for (let i = 0; i < 900; i++) {
      g.fillStyle = Math.random() < 0.5 ? 'rgba(255,255,255,0.05)' : 'rgba(0,0,0,0.1)';
      g.fillRect(Math.random() * 256, Math.random() * 256, 2, 2);
    }
    if (track) {   // red and white kerbs down both edges
      for (let j = 0; j < 4; j++) { g.fillStyle = j % 2 ? '#eef1f6' : '#e5484d'; g.fillRect(3, j * 64, 15, 64); g.fillRect(238, j * 64, 15, 64); }
    } else {
      g.fillStyle = '#e9edf5'; g.fillRect(9, 0, 6, 256); g.fillRect(241, 0, 6, 256);   // edge lines
      g.fillStyle = '#ffcf33'; g.fillRect(125, 0, 6, 128);                              // centre dashes
    }
    const t = new THREE.CanvasTexture(c);
    t.wrapT = THREE.RepeatWrapping;
    t.anisotropy = 8;
    return t;
  }
  function glowTexture() {
    const c = document.createElement('canvas');
    c.width = c.height = 64;
    const g = c.getContext('2d'), r = g.createRadialGradient(32, 32, 2, 32, 32, 32);
    r.addColorStop(0, 'rgba(255,255,255,1)'); r.addColorStop(1, 'rgba(255,255,255,0)');
    g.fillStyle = r; g.fillRect(0, 0, 64, 64);
    return new THREE.CanvasTexture(c);
  }

  /* A pool of instanced blobs or strips that fade by shrinking: used for tire smoke. */
  function Puffs(scene, n) {
    const mesh = new THREE.InstancedMesh(new THREE.IcosahedronGeometry(1, 0),
      new THREE.MeshLambertMaterial({ color: 0xffffff, emissive: 0x777788, transparent: true, opacity: 0.7, depthWrite: false }), n);
    const p = new Float32Array(n * 9), col = new THREE.Color();
    let next = 0;
    mesh.frustumCulled = false;
    mesh.renderOrder = 3;
    mesh.instanceMatrix.array.fill(0);
    for (let i = 0; i < n; i++) mesh.setColorAt(i, col.set(0xffffff));
    scene.add(mesh);
    return {
      add(x, y, z, vx, vy, vz, size, life, color) {
        const o = next * 9;
        p[o] = x; p[o + 1] = y; p[o + 2] = z; p[o + 3] = vx; p[o + 4] = vy; p[o + 5] = vz; p[o + 6] = 0; p[o + 7] = life; p[o + 8] = size;
        mesh.setColorAt(next, col.set(color));
        mesh.instanceColor.needsUpdate = true;
        next = (next + 1) % n;
      },
      update(dt) {
        const a = mesh.instanceMatrix.array, damp = Math.exp(-1.6 * dt);
        for (let i = 0; i < n; i++) {
          const o = i * 9, m = i * 16;
          let sc = 0;
          if (p[o + 6] < p[o + 7]) {
            p[o + 6] += dt;
            p[o] += p[o + 3] * dt; p[o + 1] += p[o + 4] * dt; p[o + 2] += p[o + 5] * dt;
            p[o + 3] *= damp; p[o + 5] *= damp;
            const t = p[o + 6] / p[o + 7];
            sc = t >= 1 ? 0 : p[o + 8] * (0.45 + 1.6 * t) * (t > 0.6 ? (1 - t) / 0.4 : 1);
          }
          a[m] = sc; a[m + 5] = sc; a[m + 10] = sc; a[m + 12] = p[o]; a[m + 13] = p[o + 1]; a[m + 14] = p[o + 2]; a[m + 15] = 1;
        }
        mesh.instanceMatrix.needsUpdate = true;
      },
    };
  }

  function Touge(world, state, hooks) {
    const el = { root: $('touge'), app: $('app'), score: $('tg-score'), mult: $('tg-mult'), bar: $('tg-combo-bar'), dist: $('tg-dist'),
      msg: $('tg-msg'), speed: $('tg-speed'), angle: $('tg-angle'), lives: $('tg-lives') };
    const camera = new THREE.PerspectiveCamera(58, 1, 0.5, 900);
    const tmp = new THREE.Object3D(), basis = new THREE.Matrix4(), f = new V3(), up = new V3(), side = new V3(), look = new V3();
    let active = false, scene = null, run = null, car = null, carM = null, road = null, result = null;
    let drag = null, keyL = false, keyR = false;
    let camAng = 0, shake = 0, msgT = 0, lastMult = 1, markD = 0, went = false, shown = {};
    let puffs = null, marks = null, markNext = 0, ground = null, peaks = null, shared = null, endT = 0;
    let map = { kind: 'street', id: 'pass' }, slope = T.SLOPE, lapShown = 0, markN = 700;
    const chunks = new Map();
    const gy = (z) => -slope * z;   // the mountainside is one slope, so height depends only on z; a track is flat

    /* ---------- The scene: the mountain, or a venue ---------- */
    function build() {
      const track = map.kind === 'track', theme = track ? K.THEMES[map.id] : null;
      scene = new THREE.Scene();
      if (track) {
        scene.background = new THREE.Color(theme.sky);
        scene.fog = new THREE.Fog(theme.sky, 150, 640);
        scene.add(new THREE.HemisphereLight(0xffffff, 0x9bb07a, 0.8));
        const sun = new THREE.DirectionalLight(0xfff1d8, 0.6);
        sun.position.set(-60, 90, -40);
        scene.add(sun);
      } else {
        scene.background = new THREE.Color(SKY);
        scene.fog = new THREE.Fog(FOG, 60, 290);
        scene.add(new THREE.HemisphereLight(0xc3caff, 0x25423b, 0.9));
        const sun = new THREE.DirectionalLight(0xffb37a, 0.55);
        sun.position.set(-70, 45, -40);
        scene.add(sun);
      }

      // things every chunk shares; they are freed once, when the run is cleared
      const keep = (geo) => { geo.userData.shared = true; return geo; };
      shared = {
        road: new THREE.MeshLambertMaterial({ map: roadTexture(track), side: THREE.DoubleSide }),
        shoulder: new THREE.MeshLambertMaterial({ color: 0x4d5263, side: THREE.DoubleSide }),
        glow: new THREE.MeshBasicMaterial({ color: 0xffffff }),
        cone: keep(new THREE.ConeGeometry(1, 1, 7)), rock: keep(new THREE.IcosahedronGeometry(1, 0)),
      };

      ground = null; peaks = null;
      if (track) {   // the venue's surroundings with its asphalt on top; neither moves
        const flat = (w, d, color, y) => { const g = new THREE.PlaneGeometry(w, d); g.rotateX(-Math.PI / 2); return K.mesh(g, new THREE.MeshLambertMaterial({ color }), 0, y, 0, scene); };
        flat(3200, 3200, theme.ground, -0.1);
        flat(PAD.hx * 2 * D.TRACK_SCALE, PAD.hz * 2 * D.TRACK_SCALE, theme.asphalt, -0.05);
      } else {
        // the mountainside is one big tilted sheet that follows the car
        const sheet = new THREE.PlaneGeometry(1400, 1400, 1, 1);
        sheet.rotateX(-Math.PI / 2);
        const gp = sheet.attributes.position;
        for (let i = 0; i < gp.count; i++) gp.setY(i, gy(gp.getZ(i)) - 0.08);
        sheet.computeVertexNormals();
        ground = K.mesh(sheet, new THREE.MeshLambertMaterial({ color: GROUND }), 0, 0, 0, scene);
        // far peaks, in a ring that also follows the car
        peaks = new THREE.Group();
        const peak = new THREE.ConeGeometry(150, 210, 6), peakMat = new THREE.MeshLambertMaterial({ color: 0x2c3870 });
        for (let i = 0; i < 8; i++) {
          const a = (i + 0.5) / 8 * Math.PI * 2, r = rand(400, 470);
          K.mesh(peak, peakMat, Math.sin(a) * r, rand(10, 50) + gy(Math.cos(a) * r), Math.cos(a) * r, peaks).rotation.y = rand(0, 3);
        }
        scene.add(peaks);
      }

      // the car, with a shadow under it and a pool of headlight on the road ahead
      carM = K.makeCarMesh(car);
      const shadow = new THREE.PlaneGeometry(carM.len + 0.6, carM.wid + 0.5);
      shadow.rotateX(-Math.PI / 2);
      K.mesh(shadow, new THREE.MeshBasicMaterial({ color: 0x05070c, transparent: true, opacity: 0.35, depthWrite: false }), 0, 0.05, 0, carM.g);
      if (!track) {
        const beam = new THREE.PlaneGeometry(13, 8);
        beam.rotateX(-Math.PI / 2);
        K.mesh(beam, new THREE.MeshBasicMaterial({ map: glowTexture(), color: 0xfff0b8, transparent: true, opacity: 0.4, depthWrite: false,
          blending: THREE.AdditiveBlending }), carM.len / 2 + 6, 0.09, 0, carM.g).renderOrder = 2;
      }
      scene.add(carM.g);

      puffs = Puffs(scene, 240);
      const markGeo = new THREE.PlaneGeometry(1, 1);
      markGeo.rotateX(-Math.PI / 2);
      markN = track ? 3400 : 700;   // on a track the marks of the lap before are still there to drive over
      marks = new THREE.InstancedMesh(markGeo, new THREE.MeshBasicMaterial({ color: 0x0b0d14, transparent: true, opacity: 0.45, depthWrite: false }), markN);
      marks.frustumCulled = false;
      marks.renderOrder = 1;
      marks.instanceMatrix.array.fill(0);
      markNext = 0;
      scene.add(marks);
    }

    /* One stretch of road with everything beside it: points [a, b]. The street is built in chunks of these;
       a venue track is a single one, a whole lap long. */
    function buildSpan(a, b) {
      const hw = road.halfW, track = !!road.loop, style = track ? TRACK_LOOK[map.id] || TRACK_LOOK.lot : null;
      C.extendRoad(road, b + CLEAR + 4);
      const P = road.pts, group = new THREE.Group(), textures = [];
      const edge = (i, s, off) => { const p = P[i]; return { x: p.x + Math.cos(p.h) * s * off, z: p.z - Math.sin(p.h) * s * off }; };

      const ribbon = (half, lift, material) => {
        const rows = b - a + 1, pos = new Float32Array(rows * 6), uv = new Float32Array(rows * 4), idx = [];
        for (let r = 0; r < rows; r++) {
          const l = edge(a + r, -1, half), rt = edge(a + r, 1, half);
          pos.set([l.x, gy(l.z) + lift, l.z, rt.x, gy(rt.z) + lift, rt.z], r * 6);
          uv.set([0, (a + r) / 10, 1, (a + r) / 10], r * 4);
          if (r < rows - 1) idx.push(r * 2, r * 2 + 2, r * 2 + 1, r * 2 + 1, r * 2 + 2, r * 2 + 3);
        }
        const geo = new THREE.BufferGeometry();
        geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
        geo.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
        geo.setIndex(idx);
        geo.computeVertexNormals();
        const m = new THREE.Mesh(geo, material);
        m.frustumCulled = false;
        group.add(m);
      };
      if (!track) ribbon(hw + 1.6, 0, shared.shoulder);   // a track lies on the venue's asphalt
      ribbon(hw, 0.03, shared.road);

      // barriers (guard rail on the street, painted concrete on a track), reflectors, corner boards, lamps
      const solid = new K.Batch(), glow = new K.Batch(null, shared.glow);
      for (const s of [-1, 1]) {
        for (let i = a; i < b; i += 3) {
          const p0 = edge(i, s, hw + 0.4), p1 = edge(i + 3, s, hw + 0.4), mx = (p0.x + p1.x) / 2, mz = (p0.z + p1.z) / 2;
          const len = Math.hypot(p1.x - p0.x, p1.z - p0.z), ang = Math.atan2(-(p1.z - p0.z), p1.x - p0.x);
          if (track) { solid.add(len + 0.08, 0.8, 0.5, style.wall[(i / 3) % 2], mx, 0.4, mz, ang); continue; }
          solid.add(len + 0.12, 0.3, 0.12, 0xd9dfe9, mx, gy(mz) + 0.72, mz, ang);
          solid.add(0.16, 0.85, 0.16, 0x8f98ac, p0.x, gy(p0.z) + 0.42, p0.z);
          if (i % 9 === 0) glow.add(0.2, 0.2, 0.2, s < 0 ? 0xff6a3d : 0xffe08a, p0.x, gy(p0.z) + 0.98, p0.z);
        }
      }
      for (let i = Math.ceil(a / 7) * 7; i < b; i += 7) {
        const p = P[i];
        if (i < 30 || Math.abs(p.k) < 0.03) continue;
        const e = edge(i, p.k > 0 ? -1 : 1, hw + 1.5);   // outside of the bend
        glow.add(1.5, 0.9, 0.12, (i / 7) % 2 ? 0xffc233 : 0xff5a36, e.x, gy(e.z) + 1.7, e.z, p.h);
        solid.add(0.14, 1.3, 0.14, 0x8f98ac, e.x, gy(e.z) + 0.65, e.z);
      }
      for (let i = Math.ceil((a - 30) / 70) * 70 + 30; i < b; i += 70) {
        if (i < a) continue;
        const s = ((i - 30) / 70) % 2 ? -1 : 1, e = edge(i, s, hw + 1.3), arm = edge(i, s, hw - 0.4), p = P[i];
        solid.add(0.2, 6.4, 0.2, 0x8f98ac, e.x, gy(e.z) + 3.2, e.z);
        solid.add(2, 0.14, 0.14, 0x8f98ac, (e.x + arm.x) / 2, gy(e.z) + 6.3, (e.z + arm.z) / 2, p.h);
        glow.add(0.8, 0.2, 0.45, 0xffe6a8, arm.x, gy(arm.z) + 6.2, arm.z, p.h);
      }
      // gates: the start, then one at every kilometre of the street; a track has one, where laps are counted
      const gate = (i, text, color) => {
        const p = P[i];
        for (const s of [-1, 1]) { const e = edge(i, s, hw + 0.9); solid.add(0.5, 7, 0.5, color, e.x, gy(e.z) + 3.5, e.z, p.h); }
        solid.add(hw * 2 + 2.3, 1.5, 0.4, color, p.x, gy(p.z) + 6.6, p.z, p.h);
        const tex = K.textTexture(text, '#ffffff', '#1d2433', 512, 64, 46);
        textures.push(tex);
        const sign = K.mesh(new THREE.PlaneGeometry(9.6, 1.2), new THREE.MeshBasicMaterial({ map: tex }),
          p.x - Math.sin(p.h) * 0.24, gy(p.z) + 6.6, p.z - Math.cos(p.h) * 0.24, group);
        sign.rotation.y = p.h + Math.PI;
      };
      if (track) {
        const i = T.START_OFFSET + T.LAP_LINE, p = P[i], n = Math.round(hw * 2);
        gate(i, (D.VENUES.find((v) => v.id === map.id) || { name: 'Track' }).name.toUpperCase(), style.gate);
        for (let r = 0; r < 2; r++) {   // the chequered line under it
          for (let c = 0; c < n; c++) {
            const off = (c - (n - 1) / 2) * (hw * 2 / n), along = r - 0.5;
            solid.add(hw * 2 / n, 0.02, 1, (r + c) % 2 ? 0x15181f : 0xf4f6fa, p.x + Math.cos(p.h) * off + Math.sin(p.h) * along, 0.045, p.z - Math.sin(p.h) * off + Math.cos(p.h) * along, p.h);
          }
        }
      } else {
        const startAt = T.START_OFFSET + 34;
        if (startAt >= a && startAt < b) gate(startAt, 'STREET DRIFTING', 0x2f8fff);
        for (let km = Math.max(1, Math.ceil((a - T.START_OFFSET) / 1000)); T.START_OFFSET + km * 1000 < b; km++) {
          gate(T.START_OFFSET + km * 1000, km + ' KM', 0xff6b2c);
        }
      }

      // scenery, kept clear of every nearby part of the road
      const from = Math.max(0, a - CLEAR), to = Math.min(road.n, b + CLEAR);
      const near = (x, z, r) => { for (let i = from; i < to; i += 3) { const dx = P[i].x - x, dz = P[i].z - z; if (dx * dx + dz * dz < r * r) return true; } return false; };
      const crowns = new K.Batch(shared.cone, K.WHITE), rocks = new K.Batch(shared.rock, K.WHITE);
      if (track) venueScenery(solid, crowns, near, P, hw);
      else {
        const greens = [0x1f5a48, 0x26694f, 0x1a4d44, 0x2f7a56], greys = [0x626a82, 0x4f5670, 0x767e96];
        for (let i = a; i < b; i += 4) {
          for (const s of [-1, 1]) {
            for (let t = 0; t < 2; t++) {
              const e = edge(i, s, hw + 4 + Math.pow(Math.random(), 1.6) * 60);
              if (near(e.x, e.z, hw + 3.6)) continue;
              const y = gy(e.z);
              if (Math.random() < 0.16) {
                const sc = rand(0.8, 2.4);
                rocks.add(sc, sc * 0.7, sc, greys[(i + t) % 3], e.x, y + sc * 0.2, e.z, rand(0, 3));
              } else {
                const h = rand(4.2, 8), r = rand(1.5, 2.6);
                solid.add(0.5, 1.6, 0.5, 0x5a3d2b, e.x, y + 0.8, e.z);
                crowns.add(r, h, r, greens[(i + t) % 4], e.x, y + 1.3 + h / 2, e.z, rand(0, 3));
              }
            }
          }
        }
      }
      solid.build(group, false); glow.build(group, false); crowns.build(group, false); rocks.build(group, false);
      scene.add(group);
      return { group, textures };
    }

    /* What stands around a venue's track: a crowd by the start line, then the venue's own clutter,
       all of it clear of the tarmac. Rows are kept neat (bays, stacks) so it reads as a place, not a scatter. */
    function venueScenery(solid, crowns, near, P, hw) {
      const S = D.TRACK_SCALE, px = PAD.hx * S, pz = PAD.hz * S;
      // a grandstand beside the start straight, on whichever side has room (outside the circuit if both do)
      const sp = P[T.START_OFFSET + T.LAP_LINE], fx = Math.sin(sp.h), fz = Math.cos(sp.h), nx = Math.cos(sp.h), nz = -Math.sin(sp.h);
      const at = (t, off, s) => ({ x: sp.x + fx * t + nx * s * off, z: sp.z + fz * t + nz * s * off });
      const far = (s) => { const q = at(0, hw + 8, s); return Math.hypot(q.x, q.z); };
      const side = [-1, 1].sort((u, v) => far(v) - far(u)).find((s) => [-17, 0, 17].every((t) => [hw + 3.4, hw + 13].every((o) => {
        const q = at(t, o, s);
        return !near(q.x, q.z, hw + 3) && Math.abs(q.x) < px && Math.abs(q.z) < pz;
      })));
      if (side) {
        for (let j = 0; j < 4; j++) {
          const q = at(0, hw + 4.6 + j * 2.2, side), top = 1.1 * (j + 1);
          solid.add(2.2, top, 33, j % 2 ? 0xcfd5de : 0xe6eaf0, q.x, top / 2, q.z, sp.h);
          for (let t = -15.5; t <= 15.5; t += 1.05) {
            if (Math.random() < 0.12) continue;
            const c = at(t + rand(-0.15, 0.15), hw + 4.6 + j * 2.2, side);
            solid.add(0.55, 0.85, 0.55, pick(CROWD), c.x, top + 0.42, c.z, sp.h);
          }
        }
        const back = at(0, hw + 13.6, side);
        solid.add(0.4, 7.5, 33, 0x2a3142, back.x, 3.75, back.z, sp.h);
      }
      const clearOfStand = (x, z) => { if (!side) return true; const dx = x - sp.x, dz = z - sp.z, t = dx * fx + dz * fz, o = (dx * nx + dz * nz) * side; return Math.abs(t) > 24 || o < hw || o > hw + 20; };
      const free = (x, z, r) => !near(x, z, hw + r) && clearOfStand(x, z);

      if (map.id === 'docks') {
        // container stacks in rows across the quay, a ship alongside
        const cols = [0xd9483b, 0x2f6fb5, 0x2f9e6b, 0xe0a030, 0x7a4fb5, 0xdfe4ec, 0xc9603a];
        for (let z = -pz + 10; z < pz - 10; z += 14.5) {
          for (let x = -px + 5; x < px - 5; x += 3.3) {
            if (Math.floor((x + px) / 3.3) % 9 > 5 || Math.random() < 0.3 || !free(x, z, 8)) continue;   // gaps make lanes between blocks
            const high = 1 + Math.floor(Math.random() * 3);
            for (let k = 0; k < high; k++) solid.add(2.6, 2.6, 12.2, pick(cols), x, 1.3 + k * 2.62, z);
          }
        }
        solid.add(34, 13, 250, 0x27344f, px + 34, 4, 20).add(34, 2.4, 250, 0xb63a35, px + 34, 0.2, 20);
        solid.add(24, 16, 30, 0xf1f3f6, px + 34, 18, 110).add(9, 9, 9, 0xe0a030, px + 34, 30, 112);
        for (let z = -70; z < 90; z += 14) solid.add(22, 6 + (z % 3) * 2.6, 12, pick(cols), px + 34, 13, z);
        for (const z of [-pz + 40, 0, pz - 60]) {   // quay cranes
          for (const dz of [-9, 9]) solid.add(2, 44, 2, 0xe0a030, px - 6, 22, z + dz).add(2, 44, 2, 0xe0a030, px - 22, 22, z + dz);
          solid.add(70, 3, 4, 0xe0a030, px + 6, 45, z).add(20, 2.4, 20, 0xe0a030, px - 14, 42.5, z);
        }
      } else if (map.id === 'airfield') {
        // parked planes on the apron, hangars down one side, the runway down the other
        for (let n = 0; n < 9; n++) {
          let q = null;
          for (let t = 0; t < 40 && !q; t++) { const x = rand(-px + 12, px - 12), z = rand(-pz + 12, pz - 12); if (free(x, z, 12)) q = { x, z }; }
          if (!q) continue;
          const ry = rand(0, Math.PI * 2), ax = Math.sin(ry), az = Math.cos(ry), col = pick([0xe5484d, 0x2f6fb5, 0xffc233, 0x22b573]);
          solid.add(2, 2, 15, 0xf4f6fa, q.x, 2, q.z, ry).add(17, 0.3, 2.8, 0xf4f6fa, q.x + ax * 1.2, 1.9, q.z + az * 1.2, ry);
          solid.add(0.3, 2.8, 2.2, col, q.x - ax * 6.3, 4, q.z - az * 6.3, ry).add(6, 0.25, 1.6, 0xf4f6fa, q.x - ax * 6.5, 2.9, q.z - az * 6.5, ry);
          solid.add(2.05, 0.5, 15.05, col, q.x, 1.6, q.z, ry);
        }
        for (let z = -pz + 30, n = 0; z < pz - 20; z += 62, n++) {
          solid.add(40, 13, 46, n % 2 ? 0xcfd4dc : 0xb9c0cc, -px - 34, 6.5, z).add(42, 2.2, 48, 0x7d8696, -px - 34, 14, z);
          solid.add(0.4, 10, 30, 0x4b5364, -px - 13.9, 5, z);
        }
        solid.add(46, 0.06, 2400, 0x33363f, px + 70, -0.06, 0);
        for (let z = -1100; z < 1100; z += 44) solid.add(1.2, 0.06, 22, 0xf4f6fa, px + 70, -0.03, z);
        solid.add(7, 26, 7, 0xdfe4ec, -px - 20, 13, pz + 26).add(12, 5, 12, 0x2a3142, -px - 20, 28, pz + 26);
        for (let n = 0; n < 60; n++) {
          const x = rand(-px - 240, px + 240), z = rand(-pz - 240, pz + 240);
          if ((Math.abs(x) < px + 110 && Math.abs(z) < pz + 40) || Math.abs(x - px - 70) < 40) continue;
          const h = rand(5, 9), r = rand(1.8, 3);
          solid.add(0.6, 1.8, 0.6, 0x6b4a33, x, 0.9, z);
          crowns.add(r, h, r, pick([0x8fae52, 0x7c9d47, 0xa3b85e]), x, 1.5 + h / 2, z, rand(0, 3));
        }
      } else {
        // a car park: parked cars in bays, tire stacks, trees round the edge
        const cols = [0xe5484d, 0x2f6fb5, 0xf1f3f6, 0x2a3142, 0xffc233, 0x22b573, 0x8f98ac, 0xff8a3d];
        for (let z = -pz + 9, row = 0; z < pz - 9; z += row % 2 ? 13 : 5.4, row++) {
          for (let x = -px + 5; x < px - 5; x += 3.1) {
            solid.add(0.14, 0.02, 4.9, 0xdfe4ec, x - 1.55, -0.03, z);   // bay line
            if (Math.random() < 0.72 || !free(x, z, 4.4)) continue;
            const col = pick(cols), nose = row % 2 ? 1 : -1;
            solid.add(1.9, 0.7, 4.3, col, x, 0.62, z).add(1.7, 0.6, 2.1, 0x1d2433, x, 1.25, z - nose * 0.3);
          }
        }
        for (let n = 0; n < 70; n++) {
          const x = rand(-px + 4, px - 4), z = rand(-pz + 4, pz - 4);
          if (!free(x, z, 1.6) || near(x, z, hw + 7)) continue;   // close to the track, like markers
          for (let k = 0, high = 1 + Math.floor(Math.random() * 3); k < high; k++) solid.add(1.3, 0.45, 1.3, 0x15181f, x, 0.23 + k * 0.47, z, rand(0, 3));
        }
        for (let n = 0; n < 260; n++) {
          const x = rand(-px - 150, px + 150), z = rand(-pz - 150, pz + 150);
          if (Math.abs(x) < px + 5 && Math.abs(z) < pz + 5) continue;
          const h = rand(5, 10), r = rand(2, 3.4);
          solid.add(0.7, 2, 0.7, 0x6b4a33, x, 1, z);
          crowns.add(r, h, r, pick([0x3f9b4f, 0x4fae5a, 0x2f8a4a, 0x5bb866]), x, 1.6 + h / 2, z, rand(0, 3));
        }
        solid.add(px * 2 + 2, 1.6, 0.5, 0xdfe4ec, 0, 0.8, -pz - 0.4).add(px * 2 + 2, 1.6, 0.5, 0xdfe4ec, 0, 0.8, pz + 0.4);
        solid.add(0.5, 1.6, pz * 2 + 2, 0xdfe4ec, -px - 0.4, 0.8, 0).add(0.5, 1.6, pz * 2 + 2, 0xdfe4ec, px + 0.4, 0.8, 0);
      }
    }
    const buildChunk = (ci) => buildSpan(ci * CHUNK, (ci + 1) * CHUNK);
    function dropChunk(ch) {
      scene.remove(ch.group);
      ch.group.traverse((o) => {
        if (o.geometry && o.geometry !== K.UNIT_BOX && !o.geometry.userData.shared) o.geometry.dispose();
        if (o.isInstancedMesh) o.dispose();
        if (o.material && o.material.map && o.material !== shared.road) o.material.dispose();
      });
      ch.textures.forEach((t) => t.dispose());
    }
    /* Keep the road built from one chunk behind the car to AHEAD chunks in front. */
    function streamRoad() {
      if (road.loop) { if (!chunks.has(0)) chunks.set(0, buildSpan(0, road.loop.lap)); return; }   // a track is built once
      const c = Math.floor(run.idx / CHUNK);
      for (let i = Math.max(0, c - 1); i <= c + AHEAD; i++) if (!chunks.has(i)) chunks.set(i, buildChunk(i));
      chunks.forEach((ch, i) => { if (i < c - 1) { dropChunk(ch); chunks.delete(i); } });
    }

    function mark(x, z, va) {
      tmp.position.set(x, gy(z) + 0.07, z);
      tmp.rotation.set(0, va - Math.PI / 2, 0);
      tmp.scale.set(1.1, 1, 0.3);
      tmp.updateMatrix();
      marks.setMatrixAt(markNext, tmp.matrix);
      markNext = (markNext + 1) % markN;
      marks.instanceMatrix.needsUpdate = true;
    }

    /* ---------- Input: one finger, slid left and right ---------- */
    function steerRange() { return Math.min(el.app.clientWidth, 520) * 0.2; }
    el.root.addEventListener('pointerdown', (e) => {
      if (e.target.closest('button') || !active) return;
      drag = { id: e.pointerId, x0: e.clientX, v: 0 };
      try { el.root.setPointerCapture(e.pointerId); } catch (err) { /* older browsers */ }
    });
    el.root.addEventListener('pointermove', (e) => {
      if (!drag || e.pointerId !== drag.id) return;
      const r = steerRange();
      let dx = e.clientX - drag.x0;
      if (Math.abs(dx) > r) { drag.x0 = e.clientX - Math.sign(dx) * r; dx = Math.sign(dx) * r; }   // the centre follows the finger, so reversing bites at once
      drag.v = dx / r;
    });
    const release = (e) => { if (drag && e.pointerId === drag.id) drag = null; };
    el.root.addEventListener('pointerup', release);
    el.root.addEventListener('pointercancel', release);
    const key = (down) => (e) => {
      if (!active) return;
      if (e.key === 'ArrowLeft' || e.key === 'a') keyL = down;
      if (e.key === 'ArrowRight' || e.key === 'd') keyR = down;
    };
    root.addEventListener('keydown', key(true));
    root.addEventListener('keyup', key(false));
    const steering = () => (drag ? drag.v : (keyR ? 1 : 0) - (keyL ? 1 : 0));

    /* ---------- HUD ---------- */
    function say(text, cls, time, sub) {
      el.msg.className = cls || '';
      el.msg.innerHTML = text + (sub ? '<small>' + sub + '</small>' : '');
      msgT = time;
    }
    function set(node, key, value) { if (shown[key] !== value) { shown[key] = value; node.textContent = value; } }
    function hud(dt) {
      set(el.score, 'score', Math.floor(run.score).toLocaleString('en-US'));
      set(el.mult, 'mult', 'x' + run.mult.toFixed(1));
      const frac = run.mult >= C.multCap(run) ? 1 : run.mult - Math.floor(run.mult);
      el.bar.style.width = (run.mult > 1 || run.drifting ? frac * 100 : 0) + '%';
      el.root.classList.toggle('hot', run.drifting);
      set(el.dist, 'dist', (road.loop ? 'Lap ' + (C.lapsDone(run) + 1) + ' · ' : '') + (run.dist / 1000).toFixed(2) + ' km');
      set(el.speed, 'speed', Math.round(run.speed * 3.6) + ' km/h');
      set(el.angle, 'angle', Math.round(Math.abs(run.slip) * 180 / Math.PI) + '° angle');
      if (shown.hits !== run.hits) {
        shown.hits = run.hits;
        let h = '';
        for (let i = 0; i < T.MAX_HITS; i++) h += '<i' + (i < C.hitsLeft(run) ? ' class="on"' : '') + '></i>';
        el.lives.innerHTML = T.MAX_HITS > 1 ? h : '';   // one touch ends the run, so there is nothing to count
      }
      if (run.hold > 0) {
        const n = Math.max(1, Math.ceil(run.hold / (T.COUNTDOWN / 3)));
        if (shown.count !== n) { shown.count = n; say(String(n), '', 9, 'Slide a finger left and right to steer'); }
      } else if (!went) { went = true; say('GO!', 'good', 0.8); }
      if (Math.floor(run.mult) > Math.floor(lastMult) && run.mult >= 2) say('x' + Math.floor(run.mult) + '!', 'good', 0.7);
      lastMult = run.mult;
      msgT -= dt;
      if (msgT <= 0 && el.msg.innerHTML) el.msg.innerHTML = '';
    }

    /* ---------- Frame ---------- */
    function pose(dt) {
      const k = slope, fx = Math.sin(run.h), fz = Math.cos(run.h), y = gy(run.z);
      f.set(fx, -k * fz, fz).normalize();
      up.set(0, 1, k).normalize();
      side.crossVectors(f, up).normalize();
      up.crossVectors(side, f);
      basis.makeBasis(f, up, side);
      carM.g.quaternion.setFromRotationMatrix(basis);
      carM.g.position.set(run.x, y, run.z);
      const parts = car.parts;
      carM.body.position.y = -Math.min(0.17, parts.stance * 0.034);
      carM.body.rotation.x = clamp(run.slip * 0.14, -0.1, 0.1);
      const camber = Math.min(0.22, parts.stance * 0.044);
      carM.wheels.forEach((w) => { w.pivot.rotation.y = w.front ? clamp(-run.slip * 0.9, -0.7, 0.7) : 0; w.camber.rotation.x = -w.side * camber; });

      // smoke and marks from the rear tires
      const amt = run.hold > 0 || run.done ? 0 : clamp((Math.abs(run.slip) - 0.12) / 0.4, 0, 1) * Math.min(1, run.speed / 12);
      if (amt > 0) {
        const col = parts.tires >= 4 ? new THREE.Color(0xffffff).lerp(new THREE.Color(car.color), 0.4).getHex() : 0xffffff;
        markD += run.speed * dt;
        const doMark = amt > 0.2 && markD > 0.8;
        if (doMark) markD = 0;
        for (const s of [-1, 1]) {
          const wx = run.x - f.x * carM.len * 0.33 + side.x * s * carM.wid * 0.46, wz = run.z - f.z * carM.len * 0.33 + side.z * s * carM.wid * 0.46;
          if (Math.random() < amt * (20 + parts.tires * 3) * dt) {
            puffs.add(wx, gy(wz) + 0.4, wz, -fx * 1.5 + rand(-0.8, 0.8), rand(0.8, 1.8), -fz * 1.5 + rand(-0.8, 0.8), rand(0.45, 0.8) * (1 + parts.tires * 0.06), rand(0.7, 1.2), col);
          }
          if (doMark) mark(wx, wz, run.va);
        }
      }
      puffs.update(dt);

      // chase camera: behind the direction of travel, so the slide shows as the car turning under it
      camAng += C.wrapPi(run.va - camAng) * (1 - Math.exp(-3.2 * dt));
      const bx = Math.sin(camAng), bz = Math.cos(camAng);
      shake = Math.max(0, shake - dt * 2.4);
      const cz = run.z - bz * 12.5;
      camera.position.set(run.x - bx * 12.5 + rand(-1, 1) * shake, Math.max(y + 10, gy(cz) + 4) + rand(-1, 1) * shake * 0.6, cz);
      look.set(run.x + bx * 11, y + 0.6, run.z + bz * 11);
      camera.lookAt(look);
      if (ground) ground.position.set(run.x, y, run.z);
      if (peaks) peaks.position.set(run.x, y, run.z);
    }

    function end() {
      if (result) return;
      run.done = true;
      result = C.collect(state, car, run);
      drag = null; keyL = keyR = false;
      el.root.classList.remove('hot');
      el.msg.innerHTML = '';
      if (hooks.done) hooks.done(result);
    }

    function frame(dt) {
      if (!active) return;
      world.resize();
      const sz = world.size(), aspect = sz.w / sz.h;
      if (camera.aspect !== aspect) { camera.aspect = aspect; camera.updateProjectionMatrix(); }
      const steps = Math.max(1, Math.ceil(dt * 120)), input = steering();
      for (let i = 0; i < steps && !run.done; i++) {
        const ev = C.step(run, input, dt / steps);
        if (ev.includes('km')) {
          say(run.km + ' KM', 'good', 1.3, (run.lastKmBonus ? 'Clean so far: +' + Math.floor(run.lastKmBonus).toLocaleString('en-US') + ' points. ' : '') +
            'Multiplier can now reach x' + C.multCap(run));
        }
        if (ev.includes('hit')) {
          shake = 0.5;
          const left = C.hitsLeft(run);
          say(left ? 'Barrier!' : 'Barrier! Run over', 'bad', left ? 0.8 : 3, left ? left + (left === 1 ? ' touch left' : ' touches left') : '');
          for (let j = 0; j < 8; j++) puffs.add(run.x, gy(run.z) + 0.6, run.z, rand(-4, 4), rand(1, 4), rand(-4, 4), rand(0.25, 0.5), rand(0.3, 0.6), 0xffb84d);
        }
        if (ev.includes('out')) endT = 0.9;   // a moment to see it before the results
      }
      if (road.loop && C.lapsDone(run) > lapShown) {
        lapShown = C.lapsDone(run);
        say('LAP ' + (lapShown + 1), 'good', 1.2, lapShown + (lapShown === 1 ? ' lap' : ' laps') + ' done');
      }
      if (endT > 0 && (endT -= dt) <= 0) end();
      streamRoad();
      pose(dt);
      hud(dt);
      world.renderer.render(scene, camera);
    }

    function clear() {
      if (!scene) return;
      chunks.forEach(dropChunk);
      chunks.clear();
      K.disposeGroup(scene);
      shared.road.map.dispose(); shared.road.dispose(); shared.shoulder.dispose(); shared.glow.dispose(); shared.cone.dispose(); shared.rock.dispose();
      scene = null; carM = null; puffs = null; marks = null; ground = null; peaks = null; shared = null;
    }
    /* Start a run in one of the team's cars. `where` is { kind: 'street', id } (a street map)
       or { kind: 'track', id } (the track of a venue the team owns); the mountain pass if left out. */
    function start(carId, where) {
      const c = G.getCar(state, carId);
      if (!c || !G.driverOf(state, c)) return false;
      const track = !!where && where.kind === 'track';
      if (track && !(DT.tracks.LAYOUTS[where.id] && G.ownsPlace(state, D.VENUES.findIndex((v) => v.id === where.id)))) return false;
      clear();
      car = c;
      map = track ? { kind: 'track', id: where.id } : { kind: 'street', id: (where && where.id) || 'pass' };
      slope = track ? 0 : T.SLOPE;
      road = track ? C.trackRoad(map.id) : C.newRoad(G.seeded(Math.floor(Math.random() * 4294967296)));
      run = C.newRun(road, C.carSetup(state, car));
      run.map = track ? 'track:' + map.id : map.id;   // the key its records are kept under
      result = null; drag = null; keyL = keyR = false;
      camAng = run.va; shake = 0; lastMult = 1; markD = 0; went = false; shown = {}; endT = 0; lapShown = 0;
      build();
      streamRoad();
      active = true;
      el.root.hidden = false;
      el.app.classList.add('in-touge');
      frame(0);
      return true;
    }
    function stop() {
      active = false;
      clear();
      el.root.hidden = true;
      el.app.classList.remove('in-touge');
    }

    return { start, stop, frame, quit: () => { if (active && !result) end(); }, again: () => (car ? start(car.id, map) : false), map: () => map,
      active: () => active, _run: () => run, _result: () => result, _chunks: () => Array.from(chunks.keys()) };
  }

  DT.Touge = Touge;
})(window);
