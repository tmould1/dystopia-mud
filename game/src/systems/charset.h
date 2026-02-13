/*
 * charset.h - Character Set negotiation (RFC 2066) support
 *
 * Detects whether a client supports UTF-8 via TELOPT_CHARSET.
 * Uses telnet option 42 with subnegotiation.
 *
 * Protocol:
 * 1. Server sends: IAC WILL CHARSET (offer to negotiate)
 * 2. Client sends: IAC DO CHARSET (accept) or IAC DONT CHARSET (refuse)
 * 3. Server sends: IAC SB CHARSET REQUEST <sep> UTF-8 IAC SE
 * 4. Client sends: IAC SB CHARSET ACCEPTED UTF-8 IAC SE
 *    or:           IAC SB CHARSET REJECTED IAC SE
 */

#ifndef CHARSET_H
#define CHARSET_H

/* Client charset capability states */
#define CHARSET_UNKNOWN  0   /* Not yet negotiated */
#define CHARSET_ASCII    1   /* Client does not support UTF-8 */
#define CHARSET_UTF8     2   /* Client supports UTF-8 */

/* RFC 2066 subnegotiation opcodes */
#define CHARSET_REQUEST  1
#define CHARSET_ACCEPTED 2
#define CHARSET_REJECTED 3

/*
 * Telnet negotiation strings (defined in charset.c)
 *
 * CHARSET protocol flow:
 * 1. Server sends: IAC WILL CHARSET (offer to negotiate charset)
 * 2. Client sends: IAC DO CHARSET (agrees to negotiate)
 * 3. Server sends: IAC SB CHARSET REQUEST <space> UTF-8 IAC SE
 * 4. Client sends: IAC SB CHARSET ACCEPTED UTF-8 IAC SE
 *    or:           IAC SB CHARSET REJECTED IAC SE
 */
extern const char charset_will[];  /* IAC WILL CHARSET - offer to negotiate */
extern const char charset_wont[];  /* IAC WONT CHARSET - withdraw offer */
extern const char charset_do[];    /* IAC DO CHARSET - client accepts negotiation */
extern const char charset_dont[];  /* IAC DONT CHARSET - client refuses */

/*
 * Function Prototypes
 */

/* Initialize charset defaults on a descriptor */
void charset_init( DESCRIPTOR_DATA *d );

/* Send charset request subnegotiation (called when client sends DO CHARSET) */
void charset_send_request( DESCRIPTOR_DATA *d );

/* Handle incoming CHARSET subnegotiation from client */
void charset_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len );

/* Finalize charset as ASCII if still unknown (call after first user input) */
void charset_finalize( DESCRIPTOR_DATA *d );

/* Check if descriptor supports UTF-8 */
bool charset_is_utf8( DESCRIPTOR_DATA *d );

/* Transliterate UTF-8 to ASCII in-place for non-UTF-8 clients.
 * Replaces each multi-byte UTF-8 sequence with '?'.
 * Returns new length of the string. */
int charset_transliterate( char *buf, int len );

#endif /* CHARSET_H */
