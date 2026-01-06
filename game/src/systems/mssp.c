/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*
 * mssp.c - MUD Server Status Protocol support
 *
 * MSSP allows MUD listing sites to automatically query game statistics
 * via telnet option negotiation (RFC 2217 extended).
 *
 * Protocol flow:
 *   Server sends: IAC WILL MSSP
 *   Client sends: IAC DO MSSP
 *   Server sends: IAC SB MSSP <variables> IAC SE
 */

#include "merc.h"
#include "telnet.h"
#include <stdio.h>
#include <string.h>

/* External references */
extern DESCRIPTOR_DATA *descriptor_list;
extern GAMECONFIG_DATA game_config;
extern time_t current_time;
extern time_t boot_time;

/* MSSP telnet negotiation sequences */
#if defined(WIN32)
const char mssp_will[] = { (char)IAC, (char)WILL, (char)TELOPT_MSSP, '\0' };
const char mssp_wont[] = { (char)IAC, (char)WONT, (char)TELOPT_MSSP, '\0' };
const char mssp_do[]   = { (char)IAC, (char)DO, (char)TELOPT_MSSP, '\0' };
#else
const char mssp_will[] = { IAC, WILL, TELOPT_MSSP, '\0' };
const char mssp_wont[] = { IAC, WONT, TELOPT_MSSP, '\0' };
const char mssp_do[]   = { IAC, DO, TELOPT_MSSP, '\0' };
#endif

/* Forward declarations */
bool write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length);

/*
 * Count players currently connected and playing
 */
static int mssp_count_players(void)
{
    DESCRIPTOR_DATA *d;
    int count = 0;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character != NULL)
            count++;
    }

    return count;
}

/*
 * Add an MSSP variable to the buffer
 * Format: MSSP_VAR <name> MSSP_VAL <value>
 */
static int mssp_add_var(unsigned char *buf, int pos, const char *var, const char *val)
{
    buf[pos++] = MSSP_VAR;
    while (*var)
        buf[pos++] = *var++;
    buf[pos++] = MSSP_VAL;
    while (*val)
        buf[pos++] = *val++;
    return pos;
}

/*
 * Send MSSP status data to the client
 * Called when client responds with IAC DO MSSP
 */
void mssp_send(DESCRIPTOR_DATA *d)
{
    unsigned char buf[4096];
    char numbuf[32];
    int pos = 0;

    /* Start subnegotiation: IAC SB MSSP */
    buf[pos++] = IAC;
    buf[pos++] = SB;
    buf[pos++] = TELOPT_MSSP;

    /* Game name */
    pos = mssp_add_var(buf, pos, "NAME", game_config.game_name);

    /* Player count */
    sprintf(numbuf, "%d", mssp_count_players());
    pos = mssp_add_var(buf, pos, "PLAYERS", numbuf);

    /* Uptime in seconds */
    sprintf(numbuf, "%ld", (long)(current_time - boot_time));
    pos = mssp_add_var(buf, pos, "UPTIME", numbuf);

    /* Port */
    pos = mssp_add_var(buf, pos, "PORT", "8888");

    /* Codebase lineage */
    pos = mssp_add_var(buf, pos, "CODEBASE", "Merc/Diku/Dystopia");

    /* Feature flags */
    pos = mssp_add_var(buf, pos, "ANSI", "1");
    pos = mssp_add_var(buf, pos, "MCCP", "1");

    /* End subnegotiation: IAC SE */
    buf[pos++] = IAC;
    buf[pos++] = SE;

    /* Send directly to descriptor */
    write_to_descriptor(d, (char *)buf, pos);
}
