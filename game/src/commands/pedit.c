/***************************************************************************
 *  pedit.c - Unified Player Editor Command
 *
 *  Replaces mset, pset, jope, and sset with a single comprehensive command.
 *  Supports both online players (direct CHAR_DATA editing) and offline
 *  players (via SQLite database).
 *
 *  Syntax:
 *    pedit <player>                       - Enter interactive mode
 *    pedit <player> show [category]       - Display player data
 *    pedit <player> <category> <field> <value> - Set a value
 *    pedit done                           - Save and exit interactive mode
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "merc.h"
#include "../db/db_player.h"

/* Forward declarations */
void pedit_interp( CHAR_DATA *ch, char *argument );
void pedit_show( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_identity( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_vitals( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_resources( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_stats( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_combat( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_proficiency( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_stance( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_skill( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_powers( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_flags( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_immunity( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_strings( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_statistics( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_admin( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
void pedit_inventory( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );

/* Category table - maps category names to handler functions and trust levels */
struct pedit_category_type {
    const char *name;
    void (*handler)( CHAR_DATA *ch, CHAR_DATA *victim, char *argument );
    int trust_level;
    const char *description;
};

const struct pedit_category_type pedit_categories[] = {
    { "show",       pedit_show,        LEVEL_BUILDER,     "Display player data" },
    { "identity",   pedit_identity,    LEVEL_QUESTMAKER,  "name, title, sex, class, level, clan" },
    { "vitals",     pedit_vitals,      LEVEL_QUESTMAKER,  "hp, mana, move (current and max)" },
    { "resources",  pedit_resources,   LEVEL_QUESTMAKER,  "gold, exp, quest, practice, primal" },
    { "stats",      pedit_stats,       LEVEL_JUDGE,       "str, int, wis, dex, con" },
    { "combat",     pedit_combat,      LEVEL_QUESTMAKER,  "hitroll, damroll, armor, alignment" },
    { "proficiency",pedit_proficiency, LEVEL_QUESTMAKER,  "wpn, spl, cmbt arrays" },
    { "stance",     pedit_stance,      LEVEL_QUESTMAKER,  "stance array (24 stances)" },
    { "skill",      pedit_skill,       LEVEL_QUESTMAKER,  "skill proficiencies" },
    { "powers",     pedit_powers,      LEVEL_QUESTMAKER,  "power array (disciplines)" },
    { "flags",      pedit_flags,       LEVEL_JUDGE,       "act, extra, newbits, affected_by" },
    { "immunity",   pedit_immunity,    LEVEL_QUESTMAKER,  "immune flags" },
    { "strings",    pedit_strings,     LEVEL_QUESTMAKER,  "bamfin, bamfout, messages" },
    { "statistics", pedit_statistics,  LEVEL_QUESTMAKER,  "pkill, pdeath, mkill, mdeath" },
    { "admin",      pedit_admin,       LEVEL_IMPLEMENTOR, "trust, security, bounty" },
    { "inventory",  pedit_inventory,   LEVEL_QUESTMAKER,  "wear, remove, get, drop, equipment" },
    { "", NULL, 0, "" }
};

/*
 * Helper: Load an offline player for editing
 * Returns TRUE on success, FALSE if player not found or already online
 */
bool pedit_load_offline( CHAR_DATA *ch, const char *name ) {
    DESCRIPTOR_DATA *dummy;
    char cap_name[MAX_INPUT_LENGTH];

    if ( ch->pcdata->pfile != NULL ) {
        send_to_char( "You are already editing a pfile. Use 'pedit done' first.\n\r", ch );
        return FALSE;
    }

    /* Capitalize name */
    strncpy( cap_name, name, MAX_INPUT_LENGTH - 1 );
    cap_name[0] = toupper( cap_name[0] );

    /* Check if player is online */
    if ( get_char_world( ch, cap_name ) != NULL ) {
        send_to_char( "That player is online. Edit them directly without 'pedit done'.\n\r", ch );
        return FALSE;
    }

    /* Allocate temporary descriptor */
    dummy = calloc( 1, sizeof( *dummy ) );
    if ( !dummy ) {
        bug( "pedit_load: calloc failed", 0 );
        return FALSE;
    }

    /* Load the player */
    if ( load_char_obj( dummy, cap_name ) ) {
        ch->pcdata->pfile = dummy->character;
    } else {
        send_to_char( "No such player file.\n\r", ch );
    }

    /* Free temporary descriptor */
    free( dummy );

    if ( ch->pcdata->pfile == NULL )
        return FALSE;

    return TRUE;
}

/*
 * Helper: Save and free an offline player
 */
void pedit_save_offline( CHAR_DATA *ch ) {
    CHAR_DATA *victim;

    if ( ( victim = ch->pcdata->pfile ) == NULL ) {
        send_to_char( "You are not editing an offline player.\n\r", ch );
        return;
    }

    save_char_obj( victim );
    free_char( victim );
    ch->pcdata->pfile = NULL;
}

/*
 * Main pedit command
 */
void do_pedit( CHAR_DATA *ch, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    int cmd;
    bool found = FALSE;

    if ( IS_NPC( ch ) )
        return;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    /* Handle 'pedit done' - save and exit interactive mode */
    if ( !str_cmp( arg1, "done" ) ) {
        if ( ch->pcdata->pfile == NULL ) {
            send_to_char( "You are not editing an offline player.\n\r", ch );
            return;
        }
        pedit_save_offline( ch );
        if ( ch->desc )
            ch->desc->connected = CON_PLAYING;
        send_to_char( "Player saved and edit mode exited.\n\r", ch );
        return;
    }

    /* Need at least a player name */
    if ( arg1[0] == '\0' ) {
        send_to_char( "#7Syntax: pedit <player>                       - Enter interactive mode\n\r", ch );
        send_to_char( "#7        pedit <player> show [category]       - Display data\n\r", ch );
        send_to_char( "#7        pedit <player> <category> <field> <value>\n\r", ch );
        send_to_char( "#7        pedit done                           - Save and exit\n\r", ch );
        if ( ch->level >= LEVEL_IMPLEMENTOR ) {
            send_to_char( "#7        pedit <player> delete confirm       - #RPERMANENTLY delete#n\n\r", ch );
        }
        send_to_char( "\n\r#7Categories:#n\n\r", ch );
        for ( cmd = 0; pedit_categories[cmd].name[0] != '\0'; cmd++ ) {
            if ( pedit_categories[cmd].trust_level <= ch->level ) {
                snprintf( buf, sizeof( buf ), "  #3%-12s#n - %s\n\r",
                         pedit_categories[cmd].name,
                         pedit_categories[cmd].description );
                send_to_char( buf, ch );
            }
        }
        return;
    }

    /* Try to find the victim - online first, then offline */
    victim = get_char_world( ch, arg1 );

    if ( victim == NULL ) {
        /* Try to load offline player */
        if ( !db_player_exists( arg1 ) ) {
            send_to_char( "No such player (online or offline).\n\r", ch );
            return;
        }

        /* If no category given, enter interactive mode */
        if ( arg2[0] == '\0' ) {
            if ( !pedit_load_offline( ch, arg1 ) )
                return;
            victim = ch->pcdata->pfile;
            if ( ch->desc )
                ch->desc->connected = CON_PFILE;
            snprintf( buf, sizeof( buf ), "Now editing #G%s#n (offline). Use 'pedit done' to save.\n\r", victim->name );
            send_to_char( buf, ch );
            return;
        }

        /* Load for single edit */
        if ( !pedit_load_offline( ch, arg1 ) )
            return;
        victim = ch->pcdata->pfile;
    }

    /* Security check - can't edit higher level players */
    if ( victim->level > ch->level && victim != ch ) {
        send_to_char( "You cannot edit players of higher level than yourself.\n\r", ch );
        if ( ch->pcdata->pfile == victim ) {
            pedit_save_offline( ch );
        }
        return;
    }

    /* Handle 'pedit <player> delete confirm' - PERMANENTLY delete player file */
    if ( !str_cmp( arg2, "delete" ) ) {
        if ( ch->level < LEVEL_IMPLEMENTOR ) {
            send_to_char( "Only Implementors can delete player files.\n\r", ch );
            if ( ch->pcdata->pfile == victim ) {
                free_char( victim );
                ch->pcdata->pfile = NULL;
            }
            return;
        }

        if ( victim != ch->pcdata->pfile ) {
            send_to_char( "You can only delete OFFLINE players. This player is online.\n\r", ch );
            return;
        }

        if ( str_cmp( argument, "confirm" ) ) {
            snprintf( buf, sizeof( buf ), "#RWARNING:#n This will PERMANENTLY delete %s's player file!\n\r", victim->name );
            send_to_char( buf, ch );
            send_to_char( "To confirm, type: pedit <player> delete confirm\n\r", ch );
            free_char( victim );
            ch->pcdata->pfile = NULL;
            return;
        }

        /* Delete confirmed - do it */
        {
            char victim_name[MAX_INPUT_LENGTH];
            strncpy( victim_name, victim->name, MAX_INPUT_LENGTH - 1 );
            victim_name[MAX_INPUT_LENGTH - 1] = '\0';
            free_char( victim );
            ch->pcdata->pfile = NULL;

            if ( db_player_delete( victim_name ) ) {
                snprintf( buf, sizeof( buf ), "#RPlayer %s has been permanently deleted.#n\n\r", victim_name );
                send_to_char( buf, ch );
            } else {
                send_to_char( "Failed to delete player file.\n\r", ch );
            }
        }
        return;
    }

    /* If no category given and player is online, show help */
    if ( arg2[0] == '\0' ) {
        snprintf( buf, sizeof( buf ), "#GPlayer: %s#n is online. Specify a category.\n\r", victim->name );
        send_to_char( buf, ch );
        send_to_char( "Use: pedit <player> <category> <field> <value>\n\r", ch );
        return;
    }

    /* Find and execute category handler */
    for ( cmd = 0; pedit_categories[cmd].name[0] != '\0'; cmd++ ) {
        if ( !str_prefix( arg2, pedit_categories[cmd].name ) ) {
            if ( pedit_categories[cmd].trust_level > ch->level ) {
                send_to_char( "You don't have permission for that category.\n\r", ch );
                found = TRUE;
                break;
            }
            found = TRUE;
            (*pedit_categories[cmd].handler)( ch, victim, argument );
            break;
        }
    }

    if ( !found ) {
        snprintf( buf, sizeof( buf ), "Unknown category '%s'. See 'pedit' for list.\n\r", arg2 );
        send_to_char( buf, ch );
    }

    /* If editing offline player with single command, save and free */
    if ( ch->pcdata->pfile == victim && ch->desc && ch->desc->connected != CON_PFILE ) {
        pedit_save_offline( ch );
    }
}

/*
 * pedit show - Display player data
 */
void pedit_show( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    /* Header */
    snprintf( buf, sizeof( buf ), "\n\r#G===== Player: %s =====#n\n\r\n\r", victim->name );
    send_to_char( buf, ch );

    /* Identity */
    snprintf( buf, sizeof( buf ), "#7[Identity]#n\n\r" );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Name: %s  Title: %s\n\r", victim->name,
             victim->pcdata ? victim->pcdata->title : "(none)" );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Sex: %s  Class: %d  Level: %d  Trust: %d\n\r",
             victim->sex == SEX_MALE ? "Male" : victim->sex == SEX_FEMALE ? "Female" : "Neutral",
             victim->class, victim->level, victim->trust );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Clan: %s  Generation: %d\n\r",
             victim->clan ? victim->clan : "(none)", victim->generation );
    send_to_char( buf, ch );

    /* Vitals */
    snprintf( buf, sizeof( buf ), "\n\r#7[Vitals]#n\n\r" );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  HP: %d/%d  Mana: %d/%d  Move: %d/%d\n\r",
             victim->hit, victim->max_hit,
             victim->mana, victim->max_mana,
             victim->move, victim->max_move );
    send_to_char( buf, ch );

    /* Resources */
    snprintf( buf, sizeof( buf ), "\n\r#7[Resources]#n\n\r" );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Gold: %d  Exp: %d  Practice: %d\n\r",
             victim->gold, victim->exp, victim->practice );
    send_to_char( buf, ch );
    if ( victim->pcdata ) {
        snprintf( buf, sizeof( buf ), "  Quest Points: %d\n\r", victim->pcdata->quest );
        send_to_char( buf, ch );
    }

    /* Combat */
    snprintf( buf, sizeof( buf ), "\n\r#7[Combat]#n\n\r" );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Hitroll: %d  Damroll: %d  Armor: %d\n\r",
             victim->hitroll, victim->damroll, victim->armor );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Alignment: %d  Saving Throw: %d\n\r",
             victim->alignment, victim->saving_throw );
    send_to_char( buf, ch );

    /* Stats */
    if ( victim->pcdata ) {
        snprintf( buf, sizeof( buf ), "\n\r#7[Attributes]#n\n\r" );
        send_to_char( buf, ch );
        snprintf( buf, sizeof( buf ), "  Str: %d  Int: %d  Wis: %d  Dex: %d  Con: %d\n\r",
                 victim->pcdata->perm_str, victim->pcdata->perm_int,
                 victim->pcdata->perm_wis, victim->pcdata->perm_dex,
                 victim->pcdata->perm_con );
        send_to_char( buf, ch );
    }

    /* Statistics */
    snprintf( buf, sizeof( buf ), "\n\r#7[Statistics]#n\n\r" );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Player Kills: %d  Player Deaths: %d\n\r",
             victim->pkill, victim->pdeath );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Mob Kills: %d  Mob Deaths: %d\n\r",
             victim->mkill, victim->mdeath );
    send_to_char( buf, ch );

    send_to_char( "\n\r", ch );
}

/*
 * pedit identity - Edit identity fields
 */
void pedit_identity( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg1 );
    strcpy( arg2, argument );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> identity <field> <value>\n\r", ch );
        send_to_char( "Fields: name, title, sex, class, level, clan, generation\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "name" ) ) {
        if ( arg2[0] == '\0' ) {
            send_to_char( "Change name to what?\n\r", ch );
            return;
        }
        /* Note: Changing name is complex - need to rename pfile too */
        send_to_char( "Name changes not yet supported via pedit.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "title" ) ) {
        if ( IS_NPC( victim ) || !victim->pcdata ) {
            send_to_char( "Not on NPCs.\n\r", ch );
            return;
        }
        if ( victim->pcdata->title )
            free(victim->pcdata->title);
        victim->pcdata->title = str_dup( arg2 );
        send_to_char( "Title set.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "sex" ) ) {
        if ( !str_cmp( arg2, "male" ) )
            victim->sex = SEX_MALE;
        else if ( !str_cmp( arg2, "female" ) )
            victim->sex = SEX_FEMALE;
        else if ( !str_cmp( arg2, "neutral" ) )
            victim->sex = SEX_NEUTRAL;
        else {
            send_to_char( "Sex must be male, female, or neutral.\n\r", ch );
            return;
        }
        send_to_char( "Sex set.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "class" ) ) {
        if ( !is_number( arg2 ) ) {
            send_to_char( "Class must be a number.\n\r", ch );
            return;
        }
        victim->class = atoi( arg2 );
        send_to_char( "Class set.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "level" ) ) {
        if ( !is_number( arg2 ) ) {
            send_to_char( "Level must be a number.\n\r", ch );
            return;
        }
        value = atoi( arg2 );
        if ( value < 1 || value > 12 ) {
            send_to_char( "Level must be 1-12.\n\r", ch );
            return;
        }
        victim->level = value;
        send_to_char( "Level set.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "clan" ) ) {
        if ( victim->clan )
            free(victim->clan);
        victim->clan = str_dup( arg2 );
        send_to_char( "Clan set.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "generation" ) || !str_cmp( arg1, "gen" ) ) {
        if ( !is_number( arg2 ) ) {
            send_to_char( "Generation must be a number.\n\r", ch );
            return;
        }
        victim->generation = atoi( arg2 );
        send_to_char( "Generation set.\n\r", ch );
        return;
    }

    send_to_char( "Unknown identity field. Use: name, title, sex, class, level, clan, generation\n\r", ch );
}

/*
 * pedit vitals - Edit HP, Mana, Move
 */
void pedit_vitals( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> vitals <field> <value>\n\r", ch );
        send_to_char( "Fields: hp, max_hp, mana, max_mana, move, max_move\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Value must be a number.\n\r", ch );
        return;
    }
    value = atoi( arg2 );

    if ( !str_cmp( arg1, "hp" ) ) {
        victim->hit = value;
        send_to_char( "HP set.\n\r", ch );
    } else if ( !str_cmp( arg1, "max_hp" ) || !str_cmp( arg1, "maxhp" ) ) {
        victim->max_hit = value;
        send_to_char( "Max HP set.\n\r", ch );
    } else if ( !str_cmp( arg1, "mana" ) ) {
        victim->mana = value;
        send_to_char( "Mana set.\n\r", ch );
    } else if ( !str_cmp( arg1, "max_mana" ) || !str_cmp( arg1, "maxmana" ) ) {
        victim->max_mana = value;
        send_to_char( "Max Mana set.\n\r", ch );
    } else if ( !str_cmp( arg1, "move" ) ) {
        victim->move = value;
        send_to_char( "Move set.\n\r", ch );
    } else if ( !str_cmp( arg1, "max_move" ) || !str_cmp( arg1, "maxmove" ) ) {
        victim->max_move = value;
        send_to_char( "Max Move set.\n\r", ch );
    } else {
        send_to_char( "Unknown vitals field. Use: hp, max_hp, mana, max_mana, move, max_move\n\r", ch );
    }
}

/*
 * pedit resources - Edit gold, exp, quest points, practice
 */
void pedit_resources( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> resources <field> <value>\n\r", ch );
        send_to_char( "Fields: gold, exp, quest, practice, primal\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Value must be a number.\n\r", ch );
        return;
    }
    value = atoi( arg2 );

    if ( !str_cmp( arg1, "gold" ) ) {
        victim->gold = value;
        send_to_char( "Gold set.\n\r", ch );
    } else if ( !str_cmp( arg1, "exp" ) ) {
        victim->exp = value;
        send_to_char( "Experience set.\n\r", ch );
    } else if ( !str_cmp( arg1, "quest" ) || !str_cmp( arg1, "qps" ) ) {
        if ( IS_NPC( victim ) || !victim->pcdata ) {
            send_to_char( "Not on NPCs.\n\r", ch );
            return;
        }
        victim->pcdata->quest = value;
        send_to_char( "Quest points set.\n\r", ch );
    } else if ( !str_cmp( arg1, "practice" ) || !str_cmp( arg1, "primal" ) ) {
        victim->practice = value;
        send_to_char( "Practice/Primal set.\n\r", ch );
    } else {
        send_to_char( "Unknown resources field. Use: gold, exp, quest, practice, primal\n\r", ch );
    }
}

/*
 * pedit stats - Edit attributes (str, int, wis, dex, con)
 */
void pedit_stats( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( victim ) || !victim->pcdata ) {
        send_to_char( "Not on NPCs.\n\r", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> stats <field> <value>\n\r", ch );
        send_to_char( "Fields: str, int, wis, dex, con\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Value must be a number.\n\r", ch );
        return;
    }
    value = URANGE( 1, atoi( arg2 ), 250 );

    if ( !str_cmp( arg1, "str" ) ) {
        victim->pcdata->perm_str = value;
        send_to_char( "Strength set.\n\r", ch );
    } else if ( !str_cmp( arg1, "int" ) ) {
        victim->pcdata->perm_int = value;
        send_to_char( "Intelligence set.\n\r", ch );
    } else if ( !str_cmp( arg1, "wis" ) ) {
        victim->pcdata->perm_wis = value;
        send_to_char( "Wisdom set.\n\r", ch );
    } else if ( !str_cmp( arg1, "dex" ) ) {
        victim->pcdata->perm_dex = value;
        send_to_char( "Dexterity set.\n\r", ch );
    } else if ( !str_cmp( arg1, "con" ) ) {
        victim->pcdata->perm_con = value;
        send_to_char( "Constitution set.\n\r", ch );
    } else {
        send_to_char( "Unknown stats field. Use: str, int, wis, dex, con\n\r", ch );
    }
}

/*
 * pedit combat - Edit combat modifiers
 */
void pedit_combat( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> combat <field> <value>\n\r", ch );
        send_to_char( "Fields: hitroll, damroll, armor, alignment, saving\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Value must be a number.\n\r", ch );
        return;
    }
    value = atoi( arg2 );

    if ( !str_cmp( arg1, "hitroll" ) || !str_cmp( arg1, "hit" ) ) {
        victim->hitroll = value;
        send_to_char( "Hitroll set.\n\r", ch );
    } else if ( !str_cmp( arg1, "damroll" ) || !str_cmp( arg1, "dam" ) ) {
        victim->damroll = value;
        send_to_char( "Damroll set.\n\r", ch );
    } else if ( !str_cmp( arg1, "armor" ) || !str_cmp( arg1, "ac" ) ) {
        victim->armor = value;
        send_to_char( "Armor set.\n\r", ch );
    } else if ( !str_cmp( arg1, "alignment" ) || !str_cmp( arg1, "align" ) ) {
        victim->alignment = URANGE( -1000, value, 1000 );
        send_to_char( "Alignment set.\n\r", ch );
    } else if ( !str_cmp( arg1, "saving" ) || !str_cmp( arg1, "save" ) ) {
        victim->saving_throw = value;
        send_to_char( "Saving throw set.\n\r", ch );
    } else {
        send_to_char( "Unknown combat field. Use: hitroll, damroll, armor, alignment, saving\n\r", ch );
    }
}

/*
 * pedit proficiency - Edit weapon/spell/combat proficiencies
 */
void pedit_proficiency( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    int index, value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> proficiency <type> <index> <value>\n\r", ch );
        send_to_char( "Types: wpn (0-12), spl (0-4), cmbt (0-7)\n\r", ch );
        send_to_char( "   or: pedit <player> proficiency all <value>\n\r", ch );
        return;
    }

    /* Handle 'all' - set all proficiencies to a value */
    if ( !str_cmp( arg1, "all" ) ) {
        if ( !is_number( arg2 ) ) {
            send_to_char( "Value must be a number.\n\r", ch );
            return;
        }
        value = atoi( arg2 );
        for ( index = 0; index < 13; index++ )
            victim->wpn[index] = value;
        for ( index = 0; index < 5; index++ )
            victim->spl[index] = value;
        for ( index = 0; index < 8; index++ )
            victim->cmbt[index] = value;
        send_to_char( "All proficiencies set.\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) || !is_number( arg3 ) ) {
        send_to_char( "Index and value must be numbers.\n\r", ch );
        return;
    }
    index = atoi( arg2 );
    value = atoi( arg3 );

    if ( !str_cmp( arg1, "wpn" ) ) {
        if ( index < 0 || index > 12 ) {
            send_to_char( "Weapon index must be 0-12.\n\r", ch );
            return;
        }
        victim->wpn[index] = value;
        send_to_char( "Weapon proficiency set.\n\r", ch );
    } else if ( !str_cmp( arg1, "spl" ) ) {
        if ( index < 0 || index > 4 ) {
            send_to_char( "Spell index must be 0-4.\n\r", ch );
            return;
        }
        victim->spl[index] = value;
        send_to_char( "Spell proficiency set.\n\r", ch );
    } else if ( !str_cmp( arg1, "cmbt" ) ) {
        if ( index < 0 || index > 7 ) {
            send_to_char( "Combat index must be 0-7.\n\r", ch );
            return;
        }
        victim->cmbt[index] = value;
        send_to_char( "Combat proficiency set.\n\r", ch );
    } else {
        send_to_char( "Unknown proficiency type. Use: wpn, spl, cmbt, all\n\r", ch );
    }
}

/*
 * pedit stance - Edit stance values
 */
void pedit_stance( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int index, value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> stance <index> <value>\n\r", ch );
        send_to_char( "        pedit <player> stance all <value>\n\r", ch );
        send_to_char( "Indices: 1-10 (normal), 13-17 (super stances)\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "all" ) ) {
        if ( !is_number( arg2 ) ) {
            send_to_char( "Value must be a number.\n\r", ch );
            return;
        }
        value = atoi( arg2 );
        for ( index = 0; index < 24; index++ )
            victim->stance[index] = value;
        send_to_char( "All stances set.\n\r", ch );
        return;
    }

    if ( !is_number( arg1 ) || !is_number( arg2 ) ) {
        send_to_char( "Index and value must be numbers.\n\r", ch );
        return;
    }
    index = atoi( arg1 );
    value = atoi( arg2 );

    if ( index < 0 || index > 23 ) {
        send_to_char( "Stance index must be 0-23.\n\r", ch );
        return;
    }

    victim->stance[index] = value;
    send_to_char( "Stance set.\n\r", ch );
}

/*
 * pedit skill - Edit skill proficiencies
 */
void pedit_skill( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int sn, value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( victim ) || !victim->pcdata ) {
        send_to_char( "Not on NPCs.\n\r", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> skill <skillname> <value>\n\r", ch );
        send_to_char( "        pedit <player> skill all <value>\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Value must be a number (0-100).\n\r", ch );
        return;
    }
    value = URANGE( 0, atoi( arg2 ), 100 );

    if ( !str_cmp( arg1, "all" ) ) {
        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            victim->pcdata->learned[sn] = value;
        }
        send_to_char( "All skills set.\n\r", ch );
        return;
    }

    sn = skill_lookup( arg1 );
    if ( sn < 0 ) {
        send_to_char( "No such skill.\n\r", ch );
        return;
    }

    victim->pcdata->learned[sn] = value;
    send_to_char( "Skill set.\n\r", ch );
}

/*
 * pedit powers - Edit power/discipline array
 */
void pedit_powers( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int index, value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> powers <index> <value>\n\r", ch );
        send_to_char( "Indices: 0-43 (see merc.h for DISC_* constants)\n\r", ch );
        return;
    }

    if ( !is_number( arg1 ) || !is_number( arg2 ) ) {
        send_to_char( "Index and value must be numbers.\n\r", ch );
        return;
    }
    index = atoi( arg1 );
    value = atoi( arg2 );

    if ( index < 0 || index >= MAX_DISCIPLINES ) {
        send_to_char( "Power index out of range.\n\r", ch );
        return;
    }

    victim->power[index] = value;
    send_to_char( "Power set.\n\r", ch );
}

/*
 * pedit flags - Edit flag bitfields
 */
void pedit_flags( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> flags <type> <flagname>\n\r", ch );
        send_to_char( "Types: act, extra, newbits, affected\n\r", ch );
        send_to_char( "Toggles the flag on/off.\n\r", ch );
        return;
    }

    /* This is a simplified version - expand as needed */
    if ( !str_cmp( arg1, "act" ) ) {
        if ( !str_cmp( arg2, "freeze" ) ) {
            TOGGLE_BIT( victim->act, PLR_FREEZE );
            send_to_char( "Freeze toggled.\n\r", ch );
        } else if ( !str_cmp( arg2, "silence" ) ) {
            TOGGLE_BIT( victim->act, PLR_SILENCE );
            send_to_char( "Silence toggled.\n\r", ch );
        } else if ( !str_cmp( arg2, "deny" ) ) {
            TOGGLE_BIT( victim->act, PLR_DENY );
            send_to_char( "Deny toggled.\n\r", ch );
        } else if ( !str_cmp( arg2, "ansi" ) ) {
            TOGGLE_BIT( victim->act, PLR_ANSI );
            send_to_char( "ANSI toggled.\n\r", ch );
        } else {
            send_to_char( "Unknown act flag. Common: freeze, silence, deny, ansi\n\r", ch );
        }
    } else if ( !str_cmp( arg1, "newbits" ) ) {
        if ( !str_cmp( arg2, "mastery" ) ) {
            TOGGLE_BIT( victim->newbits, NEW_MASTERY );
            send_to_char( "Mastery toggled.\n\r", ch );
        } else if ( !str_cmp( arg2, "darkness" ) ) {
            TOGGLE_BIT( victim->newbits, NEW_DARKNESS );
            send_to_char( "Darkness toggled.\n\r", ch );
        } else {
            send_to_char( "Unknown newbits flag. Common: mastery, darkness\n\r", ch );
        }
    } else {
        send_to_char( "Unknown flag type. Use: act, extra, newbits, affected\n\r", ch );
    }
}

/*
 * pedit immunity - Edit immunity flags
 */
void pedit_immunity( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];

    one_argument( argument, arg1 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> immunity <flagname>\n\r", ch );
        send_to_char( "Toggles immunity on/off.\n\r", ch );
        send_to_char( "Flags: slash, stab, smash, animal, charm, heat, cold, etc.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "slash" ) ) {
        TOGGLE_BIT( victim->immune, IMM_SLASH );
    } else if ( !str_cmp( arg1, "stab" ) ) {
        TOGGLE_BIT( victim->immune, IMM_STAB );
    } else if ( !str_cmp( arg1, "smash" ) ) {
        TOGGLE_BIT( victim->immune, IMM_SMASH );
    } else if ( !str_cmp( arg1, "charm" ) ) {
        TOGGLE_BIT( victim->immune, IMM_CHARM );
    } else if ( !str_cmp( arg1, "heat" ) ) {
        TOGGLE_BIT( victim->immune, IMM_HEAT );
    } else if ( !str_cmp( arg1, "cold" ) ) {
        TOGGLE_BIT( victim->immune, IMM_COLD );
    } else if ( !str_cmp( arg1, "sunlight" ) ) {
        TOGGLE_BIT( victim->immune, IMM_SUNLIGHT );
    } else if ( !str_cmp( arg1, "all" ) ) {
        /* Toggle all immunities */
        if ( victim->immune == 0 )
            victim->immune = 0xFFFFFFFF;
        else
            victim->immune = 0;
    } else {
        send_to_char( "Unknown immunity. Use: slash, stab, smash, charm, heat, cold, sunlight, all\n\r", ch );
        return;
    }

    send_to_char( "Immunity toggled.\n\r", ch );
}

/*
 * pedit strings - Edit string messages
 */
void pedit_strings( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg1 );
    strcpy( arg2, argument );

    if ( IS_NPC( victim ) || !victim->pcdata ) {
        send_to_char( "Not on NPCs.\n\r", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> strings <field> <text>\n\r", ch );
        send_to_char( "Fields: bamfin, bamfout, title\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "bamfin" ) ) {
        if ( victim->pcdata->bamfin )
            free(victim->pcdata->bamfin);
        victim->pcdata->bamfin = str_dup( arg2 );
        send_to_char( "Bamfin set.\n\r", ch );
    } else if ( !str_cmp( arg1, "bamfout" ) ) {
        if ( victim->pcdata->bamfout )
            free(victim->pcdata->bamfout);
        victim->pcdata->bamfout = str_dup( arg2 );
        send_to_char( "Bamfout set.\n\r", ch );
    } else if ( !str_cmp( arg1, "title" ) ) {
        if ( victim->pcdata->title )
            free(victim->pcdata->title);
        victim->pcdata->title = str_dup( arg2 );
        send_to_char( "Title set.\n\r", ch );
    } else {
        send_to_char( "Unknown string field. Use: bamfin, bamfout, title\n\r", ch );
    }
}

/*
 * pedit statistics - Edit kill/death statistics
 */
void pedit_statistics( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> statistics <field> <value>\n\r", ch );
        send_to_char( "Fields: pkill, pdeath, mkill, mdeath, awins, alosses\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Value must be a number.\n\r", ch );
        return;
    }
    value = atoi( arg2 );

    if ( !str_cmp( arg1, "pkill" ) ) {
        victim->pkill = value;
        send_to_char( "Player kills set.\n\r", ch );
    } else if ( !str_cmp( arg1, "pdeath" ) ) {
        victim->pdeath = value;
        send_to_char( "Player deaths set.\n\r", ch );
    } else if ( !str_cmp( arg1, "mkill" ) ) {
        victim->mkill = value;
        send_to_char( "Mob kills set.\n\r", ch );
    } else if ( !str_cmp( arg1, "mdeath" ) ) {
        victim->mdeath = value;
        send_to_char( "Mob deaths set.\n\r", ch );
    } else if ( !str_cmp( arg1, "awins" ) ) {
        if ( IS_NPC( victim ) || !victim->pcdata ) {
            send_to_char( "Not on NPCs.\n\r", ch );
            return;
        }
        victim->pcdata->awins = value;
        send_to_char( "Arena wins set.\n\r", ch );
    } else if ( !str_cmp( arg1, "alosses" ) ) {
        if ( IS_NPC( victim ) || !victim->pcdata ) {
            send_to_char( "Not on NPCs.\n\r", ch );
            return;
        }
        victim->pcdata->alosses = value;
        send_to_char( "Arena losses set.\n\r", ch );
    } else {
        send_to_char( "Unknown statistics field. Use: pkill, pdeath, mkill, mdeath, awins, alosses\n\r", ch );
    }
}

/*
 * pedit admin - Edit admin-level fields
 */
void pedit_admin( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( victim ) || !victim->pcdata ) {
        send_to_char( "Not on NPCs.\n\r", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> admin <field> <value>\n\r", ch );
        send_to_char( "Fields: trust, security, bounty\n\r", ch );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Value must be a number.\n\r", ch );
        return;
    }
    value = atoi( arg2 );

    if ( !str_cmp( arg1, "trust" ) ) {
        if ( value > ch->trust ) {
            send_to_char( "You can't set trust higher than your own.\n\r", ch );
            return;
        }
        victim->trust = value;
        send_to_char( "Trust set.\n\r", ch );
    } else if ( !str_cmp( arg1, "security" ) ) {
        if ( value > ch->pcdata->security ) {
            send_to_char( "You can't set security higher than your own.\n\r", ch );
            return;
        }
        victim->pcdata->security = value;
        send_to_char( "Security set.\n\r", ch );
    } else if ( !str_cmp( arg1, "bounty" ) ) {
        victim->pcdata->bounty = value;
        send_to_char( "Bounty set.\n\r", ch );
    } else {
        send_to_char( "Unknown admin field. Use: trust, security, bounty\n\r", ch );
    }
}

/*
 * pedit inventory - Manage equipment and inventory
 * Uses jope's pattern of temporarily borrowing the descriptor
 */
void pedit_inventory( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: pedit <player> inventory <subcommand> [args]\n\r", ch );
        send_to_char( "Subcommands:\n\r", ch );
        send_to_char( "  show       - Display carried items\n\r", ch );
        send_to_char( "  equipment  - Display equipped items\n\r", ch );
        send_to_char( "  wear <item> - Equip an item\n\r", ch );
        send_to_char( "  remove <item> - Unequip an item\n\r", ch );
        send_to_char( "  get <item> - Pick up an item (from room or container)\n\r", ch );
        send_to_char( "  drop <item> - Drop an item\n\r", ch );
        return;
    }

    /* Temporarily assign victim's descriptor for command execution */
    if ( ch->desc == NULL ) {
        send_to_char( "You have no descriptor.\n\r", ch );
        return;
    }

    /*
     * For offline players, we need to temporarily place them in the editor's room
     * so that get/drop commands work properly
     */
    if ( victim->in_room == NULL && ch->in_room != NULL ) {
        char_to_room( victim, ch->in_room );
    }

    ch->desc->connected = CON_PLAYING;
    victim->desc = ch->desc;

    if ( !str_cmp( arg1, "show" ) || !str_cmp( arg1, "inv" ) ) {
        do_inventory( victim, "" );
    } else if ( !str_cmp( arg1, "equipment" ) || !str_cmp( arg1, "eq" ) ) {
        do_equipment( victim, argument );
    } else if ( !str_cmp( arg1, "wear" ) ) {
        if ( argument[0] == '\0' ) {
            send_to_char( "Wear what?\n\r", ch );
        } else {
            do_wear( victim, argument );
        }
    } else if ( !str_cmp( arg1, "remove" ) ) {
        if ( argument[0] == '\0' ) {
            send_to_char( "Remove what?\n\r", ch );
        } else {
            do_remove( victim, argument );
        }
    } else if ( !str_cmp( arg1, "get" ) ) {
        if ( argument[0] == '\0' ) {
            send_to_char( "Get what?\n\r", ch );
        } else {
            do_get( victim, argument );
        }
    } else if ( !str_cmp( arg1, "drop" ) ) {
        if ( argument[0] == '\0' ) {
            send_to_char( "Drop what?\n\r", ch );
        } else {
            do_drop( victim, argument );
        }
    } else {
        send_to_char( "Unknown inventory subcommand.\n\r", ch );
        send_to_char( "Use: show, equipment, wear, remove, get, drop\n\r", ch );
    }

    /* Restore descriptor */
    victim->desc = NULL;
    ch->desc->connected = ( ch->pcdata->pfile == victim ) ? CON_PFILE : CON_PLAYING;
}

/*
 * pedit_interp - Interactive mode interpreter
 * Called from comm.c when connection state is CON_PFILE
 * Replaces jope_interp
 */
void pedit_interp( CHAR_DATA *ch, char *argument ) {
    char command[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int cmd;
    bool found = FALSE;

    while ( isspace( *argument ) )
        argument++;

    if ( argument[0] == '\0' )
        return;

    /* Get the victim we're editing */
    if ( ( victim = ch->pcdata->pfile ) == NULL ) {
        send_to_char( "You are not editing anyone.\n\r", ch );
        if ( ch->desc )
            ch->desc->connected = CON_PLAYING;
        return;
    }

    argument = one_argument( argument, command );

    /* Give prompt feedback */
    if ( ch->desc != NULL )
        write_to_buffer( ch->desc, "\n\r", 2 );

    /* Handle 'done' - save and exit */
    if ( !str_cmp( command, "done" ) ) {
        pedit_save_offline( ch );
        if ( ch->desc )
            ch->desc->connected = CON_PLAYING;
        send_to_char( "Player saved and edit mode exited.\n\r", ch );
        return;
    }

    /* Handle 'show' - display player data */
    if ( !str_cmp( command, "show" ) ) {
        pedit_show( ch, victim, argument );
        return;
    }

    /* Handle 'help' or '?' - list available commands */
    if ( !str_cmp( command, "help" ) || !str_cmp( command, "?" ) ) {
        send_to_char( "#7Available commands:#n\n\r", ch );
        send_to_char( "  done           - Save changes and exit edit mode\n\r", ch );
        send_to_char( "  show           - Display player data summary\n\r", ch );
        send_to_char( "  help or ?      - Show this help\n\r", ch );
        send_to_char( "\n\r#7Categories (use: <category> <field> <value>):#n\n\r", ch );
        for ( cmd = 0; pedit_categories[cmd].name[0] != '\0'; cmd++ ) {
            if ( pedit_categories[cmd].trust_level <= ch->level ) {
                snprintf( buf, sizeof( buf ), "  #3%-12s#n - %s\n\r",
                         pedit_categories[cmd].name,
                         pedit_categories[cmd].description );
                send_to_char( buf, ch );
            }
        }
        return;
    }

    /* Find and execute category handler */
    for ( cmd = 0; pedit_categories[cmd].name[0] != '\0'; cmd++ ) {
        if ( !str_prefix( command, pedit_categories[cmd].name ) ) {
            if ( pedit_categories[cmd].trust_level > ch->level ) {
                send_to_char( "You don't have permission for that category.\n\r", ch );
                found = TRUE;
                break;
            }
            found = TRUE;
            (*pedit_categories[cmd].handler)( ch, victim, argument );
            break;
        }
    }

    if ( !found ) {
        snprintf( buf, sizeof( buf ), "Unknown command '%s'. Type 'help' for available commands.\n\r", command );
        send_to_char( buf, ch );
    }
}
