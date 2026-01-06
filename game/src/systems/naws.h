/*
 * naws.h - Negotiate About Window Size (RFC 1073) support
 *
 * NAWS enables the MUD to know the client's terminal dimensions.
 * Uses telnet option 31 with subnegotiation.
 *
 * Protocol: IAC SB NAWS <width-hi> <width-lo> <height-hi> <height-lo> IAC SE
 */

#ifndef NAWS_H
#define NAWS_H

/*
 * Telnet negotiation strings (defined in naws.c)
 *
 * NAWS protocol flow:
 * 1. Server sends: IAC DO NAWS (request window size)
 * 2. Client sends: IAC WILL NAWS (agrees to send)
 * 3. Client sends: IAC SB NAWS <width-hi> <width-lo> <height-hi> <height-lo> IAC SE
 * 4. Client re-sends subnegotiation whenever window is resized
 */
extern const char naws_do[];    /* IAC DO NAWS - request window size (server sends) */
extern const char naws_dont[];  /* IAC DONT NAWS - stop sending size (server sends) */
extern const char naws_will[];  /* IAC WILL NAWS - client agrees to send size */
extern const char naws_wont[];  /* IAC WONT NAWS - client refuses / for copyover */

/*
 * Function Prototypes
 */

/* Initialize NAWS defaults on a descriptor */
void naws_init(DESCRIPTOR_DATA *d);

/* Handle incoming NAWS subnegotiation from client */
void naws_handle_subnegotiation(DESCRIPTOR_DATA *d, unsigned char *data, int len);

/* Get effective terminal width for display (clamped 60-120, default 80) */
int naws_get_width(CHAR_DATA *ch);

#endif /* NAWS_H */
