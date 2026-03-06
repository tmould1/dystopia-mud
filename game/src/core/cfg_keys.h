/***************************************************************************
 *  cfg_keys.h - Unified configuration keys (auto-generated)
 *
 *  This file is the SINGLE SOURCE OF TRUTH for all game config entries.
 *  The enum and table are both generated from the CFG_ENTRIES macro.
 *
 *  Categories:
 *    CFG_COMBAT_*      - Global combat parameters
 *    CFG_PROGRESSION_* - Upgrade/generation bonuses
 *    CFG_WORLD_*       - Time, weather, world settings
 *    CFG_ABILITY_*     - Per-class ability parameters
 *
 *  DO NOT EDIT MANUALLY - regenerate using:
 *      python game/tools/generate_cfg_keys.py
 ***************************************************************************/

#ifndef CFG_KEYS_H
#define CFG_KEYS_H

/*
 * X-Macro definitions for config keys.
 * Format: CFG_X(ENUM_NAME, "string.key", default_value)
 */
#define CFG_ENTRIES


/* Generate enum from X-macro */
typedef enum {
#define CFG_X(name, key, def) CFG_##name,
    CFG_ENTRIES
#undef CFG_X
    CFG_COUNT  /* Total count, also serves as invalid sentinel */
} cfg_key_t;

#endif /* CFG_KEYS_H */
