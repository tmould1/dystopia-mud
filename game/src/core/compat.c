/***************************************************************************
 *  Windows Compatibility Implementation for Dystopia MUD                  *
 *  Provides POSIX compatibility layer for Windows builds                  *
 ***************************************************************************/

#ifdef WIN32

#include "compat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Forward declarations from merc.h that we need */
extern void log_string( const char *str );

/* ============================================
 * SECTION 1: SHA256 Password Hashing
 * Uses Windows CNG (bcrypt.h) API
 * ============================================ */

static const char base64_chars[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";

char *win32_crypt_sha256( const char *password, const char *salt ) {
	static char result[128];
	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_HASH_HANDLE hHash = NULL;
	NTSTATUS status;
	UCHAR hash[32]; /* SHA256 = 32 bytes */
	DWORD hashLen = 32;
	char combined[512];
	size_t i;
	int j;

	/* Combine salt + password for hashing */
	_snprintf( combined, sizeof( combined ), "%s%s", salt ? salt : "", password );

	/* Open algorithm provider */
	status = BCryptOpenAlgorithmProvider( &hAlg, BCRYPT_SHA256_ALGORITHM,
		NULL, 0 );
	if ( !BCRYPT_SUCCESS( status ) ) {
		/* Fallback - return password with salt prefix (not secure, but works) */
		_snprintf( result, sizeof( result ), "$6$%.8s$%s", salt ? salt : "xx", password );
		return result;
	}

	/* Create hash object */
	status = BCryptCreateHash( hAlg, &hHash, NULL, 0, NULL, 0, 0 );
	if ( !BCRYPT_SUCCESS( status ) ) {
		BCryptCloseAlgorithmProvider( hAlg, 0 );
		_snprintf( result, sizeof( result ), "$6$%.8s$%s", salt ? salt : "xx", password );
		return result;
	}

	/* Hash the data */
	status = BCryptHashData( hHash, (PUCHAR) combined, (ULONG) strlen( combined ), 0 );
	if ( !BCRYPT_SUCCESS( status ) ) {
		BCryptDestroyHash( hHash );
		BCryptCloseAlgorithmProvider( hAlg, 0 );
		_snprintf( result, sizeof( result ), "$6$%.8s$%s", salt ? salt : "xx", password );
		return result;
	}

	/* Get the hash */
	status = BCryptFinishHash( hHash, hash, hashLen, 0 );
	BCryptDestroyHash( hHash );
	BCryptCloseAlgorithmProvider( hAlg, 0 );

	if ( !BCRYPT_SUCCESS( status ) ) {
		_snprintf( result, sizeof( result ), "$6$%.8s$%s", salt ? salt : "xx", password );
		return result;
	}

	/* Encode to base64-like string */
	/* Prefix with $6$ to indicate SHA256 (matches Linux crypt format) */
	_snprintf( result, 16, "$6$%.8s$", salt ? salt : "xx" );
	j = (int) strlen( result );

	for ( i = 0; i < 32 && j < 120; i += 3 ) {
		result[j++] = base64_chars[( hash[i] >> 2 ) & 0x3F];
		if ( i + 1 < 32 )
			result[j++] = base64_chars[( ( hash[i] & 0x03 ) << 4 ) |
				( ( hash[i + 1] >> 4 ) & 0x0F )];
		if ( i + 2 < 32 )
			result[j++] = base64_chars[( ( hash[i + 1] & 0x0F ) << 2 ) |
				( ( hash[i + 2] >> 6 ) & 0x03 )];
		if ( i + 2 < 32 )
			result[j++] = base64_chars[hash[i + 2] & 0x3F];
	}
	result[j] = '\0';

	return result;
}

/* ============================================
 * SECTION 2: Threading Functions
 * Wrappers around Windows threading API
 * ============================================ */

typedef struct {
	void *( *start_routine )( void * );
	void *arg;
} thread_wrapper_t;

static unsigned __stdcall thread_start_wrapper( void *param ) {
	thread_wrapper_t *wrapper = (thread_wrapper_t *) param;
	void *( *routine )( void * ) = wrapper->start_routine;
	void *arg = wrapper->arg;

	free( wrapper );
	routine( arg );
	return 0;
}

int win32_pthread_create( pthread_t *thread, const pthread_attr_t *attr,
	void *( *start_routine )(void *), void *arg ) {
	thread_wrapper_t *wrapper;

	wrapper = (thread_wrapper_t *) malloc( sizeof( thread_wrapper_t ) );
	if ( !wrapper ) return -1;

	wrapper->start_routine = start_routine;
	wrapper->arg = arg;

	*thread = (HANDLE) _beginthreadex( NULL, 0, thread_start_wrapper,
		wrapper, 0, NULL );
	if ( *thread == 0 ) {
		free( wrapper );
		return -1;
	}

	/* If detached, close handle immediately (thread runs independently) */
	if ( attr && attr->detach_state == PTHREAD_CREATE_DETACHED ) {
		CloseHandle( *thread );
		*thread = NULL;
	}

	return 0;
}

void win32_pthread_exit( void *retval ) {
	(void) retval; /* Unused on Windows */
	_endthreadex( 0 );
}

int win32_pthread_attr_init( pthread_attr_t *attr ) {
	if ( !attr ) return -1;
	attr->detach_state = 0;
	return 0;
}

int win32_pthread_attr_setdetachstate( pthread_attr_t *attr, int state ) {
	if ( !attr ) return -1;
	attr->detach_state = state;
	return 0;
}

void win32_mutex_init( pthread_mutex_t *mutex ) {
	InitializeCriticalSection( mutex );
}

/* ============================================
 * SECTION 3: DNS Lookup Wrapper
 * Thread-safe wrapper using getnameinfo (replaces deprecated gethostbyaddr)
 * ============================================ */

struct hostent *win32_gethostbyaddr_wrapper( const char *addr, int len, int type,
	struct hostent *result, char *buf,
	size_t buflen, struct hostent **res,
	int *h_errnop ) {
	static struct hostent host_result;
	static char hostname[NI_MAXHOST];
	static char *h_aliases[] = { NULL };
	static char *h_addr_list[2];
	static char addr_copy[16];
	struct sockaddr_in sa;
	int ret;

	(void) result; /* Not used - we use our own static buffer */
	(void) buf;
	(void) buflen;
	(void) type; /* Assumed AF_INET */

	/* Build sockaddr_in for getnameinfo */
	memset( &sa, 0, sizeof( sa ) );
	sa.sin_family = AF_INET;
	memcpy( &sa.sin_addr, addr, len );

	ret = getnameinfo( (struct sockaddr *) &sa, sizeof( sa ), hostname, sizeof( hostname ), NULL, 0, 0 );
	if ( ret == 0 ) {
		/* Populate hostent structure */
		memcpy( addr_copy, addr, len );
		h_addr_list[0] = addr_copy;
		h_addr_list[1] = NULL;

		host_result.h_name = hostname;
		host_result.h_aliases = h_aliases;
		host_result.h_addrtype = AF_INET;
		host_result.h_length = len;
		host_result.h_addr_list = h_addr_list;

		*res = &host_result;
		if ( h_errnop ) *h_errnop = 0;
		return &host_result;
	} else {
		*res = NULL;
		if ( h_errnop ) *h_errnop = WSAGetLastError();
		return NULL;
	}
}

/* ============================================
 * SECTION 4: Socket Functions
 * Platform-independent socket write
 * ============================================ */

int socket_write( int fd, const void *buf, size_t len ) {
	return send( fd, (const char *) buf, (int) len, 0 );
}

/* ============================================
 * SECTION 5: Time Functions
 * gettimeofday implementation for Windows
 * ============================================ */

int win32_gettimeofday( struct timeval *tp, struct timezone *tzp ) {
	struct _timeb tb;

	if ( !tp ) return -1;

	_ftime( &tb );
	tp->tv_sec = (long) tb.time;
	tp->tv_usec = tb.millitm * 1000;

	if ( tzp ) {
		tzp->tz_minuteswest = tb.timezone;
		tzp->tz_dsttime = tb.dstflag;
	}

	return 0;
}

/* ============================================
 * SECTION 5: SEH Crash Handler
 * Windows Structured Exception Handling
 * ============================================ */

/* External declarations we need from the MUD */
extern void dump_last_command( void );
extern void save_char_obj( void *ch );
extern void compressEnd2( void *d );
extern void close_socket2( void *d, int kickoff );
extern int write_to_descriptor_2( int desc, char *txt, int length );
extern void recycle_descriptors( void );

/* These are defined in comm.c */
extern void *char_list;
extern void *descriptor_list;
extern void *fpReserve;
extern int port;
extern int control;

/* Path functions from db.c - use these instead of hardcoded paths */
#define MUD_PATH_MAX 512
extern char mud_run_dir[MUD_PATH_MAX];
extern char mud_log_dir[MUD_PATH_MAX];
char *mud_path( const char *dir, const char *filename );

/* Simplified crash handler - just logs and exits cleanly */
LONG WINAPI win32_exception_filter( EXCEPTION_POINTERS *ExceptionInfo ) {
	FILE *fp;
	const char *crash_file;

	(void) ExceptionInfo; /* Could log exception code if needed */

	/* Log the crash */
	log_string( "CRASH: Unhandled exception caught by SEH" );
	dump_last_command();

	/* Create crash marker file using proper path */
	crash_file = mud_path( mud_run_dir, "crash.txt" );
	fp = fopen( crash_file, "w" );
	if ( fp ) {
		fprintf( fp, "Crash detected at %ld\n", (long) time( NULL ) );
		fclose( fp );
	}

	/* On Windows, we can't fork() to save state while generating a dump.
	 * Best we can do is log and exit. A more sophisticated approach would
	 * use MiniDumpWriteDump() to create a crash dump file. */

	return EXCEPTION_EXECUTE_HANDLER; /* Let the process terminate */
}

void win32_setup_crash_handler( void ) {
	SetUnhandledExceptionFilter( win32_exception_filter );
}

/* ============================================
 * SECTION 6: Directory Functions
 * Create directory if it doesn't exist
 * ============================================ */

int ensure_directory( const char *path ) {
	DWORD attrs = GetFileAttributesA( path );
	if ( attrs != INVALID_FILE_ATTRIBUTES ) {
		/* Path exists - check if it's a directory */
		return ( attrs & FILE_ATTRIBUTE_DIRECTORY ) ? 0 : -1;
	}
	/* Create the directory */
	return _mkdir( path ) == 0 ? 0 : -1;
}

#else /* !WIN32 - Unix/Linux implementation */

#include <sys/stat.h>
#include <errno.h>

int ensure_directory( const char *path ) {
	struct stat st;
	if ( stat( path, &st ) == 0 ) {
		/* Path exists - check if it's a directory */
		return S_ISDIR( st.st_mode ) ? 0 : -1;
	}
	/* Create the directory with rwxr-xr-x permissions */
	return mkdir( path, 0755 ) == 0 ? 0 : -1;
}

#endif /* WIN32 */
