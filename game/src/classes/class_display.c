/***************************************************************************
 *  class_display.c - Class display data (symbols, titles, look names)
 *
 *  Stores per-class display information in game.db with in-memory caching.
 ***************************************************************************/

/* Disable format-truncation warning - truncation is intentional for admin input */
#pragma GCC diagnostic ignored "-Wformat-truncation"

#include "../core/merc.h"
#include "class.h"
#include "class_display.h"

#include <string.h>
#include <stdio.h>

/* Global class display table */
CLASS_DISPLAY_DATA class_display_table[MAX_CLASS];
int                class_display_count = 0;

/*
 * Default seed data - matches original hardcoded values in act_info.c
 */
typedef struct class_seed {
    int         class_id;
    const char *name;
    const char *left_symbol;
    const char *right_symbol;
    const char *look_display;
    const char *title_color;
    const char *titles[MAX_GENERATIONS];  /* [0]=default, [1-7]=gen 1-7 */
} CLASS_SEED;

static const CLASS_SEED class_seeds[] = {
    {
        CLASS_DEMON, "Demon",
        "#0[#n", "#0]#n",
        "#0(#RDemon#0)#n ",
        "#R",
        { "Imp", "Pit Lord", "Pit Fiend", "GateKeeper", "Imp Lord", "Hell Spawn", "Demon", "Spawnling" }
    },
    {
        CLASS_MAGE, "Mage",
        "{{", "}}",
        "{{#CBattlemage#n}} ",
        "#C",
        { "Magician", "High Archmage", "Archmage", "Lord of Spells", "High Invoker", "Wizard of War", "Battlemage", "Apprentice" }
    },
    {
        CLASS_WEREWOLF, "Werewolf",
        "#y((#n", "#y))#n",
        "#y(#LWerewolf#y)#n ",
        "#L",
        { "Halfbreed Bastard", "Chieftain", "Tribe Shaman", "Pack Leader", "Fenris Wolf", "Stalker", "Hunter", "Pup of Gaia" }
    },
    {
        CLASS_VAMPIRE, "Vampire",
        "#R<<#n", "#R>>#n",
        "#R(V#0ampire#R)#n ",
        "",  /* Vampire has custom per-title colors */
        { "#RB#0loodsucker#n", "#RI#0nner #RC#0ircle#n", "#RV#0ampire #RJ#0usticar#n", "#RV#0ampire #RP#0rince#n",
          "#RV#0ampire #RA#0ncilla#n", "#RV#0ampire #RA#0rchon#n", "#RV#0ampire#n", "#RV#0ampire #RA#0narch#n" }
    },
    {
        CLASS_SAMURAI, "Samurai",
        "#C-=#n", "#C=-#n",
        "#C(#ySamu#Rrai#C)#n ",
        "",  /* Samurai has custom per-title colors */
        { "#RTrai#ynee#n", "#RSho#ygun#n", "#RKata#yna Mas#Rter#n", "#RSamu#yrai Mas#Rter#n",
          "#RSamu#yrai El#Rite#n", "#RSamu#yrai#n", "", "" }
    },
    {
        CLASS_DROW, "Drow",
        "#P.o0", "#P0o.#n",
        "#P(#0Drow#P)#n ",
        "#0",
        { "Drider", "Matron Mother", "Weaponmaster", "High Priestess", "Favored of Lloth", "Black Widow", "Drow", "Drow Male" }
    },
    {
        CLASS_MONK, "Monk",
        "#0.x[#n", "#0]x.#n",
        "#C(#nMonk#C)#n ",
        "#c",
        { "Fanatic", "Abbot", "High Priest", "Priest", "Father", "Monk", "Brother", "Acolyte" }
    },
    {
        CLASS_NINJA, "Ninja",
        "#C***#n", "#C***#n",
        "#R(#yNinja#R)#n ",
        "#y",
        { "Thug", "Master Assassin", "Expert Assassin", "Shadowlord", "Shadow Warrior", "Shadow", "Ninja", "Ninja Apprentice" }
    },
    {
        CLASS_LICH, "Lich",
        "#G>*<#n", "#G>*<#n",
        "#0(#GLich#0)#n ",
        "#7",
        { "Lich", "Demilich", "Lich Lord", "Power Lich", "Ancient Lich", "", "", "" }
    },
    {
        CLASS_SHAPESHIFTER, "Shapeshifter",
        "#0[#P*#0]#n", "#0[#P*#0]#n",
        "",  /* No look display for shapeshifter */
        "#R",
        { "Play-Doh", "Spawn of Malaug", "Shadowmaster", "Malaugrym Elder", "Malaugrym", "Shapeshifter", "Doppleganger", "Mass of tentacles" }
    },
    {
        CLASS_TANARRI, "Tanarri",
        "#y{#n", "#y}#n",
        "#y(#RTanar'ri#y)#n ",
        "#R",
        { "Cambion", "Tanar'ri Balor", "Tanar'ri Marilith", "Greater Tanar'ri", "True Tanar'ri", "Tanar'ri", "", "" }
    },
    {
        CLASS_ANGEL, "Angel",
        "#y.x#0[#n", "#0]#yx.#n",
        "#0(#7Angel#0)#n ",
        "#7",
        { "Nephalim", "Arch Angel", "Cherubim", "Seraphim", "Guardian Angel", "Angel", "", "" }
    },
    {
        CLASS_UNDEAD_KNIGHT, "Undead Knight",
        "#0|[#n", "#0]|#n",
        "#y(#0Death Knight#y)#n ",
        "#L",
        { "Skeleton Knight", "Fallen Paladin", "Undead Lord", "Undead Knight", "Undead Knight", "Undead Knight", "Skeleton Knight", "" }
    },
    {
        CLASS_DROID, "Spider Droid",
        "#p{#0-#p}#n", "#p{#0-#p}#n",
        "#p(#PDrider#p)#n ",
        "#0",
        { "Spider Droid", "Drider Lord", "Master of the Web", "", "", "", "", "" }
    },
    {
        CLASS_DIRGESINGER, "Dirgesinger",
        "#x136~#x178[#n", "#x178]#x136~#n",
        "#x178(#nDirgesinger#x178)#n ",
        "#x178",
        { "Hummer", "War Cantor", "Battle Bard", "Dirgesinger", "War Chanter", "Chanter", "", "" }
    },
    {
        CLASS_SIREN, "Siren",
        "#x039~#x147(#n", "#x147)#x039~#n",
        "#x147(#nSiren#x147)#n ",
        "#x147",
        { "Siren", "Archsiren", "Diva of Doom", "Songweaver", "Voice of Ruin", "", "", "" }
    },
    { 0, NULL, NULL, NULL, NULL, NULL, { NULL } }  /* Sentinel */
};

/*
 * Find a class in the display table by class_id
 */
CLASS_DISPLAY_DATA *class_display_get( int class_id ) {
    int i;
    for ( i = 0; i < class_display_count; i++ ) {
        if ( class_display_table[i].class_id == class_id )
            return &class_display_table[i];
    }
    return NULL;
}

/*
 * Get generation title for a class
 */
const char *class_get_title( int class_id, int generation ) {
    CLASS_DISPLAY_DATA *cd = class_display_get( class_id );

    if ( !cd )
        return "Unknown";

    if ( generation < 1 || generation > 7 || cd->titles[generation][0] == '\0' )
        return cd->titles[0];  /* Return default */

    return cd->titles[generation];
}

/*
 * Get who list symbols for a class
 */
void class_get_symbols( int class_id, const char **left, const char **right ) {
    static const char *empty = "";
    static const char *default_left = "#0[#n";
    static const char *default_right = "#0]#n";
    CLASS_DISPLAY_DATA *cd = class_display_get( class_id );

    if ( !cd ) {
        *left = default_left;
        *right = default_right;
        return;
    }

    *left = cd->left_symbol[0] ? cd->left_symbol : empty;
    *right = cd->right_symbol[0] ? cd->right_symbol : empty;
}

/*
 * Get look display string for a class
 */
const char *class_get_look_display( int class_id ) {
    CLASS_DISPLAY_DATA *cd = class_display_get( class_id );

    if ( !cd )
        return "";

    return cd->look_display;
}

/*
 * Get title color code for a class
 */
const char *class_get_title_color( int class_id ) {
    CLASS_DISPLAY_DATA *cd = class_display_get( class_id );

    if ( !cd || cd->title_color[0] == '\0' )
        return "";  /* No color prefix - title has its own colors */

    return cd->title_color;
}

/*
 * Seed the class display table with default values
 */
static void class_display_seed_defaults( void ) {
    int j;
    const CLASS_SEED *seed;

    class_display_count = 0;

    for ( seed = class_seeds; seed->name != NULL; seed++ ) {
        if ( class_display_count >= MAX_CLASS )
            break;

        CLASS_DISPLAY_DATA *cd = &class_display_table[class_display_count];

        cd->class_id = seed->class_id;
        strncpy( cd->name, seed->name, CLASS_NAME_LEN - 1 );
        cd->name[CLASS_NAME_LEN - 1] = '\0';
        strncpy( cd->left_symbol, seed->left_symbol, CLASS_SYMBOL_LEN - 1 );
        cd->left_symbol[CLASS_SYMBOL_LEN - 1] = '\0';
        strncpy( cd->right_symbol, seed->right_symbol, CLASS_SYMBOL_LEN - 1 );
        cd->right_symbol[CLASS_SYMBOL_LEN - 1] = '\0';
        strncpy( cd->look_display, seed->look_display ? seed->look_display : "", CLASS_LOOK_LEN - 1 );
        cd->look_display[CLASS_LOOK_LEN - 1] = '\0';
        strncpy( cd->title_color, seed->title_color ? seed->title_color : "", sizeof(cd->title_color) - 1 );
        cd->title_color[sizeof(cd->title_color) - 1] = '\0';

        for ( j = 0; j < MAX_GENERATIONS; j++ ) {
            if ( seed->titles[j] ) {
                strncpy( cd->titles[j], seed->titles[j], CLASS_TITLE_LEN - 1 );
                cd->titles[j][CLASS_TITLE_LEN - 1] = '\0';
            } else {
                cd->titles[j][0] = '\0';
            }
        }

        class_display_count++;
    }
}

/*
 * Initialize class display system
 */
void class_display_init( void ) {
    log_string( "  Initializing class display data..." );

    /* Seed with defaults */
    class_display_seed_defaults();

    /* Load overrides from database - this will be called after db_game_init */
}

/*
 * Validate that all known CLASS_* constants have display entries
 */
void class_display_validate( void ) {
    struct {
        int class_id;
        const char *name;
    } known_classes[] = {
        { CLASS_DEMON,         "Demon" },
        { CLASS_MAGE,          "Mage" },
        { CLASS_WEREWOLF,      "Werewolf" },
        { CLASS_VAMPIRE,       "Vampire" },
        { CLASS_SAMURAI,       "Samurai" },
        { CLASS_DROW,          "Drow" },
        { CLASS_MONK,          "Monk" },
        { CLASS_NINJA,         "Ninja" },
        { CLASS_LICH,          "Lich" },
        { CLASS_SHAPESHIFTER,  "Shapeshifter" },
        { CLASS_TANARRI,       "Tanarri" },
        { CLASS_ANGEL,         "Angel" },
        { CLASS_UNDEAD_KNIGHT, "Undead Knight" },
        { CLASS_DROID,         "Spider Droid" },
        { CLASS_DIRGESINGER,   "Dirgesinger" },
        { CLASS_SIREN,         "Siren" },
        { 0, NULL }
    };
    int i;
    int missing = 0;
    char buf[MAX_STRING_LENGTH];

    for ( i = 0; known_classes[i].name != NULL; i++ ) {
        if ( !class_display_get( known_classes[i].class_id ) ) {
            snprintf( buf, sizeof(buf),
                "  WARNING: Class %s (id %d) has no display entry!",
                known_classes[i].name, known_classes[i].class_id );
            log_string( buf );
            missing++;
        }
    }

    if ( missing == 0 ) {
        snprintf( buf, sizeof(buf), "  Validated %d class display entries.", class_display_count );
        log_string( buf );
    }
}

/*
 * Admin command: classedit
 * Syntax: classedit                     - list all classes
 *         classedit <class>             - show class details
 *         classedit <class> symbol <left> <right>
 *         classedit <class> look <display>
 *         classedit <class> color <code>
 *         classedit <class> title <gen|default> <title>
 */
void do_classedit( CHAR_DATA *ch, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CLASS_DISPLAY_DATA *cd;
    int i;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    /* List all classes */
    if ( arg1[0] == '\0' ) {
        send_to_char( "#GClass Display Configuration#n\n\r", ch );
        send_to_char( "#w--------------------------#n\n\r", ch );

        for ( i = 0; i < class_display_count; i++ ) {
            cd = &class_display_table[i];
            snprintf( buf, sizeof(buf), "  %s%-16s#n (id %5d) %s...%s\n\r",
                cd->title_color[0] ? cd->title_color : "#n",
                cd->name,
                cd->class_id,
                cd->left_symbol,
                cd->right_symbol );
            send_to_char( buf, ch );
        }

        send_to_char( "\n\rSyntax: classedit <class>\n\r", ch );
        send_to_char( "        classedit <class> symbol <left> <right>\n\r", ch );
        send_to_char( "        classedit <class> look <display>\n\r", ch );
        send_to_char( "        classedit <class> color <code>\n\r", ch );
        send_to_char( "        classedit <class> title <gen#|default> <title>\n\r", ch );
        return;
    }

    /* Find the class */
    cd = NULL;
    for ( i = 0; i < class_display_count; i++ ) {
        if ( !str_prefix( arg1, class_display_table[i].name ) ) {
            cd = &class_display_table[i];
            break;
        }
    }

    if ( !cd ) {
        send_to_char( "No such class. Use 'classedit' to see the list.\n\r", ch );
        return;
    }

    /* Show class details */
    if ( arg2[0] == '\0' ) {
        snprintf( buf, sizeof(buf), "#G%s#n (id %d)\n\r", cd->name, cd->class_id );
        send_to_char( buf, ch );
        send_to_char( "#w----------------------------------------#n\n\r", ch );

        snprintf( buf, sizeof(buf), "Left Symbol:  '%s#n'\n\r", cd->left_symbol );
        send_to_char( buf, ch );
        snprintf( buf, sizeof(buf), "Right Symbol: '%s#n'\n\r", cd->right_symbol );
        send_to_char( buf, ch );
        snprintf( buf, sizeof(buf), "Look Display: '%s#n'\n\r", cd->look_display );
        send_to_char( buf, ch );
        snprintf( buf, sizeof(buf), "Title Color:  '%s' (preview: %sExample#n)\n\r",
            cd->title_color[0] ? cd->title_color : "(none)",
            cd->title_color[0] ? cd->title_color : "" );
        send_to_char( buf, ch );

        send_to_char( "\n\r#wGeneration Titles:#n\n\r", ch );
        snprintf( buf, sizeof(buf), "  Default: %s%s#n\n\r", cd->title_color, cd->titles[0] );
        send_to_char( buf, ch );
        for ( i = 1; i < MAX_GENERATIONS; i++ ) {
            if ( cd->titles[i][0] != '\0' ) {
                snprintf( buf, sizeof(buf), "  Gen %d:   %s%s#n\n\r", i, cd->title_color, cd->titles[i] );
                send_to_char( buf, ch );
            }
        }
        return;
    }

    /* Edit symbol */
    if ( !str_prefix( arg2, "symbol" ) ) {
        argument = one_argument( argument, arg3 );

        if ( arg3[0] == '\0' || argument[0] == '\0' ) {
            send_to_char( "Syntax: classedit <class> symbol <left> <right>\n\r", ch );
            return;
        }

        snprintf( cd->left_symbol, CLASS_SYMBOL_LEN, "%s", arg3 );
        snprintf( cd->right_symbol, CLASS_SYMBOL_LEN, "%s", argument );

        db_class_display_save_one( cd->class_id );

        snprintf( buf, sizeof(buf), "Set %s symbols to: %s#n ... %s#n\n\r",
            cd->name, cd->left_symbol, cd->right_symbol );
        send_to_char( buf, ch );
        return;
    }

    /* Edit look display */
    if ( !str_prefix( arg2, "look" ) ) {
        if ( argument[0] == '\0' ) {
            cd->look_display[0] = '\0';
            send_to_char( "Look display cleared.\n\r", ch );
        } else {
            /* Re-parse to get full remaining argument including the first word */
            char *full_arg = arg2 + strlen(arg2) + 1;
            while ( *full_arg == ' ' ) full_arg++;
            snprintf( cd->look_display, CLASS_LOOK_LEN, "%s", full_arg );
            snprintf( buf, sizeof(buf), "Set %s look display to: %s#n\n\r", cd->name, cd->look_display );
            send_to_char( buf, ch );
        }

        db_class_display_save_one( cd->class_id );
        return;
    }

    /* Edit title color */
    if ( !str_prefix( arg2, "color" ) ) {
        argument = one_argument( argument, arg3 );

        snprintf( cd->title_color, sizeof(cd->title_color), "%s", arg3 );

        db_class_display_save_one( cd->class_id );

        if ( arg3[0] == '\0' ) {
            snprintf( buf, sizeof(buf), "Cleared %s title color (titles use embedded colors).\n\r", cd->name );
        } else {
            snprintf( buf, sizeof(buf), "Set %s title color to: %s (preview: %sExample#n)\n\r",
                cd->name, cd->title_color, cd->title_color );
        }
        send_to_char( buf, ch );
        return;
    }

    /* Edit generation title */
    if ( !str_prefix( arg2, "title" ) ) {
        int gen;

        argument = one_argument( argument, arg3 );

        if ( arg3[0] == '\0' ) {
            send_to_char( "Syntax: classedit <class> title <gen#|default> <title>\n\r", ch );
            return;
        }

        if ( !str_cmp( arg3, "default" ) || !str_cmp( arg3, "0" ) ) {
            gen = 0;
        } else {
            gen = atoi( arg3 );
            if ( gen < 1 || gen > 7 ) {
                send_to_char( "Generation must be 1-7 or 'default'.\n\r", ch );
                return;
            }
        }

        snprintf( cd->titles[gen], CLASS_TITLE_LEN, "%s", argument );

        db_class_display_save_one( cd->class_id );

        if ( gen == 0 ) {
            snprintf( buf, sizeof(buf), "Set %s default title to: %s%s#n\n\r",
                cd->name, cd->title_color, cd->titles[0] );
        } else {
            snprintf( buf, sizeof(buf), "Set %s gen %d title to: %s%s#n\n\r",
                cd->name, gen, cd->title_color, cd->titles[gen] );
        }
        send_to_char( buf, ch );
        return;
    }

    send_to_char( "Unknown option. Use 'classedit' for syntax.\n\r", ch );
}
