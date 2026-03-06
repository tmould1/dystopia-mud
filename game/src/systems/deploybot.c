#include "merc.h"

#if !defined( WIN32 )

#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "deploybot.h"

/* Maximum lines of script output to broadcast. */
#define DEPLOY_MAX_OUTPUT_LINES 10

/* State for the currently running deploy task. */
static pid_t deploy_pid   = -1;
static char deploy_stage[32];          /* "validate", "migrate" */
static char deploy_output_path[MUD_PATH_MAX];

/*
 * Broadcast a message to all immortals on the immtalk channel,
 * formatted as DeployBot using the native immtalk color scheme.
 */
static void deploybot_broadcast( const char *message ) {
	char buf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;

	snprintf( buf, sizeof( buf ), "#y.:#PDeployBot#y:.#C %s#n\n\r", message );

	LIST_FOR_EACH( d, &g_descriptors, DESCRIPTOR_DATA, node ) {
		CHAR_DATA *och = d->original ? d->original : d->character;

		if ( d->connected != CON_PLAYING || och == NULL )
			continue;
		if ( !IS_IMMORTAL( och ) )
			continue;
		if ( IS_SET( och->deaf, CHANNEL_IMMTALK ) )
			continue;

		send_to_char( buf, d->character );
	}
}

/*
 * Build the path to a deploy script.
 * Scripts live at server/scripts/ which is ../../scripts/ relative to
 * mud_base_dir (gamedata/).
 */
static int deploy_script_path( char *out, size_t out_size, const char *stage ) {
	snprintf( out, out_size, "%s/../../scripts/%s.sh", mud_base_dir, stage );

	if ( access( out, X_OK ) != 0 ) {
		return -1;
	}

	return 0;
}

/*
 * Read the last N lines of a file into a buffer for broadcasting.
 */
static void deploy_read_output( const char *path, char *buf, size_t buf_size ) {
	FILE *fp;
	char lines[DEPLOY_MAX_OUTPUT_LINES][256];
	int count = 0;
	int i;
	size_t offset;

	fp = fopen( path, "r" );
	if ( fp == NULL ) {
		snprintf( buf, buf_size, "(no output captured)" );
		return;
	}

	/* Read all lines, keeping only the last DEPLOY_MAX_OUTPUT_LINES. */
	while ( fgets( lines[count % DEPLOY_MAX_OUTPUT_LINES],
		(int) sizeof( lines[0] ), fp ) != NULL ) {
		/* Strip trailing newline. */
		char *nl = strchr( lines[count % DEPLOY_MAX_OUTPUT_LINES], '\n' );
		if ( nl ) *nl = '\0';
		count++;
	}
	fclose( fp );

	if ( count == 0 ) {
		snprintf( buf, buf_size, "(no output)" );
		return;
	}

	/* Assemble the last lines into the output buffer. */
	offset = 0;
	int start = ( count > DEPLOY_MAX_OUTPUT_LINES ) ? count - DEPLOY_MAX_OUTPUT_LINES : 0;

	for ( i = start; i < count; i++ ) {
		int idx = i % DEPLOY_MAX_OUTPUT_LINES;
		size_t len = strlen( lines[idx] );

		if ( offset + len + 2 >= buf_size )
			break;

		memcpy( buf + offset, lines[idx], len );
		offset += len;
		buf[offset++] = '\n';
	}

	buf[offset] = '\0';
}

void do_deploybot( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char script_path[MUD_PATH_MAX];
	char msg[MAX_STRING_LENGTH];
	pid_t pid;

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Usage: deploybot <check|validate|migrate|status>\n\r", ch );
		return;
	}

	/* Status check. */
	if ( !str_cmp( arg, "status" ) ) {
		if ( deploy_pid > 0 ) {
			snprintf( msg, sizeof( msg ), "DeployBot is running: %s (pid %d)\n\r",
				deploy_stage, (int) deploy_pid );
		} else {
			snprintf( msg, sizeof( msg ), "DeployBot is idle.\n\r" );
		}
		send_to_char( msg, ch );
		return;
	}

	/* Check for pending release (local, no fork needed). */
	if ( !str_cmp( arg, "check" ) ) {
		char ready_path[MUD_PATH_MAX];
		char version_path[MUD_PATH_MAX];
		char current_path[MUD_PATH_MAX];
		char pending[256];
		char current[256];
		FILE *fp;

		snprintf( ready_path, sizeof( ready_path ),
			"%s/../../ingest/.ready", mud_base_dir );
		snprintf( version_path, sizeof( version_path ),
			"%s/../../ingest/.pending_version", mud_base_dir );
		snprintf( current_path, sizeof( current_path ),
			"%s/../../live/.version", mud_base_dir );

		if ( access( ready_path, F_OK ) != 0 ) {
			deploybot_broadcast( "No pending release." );
			send_to_char( "No pending release in ingest/.\n\r", ch );
			return;
		}

		/* Read pending version. */
		pending[0] = '\0';
		fp = fopen( version_path, "r" );
		if ( fp != NULL ) {
			if ( fgets( pending, sizeof( pending ), fp ) != NULL ) {
				char *nl = strchr( pending, '\n' );
				if ( nl ) *nl = '\0';
			}
			fclose( fp );
		}

		/* Read current version. */
		current[0] = '\0';
		fp = fopen( current_path, "r" );
		if ( fp != NULL ) {
			if ( fgets( current, sizeof( current ), fp ) != NULL ) {
				char *nl = strchr( current, '\n' );
				if ( nl ) *nl = '\0';
			}
			fclose( fp );
		}

		if ( pending[0] == '\0' ) {
			snprintf( msg, sizeof( msg ),
				"Release staged but version unknown. Run 'deploybot validate'." );
		} else if ( current[0] == '\0' ) {
			snprintf( msg, sizeof( msg ),
				"New version %s ready (fresh install). Run 'deploybot validate'.",
				pending );
		} else {
			snprintf( msg, sizeof( msg ),
				"New version %s ready (current: %s). Run 'deploybot validate'.",
				pending, current );
		}

		deploybot_broadcast( msg );
		send_to_char( msg, ch );
		send_to_char( "\n\r", ch );
		return;
	}

	/* Validate subcommand. */
	if ( str_cmp( arg, "validate" ) != 0
		&& str_cmp( arg, "migrate" ) != 0 ) {
		send_to_char( "Usage: deploybot <check|validate|migrate|status>\n\r", ch );
		return;
	}

	/* Only one task at a time. */
	if ( deploy_pid > 0 ) {
		snprintf( msg, sizeof( msg ),
			"DeployBot is already running '%s'. Wait for it to finish.\n\r",
			deploy_stage );
		send_to_char( msg, ch );
		return;
	}

	/* Find the script. */
	if ( deploy_script_path( script_path, sizeof( script_path ), arg ) != 0 ) {
		snprintf( msg, sizeof( msg ),
			"Script not found or not executable: %s.sh\n\r"
			"Ensure server/scripts/ is set up relative to gamedata/.\n\r", arg );
		send_to_char( msg, ch );
		return;
	}

	/* Prepare output file. */
	snprintf( deploy_output_path, sizeof( deploy_output_path ),
		"%s/deploy_%s.log", mud_run_dir, arg );

	/* Fork child to run the script. */
	pid = fork();

	if ( pid < 0 ) {
		send_to_char( "DeployBot: fork() failed.\n\r", ch );
		log_string( "deploybot: fork() failed" );
		return;
	}

	if ( pid == 0 ) {
		/* Child process — run the script with output redirected. */
		FILE *fp = freopen( deploy_output_path, "w", stdout );
		if ( fp == NULL ) _exit( 127 );
		dup2( fileno( stdout ), fileno( stderr ) );

		execl( "/bin/sh", "sh", script_path, (char *) NULL );
		_exit( 127 ); /* exec failed */
	}

	/* Parent — record state and announce. */
	deploy_pid = pid;
	snprintf( deploy_stage, sizeof( deploy_stage ), "%s", arg );

	snprintf( msg, sizeof( msg ), "Starting %s...", deploy_stage );
	deploybot_broadcast( msg );

	snprintf( msg, sizeof( msg ), "%s started %s via DeployBot.",
		ch->name, deploy_stage );
	log_string( msg );
}

/*
 * Called each pulse from game_tick(). Checks if a deploy child process
 * has finished and broadcasts the results.
 */
void deploybot_check( void ) {
	int status;
	pid_t result;
	char output[MAX_STRING_LENGTH];
	char msg[MAX_STRING_LENGTH];

	if ( deploy_pid <= 0 )
		return;

	result = waitpid( deploy_pid, &status, WNOHANG );

	if ( result == 0 )
		return; /* still running */

	if ( result < 0 ) {
		/* waitpid error — child may have been reaped already. */
		snprintf( msg, sizeof( msg ), "%s finished (status unknown).", deploy_stage );
		deploybot_broadcast( msg );
		deploy_pid = -1;
		return;
	}

	/* Child finished — read output and broadcast. */
	deploy_read_output( deploy_output_path, output, sizeof( output ) );

	if ( WIFEXITED( status ) && WEXITSTATUS( status ) == 0 ) {
		/* Find the last non-empty line for a summary. */
		char *last_line = strrchr( output, '\n' );
		if ( last_line != NULL && last_line != output ) {
			*last_line = '\0';
			last_line = strrchr( output, '\n' );
		}
		if ( last_line != NULL )
			last_line++;
		else
			last_line = output;

		/* Skip the [stage] prefix if present. */
		char prefix[48];
		snprintf( prefix, sizeof( prefix ), "[%s] ", deploy_stage );
		if ( strncmp( last_line, prefix, strlen( prefix ) ) == 0 )
			last_line += strlen( prefix );

		snprintf( msg, sizeof( msg ), "%s complete: %s", deploy_stage, last_line );
		deploybot_broadcast( msg );
	} else {
		int code = WIFEXITED( status ) ? WEXITSTATUS( status ) : -1;

		/* Find the last meaningful line. */
		char *last_line = strrchr( output, '\n' );
		if ( last_line != NULL && last_line != output ) {
			*last_line = '\0';
			last_line = strrchr( output, '\n' );
		}
		if ( last_line != NULL )
			last_line++;
		else
			last_line = output;

		snprintf( msg, sizeof( msg ), "%s FAILED (exit %d): %s",
			deploy_stage, code, last_line );
		deploybot_broadcast( msg );
	}

	/* Clean up. */
	unlink( deploy_output_path );
	deploy_pid = -1;
	deploy_stage[0] = '\0';

	snprintf( msg, sizeof( msg ), "deploybot: %s finished with status %d",
		deploy_stage, WIFEXITED( status ) ? WEXITSTATUS( status ) : -1 );
	log_string( msg );
}

#endif /* !WIN32 */
