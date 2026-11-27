/* Drift Tycoon — boot, save and the frame loop. */
(function (root) {
  'use strict';
  const DT = root.DT, G = DT.game;
  const SAVE_KEY = 'drifttycoon.save';   // the save's own `v` field carries its version; other versions start a new team

  function readSave() { try { return root.localStorage.getItem(SAVE_KEY); } catch (e) { return null; } }
  function writeSave(state) {
    state.lastSeen = Date.now();
    try { root.localStorage.setItem(SAVE_KEY, G.serialize(state)); } catch (e) { /* private window: play without saving */ }
  }

  function start() {
    const saved = readSave();
    const state = saved ? G.load(saved, Date.now()) : G.newState(Date.now());
    const canvas = document.getElementById('view');
    let world, ui;
    try {
      world = DT.World(canvas, state, {
        popup: (x, y, z, text, kind) => ui && ui.popup(x, y, z, text, kind),
        crash: (car, x, z) => {
          if (!ui) return;
          const d = G.driverOf(state, car);
          ui.popup(x, 3, z, 'Crash!', 'bad');
          ui.toast((d ? d.name : 'A driver') + ' binned the ' + G.model(car.model).name + '. Mechanics are on it.');
        },
      });
    } catch (e) {
      console.error(e);
      document.getElementById('nogl').hidden = false;
      return;
    }
    ui = DT.UI(state, world, {
      changed: () => { world.sync(); writeSave(state); },
      tougeStart: (carId, where) => touge.start(carId, where),
      tougeActive: () => touge.active(),
      tougeAgain: () => touge.again(),
      tougeQuit: () => touge.quit(),
      tougeExit: () => touge.stop(),
      // Wipe in place (no page reload, which a sandboxed page may not allow): empty the state object that
      // the world and the UI already hold, refill it as a new team, and save over the old progress.
      reset: () => {
        const keep = state.settings;   // settings are not progress: they survive a reset
        Object.keys(state).forEach((k) => delete state[k]);
        Object.assign(state, G.newState(Date.now()));
        if (keep) state.settings = keep;
        world.sync();
        writeSave(state);
      },
    });

    const touge = DT.Touge(world, state, { done: (res) => { ui.showTougeResult(res); writeSave(state); } });

    const away = saved ? G.offlineEarnings(state, Date.now()) : { cash: 0 };
    if (away.cash > 0) {
      G.earn(state, away.cash);
      away.levelUps = G.awayXP(state, away.paidSeconds).length;
      ui.showAway(away);
    }
    writeSave(state);

    let last = performance.now(), saveT = 0;
    function frame(now) {
      let dt = (now - last) / 1000;
      last = now;
      if (dt > 1) {   // the tab was in the background: pay the long-run rate for the gap, then carry on
        const gap = Math.min(dt, DT.data.OFFLINE_CAP);
        G.earn(state, G.expectedIncome(state) * gap);
        G.awayXP(state, gap).forEach((d) => ui.levelUp(d));
        dt = 0.016;
      }
      dt = Math.min(dt, 0.05);
      if (touge.active()) {   // out on the pass: the venue is paused and the team earns its long-run rate meanwhile
        G.earn(state, G.expectedIncome(state) * dt);
        G.awayXP(state, dt);
        touge.frame(dt);
        saveT += dt;
        if (saveT > 5) { saveT = 0; writeSave(state); }
        root.requestAnimationFrame(frame);
        return;
      }
      world.update(dt);
      const running = world.runningIds();
      const inc = G.incomeRate(state, running);
      const earned = inc.cash * dt;
      G.earn(state, earned);
      if (inc.style > 0) {
        running.forEach((id) => world.addEarnings(id, earned * G.carStyle(state, G.getCar(state, id)) / inc.style));
      }
      G.tickXP(state, running, dt).forEach((d) => ui.levelUp(d));
      G.tickFx(state, dt);
      world.hype = G.boost(state).crowd > 1 ? 1 : inc.cap ? Math.min(1, inc.style / inc.cap) : 0;
      ui.frame(inc, dt);
      world.render();
      saveT += dt;
      if (saveT > 5) { saveT = 0; writeSave(state); }
      root.requestAnimationFrame(frame);
    }
    root.requestAnimationFrame(frame);
    document.addEventListener('visibilitychange', () => { if (document.hidden) writeSave(state); });
    root.addEventListener('pagehide', () => writeSave(state));
    DT.debug = { state, world, ui, touge, save: () => writeSave(state) };
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start);
  else start();
})(window);
