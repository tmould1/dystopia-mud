/*
 * telnet.h - Telnet protocol definitions
 *
 * Defines telnet command bytes and option codes per RFC 854 and extensions.
 * Used for MUD client communication, compression (MCCP), and terminal control.
 */

#ifndef TELNET_H
#define TELNET_H

/*
 * Telnet Command Bytes (RFC 854)
 * These are the core protocol commands sent after IAC (255)
 */
#define IAC		255 /* Interpret As Command - escape byte */
#define DONT	254 /* Refuse to perform option */
#define DO		253 /* Request other side to perform option */
#define WONT	252 /* Refuse to perform option */
#define WILL	251 /* Agree to perform option */
#define SB		250 /* Subnegotiation Begin */
#define GA		249 /* Go Ahead - half-duplex signal */
#define EL		248 /* Erase Line */
#define EC		247 /* Erase Character */
#define AYT		246 /* Are You There */
#define AO		245 /* Abort Output */
#define IP		244 /* Interrupt Process */
#define BREAK	243 /* Break */
#define DM		242 /* Data Mark - for Synch */
#define NOP		241 /* No Operation */
#define SE		240 /* Subnegotiation End */
#define EOR		239 /* End of Record (RFC 885) */
#define ABORT	238 /* Abort Process */
#define SUSP	237 /* Suspend Process */
#define EOF_CMD 236 /* End of File */

#define SYNCH 242 /* Telnet Synch operation */

/*
 * Standard Telnet Options (RFC 855+)
 * Option codes negotiated via WILL/WONT/DO/DONT
 */
#define TELOPT_BINARY		  0	 /* Binary Transmission (RFC 856) */
#define TELOPT_ECHO			  1	 /* Echo (RFC 857) */
#define TELOPT_RCP			  2	 /* Reconnection */
#define TELOPT_SGA			  3	 /* Suppress Go Ahead (RFC 858) */
#define TELOPT_NAMS			  4	 /* Approx Message Size Negotiation */
#define TELOPT_STATUS		  5	 /* Status (RFC 859) */
#define TELOPT_TM			  6	 /* Timing Mark (RFC 860) */
#define TELOPT_RCTE			  7	 /* Remote Controlled Trans and Echo */
#define TELOPT_NAOL			  8	 /* Output Line Width */
#define TELOPT_NAOP			  9	 /* Output Page Size */
#define TELOPT_NAOCRD		  10 /* Output Carriage-Return Disposition */
#define TELOPT_NAOHTS		  11 /* Output Horizontal Tab Stops */
#define TELOPT_NAOHTD		  12 /* Output Horizontal Tab Disposition */
#define TELOPT_NAOFFD		  13 /* Output Formfeed Disposition */
#define TELOPT_NAOVTS		  14 /* Output Vertical Tabstops */
#define TELOPT_NAOVTD		  15 /* Output Vertical Tab Disposition */
#define TELOPT_NAOLFD		  16 /* Output Linefeed Disposition */
#define TELOPT_XASCII		  17 /* Extended ASCII */
#define TELOPT_LOGOUT		  18 /* Logout (RFC 727) */
#define TELOPT_BM			  19 /* Byte Macro */
#define TELOPT_DET			  20 /* Data Entry Terminal */
#define TELOPT_SUPDUP		  21 /* SUPDUP */
#define TELOPT_SUPDUPOUTPUT	  22 /* SUPDUP Output */
#define TELOPT_SNDLOC		  23 /* Send Location */
#define TELOPT_TTYPE		  24 /* Terminal Type (RFC 1091) */
#define TELOPT_EOR			  25 /* End of Record (RFC 885) */
#define TELOPT_TUID			  26 /* TACACS User Identification */
#define TELOPT_OUTMRK		  27 /* Output Marking */
#define TELOPT_TTYLOC		  28 /* Terminal Location Number */
#define TELOPT_3270REGIME	  29 /* 3270 Regime */
#define TELOPT_X3PAD		  30 /* X.3 PAD */
#define TELOPT_NAWS			  31 /* Window Size (RFC 1073) */
#define TELOPT_TSPEED		  32 /* Terminal Speed */
#define TELOPT_LFLOW		  33 /* Remote Flow Control */
#define TELOPT_LINEMODE		  34 /* Linemode (RFC 1184) */
#define TELOPT_XDISPLOC		  35 /* X Display Location */
#define TELOPT_OLD_ENVIRON	  36 /* Environment Option (old) */
#define TELOPT_AUTHENTICATION 37 /* Authentication (RFC 2941) */
#define TELOPT_ENCRYPT		  38 /* Encryption (RFC 2946) */
#define TELOPT_NEW_ENVIRON	  39 /* New Environment Option (RFC 1572) */
#define TELOPT_TN3270E		  40 /* TN3270E (RFC 2355) */
#define TELOPT_CHARSET		  42 /* Character Set (RFC 2066) */
#define TELOPT_COMPORT		  44 /* Com Port Control (RFC 2217) */

/*
 * MUD-Specific Telnet Options
 * Extensions commonly used by MUD clients
 */
#define TELOPT_MSSP		 70	 /* MUD Server Status Protocol */
#define TELOPT_COMPRESS	 85	 /* MCCP v1 - MUD Client Compression */
#define TELOPT_COMPRESS2 86	 /* MCCP v2 - Preferred compression */
#define TELOPT_MXP		 91	 /* MUD eXtension Protocol */
#define TELOPT_ZMP		 93	 /* Zenith MUD Protocol */
#define TELOPT_ATCP		 200 /* Achaea Telnet Client Protocol */
#define TELOPT_GMCP		 201 /* Generic MUD Communication Protocol */

#define TELOPT_EXOPL 255 /* Extended Options List (RFC 861) */

/*
 * Terminal Type Subnegotiation (RFC 1091)
 */
#define TELQUAL_IS	 0 /* Option IS */
#define TELQUAL_SEND 1 /* Send option */
#define TELQUAL_INFO 2 /* ENVIRON: informational version */

/*
 * MSSP (MUD Server Status Protocol) Variables
 * Common variable names for server status reporting
 */
#define MSSP_VAR 1 /* Variable name follows */
#define MSSP_VAL 2 /* Variable value follows */

/*
 * Linemode Subnegotiation (RFC 1184)
 */
#define LM_MODE		   1 /* Mode */
#define LM_FORWARDMASK 2 /* Forward Mask */
#define LM_SLC		   3 /* Set Local Characters */

/* Linemode mode flags */
#define MODE_EDIT	  0x01 /* Edit mode */
#define MODE_TRAPSIG  0x02 /* Trap signals */
#define MODE_ACK	  0x04 /* Acknowledge */
#define MODE_SOFT_TAB 0x08 /* Soft tab */
#define MODE_LIT_ECHO 0x10 /* Literal echo */

/*
 * NAWS (Negotiate About Window Size) defaults
 * Format: IAC SB NAWS <width-hi> <width-lo> <height-hi> <height-lo> IAC SE
 */
#define NAWS_DEFAULT_WIDTH	80
#define NAWS_DEFAULT_HEIGHT 24

/*
 * Common Control Characters
 */
#define CHAR_NULL 0	  /* NUL */
#define CHAR_BEL  7	  /* Bell */
#define CHAR_BS	  8	  /* Backspace */
#define CHAR_HT	  9	  /* Horizontal Tab */
#define CHAR_LF	  10  /* Line Feed */
#define CHAR_VT	  11  /* Vertical Tab */
#define CHAR_FF	  12  /* Form Feed */
#define CHAR_CR	  13  /* Carriage Return */
#define CHAR_ESC  27  /* Escape */
#define CHAR_DEL  127 /* Delete */

#endif /* TELNET_H */
