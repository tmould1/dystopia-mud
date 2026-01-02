/*
 * gmcp.h - Generic MUD Communication Protocol support
 *
 * GMCP enables structured JSON data exchange with modern MUD clients.
 * Uses telnet option 201 with subnegotiation.
 *
 * Protocol: IAC SB GMCP <package.name> <JSON data> IAC SE
 */

#ifndef GMCP_H
#define GMCP_H

/*
 * GMCP Package Flags
 * Bitmask of packages the client has requested via Core.Supports.Set
 */
#define GMCP_PACKAGE_CORE       (1 << 0)    /* Core.* - basic handshake */
#define GMCP_PACKAGE_CHAR       (1 << 1)    /* Char.* - character info */
#define GMCP_PACKAGE_CHAR_VITALS (1 << 2)   /* Char.Vitals - hp/mana/move */
#define GMCP_PACKAGE_CHAR_STATUS (1 << 3)   /* Char.Status - level/class */
#define GMCP_PACKAGE_CHAR_INFO  (1 << 4)    /* Char.Info - name/race */

/*
 * Telnet negotiation strings (defined in gmcp.c)
 */
extern const char gmcp_will[];  /* IAC WILL GMCP - offer support */
extern const char gmcp_do[];    /* IAC DO GMCP - client accepts */
extern const char gmcp_dont[];  /* IAC DONT GMCP - client refuses */

/*
 * Function Prototypes
 */

/* Initialize GMCP on a descriptor after client sends IAC DO GMCP */
void gmcp_init(DESCRIPTOR_DATA *d);

/* Send a raw GMCP message: IAC SB GMCP <package> <data> IAC SE */
void gmcp_send(DESCRIPTOR_DATA *d, const char *package, const char *data);

/* Send Char.Vitals with current/max hp, mana, move */
void gmcp_send_vitals(CHAR_DATA *ch);

/* Send Char.Status with level, class, position, experience */
void gmcp_send_status(CHAR_DATA *ch);

/* Send Char.Info with name, race, guild/clan */
void gmcp_send_info(CHAR_DATA *ch);

/* Handle incoming GMCP subnegotiation from client */
void gmcp_handle_subnegotiation(DESCRIPTOR_DATA *d, unsigned char *data, int len);

/* Send all character data (called on login) */
void gmcp_send_char_data(CHAR_DATA *ch);

#endif /* GMCP_H */
