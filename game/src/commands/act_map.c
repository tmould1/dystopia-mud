/***************************************************************************
 *  ASCII Map Command for Dystopia MUD                                     *
 *                                                                         *
 *  Provides in-game mapping functionality showing nearby rooms.           *
 *  Displays one-way exits, non-Euclidean connections, and dead-ends.      *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"

/* Maximum map radius (creates 2*radius+1 square grid) */
#define MAX_MAP_RADIUS 5
#define MAX_MAP_SIZE   ( MAX_MAP_RADIUS * 2 + 1 )

/* Map cell types */
#define MAP_EMPTY	 0
#define MAP_ROOM	 1
#define MAP_PLAYER	 2
#define MAP_DEAD_END 3

/* Direction offsets: N=0, E=1, S=2, W=3, U=4, D=5 */
static const int dir_x[] = { 0, 1, 0, -1, 0, 0 };
static const int dir_y[] = { 1, 0, -1, 0, 0, 0 };

/* Reverse directions - use map_rev_dir to avoid conflict with global rev_dir */
static const sh_int map_rev_dir[] = { 2, 3, 0, 1, 5, 4 };

/* Map data structure */
typedef struct map_data {
	int cell[MAX_MAP_SIZE][MAX_MAP_SIZE];
	ROOM_INDEX_DATA *rooms[MAX_MAP_SIZE][MAX_MAP_SIZE];
	bool has_up[MAX_MAP_SIZE][MAX_MAP_SIZE];
	bool has_down[MAX_MAP_SIZE][MAX_MAP_SIZE];
	/* Exit info: 0=none, 1=bidirectional, 2=one-way out, 3=warped */
	int exit_n[MAX_MAP_SIZE][MAX_MAP_SIZE];
	int exit_e[MAX_MAP_SIZE][MAX_MAP_SIZE];
	int exit_s[MAX_MAP_SIZE][MAX_MAP_SIZE];
	int exit_w[MAX_MAP_SIZE][MAX_MAP_SIZE];
} MAP_DATA;

/* Local function prototypes */
static void fill_map_bfs args( ( ROOM_INDEX_DATA * start, MAP_DATA *map,
	int cx, int cy, int radius ) );
static bool is_exit_one_way args( ( ROOM_INDEX_DATA * from, int dir ) );
static void render_map args( ( CHAR_DATA * ch, MAP_DATA *map, int radius ) );

/*
 * Fill the map using BFS from the starting room.
 */
static void fill_map_bfs( ROOM_INDEX_DATA *start, MAP_DATA *map,
	int cx, int cy, int radius ) {
	typedef struct {
		ROOM_INDEX_DATA *room;
		int x, y;
	} queue_entry;

	queue_entry queue[MAX_MAP_SIZE * MAX_MAP_SIZE];
	bool visited[MAX_MAP_SIZE][MAX_MAP_SIZE];
	int head = 0, tail = 0;
	int dir;
	int size = radius * 2 + 1;

	/* Initialize */
	memset( visited, 0, sizeof( visited ) );

	/* Start with player's room */
	map->cell[cx][cy] = MAP_PLAYER;
	map->rooms[cx][cy] = start;
	visited[cx][cy] = TRUE;

	/* Check for up/down exits in starting room */
	if ( start->exit[DIR_UP] && start->exit[DIR_UP]->to_room )
		map->has_up[cx][cy] = TRUE;
	if ( start->exit[DIR_DOWN] && start->exit[DIR_DOWN]->to_room )
		map->has_down[cx][cy] = TRUE;

	/* Add starting room to queue */
	queue[tail].room = start;
	queue[tail].x = cx;
	queue[tail].y = cy;
	tail++;

	/* BFS loop */
	while ( head < tail ) {
		ROOM_INDEX_DATA *current = queue[head].room;
		int cur_x = queue[head].x;
		int cur_y = queue[head].y;
		head++;

		/* Check each cardinal direction (N, E, S, W only for 2D map) */
		for ( dir = 0; dir < 4; dir++ ) {
			EXIT_DATA *pexit;
			ROOM_INDEX_DATA *to_room;
			int new_x, new_y;
			int exit_type;

			pexit = current->exit[dir];
			if ( !pexit || !pexit->to_room )
				continue;

			/* Skip closed doors for mapping? Optional - include them */
			to_room = pexit->to_room;
			new_x = cur_x + dir_x[dir];
			new_y = cur_y + dir_y[dir];

			/* Check bounds */
			if ( new_x < 0 || new_x >= size || new_y < 0 || new_y >= size )
				continue;

			/* Determine exit type */
			if ( is_exit_one_way( current, dir ) )
				exit_type = 2; /* One-way */
			else
				exit_type = 1; /* Bidirectional */

			/* Store exit info based on direction */
			switch ( dir ) {
			case DIR_NORTH:
				map->exit_n[cur_x][cur_y] = exit_type;
				break;
			case DIR_EAST:
				map->exit_e[cur_x][cur_y] = exit_type;
				break;
			case DIR_SOUTH:
				map->exit_s[cur_x][cur_y] = exit_type;
				break;
			case DIR_WEST:
				map->exit_w[cur_x][cur_y] = exit_type;
				break;
			}

			/* Check if already visited */
			if ( visited[new_x][new_y] ) {
				/* Check for non-Euclidean (warped) connection */
				if ( map->rooms[new_x][new_y] != to_room ) {
					/* Different room at expected location = warped */
					switch ( dir ) {
					case DIR_NORTH:
						map->exit_n[cur_x][cur_y] = 3;
						break;
					case DIR_EAST:
						map->exit_e[cur_x][cur_y] = 3;
						break;
					case DIR_SOUTH:
						map->exit_s[cur_x][cur_y] = 3;
						break;
					case DIR_WEST:
						map->exit_w[cur_x][cur_y] = 3;
						break;
					}
				}
				continue;
			}

			/* Mark as visited and add to map */
			visited[new_x][new_y] = TRUE;
			map->rooms[new_x][new_y] = to_room;

			/* Determine cell type */
			if ( to_room->exit[0] == NULL && to_room->exit[1] == NULL &&
				to_room->exit[2] == NULL && to_room->exit[3] == NULL &&
				to_room->exit[4] == NULL && to_room->exit[5] == NULL ) {
				map->cell[new_x][new_y] = MAP_DEAD_END;
			} else {
				map->cell[new_x][new_y] = MAP_ROOM;
			}

			/* Check for up/down exits */
			if ( to_room->exit[DIR_UP] && to_room->exit[DIR_UP]->to_room )
				map->has_up[new_x][new_y] = TRUE;
			if ( to_room->exit[DIR_DOWN] && to_room->exit[DIR_DOWN]->to_room )
				map->has_down[new_x][new_y] = TRUE;

			/* Add to queue for further exploration */
			queue[tail].room = to_room;
			queue[tail].x = new_x;
			queue[tail].y = new_y;
			tail++;
		}
	}
}

/*
 * Check if an exit is one-way (no reverse exit back).
 */
static bool is_exit_one_way( ROOM_INDEX_DATA *from, int dir ) {
	EXIT_DATA *pexit;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *reverse;

	pexit = from->exit[dir];
	if ( !pexit || !pexit->to_room )
		return FALSE;

	to_room = pexit->to_room;
	reverse = to_room->exit[map_rev_dir[dir]];

	/* One-way if no reverse exit or reverse doesn't point back */
	if ( !reverse || !reverse->to_room || reverse->to_room != from )
		return TRUE;

	return FALSE;
}

/*
 * Render the map to the character.
 */
static void render_map( CHAR_DATA *ch, MAP_DATA *map, int radius ) {
	char buf[MAX_STRING_LENGTH];
	char line[MAX_STRING_LENGTH];
	int size = radius * 2 + 1;
	int x, y;

	/* Header */
	send_to_char( "\n\r", ch );

	/* Draw from top (north) to bottom (south) */
	for ( y = size - 1; y >= 0; y-- ) {
		/* Room row with horizontal exits - start with left margin */
		strcpy( line, "  " );
		for ( x = 0; x < size; x++ ) {
			int cell = map->cell[x][y];
			bool up = map->has_up[x][y];
			bool down = map->has_down[x][y];
			int exit_e = map->exit_e[x][y];
			int exit_w_next = ( x + 1 < size ) ? map->exit_w[x + 1][y] : 0;

			if ( cell == MAP_EMPTY ) {
				strcat( line, "   " ); /* 3 chars for empty room */
			} else {
				/* Room symbol - [X] is 3 visible chars */
				char room_str[32];

				if ( cell == MAP_PLAYER )
					snprintf( room_str, sizeof( room_str ), "#G[@]#n" );
				else if ( cell == MAP_DEAD_END )
					snprintf( room_str, sizeof( room_str ), "#R[!]#n" );
				else {
					/* Show up/down indicator inside the room */
					char ud_char = ' ';
					if ( up && down )
						ud_char = '*';
					else if ( up )
						ud_char = '^';
					else if ( down )
						ud_char = 'v';
					snprintf( room_str, sizeof( room_str ), "#C[%c]#n", ud_char );
				}

				strcat( line, room_str );
			}

			/* Horizontal exit between this room and next (if not last column) */
			if ( x < size - 1 ) {
				int next_cell = map->cell[x + 1][y];
				if ( ( cell != MAP_EMPTY && next_cell != MAP_EMPTY ) &&
					( exit_e > 0 || exit_w_next > 0 ) ) {
					int type = ( exit_e > exit_w_next ) ? exit_e : exit_w_next;
					if ( type == 3 )
						strcat( line, "#R~#n" ); /* Warped */
					else if ( type == 2 )
						strcat( line, "#Y-#n" ); /* One-way */
					else
						strcat( line, "#w-#n" ); /* Normal */
				} else {
					strcat( line, " " );
				}
			}
		}
		snprintf( buf, sizeof( buf ), "%s\n\r", line );
		send_to_char( buf, ch );

		/* Second line: vertical exits (between rows) */
		if ( y > 0 ) {
			/* Start with left margin */
			strcpy( line, "  " );
			for ( x = 0; x < size; x++ ) {
				int exit_s = map->exit_s[x][y];
				int exit_n_below = map->exit_n[x][y - 1];

				/* Room position is 3 chars wide, center the vertical connector */
				if ( exit_s > 0 || exit_n_below > 0 ) {
					int type = ( exit_s > exit_n_below ) ? exit_s : exit_n_below;
					if ( type == 3 )
						strcat( line, " #R~#n " ); /* Warped - 3 visible chars */
					else if ( type == 2 )
						strcat( line, " #Yv#n " ); /* One-way - 3 visible chars */
					else
						strcat( line, " #w|#n " ); /* Normal - 3 visible chars */
				} else {
					strcat( line, "   " ); /* 3 spaces for empty */
				}

				/* Add spacing for horizontal connector position (1 char) */
				if ( x < size - 1 ) {
					strcat( line, " " );
				}
			}
			snprintf( buf, sizeof( buf ), "%s\n\r", line );
			send_to_char( buf, ch );
		}
	}

	/* Legend */
	send_to_char( "\n\r", ch );
	send_to_char( "  #wLegend:#n #G[@]#n=You #C[ ]#n=Room #R[!]#n=Dead-end\n\r", ch );
	send_to_char( "          #w|#n=Exit #Y-v#n=One-way #R~#n=Warped #w^v*#n=Up/Down\n\r", ch );
}

/*
 * The map command.
 * Syntax: map [radius]
 */
void do_map( CHAR_DATA *ch, char *argument ) {
	MAP_DATA map;
	int radius = 2; /* Default 5x5 */
	int center;
	char arg[MAX_INPUT_LENGTH];

	if ( ch->in_room == NULL ) {
		send_to_char( "You are nowhere!\n\r", ch );
		return;
	}

	/* Parse optional radius argument */
	one_argument( argument, arg );
	if ( arg[0] != '\0' ) {
		radius = atoi( arg );
		if ( radius < 1 )
			radius = 1;
		if ( radius > MAX_MAP_RADIUS )
			radius = MAX_MAP_RADIUS;
	}

	center = radius;

	/* Initialize map data */
	memset( &map, 0, sizeof( map ) );

	/* Fill the map using BFS */
	fill_map_bfs( ch->in_room, &map, center, center, radius );

	/* Render and send to character */
	render_map( ch, &map, radius );

	/* Show current room info */
	if ( ch->in_room->name ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "\n\r  #wCurrent:#n %s #w(#n%d#w)#n\n\r",
			ch->in_room->name, ch->in_room->vnum );
		send_to_char( buf, ch );
	}
}

/*
 * Builder map command - shows VNUMs.
 * Syntax: amap [radius]
 */
void do_amap( CHAR_DATA *ch, char *argument ) {
	MAP_DATA map;
	char buf[MAX_STRING_LENGTH];
	char line[MAX_STRING_LENGTH];
	int radius = 3;
	int center;
	int size;
	int x, y;
	char arg[MAX_INPUT_LENGTH];

	if ( ch->in_room == NULL ) {
		send_to_char( "You are nowhere!\n\r", ch );
		return;
	}

	/* Check for builder access */
	if ( ch->level < 7 ) {
		send_to_char( "This command is for builders only.\n\r", ch );
		return;
	}

	/* Parse optional radius argument */
	one_argument( argument, arg );
	if ( arg[0] != '\0' ) {
		radius = atoi( arg );
		if ( radius < 1 )
			radius = 1;
		if ( radius > MAX_MAP_RADIUS )
			radius = MAX_MAP_RADIUS;
	}

	center = radius;
	size = radius * 2 + 1;

	/* Initialize and fill map */
	memset( &map, 0, sizeof( map ) );
	fill_map_bfs( ch->in_room, &map, center, center, radius );

	/* Render with VNUMs */
	send_to_char( "\n\r  #wArea Map (Builder View)#n\n\r\n\r", ch );

	for ( y = size - 1; y >= 0; y-- ) {
		/* Start with left margin */
		strcpy( line, "  " );
		for ( x = 0; x < size; x++ ) {
			ROOM_INDEX_DATA *room = map.rooms[x][y];
			int cell = map.cell[x][y];

			if ( cell == MAP_EMPTY || !room ) {
				strcat( line, "      " );
			} else if ( cell == MAP_PLAYER ) {
				snprintf( buf, sizeof( buf ), "#G%5d#n ", room->vnum );
				strcat( line, buf );
			} else if ( cell == MAP_DEAD_END ) {
				snprintf( buf, sizeof( buf ), "#R%5d#n ", room->vnum );
				strcat( line, buf );
			} else {
				snprintf( buf, sizeof( buf ), "#C%5d#n ", room->vnum );
				strcat( line, buf );
			}
		}
		snprintf( buf, sizeof( buf ), "%s\n\r", line );
		send_to_char( buf, ch );

		/* Connection line */
		if ( y > 0 ) {
			/* Start with left margin */
			strcpy( line, "  " );
			for ( x = 0; x < size; x++ ) {
				int exit_s = map.exit_s[x][y];
				if ( exit_s > 0 )
					strcat( line, "  |   " );
				else
					strcat( line, "      " );
			}
			snprintf( buf, sizeof( buf ), "%s\n\r", line );
			send_to_char( buf, ch );
		}
	}

	/* Current room details */
	snprintf( buf, sizeof( buf ), "\n\r  #wCurrent:#n %s\n\r", ch->in_room->name );
	send_to_char( buf, ch );
	snprintf( buf, sizeof( buf ), "  #wVNUM:#n %d  #wArea:#n %s\n\r",
		ch->in_room->vnum,
		ch->in_room->area ? ch->in_room->area->name : "Unknown" );
	send_to_char( buf, ch );
}
