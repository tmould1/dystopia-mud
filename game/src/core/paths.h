#ifndef PATHS_H
#define PATHS_H

/*
 * Data files used by the server.
 *
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */

/*
 * Path management - allows executable to run from any directory
 */
#define MUD_PATH_MAX 512

#if defined( WIN32 )
#define PATH_SEPARATOR "\\"
#define NULL_FILE	   "nul"
#else
#define PATH_SEPARATOR "/"
#define NULL_FILE	   "/dev/null"
#endif

/* Base directory paths - initialized at startup */
extern char mud_base_dir[MUD_PATH_MAX];	  /* Base directory (gamedata/) */
extern char mud_run_dir[MUD_PATH_MAX];	  /* Runtime files (copyover, crash, shutdown) */
extern char mud_log_dir[MUD_PATH_MAX];	  /* Log files */

/* Path building function - returns static buffer, use immediately or copy */
char *mud_path( const char *dir, const char *filename );

/* Initialize all paths based on executable location or current directory */
void mud_init_paths( const char *exe_path );

#define SHUTDOWN_FILE	mud_path( mud_run_dir, "shutdown.txt" )
#define CRASH_TEMP_FILE mud_path( mud_run_dir, "crashtmp.txt" )
#define COPYOVER_FILE	mud_path( mud_run_dir, "copyover.data" )
#define UPTIME_FILE		mud_path( mud_run_dir, "uptime.dat" )

/* Windows socket info file for copyover (WSADuplicateSocket data) */
#if defined( WIN32 )
#define COPYOVER_SOCKET_FILE mud_path( mud_run_dir, "copyover_socket.dat" )
#endif

/* Executable file for copyover/crash recovery */
#if defined( WIN32 )
#define EXE_FILE	 mud_path( mud_base_dir, "dystopia.exe" )
#define EXE_FILE_NEW mud_path( mud_base_dir, "dystopia_new.exe" )
#else
#define EXE_FILE mud_path( mud_base_dir, "dystopia" )
#endif

#endif /* PATHS_H */
