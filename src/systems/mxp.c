/*
 * mxp.c - MUD eXtension Protocol implementation
 *
 * Provides MXP support for rich text features in compatible clients.
 * Follows the MCCP pattern for protocol negotiation.
 */

#include "../core/merc.h"
#include "telnet.h"
#include "mxp.h"

/* External function from comm.c */
bool write_to_descriptor args( ( DESCRIPTOR_DATA *d, char *txt, int length ) );

/*
 * Telnet negotiation sequences for MXP
 * These are extern'd and used in comm.c for protocol negotiation.
 */
#if defined(unix)
const char mxp_will[] = { IAC, WILL, TELOPT_MXP, '\0' };
const char mxp_do[]   = { IAC, DO, TELOPT_MXP, '\0' };
const char mxp_dont[] = { IAC, DONT, TELOPT_MXP, '\0' };
const char mxp_sb[]   = { IAC, SB, TELOPT_MXP, IAC, SE, '\0' };
#endif

#if defined(WIN32)
const char mxp_will[] = { (char)IAC, (char)WILL, (char)TELOPT_MXP, '\0' };
const char mxp_do[]   = { (char)IAC, (char)DO, (char)TELOPT_MXP, '\0' };
const char mxp_dont[] = { (char)IAC, (char)DONT, (char)TELOPT_MXP, '\0' };
const char mxp_sb[]   = { (char)IAC, (char)SB, (char)TELOPT_MXP, (char)IAC, (char)SE, '\0' };
#endif

#if defined(macintosh) || defined(MSDOS)
const char mxp_will[] = { '\0' };
const char mxp_do[]   = { '\0' };
const char mxp_dont[] = { '\0' };
const char mxp_sb[]   = { '\0' };
#endif

/*
 * Start MXP mode on a descriptor
 * Called when client responds with IAC DO MXP
 */
bool mxpStart(DESCRIPTOR_DATA *desc)
{
    if (desc == NULL)
        return FALSE;

    if (desc->mxp_enabled)
        return TRUE;  /* Already enabled */

    /* Send subnegotiation to begin MXP mode */
    write_to_descriptor(desc, (char *)mxp_sb, strlen(mxp_sb));

    /* Set default to locked mode for security */
    write_to_descriptor(desc, MXP_LOCKED_LINE, strlen(MXP_LOCKED_LINE));

    desc->mxp_enabled = TRUE;
    return TRUE;
}

/*
 * End MXP mode on a descriptor
 * Called when client sends IAC DONT MXP or on disconnect
 */
void mxpEnd(DESCRIPTOR_DATA *desc)
{
    if (desc == NULL)
        return;

    desc->mxp_enabled = FALSE;
}

/*
 * User command to check MXP status
 */
void do_mxp(CHAR_DATA *ch, char *argument)
{
    if (ch == NULL)
        return;

    if (ch->desc == NULL)
    {
        send_to_char("You have no descriptor!\n\r", ch);
        return;
    }

    if (ch->desc->mxp_enabled)
    {
        send_to_char("MXP is enabled for your connection.\n\r", ch);
        send_to_char("Your client supports the MUD eXtension Protocol.\n\r", ch);
    }
    else
    {
        send_to_char("MXP is not enabled for your connection.\n\r", ch);
        send_to_char("Your client either doesn't support MXP or declined the negotiation.\n\r", ch);
    }
}
