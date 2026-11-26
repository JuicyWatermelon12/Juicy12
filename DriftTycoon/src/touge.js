/* Drift Tycoon — Touge practice, the 3D side: the mountain road at dusk, chase camera, smoke, steering and HUD.
   The rules are in tougecore.js. It draws with the venue's renderer while the venue is paused. */
(function (root) {
  'use strict';
  const DT = root.DT, THREE = root.THREE, D = DT.data, G = DT.game, C = DT.touge, K = DT.kit, T = D.TOUGE;
  const V3 = THREE.Vector3;
  const SKY = 0x262c5c, FOG = 0x3a4180, GROUND = 0x2e5a4d;
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const rand = (a, b) => a + Math.random() * (b - a);
  const $ = (id) => document.getElementById(id);
  const gy = (z) => -T.SLOPE * z;   // the whole mountainside is one slope; height depends only on z

  function roadTexture() {
    const c = document.createElement('canvas');
    c.width = 256; c.height = 256;
    const g = c.getContext('2d');
    g.fillStyle = '#353b4f'; g.fillRect(0, 0, 256, 256);
    for (let i = 0; i < 900; i++) {
      g.fillStyle = Math.random() < 0.5 ? 'rgba(255,255,255,0.05)' : 'rgba(0,0,0,0.1)';
      g.fillRect(Math.random() * 256, Math.random() * 256, 2, 2);
    }
    g.fillStyle = '#e9edf5'; g.fillRect(9, 0, 6, 256); g.fillRect(241, 0, 6, 256);   // edge lines
    g.fillStyle = '#ffcf33'; g.fillRect(125, 0, 6, 128);                              // centre dashes
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
    const el = { root: $('touge'), app: $('app'), score: $('tg-score'), mult: $('tg-mult'), bar: $('tg-combo-bar'), progress: $('tg-progress'),
      msg: $('tg-msg'), speed: $('tg-speed'), angle: $('tg-angle'), hits: $('tg-hits') };
    const camera = new THREE.PerspectiveCamera(58, 1, 0.5, 900);
    const tmp = new THREE.Object3D(), basis = new THREE.Matrix4(), f = new V3(), up = new V3(), side = new V3(), look = new V3();
    let active = false, scene = null, run = null, car = null, carM = null, road = null, result = null;
    let drag = null, keyL = false, keyR = false;
    let camAng = 0, shake = 0, msgT = 0, lastMult = 1, markD = 0, went = false, shown = {};
    let puffs = null, marks = null, markNext = 0;
    const MARK_N = 700;

    /* ---------- The mountain ---------- */
    function build() {
      scene = new THREE.Scene();
      scene.background = new THREE.Color(SKY);
      scene.fog = new THREE.Fog(FOG, 60, 290);
      scene.add(new THREE.HemisphereLight(0xc3caff, 0x25423b, 0.9));
      const sun = new THREE.DirectionalLight(0xffb37a, 0.55);
      sun.position.set(-70, 45, -40);
      scene.add(sun);

      const P = road.pts, n = road.n, hw = road.halfW;
      let x0 = Infinity, x1 = -Infinity;
      P.forEach((p) => { x0 = Math.min(x0, p.x); x1 = Math.max(x1, p.x); });
      const z1 = P[n - 1].z, cx = (x0 + x1) / 2, cz = z1 / 2;

      const ground = new THREE.PlaneGeometry(x1 - x0 + 700, z1 + 700, 1, 1);
      ground.rotateX(-Math.PI / 2);
      const gp = ground.attributes.position;
      for (let i = 0; i < gp.count; i++) gp.setY(i, gy(gp.getZ(i) + cz) - 0.08);
      ground.computeVertexNormals();
      K.mesh(ground, new THREE.MeshLambertMaterial({ color: GROUND }), cx, 0, cz, scene);

      const ribbon = (half, lift, material, vScale) => {
        const pos = new Float32Array(n * 6), uv = new Float32Array(n * 4), idx = [];
        for (let i = 0; i < n; i++) {
          const p = P[i], nx = Math.cos(p.h), nz = -Math.sin(p.h);
          const lx = p.x - nx * half, lz = p.z - nz * half, rx = p.x + nx * half, rz = p.z + nz * half;
          pos.set([lx, gy(lz) + lift, lz, rx, gy(rz) + lift, rz], i * 6);
          uv.set([0, i / vScale, 1, i / vScale], i * 4);
          if (i < n - 1) idx.push(i * 2, i * 2 + 2, i * 2 + 1, i * 2 + 1, i * 2 + 2, i * 2 + 3);
        }
        const geo = new THREE.BufferGeometry();
        geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
        geo.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
        geo.setIndex(idx);
        geo.computeVertexNormals();
        const m = new THREE.Mesh(geo, material);
        m.frustumCulled = false;
        scene.add(m);
      };
      ribbon(hw + 1.6, 0, new THREE.MeshLambertMaterial({ color: 0x4d5263, side: THREE.DoubleSide }), 10);
      ribbon(hw, 0.03, new THREE.MeshLambertMaterial({ map: roadTexture(), side: THREE.DoubleSide }), 10);

      // barriers, reflectors, corner boards, lamps
      const solid = new K.Batch(), glow = new K.Batch(null, new THREE.MeshBasicMaterial({ color: 0xffffff }));
      const edge = (i, s, off) => { const p = P[i]; return { x: p.x + Math.cos(p.h) * s * off, z: p.z - Math.sin(p.h) * s * off }; };
      for (const s of [-1, 1]) {
        for (let i = 0; i + 3 < n; i += 3) {
          const a = edge(i, s, hw + 0.4), b = edge(i + 3, s, hw + 0.4), mx = (a.x + b.x) / 2, mz = (a.z + b.z) / 2;
          solid.add(Math.hypot(b.x - a.x, b.z - a.z) + 0.12, 0.3, 0.12, 0xd9dfe9, mx, gy(mz) + 0.72, mz, Math.atan2(-(b.z - a.z), b.x - a.x));
          solid.add(0.16, 0.85, 0.16, 0x8f98ac, a.x, gy(a.z) + 0.42, a.z);
          if (i % 9 === 0) glow.add(0.2, 0.2, 0.2, s < 0 ? 0xff6a3d : 0xffe08a, a.x, gy(a.z) + 0.98, a.z);
        }
      }
      for (let i = 20; i < n - 20; i += 7) {
        const p = P[i];
        if (Math.abs(p.k) < 0.03) continue;
        const e = edge(i, p.k > 0 ? -1 : 1, hw + 1.5);   // outside of the bend
        glow.add(1.5, 0.9, 0.12, (i / 7) % 2 ? 0xffc233 : 0xff5a36, e.x, gy(e.z) + 1.7, e.z, p.h);
        solid.add(0.14, 1.3, 0.14, 0x8f98ac, e.x, gy(e.z) + 0.65, e.z);
      }
      for (let i = 30, s = 1; i < n - 10; i += 70, s = -s) {
        const e = edge(i, s, hw + 1.3), a = edge(i, s, hw - 0.4), p = P[i];
        solid.add(0.2, 6.4, 0.2, 0x8f98ac, e.x, gy(e.z) + 3.2, e.z);
        solid.add(2, 0.14, 0.14, 0x8f98ac, (e.x + a.x) / 2, gy(e.z) + 6.3, (e.z + a.z) / 2, p.h);
        glow.add(0.8, 0.2, 0.45, 0xffe6a8, a.x, gy(a.z) + 6.2, a.z, p.h);
      }
      // start and finish gates
      [[T.START_OFFSET + 34, 'TOUGE PRACTICE', 0x2f8fff], [n - T.FINISH_MARGIN, 'FINISH', 0xff6b2c]].forEach((gate) => {
        const i = gate[0], p = P[i];
        for (const s of [-1, 1]) { const e = edge(i, s, hw + 0.9); solid.add(0.5, 7, 0.5, gate[2], e.x, gy(e.z) + 3.5, e.z, p.h); }
        solid.add(hw * 2 + 2.3, 1.5, 0.4, gate[2], p.x, gy(p.z) + 6.6, p.z, p.h);
        const sign = K.mesh(new THREE.PlaneGeometry(9.6, 1.2), new THREE.MeshBasicMaterial({ map: K.textTexture(gate[1], '#ffffff', '#1d2433', 512, 64, 46) }),
          p.x - Math.sin(p.h) * 0.24, gy(p.z) + 6.6, p.z - Math.cos(p.h) * 0.24, scene);
        sign.rotation.y = p.h + Math.PI;
      });

      // trees and rocks, kept clear of every part of the road
      const near = (x, z, r) => { for (let i = 0; i < n; i += 3) { const dx = P[i].x - x, dz = P[i].z - z; if (dx * dx + dz * dz < r * r) return true; } return false; };
      const crowns = new K.Batch(new THREE.ConeGeometry(1, 1, 7), K.WHITE), rocks = new K.Batch(new THREE.IcosahedronGeometry(1, 0), K.WHITE);
      const greens = [0x1f5a48, 0x26694f, 0x1a4d44, 0x2f7a56], greys = [0x626a82, 0x4f5670, 0x767e96];
      for (let i = 0; i < n; i += 4) {
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
      solid.build(scene, false); glow.build(scene, false); crowns.build(scene, false); rocks.build(scene, false);
      // far peaks
      const peak = new THREE.ConeGeometry(150, 210, 6), peakMat = new THREE.MeshLambertMaterial({ color: 0x2c3870 });
      for (let i = 0; i < 7; i++) {
        const z = -150 + i * (z1 + 300) / 6, s = i % 2 ? 1 : -1;
        K.mesh(peak, peakMat, (s < 0 ? x0 : x1) + s * rand(300, 420), gy(z) + rand(20, 60), z, scene).rotation.y = rand(0, 3);
      }

      // the car, with a shadow under it and a pool of headlight on the road ahead
      carM = K.makeCarMesh(car);
      const shadow = new THREE.PlaneGeometry(carM.len + 0.6, carM.wid + 0.5);
      shadow.rotateX(-Math.PI / 2);
      K.mesh(shadow, new THREE.MeshBasicMaterial({ color: 0x05070c, transparent: true, opacity: 0.35, depthWrite: false }), 0, 0.05, 0, carM.g);
      const beam = new THREE.PlaneGeometry(13, 8);
      beam.rotateX(-Math.PI / 2);
      K.mesh(beam, new THREE.MeshBasicMaterial({ map: glowTexture(), color: 0xfff0b8, transparent: true, opacity: 0.4, depthWrite: false,
        blending: THREE.AdditiveBlending }), carM.len / 2 + 6, 0.09, 0, carM.g).renderOrder = 2;
      scene.add(carM.g);

      puffs = Puffs(scene, 240);
      const markGeo = new THREE.PlaneGeometry(1, 1);
      markGeo.rotateX(-Math.PI / 2);
      marks = new THREE.InstancedMesh(markGeo, new THREE.MeshBasicMaterial({ color: 0x0b0d14, transparent: true, opacity: 0.45, depthWrite: false }), MARK_N);
      marks.frustumCulled = false;
      marks.renderOrder = 1;
      marks.instanceMatrix.array.fill(0);
      markNext = 0;
      scene.add(marks);
    }

    function mark(x, z, va) {
      tmp.position.set(x, gy(z) + 0.07, z);
      tmp.rotation.set(0, va - Math.PI / 2, 0);
      tmp.scale.set(1.1, 1, 0.3);
      tmp.updateMatrix();
      marks.setMatrixAt(markNext, tmp.matrix);
      markNext = (markNext + 1) % MARK_N;
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
      const frac = run.mult >= T.COMBO_MAX ? 1 : (run.comboT % T.COMBO_TIME) / T.COMBO_TIME;
      el.bar.style.width = (run.mult > 1 || run.drifting ? frac * 100 : 0) + '%';
      el.root.classList.toggle('hot', run.drifting);
      el.progress.style.width = Math.min(100, run.idx / (road.n - T.FINISH_MARGIN) * 100) + '%';
      set(el.speed, 'speed', Math.round(run.speed * 3.6) + ' km/h');
      set(el.angle, 'angle', Math.round(Math.abs(run.slip) * 180 / Math.PI) + '° angle');
      set(el.hits, 'hits', run.hits ? run.hits + (run.hits === 1 ? ' barrier touch' : ' barrier touches') : 'Clean');
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
      const k = T.SLOPE, fx = Math.sin(run.h), fz = Math.cos(run.h), y = gy(run.z);
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
        if (ev.includes('hit')) {
          shake = 0.5;
          say('Barrier!', 'bad', 0.7);
          for (let j = 0; j < 8; j++) puffs.add(run.x, gy(run.z) + 0.6, run.z, rand(-4, 4), rand(1, 4), rand(-4, 4), rand(0.25, 0.5), rand(0.3, 0.6), 0xffb84d);
        }
        if (ev.includes('finish')) end();
      }
      pose(dt);
      hud(dt);
      world.renderer.render(scene, camera);
    }

    function clear() {
      if (scene) { K.disposeGroup(scene); scene = null; }
      carM = null; puffs = null; marks = null;
    }
    function start(carId) {
      const c = G.getCar(state, carId);
      if (!c || !G.driverOf(state, c)) return false;
      clear();
      car = c;
      road = C.buildRoad(G.seeded(Math.floor(Math.random() * 4294967296)));
      run = C.newRun(road, C.carSetup(state, car));
      result = null; drag = null; keyL = keyR = false;
      camAng = 0; shake = 0; lastMult = 1; markD = 0; went = false; shown = {};
      build();
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

    return { start, stop, frame, quit: () => { if (active && !result) end(); }, again: () => (car ? start(car.id) : false),
      active: () => active, _run: () => run, _result: () => result };
  }

  DT.Touge = Touge;
})(window);
