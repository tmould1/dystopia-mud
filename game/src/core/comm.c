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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include <sys/types.h>
#if !defined( WIN32 )
#include <sys/time.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#if !defined( WIN32 )
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> // we need these to call wait()
#endif

#include "merc.h"
#include "../db/db_game.h"
#include "../systems/profile.h"

#if defined( WIN32 )
#include <sys/timeb.h> /*for _ftime(), uses _timeb struct*/
#endif

extern GAMECONFIG_DATA game_config;

#include <signal.h>
#if !defined( WIN32 )
#include <unistd.h>
#include <sys/resource.h> /* for RLIMIT_NOFILE */
#endif

/*
 * Socket and TCP/IP stuff.
 */
#if !defined( WIN32 )
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h> /* TCP_NODELAY */
#include <sys/socket.h>
#include <arpa/telnet.h>
#include "telnet.h" /* Our extensions: MCCP, GMCP, NAWS, etc. */
const char echo_off_str[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const char echo_on_str[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const char go_ahead_str[] = { IAC, GA, '\0' };
/* MCCP v1 */
const char compress_will[] = { IAC, WILL, TELOPT_COMPRESS, '\0' };
const char compress_wont[] = { IAC, WONT, TELOPT_COMPRESS, '\0' };
const char compress_do[] = { IAC, DO, TELOPT_COMPRESS, '\0' };
const char compress_dont[] = { IAC, DONT, TELOPT_COMPRESS, '\0' };
/* MCCP v2 */
const char compress2_will[] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
const char compress2_wont[] = { IAC, WONT, TELOPT_COMPRESS2, '\0' };
const char compress2_do[] = { IAC, DO, TELOPT_COMPRESS2, '\0' };
const char compress2_dont[] = { IAC, DONT, TELOPT_COMPRESS2, '\0' };
/* MSSP */
extern const char mssp_will[];
extern const char mssp_wont[];
extern const char mssp_do[];
/* MXP */
extern const char mxp_will[];
extern const char mxp_wont[];
extern const char mxp_do[];
extern const char mxp_dont[];
/* GMCP */
extern const char gmcp_will[];
extern const char gmcp_wont[];
extern const char gmcp_do[];
extern const char gmcp_dont[];
/* NAWS */
extern const char naws_do[];
extern const char naws_dont[];
extern const char naws_will[];
extern const char naws_wont[];

void show_string args( ( DESCRIPTOR_DATA * d, char *input ) );

#endif

#if defined( WIN32 )
#include "telnet.h"

const char echo_off_str[] = { (char) IAC, (char) WILL, (char) TELOPT_ECHO, '\0' };
const char echo_on_str[] = { (char) IAC, (char) WONT, (char) TELOPT_ECHO, '\0' };
const char go_ahead_str[] = { (char) IAC, (char) GA, '\0' };
/* MCCP v1 */
const char compress_will[] = { (char) IAC, (char) WILL, (char) TELOPT_COMPRESS, '\0' };
const char compress_wont[] = { (char) IAC, (char) WONT, (char) TELOPT_COMPRESS, '\0' };
const char compress_do[] = { (char) IAC, (char) DO, (char) TELOPT_COMPRESS, '\0' };
const char compress_dont[] = { (char) IAC, (char) DONT, (char) TELOPT_COMPRESS, '\0' };
/* MCCP v2 */
const char compress2_will[] = { (char) IAC, (char) WILL, (char) TELOPT_COMPRESS2, '\0' };
const char compress2_wont[] = { (char) IAC, (char) WONT, (char) TELOPT_COMPRESS2, '\0' };
const char compress2_do[] = { (char) IAC, (char) DO, (char) TELOPT_COMPRESS2, '\0' };
const char compress2_dont[] = { (char) IAC, (char) DONT, (char) TELOPT_COMPRESS2, '\0' };
/* MSSP */
extern const char mssp_will[];
extern const char mssp_wont[];
extern const char mssp_do[];
/* MXP */
extern const char mxp_will[];
extern const char mxp_wont[];
extern const char mxp_do[];
extern const char mxp_dont[];
/* GMCP */
extern const char gmcp_will[];
extern const char gmcp_wont[];
extern const char gmcp_do[];
extern const char gmcp_dont[];
/* NAWS */
extern const char naws_do[];
extern const char naws_dont[];
extern const char naws_will[];
extern const char naws_wont[];

void show_string args( ( DESCRIPTOR_DATA * d, char *input ) );
#endif

/*
 * Global variables.
 */
DESCRIPTOR_DATA *descriptor_free; /* Free list for descriptors	*/
DESCRIPTOR_DATA *descriptor_list; /* All open descriptors		*/
DESCRIPTOR_DATA *d_next;		  /* Next descriptor in loop	*/
FILE *fpReserve;				  /* Reserved file handle		*/
bool god;						  /* All new chars are gods!	*/
bool merc_down;					  /* Shutdown			*/
bool wizlock;					  /* Game is wizlocked		*/
char str_boot_time[MAX_INPUT_LENGTH];
char crypt_pwd[MAX_INPUT_LENGTH];
time_t current_time; /* Time of this pulse		*/
time_t boot_time;	 /* Time of server boot		*/
int arena;

/* Colour scale - returns # color code based on current/max ratio */
#define NUM_SCALE_CODES 4

const char *col_scale_code( int current, int max ) {
	static const char *codes[NUM_SCALE_CODES] = { "#R", "#L", "#G", "#y" };
	if ( current < 1 ) return "#R";
	if ( current >= max ) return "#C";
	return codes[( NUM_SCALE_CODES * current ) / ( max > 0 ? max : 1 )];
}

void game_loop args( ( int control ) );
int init_socket args( ( int port ) );
void new_descriptor args( ( int control ) );
bool read_from_descriptor args( ( DESCRIPTOR_DATA * d ) );
bool write_to_descriptor args( ( DESCRIPTOR_DATA * d, char *txt, int length ) );
bool write_to_descriptor_2 args( ( int desc, char *txt, int length ) );

/*
 * Other local functions.
 */
bool check_reconnect args( ( DESCRIPTOR_DATA * d, char *name, bool fConn ) );
bool check_kickoff args( ( DESCRIPTOR_DATA * d, char *name, bool fConn ) );
bool check_playing args( ( DESCRIPTOR_DATA * d, char *name ) );
int main args( ( int argc, char **argv ) );
void nanny args( ( DESCRIPTOR_DATA * d, char *argument ) );
bool process_output args( ( DESCRIPTOR_DATA * d, bool fPrompt ) );
void read_from_buffer args( ( DESCRIPTOR_DATA * d ) );
void stop_idling args( ( CHAR_DATA * ch ) );
void bust_a_prompt args( ( DESCRIPTOR_DATA * d ) );
void bust_a_header args( ( DESCRIPTOR_DATA * d ) );

void lookup_address args( ( DUMMY_ARG * dummyarg ) ); // Only threaded calls, please.
bool check_banned args( ( DESCRIPTOR_DATA * dnew ) ); // Ban check

#if defined( WIN32 )
/*
 * Promote dystopia_new.exe to dystopia.exe after hot-reload.
 * When we're running as dystopia_new.exe (from a copyover with a fresh build),
 * rename ourselves to dystopia.exe now that the old process has exited.
 */
void promote_new_executable( const char *argv0 ) {
	char new_path[MUD_PATH_MAX];
	char old_path[MUD_PATH_MAX];

	/* Check if we're running as dystopia_new.exe */
	if ( strstr( argv0, "_new" ) == NULL )
		return; /* Not a shadow binary, nothing to do */

	snprintf( old_path, sizeof( old_path ), "%s", EXE_FILE );
	snprintf( new_path, sizeof( new_path ), "%s", EXE_FILE_NEW );

	/* Delete old executable (now unlocked since old process exited) */
	if ( remove( old_path ) == 0 ) {
		log_string( "promote_new_executable: removed old dystopia.exe" );
	}

	/* Rename ourselves to the standard name */
	if ( rename( new_path, old_path ) == 0 ) {
		log_string( "promote_new_executable: promoted dystopia_new.exe to dystopia.exe" );
	} else {
		log_string( "promote_new_executable: rename failed (this is normal if running from IDE)" );
	}
}
#endif

int proc_pid;
int port, control;

int main( int argc, char **argv ) {
	struct timeval now_time;
	bool fCopyOver = FALSE;

#if defined( WIN32 )
	/* Windows buffers stderr by default - disable for immediate log output */
	setvbuf( stderr, NULL, _IONBF, 0 );
	setvbuf( stdout, NULL, _IONBF, 0 );
	/* Initialize mutex for memory allocation (Windows CRITICAL_SECTION needs runtime init) */
	{
		extern pthread_mutex_t memory_mutex;
		InitializeCriticalSection( &memory_mutex );
	}
#else
	/* Initialize memory mutex as recursive to prevent deadlock in crash handler.
	 * If a crash occurs while holding the mutex, the signal handler needs to
	 * be able to allocate memory for crash logging without deadlocking. */
	{
		extern pthread_mutex_t memory_mutex;
		pthread_mutexattr_t attr;
		pthread_mutexattr_init( &attr );
		pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
		pthread_mutex_init( &memory_mutex, &attr );
		pthread_mutexattr_destroy( &attr );
	}
#endif

	/*
	 * Initialize paths based on executable location.
	 * This allows the MUD to be run from any directory.
	 */
	mud_init_paths( argv[0] );

#if defined( WIN32 )
	/*
	 * If we're running as dystopia_new.exe (from a copyover with fresh build),
	 * promote ourselves to dystopia.exe now that the old process has exited.
	 */
	promote_new_executable( argv[0] );
#endif

	/*
	 * Memory debugging if needed.
	 */
#if defined( MALLOC_DEBUG )
	malloc_debug( 2 );
#endif

#ifdef RLIMIT_NOFILE
#ifndef min
#define min( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif
	{
		struct rlimit rlp;
		(void) getrlimit( RLIMIT_NOFILE, &rlp );
		rlp.rlim_cur = min( rlp.rlim_max, FD_SETSIZE );
		(void) setrlimit( RLIMIT_NOFILE, &rlp );
	}
#endif

	/*
	 * Crash recovery by Mandrax
	 */

	signal( SIGSEGV, crashrecov );

	proc_pid = getpid();

	/*
	 * Init time and encryption.
	 */
	gettimeofday( &now_time, NULL );
	current_time = (time_t) now_time.tv_sec;
	boot_time = current_time;
	strcpy( str_boot_time, ctime( &current_time ) );
	sprintf( crypt_pwd, "Don't bother." );

	/*
	 * Reserve one channel for our use.
	 */
	if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL ) {
		perror( NULL_FILE );
		exit( 1 );
	}

	/*
	 * Get the port number.
	 */
	port = 8888;
	if ( argc > 1 ) {
		if ( !is_number( argv[1] ) ) {
			fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
			exit( 1 );
		} else if ( ( port = atoi( argv[1] ) ) <= 1024 ) {
			fprintf( stderr, "Port number must be above 1024.\n" );
			exit( 1 );
		}
	}

	if ( argc > 2 && argv[2] && argv[2][0] ) {
		fCopyOver = TRUE;
#if defined( WIN32 )
		/*
		 * On Windows, socket handles don't inherit across _spawnl/_execl.
		 * We used WSADuplicateSocket to serialize the socket state to a file,
		 * and now we need to recreate it using WSASocket.
		 */
		if ( strcmp( argv[3], "wsasocket" ) == 0 ) {
			WSAPROTOCOL_INFOW proto_info;
			FILE *socket_fp;
			WORD wVersionRequested = MAKEWORD( 1, 1 );
			WSADATA wsaData;

			/* Initialize Winsock first */
			if ( WSAStartup( wVersionRequested, &wsaData ) != 0 ) {
				perror( "Copyover recovery: WSAStartup failed" );
				exit( 1 );
			}

			/* Read the protocol info from file */
			socket_fp = fopen( COPYOVER_SOCKET_FILE, "rb" );
			if ( !socket_fp ) {
				perror( "Copyover recovery: fopen socket file" );
				exit( 1 );
			}
			if ( fread( &proto_info, sizeof( proto_info ), 1, socket_fp ) != 1 ) {
				perror( "Copyover recovery: fread socket file" );
				fclose( socket_fp );
				exit( 1 );
			}
			fclose( socket_fp );

			/* NOTE: Don't delete the socket file here!
			 * copyover_recover() in boot_db() still needs to read
			 * the client socket entries. It will delete the file
			 * after reading all entries.
			 */

			/* Recreate the socket using the duplicated info */
			control = (int) WSASocketW( AF_INET, SOCK_STREAM, IPPROTO_TCP,
				&proto_info, 0, 0 );
			if ( control == INVALID_SOCKET ) {
				fprintf( stderr, "Copyover recovery: WSASocket failed: %d\n",
					WSAGetLastError() );
				exit( 1 );
			}
		} else {
			/* Fallback for old-style copyover (shouldn't happen on Windows) */
			control = atoi( argv[3] );
		}
#else
		control = atoi( argv[3] );
#endif
	}

	else
		fCopyOver = FALSE;
	/*
	 * Run the game.
	 */
	if ( !fCopyOver ) /* We have already the port if copyover'ed */
		control = init_socket( port );
	boot_db( fCopyOver );

	arena = FIGHT_OPEN;
	sprintf( log_buf, "%s is ready to rock on port %d.", game_config.game_name, port );
	log_string( log_buf );
	game_loop( control );
#if !defined( WIN32 )
	close( control );
#else
	closesocket( control );
	WSACleanup();
#endif

	log_string( "Normal termination of game." );
	exit( 0 );
	return 0;
}

int init_socket( int port ) {
	static struct sockaddr_in sa_zero;
	struct sockaddr_in sa;
	int x = 1;
	int fd;

#if !defined( WIN32 )
	if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		perror( "Init_socket: socket" );
		exit( 1 );
	}
#else
	WORD wVersionRequested = MAKEWORD( 1, 1 );
	WSADATA wsaData;
	int err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		perror( "No useable WINSOCK.DLL" );
		exit( 1 );
	}

	if ( ( fd = (int) socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		perror( "Init_socket: socket" );
		exit( 1 );
	}
#endif

	if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &x, sizeof( x ) ) < 0 ) {
		perror( "Init_socket: SO_REUSEADDR" );
#if defined( WIN32 )
		_close( fd );
#else
		close( fd );
#endif
		exit( 1 );
	}

#if defined( SO_DONTLINGER ) && !defined( SYSV )
	{
		struct linger ld;

		ld.l_onoff = 1;
		ld.l_linger = 1000;

		if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
				 (char *) &ld, sizeof( ld ) ) < 0 ) {
			perror( "Init_socket: SO_DONTLINGER" );
#if !defined( WIN32 )
			close( fd );
#else
			closesocket( fd );
#endif
			exit( 1 );
		}
	}
#endif

	sa = sa_zero;
#if !defined( WIN32 )
	sa.sin_family = AF_INET;
#else
	sa.sin_family = PF_INET;
#endif
	sa.sin_port = htons( port );

	if ( bind( fd, (struct sockaddr *) &sa, sizeof( sa ) ) < 0 ) {
		perror( "Init_socket: bind" );
#if !defined( WIN32 )
		close( fd );
#else
		closesocket( fd );
#endif
		exit( 1 );
	}

	if ( listen( fd, 3 ) < 0 ) {
		perror( "Init_socket: listen" );
#if !defined( WIN32 )
		close( fd );
#else
		closesocket( fd );
#endif
		exit( 1 );
	}

	return fd;
}

void excessive_cpu( int blx ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	for ( vch = char_list; vch != NULL; vch = vch_next ) {
		vch_next = vch->next;

		if ( !IS_NPC( vch ) ) {
			send_to_char( "Mud frozen: Autosave and quit.  The mud will reboot in 2 seconds.\n\r", vch );
			interpret( vch, "quit" );
		}
	}
	exit( 1 );
}

void game_loop( int control ) {
	static struct timeval null_time;
	struct timeval last_time;

#if !defined( WIN32 )
	signal( SIGPIPE, SIG_IGN );
#endif
	gettimeofday( &last_time, NULL );
	current_time = (time_t) last_time.tv_sec;

	/* Main loop */
	while ( !merc_down ) {
		fd_set in_set;
		fd_set out_set;
		fd_set exc_set;
		DESCRIPTOR_DATA *d;
		int maxdesc;

#if defined( MALLOC_DEBUG )
		if ( malloc_verify() != 1 )
			abort();
#endif

		PROFILE_START("game_loop_work");

		/*
		 * Poll all active descriptors.
		 */
		FD_ZERO( &in_set );
		FD_ZERO( &out_set );
		FD_ZERO( &exc_set );
		FD_SET( control, &in_set );
		maxdesc = control;
		/* kavirpoint
			maxdesc	= control * 2;
		*/
		for ( d = descriptor_list; d != NULL; d = d->next ) {
			maxdesc = UMAX( maxdesc, d->descriptor );
			FD_SET( d->descriptor, &in_set );
			FD_SET( d->descriptor, &out_set );
			FD_SET( d->descriptor, &exc_set );
		}

		if ( select( maxdesc + 1, &in_set, &out_set, &exc_set, &null_time ) < 0 ) {
			perror( "Game_loop: select: poll" );
			exit( 1 );
		}

		/*
		 * New connection?
		 */
		if ( FD_ISSET( control, &in_set ) )
			new_descriptor( control );

		/*
		 * Kick out the freaky folks.
		 */
		for ( d = descriptor_list; d != NULL; d = d_next ) {
			d_next = d->next;
			if ( FD_ISSET( d->descriptor, &exc_set ) ) {
				FD_CLR( d->descriptor, &in_set );
				FD_CLR( d->descriptor, &out_set );
				if ( d->character )
					save_char_obj( d->character );
				d->outtop = 0;
				close_socket( d );
			}
		}

		/*
		 * Process input.
		 */
		for ( d = descriptor_list; d != NULL; d = d_next ) {
			d_next = d->next;
			d->fcommand = FALSE;

			if ( FD_ISSET( d->descriptor, &in_set ) ) {
				if ( d->character != NULL )
					d->character->timer = 0;
				if ( !read_from_descriptor( d ) ) {
					FD_CLR( d->descriptor, &out_set );
					if ( d->character != NULL )
						save_char_obj( d->character );
					d->outtop = 0;
					close_socket( d );
					continue;
				}
			}

			if ( d->character != NULL && d->character->wait > 0 ) {
				--d->character->wait;
				continue;
			}

			read_from_buffer( d );
			if ( d->incomm[0] != '\0' ) {
				d->fcommand = TRUE;
				stop_idling( d->character );

				/* OLC */
				if ( d->showstr_point )
					show_string( d, d->incomm );
				else if ( d->pString )
					string_add( d->character, d->incomm );
				else
					switch ( d->connected ) {
					default:
						nanny( d, d->incomm );
						break;
					case CON_PLAYING:
						if ( !run_olc_editor( d ) )
							interpret( d->character, d->incomm );
						break;
					case CON_EDITING:
						edit_buffer( d->character, d->incomm );
						break;
					case CON_PFILE:
						jope_interp( d->character, d->incomm );
						break;
					}

				/* Flush output immediately so prompts appear before next input */
				if ( d->outtop > 0 ) {
					/* Send GA (Go Ahead) signal for clients that need it */
					if ( d->character && IS_SET( d->character->act, PLR_TELNET_GA ) )
						write_to_buffer( d, go_ahead_str, 0 );
					process_output( d, FALSE );
					if ( d->out_compress )
						processCompressed( d );
				}

				d->incomm[0] = '\0';
			}
		}

		/*
		 * Autonomous game motion.
		 */
		update_handler();

		/*
		 * Output.
		 */
		for ( d = descriptor_list; d != NULL; d = d_next ) {
			d_next = d->next;

			if ( ( d->fcommand || d->outtop > 0 ) && FD_ISSET( d->descriptor, &out_set ) ) {
				if ( !process_output( d, TRUE ) ) {
					if ( d->character != NULL )
						save_char_obj( d->character );
					d->outtop = 0;
					close_socket( d );
				}
			}
		}

		PROFILE_END("game_loop_work");

		/*
		 * Synchronize to a clock.
		 * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
		 * Careful here of signed versus unsigned arithmetic.
		 */
#if !defined( WIN32 )
		{
			struct timeval now_time;
			long secDelta;
			long usecDelta;
			int effective_pps = PULSE_PER_SECOND * ( profile_stats.tick_multiplier > 0 ? profile_stats.tick_multiplier : 1 );

			gettimeofday( &now_time, NULL );
			usecDelta = ( (int) last_time.tv_usec ) - ( (int) now_time.tv_usec ) + 1000000 / effective_pps;
			secDelta = ( (int) last_time.tv_sec ) - ( (int) now_time.tv_sec );
			while ( usecDelta < 0 ) {
				usecDelta += 1000000;
				secDelta -= 1;
			}

			while ( usecDelta >= 1000000 ) {
				usecDelta -= 1000000;
				secDelta += 1;
			}

			if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) ) {
				struct timeval stall_time;

				stall_time.tv_usec = usecDelta;
				stall_time.tv_sec = secDelta;
				if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 ) {
					perror( "Game_loop: select: stall" );
					exit( 1 );
				}
			}
		}
#else
		{
			struct _timeb start_time;
			struct _timeb end_time;
			int effective_pps = PULSE_PER_SECOND * ( profile_stats.tick_multiplier > 0 ? profile_stats.tick_multiplier : 1 );
			int pulse_ms = 1000 / effective_pps;
			int elapsed_ms;

			_ftime( &start_time );

			for ( ;; ) {
				_ftime( &end_time );

				/* Calculate elapsed time in milliseconds */
				elapsed_ms = (int) ( ( end_time.time - start_time.time ) * 1000 + ( end_time.millitm - start_time.millitm ) );

				if ( elapsed_ms >= pulse_ms )
					break;

				Sleep( pulse_ms - elapsed_ms );
				break;
			}
		}
#endif

		gettimeofday( &last_time, NULL );
		current_time = (time_t) last_time.tv_sec;
	}

	return;
}

void init_descriptor( DESCRIPTOR_DATA *dnew, int desc ) {
	static DESCRIPTOR_DATA d_zero;

	*dnew = d_zero;
	dnew->descriptor = desc;
	dnew->character = NULL;
	dnew->connected = CON_GET_NAME;
	dnew->lookup_status = STATUS_LOOKUP;
	dnew->showstr_head = str_dup( "" );
	dnew->showstr_point = 0;
	dnew->pEdit = NULL;	  /* OLC */
	dnew->pString = NULL; /* OLC */
	dnew->editor = 0;	  /* OLC */
	dnew->outsize = 2000;
	dnew->outbuf = alloc_mem( dnew->outsize );
	dnew->mxp_enabled = FALSE;
	/* NAWS defaults */
	dnew->naws_enabled = FALSE;
	dnew->client_width = NAWS_DEFAULT_WIDTH;
	dnew->client_height = NAWS_DEFAULT_HEIGHT;
}

void new_descriptor( int control ) {
	char buf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *dnew;
	struct sockaddr_in sock;
	int desc;
	socklen_t size;
	pthread_attr_t attr;
	pthread_t thread_lookup;
	DUMMY_ARG *dummyarg;
	bool DOS_ATTACK = FALSE;

	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

	/* New Dummy Argument */
	if ( dummy_free == NULL ) {
		dummyarg = alloc_perm( sizeof( *dummyarg ) );
	} else {
		dummyarg = dummy_free;
		dummy_free = dummy_free->next;
	}
	dummyarg->status = 1;
	dummyarg->next = dummy_list;
	dummy_list = dummyarg;

	size = sizeof( sock );
	getsockname( control, (struct sockaddr *) &sock, &size );
	if ( ( desc = (int) accept( control, (struct sockaddr *) &sock, &size ) ) < 0 ) {
		perror( "New_descriptor: accept" );
		return;
	}

#if !defined( FNDELAY )
#define FNDELAY O_NDELAY
#endif

#if !defined( WIN32 )
	if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 ) {
		perror( "New_descriptor: fcntl: FNDELAY" );
		return;
	}
#else
	/* Set non-blocking mode on Windows */
	{
		u_long nonblocking = 1;
		ioctlsocket( desc, FIONBIO, &nonblocking );
	}
#endif

	/* Disable Nagle algorithm for immediate send - helps prompts appear instantly */
	{
		int nodelay = 1;
		setsockopt( desc, IPPROTO_TCP, TCP_NODELAY, (const char *) &nodelay, sizeof( nodelay ) );
	}

	/*
	 * Cons a new descriptor.
	 */
	if ( descriptor_free == NULL ) {
		dnew = alloc_perm( sizeof( *dnew ) );
	} else {
		dnew = descriptor_free;
		descriptor_free = descriptor_free->next;
	}

	init_descriptor( dnew, desc );

	size = sizeof( sock );
	if ( getpeername( desc, (struct sockaddr *) &sock, &size ) < 0 ) {
		perror( "New_descriptor: getpeername" );
		dnew->host = str_dup( "(unknown)" );
	} else {
		/*
		 * Would be nice to use inet_ntoa here but it takes a struct arg,
		 * which ain't very compatible between gcc and system libraries.
		 */
		int addr;
		addr = ntohl( sock.sin_addr.s_addr );
		sprintf( buf, "%d.%d.%d.%d",
			( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
			( addr >> 8 ) & 0xFF, ( addr ) & 0xFF );

		sprintf( log_buf, "Connection Established: %s", buf );
		log_string( log_buf );

		dnew->host = str_dup( buf ); // set the temporary ip as the host.

		/* Skip DNS lookup for localhost - no point and it can timeout on Windows */
		if ( addr == 0x7F000001 ) /* 127.0.0.1 */
		{
			dnew->lookup_status = STATUS_DONE;
			dnew->host = str_dup( "localhost" );
		} else {
			/* Set up the dummyarg for use in lookup_address */
			dummyarg->buf = str_dup( (char *) &sock.sin_addr );
			dummyarg->d = dnew;

			if ( thread_count < 50 ) /* should be more than plenty */
			{
				/* just use the ip, then make the thread do the lookup */
				pthread_create( &thread_lookup, &attr, (void *) &lookup_address, (void *) dummyarg );
			} else
				DOS_ATTACK = TRUE;
		}
	}

	/*
	 * Init descriptor data.
	 */
	dnew->next = descriptor_list;
	descriptor_list = dnew;

	if ( DOS_ATTACK ) {
		write_to_buffer( dnew, "Sorry, currently under attack, try again later.\n\r", 0 );
		close_socket( dnew );
		return;
	}

	/* Offer protocol support to client */
	/* Order matters - Mudlet expects: MCCP, MSSP, GMCP, then MXP last */
	write_to_buffer( dnew, compress2_will, 0 ); /* MCCP v2 preferred */
	write_to_buffer( dnew, compress_will, 0 );	/* MCCP v1 fallback */
	write_to_buffer( dnew, mssp_will, 0 );		/* MSSP server status */
	write_to_buffer( dnew, gmcp_will, 0 );		/* GMCP protocol */
	write_to_buffer( dnew, mxp_will, 0 );		/* MXP rich text */
	write_to_buffer( dnew, naws_do, 0 );		/* NAWS window size (request) */

	/* send greeting */
	{
		extern char *help_greeting;
		if ( help_greeting[0] == '.' )
			write_to_buffer( dnew, help_greeting + 1, 0 );
		else
			write_to_buffer( dnew, help_greeting, 0 );
	}

	return;
}

void lookup_address( DUMMY_ARG *darg ) {
	struct hostent *from = 0;
	struct hostent ent;
	char buf[16384]; // enough ??
	int err;

	thread_count++;

	gethostbyaddr_r( darg->buf, sizeof( darg->buf ), AF_INET, &ent, buf, 16384, &from, &err );

	if ( from && from->h_name ) {
		free_string( darg->d->host );
		darg->d->host = str_dup( from->h_name );
	}

	/*
	 * Brilliant system there Mr. Jobo
	 */
	darg->d->lookup_status++;

	free_string( darg->buf );
	darg->status = 0;

	thread_count--;

	pthread_exit( 0 );
}

bool check_banned( DESCRIPTOR_DATA *dnew ) {
	BAN_DATA *pban;

	for ( pban = ban_list; pban != NULL; pban = pban->next )
		if ( !str_suffix( pban->name, dnew->host ) ) return TRUE;
	return FALSE;
}

void close_socket( DESCRIPTOR_DATA *dclose ) {
	CHAR_DATA *ch;

	if ( dclose->lookup_status > STATUS_DONE ) return;
	dclose->lookup_status += 2;

	if ( dclose->outtop > 0 ) process_output( dclose, FALSE );
	if ( dclose->snoop_by != NULL )
		write_to_buffer( dclose->snoop_by, "Your victim has left the game.\n\r", 0 );

	if ( dclose->character != NULL &&
		( dclose->connected == CON_PLAYING || dclose->connected == CON_EDITING ) &&
		IS_NPC( dclose->character ) ) do_return( dclose->character, "" );
	if ( dclose->connected == CON_PFILE ) jope_done( dclose->character, "" );

	/*
	 * Clear snoops
	 */
	{
		DESCRIPTOR_DATA *d;

		for ( d = descriptor_list; d != NULL; d = d->next )
			if ( d->snoop_by == dclose ) d->snoop_by = NULL;
	}

	/*
	 * Loose link or free char
	 */
	if ( ( ch = dclose->character ) != NULL ) {
		sprintf( log_buf, "Closing link to %s.", ch->name );
		log_string( log_buf );
		/* If ch is writing note or playing, just lose link otherwise clear char */
		if ( ( dclose->connected == CON_PLAYING ) || ( ( dclose->connected >= CON_NOTE_TO ) && ( dclose->connected <= CON_NOTE_FINISH ) ) ) {
			if ( IS_SET( ch->extra, EXTRA_AFK ) ) REMOVE_BIT( ch->extra, EXTRA_AFK );
			if ( IS_SET( ch->extra, EXTRA_OSWITCH ) ) do_humanform( ch, "" );
			do_call( ch, "all" );
			act( "$n has lost $s link.", ch, NULL, NULL, TO_ROOM );
			ch->desc = NULL;
		} else {
			free_char( dclose->character );
		}
	}
	if ( d_next == dclose ) d_next = d_next->next;
	dclose->connected = CON_NOT_PLAYING;
	return;
}

/* For a better kickoff message :) KaVir */
void close_socket2( DESCRIPTOR_DATA *dclose, bool kickoff ) {
	CHAR_DATA *ch;

	if ( dclose->lookup_status > STATUS_DONE ) return;
	dclose->lookup_status += 2;

	if ( dclose->outtop > 0 ) process_output( dclose, FALSE );
	if ( dclose->snoop_by != NULL )
		write_to_buffer( dclose->snoop_by, "Your victim has left the game.\n\r", 0 );

	if ( dclose->character != NULL &&
		( dclose->connected == CON_PLAYING || dclose->connected == CON_EDITING ) &&
		IS_NPC( dclose->character ) ) do_return( dclose->character, "" );
	if ( dclose->connected == CON_PFILE ) jope_done( dclose->character, "" );

	/*
	 * Clear snoops
	 */
	{
		DESCRIPTOR_DATA *d;

		for ( d = descriptor_list; d != NULL; d = d->next )
			if ( d->snoop_by == dclose ) d->snoop_by = NULL;
	}

	if ( ( ch = dclose->character ) != NULL ) {
		if ( dclose->connected == CON_PLAYING || dclose->connected == CON_EDITING ) {
			if ( kickoff )
				act( "$n doubles over in agony and $s eyes roll up into $s head.", ch, NULL, NULL, TO_ROOM );
			save_char_obj( ch );
			ch->desc = NULL;
		} else {
			free_char( dclose->character );
		}
	}
	if ( d_next == dclose ) d_next = d_next->next;
	dclose->connected = CON_NOT_PLAYING;
	return;
}

bool read_from_descriptor( DESCRIPTOR_DATA *d ) {
	int iStart;

	/* Hold horses if pending command already. */
	if ( d->incomm[0] != '\0' )
		return TRUE;

	/* one dirty patch to avoid spams of EOF's */
	if ( d->connected == CON_NOT_PLAYING )
		return TRUE;

	/* Check for overflow. */
	iStart = (int) strlen( d->inbuf );
	if ( iStart >= (int) sizeof( d->inbuf ) - 10 ) {
		if ( d != NULL && d->character != NULL ) {
			sprintf( log_buf, "%s input overflow!", mask_ip( d->character->lasthost ) );
			log_string( log_buf );
		} else if ( d->lookup_status != STATUS_LOOKUP ) {
			sprintf( log_buf, "%s input overflow!", mask_ip( d->host ) );
			log_string( log_buf );
		}
		write_to_descriptor( d,
			"\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
		return FALSE;
	}

	/* Snarf input. */
	for ( ;; ) {
		int nRead;

#if !defined( WIN32 )
		nRead = read( d->descriptor, d->inbuf + iStart,
			sizeof( d->inbuf ) - 10 - iStart );
#else
		nRead = recv( d->descriptor, d->inbuf + iStart,
			sizeof( d->inbuf ) - 10 - iStart, 0 );
#endif
		if ( nRead > 0 ) {
			iStart += nRead;
			if ( d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r' )
				break;
		} else if ( nRead == 0 ) {
			log_string( "EOF encountered on read." );
			return FALSE;
		}
#if !defined( WIN32 )
		else if ( errno == EWOULDBLOCK )
			break;
#else
		else if ( WSAGetLastError() == WSAEWOULDBLOCK || errno == EAGAIN )
			break;
#endif
		else {
			perror( "Read_from_descriptor" );
			return FALSE;
		}
	}

	d->inbuf[iStart] = '\0';
	return TRUE;
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d ) {
	int i, j, k;
	bool has_newline = FALSE;

	/*
	 * Hold horses if pending command already.
	 */
	if ( d->incomm[0] != '\0' )
		return;

	/*
	 * Look for at least one new line, but still process telnet sequences.
	 */
	for ( i = 0; d->inbuf[i] != '\0'; i++ ) {
		if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' ) {
			has_newline = TRUE;
			break;
		}
	}

	/* Process telnet sequences even without newline */
	if ( !has_newline ) {
		/* Scan for and handle IAC sequences */
		for ( i = 0; d->inbuf[i] != '\0'; i++ ) {
			if ( d->inbuf[i] == (signed char) IAC ) {
				/* MCCP v2 (preferred) */
				if ( !memcmp( &d->inbuf[i], compress2_do, strlen( compress2_do ) ) ) {
					i += (int) strlen( compress2_do ) - 1;
					compressStart( d, 2 );
				} else if ( !memcmp( &d->inbuf[i], compress2_dont, strlen( compress2_dont ) ) ) {
					i += (int) strlen( compress2_dont ) - 1;
					/* Only end compression if we're using v2 */
					if ( d->mccp_version == 2 )
						compressEnd( d );
				}
				/* MCCP v1 (fallback) */
				else if ( !memcmp( &d->inbuf[i], compress_do, strlen( compress_do ) ) ) {
					i += (int) strlen( compress_do ) - 1;
					compressStart( d, 1 );
				} else if ( !memcmp( &d->inbuf[i], compress_dont, strlen( compress_dont ) ) ) {
					i += (int) strlen( compress_dont ) - 1;
					/* Only end compression if we're using v1 */
					if ( d->mccp_version == 1 )
						compressEnd( d );
				}
				/* MSSP */
				else if ( !memcmp( &d->inbuf[i], mssp_do, strlen( mssp_do ) ) ) {
					i += (int) strlen( mssp_do ) - 1;
					mssp_send( d );
				}
				/* GMCP */
				else if ( !memcmp( &d->inbuf[i], gmcp_do, strlen( gmcp_do ) ) ) {
					i += (int) strlen( gmcp_do ) - 1;
					gmcp_init( d );
				} else if ( !memcmp( &d->inbuf[i], gmcp_dont, strlen( gmcp_dont ) ) ) {
					i += (int) strlen( gmcp_dont ) - 1;
					d->gmcp_enabled = FALSE;
				}
				/* GMCP subnegotiation: IAC SB GMCP ... IAC SE */
				else if ( d->inbuf[i + 1] == (signed char) SB &&
					d->inbuf[i + 2] == (signed char) TELOPT_GMCP ) {
					int sb_start = i + 3;
					int sb_len = 0;
					while ( d->inbuf[sb_start + sb_len] != '\0' ) {
						if ( d->inbuf[sb_start + sb_len] == (signed char) IAC &&
							d->inbuf[sb_start + sb_len + 1] == (signed char) SE ) {
							break;
						}
						sb_len++;
					}
					if ( sb_len > 0 ) {
						gmcp_handle_subnegotiation( d, (unsigned char *) &d->inbuf[sb_start], sb_len );
					}
					i += 3 + sb_len + 1; /* Skip IAC SB GMCP ... IAC SE */
				}
				/* MXP - check strlen > 0 to avoid matching empty strings */
				else if ( strlen( mxp_do ) > 0 && !memcmp( &d->inbuf[i], mxp_do, strlen( mxp_do ) ) ) {
					i += (int) strlen( mxp_do ) - 1;
					mxpStart( d );
				} else if ( strlen( mxp_dont ) > 0 && !memcmp( &d->inbuf[i], mxp_dont, strlen( mxp_dont ) ) ) {
					i += (int) strlen( mxp_dont ) - 1;
					mxpEnd( d );
				}
				/* NAWS - client agrees to send window size */
				else if ( !memcmp( &d->inbuf[i], naws_will, strlen( naws_will ) ) ) {
					i += (int) strlen( naws_will ) - 1;
					/* Client will send subnegotiation with size */
				} else if ( !memcmp( &d->inbuf[i], naws_wont, strlen( naws_wont ) ) ) {
					i += (int) strlen( naws_wont ) - 1;
					d->naws_enabled = FALSE;
				}
				/* NAWS subnegotiation: IAC SB NAWS <4+ bytes> IAC SE */
				else if ( d->inbuf[i + 1] == (signed char) SB &&
					d->inbuf[i + 2] == (signed char) TELOPT_NAWS ) {
					int sb_start = i + 3;
					int sb_len = 0;
					/* Find IAC SE that ends subnegotiation
					 * Note: NAWS data can contain NUL bytes (width/height high byte = 0)
					 * so we can't use '\0' as terminator. Limit search to reasonable length. */
					while ( sb_len < 16 ) {
						if ( (unsigned char) d->inbuf[sb_start + sb_len] == IAC &&
							(unsigned char) d->inbuf[sb_start + sb_len + 1] == SE ) {
							break;
						}
						sb_len++;
					}
					/* 4-8 bytes: 4 data bytes, possibly with IAC escaping */
					if ( sb_len >= 4 && sb_len <= 8 ) {
						naws_handle_subnegotiation( d, (unsigned char *) &d->inbuf[sb_start], sb_len );
					}
					i += 3 + sb_len + 1; /* Skip IAC SB NAWS ... IAC SE */
				}
			}
		}
		/* Clear buffer after processing telnet-only data */
		d->inbuf[0] = '\0';
		return;
	}

	/*
	 * Canonical input processing.
	 */
	for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ ) {
		if ( k >= MAX_INPUT_LENGTH - 2 ) {
			write_to_descriptor( d, "Line too long.\n\r", 0 );

			/* skip the rest of the line */
			for ( ; d->inbuf[i] != '\0'; i++ ) {
				if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
					break;
			}
			d->inbuf[i] = '\n';
			d->inbuf[i + 1] = '\0';
			break;
		}

		if ( d->inbuf[i] == '\b' && k > 0 )
			--k;
		else if ( isascii( d->inbuf[i] ) && isprint( d->inbuf[i] ) )
			d->incomm[k++] = d->inbuf[i];
		else if ( d->inbuf[i] == (signed char) IAC ) {
			/* MCCP v2 (preferred) */
			if ( !memcmp( &d->inbuf[i], compress2_do, strlen( compress2_do ) ) ) {
				i += (int) strlen( compress2_do ) - 1;
				compressStart( d, 2 );
			} else if ( !memcmp( &d->inbuf[i], compress2_dont, strlen( compress2_dont ) ) ) {
				i += (int) strlen( compress2_dont ) - 1;
				/* Only end compression if we're using v2 */
				if ( d->mccp_version == 2 )
					compressEnd( d );
			}
			/* MCCP v1 (fallback) */
			else if ( !memcmp( &d->inbuf[i], compress_do, strlen( compress_do ) ) ) {
				i += (int) strlen( compress_do ) - 1;
				compressStart( d, 1 );
			} else if ( !memcmp( &d->inbuf[i], compress_dont, strlen( compress_dont ) ) ) {
				i += (int) strlen( compress_dont ) - 1;
				/* Only end compression if we're using v1 */
				if ( d->mccp_version == 1 )
					compressEnd( d );
			}
			/* MSSP */
			else if ( !memcmp( &d->inbuf[i], mssp_do, strlen( mssp_do ) ) ) {
				i += (int) strlen( mssp_do ) - 1;
				mssp_send( d );
			}
			/* MXP - check strlen > 0 to avoid matching empty strings */
			else if ( strlen( mxp_do ) > 0 && !memcmp( &d->inbuf[i], mxp_do, strlen( mxp_do ) ) ) {
				i += (int) strlen( mxp_do ) - 1;
				mxpStart( d );
			} else if ( strlen( mxp_dont ) > 0 && !memcmp( &d->inbuf[i], mxp_dont, strlen( mxp_dont ) ) ) {
				i += (int) strlen( mxp_dont ) - 1;
				mxpEnd( d );
			}
			/* GMCP */
			else if ( !memcmp( &d->inbuf[i], gmcp_do, strlen( gmcp_do ) ) ) {
				i += (int) strlen( gmcp_do ) - 1;
				gmcp_init( d );
			} else if ( !memcmp( &d->inbuf[i], gmcp_dont, strlen( gmcp_dont ) ) ) {
				i += (int) strlen( gmcp_dont ) - 1;
				d->gmcp_enabled = FALSE;
			}
			/* GMCP subnegotiation: IAC SB GMCP ... IAC SE */
			else if ( d->inbuf[i + 1] == (signed char) SB &&
				d->inbuf[i + 2] == (signed char) TELOPT_GMCP ) {
				int sb_start = i + 3;
				int sb_len = 0;
				/* Find IAC SE that ends subnegotiation */
				while ( d->inbuf[sb_start + sb_len] != '\0' ) {
					if ( d->inbuf[sb_start + sb_len] == (signed char) IAC &&
						d->inbuf[sb_start + sb_len + 1] == (signed char) SE ) {
						break;
					}
					sb_len++;
				}
				if ( sb_len > 0 ) {
					gmcp_handle_subnegotiation( d, (unsigned char *) &d->inbuf[sb_start], sb_len );
				}
				i += 3 + sb_len + 1; /* Skip IAC SB GMCP ... IAC SE */
			}
			/* NAWS - client agrees to send window size */
			else if ( !memcmp( &d->inbuf[i], naws_will, strlen( naws_will ) ) ) {
				i += (int) strlen( naws_will ) - 1;
				/* Client will send subnegotiation with size */
			} else if ( !memcmp( &d->inbuf[i], naws_wont, strlen( naws_wont ) ) ) {
				i += (int) strlen( naws_wont ) - 1;
				d->naws_enabled = FALSE;
			}
			/* NAWS subnegotiation: IAC SB NAWS <4+ bytes> IAC SE */
			else if ( d->inbuf[i + 1] == (signed char) SB &&
				d->inbuf[i + 2] == (signed char) TELOPT_NAWS ) {
				int sb_start = i + 3;
				int sb_len = 0;
				/* Find IAC SE that ends subnegotiation
				 * Note: NAWS data can contain NUL bytes (width/height high byte = 0)
				 * so we can't use '\0' as terminator. Limit search to reasonable length. */
				while ( sb_len < 16 ) {
					if ( (unsigned char) d->inbuf[sb_start + sb_len] == IAC &&
						(unsigned char) d->inbuf[sb_start + sb_len + 1] == SE ) {
						break;
					}
					sb_len++;
				}
				/* 4-8 bytes: 4 data bytes, possibly with IAC escaping */
				if ( sb_len >= 4 && sb_len <= 8 ) {
					naws_handle_subnegotiation( d, (unsigned char *) &d->inbuf[sb_start], sb_len );
				}
				i += 3 + sb_len + 1; /* Skip IAC SB NAWS ... IAC SE */
			}
		}
	}

	/*
	 * Finish off the line.
	 */
	if ( k == 0 )
		d->incomm[k++] = ' ';
	d->incomm[k] = '\0';

	/*
	 * Deal with bozos with #repeat 1000 ...
	 */
	if ( k > 1 || d->incomm[0] == '!' ) {
		if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) ) {
			d->repeat = 0;
		} else {
			if ( ++d->repeat >= 40 ) {
				if ( d != NULL && d->character != NULL ) {
					sprintf( log_buf, "%s input overflow!", mask_ip( d->character->lasthost ) );
					log_string( log_buf );
				} else if ( d->lookup_status != STATUS_LOOKUP ) {
					sprintf( log_buf, "%s input overflow!", mask_ip( d->host ) );
					log_string( log_buf );
				}
				write_to_descriptor( d,
					"\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
				sprintf( d->incomm, "quit" );
			}
		}
	}

	/*
	 * Do '!' substitution.
	 */
	if ( d->incomm[0] == '!' )
		strcpy( d->incomm, d->inlast );
	else
		strcpy( d->inlast, d->incomm );

	/*
	 * Shift the input buffer.
	 */
	while ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
		i++;
	for ( j = 0; ( d->inbuf[j] = d->inbuf[i + j] ) != '\0'; j++ );
	return;
}

/* COPYOVER_FILE and EXE_FILE are now defined in merc.h using mud_path() */

/*
 * Crash recovery system written by Mandrax, based on copyover
 * Includes call to signal() in main.
 * Mostly copied from copyover, but doesn't save chars.
 */
void crashrecov( int iSignal ) {
	CHAR_DATA *gch;
	FILE *fp;
	DESCRIPTOR_DATA *d, *d_next;
	char buf[200], buf2[100];
	int pid;
#ifndef WIN32
	/* Note: fork() return value intentionally unused - we only care about pid */
#endif
	FILE *fReturn;
	FILE *fCrash;

	/*
	 * An attempt to avoid spam crashes
	 */
	if ( ( fReturn = fopen( CRASH_TEMP_FILE, "r" ) ) != NULL ) {
		signal( SIGSEGV, SIG_IGN );
		raise( SIGSEGV );
		return;
	}
	fCrash = fopen( CRASH_TEMP_FILE, "w" );
	fprintf( fCrash, "0" );
	fclose( fCrash );

	dump_last_command();

	/*
	 * This will cause a core dump, even though the signal was handled
	 * Note: fork() is not available on Windows, so skip the core dump logic there
	 */
#ifndef WIN32
	(void) fork(); /* Return value intentionally ignored */
	wait( NULL );
	if ( ( pid = getpid() ) != proc_pid )
#else
	/* On Windows, compat.h defines fork() as -1, which means it "failed" */
	/* We just continue with crash recovery without a core dump */
	(void) pid; /* Suppress unused variable warning on Windows */
	if ( 0 )
#endif
	{
		signal( SIGSEGV, SIG_IGN );
		raise( SIGSEGV );
		return;
	}

	fp = fopen( COPYOVER_FILE, "w" );
	if ( !fp ) {
		perror( "crashrecov:fopen" );
		return;
	}

	for ( gch = char_list; gch != NULL; gch = gch->next ) {
		if ( IS_NPC( gch ) ) continue;

		/* Fix any possibly head/object forms */
		if ( IS_HEAD( gch, LOST_HEAD ) || IS_SET( gch->extra, EXTRA_OSWITCH ) ) {
			REMOVE_BIT( gch->loc_hp[0], LOST_HEAD );
			REMOVE_BIT( gch->affected_by, AFF_POLYMORPH );
			REMOVE_BIT( gch->extra, EXTRA_OSWITCH );
			gch->morph = str_dup( "" );
			gch->pcdata->chobj = NULL;
			gch->pcdata->obj_vnum = 0;
			char_from_room( gch );
			char_to_room( gch, get_room_index( ROOM_VNUM_ALTAR ) );
		}

		/* Make sure the player is saved with all his eq */
		gch->hit = gch->max_hit;
		gch->position = POS_STANDING;
		do_call( gch, "all" );
		save_char_obj( gch );
	}

	sprintf( buf, "\n\r <*>          Dystopia has crashed           <*>\n\r\n\r <*>   Attempting to restore last savefile   <*>\n\r" );

	/* For each playing descriptor, save its state */
	for ( d = descriptor_list; d; d = d_next ) {
		CHAR_DATA *och = d->character;

		compressEnd2( d );
		d_next = d->next;						  /* We delete from the list , so need to save this */
		if ( !d->character || d->connected != 0 ) /* drop those logging on */
		{
			write_to_descriptor_2( d->descriptor, "\n\rSorry, the mud has crashed.\n\rPlease log on another char and report this, your char MAY be bugged.\n\r", 0 );
			close_socket2( d, FALSE ); /* throw'em out */
		} else {
			fprintf( fp, "%d %s %s\n", d->descriptor, och->name, d->host );
			write_to_descriptor_2( d->descriptor, buf, 0 );
		}
	}

	fprintf( fp, "-1\n" );
	fclose( fp );

	/* Close reserve and other always-open files and release other resources */
	fclose( fpReserve );

	/* recycle descriptors */
	recycle_descriptors();

	/* exec - descriptors are inherited */

	sprintf( buf, "%d", port );
	sprintf( buf2, "%d", control );

	execl( EXE_FILE, "dystopia", buf, "crashrecov", buf2, (char *) NULL );

	/* Failed - sucessful exec will not return */

	perror( "crashrecov: execl" );
	log_string( "Crash recovery FAILED!\n\r" );

	/* The least we can do is exit gracefully :P */
	exit( 1 );
}

void retell_protocols( DESCRIPTOR_DATA *d ) {
	/* Re-negotiate all protocols after copyover */
	/* Per RFC 854, we must send WONT first to reset client's protocol state, */
	/* then send WILL to re-offer. MUSHclient strictly follows RFC and ignores */
	/* WILL offers for protocols it thinks are already active from pre-copyover. */

	/* First, send WONT/DONT to reset client's protocol state */
	write_to_descriptor( d, (char *) compress2_wont, (int) strlen( compress2_wont ) );
	write_to_descriptor( d, (char *) compress_wont, (int) strlen( compress_wont ) );
	write_to_descriptor( d, (char *) mssp_wont, (int) strlen( mssp_wont ) );
	write_to_descriptor( d, (char *) gmcp_wont, (int) strlen( gmcp_wont ) );
	write_to_descriptor( d, (char *) mxp_wont, (int) strlen( mxp_wont ) );
	write_to_descriptor( d, (char *) naws_dont, (int) strlen( naws_dont ) ); /* NAWS uses DO/DONT */

	/* Now send WILL/DO to offer protocols fresh */
	/* Order matters - Mudlet expects: MCCP, MSSP, GMCP, then MXP last */
	write_to_descriptor( d, (char *) compress2_will, (int) strlen( compress2_will ) ); /* MCCP v2 */
	write_to_descriptor( d, (char *) compress_will, (int) strlen( compress_will ) );   /* MCCP v1 */
	write_to_descriptor( d, (char *) mssp_will, (int) strlen( mssp_will ) );		   /* MSSP */
	write_to_descriptor( d, (char *) gmcp_will, (int) strlen( gmcp_will ) );		   /* GMCP */
	write_to_descriptor( d, (char *) mxp_will, (int) strlen( mxp_will ) );			   /* MXP */
	write_to_descriptor( d, (char *) naws_do, (int) strlen( naws_do ) );			   /* NAWS */
	return;
}

bool process_output( DESCRIPTOR_DATA *d, bool fPrompt ) {
	extern bool merc_down;

	/*
	 * Bust a prompt.
	 */
	if ( fPrompt && !merc_down && ( d->connected == CON_PLAYING || d->connected == CON_PFILE ) ) {
		CHAR_DATA *ch;
		CHAR_DATA *victim;

		ch = d->original ? d->original : d->character;
		if ( IS_SET( ch->act, PLR_BLANK ) )
			write_to_buffer( d, "\n\r", 2 );

		if ( IS_SET( ch->act, PLR_PROMPT ) && IS_EXTRA( ch, EXTRA_PROMPT ) )
			bust_a_prompt( d );
		else if ( IS_SET( ch->act, PLR_PROMPT ) ) {
			char buf[MAX_STRING_LENGTH];
			char cond[MAX_INPUT_LENGTH];
			char hit_str[MAX_INPUT_LENGTH];
			char mana_str[MAX_INPUT_LENGTH];
			char move_str[MAX_INPUT_LENGTH];
			char exp_str[MAX_INPUT_LENGTH];
			char tmp_str[MAX_INPUT_LENGTH];
			char resource_str[MAX_INPUT_LENGTH];

			ch = d->character;

			/* Build class-specific resource string */
			resource_str[0] = '\0';
			if ( !IS_NPC( ch ) && ch->pcdata != NULL ) {
				if ( IS_CLASS( ch, CLASS_WEREWOLF ) ) {
					if ( ch->gnosis[GMAXIMUM] > 0 ) {
						snprintf( resource_str, sizeof( resource_str ), " [#rR:%d#n #CG:%d#n]",
							ch->rage, ch->gnosis[GCURRENT] );
					} else {
						snprintf( resource_str, sizeof( resource_str ), " [#rR:%d#n]", ch->rage );
					}
				} else if ( IS_CLASS( ch, CLASS_VAMPIRE ) ) {
					snprintf( resource_str, sizeof( resource_str ), " [#rR:%d#n #rB:%d#n]",
						ch->rage, ch->pcdata->condition[COND_THIRST] );
				} else if ( IS_CLASS( ch, CLASS_NINJA ) ) {
					snprintf( resource_str, sizeof( resource_str ), " [#rR:%d#n]", ch->rage );
				} else if ( IS_CLASS( ch, CLASS_DIRGESINGER ) || IS_CLASS( ch, CLASS_SIREN ) ) {
					snprintf( resource_str, sizeof( resource_str ), " [#rRes:%d#n]", ch->rage );
				} else if ( IS_CLASS( ch, CLASS_MONK ) ) {
					snprintf( resource_str, sizeof( resource_str ), " [#CC:%d/%d#n]",
						ch->chi[CURRENT], ch->chi[MAXIMUM] );
				} else if ( IS_CLASS( ch, CLASS_DEMON ) || IS_CLASS( ch, CLASS_DROW ) ) {
					snprintf( resource_str, sizeof( resource_str ), " [#CP:%d/%d#n]",
						ch->pcdata->stats[8], ch->pcdata->stats[9] );
				} else if ( IS_CLASS( ch, CLASS_TANARRI ) || IS_CLASS( ch, CLASS_DROID ) ) {
					snprintf( resource_str, sizeof( resource_str ), " [#CP:%d#n]",
						ch->pcdata->stats[8] );
				} else if ( IS_CLASS( ch, CLASS_PSION ) || IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
					snprintf( resource_str, sizeof( resource_str ), " [#CFoc:%d#n]", ch->rage );
				}
			}

			if ( IS_HEAD( ch, LOST_HEAD ) || IS_EXTRA( ch, EXTRA_OSWITCH ) ) {
				add_commas_to_number( ch->exp, tmp_str, sizeof( tmp_str ) );
				snprintf( exp_str, sizeof( exp_str ), "%s%s#n", col_scale_code( ch->exp, 10000000 ), tmp_str );
				snprintf( buf, sizeof( buf ), "#7<[#L%sX#7] [#y?#RH #y?#CM #y?#GV#7]>#n ", exp_str );
			} else if ( ch->position == POS_FIGHTING ) {
				victim = ch->fighting;
				if ( victim == NULL ) {
					snprintf( cond, sizeof( cond ), "#RNA#n" );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 25 ) {
					snprintf( cond, sizeof( cond ), "#RAwful#n" );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 50 ) {
					snprintf( cond, sizeof( cond ), "#LPoor#n" );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 75 ) {
					snprintf( cond, sizeof( cond ), "#GFair#n" );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 100 ) {
					snprintf( cond, sizeof( cond ), "#yGood#n" );
				} else {
					snprintf( cond, sizeof( cond ), "#CPerfect#n" );
				}
				snprintf( hit_str, sizeof( hit_str ), "%s%d#n", col_scale_code( ch->hit, ch->max_hit ), ch->hit );
				snprintf( mana_str, sizeof( mana_str ), "%s%d#n", col_scale_code( ch->mana, ch->max_mana ), ch->mana );
				snprintf( move_str, sizeof( move_str ), "%s%d#n", col_scale_code( ch->move, ch->max_move ), ch->move );
				snprintf( buf, sizeof( buf ), "#7<[%s] [%sH %sM %sV]%s>#n ", cond,
					hit_str, mana_str, move_str, resource_str );
			} else {
				snprintf( hit_str, sizeof( hit_str ), "%s%d#n", col_scale_code( ch->hit, ch->max_hit ), ch->hit );
				snprintf( mana_str, sizeof( mana_str ), "%s%d#n", col_scale_code( ch->mana, ch->max_mana ), ch->mana );
				snprintf( move_str, sizeof( move_str ), "%s%d#n", col_scale_code( ch->move, ch->max_move ), ch->move );
				add_commas_to_number( ch->exp, tmp_str, sizeof( tmp_str ) );
				snprintf( exp_str, sizeof( exp_str ), "%s%s#n", col_scale_code( ch->exp, 10000000 ), tmp_str );
				snprintf( buf, sizeof( buf ), "#7<[%s] [%sH %sM %sV]%s>#n ", exp_str, hit_str, mana_str, move_str, resource_str );
			}
			write_to_buffer( d, buf, 0 );
		}

		if ( IS_SET( ch->act, PLR_TELNET_GA ) )
			write_to_buffer( d, go_ahead_str, 0 );
	}

	/*
	 * Short-circuit if nothing to write.
	 */
	if ( d->outtop == 0 )
		return TRUE;

	/*
	 * Snoop-o-rama.
	 */
	if ( d->snoop_by != NULL ) {
		write_to_buffer( d->snoop_by, "% ", 2 );
		write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
	}

	/*
	 * OS-dependent output.
	 */
	if ( !write_to_descriptor( d, d->outbuf, d->outtop ) ) {
		d->outtop = 0;
		return FALSE;
	} else {
		d->outtop = 0;
		return TRUE;
	}
}

/*
 * Color code table - maps color code characters to ANSI sequences.
 * Format: { code_char, ansi_sequence }
 */
static const struct {
	char code;
	const char *ansi;
} color_codes[] = {
	/* Numbers: bright colors */
	{ '0', "\033[0;1;30m" }, /* Bright Black */
	{ '1', "\033[0;1;31m" }, /* Bright Red */
	{ '2', "\033[0;1;32m" }, /* Bright Green */
	{ '3', "\033[0;1;33m" }, /* Bright Yellow */
	{ '4', "\033[0;1;34m" }, /* Bright Blue */
	{ '5', "\033[0;1;35m" }, /* Bright Purple */
	{ '6', "\033[0;1;36m" }, /* Bright Cyan */
	{ '7', "\033[0;0;37m" }, /* White */
	{ '8', "\033[0;0;30m" }, /* Black */
	{ '9', "\033[0;1;37m" }, /* Bright White */
	/* Lowercase: dark colors */
	{ 'r', "\033[0;0;31m" }, /* Red */
	{ 'g', "\033[0;0;32m" }, /* Green */
	{ 'o', "\033[0;0;33m" }, /* Yellow/Brown */
	{ 'l', "\033[0;0;34m" }, /* Blue */
	{ 'p', "\033[0;0;35m" }, /* Purple */
	{ 'c', "\033[0;0;36m" }, /* Cyan */
	{ 'y', "\033[0;1;33m" }, /* Bright Yellow */
	/* Uppercase: bright colors (aliases) */
	{ 'R', "\033[0;1;31m" }, /* Bright Red */
	{ 'G', "\033[0;1;32m" }, /* Bright Green */
	{ 'L', "\033[0;1;34m" }, /* Bright Blue */
	{ 'P', "\033[0;1;35m" }, /* Bright Purple */
	{ 'C', "\033[0;1;36m" }, /* Bright Cyan */
	/* Special formatting */
	{ 'n', "\033[0m" },     /* Reset */
	{ 'i', "\033[7m" },     /* Inverse */
	{ 'u', "\033[4m" },     /* Underline */
	{ 0, NULL }             /* End marker */
};

/* Colors for random #s code (indices 0-14) */
static const char *random_colors[] = {
	"\033[0;1;37m", "\033[0;1;30m", "\033[0;0;30m", "\033[0;0;31m", "\033[0;1;31m",
	"\033[0;0;32m", "\033[0;1;32m", "\033[0;0;33m", "\033[0;1;33m", "\033[0;0;34m",
	"\033[0;1;34m", "\033[0;0;35m", "\033[0;1;35m", "\033[0;0;36m", "\033[0;1;36m"
};
#define NUM_RANDOM_COLORS 15

/* Look up a color code character, return ANSI sequence or NULL */
static const char *lookup_color( char code ) {
	int i;
	for ( i = 0; color_codes[i].code != 0; i++ ) {
		if ( color_codes[i].code == code )
			return color_codes[i].ansi;
	}
	return NULL;
}

void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length ) {
	static char output[65536];  /* 64KB - increased for MXP/color expansion */
	char *ptr;
	char *output_end;
	int i = 0;
	const char *ansi;
	CHAR_DATA *wch = d->character ? ( d->original ? d->original : d->character ) : NULL;
	bool use_ansi = ( !wch || IS_NPC( wch ) || IS_SET( wch->act, PLR_ANSI ) );

	/* clear the output buffer, and set the pointer */
	output[0] = '\0';
	ptr = output;
	output_end = output + sizeof(output) - 20;  /* Reserve space for final ANSI reset + safety margin */

	if ( length <= 0 )
		length = (int) strlen( txt );

	if ( length >= MAX_STRING_LENGTH ) {
		bug( "Write_to_buffer: Way too big. Closing.", 0 );
		return;
	}

	/* initial linebreak */
	if ( d->outtop == 0 && !d->fcommand ) {
		d->outbuf[0] = '\n';
		d->outbuf[1] = '\r';
		d->outtop = 2;
	}

	while ( *txt != '\0' && i++ < length && ptr != NULL && ptr < output_end ) {
		if ( *txt != '#' ) {
			*ptr++ = *txt++;
			continue;
		}

		/* Handle # escape codes */
		i++;
		txt++;
		if ( *txt == '\0' )
			break;

		/* Check for special character escapes */
		switch ( *txt ) {
		case '#':
			if ( ptr < output_end )
				*ptr++ = '#';
			txt++;
			continue;
		case '-':
			if ( ptr < output_end )
				*ptr++ = '~';
			txt++;
			continue;
		case '+':
			if ( ptr < output_end )
				*ptr++ = '%';
			txt++;
			continue;
		}

		/* Check color lookup table */
		if ( ( ansi = lookup_color( *txt ) ) != NULL ) {
			if ( use_ansi ) {
				char *new_ptr = buf_append_safe( ptr, ansi, output, sizeof(output), 20 );
				if ( new_ptr == NULL ) {
					bug( "write_to_buffer: color lookup overflow, truncating", 0 );
					break; /* Exit while loop */
				}
				ptr = new_ptr;
			}
			txt++;
			continue;
		}

		/* Handle special codes not in the table */
		switch ( *txt ) {
		case 's': /* Random color */
			if ( use_ansi ) {
				ansi = random_colors[number_range( 0, NUM_RANDOM_COLORS - 1 )];
				{
					char *new_ptr = buf_append_safe( ptr, ansi, output, sizeof(output), 20 );
					if ( new_ptr == NULL ) {
						bug( "write_to_buffer: random color overflow, truncating", 0 );
						ptr = NULL; /* Signal to exit main loop */
					} else {
						ptr = new_ptr;
					}
				}
			}
			txt++;
			break;
		case 'x': /* 256-color: #x### */
			if ( isdigit( txt[1] ) && isdigit( txt[2] ) && isdigit( txt[3] ) ) {
				if ( use_ansi ) {
					/* Need 14 bytes for ANSI sequence */
					if ( ptr + 14 < output_end ) {
						*ptr++ = '\033';
						*ptr++ = '[';
						*ptr++ = '0';
						*ptr++ = ';';
						*ptr++ = '3';
						*ptr++ = '8';
						*ptr++ = ';';
						*ptr++ = '5';
						*ptr++ = ';';
						*ptr++ = txt[1];
						*ptr++ = txt[2];
						*ptr++ = txt[3];
						*ptr++ = 'm';
					}
				}
				txt += 4;
				i += 3; /* Account for 3 extra digit bytes consumed beyond #x */
			} else {
				txt++;
			}
			break;
		case 'M': /* MXP Secure line start */
			if ( d->mxp_enabled && ptr + 4 < output_end ) {
				*ptr++ = '\033';
				*ptr++ = '[';
				*ptr++ = '1';
				*ptr++ = 'z';
			}
			txt++;
			break;
		case '<': /* MXP entity escape for < */
			if ( d->mxp_enabled && ptr + 4 < output_end ) {
				*ptr++ = '&';
				*ptr++ = 'l';
				*ptr++ = 't';
				*ptr++ = ';';
			} else if ( ptr < output_end ) {
				*ptr++ = '<';
			}
			txt++;
			break;
		case '>': /* MXP entity escape for > */
			if ( d->mxp_enabled && ptr + 4 < output_end ) {
				*ptr++ = '&';
				*ptr++ = 'g';
				*ptr++ = 't';
				*ptr++ = ';';
			} else if ( ptr < output_end ) {
				*ptr++ = '>';
			}
			txt++;
			break;
		case ']': /* MXP Locked line (reset) */
			if ( d->mxp_enabled && ptr + 4 < output_end ) {
				*ptr++ = '\033';
				*ptr++ = '[';
				*ptr++ = '2';
				*ptr++ = 'z';
			}
			txt++;
			break;
		default:
			/* Unknown code - skip it */
			txt++;
			break;
		}
	}

	/* Terminate with reset color (we reserved space for this) */
	/* Handle case where ptr might be invalid */
	if ( ptr == NULL || ptr < output || ptr >= output + sizeof(output) ) {
		/* Buffer overflow - find last valid position */
		bug( "write_to_buffer: buffer overflow detected, truncating", 0 );
		/* Scan backwards to find null terminator or use safe fallback */
		ptr = output;
		while ( *ptr != '\0' && ptr < output + sizeof(output) - 5 ) {
			ptr++;
		}
		if ( ptr >= output + sizeof(output) - 5 ) {
			ptr = output + sizeof(output) - 5;
		}
	}

	/* Add ANSI reset sequence only if ANSI enabled and there's room */
	if ( use_ansi && ptr >= output && ptr + 5 <= output + sizeof(output) ) {
		*ptr++ = '\033';
		*ptr++ = '[';
		*ptr++ = '0';
		*ptr++ = 'm';
		*ptr = '\0';
	} else {
		/* No room for reset, just null terminate */
		if ( ptr >= output && ptr < output + sizeof(output) ) {
			*ptr = '\0';
		} else {
			output[sizeof(output) - 1] = '\0';
		}
	}

	length = (int) strlen( output ); /* Use strlen for safety */

	/*
	 * Screen reader post-processing: collapse multiple spaces to one.
	 * This cleans up alignment padding in who list, equipment, etc.
	 */
	if ( wch && !IS_NPC( wch ) && IS_SET( wch->act, PLR_SCREENREADER ) ) {
		char *src = output;
		char *dst = output;
		bool prev_space = FALSE;

		while ( *src != '\0' ) {
			if ( *src == ' ' ) {
				if ( !prev_space ) {
					*dst++ = ' ';
					prev_space = TRUE;
				}
				src++;
			} else {
				prev_space = FALSE;
				*dst++ = *src++;
			}
		}
		*dst = '\0';
		length = (int) ( dst - output );
	}

	/* Expand the buffer as needed */
	while ( d->outtop + length >= d->outsize ) {
		char *obuf;

		if ( d->outsize >= 262144 ) {  /* 256KB - increased for MXP markup */
			bug( "Buffer overflow. Closing.", 0 );
			close_socket( d );
			return;
		}
		obuf = alloc_mem( 2 * d->outsize );
		memcpy( obuf, d->outbuf, d->outtop );
		free_mem( d->outbuf, d->outsize );
		d->outbuf = obuf;
		d->outsize *= 2;
	}

	/* copy data */
	memcpy( d->outbuf + d->outtop, output, length );
	d->outtop += length;
	return;
}

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor_2( int desc, char *txt, int length ) {
	int iStart;
	int nWrite;
	int nBlock;

	if ( length <= 0 )
		length = (int) strlen( txt );

	for ( iStart = 0; iStart < length; iStart += nWrite ) {
		nBlock = UMIN( length - iStart, 4096 );
#if !defined( WIN32 )
		if ( ( nWrite = write( desc, txt + iStart, nBlock ) ) < 0 )
#else
		if ( ( nWrite = send( desc, txt + iStart, nBlock, 0 ) ) < 0 )
#endif
		{
			perror( "Write_to_descriptor" );
			return FALSE;
		}
	}

	return TRUE;
}

/* mccp: write_to_descriptor wrapper */
bool write_to_descriptor( DESCRIPTOR_DATA *d, char *txt, int length ) {
	if ( d->out_compress )
		return writeCompressed( d, txt, length );
	else
		return write_to_descriptor_2( d->descriptor, txt, length );
}

/* Nanny and login functions moved to nanny.c:
 * nanny, check_parse_name, check_reconnect, check_kickoff,
 * check_playing, stop_idling
 */

/* Output functions moved to output.c:
 * stc, col_str_len, cent_to_char, divide*_to_char, banner*_to_char,
 * send_to_char, act, act2, kavitem
 */

/* Prompt functions moved to prompt.c:
 * bust_a_header, bust_a_prompt
 */

/* show_string (pager) moved to prompt.c */

/* gettimeofday for WIN32 is now in compat.c */

void merc_logf( char *fmt, ... ) {
	char buf[2 * MSL];
	va_list args;
	va_start( args, fmt );
	vsprintf( buf, fmt, args );
	va_end( args );

	log_string( buf );
}

/*
 * Returns a masked version of an IP address for GDPR compliance.
 * IPv4: masks last two octets (e.g., "192.168.x.x")
 * Hostnames: returns "***masked***"
 * Uses a static buffer (not reentrant).
 */
const char *mask_ip( const char *ip ) {
	static char buf[64];
	int a, b, c, d;

	if ( ip == NULL || ip[0] == '\0' )
		return "(unknown)";

	if ( sscanf( ip, "%d.%d.%d.%d", &a, &b, &c, &d ) == 4 ) {
		snprintf( buf, sizeof( buf ), "%d.%d.x.x", a, b );
		return buf;
	}

	snprintf( buf, sizeof( buf ), "***masked***" );
	return buf;
}
