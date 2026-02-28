#ifndef NETWORK_H
#define NETWORK_H

/* Network/connection layer structures and constants */
/* Extracted from merc.h — Phase 3 struct decomposition */

#include "types.h"

/*
 * Site ban structure.
 */
struct ban_data {
	list_node_t node;
	char *name;
	char *reason;
};
/*
 * threaded status - Jobo
 */
#define STATUS_LOOKUP 0 // New Descriptor, in lookup pr. default.
#define STATUS_DONE	  1 // The lookup is done.
#define STATUS_WAIT	  2 // Closed while in thread.
#define STATUS_CLOSED 3 // Closed, ready to be recycled.
/*
 * Connected state for a channel.
 */
#define CON_PLAYING				 0
#define CON_GET_NAME			 1
#define CON_GET_OLD_PASSWORD	 2
#define CON_CONFIRM_NEW_NAME	 3
#define CON_GET_NEW_PASSWORD	 4
#define CON_CONFIRM_NEW_PASSWORD 5
#define CON_GET_NEW_SEX			 6
#define CON_GET_NEW_CLASS		 7
#define CON_GET_NEW_VT102		 8
#define CON_GET_NEW_ANSI		 9
#define CON_READ_MOTD			 10
#define CON_NOT_PLAYING			 11
#define CON_EDITING				 12
#define CON_COPYOVER_RECOVER	 13
/* Values 14-18 reserved for CON_NOTE_* in board.h */
#define CON_PFILE			 20
#define CON_GET_NEW_GMCP	 21
#define CON_GET_NEW_MXP		 22
#define CON_GET_NEW_EXPLEVEL 23
#define CON_DETECT_CAPS		 24
/*
 * DNS reverse-lookup task, passed to a worker thread.
 */
struct dns_lookup {
	list_node_t node;
	DESCRIPTOR_DATA *d;
	char *buf;
	int status;
};

/*
 * Descriptor (channel) structure.
 */
struct descriptor_data {
	list_node_t node;
	DESCRIPTOR_DATA *snoop_by;
	CHAR_DATA *character;
	CHAR_DATA *original;
	char *host;
	int descriptor;
	int connected;
	int lookup_status;
	bool fcommand;
	bool vt102;
	char inbuf[4 * MAX_INPUT_LENGTH];
	char incomm[MAX_INPUT_LENGTH];
	char inlast[MAX_INPUT_LENGTH];
	int repeat;
	char *showstr_head;	 /* From ENVY code to compile */
	char *showstr_point; /* From ENVY code to compile */
	char *outbuf;
	int outsize;
	int outtop;
	void *pEdit;	/* OLC */
	char **pString; /* OLC */
	int editor;		/* OLC */
	/* mccp: support data */
	z_stream *out_compress;
	unsigned char *out_compress_buf;
	int mccp_version; /* 0=none, 1=v1, 2=v2 */
	/* gmcp: support data */
	bool gmcp_enabled; /* GMCP negotiation successful */
	int gmcp_packages; /* Bitmask of supported packages */
	/* mxp: MUD eXtension Protocol support */
	bool mxp_enabled; /* MXP negotiation successful */
	/* naws: window size support (RFC 1073) */
	bool naws_enabled; /* NAWS negotiation successful */
	int client_width;  /* Terminal width in columns */
	int client_height; /* Terminal height in rows */
	/* ttype: Terminal Type / MTTS support (RFC 1091) */
	bool ttype_enabled; /* TTYPE negotiation successful */
	int  ttype_round;   /* Current TTYPE round (1-3) */
	int  mtts_flags;    /* MTTS capability bitfield */
	char client_name[64];    /* Terminal/client name from round 1 */
	char terminal_type[64];  /* Standard terminal name from round 2 */
	/* charset: client encoding support (RFC 2066) */
	int  client_charset;     /* CHARSET_UNKNOWN=0, CHARSET_ASCII=1, CHARSET_UTF8=2 */
	bool charset_negotiated; /* TRUE once charset is determined */
	/* intro: capability detection timing */
	int  intro_pulse;        /* Pulses elapsed since connection (for CON_DETECT_CAPS) */
};

#endif /* NETWORK_H */
