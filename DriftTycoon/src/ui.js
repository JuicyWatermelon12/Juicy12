/* Drift Tycoon — the HTML interface over the 3D view: money bar, tabs, upgrade sheets, pop-ups. */
(function (root) {
  'use strict';
  const DT = root.DT, D = DT.data, G = DT.game;
  const $ = (id) => document.getElementById(id);
  const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const pct = (x) => '+' + Math.round(x * 100) + '%';

  const STATUS = {
    run: ['Drifting', 'ok'], merge: ['Heading out', 'ok'], queue: ['Waiting for a gap', 'idle'], parked: ['No driver', 'idle'],
    spin: ['Crashed', 'bad'], wait: ['Waiting for a mechanic', 'bad'], repair: ['Being repaired', 'warn'],
  };
  const TITLES = { garage: 'Garage', team: 'Team', venue: 'Venue' };

  function UI(state, world, actions) {
    let tab = null, openCar = state.cars[0] ? state.cars[0].id : null, armed = '', tryKey = '';
    const el = {
      cash: $('cash'), rate: $('rate'), meterFill: $('meter-fill'), meterText: $('meter-text'), meter: $('meter'),
      sheet: $('sheet'), body: $('sheet-body'), title: $('sheet-title'), tabs: $('tabs'), modal: $('modal'),
      popups: $('popups'), toast: $('toast'), hud: $('hud'),
    };

    /* ---------- Small pieces ---------- */
    function buy(label, cost, act, attrs, cls) {
      const data = Object.keys(attrs || {}).map((k) => ' data-' + k + '="' + esc(attrs[k]) + '"').join('');
      if (!isFinite(cost)) return '<button class="buy maxed" disabled>Max</button>';
      return '<button class="buy ' + (cls || '') + '" data-act="' + act + '" data-cost="' + cost + '"' + data + '>' +
        (label ? '<span class="buy-label">' + label + '</span>' : '') + '<span class="buy-cost">' + G.money(cost) + '</span></button>';
    }
    const bar = (label, v) => '<div class="stat"><span>' + label + '</span><i><b style="width:' + Math.min(100, Math.round(v / 1.6)) +
      '%"></b></i><em>' + Math.round(v) + '</em></div>';
    const stats = (d) => { const s = G.driverStats(d); return bar('Skill', s.skill) + bar('Control', s.control) + bar('Flair', s.flair); };
    const rarityTag = (d) => { const r = G.rarity(d.rarity); return '<span class="rarity" style="--r:' + r.color + '">' + r.name + '</span>'; };
    const traitLine = (d) => { const t = G.trait(d); return t.name ? '<div class="trait"><b>' + t.name + '</b> ' + t.blurb + '</div>' : ''; };
    const pips = (n) => { let h = '<span class="pips">'; for (let i = 0; i < D.PART_MAX; i++) h += '<i' + (i < n ? ' class="on"' : '') + '></i>'; return h + '</span>'; };
    const xpPct = (d) => (G.xpToNext(d) ? Math.min(100, (d.xp / G.xpToNext(d)) * 100) : 100);
    function levelRow(d) {
      const cap = G.levelCap(d), top = d.level >= cap;
      return '<div class="level"><span>Lv ' + d.level + '<small> / ' + cap + '</small></span><i><b data-xp="' + d.id + '" style="width:' + xpPct(d) +
        '%"></b></i><em>' + (top ? 'Top level' : 'Levels up with seat time') + '</em></div>';
    }
    const statLine = (d) => { const s = G.driverStats(d); return 'Lv ' + d.level + ' · Skill ' + s.skill + ' · Control ' + s.control + ' · Flair ' + s.flair; };

    /* ---------- Garage ---------- */
    function carCard(car, i) {
      const m = G.model(car.model), d = G.driverOf(state, car), open = openCar === car.id;
      const st = STATUS[world.status(car.id)] || STATUS.parked;
      let h = '<article class="card car' + (open ? ' open' : '') + '">' +
        '<button class="car-head" data-act="toggle-car" data-car="' + car.id + '" aria-expanded="' + open + '">' +
        '<span class="swatch" style="background:' + car.color + '"></span>' +
        '<span class="car-name"><strong>' + esc(m.name) + '</strong><small>Pit ' + (i + 1) + ' · ' +
        (d ? esc(d.name) : 'no driver') + '</small></span>' +
        '<span class="car-num"><strong>' + G.fmt(G.carStyle(state, car), 1) + '</strong><small>style/s</small></span></button>' +
        '<div class="chip ' + st[1] + '" data-status="' + car.id + '">' + st[0] + '</div>';
      if (open) {
        h += '<div class="row driver-row"><div><small>Driver</small><div>' + (d ? esc(d.name) + ' ' + rarityTag(d) : 'Empty seat — this car is parked') +
          '</div></div><button class="ghost" data-act="pick-driver" data-car="' + car.id + '">' + (d ? 'Change' : 'Choose') + '</button></div>';
        h += '<div class="parts">';
        D.PARTS.forEach((p) => {
          const lv = car.parts[p.id], maxed = lv >= D.PART_MAX;
          h += '<div class="row part"><div><strong>' + p.name + '</strong> <span class="lv">' + D.PART_STAGES[lv] + '</span>' + pips(lv) + '<small>' + p.blurb +
            '</small><small class="delta">' + (maxed ? 'Style ' + pct(lv * p.per) : D.PART_STAGES[lv + 1] + ': style ' + pct(lv * p.per) + ' → ' + pct((lv + 1) * p.per)) +
            '</small></div>' + buy('', maxed ? Infinity : G.partCost(car, p.id), 'part', { car: car.id, part: p.id }) + '</div>';
        });
        h += '</div>';
        const next = D.MODELS[G.modelIndex(car.model) + 1];
        if (next) {
          h += '<div class="row trade"><div><strong>Trade up: ' + esc(next.name) + '</strong><small>' + esc(next.blurb) + '</small><small class="delta">Base style ' +
            G.fmt(m.base) + ' → ' + G.fmt(next.base) + '. Parts and driver move across.</small></div>' +
            buy('', next.cost, 'trade', { car: car.id, model: next.id }, 'alt') + '</div>';
        }
        const rate = G.carCrashRate(state, car);
        if (rate > 0) h += '<p class="note">Crashes about once every ' + G.duration(1 / rate) + ' of drifting.</p>';
      }
      return h + '</article>';
    }

    function renderGarage() {
      let h = '';
      state.cars.forEach((c, i) => (h += carCard(c, i)));
      if (state.cars.length < state.slots) {
        h += '<article class="card"><h3>Pit ' + (state.cars.length + 1) + ' is empty</h3><p class="note">Pick a car for it.</p>';
        D.MODELS.forEach((m) => {
          h += '<div class="row"><div><strong>' + esc(m.name) + '</strong><small>' + esc(m.blurb) + ' Base style ' + G.fmt(m.base) + '/s.</small></div>' +
            buy('', m.cost, 'buy-car', { model: m.id }) + '</div>';
        });
        h += '</article>';
      } else if (state.slots < G.MAX_SLOTS) {
        h += '<article class="card"><div class="row"><div><strong>Open pit ' + (state.slots + 1) + '</strong><small>Room for one more car on track, and one more driver on the team.</small></div>' +
          buy('', G.slotCost(state), 'slot') + '</div></article>';
      }
      return h;
    }

    /* ---------- Team ---------- */
    function tryoutState() {
      if (state.prospects) return 'p';
      return G.tryoutWait(state, Date.now()) > 0 ? 'w' : 'r';
    }
    function driverCard(d) {
      const car = G.carOfDriver(state, d.id);
      return '<article class="card driver" style="--r:' + G.rarity(d.rarity).color + '"><div class="row"><div><strong>' + esc(d.name) + '</strong> ' + rarityTag(d) +
        '<small>' + (car ? 'Drives the ' + esc(G.model(car.model).name) + ' (pit ' + (state.cars.indexOf(car) + 1) + ')' : 'Reserve driver. Only seat time earns experience') + '</small></div>' +
        '<button class="ghost" data-act="pick-car" data-driver="' + d.id + '">' + (car ? 'Move' : 'Assign') + '</button></div>' +
        levelRow(d) + traitLine(d) + '<div class="stats">' + stats(d) + '</div></article>';
    }

    function renderTeam() {
      tryKey = tryoutState();
      const cap = G.rosterCap(state);
      let action;
      if (tryKey === 'p') action = '<button class="buy alt" data-act="view-tryout"><span class="buy-label">' + state.prospects.length + ' waiting</span><span class="buy-cost">Meet them</span></button>';
      else if (tryKey === 'w') action = '<div class="wait"><small>Next tryout</small><b data-tryout-wait>' + G.clock(G.tryoutWait(state, Date.now())) + '</b></div>';
      else action = buy('Hold one', G.tryoutCost(state), 'tryout');
      let h = '<article class="card"><div class="row"><div><strong>Tryout day</strong><small>' + D.TRYOUT_SIZE +
        ' prospects show up. Sign one, or send them all home.</small></div>' + action + '</div><p class="note">Drivers: ' + state.drivers.length + ' of ' + cap +
        ' (one per pit, plus a reserve). With a full team, a signing replaces someone: they retire and pass half their experience to the newcomer.</p></article>';
      const order = D.RARITIES.map((r) => r.id);
      state.drivers.slice().sort((a, b) => order.indexOf(b.rarity) - order.indexOf(a.rarity) || G.driverMult(b) - G.driverMult(a))
        .forEach((d) => (h += driverCard(d)));
      const t = G.repairTime(state), t2 = G.repairTime(state, state.mech.level + 1);
      h += '<h3 class="section">Mechanics</h3><article class="card">' +
        '<div class="row"><div><strong>Crew: ' + state.mech.count + ' of ' + D.MECH_MAX + '</strong><small>Each mechanic fixes one crashed car at a time.</small></div>' +
        buy('Hire', G.mechHireCost(state), 'hire') + '</div>' +
        '<div class="row"><div><strong>Training</strong> <span class="lv">Lv ' + state.mech.level + '</span><small>Repairs take ' + t.toFixed(1) + ' s' +
        (G.mechTrainMaxed(state) ? '' : ' → ' + t2.toFixed(1) + ' s') + '</small></div>' +
        buy('', G.mechTrainMaxed(state) ? Infinity : G.mechTrainCost(state), 'train') + '</div></article>';
      return h;
    }

    /* ---------- Venue ---------- */
    function renderVenue() {
      const inc = G.incomeRate(state);
      let h = '<article class="card"><h3>Parking Lot</h3><p class="note">Your first venue. Style from the cars turns into cash only as fast as the crowd can cheer: right now ' +
        G.fmt(inc.cap, 1) + ' style/s.</p></article>';
      const now = { stands: G.fmt(G.crowdCap(state), 1) + ' style/s cheered', sponsors: 'Cash x' + G.sponsorMult(state).toFixed(2), prep: 'Style x' + G.prepMult(state).toFixed(2) };
      const then = {
        stands: G.fmt(G.crowdCap(state) * D.CROWD_GROWTH, 1) + ' style/s', sponsors: 'x' + (G.sponsorMult(state) * D.SPONSOR_MULT).toFixed(2),
        prep: 'x' + (G.prepMult(state) * D.PREP_MULT).toFixed(2),
      };
      D.VENUE.forEach((v) => {
        h += '<article class="card"><div class="row"><div><strong>' + v.name + '</strong> <span class="lv">Lv ' + state.venue[v.id] + '</span><small>' + v.blurb +
          '</small><small class="delta">' + now[v.id] + ' → ' + then[v.id] + '</small></div>' + buy('', G.venueCost(state, v.id), 'venue', { id: v.id }) + '</div></article>';
      });
      h += '<article class="card"><div class="row"><div><strong>Pits: ' + state.slots + ' of ' + G.MAX_SLOTS + '</strong><small>Each pit is one more car on track.</small></div>' +
        buy('', G.slotCost(state), 'slot') + '</div></article>';
      h += '<div class="reset"><button class="ghost danger" data-act="reset">' + (armed === 'reset' ? 'Tap again to wipe this team' : 'Start over') + '</button></div>';
      return h;
    }

    function render() {
      if (!tab) return;
      const keep = el.body.scrollTop;
      el.title.textContent = TITLES[tab];
      el.body.innerHTML = tab === 'garage' ? renderGarage() : tab === 'team' ? renderTeam() : renderVenue();
      el.body.scrollTop = keep;
      refresh();
    }

    /* Cheap updates between renders: what can be afforded, car status chips, experience bars, the tryout clock. */
    function refresh() {
      document.querySelectorAll('#sheet-body [data-cost], #modal [data-cost]').forEach((b) => b.classList.toggle('cant', state.cash < +b.dataset.cost));
      el.body.querySelectorAll('[data-status]').forEach((c) => {
        const st = STATUS[world.status(+c.dataset.status)] || STATUS.parked;
        if (c.textContent !== st[0]) { c.textContent = st[0]; c.className = 'chip ' + st[1]; }
      });
      if (tab !== 'team') return;
      if (tryoutState() !== tryKey) { render(); return; }
      el.body.querySelectorAll('[data-xp]').forEach((b) => { const d = G.getDriver(state, +b.dataset.xp); if (d) b.style.width = xpPct(d) + '%'; });
      const w = el.body.querySelector('[data-tryout-wait]');
      if (w) w.textContent = G.clock(G.tryoutWait(state, Date.now()));
    }

    /* ---------- Sheet and camera room ---------- */
    function layout(instant) {
      const app = $('app').getBoundingClientRect();
      const top = el.hud.getBoundingClientRect().bottom - app.top;
      const blocker = tab ? el.sheet : el.tabs;
      const bottom = app.bottom - blocker.getBoundingClientRect().top;
      world.setInsets(top + 4, bottom + 4, instant);
    }
    function openTab(name) {
      tab = tab === name ? null : name;
      armed = '';
      el.sheet.hidden = !tab;
      el.tabs.querySelectorAll('button').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.tab === tab)));
      if (tab) { el.body.scrollTop = 0; render(); }
      layout();
    }

    /* ---------- Modals ---------- */
    function showModal(html) { el.modal.innerHTML = '<div class="modal-card">' + html + '</div>'; el.modal.hidden = false; }
    function closeModal() { el.modal.hidden = true; el.modal.innerHTML = ''; armed = ''; }

    function prospectCard(p, i) {
      const now = G.driverStats(p), top = G.driverStats(Object.assign({}, p, { level: G.levelCap(p) }));
      const cell = (label, k) => '<div><small>' + label + '</small><b>' + now[k] + '</b><small>up to ' + top[k] + '</small></div>';
      return '<div class="prospect" style="--r:' + G.rarity(p.rarity).color + '"><div class="row"><div><strong>' + esc(p.name) + '</strong> ' + rarityTag(p) +
        '<small>Can reach level ' + G.levelCap(p) + '</small></div><button class="buy" data-act="sign" data-i="' + i + '">Sign</button></div>' +
        traitLine(p) + '<div class="grid3">' + cell('Skill', 'skill') + cell('Control', 'control') + cell('Flair', 'flair') + '</div></div>';
    }
    function viewTryout() {
      if (!state.prospects) return;
      const full = G.rosterFull(state);
      showModal('<h2>Tryout day</h2><p class="note">Sign one and the others go home. ' +
        (full ? 'Your team is full, so whoever you sign replaces a driver, who retires and mentors them.' : 'There is a free place on the team.') + '</p>' +
        state.prospects.map(prospectCard).join('') +
        '<div class="pair"><button class="ghost" data-act="close">Decide later</button><button class="ghost danger" data-act="dismiss">' +
        (armed === 'dismiss' ? 'Tap again to confirm' : 'Send them home') + '</button></div>');
    }
    function pickRetiree(i) {
      const p = state.prospects[i];
      let h = '<h2>Who makes way for ' + esc(p.name) + '?</h2><p class="note">They retire and mentor the newcomer, passing on half their experience. ' +
        esc(p.name) + ' takes their seat.</p><div class="list">';
      state.drivers.forEach((d) => {
        const after = G.mentored(p, d), car = G.carOfDriver(state, d.id);
        h += '<button class="pick" data-act="sign-over" data-i="' + i + '" data-driver="' + d.id + '"><span><strong>' + esc(d.name) + '</strong> ' + rarityTag(d) +
          '<small>' + statLine(d) + '</small><small>' + (car ? 'Drives the ' + esc(G.model(car.model).name) : 'Reserve') + '</small></span><em>Newcomer starts at Lv ' +
          after.level + '</em></button>';
      });
      showModal(h + '</div><button class="ghost wide" data-act="view-tryout">Back</button>');
    }
    function driverReveal(res) {
      const d = res.driver, r = G.rarity(d.rarity), car = G.carOfDriver(state, d.id);
      showModal('<div class="reveal" style="--r:' + r.color + '"><small>Signed</small><div class="helmet" style="background:' + d.helmet + '"><i></i></div>' +
        '<h2>' + esc(d.name) + '</h2>' + rarityTag(d) + traitLine(d) + '<div class="stats">' + stats(d) + '</div>' +
        '<p class="note">' + (res.retired ? esc(res.retired.name) + ' retires and stays on as mentor: ' + esc(d.name) + ' starts at level ' + d.level + '. ' : '') +
        (car ? 'Drives the ' + esc(G.model(car.model).name) + '.' : 'Reserve driver until a seat opens. Assign them from the Team tab.') + '</p>' +
        '<button class="buy wide" data-act="close">Welcome aboard</button></div>');
    }
    function pickDriver(carId) {
      const car = G.getCar(state, carId);
      let h = '<h2>Who drives the ' + esc(G.model(car.model).name) + '?</h2><div class="list">';
      state.drivers.forEach((d) => {
        const other = G.carOfDriver(state, d.id), mine = other === car;
        h += '<button class="pick' + (mine ? ' current' : '') + '" data-act="assign" data-car="' + carId + '" data-driver="' + d.id + '"><span><strong>' + esc(d.name) +
          '</strong> ' + rarityTag(d) + '<small>' + statLine(d) + '</small></span><em>' +
          (mine ? 'Driving it now' : other ? 'Swaps from pit ' + (state.cars.indexOf(other) + 1) : 'Reserve') + '</em></button>';
      });
      showModal(h + '</div><button class="ghost wide" data-act="close">Cancel</button>');
    }
    function pickCar(driverId) {
      const d = G.getDriver(state, driverId);
      let h = '<h2>Which car for ' + esc(d.name) + '?</h2><div class="list">';
      state.cars.forEach((c, i) => {
        const cur = G.driverOf(state, c);
        h += '<button class="pick' + (cur === d ? ' current' : '') + '" data-act="assign" data-car="' + c.id + '" data-driver="' + driverId + '"><span><strong><i class="swatch" style="background:' +
          c.color + '"></i> ' + esc(G.model(c.model).name) + '</strong><small>Pit ' + (i + 1) + '</small></span><em>' + (cur === d ? 'Driving it now' : cur ? 'Swaps with ' + esc(cur.name) : 'Empty seat') + '</em></button>';
      });
      showModal(h + '</div><button class="ghost wide" data-act="close">Cancel</button>');
    }
    function showAway(info) {
      showModal('<div class="reveal away"><small>While you were away</small><h2>' + G.money(info.cash) + '</h2><p class="note">The team kept drifting for ' +
        G.duration(info.paidSeconds) + (info.seconds > info.paidSeconds ? ' (the crowd goes home after ' + G.duration(D.OFFLINE_CAP) + ')' : '') +
        (info.levelUps ? '. ' + info.levelUps + (info.levelUps === 1 ? ' driver' : ' drivers') + ' levelled up' : '') +
        '.</p><button class="buy wide" data-act="close">Collect</button></div>');
    }

    /* ---------- Clicks ---------- */
    function sign(i, retireId) {
      const res = G.signProspect(state, i, retireId);
      if (!res) return false;
      driverReveal(res);
      return true;
    }
    function onAct(b) {
      const a = b.dataset.act, carId = +b.dataset.car, driverId = +b.dataset.driver, i = +b.dataset.i;
      let changed = false;
      if (a === 'toggle-car') { openCar = openCar === carId ? null : carId; render(); return; }
      if (a === 'close') { closeModal(); return; }
      if (a === 'pick-driver') { pickDriver(carId); return; }
      if (a === 'pick-car') { if (state.cars.length) pickCar(driverId); return; }
      if (a === 'view-tryout') { armed = ''; viewTryout(); return; }
      if (a === 'reset') {
        if (armed !== 'reset') { armed = 'reset'; render(); return; }
        actions.reset(); return;
      }
      if (a === 'part') changed = G.buyPart(state, carId, b.dataset.part);
      else if (a === 'trade') changed = G.tradeCar(state, carId, b.dataset.model);
      else if (a === 'buy-car') { changed = G.buyCar(state, b.dataset.model); if (changed) openCar = state.cars[state.cars.length - 1].id; }
      else if (a === 'slot') changed = G.buySlot(state);
      else if (a === 'venue') changed = G.buyVenue(state, b.dataset.id);
      else if (a === 'hire') changed = G.hireMech(state);
      else if (a === 'train') changed = G.trainMech(state);
      else if (a === 'assign') { changed = G.assignDriver(state, carId, driverId); closeModal(); }
      else if (a === 'tryout') { changed = G.holdTryout(state, Date.now(), Math.random); if (changed) viewTryout(); }
      else if (a === 'sign') { if (G.rosterFull(state)) { pickRetiree(i); return; } changed = sign(i); }
      else if (a === 'sign-over') changed = sign(i, driverId);
      else if (a === 'dismiss') {
        if (armed !== 'dismiss') { armed = 'dismiss'; viewTryout(); return; }
        changed = G.dismissProspects(state); closeModal();
      }
      if (changed) { actions.changed(); render(); }
      else if (b.dataset.cost) { b.classList.remove('shake'); void b.offsetWidth; b.classList.add('shake'); }
    }
    document.addEventListener('click', (e) => {
      const t = e.target.closest('[data-tab]');
      if (t) { openTab(t.dataset.tab); return; }
      if (e.target.closest('#sheet-close')) { openTab(tab); return; }
      const b = e.target.closest('[data-act]');
      if (b && !b.disabled) onAct(b);
    });
    root.addEventListener('resize', () => layout(true));

    /* ---------- Pop-ups over the 3D view ---------- */
    const pool = [];
    function popup(x, y, z, text, kind) {
      const p = world.toScreen(x, y, z);
      const n = pool.pop() || document.createElement('div');
      n.className = 'pop ' + kind;
      n.textContent = text;
      n.style.left = p.x + 'px'; n.style.top = p.y + 'px';
      el.popups.appendChild(n);
      setTimeout(() => { n.remove(); pool.push(n); }, 1100);
    }
    let toastT = 0;
    function toast(text) {
      el.toast.textContent = text; el.toast.classList.add('show');
      clearTimeout(toastT);
      toastT = setTimeout(() => el.toast.classList.remove('show'), 2400);
    }
    function levelUp(d) {
      const car = G.carOfDriver(state, d.id), p = car && world.pos(car.id);
      if (p) popup(p.x, 5, p.z, 'Level ' + d.level + '!', 'good');
      toast(d.name + ' reached level ' + d.level + (d.level >= G.levelCap(d) ? ', their top level.' : '.'));
      if (tab) render();
    }
    const marks = new Map();
    function drawIndicators() {
      const seen = new Set();
      world.indicators().forEach((it) => {
        let n = marks.get(it.id);
        if (!n) { n = document.createElement('div'); n.innerHTML = '<i></i>'; el.popups.appendChild(n); marks.set(it.id, n); }
        n.className = 'fix' + (it.repairing ? ' on' : '');
        n.style.left = it.x + 'px'; n.style.top = it.y + 'px';
        n.style.setProperty('--p', Math.round(it.p * 100) + '%');
        seen.add(it.id);
      });
      marks.forEach((n, id) => { if (!seen.has(id)) { n.remove(); marks.delete(id); } });
    }

    /* ---------- Per-frame ---------- */
    let slow = 0, shownCash = -1;
    function frame(inc, dt) {
      const cash = Math.floor(state.cash);
      if (cash !== shownCash) { shownCash = cash; el.cash.textContent = G.money(state.cash); }
      drawIndicators();
      slow -= dt;
      if (slow > 0) return;
      slow = 0.25;
      el.rate.textContent = G.money(inc.cash, 1) + '/s';
      el.meterFill.style.width = Math.min(100, inc.cap ? (inc.style / inc.cap) * 100 : 0) + '%';
      el.meter.classList.toggle('full', inc.full);
      el.meterText.textContent = inc.full ? 'Crowd maxed: ' + G.fmt(inc.style, 1) + ' of ' + G.fmt(inc.cap, 1) + ' style/s. Build grandstands'
        : 'Style ' + G.fmt(inc.style, 1) + ' / ' + G.fmt(inc.cap, 1) + ' the crowd can cheer';
      refresh();
    }

    layout(true);
    return { frame, render, popup, toast, showAway, levelUp, layout, openTab, isOpen: () => tab };
  }

  DT.UI = UI;
})(window);
