/***************************************************************************
 *  quest_new.h - Quest system engine
 *
 *  Provides the quest command, progress checking, event hooks, and
 *  quest state management.  Quest definitions are loaded from quest.db
 *  via db_quest.h; this file handles the gameplay logic.
 ***************************************************************************/

#ifndef QUEST_NEW_H
#define QUEST_NEW_H

#include "../core/merc.h"
#include "../db/db_quest.h"

/*--------------------------------------------------------------------------
 * Command: do_quest
 *
 * Player-facing quest command with subcommands:
 *   quest           - Show active quests
 *   quest list [cat] - Show available quests
 *   quest accept <id> - Accept a quest
 *   quest progress   - Show detailed progress
 *   quest complete   - Turn in completed quest
 *   quest abandon <id> - Abandon a quest
 *   quest path       - Show main progression path
 *   quest history    - Show completed quests
 *--------------------------------------------------------------------------*/

void do_quest( CHAR_DATA *ch, char *argument );

/*--------------------------------------------------------------------------
 * Quest System Initialization
 *
 * Called during player login after quest_tracker is allocated.
 * Evaluates all quests and unlocks any that meet prerequisites.
 *--------------------------------------------------------------------------*/

void quest_init_player( CHAR_DATA *ch );

/* Grant the entry quest (T01/T06/M01) to a player who missed auto-grant.
 * Returns TRUE if a quest was activated, FALSE if already active/completed. */
bool quest_grant_entry( CHAR_DATA *ch );

/*--------------------------------------------------------------------------
 * Event Hook: quest_check_progress
 *
 * Called from various game systems when an event occurs that might
 * advance a quest objective.  Checks all active quests for matching
 * objectives and updates progress.
 *
 * Parameters:
 *   ch    - The character whose progress is being checked
 *   type  - Objective type string (QOBJ_* constants from db_quest.h)
 *   target - Target string (command name, stat type, "any", etc.)
 *   value  - The current value or increment amount
 *--------------------------------------------------------------------------*/

void quest_check_progress( CHAR_DATA *ch, const char *type,
                           const char *target, int value );

/*--------------------------------------------------------------------------
 * Objective Evaluation Helpers
 *
 * These check if a specific objective is met by examining the character's
 * current state, rather than relying on event hooks.  Used for REACH_*
 * type objectives during periodic checks.
 *--------------------------------------------------------------------------*/

/* Check all REACH_* objectives for a character (call from tick/update). */
void quest_check_milestones( CHAR_DATA *ch );

/* Evaluate prerequisites and unlock available quests. */
void quest_evaluate_availability( CHAR_DATA *ch );

/*--------------------------------------------------------------------------
 * Admin Command: do_qadmin
 *
 * Immortal-only quest administration:
 *   qadmin <player> info [quest_id]  - Show quest state for player
 *   qadmin <player> set <id> <status> - Set quest status (0-4)
 *   qadmin <player> complete <id>     - Instantly complete + award quest
 *   qadmin <player> reset [id|all]    - Reset quest(s) to locked
 *   qadmin <player> obj <id> <n> <val> - Set objective progress
 *   qadmin list                       - List all quest definitions
 *--------------------------------------------------------------------------*/

void do_qadmin( CHAR_DATA *ch, char *argument );

#endif /* QUEST_NEW_H */
