/* Drift Tycoon — tuning data. Every number the game uses lives here. */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});

  DT.data = {
    START_CASH: 30,

    /* Parts come in a few big stages, not many small levels. Each stage adds `per` to that part's
       multiplier (parts multiply each other) and costs PART_GROWTH times the last. Parts belong to the
       pit, so they carry over when the car is traded up; their prices do not depend on the car.
       `crash` multiplies the car's crash rate once per stage. */
    PART_GROWTH: 40,
    PART_MAX: 5,
    PART_STAGES: ['Stock', 'Street', 'Sport', 'Race', 'Pro', 'Works'],
    PARTS: [
      { id: 'engine', name: 'Engine', blurb: 'More power down the straights', per: 0.25, cost: 80, crash: 1.0 },
      { id: 'turbo', name: 'Turbo', blurb: 'Big boost, a little more risk', per: 0.3, cost: 320, crash: 1.07 },
      { id: 'tires', name: 'Tires', blurb: 'More smoke and more grip', per: 0.12, cost: 25, crash: 0.9 },
      { id: 'suspension', name: 'Suspension', blurb: 'Keeps the car off the wall', per: 0.08, cost: 45, crash: 0.75 },
      { id: 'stance', name: 'Stance', blurb: 'Low and wide. The crowd loves it', per: 0.15, cost: 130, crash: 1.0 },
      { id: 'angle', name: 'Angle kit', blurb: 'More steering lock, bigger angle', per: 0.2, cost: 200, crash: 1.0 },
    ],

    /* Cars are fictional. `base` is style points a second before parts and driver.
       Each model scores 4x the last and costs 100x the last: prices have to outrun what they
       earn, or the game snowballs. `body` drives the low-poly model. */
    MODELS: [
      { id: 'kumo', name: 'Kumo 180', blurb: 'Light pop-up-lamp coupe. Every team starts here.', cost: 300, base: 2,
        body: { len: 4.1, wid: 1.75, cabLen: 2.0, cabOff: -0.35, cabH: 0.58, tf: 0.55, tr: 0.75, wing: 0 } },
      { id: 'hachi', name: 'Hachi Coupe', blurb: 'Boxy little hatch that punches up.', cost: 30000, base: 8,
        body: { len: 3.9, wid: 1.7, cabLen: 2.1, cabOff: -0.45, cabH: 0.62, tf: 0.5, tr: 0.3, wing: 0 } },
      { id: 'kaze', name: 'Kaze Tourer', blurb: 'Four doors, straight six, endless smoke.', cost: 3e6, base: 32,
        body: { len: 4.6, wid: 1.8, cabLen: 2.4, cabOff: -0.2, cabH: 0.6, tf: 0.55, tr: 0.55, wing: 0 } },
      { id: 'rotor', name: 'Rotor Seven', blurb: 'Screaming rotary with a long nose.', cost: 3e8, base: 128,
        body: { len: 4.3, wid: 1.85, cabLen: 1.8, cabOff: -0.5, cabH: 0.52, tf: 0.7, tr: 0.8, wing: 1 } },
      { id: 'skyward', name: 'Skyward R', blurb: 'Wide-body legend with round tail lamps.', cost: 3e10, base: 512,
        body: { len: 4.6, wid: 1.95, cabLen: 2.1, cabOff: -0.3, cabH: 0.58, tf: 0.6, tr: 0.6, wing: 1 } },
      { id: 'shogun', name: 'Shogun GT', blurb: 'A thousand horsepower and a wing to match.', cost: 3e12, base: 2048,
        body: { len: 4.7, wid: 2.0, cabLen: 2.0, cabOff: -0.4, cabH: 0.54, tf: 0.75, tr: 0.9, wing: 2 } },
    ],
    CAR_COLORS: ['#ff5a36', '#19b3f5', '#ffd23f', '#7ed957', '#b56bff', '#ff6fb5'],

    /* Drivers. Base stats are rolled evenly inside the rarity's range. Seat time earns experience;
       every level adds LEVEL_STAT of the base stats, up to the rarity's level cap.
       `trait` is the chance that a driver of that rarity has a trait. */
    RARITIES: [
      { id: 'common', name: 'Common', weight: 60, min: 8, max: 30, cap: 10, trait: 0.35, color: '#8d99ad' },
      { id: 'rare', name: 'Rare', weight: 28, min: 25, max: 50, cap: 15, trait: 0.6, color: '#2f9bff' },
      { id: 'epic', name: 'Epic', weight: 10, min: 45, max: 75, cap: 20, trait: 1, color: '#b05cff' },
      { id: 'legendary', name: 'Legendary', weight: 2, min: 70, max: 100, cap: 25, trait: 1, color: '#ffab00' },
    ],
    TRAITS: [
      { id: 'showboat', name: 'Showboat', blurb: '+25% style, but crashes 30% more often', style: 1.25, crash: 1.3 },
      { id: 'steady', name: 'Safe Hands', blurb: 'Crashes half as often', crash: 0.5 },
      { id: 'learner', name: 'Quick Learner', blurb: 'Gains experience 60% faster', xp: 1.6 },
      { id: 'fanfav', name: 'Crowd Favourite', blurb: 'The crowd can cheer 10% more style while they have a seat', crowd: 0.1 },
      { id: 'sponsor', name: 'Sponsor Magnet', blurb: '+8% cash while they have a seat', cash: 0.08 },
      { id: 'gearhead', name: 'Gearhead', blurb: 'Their car is repaired 35% faster', repair: 0.65 },
      { id: 'tandem', name: 'Tandem Ace', blurb: '+6% style for every other car with a driver', perCar: 0.06 },
    ],
    SKILL_DIV: 25,     // style x (1 + skill / 25)
    FLAIR_DIV: 200,    // style x (1 + flair / 200)
    CONTROL_HALF: 40,  // crash rate / (1 + control / 40)
    LEVEL_STAT: 0.08,  // each level adds 8% of the base stats
    XP_BASE: 90,       // seconds of drifting for level 2
    XP_GROWTH: 1.25,   // each level needs this much more than the last
    MENTOR_SHARE: 0.5, // a retiring driver passes this share of their experience to their replacement

    /* Tryouts: pay, three prospects show up, sign one or none. One driver per pit plus the reserve. */
    TRYOUT_SIZE: 3,
    TRYOUT_COST: 60, TRYOUT_GROWTH: 1.45,
    TRYOUT_COOLDOWN: 180,   // seconds between tryouts (waived while a car has nobody to drive it)
    RESERVE_SEATS: 1,
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
    REPAIR_STEP: 0.7,     // x per training level
    REPAIR_MIN: 2,
    REJOIN_OVERHEAD: 4,   // seconds a crash costs besides the repair (spin, mechanic's run, rejoin)
    MECH_MAX: 5,
    MECH_HIRE: 500, MECH_HIRE_GROWTH: 5,
    MECH_TRAIN: 200, MECH_TRAIN_GROWTH: 5,
    MECH_NAMES: ['Gus', 'Bea', 'Mo', 'Tilly', 'Rex'],

    /* Venue. Pits = how many cars can run. */
    SLOT_COSTS: [0, 600, 40000, 4e6, 4e8, 4e10],
    VENUE: [
      { id: 'stands', name: 'Grandstands', blurb: 'More fans can watch. Raises how much style turns into cash.',
        cost: 60, growth: 3.6 },
      { id: 'sponsors', name: 'Sponsors', blurb: 'Banners, an arch and one day a blimp. More cash for every point.',
        cost: 600, growth: 6 },
      { id: 'prep', name: 'Track prep', blurb: 'Tire walls, lights and kerbs. Every car scores more style.',
        cost: 400, growth: 6 },
    ],
    CROWD_BASE: 15, CROWD_GROWTH: 3,     // style points a second the crowd can cheer for, x3 per level
    SPONSOR_MULT: 1.25,                  // cash x1.25 per level
    PREP_MULT: 1.2,                      // style x1.2 per level

    OFFLINE_MIN: 30,          // seconds away before away-earnings are paid
    OFFLINE_CAP: 2 * 3600,
  };
})(typeof window !== 'undefined' ? window : globalThis);
