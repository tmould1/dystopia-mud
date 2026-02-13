/*
 * ttype.h - Terminal Type / MTTS (MUD Terminal Type Standard) support
 *
 * Implements TTYPE negotiation (RFC 1091) with MTTS extensions.
 * Uses telnet option 24 (TELOPT_TTYPE) with subnegotiation.
 *
 * MTTS protocol uses three rounds of TTYPE negotiation:
 *   Round 1: Client sends terminal name (e.g., "MUDLET", "TINTIN++")
 *   Round 2: Client sends extended name (or same)
 *   Round 3: Client sends "MTTS <flags>" bitfield
 */

#ifndef TTYPE_H
#define TTYPE_H

/*
 * MTTS capability bitfield flags (sent in round 3 as "MTTS <number>")
 */
#define MTTS_ANSI            (1 << 0)  /*   1 - ANSI color support */
#define MTTS_VT100           (1 << 1)  /*   2 - VT100 interface */
#define MTTS_UTF8            (1 << 2)  /*   4 - UTF-8 character encoding */
#define MTTS_256_COLORS      (1 << 3)  /*   8 - Xterm 256-color support */
#define MTTS_MOUSE_TRACKING  (1 << 4)  /*  16 - Mouse tracking */
#define MTTS_OSC_COLOR       (1 << 5)  /*  32 - OSC color palette */
#define MTTS_SCREEN_READER   (1 << 6)  /*  64 - Screen reader active */
#define MTTS_PROXY           (1 << 7)  /* 128 - Proxy/relay connection */
#define MTTS_TRUECOLOR       (1 << 8)  /* 256 - True color (24-bit RGB) */
#define MTTS_MNES            (1 << 9)  /* 512 - MUD New-Environ Standard */
#define MTTS_MSLP            (1 << 10) /* 1024 - MUD Server Link Protocol */
#define MTTS_SSL             (1 << 11) /* 2048 - SSL/TLS secured connection */

/*
 * Telnet negotiation strings (defined in ttype.c)
 *
 * TTYPE protocol flow:
 * 1. Server sends: IAC DO TTYPE (request terminal type)
 * 2. Client sends: IAC WILL TTYPE (agrees to send)
 * 3. Server sends: IAC SB TTYPE SEND IAC SE (request info)
 * 4. Client sends: IAC SB TTYPE IS <terminal-type> IAC SE
 * 5. Repeat steps 3-4 for rounds 2 and 3
 */
extern const char ttype_do[];   /* IAC DO TTYPE - request terminal type */
extern const char ttype_dont[]; /* IAC DONT TTYPE - stop / copyover reset */
extern const char ttype_will[]; /* IAC WILL TTYPE - client agrees */
extern const char ttype_wont[]; /* IAC WONT TTYPE - client refuses */

/*
 * Function Prototypes
 */

/* Initialize TTYPE/MTTS defaults on a descriptor */
void ttype_init( DESCRIPTOR_DATA *d );

/* Send TTYPE SEND subnegotiation to request terminal info */
void ttype_request( DESCRIPTOR_DATA *d );

/* Handle incoming TTYPE subnegotiation response from client */
void ttype_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len );

/* Apply MTTS-detected capabilities to a player (upgrade-only, never downgrades) */
void ttype_apply_mtts( DESCRIPTOR_DATA *d, CHAR_DATA *ch );

#endif /* TTYPE_H */
