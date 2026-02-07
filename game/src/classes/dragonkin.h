/* Dragonkin / Wyrm class constants */

/* pcdata->powers[] indices - Ability state (0-9) */
#define DRAGON_ATTUNEMENT        0   /* Current elemental attunement (0=Fire,1=Frost,2=Storm,3=Earth) */
#define DRAGON_SCALESHIELD       1   /* Scaleshield ticks remaining */
#define DRAGON_DRAGONHIDE        2   /* Dragonhide toggle (1=active) */
#define DRAGON_PRIMALWARDING     3   /* Primalwarding ticks remaining */
#define DRAGON_DRAKEWINGS        4   /* Drakewings ticks remaining */
#define DRAGON_DOT_TICKS         5   /* Searing blast DoT ticks remaining */
#define DRAGON_DOT_STACKS        6   /* Searing blast DoT stacks (1-5) */
#define DRAGON_WYRMFORM          7   /* Wyrm: wyrmform ticks remaining */
#define DRAGON_ANCIENTWRATH      8   /* Wyrm: ancientwrath ticks remaining */
#define DRAGON_TERRAIN_TYPE      9   /* Wyrm: current terrain shift type */

/* pcdata->powers[] training category levels (10+) */
#define DRAGON_TRAIN_BREATH      10  /* Draconic Breath level (0-3) */
#define DRAGON_TRAIN_SCALES      11  /* Dragon Scales level (0-3) */
#define DRAGON_TRAIN_MIGHT       12  /* Dragon Might level (0-3) */
#define DRAGON_TRAIN_ESSENCE     13  /* Essence Mastery level (0-2) */

/* Wyrm training category levels (also pcdata->powers[], reuses indices) */
#define WYRM_TRAIN_DEVASTATION   10  /* Devastation level (0-4) */
#define WYRM_TRAIN_DOMINION      11  /* Dominion level (0-3) */
#define WYRM_TRAIN_ASCENSION     12  /* Ascension level (0-3) */

/* Elemental attunement constants */
#define ATTUNE_FIRE              0
#define ATTUNE_FROST             1
#define ATTUNE_STORM             2
#define ATTUNE_EARTH             3

/* pcdata->stats[] indices */
#define DRAGON_SCALESHIELD_HP    0   /* Current scaleshield HP remaining */
#define DRAGON_ESSENCE_PEAK      1   /* Peak Essence reached this session */

/* Resource limits */
#define DRAGONKIN_ESSENCE_MAX    100
#define WYRM_ESSENCE_MAX         150
