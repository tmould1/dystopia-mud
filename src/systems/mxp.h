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
#define MXP_OPEN_LINE       "\033[0z"   /* Open line - all tags OK */
#define MXP_SECURE_LINE     "\033[1z"   /* Secure line - MUD tags only */
#define MXP_LOCKED_LINE     "\033[2z"   /* Locked line - no tags interpreted */
#define MXP_RESET           "\033[3z"   /* Reset to default mode */
#define MXP_TEMP_SECURE     "\033[4z"   /* Temp secure - resets at newline */
#define MXP_LOCK_OPEN       "\033[5z"   /* Lock in open mode */
#define MXP_LOCK_SECURE     "\033[6z"   /* Lock in secure mode */
#define MXP_LOCK_LOCKED     "\033[7z"   /* Lock in locked mode */

/*
 * MXP Common Elements
 * These are the most useful MXP tags for a MUD
 */
#define MXP_SEND_START      "<SEND>"        /* Clickable text start */
#define MXP_SEND_END        "</SEND>"       /* Clickable text end */
#define MXP_A_END           "</A>"          /* End anchor/link */

/*
 * MXP Buffer sizes
 */
#define MXP_TAG_MAX_LEN     256     /* Max length of a single MXP tag */

/*
 * Function prototypes
 */
bool mxpStart           args( ( DESCRIPTOR_DATA *desc ) );
void mxpEnd             args( ( DESCRIPTOR_DATA *desc ) );

/* Player command */
void do_mxp             args( ( CHAR_DATA *ch, char *argument ) );

#endif /* MXP_H */
