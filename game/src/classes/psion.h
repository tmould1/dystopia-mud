/* Psion / Mindflayer class constants */

/* pcdata->powers[] indices for ability state (0-9) */
#define PSION_THOUGHT_SHIELD     0   /* Thought Shield ticks remaining */
#define PSION_KINETIC_BARRIER    1   /* Kinetic Barrier ticks remaining */
#define PSION_LEVITATE           2   /* Levitate ticks remaining */
#define PSION_MENTAL_LINK        3   /* Mental Link ticks remaining */
#define PSION_MINDSCAN           4   /* Mindscan detection ticks */
#define PSION_LINKED_PLAYER      5   /* Mental Link target ID */
#define MIND_HIVEMIND            6   /* Mindflayer: Hivemind ticks */
#define MIND_THRALL_COUNT        7   /* Mindflayer: Active thralls */

/* pcdata->powers[] training category levels (10+) */
#define PSION_TRAIN_TELEPATHY   10   /* Telepathy level 0-3 */
#define PSION_TRAIN_TELEKINESIS 11   /* Telekinesis level 0-3 */
#define PSION_TRAIN_COMBAT      12   /* Psychic Combat level 0-3 */

/* Mindflayer training category levels (also pcdata->powers[]) */
#define MIND_TRAIN_DOMINATION   10   /* Domination level 0-4 */
#define MIND_TRAIN_CONSUMPTION  11   /* Consumption level 0-3 */
#define MIND_TRAIN_PSIONIC      12   /* Psionic Storm level 0-3 */

/* pcdata->stats[] indices */
#define PSION_THOUGHT_SHIELD_HP  0   /* Current thought shield HP */
#define PSION_KINETIC_BARRIER_HP 1   /* Current kinetic barrier HP */
#define PSION_PEAK_FOCUS         2   /* Highest focus reached this session */
#define MIND_PEAK_FOCUS          0   /* Mindflayer peak focus */
