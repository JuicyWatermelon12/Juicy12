/* Drift Tycoon — tuning data. Every number the game uses lives here. */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});

  DT.data = {
    START_CASH: 30,

    /* Parts: each level adds `per` to that part's multiplier (parts multiply each other).
       `crash` multiplies the car's crash rate once per level. */
    PART_GROWTH: 1.32,
    PART_MAX: 25,
    PARTS: [
      { id: 'engine', name: 'Engine', blurb: 'More power down the straights', per: 0.25, cost: 12, crash: 1.0 },
      { id: 'turbo', name: 'Turbo', blurb: 'Big boost, a little more risk', per: 0.3, cost: 40, crash: 1.02 },
      { id: 'tires', name: 'Tires', blurb: 'More smoke and more grip', per: 0.15, cost: 8, crash: 0.96 },
      { id: 'suspension', name: 'Suspension', blurb: 'Keeps the car off the wall', per: 0.08, cost: 15, crash: 0.92 },
      { id: 'stance', name: 'Stance', blurb: 'Low and wide. The crowd loves it', per: 0.12, cost: 20, crash: 1.0 },
      { id: 'angle', name: 'Angle kit', blurb: 'More steering lock, bigger angle', per: 0.18, cost: 30, crash: 1.0 },
    ],

    /* Cars are fictional. `base` is style points a second before parts and driver.
       Part prices scale with base / 2. `body` drives the low-poly model. */
    MODELS: [
      { id: 'kumo', name: 'Kumo 180', blurb: 'Light pop-up-lamp coupe. Every team starts here.', cost: 100, base: 2,
        body: { len: 4.1, wid: 1.75, cabLen: 2.0, cabOff: -0.35, cabH: 0.58, tf: 0.55, tr: 0.75, wing: 0 } },
      { id: 'hachi', name: 'Hachi Coupe', blurb: 'Boxy little hatch that punches up.', cost: 2000, base: 10,
        body: { len: 3.9, wid: 1.7, cabLen: 2.1, cabOff: -0.45, cabH: 0.62, tf: 0.5, tr: 0.3, wing: 0 } },
      { id: 'kaze', name: 'Kaze Tourer', blurb: 'Four doors, straight six, endless smoke.', cost: 40000, base: 55,
        body: { len: 4.6, wid: 1.8, cabLen: 2.4, cabOff: -0.2, cabH: 0.6, tf: 0.55, tr: 0.55, wing: 0 } },
      { id: 'rotor', name: 'Rotor Seven', blurb: 'Screaming rotary with a long nose.', cost: 800000, base: 320,
        body: { len: 4.3, wid: 1.85, cabLen: 1.8, cabOff: -0.5, cabH: 0.52, tf: 0.7, tr: 0.8, wing: 1 } },
      { id: 'skyward', name: 'Skyward R', blurb: 'Wide-body legend with round tail lamps.', cost: 2e7, base: 2000,
        body: { len: 4.6, wid: 1.95, cabLen: 2.1, cabOff: -0.3, cabH: 0.58, tf: 0.6, tr: 0.6, wing: 1 } },
      { id: 'shogun', name: 'Shogun GT', blurb: 'A thousand horsepower and a wing to match.', cost: 6e8, base: 13000,
        body: { len: 4.7, wid: 2.0, cabLen: 2.0, cabOff: -0.4, cabH: 0.54, tf: 0.75, tr: 0.9, wing: 2 } },
    ],
    CAR_COLORS: ['#ff5a36', '#19b3f5', '#ffd23f', '#7ed957', '#b56bff', '#ff6fb5'],

    /* Drivers: stats are rolled evenly inside the rarity's range. */
    RARITIES: [
      { id: 'common', name: 'Common', weight: 60, min: 8, max: 30, color: '#8d99ad' },
      { id: 'rare', name: 'Rare', weight: 28, min: 25, max: 50, color: '#2f9bff' },
      { id: 'epic', name: 'Epic', weight: 10, min: 45, max: 75, color: '#b05cff' },
      { id: 'legendary', name: 'Legendary', weight: 2, min: 70, max: 100, color: '#ffab00' },
    ],
    SCOUT_COST: 60,
    SCOUT_GROWTH: 2.0,
    SKILL_DIV: 25,    // style x (1 + skill / 25)
    FLAIR_DIV: 200,   // style x (1 + flair / 200)
    CONTROL_DIV: 125, // crash rate x (1 - control / 125)
    FIRST_NAMES: ['Kenji', 'Tasha', 'Dmitri', 'Lena', 'Marco', 'Aiko', 'Jax', 'Priya', 'Sven', 'Rosa', 'Tobi', 'Mika',
      'Dara', 'Luca', 'Nia', 'Hiro', 'Zane', 'Ines', 'Otto', 'Yuki', 'Cole', 'Farah', 'Remy', 'Sora'],
    LAST_NAMES: ['Mori', 'Vance', 'Volkov', 'Kowalski', 'Santos', 'Tanaka', 'Reyes', 'Lindqvist', 'Okafor', 'Bianchi',
      'Park', 'Hale', 'Nakamura', 'Duarte', 'Fischer', 'Quinn', 'Abara', 'Leclair', 'Ito', 'Marsh'],
    NICKNAMES: ['Sideways', 'Smokey', 'Clutch-Kick', 'Wall-Tap', 'Backwards', 'Full-Lock', 'Handbrake', 'Limiter',
      'Tandem', 'Zip-Tie', 'Manji', 'Feint'],
    HELMETS: ['#ff5a36', '#19b3f5', '#ffd23f', '#7ed957', '#b56bff', '#ff6fb5', '#ffffff', '#20283a'],

    /* Crashes and mechanics. */
    BASE_CRASH: 1 / 80,   // crashes a second for a car with no control and stock parts
    REPAIR_BASE: 14,      // seconds
    REPAIR_STEP: 0.88,    // x per training level
    REPAIR_MIN: 2,
    REJOIN_OVERHEAD: 4,   // seconds a crash costs besides the repair (spin, mechanic's run, rejoin)
    MECH_MAX: 5,
    MECH_HIRE: 500, MECH_HIRE_GROWTH: 5,
    MECH_TRAIN: 120, MECH_TRAIN_GROWTH: 2.1,
    MECH_NAMES: ['Gus', 'Bea', 'Mo', 'Tilly', 'Rex'],

    /* Venue. Pits = how many cars can run. */
    SLOT_COSTS: [0, 200, 3000, 60000, 1.5e6, 5e7],
    VENUE: [
      { id: 'stands', name: 'Grandstands', blurb: 'More fans can watch. Raises how much style turns into cash.',
        cost: 40, growth: 2.3 },
      { id: 'sponsors', name: 'Sponsors', blurb: 'Banners, an arch and one day a blimp. More cash for every point.',
        cost: 500, growth: 2.7 },
      { id: 'prep', name: 'Track prep', blurb: 'Tire walls, lights and kerbs. Every car scores more style.',
        cost: 300, growth: 2.5 },
    ],
    CROWD_BASE: 12, CROWD_GROWTH: 1.9,   // style points a second the crowd can cheer for
    SPONSOR_PER: 0.25,                   // cash x (1 + 0.25 x level)
    PREP_PER: 0.2,                       // style x (1 + 0.2 x level)

    OFFLINE_MIN: 30,          // seconds away before away-earnings are paid
    OFFLINE_CAP: 2 * 3600,
  };
})(typeof window !== 'undefined' ? window : globalThis);
