/***************************************************************************
 *  class_display.h - Class display data (symbols, titles, look names)
 *
 *  Stores per-class display information:
 *  - Who list bracket symbols (left/right)
 *  - Generation titles (ranks 1-7 + default)
 *  - Look command display names
 *
 *  Data is stored in game.db and cached in memory at boot.
 *  Admins can edit via the 'classedit' command.
 ***************************************************************************/

#ifndef CLASS_DISPLAY_H
#define CLASS_DISPLAY_H

/* Maximum number of classes supported */
#define MAX_CLASS               16

/* Maximum generations per class (1-7 plus default at index 0) */
#define MAX_GENERATIONS         8

/* String buffer sizes for display strings */
#define CLASS_SYMBOL_LEN        32
#define CLASS_TITLE_LEN         64
#define CLASS_LOOK_LEN          64
#define CLASS_NAME_LEN          32

/*
 * Class display data - cached in memory, persisted to game.db
 */
typedef struct class_display_data {
    int         class_id;                           /* CLASS_* constant value */
    char        name[CLASS_NAME_LEN];               /* Class name (e.g., "Vampire") */
    char        left_symbol[CLASS_SYMBOL_LEN];      /* Who list left bracket */
    char        right_symbol[CLASS_SYMBOL_LEN];     /* Who list right bracket */
    char        look_display[CLASS_LOOK_LEN];       /* Look command class name */
    char        title_color[16];                    /* Color code for generation titles */
    char        titles[MAX_GENERATIONS][CLASS_TITLE_LEN];  /* [0]=default, [1-7]=gen titles */
} CLASS_DISPLAY_DATA;

/*
 * Master table of all class display data.
 * Indexed 0 to MAX_CLASS-1, with class_id used for lookup.
 */
extern CLASS_DISPLAY_DATA class_display_table[MAX_CLASS];
extern int                class_display_count;

/*
 * Lookup functions - return cached data or defaults
 */

/* Get display data for a class by CLASS_* id. Returns NULL if not found. */
CLASS_DISPLAY_DATA *class_display_get( int class_id );

/* Get generation title for a class. Returns default if gen out of range. */
const char *class_get_title( int class_id, int generation );

/* Get who list symbols. Returns empty strings if class not found. */
void class_get_symbols( int class_id, const char **left, const char **right );

/* Get look display string. Returns empty string if not found. */
const char *class_get_look_display( int class_id );

/* Get title color code for a class. Returns "#n" if not found. */
const char *class_get_title_color( int class_id );

/*
 * Database functions
 */

/* Initialize class display table with defaults, then load DB overrides */
void class_display_init( void );

/* Load class display data from game.db (called by db_game) */
void db_class_display_load( void );

/* Save all class display data to game.db */
void db_class_display_save( void );

/* Save a single class's display data to game.db */
void db_class_display_save_one( int class_id );

/*
 * Validation
 */

/* Validate all CLASS_* constants have display entries. Logs warnings. */
void class_display_validate( void );

/*
 * Admin command
 */

/* classedit command - edit class display data (level 12) */
void do_classedit( CHAR_DATA *ch, char *argument );

#endif /* CLASS_DISPLAY_H */
