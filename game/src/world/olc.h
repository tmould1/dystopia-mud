/***************************************************************************
 *  File: olc.h                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/
/*
 * This is a header file for all the OLC files.  Feel free to copy it into
 * merc.h if you wish.  Many of these routines may be handy elsewhere in
 * the code.  -Jason Dinkel
 */

/*
 * The version info.  Please use this info when reporting bugs.
 * It is displayed in the game by typing 'version' while editing.
 * Do not remove these from the code - by request of Jason Dinkel
 */
#define VERSION "ILAB Online Creation [Beta 1.1]"
#define AUTHOR	"     By Jason(jdinkel@mines.colorado.edu)"
#define DATE	"     (May. 15, 1995)"
#define CREDITS "     Original by Surreality(cxw197@psu.edu) and Locke(locke@lm.com)"

/*
 * New typedefs.
 */
typedef bool OLC_FUN ( CHAR_DATA * ch, char *argument );

/*
 * Connected states for editor.
 */
#define ED_AREA	  1
#define ED_ROOM	  2
#define ED_OBJECT 3
#define ED_MOBILE 4

/*
 * Interpreter Prototypes
 */
void aedit ( CHAR_DATA * ch, char *argument );
void redit ( CHAR_DATA * ch, char *argument );
void medit ( CHAR_DATA * ch, char *argument );
void oedit ( CHAR_DATA * ch, char *argument );

/*
 * OLC Constants
 */
#define MAX_MOB 1 /* Default maximum number for resetting mobs */

/*
 * Structure for an OLC editor command.
 */
struct olc_cmd_type {
	char *const name;
	OLC_FUN *olc_fun;
};

/*
 * Structure for an OLC editor startup command.
 */
struct editor_cmd_type {
	char *const name;
	DO_FUN *do_fun;
};

/*
 * Utils.
 */
AREA_DATA *get_vnum_area ( int vnum );
AREA_DATA *get_area_data ( int vnum );
int flag_value ( const struct flag_type *flag_table,
	char *argument );
char *flag_string ( const struct flag_type *flag_table,
	int bits );
void add_reset ( ROOM_INDEX_DATA * room,
	RESET_DATA *pReset, int index );

/*
 * Interpreter Table Prototypes
 */
extern const struct olc_cmd_type hedit_table[];
extern const struct olc_cmd_type aedit_table[];
extern const struct olc_cmd_type redit_table[];
extern const struct olc_cmd_type oedit_table[];
extern const struct olc_cmd_type medit_table[];

/*
 * General Functions
 */
bool show_commands ( CHAR_DATA * ch, char *argument );
bool show_help ( CHAR_DATA * ch, char *argument );
bool edit_done ( CHAR_DATA * ch );
bool show_version ( CHAR_DATA * ch, char *argument );

/* help editor */
#define ED_HELP 6
void hedit ( CHAR_DATA * ch, char *argument );
extern const struct olc_cmd_type hedit_table[];
DO_FUN do_hedit;
OLC_FUN hedit_create;
OLC_FUN hedit_level;
OLC_FUN hedit_keyword;
OLC_FUN hedit_text;
OLC_FUN hedit_index;
OLC_FUN hedit_change;
OLC_FUN hedit_delete;
#define EDIT_HELP( Ch, Help ) ( Help = (HELP_DATA *) Ch->desc->pEdit )
HELP_DATA *new_help_data (void);

/*
 * Save Prototypes (olc_save.c)
 */
void save_area ( AREA_DATA *pArea );

/*
 * Area Editor Prototypes
 */
OLC_FUN aedit_show;
OLC_FUN aedit_create;
OLC_FUN aedit_name;
OLC_FUN aedit_file;
OLC_FUN aedit_hidden;
OLC_FUN aedit_age;
OLC_FUN aedit_recall;
OLC_FUN aedit_reset;
OLC_FUN aedit_security;
OLC_FUN aedit_builder;
OLC_FUN aedit_vnum;
OLC_FUN aedit_lvnum;
OLC_FUN aedit_uvnum;

/*
 * Room Editor Prototypes
 */
OLC_FUN redit_show;
OLC_FUN redit_create;
OLC_FUN redit_name;
OLC_FUN redit_desc;
OLC_FUN redit_ed;
OLC_FUN redit_format;
OLC_FUN redit_north;
OLC_FUN redit_south;
OLC_FUN redit_east;
OLC_FUN redit_west;
OLC_FUN redit_up;
OLC_FUN redit_down;
OLC_FUN redit_move;
OLC_FUN redit_mreset;
OLC_FUN redit_oreset;
OLC_FUN redit_mlist;
OLC_FUN redit_olist;
OLC_FUN redit_mshow;
OLC_FUN redit_oshow;

/*
 * Object Editor Prototypes
 */
OLC_FUN oedit_show;
OLC_FUN oedit_create;
OLC_FUN oedit_name;
OLC_FUN oedit_short;
OLC_FUN oedit_long;
OLC_FUN oedit_addaffect;
OLC_FUN oedit_delaffect;
OLC_FUN oedit_value0;
OLC_FUN oedit_value1;
OLC_FUN oedit_value2;
OLC_FUN oedit_value3;
OLC_FUN oedit_weight;
OLC_FUN oedit_cost;
OLC_FUN oedit_ed;

/*
 * Mobile Editor Prototypes
 */
OLC_FUN medit_show;
OLC_FUN medit_create;
OLC_FUN medit_name;
OLC_FUN medit_short;
OLC_FUN medit_long;
OLC_FUN medit_shop;
OLC_FUN medit_desc;
OLC_FUN medit_level;
OLC_FUN medit_align;
OLC_FUN medit_spec;
OLC_FUN medit_value0;
OLC_FUN medit_value1;
OLC_FUN medit_value2;

/*
 * Macros
 */
#define IS_BUILDER( ch, Area ) ( ( ch->pcdata->security >= Area->security || strstr( Area->builders, ch->name ) || strstr( Area->builders, "All" ) ) && !IS_EXTRA( ch, EXTRA_OSWITCH ) && !IS_HEAD( ch, LOST_HEAD ) && !IS_EXTRA( ch, EXTRA_SWITCH ) )

/* TOGGLE_BIT is now defined in merc.h */

/* Return pointers to what is being edited. */
#define EDIT_MOB( Ch, Mob )	  ( Mob = (MOB_INDEX_DATA *) Ch->desc->pEdit )
#define EDIT_OBJ( Ch, Obj )	  ( Obj = (OBJ_INDEX_DATA *) Ch->desc->pEdit )
#define EDIT_ROOM( Ch, Room ) ( Room = Ch->in_room )
#define EDIT_AREA( Ch, Area ) ( Area = (AREA_DATA *) Ch->desc->pEdit )

/*
 * Prototypes
 */
/* mem.c - memory prototypes. */
#define ED EXTRA_DESCR_DATA
RESET_DATA *new_reset_data (void);
void free_reset_data ( RESET_DATA * pReset );
AREA_DATA *new_area (void);
void free_area ( AREA_DATA * pArea );
EXIT_DATA *new_exit (void);
void free_exit ( EXIT_DATA * pExit );
ED *new_extra_descr (void);
void free_extra_descr ( ED * pExtra );
ROOM_INDEX_DATA *new_room_index (void);
void free_room_index ( ROOM_INDEX_DATA * pRoom );
AFFECT_DATA *new_affect (void);
void free_affect ( AFFECT_DATA * pAf );
SHOP_DATA *new_shop (void);
void free_shop ( SHOP_DATA * pShop );
OBJ_INDEX_DATA *new_obj_index (void);
void free_obj_index ( OBJ_INDEX_DATA * pObj );
MOB_INDEX_DATA *new_mob_index (void);
void free_mob_index ( MOB_INDEX_DATA * pMob );
#undef ED
