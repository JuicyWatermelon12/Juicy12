/* Drift Tycoon — tuning data. Every number the game uses lives here. */
(function (root) {
  'use strict';
  const DT = (root.DT = root.DT || {});

  DT.data = {
    /* Prices are set for a team that goes Street Drifting: a decent run pays several times what the
       venue earns over the same time, so a team that only idles moves about five times slower. */
    START_CASH: 150,

    /* Parts come in a few big stages, not many small levels. Each stage adds `per` to that part's
       multiplier (parts multiply each other) and costs PART_GROWTH times the last. Parts belong to the
       pit, so they carry over when the car is traded up; their prices do not depend on the car.
       `crash` multiplies the car's crash rate once per stage. */
    PART_GROWTH: 50,
    PART_MAX: 6,
    PART_STAGES: ['Stock', 'Street', 'Sport', 'Race', 'Pro', 'Works', 'Prototype'],
    PARTS: [
      { id: 'engine', name: 'Engine', blurb: 'More power down the straights', per: 0.25, cost: 200, crash: 1.0 },
      { id: 'turbo', name: 'Turbo', blurb: 'Big boost, a little more risk', per: 0.3, cost: 800, crash: 1.07 },
      { id: 'tires', name: 'Tires', blurb: 'More smoke and more grip', per: 0.12, cost: 60, crash: 0.9 },
      { id: 'suspension', name: 'Suspension', blurb: 'Keeps the car off the wall', per: 0.08, cost: 110, crash: 0.75 },
      { id: 'stance', name: 'Stance', blurb: 'Low and wide. The crowd loves it', per: 0.15, cost: 320, crash: 1.0 },
      { id: 'angle', name: 'Angle kit', blurb: 'More steering lock, bigger angle', per: 0.2, cost: 500, crash: 1.0 },
    ],

    /* Cars are fictional. `base` is style points a second before parts and driver.
       Each model scores 4x the last and costs 100x the last: prices have to outrun what they
       earn, or the game snowballs. `body` drives the low-poly model. */
    MODELS: [
      { id: 'kumo', kitScale: 1, name: 'Kumo 180', blurb: 'Light pop-up-lamp coupe. Every team starts here.', cost: 800, base: 2,
        body: { len: 4.1, wid: 1.75, cabLen: 2.0, cabOff: -0.35, cabH: 0.58, tf: 0.55, tr: 0.75, wing: 0 } },
      { id: 'hachi', kitScale: 6, name: 'Hachi Coupe', blurb: 'Boxy little hatch that punches up.', cost: 60000, base: 8,
        body: { len: 3.9, wid: 1.7, cabLen: 2.1, cabOff: -0.45, cabH: 0.62, tf: 0.5, tr: 0.3, wing: 0 } },
      { id: 'kaze', kitScale: 40, name: 'Kaze Tourer', blurb: 'Four doors, straight six, endless smoke.', cost: 1.5e7, base: 32,
        body: { len: 4.6, wid: 1.8, cabLen: 2.4, cabOff: -0.2, cabH: 0.6, tf: 0.55, tr: 0.55, wing: 0 } },
      { id: 'rotor', kitScale: 250, name: 'Rotor Seven', blurb: 'Screaming rotary with a long nose.', cost: 1.5e9, base: 128,
        body: { len: 4.3, wid: 1.85, cabLen: 1.8, cabOff: -0.5, cabH: 0.52, tf: 0.7, tr: 0.8, wing: 1 } },
      { id: 'skyward', kitScale: 1500, name: 'Skyward R', blurb: 'Wide-body legend with round tail lamps.', cost: 1.5e11, base: 512,
        body: { len: 4.6, wid: 1.95, cabLen: 2.1, cabOff: -0.3, cabH: 0.58, tf: 0.6, tr: 0.6, wing: 1 } },
      { id: 'shogun', kitScale: 10000, name: 'Shogun GT', blurb: 'A thousand horsepower and a wing to match.', cost: 1.5e13, base: 2048,
        body: { len: 4.7, wid: 2.0, cabLen: 2.0, cabOff: -0.4, cabH: 0.54, tf: 0.75, tr: 0.9, wing: 2 } },
    ],

    /* Cosmetics. Bought once with cash at a set price that never changes. The crowd likes a good-looking car:
       `style` is added to the style the car wearing it scores at the venue (paint + rims + kit add up).
       It does nothing to how the car handles or scores on the street.
       `rank` is the street cred rank that puts the item on sale. Paints and rims belong to the team and
       can go on any car; body kits are bought per car model (price x the model's kitScale). */
    PAINTS: [
      { id: 'red', name: 'Factory Red', color: '#ff5a36', cost: 0, rank: 0 },
      { id: 'blue', name: 'Factory Blue', color: '#19b3f5', cost: 0, rank: 0 },
      { id: 'yellow', name: 'Factory Yellow', color: '#ffd23f', cost: 0, rank: 0 },
      { id: 'green', name: 'Factory Green', color: '#7ed957', cost: 0, rank: 0 },
      { id: 'violet', name: 'Factory Violet', color: '#b56bff', cost: 0, rank: 0 },
      { id: 'pink', name: 'Factory Pink', color: '#ff6fb5', cost: 0, rank: 0 },
      { id: 'white', name: 'Pearl White', color: '#f4f6fa', style: 0.02, cost: 2500, rank: 0 },
      { id: 'black', name: 'Gloss Black', color: '#20242e', style: 0.02, cost: 2500, rank: 0 },
      { id: 'gunmetal', name: 'Gunmetal', color: '#6b7386', style: 0.03, cost: 8000, rank: 1 },
      { id: 'forest', name: 'Forest Green', color: '#1f6b4a', style: 0.03, cost: 8000, rank: 1 },
      { id: 'midnight', name: 'Midnight Blue', color: '#1c2f8f', style: 0.04, cost: 25000, rank: 2 },
      { id: 'burnt', name: 'Burnt Orange', color: '#d9651a', style: 0.05, cost: 60000, rank: 3 },
      { id: 'grape', name: 'Midnight Purple', color: '#4b2a8c', style: 0.06, cost: 150000, rank: 4 },
      { id: 'teal', name: 'Lagoon Teal', color: '#12a59a', style: 0.08, cost: 400000, rank: 5 },
      { id: 'gold', name: 'Champagne Gold', color: '#d8b45a', style: 0.1, cost: 1.5e6, rank: 6 },
      { id: 'magenta', name: 'Hot Magenta', color: '#e0177f', style: 0.12, cost: 5e6, rank: 7 },
      { id: 'silver', name: 'Liquid Silver', color: '#c9d2e0', style: 0.15, cost: 2e7, rank: 8 },
      { id: 'blood', name: 'Blood Red', color: '#a30f1f', style: 0.18, cost: 8e7, rank: 9 },
    ],
    FACTORY_PAINTS: ['red', 'blue', 'yellow', 'green', 'violet', 'pink'],   // free; new cars arrive in one of these
    /* Rims: `spokes` 0 is a plain disc; `r` is the rim's radius in metres (the tire is 0.4). */
    RIMS: [
      { id: 'steel', name: 'Steelies', color: '#dfe5ee', spokes: 0, r: 0.22, cost: 0, rank: 0 },
      { id: 'five', name: 'Silver Five-Spoke', color: '#e6ebf2', spokes: 5, r: 0.3, style: 0.04, cost: 15000, rank: 1 },
      { id: 'six', name: 'White Six-Spoke', color: '#ffffff', spokes: 6, r: 0.3, style: 0.06, cost: 40000, rank: 2 },
      { id: 'bronze', name: 'Bronze Mesh', color: '#b07a3c', spokes: 10, r: 0.3, style: 0.08, cost: 120000, rank: 3 },
      { id: 'dish', name: 'Black Deep Dish', color: '#2a2f3b', spokes: 0, r: 0.31, style: 0.1, cost: 300000, rank: 4 },
      { id: 'goldfive', name: 'Gold Five-Spoke', color: '#e8b730', spokes: 5, r: 0.31, style: 0.14, cost: 2e6, rank: 6 },
      { id: 'neon', name: 'Neon Six-Spoke', color: '#2ff5e0', spokes: 6, r: 0.31, style: 0.2, cost: 2.5e7, rank: 8 },
    ],
    KITS: [
      { id: 'stock', name: 'Stock body', blurb: 'As it left the factory.', cost: 0, rank: 0 },
      { id: 'street', name: 'Street kit', blurb: 'Front lip, side skirts and a ducktail.', style: 0.08, cost: 30000, rank: 2 },
      { id: 'drift', name: 'Drift kit', blurb: 'Wide arches, a front-mount intercooler and a GT wing.', style: 0.16, cost: 250000, rank: 5 },
      { id: 'pro', name: 'Pro kit', blurb: 'Full wide-body: canards, roof scoop, diffuser and a big wing.', style: 0.3, cost: 5e6, rank: 8 },
    ],

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
    TRYOUT_COST: 150, TRYOUT_GROWTH: 1.45,
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
    MECH_HIRE: 2500, MECH_HIRE_GROWTH: 5,
    MECH_TRAIN: 1000, MECH_TRAIN_GROWTH: 5,
    MECH_NAMES: ['Gus', 'Bea', 'Mo', 'Tilly', 'Rex'],

    /* Venue. Pits = how many cars can run. */
    SLOT_COSTS: [0, 1200, 80000, 2e7, 2e9, 2e11],
    VENUE: [
      { id: 'stands', name: 'Grandstands', blurb: 'More fans can watch. Raises how much style turns into cash.',
        cost: 300, growth: 3.6 },
      { id: 'sponsors', name: 'Sponsors', blurb: 'Banners, an arch and one day a blimp. More cash for every point.',
        cost: 3000, growth: 6 },
      { id: 'prep', name: 'Track prep', blurb: 'Tire walls, lights and kerbs. Every car scores more style.',
        cost: 2000, growth: 6 },
    ],
    VENUE_MAX: { stands: 5, sponsors: 6, prep: 5 },   // top level of each upgrade, at every venue
    CROWD_GROWTH: 3,                     // the crowd can cheer x3 more style per grandstand level

    /* The venue ladder. Every venue the team buys is kept, with its own grandstands, sponsors and track prep.
       The team runs at one venue at a time and can move between the ones it owns. What is on sale (pits, car
       models, part stages) follows the best venue owned; style, crowd and upgrade prices follow the venue
       the team is at. Each owned venue's track can also be driven in Drift mode.
         cost    price of the venue: about 25 minutes of Street Drifting at the venue before, fully built
         pits    most cars it can run          cars    car models it puts on sale
         style   multiplier on all style there  crowd   style a second the crowd can cheer with no grandstands
         scale   multiplier on the price of its grandstands, sponsors and track prep
         stage   highest part stage on sale there (an index into PART_STAGES), so parts last the whole ladder
       `track` (the drift line) and `theme` are for the 3D view. */
    /* The venue ladder. `bar` is the average Drift run (km) a player needs to keep the intended pace there: it
       rises with every venue, because the player is expected to get better. `cash` is the venue's cash per style
       next to the others; it sets that pace (drift pay is tied to income, so it scales both). The first three
       venues are the hook: they come quickly and ask for short runs. */
    VENUES: [
      { id: 'lot', name: 'Parking Lot', blurb: 'An empty car park behind the team shed. Everyone starts somewhere.',
        cost: 0, pits: 3, cars: ['kumo', 'hachi'], style: 1, crowd: 20, scale: 0.4, stage: 2, bar: 2, cash: 1.7 },
      { id: 'docks', name: 'Docks', blurb: 'Container stacks, cranes and a quay that is closed at night.',
        cost: 4e7, pits: 4, cars: ['kaze'], style: 4, crowd: 3000, scale: 150, stage: 3, bar: 3, cash: 1 },
      { id: 'airfield', name: 'Airfield', blurb: 'A disused runway with room to go properly sideways.',
        cost: 6e9, pits: 5, cars: ['rotor'], style: 16, crowd: 1.2e5, scale: 10000, stage: 4, bar: 4, cash: 0.7 },
      { id: 'pro', name: 'Pro Circuit', blurb: 'A real race circuit, hired by the day: pit garages, kerbs and a proper grandstand.',
        cost: 7e11, pits: 6, cars: ['skyward'], style: 64, crowd: 5e6, scale: 7e5, stage: 5, bar: 5, cash: 0.7 },
      { id: 'stadium', name: 'Stadium', blurb: 'A football stadium turned arena for the night: floodlights and sixty thousand seats.',
        cost: 6e13, pits: 6, cars: ['shogun'], style: 256, crowd: 2e8, scale: 5e7, stage: 6, bar: 6, cash: 0.45 },
    ],
    VENUE_CASH: 2,                       // cash for every point of style the crowd cheers, before sponsors
    SPONSOR_MULT: 1.25,                  // cash x1.25 per level
    PREP_MULT: 1.2,                      // style x1.2 per level

    /* Street Drifting: the drive-it-yourself run down an endless mountain road. The code calls it "touge".
       Distances in metres, angles in radians. Where a value has two numbers, the road moves from the first
       to the second over the first RAMP_KM kilometres. */
    TOUGE: {
      STEP: 1, HALF_WIDTH: 6.2, CAR_HALF: 1.1, SLOPE: 0.11,
      START_STRAIGHT: 60,
      START_OFFSET: 18,                       // the car starts this far down the road, so there is road behind it
      LAP_LINE: 34,                           // a track's start line is this far ahead of the car's starting spot
      // The street gets harder in two stretches: from the start to RAMP_KM, then on to HARD_KM, where it stops.
      // Each pair below is [at the start, at RAMP_KM, at HARD_KM].
      RAMP_KM: 6, HARD_KM: 10,
      TURN_MIN: [0.6, 0.95, 1.05], TURN_MAX: [1.3, 1.4, 1.45],   // how far a bend swings the road away from straight downhill
      RADIUS_MIN: [23, 18, 16], RADIUS_MAX: [43, 28, 23],
      STRAIGHT_MIN: [10, 5, 4], STRAIGHT_MAX: [35, 13, 10],
      SPEED_PER_KM: 0.05,                     // top speed grows 5% a kilometre, up to HARD_KM (+50%)
      // A venue track keeps its layout, so it is made harder with speed instead: the car's top speed is
      // multiplied by TRACK_SPEED for that track, and grows TRACK_SPEED_PER_KM a kilometre up to HARD_KM.
      TRACK_SPEED: { lot: 1, docks: 1, airfield: 1.15, pro: 0.96, stadium: 1.03 }, TRACK_SPEED_PER_KM: 0.07,
      // Part stages past HANDLING_STAGES change the car's handling here by only HANDLING_EXTRA of a stage each,
      // so the latest stages keep cars drivable (they still add their full style at the venue and score bonus).
      HANDLING_STAGES: 5, HANDLING_EXTRA: 0.4,
      MAX_HITS: 1,                            // barrier touches that end the run
      COUNTDOWN: 2.4, START_SPEED: 13,
      TOP_SPEED: 24, TOP_PER_STAGE: 0.4,       // m/s; engine and turbo stages each add TOP_PER_STAGE
      ACCEL: 1.2,
      LOCK: 0.105, LOCK_PER_ANGLE: 0.005,     // tightest turn at full lock, as 1 / radius in metres: the nose turns faster the faster the car goes, so quick cars can still make the bends
      GRIP: 2.3,                              // how fast the car's direction of travel catches up with its nose
      MAX_SLIP: 0.72, SLIP_PER_ANGLE: 0.1,    // biggest drift angle; the angle kit adds to it
      SLIDE_DRAG: 0.3,                        // share of top speed lost at a full-angle slide
      HIT_KEEP: 0.5, HIT_KEEP_PER_SUSP: 0.06, // share of speed kept after touching a barrier
      BOUNCE: 0.35,
      DRIFT_MIN: 0.17, MIN_SPEED: 7,          // a slide counts as a drift past this angle and speed
      SCORE_RATE: 12,                         // points = speed x angle x this x multiplier, per second
      COMBO_TIME: 3,                          // the multiplier gains 1 for every COMBO_TIME seconds of unbroken drifting
      COMBO_MAX: 5, COMBO_MAX_PER_KM: 1,      // its cap, and how much the cap rises each kilometre
      COMBO_GRACE: 0.8,                       // seconds straight before the multiplier resets
      BONUS_PER_STANCE: 0.06, BONUS_PER_TIRES: 0.04, SKILL_DIV: 400, FLAIR_DIV: 600,
      // Pay: the points scored in each kilometre pay at that kilometre's rate. At pay x1, POINTS_PER_SECOND
      // points pay one second of the team's income. The rate rises every kilometre and holds from the last entry on,
      // so short runs pay little and the money is far down the road: against a flat x5.5 for every kilometre
      // (what runs paid before), a run that ends at 1 km pays about 15%, 2 km half, 3 km the same, 5 km twice
      // and 8 km three times as much. Tuned with tools/sim.js and the test drivers.
      POINTS_PER_SECOND: 330,
      PAY_PER_KM: [1, 5, 11, 15, 21, 23, 25, 27, 29, 31],
      XP_MULT: 2,                             // the driver's seat time counts double
    },

    /* Drift mode, Street: roads to drift down. One for now; more maps go here. */
    STREET_MAPS: [
      { id: 'pass', name: 'Mountain Pass', blurb: 'An endless mountain road at dusk. It gets tighter and faster the further you go.' },
    ],
    TRACK_SCALE: 5.5,   // Drift mode, Track: a venue's drift line is this many times bigger when you drive it

    /* Street cred: a rank earned only by Street Drifting. It never goes down and is never spent. */
    CRED_PER_100M: 1, CRED_PER_1000_POINTS: 1, CRED_NEW_BEST: 10,   // a new best score and a new best distance each pay CRED_NEW_BEST
    RANKS: [
      { name: 'Unknown', cred: 0 }, { name: 'Newcomer', cred: 40 }, { name: 'Local', cred: 100 }, { name: 'Regular', cred: 200 },
      { name: 'Known', cred: 350 }, { name: 'Respected', cred: 550 }, { name: 'Notorious', cred: 800 }, { name: 'Street Hero', cred: 1100 },
      { name: 'Mountain Ace', cred: 1500 }, { name: 'Street King', cred: 2000 }, { name: 'Legend', cred: 2700 }, { name: 'Myth', cred: 3600 },
    ],

    /* Tap bubbles: they float up over the venue while you are watching it and vanish if ignored.
         cash   pays this many seconds of the team's long-run income at once
         crowd  multiplies how much style the crowd can cheer, for `time` seconds
         boost  multiplies the venue's cash, for `time` seconds
         train  the cars bunch up into a tandem train while it lasts */
    BUBBLE_GAP: [30, 60],     // seconds of watching between bubbles
    BUBBLE_LIFE: 10,          // seconds before an ignored bubble goes
    BUBBLES: [
      { id: 'perfect', name: 'Perfect drift', weight: 62, cash: 20 },
      { id: 'wave', name: 'Crowd wave', weight: 30, crowd: 2, boost: 1.25, time: 30 },
      { id: 'meet', name: 'Drift meet', weight: 8, crowd: 2, boost: 2, time: 60, train: true },
    ],

    OFFLINE_MIN: 30,          // seconds away before away-earnings are paid
    OFFLINE_CAP: 4 * 3600,
  };
})(typeof window !== 'undefined' ? window : globalThis);
