/* Dirgesinger / Siren class constants */

/* pcdata->powers[] indices */
#define DIRGE_WARSONG_ACTIVE      0   /* 1 if warsong toggle is on */
#define DIRGE_BATTLEHYMN_ACTIVE   1   /* ticks remaining on battlehymn */
#define DIRGE_IRONSONG_ACTIVE     2   /* ticks remaining on ironsong */
#define DIRGE_CADENCE_ACTIVE      3   /* ticks remaining on cadence */
#define DIRGE_DIRGE_TICKS         4   /* DOT: ticks remaining */
#define DIRGE_DIRGE_STACKS        5   /* DOT: progressive stacks (1-5) */
#define DIRGE_ECHOSHIELD_ACTIVE   6   /* Siren: echoshield ticks */
#define DIRGE_CRESCENDO_STAGE     7   /* Siren: crescendo build stage 0-3 */
#define DIRGE_DISSONANCE_TICKS    8   /* ticks remaining on dissonance */

/* pcdata->powers[] training category levels */
#define DIRGE_TRAIN_WARCHANTS    10  /* War Chants path level (0-3) */
#define DIRGE_TRAIN_BATTLESONGS  11  /* Battle Songs path level (0-3) */
#define DIRGE_TRAIN_DIRGES       12  /* Dirges path level (0-2) */
#define DIRGE_TRAIN_IRONVOICE    13  /* Iron Voice path level (0-2) */

/* Siren training category levels (also pcdata->powers[]) */
#define SIREN_TRAIN_DEVASTATION  10  /* Devastation path level (0-3) */
#define SIREN_TRAIN_DOMINATION   11  /* Domination path level (0-4) */
#define SIREN_TRAIN_UNMAKING     12  /* Unmaking path level (0-3) */

/* pcdata->stats[] indices */
#define DIRGE_ARMOR_BONUS         0   /* current sonic barrier HP remaining */
#define DIRGE_RESONANCE_PEAK      1   /* highest resonance reached this session */
