/***************************************************************************
 *  db_game.h - SQLite persistence for global game data
 *
 *  Manages base_help.db + live_help.db (help entries) and game.db
 *  (config, leaderboards, kingdoms, notes, bugs, bans, disabled commands)
 *  stored in gamedata/db/game/.
 *
 *  base_help.db is deployed read-only with each release.
 *  live_help.db holds runtime additions/overrides (preserved on server).
 ***************************************************************************/

#ifndef DB_GAME_H
#define DB_GAME_H

#include "../core/merc.h"

/* Initialize game databases (creates game/ subdir, opens connections) */
void db_game_init( void );

/* Close game database connections after boot (data cached in memory) */
void db_game_close_boot_connections( void );

/* Help entries (base_help.db + live_help.db) */
void db_game_load_helps( void );
void db_game_save_helps( void );
bool db_game_reload_help( const char *keyword );

/* Game config (game.db) */
void db_game_load_gameconfig( void );
void db_game_save_gameconfig( void );

/* Topboard (game.db) */
void db_game_load_topboard( void );
void db_game_save_topboard( void );

/* Leaderboard (game.db) */
void db_game_load_leaderboard( void );
void db_game_save_leaderboard( void );

/* Kingdoms (game.db) */
void db_game_load_kingdoms( void );
void db_game_save_kingdoms( void );

/* Board notes (game.db) */
void db_game_load_notes( int board_idx );
void db_game_save_board_notes( int board_idx );
void db_game_append_note( int board_idx, NOTE_DATA *note );

/* Bug reports (game.db) */
void db_game_append_bug( int room_vnum, const char *player, const char *message );

/* Bans (game.db) */
void db_game_load_bans( void );
void db_game_save_bans( void );

/* Disabled commands (game.db) */
void db_game_load_disabled( void );
void db_game_save_disabled( void );

/* Balance config (game.db) */
void db_game_load_balance( void );
void db_game_save_balance( void );

/* Ability config (game.db) */
void db_game_load_ability_config( void );
void db_game_save_ability_config( void );

/* Super admins (game.db) - character names with relevel access */
bool db_game_is_super_admin( const char *name );
void db_game_add_super_admin( const char *name );
void db_game_remove_super_admin( const char *name );
void db_game_list_super_admins( CHAR_DATA *ch );

/* Immortal pretitles (game.db) - custom who list pretitles */
void db_game_load_pretitles( void );
const char *db_game_get_pretitle( const char *name );
void db_game_set_pretitle( const char *name, const char *pretitle, const char *set_by );
void db_game_delete_pretitle( const char *name );
void db_game_list_pretitles( CHAR_DATA *ch );

/* Class display config (game.db) - brackets and generation names for who list */
typedef struct class_bracket {
	int    class_id;       /* CLASS_* constant (1, 2, 4, 8, ...) */
	char   *class_name;    /* Human-readable name ("Vampire", "Werewolf") */
	char   *open_bracket;  /* Opening bracket with colors, e.g., "#R<<#n" */
	char   *close_bracket; /* Closing bracket with colors, e.g., "#R>>#n" */
	char   *accent_color;  /* Summary: bracket/decorative color, e.g., "#x136" */
	char   *primary_color; /* Summary: title/name color, e.g., "#x178" */
} CLASS_BRACKET;

typedef struct class_generation {
	int    class_id;       /* CLASS_* constant */
	int    generation;     /* 1-8 for specific gen, 0 for default/fallback */
	char   *title;         /* Generation title with colors, e.g., "#RI#0nner #RC#0ircle#n" */
} CLASS_GENERATION;

typedef struct class_aura {
	int    class_id;       /* CLASS_* constant */
	char   *aura_text;     /* Display text with colors, e.g., "#y(#LWerewolf#y)#n " */
	char   *mxp_tooltip;   /* MXP tooltip text, e.g., "Werewolf" */
	int    display_order;  /* Order for multiple class display (lower = first) */
} CLASS_AURA;

typedef struct class_armor_config {
	int    class_id;         /* CLASS_* constant */
	char   *acfg_cost_key;   /* ability_config key for primal cost, e.g., "vampire.vampirearmor.practice_cost" */
	char   *usage_message;   /* Message showing available pieces */
	char   *act_to_char;     /* Message to player when creating, e.g., "$p appears in your hands." */
	char   *act_to_room;     /* Message to room when creating, e.g., "$p appears in $n's hands." */
} CLASS_ARMOR_CONFIG;

typedef struct class_armor_piece {
	int    class_id;         /* CLASS_* constant */
	char   *keyword;         /* Piece name, e.g., "ring", "plate" */
	int    vnum;             /* Object vnum */
} CLASS_ARMOR_PIECE;

typedef struct class_starting {
	int    class_id;         /* CLASS_* constant */
	int    starting_beast;   /* Beast value on class selection (default 15, vampire 30) */
	int    starting_level;   /* Level set on class selection (default 1, monk/mage 3) */
	bool   has_disciplines;  /* Whether to call set_learnable_disciplines() */
} CLASS_STARTING;

/* Class registry (game.db) - centralized class metadata for mudstat/selfclass */
typedef struct class_registry_entry {
	int    class_id;         /* CLASS_* constant */
	char   *class_name;      /* "Demon", "Vampire" */
	char   *keyword;         /* "demon", "vampire" (for selfclass) */
	char   *keyword_alt;     /* alternate keyword or NULL ("battlemage" for mage) */
	char   *mudstat_label;   /* "Demons", "Vampires" (plural for mudstat) */
	char   *selfclass_message; /* confirmation message with colors */
	int    display_order;    /* mudstat display ordering */
	int    upgrade_class;    /* 0=base class, else class_id this upgrades FROM */
	char   *requirements;    /* optional prereq description */
} CLASS_REGISTRY_ENTRY;

/* Class vnum ranges (game.db) - equipment vnum tracking to prevent conflicts */
typedef struct class_vnum_range {
	int    class_id;         /* CLASS_* constant */
	int    armor_vnum_start; /* First armor vnum (or 0 if none) */
	int    armor_vnum_end;   /* Last armor vnum (or 0 if none) */
	int    mastery_vnum;     /* Mastery item vnum (or 0 if none) */
	char   *description;     /* Optional description */
} CLASS_VNUM_RANGE;

/* Helper macro for base class check */
#define IS_BASE_CLASS(reg) ((reg)->upgrade_class == 0)

/* Audio config (game.db) - MCMP audio file mappings */
typedef struct audio_entry {
	char   *category;      /* "ambient", "footstep", "combat", etc. */
	char   *trigger_key;   /* "SECT_FOREST", "combat_miss", etc. */
	char   *filename;      /* "ambient/forest.mp3" */
	int    volume;         /* 1-100, default 50 */
	int    priority;       /* 1-100, default 50 */
	int    loops;          /* 1 = once, -1 = infinite */
	char   *media_type;    /* "sound" or "music" */
	char   *tag;           /* "combat", "environment", etc. */
	char   *caption;       /* accessibility caption */
	char   *use_key;       /* binding key (e.g., "ambient", "weather") */
	bool   use_continue;   /* TRUE = don't restart if same file playing */
} AUDIO_ENTRY;

void db_game_load_audio_config( void );
AUDIO_ENTRY *audio_config_find( const char *category, const char *trigger_key );

/* Sector-indexed audio lookup arrays (for fast ambient/footstep access) */
extern AUDIO_ENTRY *audio_ambient[SECT_MAX];
extern AUDIO_ENTRY *audio_footstep[SECT_MAX];

/* Class display config (game.db) - brackets and generation names for who list */
void db_game_load_class_display( void );
const CLASS_BRACKET *db_game_get_bracket( int class_id );
const char *db_game_get_generation_title( int class_id, int generation );

/* Class aura config (game.db) - room aura text for class display */
void db_game_load_class_aura( void );
const CLASS_AURA *db_game_get_aura( int class_id );
int db_game_get_aura_count( void );
const CLASS_AURA *db_game_get_aura_by_index( int index );

/* Class armor config (game.db) - armor creation data */
void db_game_load_class_armor( void );
const CLASS_ARMOR_CONFIG *db_game_get_armor_config( int class_id );
int db_game_get_armor_vnum( int class_id, const char *keyword );

/* Class starting config (game.db) - starting values for class selection */
void db_game_load_class_starting( void );
const CLASS_STARTING *db_game_get_starting( int class_id );

/* Class registry (game.db) - centralized class metadata */
void db_game_load_class_registry( void );
int db_game_get_registry_count( void );
const CLASS_REGISTRY_ENTRY *db_game_get_registry_by_id( int class_id );
const CLASS_REGISTRY_ENTRY *db_game_get_registry_by_keyword( const char *keyword );
const CLASS_REGISTRY_ENTRY *db_game_get_registry_by_index( int index );

/* Class vnum ranges (game.db) - equipment vnum tracking */
void db_game_load_class_vnum_ranges( void );
int db_game_get_vnum_range_count( void );
const CLASS_VNUM_RANGE *db_game_get_vnum_range_by_id( int class_id );
const CLASS_VNUM_RANGE *db_game_get_vnum_range_by_index( int index );
bool db_game_check_vnum_overlap( int start, int end, int exclude_class_id );
int db_game_get_next_available_vnum( int range_size );

/* Class score stats (game.db) - customizable score display per class */
typedef enum {
	STAT_NONE = 0,          /* Invalid/placeholder */
	STAT_BEAST,             /* ch->beast */
	STAT_RAGE,              /* ch->rage */
	STAT_CHI_CURRENT,       /* ch->chi[CURRENT] */
	STAT_CHI_MAXIMUM,       /* ch->chi[MAXIMUM] */
	STAT_GNOSIS_CURRENT,    /* ch->gnosis[GCURRENT] */
	STAT_GNOSIS_MAXIMUM,    /* ch->gnosis[GMAXIMUM] */
	STAT_MONKBLOCK,         /* ch->monkblock */
	STAT_SILTOL,            /* ch->siltol */
	STAT_SOULS,             /* ch->pcdata->souls */
	STAT_DEMON_POWER,       /* ch->pcdata->stats[DEMON_CURRENT] */
	STAT_DEMON_TOTAL,       /* ch->pcdata->stats[DEMON_TOTAL] */
	STAT_DROID_POWER,       /* ch->pcdata->stats[DROID_POWER] */
	STAT_DROW_POWER,        /* ch->pcdata->stats[DROW_POWER] */
	STAT_DROW_MAGIC,        /* ch->pcdata->stats[DROW_MAGIC] */
	STAT_TPOINTS,           /* ch->pcdata->stats[TPOINTS] */
	STAT_ANGEL_JUSTICE,     /* ch->pcdata->powers[ANGEL_JUSTICE] */
	STAT_ANGEL_LOVE,        /* ch->pcdata->powers[ANGEL_LOVE] */
	STAT_ANGEL_HARMONY,     /* ch->pcdata->powers[ANGEL_HARMONY] */
	STAT_ANGEL_PEACE,       /* ch->pcdata->powers[ANGEL_PEACE] */
	STAT_SHAPE_COUNTER,     /* ch->pcdata->powers[SHAPE_COUNTER] */
	STAT_PHASE_COUNTER,     /* ch->pcdata->powers[PHASE_COUNTER] */
	STAT_HARA_KIRI,         /* ch->pcdata->powers[HARA_KIRI] */
	STAT_MAX
} STAT_SOURCE;

typedef struct class_score_stat {
	int    class_id;         /* CLASS_* constant */
	int    stat_source;      /* STAT_SOURCE enum value */
	int    stat_source_max;  /* Optional max value source (for current/max display), 0 if none */
	char   *stat_label;      /* Display label, e.g., "Your current beast is" */
	char   *format_string;   /* Full format string with %d placeholder, e.g., "#R[#n%s: #C%d#R]\n\r" */
	int    display_order;    /* Order within class (lower = first) */
} CLASS_SCORE_STAT;

void db_game_load_class_score( void );
int db_game_get_score_stat_count( int class_id );
const CLASS_SCORE_STAT *db_game_get_score_stats( int class_id );
int get_stat_value( CHAR_DATA *ch, int stat_source );

#endif /* DB_GAME_H */
