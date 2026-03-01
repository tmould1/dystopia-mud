/*
 * mxp.h - MUD eXtension Protocol definitions
 *
 * MXP enables rich text features for compatible MUD clients including
 * clickable links, colors, and formatted text. This implementation
 * provides basic telnet negotiation and mode switching.
 *
 * Reference: http://www.zuggsoft.com/zmud/mxp.htm
 */

#ifndef MXP_H
#define MXP_H

/*
 * MXP Mode Line Tags (sent at start of lines)
 * Format: ESC [ <mode> z
 *
 * These escape sequences switch the MXP parser mode for the current line.
 */
#define MXP_OPEN_LINE	"\033[0z" /* Open line - all tags OK */
#define MXP_SECURE_LINE "\033[1z" /* Secure line - MUD tags only */
#define MXP_LOCKED_LINE "\033[2z" /* Locked line - no tags interpreted */
#define MXP_RESET		"\033[3z" /* Reset to default mode */
#define MXP_TEMP_SECURE "\033[4z" /* Temp secure - resets at newline */
#define MXP_LOCK_OPEN	"\033[5z" /* Lock in open mode */
#define MXP_LOCK_SECURE "\033[6z" /* Lock in secure mode */
#define MXP_LOCK_LOCKED "\033[7z" /* Lock in locked mode */

/*
 * MXP Common Elements
 * These are the most useful MXP tags for a MUD
 */
#define MXP_SEND_START "<SEND>"	 /* Clickable text start */
#define MXP_SEND_END   "</SEND>" /* Clickable text end */
#define MXP_A_END	   "</A>"	 /* End anchor/link */

/*
 * MXP Buffer sizes
 */
#define MXP_TAG_MAX_LEN 256	 /* Max length of a single MXP tag */
#define MXP_BUF_MAX_LEN 16384 /* Max length for MXP formatted output (needs to hold MAX_STRING_LENGTH + MXP markup) */

/*
 * Function prototypes
 */
bool mxpStart ( DESCRIPTOR_DATA * desc );
void mxpEnd ( DESCRIPTOR_DATA * desc );

/* MXP formatting helpers */
char *mxp_obj_link ( OBJ_DATA * obj, CHAR_DATA *ch, char *display_text, bool in_room, int count );
char *mxp_equip_link ( OBJ_DATA * obj, CHAR_DATA *ch, char *display_text );
char *mxp_container_item_link ( OBJ_DATA * obj, OBJ_DATA *container, CHAR_DATA *ch, char *display_text );
char *mxp_player_link ( CHAR_DATA * victim, CHAR_DATA *ch, char *display_text );
char *mxp_exit_link ( EXIT_DATA * pexit, int door, CHAR_DATA *ch, char *display_text );
char *mxp_char_link ( CHAR_DATA * victim, CHAR_DATA *ch, char *display_text );
char *mxp_aura_tag ( CHAR_DATA * ch, const char *prefix, const char *tooltip, int sn );
void mxp_escape_string ( char *dest, const char *src, size_t maxlen );

/* Player command */
void do_mxp ( CHAR_DATA * ch, char *argument );

#endif /* MXP_H */
