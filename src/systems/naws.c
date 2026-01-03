/*
 * naws.c - Negotiate About Window Size (RFC 1073) support
 *
 * NAWS enables the MUD to know the client's terminal dimensions.
 * Uses telnet option 31 (TELOPT_NAWS) with subnegotiation.
 *
 * Protocol: IAC SB NAWS <width-hi> <width-lo> <height-hi> <height-lo> IAC SE
 *
 * The client sends window size as two 16-bit unsigned integers in
 * network byte order (big-endian).
 */

#include "merc.h"
#if !defined(WIN32)
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include "telnet.h"
#include "naws.h"

/*
 * Telnet negotiation strings for NAWS
 *
 * NAWS is client-to-server (client sends size), so server uses DO to request.
 * Server sends: DO NAWS (request), Client sends: WILL NAWS + subnegotiation
 * We also handle WONT (client refuses) and DONT (for copyover reset).
 */
#if defined(WIN32)
const char naws_do[]   = { (char)IAC, (char)DO, (char)TELOPT_NAWS, '\0' };
const char naws_dont[] = { (char)IAC, (char)DONT, (char)TELOPT_NAWS, '\0' };
const char naws_will[] = { (char)IAC, (char)WILL, (char)TELOPT_NAWS, '\0' };
const char naws_wont[] = { (char)IAC, (char)WONT, (char)TELOPT_NAWS, '\0' };
#else
const char naws_do[]   = { IAC, DO, TELOPT_NAWS, '\0' };
const char naws_dont[] = { IAC, DONT, TELOPT_NAWS, '\0' };
const char naws_will[] = { IAC, WILL, TELOPT_NAWS, '\0' };
const char naws_wont[] = { IAC, WONT, TELOPT_NAWS, '\0' };
#endif

/* Width limits for banner display */
#define NAWS_MIN_WIDTH  60
#define NAWS_MAX_WIDTH  120

/*
 * Initialize NAWS defaults on a descriptor
 */
void naws_init(DESCRIPTOR_DATA *d)
{
    if (d == NULL)
        return;

    d->naws_enabled = FALSE;
    d->client_width = NAWS_DEFAULT_WIDTH;
    d->client_height = NAWS_DEFAULT_HEIGHT;
}

/*
 * Handle incoming NAWS subnegotiation from client
 * Data format: <width-hi> <width-lo> <height-hi> <height-lo>
 *
 * Note: IAC bytes (255) in the data are escaped as IAC IAC per RFC 854.
 * We need to unescape before processing.
 */
void naws_handle_subnegotiation(DESCRIPTOR_DATA *d, unsigned char *data, int len)
{
    unsigned short width, height;
    unsigned char unescaped[4];
    int src, dst;

    if (d == NULL || data == NULL || len < 4)
        return;

    /* Unescape IAC bytes: IAC IAC -> IAC */
    for (src = 0, dst = 0; src < len && dst < 4; src++, dst++) {
        unescaped[dst] = data[src];
        /* Skip second IAC if doubled */
        if (data[src] == IAC && src + 1 < len && data[src + 1] == IAC) {
            src++;
        }
    }

    if (dst != 4)
        return;

    /* Unpack from big-endian 16-bit values */
    width = (unescaped[0] << 8) | unescaped[1];
    height = (unescaped[2] << 8) | unescaped[3];

    /* Store values, using defaults if zero */
    d->client_width = width ? width : NAWS_DEFAULT_WIDTH;
    d->client_height = height ? height : NAWS_DEFAULT_HEIGHT;
    d->naws_enabled = TRUE;
}

/*
 * Get effective terminal width for display
 * Clamps to reasonable range (60-120) for banner generation
 * Returns 80 if no descriptor or NAWS not enabled
 */
int naws_get_width(CHAR_DATA *ch)
{
    int width;

    if (ch == NULL || ch->desc == NULL || !ch->desc->naws_enabled)
        return NAWS_DEFAULT_WIDTH;

    width = ch->desc->client_width;

    /* Clamp to reasonable range */
    if (width < NAWS_MIN_WIDTH)
        width = NAWS_MIN_WIDTH;
    else if (width > NAWS_MAX_WIDTH)
        width = NAWS_MAX_WIDTH;

    return width;
}
