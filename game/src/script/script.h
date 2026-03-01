#ifndef SCRIPT_H
#define SCRIPT_H

#include "merc.h"

/*
 * Trigger types for mob/obj/room scripts.
 * Mob and room triggers use separate bit ranges so a single script
 * can only fire on one entity type.
 */

/* Mob triggers */
#define TRIG_GREET          (1 << 0)    /* Player enters room with this mob */
#define TRIG_SPEECH         (1 << 1)    /* Player says something near mob */

/* Script data — one Lua script attached to a mob/obj/room template.
 * Stored in an intrusive list on the owning index structure. */
typedef struct script_data {
	char         *name;       /* Builder-friendly name */
	uint32_t      trigger;    /* Trigger type bitmask */
	char         *code;       /* Lua source code */
	char         *pattern;    /* Pattern match for SPEECH (NULL = any) */
	int           chance;     /* Percent chance to fire (0 = always) */
	list_node_t   node;       /* Intrusive list linkage */
} SCRIPT_DATA;

/* Lifecycle — call from boot_db() and shutdown */
void script_init( void );
void script_shutdown( void );

/* Trigger dispatch — mob scripts (iterate NPCs in room) */
void script_trigger_greet( CHAR_DATA *ch, ROOM_INDEX_DATA *room );
void script_trigger_speech( CHAR_DATA *ch, const char *text );

/* Trigger dispatch — room scripts (iterate room's own scripts) */
void script_trigger_room_enter( CHAR_DATA *ch, ROOM_INDEX_DATA *room );
void script_trigger_room_speech( CHAR_DATA *ch, const char *text );

#endif /* SCRIPT_H */
