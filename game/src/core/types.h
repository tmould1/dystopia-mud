#ifndef TYPES_H
#define TYPES_H

/* Forward type declarations and function pointer typedefs */
/* Extracted from merc.h — Phase 3 struct decomposition */

/*
 * Structure types.
 */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct ban_data BAN_DATA;
typedef struct char_data CHAR_DATA;
typedef struct alias_data ALIAS_DATA;

typedef struct top_board TOP_BOARD;
typedef struct l_board LEADER_BOARD;

typedef struct editor_data EDITOR_DATA;

typedef struct dns_lookup DNS_LOOKUP;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct kill_data KILL_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct war_data WAR_DATA;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;

typedef struct disabled_data DISABLED_DATA;

/* one disabled command */
struct disabled_data {
	list_node_t node;
	struct cmd_type const *command; /* pointer to the command struct*/
	char *disabled_by;				/* name of disabler */
	int level;					/* level of disabler */
};
extern list_head_t disabled_list;

/*
 * Function types.
 */
typedef void DO_FUN ( CHAR_DATA * ch, char *argument );
typedef bool SPEC_FUN ( CHAR_DATA * ch );
typedef void SPELL_FUN ( int sn, int level, CHAR_DATA *ch, void *vo );

/*
 * Utility macros.
 */
static inline int UMIN( int a, int b ) { return a < b ? a : b; }
static inline int UMAX( int a, int b ) { return a > b ? a : b; }
static inline int URANGE( int a, int b, int c ) { return b < a ? a : ( b > c ? c : b ); }
#define IS_SET( flag, bit )	   ( ( flag ) & ( bit ) )
#define SET_BIT( var, bit )	   ( ( var ) |= ( bit ) )
#define REMOVE_BIT( var, bit ) ( ( var ) &= ~( bit ) )
#define TOGGLE_BIT( var, bit ) ( ( var ) ^= ( bit ) )

#endif /* TYPES_H */
