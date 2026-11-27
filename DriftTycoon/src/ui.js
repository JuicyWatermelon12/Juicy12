/* Drift Tycoon — the HTML interface over the 3D view: money bar, tabs, upgrade sheets, pop-ups. */
(function (root) {
  'use strict';
  const DT = root.DT, D = DT.data, G = DT.game, C = DT.touge, T = D.TOUGE;
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
    const drift = { kind: 'street', street: D.STREET_MAPS[0].id, track: '' };   // what the Drift menu last chose
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
    const placeName = (i) => (D.VENUES[i] ? 'the ' + D.VENUES[i].name : 'a later venue');
    const stagePlace = (stage) => placeName(D.VENUES.findIndex((v) => v.stage >= stage));
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
        h += '<div class="row driver-row"><div><small>Looks</small><div>' + esc(G.cosmetic('paint', car.paint).name) + ' · ' + esc(G.cosmetic('rims', car.rims).name) +
          ' · ' + esc(G.cosmetic('kit', car.kit).name) + (car.glow !== 'none' ? ' · ' + esc(G.cosmetic('glow', car.glow).name) : '') + '</div><small class="delta">+' + Math.round((G.looksMult(car) - 1) * 100) + '% style from looks</small></div><button class="ghost" data-act="style" data-car="' + car.id + '">Customise</button></div>';
        h += '<div class="parts">';
        D.PARTS.forEach((p) => {
          const lv = car.parts[p.id], maxed = lv >= G.partCap(state), last = lv >= D.PART_MAX;
          h += '<div class="row part"><div><strong>' + p.name + '</strong> <span class="lv">' + D.PART_STAGES[lv] + '</span>' + pips(lv) + '<small>' + p.blurb +
            '</small><small class="delta">' + (last ? 'Style ' + pct(lv * p.per) : maxed ? 'Style ' + pct(lv * p.per) + '. ' + D.PART_STAGES[lv + 1] + ' goes on sale at ' + stagePlace(lv + 1)
              : D.PART_STAGES[lv + 1] + ': style ' + pct(lv * p.per) + ' → ' + pct((lv + 1) * p.per)) +
            '</small></div>' + buy('', maxed ? Infinity : G.partCost(car, p.id), 'part', { car: car.id, part: p.id }) + '</div>';
        });
        h += '</div>';
        const next = D.MODELS[G.modelIndex(car.model) + 1];
        if (next && G.modelUnlocked(state, next.id)) {
          h += '<div class="row trade"><div><strong>Trade up: ' + esc(next.name) + '</strong><small>' + esc(next.blurb) + '</small><small class="delta">Base style ' +
            G.fmt(m.base) + ' → ' + G.fmt(next.base) + '. Parts and driver move across.</small></div>' +
            buy('', next.cost, 'trade', { car: car.id, model: next.id }, 'alt') + '</div>';
        } else if (next) {
          h += '<div class="row trade"><div><strong>Next car: ' + esc(next.name) + '</strong><small>On sale at ' + placeName(G.modelPlace(next.id)) + '.</small></div></div>';
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
          const open = G.modelUnlocked(state, m.id);
          h += '<div class="row' + (open ? '' : ' dim') + '"><div><strong>' + esc(m.name) + '</strong><small>' + esc(m.blurb) + ' Base style ' + G.fmt(m.base) + '/s.' +
            (open ? '' : ' On sale at ' + placeName(G.modelPlace(m.id)) + '.') + '</small></div>' + (open ? buy('', m.cost, 'buy-car', { model: m.id }) : '') + '</div>';
        });
        h += '</article>';
      } else if (state.slots < G.maxPits(state)) {
        h += '<article class="card"><div class="row"><div><strong>Open pit ' + (state.slots + 1) + '</strong><small>Room for one more car on track, and one more driver on the team.</small></div>' +
          buy('', G.slotCost(state), 'slot') + '</div></article>';
      } else if (G.nextPlace(state)) {
        h += '<article class="card"><p class="note">' + esc(G.place(state).name) + ' has ' + G.maxPits(state) + ' pits, all open. ' + esc(G.nextPlace(state).name) + ' has ' +
          G.nextPlace(state).pits + '.</p></article>';
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
      const inc = G.incomeRate(state), here = G.place(state), next = G.nextPlace(state);
      let h = '<article class="card"><h3>' + esc(here.name) + '</h3><p class="note">' + esc(here.blurb) + ' Style from the cars turns into cash only as fast as the crowd can cheer: right now ' +
        G.fmt(inc.cap, 1) + ' style/s.' + (here.style > 1 ? ' Everything scores x' + here.style + ' style here.' : '') + '</p></article>';
      const now = { stands: G.fmt(G.crowdCap(state), 1) + ' style/s cheered', sponsors: 'Cash x' + G.sponsorMult(state).toFixed(2), prep: 'Style x' + (G.prepMult(state) / here.style).toFixed(2) };
      const then = {
        stands: G.fmt(G.crowdCap(state) * D.CROWD_GROWTH, 1) + ' style/s', sponsors: 'x' + (G.sponsorMult(state) * D.SPONSOR_MULT).toFixed(2),
        prep: 'x' + (G.prepMult(state) / here.style * D.PREP_MULT).toFixed(2),
      };
      D.VENUE.forEach((v) => {
        const top = state.venue[v.id] >= D.VENUE_MAX[v.id];
        h += '<article class="card"><div class="row"><div><strong>' + v.name + '</strong> <span class="lv">Lv ' + state.venue[v.id] + ' / ' + D.VENUE_MAX[v.id] + '</span><small>' + v.blurb +
          '</small><small class="delta">' + now[v.id] + (top ? '' : ' → ' + then[v.id]) + '</small></div>' + buy('', G.venueCost(state, v.id), 'venue', { id: v.id }) + '</div></article>';
      });
      h += '<article class="card"><div class="row"><div><strong>Pits: ' + state.slots + ' of ' + G.maxPits(state) + '</strong><small>Each pit is one more car on track.</small></div>' +
        buy('', G.slotCost(state), 'slot') + '</div></article>';
      if (state.placeOwned > 0) {   // every venue bought is kept, with its own upgrades
        h += '<h3 class="section">Your venues</h3>';
        D.VENUES.forEach((v, n) => {
          if (!G.ownsPlace(state, n)) return;
          const lv = state.levels[n] || {}, at = n === state.place;
          h += '<article class="card' + (at ? ' here' : '') + '"><div class="row"><div><strong>' + esc(v.name) + '</strong><small>x' + v.style + ' style · ' +
            D.VENUE.map((u) => u.name + ' ' + (lv[u.id] || 0)).join(' · ') + '</small></div>' +
            (at ? '<span class="tag">Team is here</span>' : '<button class="buy alt" data-act="move-place" data-place="' + n + '"><span class="buy-cost">Move team</span></button>') +
            '</div></article>';
        });
        h += '<p class="note">The team drifts for the crowd at one venue at a time. Each venue keeps its own grandstands, sponsors and track prep, ' +
          'and every track you own can be driven in Drift mode.</p>';
      }
      if (next) {
        const cars = next.cars.map((id) => G.model(id).name).join(', '), best = G.bestPlace(state);
        h += '<h3 class="section">Next venue</h3><article class="card next-place"><div class="row"><div><strong>' + esc(next.name) + '</strong><small>' + esc(next.blurb) + '</small>' +
          '<small class="delta">' + next.pits + ' pits · x' + (next.style / best.style) + ' style · ' + D.PART_STAGES[next.stage] + ' parts · new car: ' + esc(cars) + '</small></div>' +
          buy('Buy venue', next.cost, 'buy-place') + '</div><p class="note">' +
          'Yours for good once bought, along with everything you already own. Its car, pits and parts go on sale at once, and its track opens in Drift mode. ' +
          'Move the team there when you are ready: its grandstands, sponsors and track prep start from zero.</p></article>';
      } else {
        h += '<article class="card"><p class="note">You own every venue there is for now. More are on the way.</p></article>';
      }
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
    function showMenu() {
      const on = armed === 'reset';
      showModal('<h2>Menu</h2><p class="note">Wipes this team for good: cash, cars, parts, drivers, mechanics and venue upgrades. You start again with one Kumo and the rookie.</p>' +
        '<button class="ghost wide danger' + (on ? ' armed' : '') + '" data-act="reset">' + (on ? 'Tap again to wipe everything' : 'Reset all progress') + '</button>' +
        '<button class="ghost wide" data-act="close">Close</button>');
    }
    /* Called after the state has been wiped in place. */
    function afterReset() {
      closeModal();
      openCar = state.cars[0] ? state.cars[0].id : null;
      shownCash = -1;
      if (tab) { el.body.scrollTop = 0; render(); }
      toast('Progress wiped. Fresh team, fresh tires.');
    }
    /* ---------- Cosmetics and street cred ---------- */
    function rankBar() {
      const r = G.rank(state);
      return '<div class="rankbar"><div><b>Rank ' + r.index + ' · ' + r.name + '</b><small>' + (r.next ? Math.floor(r.cred) + ' / ' + r.next.cred + ' street cred' : 'Top rank · ' +
        Math.floor(r.cred) + ' street cred') + '</small></div><i><b style="width:' + Math.round(r.progress * 100) + '%"></b></i></div>';
    }
    function cosTile(car, kind, c) {
      const worn = car[kind] === c.id, owned = G.ownsCosmetic(state, kind, c.id, car.model), open = G.cosmeticUnlocked(state, kind, c.id);
      const cost = G.cosmeticCost(kind, c.id, car.model);
      const art = kind === 'paint' ? '<i class="dot' + (c.finish ? ' ' + c.finish : '') + '" style="background:' + c.color + ';--c2:' + (c.color2 || c.color) + '"></i>'
        : kind === 'rims' ? '<i class="wheel' + (c.spokes ? ' spoked' : '') + '" style="--c:' + c.color + '"></i>'
        : kind === 'glow' ? '<i class="glow' + (c.rainbow ? ' rainbow' : '') + (c.cost ? '' : ' off') + '" style="--c:' + c.color + '"></i>' : '';
      const plus = c.style ? '<small class="delta">+' + Math.round(c.style * 100) + '% style</small>' : '';
      const foot = worn ? '<em class="on">On the car</em>' : owned ? '<em>Owned · tap to fit</em>'
        : open ? '<em class="price">' + G.money(cost) + '</em>' : '<em class="lock">Rank ' + c.rank + ' · ' + D.RANKS[c.rank].name + '</em>';
      return '<button class="cos' + (worn ? ' worn' : '') + (!owned && !open ? ' locked' : '') + '" data-act="cos" data-car="' + car.id + '" data-kind="' + kind +
        '" data-id="' + c.id + '"' + (!owned && open ? ' data-cost="' + cost + '"' : '') + '>' + art + '<span>' + esc(c.name) +
        (kind === 'kit' ? '<small>' + esc(c.blurb) + '</small>' : '') + plus + '</span>' + foot + '</button>';
    }
    function styleCar(carId) {
      const car = G.getCar(state, carId);
      if (!car) return;
      const sec = (title, note, kind, list, cls) => '<h3>' + title + '</h3><p class="note">' + note + '</p><div class="cos-grid ' + cls + '">' +
        list.map((c) => cosTile(car, kind, c)).join('') + '</div>';
      showModal('<h2>' + esc(G.model(car.model).name) + ' looks</h2><p class="note">The crowd pays for a good-looking car: paint, rims, kit and underglow each add style at the venue. This car has <b>+' +
        Math.round((G.looksMult(car) - 1) * 100) + '% style</b> from its looks. They do not change how it drives on the street. ' +
        'Street cred rank puts new items on sale; you earn it in Drift mode.</p>' + rankBar() +
        sec('Paint', 'Yours for the whole team once bought.', 'paint', D.PAINTS, 'small') +
        sec('Rims', 'Yours for the whole team once bought.', 'rims', D.RIMS, '') +
        sec('Body kit', 'Bought for this model, the ' + esc(G.model(car.model).name) + '.', 'kit', D.KITS, 'wide') +
        sec('Underglow', 'A coloured light under the car. Yours for the whole team once bought.', 'glow', D.GLOWS, 'small') +
        '<button class="ghost wide" data-act="close">Done</button>');
    }

    /* ---------- Drift mode: Street maps, or the Track of any venue owned ---------- */
    const trackKey = (id) => 'track:' + id;
    const bestLine = (key) => {
      const b = state.street.best[key];
      return b ? 'Best ' + b.score.toLocaleString('en-US') + ' pts · ' + (b.dist / 1000).toFixed(2) + ' km' : 'Not driven yet';
    };
    function driftMap() {   // what the menu has chosen, as touge.start wants it
      if (drift.kind === 'track') {
        const n = D.VENUES.findIndex((v) => v.id === drift.track);
        return { kind: 'track', id: G.ownsPlace(state, n) ? drift.track : G.place(state).id };
      }
      return { kind: 'street', id: D.STREET_MAPS.some((m) => m.id === drift.street) ? drift.street : D.STREET_MAPS[0].id };
    }
    function mapName(key) {
      if (key.indexOf('track:') === 0) { const v = D.VENUES.find((x) => trackKey(x.id) === key); return v ? v.name + ' track' : 'Track'; }
      const m = D.STREET_MAPS.find((x) => x.id === key);
      return m ? m.name : 'Street';
    }
    function tougeMenu() {
      const income = G.expectedIncome(state), sel = driftMap(), track = sel.kind === 'track';
      let h = '<h2>Drift</h2><p class="note">Drift one of your cars yourself. The car drives; you slide a finger left and right to steer. ' +
        'Hold it sideways to build the multiplier. ' + (T.MAX_HITS === 1 ? 'Touching a barrier ends the run' : 'Barrier touch number ' + T.MAX_HITS + ' ends the run') +
        ', and you keep everything you scored.</p><p class="note"><b>The money is far down the road.</b> Points pay in team income (now ' + G.money(income, 1) +
        '/s), and the pay rate rises every kilometre: x' + T.PAY_PER_KM[0] + ' in the first, x' + T.PAY_PER_KM[2] + ' in the third, x' +
        T.PAY_PER_KM[T.PAY_PER_KM.length - 1] + ' from the ' + T.PAY_PER_KM.length + 'th on. Runs of about ' + G.place(state).bar + ' km keep you on pace at the ' + esc(G.place(state).name) + '.</p>' +
        '<div class="seg"><button data-act="drift-kind" data-kind="street" aria-pressed="' + !track + '">Street</button>' +
        '<button data-act="drift-kind" data-kind="track" aria-pressed="' + track + '">Track</button></div><div class="list">';
      if (track) {
        D.VENUES.forEach((v, n) => {
          if (!G.ownsPlace(state, n)) { h += '<div class="pick locked"><span><strong>' + esc(v.name) + '</strong><small>Buy this venue to drive its track</small></span><em>Not owned</em></div>'; return; }
          h += '<button class="pick' + (v.id === sel.id ? ' current' : '') + '" data-act="drift-map" data-map="' + v.id + '"><span><strong>' + esc(v.name) + '</strong><small>' +
            bestLine(trackKey(v.id)) + '</small></span><em>' + (DT.tracks.trace(v.id).length * D.TRACK_SCALE / 1000).toFixed(2) + ' km lap</em></button>';
        });
      } else {
        D.STREET_MAPS.forEach((m) => {
          h += '<button class="pick' + (m.id === sel.id ? ' current' : '') + '" data-act="drift-map" data-map="' + m.id + '"><span><strong>' + esc(m.name) + '</strong><small>' +
            bestLine(m.id) + '</small></span><em>Endless</em></button>';
        });
      }
      h += '</div><p class="note">' + (track ? 'Lap the drift track of a venue you own, at full size. The laps never end and the pace rises every kilometre.'
        : esc((D.STREET_MAPS.find((m) => m.id === sel.id) || {}).blurb || '') + ' More street maps are on the way.') + '</p>' + rankBar() +
        '<h3>Pick a car to start</h3><div class="list">';
      let any = false;
      state.cars.forEach((c, i) => {
        const d = G.driverOf(state, c);
        if (!d) return;
        any = true;
        const s = C.carSetup(state, c);
        h += '<button class="pick" data-act="touge-go" data-car="' + c.id + '"><span><strong><i class="swatch" style="background:' + c.color + '"></i> ' +
          esc(G.model(c.model).name) + '</strong><small>' + esc(d.name) + ' · pit ' + (i + 1) + '</small></span><em>' + Math.round(s.top * 3.6) + ' km/h · ' +
          Math.round(s.maxSlip * 180 / Math.PI) + '° · score x' + s.bonus.toFixed(2) + '</em></button>';
      });
      if (!any) h += '<p class="note">No car has a driver yet.</p>';
      showModal(h + '</div><button class="ghost wide" data-act="close">Not now</button>');
    }
    /* What each kilometre of a run paid, as small bars: the money is far down the road. */
    function kmChart(res) {
      const list = res.perKm.slice(-12), top = Math.max(1e-9, ...list.map((k) => k.cash));
      if (!list.length) return '';
      const best = list.reduce((a, b) => (b.cash > a.cash ? b : a));
      return '<p class="note">Pay by kilometre: the further you go, the more every point pays. ' +
        (res.perKm.length > 1 ? 'Kilometre ' + best.km + ' paid most: ' + G.money(best.cash, 1) + ' at x' + best.rate + '.' : '') + '</p>' +
        '<div class="km-chart">' + list.map((k, i) => '<div' + (i === list.length - 1 && res.dist < k.km * 1000 ? ' class="part"' : '') + ' title="Km ' + k.km + ': ' +
          G.money(k.cash, 1) + ' at x' + k.rate + '"><i style="height:' + Math.max(2, Math.round(k.cash / top * 46)) + 'px"></i><span>' + k.km + '</span></div>').join('') + '</div>';
    }
    function showTougeResult(res) {
      const d = res.driver, row = (a, b) => '<div><span>' + a + '</span><b>' + b + '</b></div>';
      const best = (on) => (on ? ' <span class="rarity" style="--r:var(--accent)">New best</span>' : '');
      const track = res.map.indexOf('track:') === 0;
      showModal('<div class="reveal away"><small>' + esc(mapName(res.map)) + ' · ' + (res.out ? 'hit the barrier' : 'run banked') + '</small><h2>' + G.money(res.cash, 1) + '</h2>' +
        '<div class="result-rows">' + row('Points' + best(res.bestScore), res.score.toLocaleString('en-US')) +
        row('Distance' + best(res.bestDist), (res.dist / 1000).toFixed(2) + ' km') + (track ? row('Laps', res.laps) : '') + row('Best multiplier', 'x' + res.bestMult.toFixed(1)) +
        row('Worth', G.duration(res.seconds) + ' of team income') + '</div>' + kmChart(res) +
        '<p class="note">+' + res.cred + ' street cred' + (res.rank.after.index > res.rank.before.index ? '. New rank: <b>' + res.rank.after.name + '</b>' +
          (res.rank.unlocked.length ? '. Now on sale: ' + res.rank.unlocked.map((u) => esc(u.item.name)).join(', ') : '') : '') + '.</p>' + rankBar() +
        (d ? '<p class="note">' + esc(d.name) + ' banked ' + G.duration(res.xp) + ' of seat time' + (res.levels ? ' and reached level ' + d.level : '') + '.</p>' : '') +
        '<button class="buy wide" data-act="touge-again">Go again</button><button class="ghost wide" data-act="touge-menu">Change map or car</button>' +
        '<button class="ghost wide" data-act="touge-exit">Back to the venue</button></div>');
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
      if (a === 'bubble') { popBubbleNow(); return; }
      if (a === 'menu') { armed = ''; showMenu(); return; }
      if (a === 'style') { styleCar(carId); return; }
      if (a === 'cos') {
        const kind = b.dataset.kind, id = b.dataset.id, car = G.getCar(state, carId);
        if (!car || car[kind] === id) return;
        const keep = el.modal.firstChild.scrollTop;
        const done = G.ownsCosmetic(state, kind, id, car.model) ? G.applyCosmetic(state, carId, kind, id) : G.buyCosmetic(state, carId, kind, id);
        if (done) { actions.changed(); render(); styleCar(carId); el.modal.firstChild.scrollTop = keep; }
        else if (b.dataset.cost) { b.classList.remove('shake'); void b.offsetWidth; b.classList.add('shake'); }
        return;
      }
      if (a === 'touge-menu') {
        if (actions.tougeActive()) { actions.tougeExit(); shownCash = -1; if (tab) render(); layout(true); }   // from a result: leave the run first
        if (!drift.track) drift.track = G.place(state).id;
        tougeMenu(); return;
      }
      if (a === 'drift-kind') { drift.kind = b.dataset.kind === 'track' ? 'track' : 'street'; if (!drift.track) drift.track = G.place(state).id; tougeMenu(); return; }
      if (a === 'drift-map') { drift[drift.kind] = b.dataset.map; tougeMenu(); return; }
      if (a === 'touge-go') { const where = driftMap(); closeModal(); actions.tougeStart(carId, where); return; }
      if (a === 'touge-again') { closeModal(); actions.tougeAgain(); return; }
      if (a === 'touge-quit') { actions.tougeQuit(); return; }
      if (a === 'touge-exit') { closeModal(); actions.tougeExit(); shownCash = -1; if (tab) render(); layout(true); return; }
      if (a === 'reset') {
        if (armed !== 'reset') { armed = 'reset'; showMenu(); return; }
        actions.reset(); afterReset(); return;
      }
      if (a === 'part') changed = G.buyPart(state, carId, b.dataset.part);
      else if (a === 'trade') changed = G.tradeCar(state, carId, b.dataset.model);
      else if (a === 'buy-car') { changed = G.buyCar(state, b.dataset.model); if (changed) openCar = state.cars[state.cars.length - 1].id; }
      else if (a === 'slot') changed = G.buySlot(state);
      else if (a === 'buy-place') { changed = G.buyPlace(state); if (changed) toast(G.bestPlace(state).name + ' bought. Move the team when you are ready.'); }
      else if (a === 'move-place') {
        changed = G.movePlace(state, +b.dataset.place);
        if (changed) { el.body.scrollTop = 0; toast('Welcome to the ' + G.place(state).name + '.'); }
      }
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

    /* ---------- Tap bubbles ---------- */
    const SPOTS = { wave: [-21, 8, 2], meet: [0, 10, -6] };   // where the crowd and meet bubbles rise from; a perfect-drift one follows a car
    let bubbleIn = G.bubbleGap(), bubbleNow = null;
    const fxEl = document.createElement('div');
    fxEl.id = 'fx';
    el.hud.appendChild(fxEl);
    function bubblePoint(b) {
      if (b.id === 'perfect') { const p = world.pos(b.car); return p ? [p.x, 6, p.z] : null; }
      return SPOTS[b.id];
    }
    function spawnBubble(id) {
      const ids = world.runningIds();
      if (!G.bubble(id) || (id === 'perfect' && !ids.length)) return false;
      dropBubble();
      const node = document.createElement('button');
      node.className = 'bubble ' + id;
      node.dataset.act = 'bubble';
      node.setAttribute('aria-label', G.bubble(id).name);
      node.innerHTML = '<i></i><b>' + (id === 'perfect' ? '$' : id === 'wave' ? 'Wave' : 'Meet') + '</b>';
      el.popups.appendChild(node);
      bubbleNow = { id, node, left: D.BUBBLE_LIFE, car: ids[Math.floor(Math.random() * ids.length)] };
      return true;
    }
    function dropBubble() { if (bubbleNow) { bubbleNow.node.remove(); bubbleNow = null; } }
    function popBubbleNow() {
      const b = bubbleNow;
      if (!b) return;
      const def = G.bubble(b.id), at = bubblePoint(b) || [0, 6, 0];
      const cash = G.popBubble(state, b.id);
      popup(at[0], at[1], at[2], cash ? '+' + G.money(cash, 1) : def.name + '!', 'good big');
      if (def.time) toast(def.id === 'meet' ? 'Drift meet! The cars run as a train and cash is doubled for ' + def.time + ' s.'
        : 'Crowd wave: the crowd cheers twice as much for ' + def.time + ' s.');
      dropBubble();
      actions.changed();
    }
    function bubbles(dt) {
      // effect chips under the money bar
      const key = state.fx.map((f) => f.id + Math.ceil(f.left)).join(',');
      if (key !== fxEl.dataset.key) {
        fxEl.dataset.key = key;
        fxEl.innerHTML = state.fx.map((f) => '<span class="' + f.id + '">' + G.bubble(f.id).name + ' ' + G.clock(f.left) + '</span>').join('');
      }
      if (!bubbleNow) {
        bubbleIn -= dt;
        if (bubbleIn > 0) return;
        bubbleIn = G.bubbleGap();
        if (!spawnBubble(G.rollBubble(state))) return;
      }
      const b = bubbleNow, at = bubblePoint(b);
      b.left -= dt;
      if (b.left <= 0 || !at) { dropBubble(); return; }
      const p = world.toScreen(at[0], at[1], at[2]);
      b.node.style.left = p.x + 'px'; b.node.style.top = p.y + 'px';
      b.node.classList.toggle('going', b.left < 2.5);
    }

    /* ---------- Per-frame ---------- */
    let slow = 0, shownCash = -1;
    function frame(inc, dt) {
      const cash = Math.floor(state.cash);
      if (cash !== shownCash) { shownCash = cash; el.cash.textContent = G.money(state.cash); }
      drawIndicators();
      bubbles(dt);
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
    return { spawnBubble, frame, render, popup, toast, showAway, showTougeResult, levelUp, layout, openTab, isOpen: () => tab };
  }

  DT.UI = UI;
})(window);
