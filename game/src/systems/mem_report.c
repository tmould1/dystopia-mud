/***************************************************************************
 *  File: mem_report.c                                                     *
 *                                                                         *
 *  Exact memory usage reporting with per-category drill-down.             *
 *  Walks live data structures at query time — zero runtime overhead.      *
 *                                                                         *
 *  Usage:  memory              — summary table                            *
 *          memory characters   — active characters detail                 *
 *          memory objects      — active objects detail                     *
 *          memory rooms        — rooms + exits + extra descs              *
 *          memory mobs         — mob index templates                      *
 *          memory objs         — obj index templates                      *
 *          memory descriptors  — network connections                      *
 *          memory helps        — help entries                             *
 *          memory areas        — per-area breakdown                       *
 *          memory scripts      — Lua scripts                             *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "merc.h"
#include "../script/script.h"
#include "../db/db_quest.h"

/*
 * Accumulated stats for one memory category.
 */
typedef struct {
	const char *name;
	int         count;
	size_t      struct_bytes;
	size_t      string_bytes;
	size_t      child_bytes;
} mem_category_t;

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */

static size_t str_mem( const char *s ) {
	return s ? strlen( s ) + 1 : 0;
}

static const char *format_bytes( size_t bytes, char *buf, size_t buflen ) {
	if ( bytes >= 1048576 )
		snprintf( buf, buflen, "%7.1f MB", (double) bytes / 1048576.0 );
	else if ( bytes >= 1024 )
		snprintf( buf, buflen, "%7.1f KB", (double) bytes / 1024.0 );
	else
		snprintf( buf, buflen, "%7zu B ", bytes );
	return buf;
}

static size_t category_total( const mem_category_t *c ) {
	return c->struct_bytes + c->string_bytes + c->child_bytes;
}

static void send_line( CHAR_DATA *ch, const char *fmt, ... ) {
	char buf[MAX_STRING_LENGTH];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args );
	send_to_char( buf, ch );
}

/* -----------------------------------------------------------------------
 * Category reporters
 * ----------------------------------------------------------------------- */

static mem_category_t mem_characters( int *out_npcs, int *out_players,
	int *out_pc_data, int *out_affects, int *out_aliases,
	int *out_editors, int *out_trackers,
	size_t *field_bytes, int field_count )
{
	mem_category_t r = { "Characters", 0, 0, 0, 0 };
	CHAR_DATA *ch;
	int npcs = 0, players = 0, pc_count = 0, aff_count = 0;
	int alias_count = 0, editor_count = 0, tracker_count = 0;
	int fi;

	/* field_bytes array indices for per-field breakdown (CHAR_DATA strings) */
	enum {
		F_NAME, F_SHORT, F_LONG, F_DESC, F_LORD, F_MORPH,
		F_CREATETIME, F_LASTTIME, F_LASTHOST, F_HUNTING, F_PLOAD,
		F_POWERACTION, F_POWERTYPE, F_PROMPT, F_CPROMPT, F_PREFIX,
		F_CLAN, F_OBJDESC, F_INTER_EDITING,
		/* PC_DATA strings */
		FP_PWD, FP_BAMFIN, FP_BAMFOUT, FP_TITLE, FP_CONCEPTION,
		FP_PARENTS, FP_CPARENTS, FP_MARRIAGE, FP_SWITCHNAME,
		FP_DECAPMESSAGE, FP_LOGINMESSAGE, FP_LOGOUTMESSAGE,
		FP_AVATARMESSAGE, FP_TIEMESSAGE, FP_LAST_DECAP0, FP_LAST_DECAP1,
		F_MAX
	};

	if ( field_bytes ) {
		for ( fi = 0; fi < field_count && fi < F_MAX; fi++ )
			field_bytes[fi] = 0;
	}

	LIST_FOR_EACH( ch, &g_characters, CHAR_DATA, char_node ) {
		size_t s;
		r.count++;
		r.struct_bytes += sizeof( CHAR_DATA );

		if ( IS_NPC( ch ) ) npcs++; else players++;

#define FTRACK( idx, ptr ) do { s = str_mem( ptr ); r.string_bytes += s; \
	if ( field_bytes && (idx) < field_count ) field_bytes[(idx)] += s; } while(0)

		/* Flyweight: NPC content strings shared with template aren't counted
		 * here (they're already counted in the mob_index report).  Only
		 * count if the pointer was replaced (summoned-pet customization). */
		if ( IS_NPC( ch ) && ch->pIndexData ) {
			MOB_INDEX_DATA *idx = ch->pIndexData;
			if ( ch->name != idx->player_name )
				FTRACK( F_NAME, ch->name );
			if ( ch->short_descr != idx->short_descr )
				FTRACK( F_SHORT, ch->short_descr );
			if ( ch->long_descr != idx->long_descr )
				FTRACK( F_LONG, ch->long_descr );
			if ( ch->description != idx->description )
				FTRACK( F_DESC, ch->description );
		} else {
			FTRACK( F_NAME, ch->name );
			FTRACK( F_SHORT, ch->short_descr );
			FTRACK( F_LONG, ch->long_descr );
			FTRACK( F_DESC, ch->description );
		}
		FTRACK( F_LORD, ch->lord );
		FTRACK( F_MORPH, ch->morph );
		FTRACK( F_HUNTING, ch->hunting );
		FTRACK( F_PREFIX, ch->prefix );
		FTRACK( F_CLAN, ch->clan );

		if ( ch->pcdata ) {
			FTRACK( F_INTER_EDITING, ch->pcdata->inter_editing );
			FTRACK( F_OBJDESC, ch->pcdata->objdesc );
			FTRACK( F_CREATETIME, ch->pcdata->createtime );
			FTRACK( F_LASTTIME, ch->pcdata->lasttime );
			FTRACK( F_LASTHOST, ch->pcdata->lasthost );
			FTRACK( F_PLOAD, ch->pcdata->pload );
			FTRACK( F_POWERACTION, ch->pcdata->poweraction );
			FTRACK( F_POWERTYPE, ch->pcdata->powertype );
			FTRACK( F_PROMPT, ch->pcdata->prompt );
			FTRACK( F_CPROMPT, ch->pcdata->cprompt );
			pc_count++;
			r.child_bytes += sizeof( PC_DATA );

			FTRACK( FP_PWD, ch->pcdata->pwd );
			FTRACK( FP_BAMFIN, ch->pcdata->bamfin );
			FTRACK( FP_BAMFOUT, ch->pcdata->bamfout );
			FTRACK( FP_TITLE, ch->pcdata->title );
			FTRACK( FP_CONCEPTION, ch->pcdata->conception );
			FTRACK( FP_PARENTS, ch->pcdata->parents );
			FTRACK( FP_CPARENTS, ch->pcdata->cparents );
			FTRACK( FP_MARRIAGE, ch->pcdata->marriage );
			FTRACK( FP_SWITCHNAME, ch->pcdata->switchname );
			FTRACK( FP_DECAPMESSAGE, ch->pcdata->decapmessage );
			FTRACK( FP_LOGINMESSAGE, ch->pcdata->loginmessage );
			FTRACK( FP_LOGOUTMESSAGE, ch->pcdata->logoutmessage );
			FTRACK( FP_AVATARMESSAGE, ch->pcdata->avatarmessage );
			FTRACK( FP_TIEMESSAGE, ch->pcdata->tiemessage );
			FTRACK( FP_LAST_DECAP0, ch->pcdata->last_decap[0] );
			FTRACK( FP_LAST_DECAP1, ch->pcdata->last_decap[1] );
			{
				ALIAS_DATA *alias;
				LIST_FOR_EACH( alias, &ch->pcdata->aliases, ALIAS_DATA, node ) {
					alias_count++;
					r.child_bytes += sizeof( ALIAS_DATA );
					r.string_bytes += str_mem( alias->short_n );
					r.string_bytes += str_mem( alias->long_n );
				}
			}

			if ( ch->pcdata->quest_tracker ) {
				tracker_count++;
				r.child_bytes += sizeof( QUEST_TRACKER );
				r.child_bytes += (size_t) ch->pcdata->quest_tracker->capacity
					* sizeof( QUEST_PROGRESS );
			}

			if ( ch->pcdata->editor ) {
				editor_count++;
				r.child_bytes += sizeof( EDITOR_DATA );
			}
		}

		{
			AFFECT_DATA *aff;
			LIST_FOR_EACH( aff, &ch->affects, AFFECT_DATA, node ) {
				aff_count++;
				r.child_bytes += sizeof( AFFECT_DATA );
			}
		}
#undef FTRACK
	}

	if ( out_npcs )     *out_npcs     = npcs;
	if ( out_players )  *out_players  = players;
	if ( out_pc_data )  *out_pc_data  = pc_count;
	if ( out_affects )  *out_affects  = aff_count;
	if ( out_aliases )  *out_aliases  = alias_count;
	if ( out_editors )  *out_editors  = editor_count;
	if ( out_trackers ) *out_trackers = tracker_count;
	return r;
}

static mem_category_t mem_objects( int *out_affects, int *out_ed,
	size_t *field_bytes, int field_count )
{
	mem_category_t r = { "Objects", 0, 0, 0, 0 };
	OBJ_DATA *obj;
	int aff_count = 0, ed_count = 0;
	int fi;

	enum {
		F_NAME, F_SHORT, F_DESC,
		F_CHPOWERON, F_CHPOWEROFF, F_CHPOWERUSE,
		F_VICTPOWERON, F_VICTPOWEROFF, F_VICTPOWERUSE,
		F_QUESTMAKER, F_QUESTOWNER,
		F_MAX
	};

	if ( field_bytes ) {
		for ( fi = 0; fi < field_count && fi < F_MAX; fi++ )
			field_bytes[fi] = 0;
	}

	LIST_FOR_EACH( obj, &g_objects, OBJ_DATA, obj_node ) {
		size_t s;
		AFFECT_DATA *aff;
		EXTRA_DESCR_DATA *ed;

		r.count++;
		r.struct_bytes += sizeof( OBJ_DATA );

#define FTRACK( idx, ptr ) do { s = str_mem( ptr ); r.string_bytes += s; \
	if ( field_bytes && (idx) < field_count ) field_bytes[(idx)] += s; } while(0)

		FTRACK( F_NAME, obj->name );
		FTRACK( F_SHORT, obj->short_descr );
		FTRACK( F_DESC, obj->description );
		FTRACK( F_CHPOWERON, obj->chpoweron );
		FTRACK( F_CHPOWEROFF, obj->chpoweroff );
		FTRACK( F_CHPOWERUSE, obj->chpoweruse );
		FTRACK( F_VICTPOWERON, obj->victpoweron );
		FTRACK( F_VICTPOWEROFF, obj->victpoweroff );
		FTRACK( F_VICTPOWERUSE, obj->victpoweruse );
		FTRACK( F_QUESTMAKER, obj->questmaker );
		FTRACK( F_QUESTOWNER, obj->questowner );
#undef FTRACK

		LIST_FOR_EACH( aff, &obj->affects, AFFECT_DATA, node ) {
			aff_count++;
			r.child_bytes += sizeof( AFFECT_DATA );
		}

		LIST_FOR_EACH( ed, &obj->extra_descr, EXTRA_DESCR_DATA, node ) {
			ed_count++;
			r.child_bytes += sizeof( EXTRA_DESCR_DATA );
			r.string_bytes += str_mem( ed->keyword );
			r.string_bytes += str_mem( ed->description );
		}
	}

	if ( out_affects ) *out_affects = aff_count;
	if ( out_ed )      *out_ed      = ed_count;
	return r;
}

static mem_category_t mem_rooms( int *out_exits, int *out_ed, int *out_resets,
	int *out_scripts, int *out_dynamic, int *out_extras )
{
	mem_category_t r = { "Rooms", 0, 0, 0, 0 };
	AREA_DATA *pArea;
	ROOM_INDEX_DATA *pRoom;
	int exit_count = 0, ed_count = 0, reset_count = 0, script_count = 0;
	int dynamic_count = 0, extras_count = 0;
	int door, i;

	LIST_FOR_EACH( pArea, &g_areas, AREA_DATA, node ) {
	for ( pRoom = pArea->room_first; pRoom; pRoom = pRoom->next_in_area ) {
		EXTRA_DESCR_DATA *ed;
		RESET_DATA *rst;
		SCRIPT_DATA *scr;

		r.count++;
		r.struct_bytes += sizeof( ROOM_INDEX_DATA );
		r.string_bytes += str_mem( pRoom->name );
		r.string_bytes += str_mem( pRoom->description );
		if ( pRoom->dynamic ) {
			dynamic_count++;
			r.child_bytes += sizeof( ROOM_DYNAMIC_DATA );
			for ( i = 0; i < 5; i++ )
				r.string_bytes += str_mem( pRoom->dynamic->track[i] );
		}

		if ( pRoom->extras ) {
			extras_count++;
			r.child_bytes += sizeof( ROOM_EXTRAS );
		}

		for ( door = 0; door < 6; door++ ) {
			EXIT_DATA *pexit = pRoom->exit[door];
			if ( pexit ) {
				exit_count++;
				r.child_bytes += sizeof( EXIT_DATA );
				r.string_bytes += str_mem( pexit->keyword );
				r.string_bytes += str_mem( pexit->description );
			}
		}

		LIST_FOR_EACH( ed, room_extra_descrs( pRoom ), EXTRA_DESCR_DATA, node ) {
			ed_count++;
			r.child_bytes += sizeof( EXTRA_DESCR_DATA );
			r.string_bytes += str_mem( ed->keyword );
			r.string_bytes += str_mem( ed->description );
		}

		LIST_FOR_EACH( rst, &pRoom->resets, RESET_DATA, node ) {
			reset_count++;
			r.child_bytes += sizeof( RESET_DATA );
		}

		LIST_FOR_EACH( scr, room_scripts( pRoom ), SCRIPT_DATA, node ) {
			script_count++;
			r.child_bytes += sizeof( SCRIPT_DATA );
			r.string_bytes += str_mem( scr->name );
			r.string_bytes += str_mem( scr->code );
			r.string_bytes += str_mem( scr->pattern );
			r.string_bytes += str_mem( scr->library_name );
		}
	}
	}

	if ( out_exits )   *out_exits   = exit_count;
	if ( out_ed )      *out_ed      = ed_count;
	if ( out_resets )  *out_resets  = reset_count;
	if ( out_scripts ) *out_scripts = script_count;
	if ( out_dynamic ) *out_dynamic = dynamic_count;
	if ( out_extras )  *out_extras  = extras_count;
	return r;
}

static mem_category_t mem_mob_index( int *out_shops, int *out_scripts ) {
	mem_category_t r = { "Mob Index", 0, 0, 0, 0 };
	MOB_INDEX_DATA *pMob;
	int iHash, shop_count = 0, script_count = 0;

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
		for ( pMob = mob_index_hash[iHash]; pMob; pMob = pMob->next ) {
			SCRIPT_DATA *scr;

			r.count++;
			r.struct_bytes += sizeof( MOB_INDEX_DATA );

			r.string_bytes += str_mem( pMob->hunting );
			r.string_bytes += str_mem( pMob->player_name );
			r.string_bytes += str_mem( pMob->short_descr );
			r.string_bytes += str_mem( pMob->long_descr );
			r.string_bytes += str_mem( pMob->description );
			r.string_bytes += str_mem( pMob->lord );
			r.string_bytes += str_mem( pMob->morph );
			r.string_bytes += str_mem( pMob->createtime );
			r.string_bytes += str_mem( pMob->pload );
			r.string_bytes += str_mem( pMob->lasttime );
			r.string_bytes += str_mem( pMob->lasthost );
			r.string_bytes += str_mem( pMob->powertype );
			r.string_bytes += str_mem( pMob->poweraction );
			r.string_bytes += str_mem( pMob->prompt );
			r.string_bytes += str_mem( pMob->cprompt );

			if ( pMob->pShop ) {
				shop_count++;
				r.child_bytes += sizeof( SHOP_DATA );
			}

			LIST_FOR_EACH( scr, &pMob->scripts, SCRIPT_DATA, node ) {
				script_count++;
				r.child_bytes += sizeof( SCRIPT_DATA );
				r.string_bytes += str_mem( scr->name );
				r.string_bytes += str_mem( scr->code );
				r.string_bytes += str_mem( scr->pattern );
				r.string_bytes += str_mem( scr->library_name );
			}
		}
	}

	if ( out_shops )   *out_shops   = shop_count;
	if ( out_scripts ) *out_scripts = script_count;
	return r;
}

static mem_category_t mem_obj_index( int *out_affects, int *out_ed,
	int *out_scripts )
{
	mem_category_t r = { "Obj Index", 0, 0, 0, 0 };
	OBJ_INDEX_DATA *pObj;
	int iHash, aff_count = 0, ed_count = 0, script_count = 0;

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
		for ( pObj = obj_index_hash[iHash]; pObj; pObj = pObj->next ) {
			AFFECT_DATA *aff;
			EXTRA_DESCR_DATA *ed;
			SCRIPT_DATA *scr;

			r.count++;
			r.struct_bytes += sizeof( OBJ_INDEX_DATA );

			r.string_bytes += str_mem( pObj->name );
			r.string_bytes += str_mem( pObj->short_descr );
			r.string_bytes += str_mem( pObj->description );
			r.string_bytes += str_mem( pObj->chpoweron );
			r.string_bytes += str_mem( pObj->chpoweroff );
			r.string_bytes += str_mem( pObj->chpoweruse );
			r.string_bytes += str_mem( pObj->victpoweron );
			r.string_bytes += str_mem( pObj->victpoweroff );
			r.string_bytes += str_mem( pObj->victpoweruse );
			r.string_bytes += str_mem( pObj->questmaker );
			r.string_bytes += str_mem( pObj->questowner );

			LIST_FOR_EACH( aff, &pObj->affects, AFFECT_DATA, node ) {
				aff_count++;
				r.child_bytes += sizeof( AFFECT_DATA );
			}

			LIST_FOR_EACH( ed, &pObj->extra_descr, EXTRA_DESCR_DATA, node ) {
				ed_count++;
				r.child_bytes += sizeof( EXTRA_DESCR_DATA );
				r.string_bytes += str_mem( ed->keyword );
				r.string_bytes += str_mem( ed->description );
			}

			LIST_FOR_EACH( scr, &pObj->scripts, SCRIPT_DATA, node ) {
				script_count++;
				r.child_bytes += sizeof( SCRIPT_DATA );
				r.string_bytes += str_mem( scr->name );
				r.string_bytes += str_mem( scr->code );
				r.string_bytes += str_mem( scr->pattern );
				r.string_bytes += str_mem( scr->library_name );
			}
		}
	}

	if ( out_affects ) *out_affects = aff_count;
	if ( out_ed )      *out_ed      = ed_count;
	if ( out_scripts ) *out_scripts = script_count;
	return r;
}

static mem_category_t mem_descriptors( size_t *out_outbuf, size_t *out_compress ) {
	mem_category_t r = { "Descriptors", 0, 0, 0, 0 };
	DESCRIPTOR_DATA *d;
	size_t outbuf_total = 0, compress_total = 0;

	LIST_FOR_EACH( d, &g_descriptors, DESCRIPTOR_DATA, node ) {
		r.count++;
		r.struct_bytes += sizeof( DESCRIPTOR_DATA );
		r.string_bytes += str_mem( d->host );
		r.string_bytes += str_mem( d->showstr_head );

		if ( d->outbuf ) {
			outbuf_total += (size_t) d->outsize;
			r.child_bytes += (size_t) d->outsize;
		}
		if ( d->out_compress_buf ) {
			compress_total += COMPRESS_BUF_SIZE;
			r.child_bytes += COMPRESS_BUF_SIZE;
		}
	}

	if ( out_outbuf )   *out_outbuf   = outbuf_total;
	if ( out_compress ) *out_compress = compress_total;
	return r;
}

static mem_category_t mem_helps( void ) {
	mem_category_t r = { "Helps", 0, 0, 0, 0 };
	HELP_DATA *pHelp;

	LIST_FOR_EACH( pHelp, &g_helps, HELP_DATA, node ) {
		r.count++;
		r.struct_bytes += sizeof( HELP_DATA );
		r.string_bytes += str_mem( pHelp->keyword );
		r.string_bytes += str_mem( pHelp->text );
	}
	return r;
}

static mem_category_t mem_areas_summary( void ) {
	mem_category_t r = { "Areas", 0, 0, 0, 0 };
	AREA_DATA *pArea;

	LIST_FOR_EACH( pArea, &g_areas, AREA_DATA, node ) {
		r.count++;
		r.struct_bytes += sizeof( AREA_DATA );
		r.string_bytes += str_mem( pArea->name );
		r.string_bytes += str_mem( pArea->filename );
		r.string_bytes += str_mem( pArea->builders );
	}
	return r;
}

/* -----------------------------------------------------------------------
 * Summary view
 * ----------------------------------------------------------------------- */

static void mem_show_summary( CHAR_DATA *ch ) {
	char s_buf[32], st_buf[32], ch_buf[32], t_buf[32];
	mem_category_t cats[9];
	size_t grand_total = 0;
	int i;

	cats[0] = mem_characters( NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0 );
	cats[1] = mem_objects( NULL, NULL, NULL, 0 );
	cats[2] = mem_rooms( NULL, NULL, NULL, NULL, NULL, NULL );
	cats[3] = mem_mob_index( NULL, NULL );
	cats[4] = mem_obj_index( NULL, NULL, NULL );
	cats[5] = mem_descriptors( NULL, NULL );
	cats[6] = mem_helps();
	cats[7] = mem_areas_summary();

	send_to_char( "\n\r#R===== #yMemory Report #R=====#n\n\r\n\r", ch );
	send_to_char( "#CCategory       Count   Struct       Strings      Children     Total#n\n\r", ch );
	send_to_char( "-------------  ------  -----------  -----------  -----------  -----------\n\r", ch );

	for ( i = 0; i < 8; i++ ) {
		size_t total = category_total( &cats[i] );
		grand_total += total;
		format_bytes( cats[i].struct_bytes, s_buf, sizeof( s_buf ) );
		format_bytes( cats[i].string_bytes, st_buf, sizeof( st_buf ) );
		format_bytes( cats[i].child_bytes, ch_buf, sizeof( ch_buf ) );
		format_bytes( total, t_buf, sizeof( t_buf ) );
		send_line( ch, "%-13s  %6d  %s  %s  %s  %s\n\r",
			cats[i].name, cats[i].count,
			s_buf, st_buf, ch_buf, t_buf );
	}

	format_bytes( grand_total, t_buf, sizeof( t_buf ) );
	send_to_char( "-------------  ------  -----------  -----------  -----------  -----------\n\r", ch );
	send_line( ch, "#CTOTAL                                                        %s#n\n\r", t_buf );
}

/* -----------------------------------------------------------------------
 * Detail views
 * ----------------------------------------------------------------------- */

static void mem_show_characters( CHAR_DATA *ch ) {
	int npcs, players, pc_count, aff_count, alias_count, editor_count, tracker_count;
	char buf[32];
	mem_category_t r;

	/* 36 = 19 CHAR_DATA fields + 17 PC_DATA fields */
	size_t field_bytes[36];
	static const char *char_fields[] = {
		"name", "short_descr", "long_descr", "description",
		"lord", "morph", "createtime", "lasttime", "lasthost",
		"hunting", "pload", "poweraction", "powertype",
		"prompt", "cprompt", "prefix", "clan", "objdesc", "inter_editing"
	};
	static const char *pc_fields[] = {
		"pwd", "bamfin", "bamfout", "title", "conception",
		"parents", "cparents", "marriage", "switchname",
		"decapmessage", "loginmessage", "logoutmessage",
		"avatarmessage", "tiemessage", "last_decap[0]", "last_decap[1]"
	};
	int i;

	r = mem_characters( &npcs, &players, &pc_count, &aff_count,
		&alias_count, &editor_count, &tracker_count, field_bytes, 36 );

	send_to_char( "\n\r#R===== #yMemory Detail: Characters #R=====#n\n\r\n\r", ch );
	send_line( ch, "Instances:     %6d  (%d players, %d NPCs)\n\r", r.count, players, npcs );
	send_line( ch, "Struct size:   sizeof(CHAR_DATA) = %zu bytes\n\r\n\r", sizeof( CHAR_DATA ) );

	format_bytes( r.struct_bytes, buf, sizeof( buf ) );
	send_line( ch, "Struct total:  %12zu  (%s)\n\r\n\r", r.struct_bytes, buf );

	send_to_char( "#CChar string fields:#n\n\r", ch );
	for ( i = 0; i < 19; i++ ) {
		if ( field_bytes[i] > 0 )
			send_line( ch, "  %-18s %10zu\n\r", char_fields[i], field_bytes[i] );
	}

	if ( pc_count > 0 ) {
		send_line( ch, "\n\r#CPC_DATA string fields:#n  (%d players)\n\r", pc_count );
		for ( i = 0; i < 17; i++ ) {
			if ( field_bytes[19 + i] > 0 )
				send_line( ch, "  %-18s %10zu\n\r", pc_fields[i], field_bytes[19 + i] );
		}
	}

	format_bytes( r.string_bytes, buf, sizeof( buf ) );
	send_line( ch, "  %-18s %10zu  (%s)\n\r\n\r", "String total:", r.string_bytes, buf );

	send_to_char( "#CChild structures:#n\n\r", ch );
	if ( pc_count > 0 )
		send_line( ch, "  PC_DATA:         %10zu  (%d instances)\n\r",
			(size_t) pc_count * sizeof( PC_DATA ), pc_count );
	if ( aff_count > 0 )
		send_line( ch, "  Affects:         %10zu  (%d instances)\n\r",
			(size_t) aff_count * sizeof( AFFECT_DATA ), aff_count );
	if ( alias_count > 0 )
		send_line( ch, "  Aliases:         %10zu  (%d instances)\n\r",
			(size_t) alias_count * sizeof( ALIAS_DATA ), alias_count );
	if ( tracker_count > 0 )
		send_line( ch, "  Quest Trackers:  %10s  (%d instances)\n\r",
			"(variable)", tracker_count );
	if ( editor_count > 0 )
		send_line( ch, "  Editors:         %10zu  (%d instances)\n\r",
			(size_t) editor_count * sizeof( EDITOR_DATA ), editor_count );

	format_bytes( r.child_bytes, buf, sizeof( buf ) );
	send_line( ch, "  %-18s %10zu  (%s)\n\r\n\r", "Child total:", r.child_bytes, buf );

	format_bytes( category_total( &r ), buf, sizeof( buf ) );
	send_to_char( "------------------------------------------\n\r", ch );
	send_line( ch, "#CGrand total:       %10zu  (%s)#n\n\r", category_total( &r ), buf );
}

static void mem_show_objects( CHAR_DATA *ch ) {
	int aff_count, ed_count;
	char buf[32];
	mem_category_t r;

	size_t field_bytes[11];
	static const char *obj_fields[] = {
		"name", "short_descr", "description",
		"chpoweron", "chpoweroff", "chpoweruse",
		"victpoweron", "victpoweroff", "victpoweruse",
		"questmaker", "questowner"
	};
	int i;

	r = mem_objects( &aff_count, &ed_count, field_bytes, 11 );

	send_to_char( "\n\r#R===== #yMemory Detail: Objects #R=====#n\n\r\n\r", ch );
	send_line( ch, "Instances:     %6d\n\r", r.count );
	send_line( ch, "Struct size:   sizeof(OBJ_DATA) = %zu bytes\n\r\n\r", sizeof( OBJ_DATA ) );

	format_bytes( r.struct_bytes, buf, sizeof( buf ) );
	send_line( ch, "Struct total:  %12zu  (%s)\n\r\n\r", r.struct_bytes, buf );

	send_to_char( "#CString fields:#n\n\r", ch );
	for ( i = 0; i < 11; i++ ) {
		if ( field_bytes[i] > 0 )
			send_line( ch, "  %-18s %10zu\n\r", obj_fields[i], field_bytes[i] );
	}
	format_bytes( r.string_bytes, buf, sizeof( buf ) );
	send_line( ch, "  %-18s %10zu  (%s)\n\r\n\r", "String total:", r.string_bytes, buf );

	send_to_char( "#CChild structures:#n\n\r", ch );
	if ( aff_count > 0 )
		send_line( ch, "  Affects:         %10zu  (%d instances)\n\r",
			(size_t) aff_count * sizeof( AFFECT_DATA ), aff_count );
	if ( ed_count > 0 )
		send_line( ch, "  Extra descs:     %10zu  (%d instances)\n\r",
			(size_t) ed_count * sizeof( EXTRA_DESCR_DATA ), ed_count );

	format_bytes( r.child_bytes, buf, sizeof( buf ) );
	send_line( ch, "  %-18s %10zu  (%s)\n\r\n\r", "Child total:", r.child_bytes, buf );

	format_bytes( category_total( &r ), buf, sizeof( buf ) );
	send_to_char( "------------------------------------------\n\r", ch );
	send_line( ch, "#CGrand total:       %10zu  (%s)#n\n\r", category_total( &r ), buf );
}

static void mem_show_rooms( CHAR_DATA *ch ) {
	int exit_count, ed_count, reset_count, script_count, dynamic_count, extras_count;
	char buf[32];
	mem_category_t r;

	r = mem_rooms( &exit_count, &ed_count, &reset_count, &script_count,
		&dynamic_count, &extras_count );

	send_to_char( "\n\r#R===== #yMemory Detail: Rooms #R=====#n\n\r\n\r", ch );
	send_line( ch, "Instances:     %6d\n\r", r.count );
	send_line( ch, "Struct size:   sizeof(ROOM_INDEX_DATA) = %zu bytes\n\r\n\r", sizeof( ROOM_INDEX_DATA ) );

	format_bytes( r.struct_bytes, buf, sizeof( buf ) );
	send_line( ch, "Struct total:  %12zu  (%s)\n\r", r.struct_bytes, buf );
	format_bytes( r.string_bytes, buf, sizeof( buf ) );
	send_line( ch, "String total:  %12zu  (%s)\n\r\n\r", r.string_bytes, buf );

	send_to_char( "#CChild structures:#n\n\r", ch );
	send_line( ch, "  Exits:           %10zu  (%d instances)\n\r",
		(size_t) exit_count * sizeof( EXIT_DATA ), exit_count );
	send_line( ch, "  Extra descs:     %10zu  (%d instances)\n\r",
		(size_t) ed_count * sizeof( EXTRA_DESCR_DATA ), ed_count );
	send_line( ch, "  Resets:          %10zu  (%d instances)\n\r",
		(size_t) reset_count * sizeof( RESET_DATA ), reset_count );
	send_line( ch, "  Dynamic:         %10zu  (%d of %d rooms)\n\r",
		(size_t) dynamic_count * sizeof( ROOM_DYNAMIC_DATA ), dynamic_count, r.count );
	send_line( ch, "  Extras:          %10zu  (%d of %d rooms)\n\r",
		(size_t) extras_count * sizeof( ROOM_EXTRAS ), extras_count, r.count );
	send_line( ch, "  Scripts:         %10zu  (%d instances)\n\r",
		(size_t) script_count * sizeof( SCRIPT_DATA ), script_count );

	format_bytes( r.child_bytes, buf, sizeof( buf ) );
	send_line( ch, "  %-18s %10zu  (%s)\n\r\n\r", "Child total:", r.child_bytes, buf );

	format_bytes( category_total( &r ), buf, sizeof( buf ) );
	send_to_char( "------------------------------------------\n\r", ch );
	send_line( ch, "#CGrand total:       %10zu  (%s)#n\n\r", category_total( &r ), buf );
}

static void mem_show_mob_index( CHAR_DATA *ch ) {
	int shop_count, script_count;
	char buf[32];
	mem_category_t r;

	r = mem_mob_index( &shop_count, &script_count );

	send_to_char( "\n\r#R===== #yMemory Detail: Mob Index #R=====#n\n\r\n\r", ch );
	send_line( ch, "Instances:     %6d\n\r", r.count );
	send_line( ch, "Struct size:   sizeof(MOB_INDEX_DATA) = %zu bytes\n\r\n\r", sizeof( MOB_INDEX_DATA ) );

	format_bytes( r.struct_bytes, buf, sizeof( buf ) );
	send_line( ch, "Struct total:  %12zu  (%s)\n\r", r.struct_bytes, buf );
	format_bytes( r.string_bytes, buf, sizeof( buf ) );
	send_line( ch, "String total:  %12zu  (%s)\n\r\n\r", r.string_bytes, buf );

	send_to_char( "#CChild structures:#n\n\r", ch );
	send_line( ch, "  Shops:           %10zu  (%d instances)\n\r",
		(size_t) shop_count * sizeof( SHOP_DATA ), shop_count );
	send_line( ch, "  Scripts:         %10zu  (%d instances)\n\r",
		(size_t) script_count * sizeof( SCRIPT_DATA ), script_count );

	format_bytes( r.child_bytes, buf, sizeof( buf ) );
	send_line( ch, "  %-18s %10zu  (%s)\n\r\n\r", "Child total:", r.child_bytes, buf );

	format_bytes( category_total( &r ), buf, sizeof( buf ) );
	send_to_char( "------------------------------------------\n\r", ch );
	send_line( ch, "#CGrand total:       %10zu  (%s)#n\n\r", category_total( &r ), buf );
}

static void mem_show_obj_index( CHAR_DATA *ch ) {
	int aff_count, ed_count, script_count;
	char buf[32];
	mem_category_t r;

	r = mem_obj_index( &aff_count, &ed_count, &script_count );

	send_to_char( "\n\r#R===== #yMemory Detail: Obj Index #R=====#n\n\r\n\r", ch );
	send_line( ch, "Instances:     %6d\n\r", r.count );
	send_line( ch, "Struct size:   sizeof(OBJ_INDEX_DATA) = %zu bytes\n\r\n\r", sizeof( OBJ_INDEX_DATA ) );

	format_bytes( r.struct_bytes, buf, sizeof( buf ) );
	send_line( ch, "Struct total:  %12zu  (%s)\n\r", r.struct_bytes, buf );
	format_bytes( r.string_bytes, buf, sizeof( buf ) );
	send_line( ch, "String total:  %12zu  (%s)\n\r\n\r", r.string_bytes, buf );

	send_to_char( "#CChild structures:#n\n\r", ch );
	send_line( ch, "  Affects:         %10zu  (%d instances)\n\r",
		(size_t) aff_count * sizeof( AFFECT_DATA ), aff_count );
	send_line( ch, "  Extra descs:     %10zu  (%d instances)\n\r",
		(size_t) ed_count * sizeof( EXTRA_DESCR_DATA ), ed_count );
	send_line( ch, "  Scripts:         %10zu  (%d instances)\n\r",
		(size_t) script_count * sizeof( SCRIPT_DATA ), script_count );

	format_bytes( r.child_bytes, buf, sizeof( buf ) );
	send_line( ch, "  %-18s %10zu  (%s)\n\r\n\r", "Child total:", r.child_bytes, buf );

	format_bytes( category_total( &r ), buf, sizeof( buf ) );
	send_to_char( "------------------------------------------\n\r", ch );
	send_line( ch, "#CGrand total:       %10zu  (%s)#n\n\r", category_total( &r ), buf );
}

static void mem_show_descriptors( CHAR_DATA *ch ) {
	size_t outbuf_total, compress_total;
	char buf[32];
	mem_category_t r;

	r = mem_descriptors( &outbuf_total, &compress_total );

	send_to_char( "\n\r#R===== #yMemory Detail: Descriptors #R=====#n\n\r\n\r", ch );
	send_line( ch, "Instances:     %6d\n\r", r.count );
	send_line( ch, "Struct size:   sizeof(DESCRIPTOR_DATA) = %zu bytes\n\r\n\r", sizeof( DESCRIPTOR_DATA ) );

	format_bytes( r.struct_bytes, buf, sizeof( buf ) );
	send_line( ch, "Struct total:  %12zu  (%s)\n\r", r.struct_bytes, buf );
	format_bytes( r.string_bytes, buf, sizeof( buf ) );
	send_line( ch, "String total:  %12zu  (%s)\n\r\n\r", r.string_bytes, buf );

	send_to_char( "#CDynamic buffers:#n\n\r", ch );
	send_line( ch, "  Output bufs:     %10zu\n\r", outbuf_total );
	send_line( ch, "  Compress bufs:   %10zu  (MCCP %d bytes each)\n\r",
		compress_total, COMPRESS_BUF_SIZE );

	send_to_char( "\n\r#CFixed buffers (embedded in struct):#n\n\r", ch );
	send_line( ch, "  inbuf:         %zu bytes each\n\r", sizeof( ((DESCRIPTOR_DATA*)0)->inbuf ) );
	send_line( ch, "  incomm:        %zu bytes each\n\r", sizeof( ((DESCRIPTOR_DATA*)0)->incomm ) );
	send_line( ch, "  inlast:        %zu bytes each\n\r", sizeof( ((DESCRIPTOR_DATA*)0)->inlast ) );
	send_line( ch, "  client_name:   %zu bytes each\n\r", sizeof( ((DESCRIPTOR_DATA*)0)->client_name ) );
	send_line( ch, "  terminal_type: %zu bytes each\n\r\n\r", sizeof( ((DESCRIPTOR_DATA*)0)->terminal_type ) );

	format_bytes( category_total( &r ), buf, sizeof( buf ) );
	send_to_char( "------------------------------------------\n\r", ch );
	send_line( ch, "#CGrand total:       %10zu  (%s)#n\n\r", category_total( &r ), buf );
}

static void mem_show_helps( CHAR_DATA *ch ) {
	char buf[32];
	mem_category_t r = mem_helps();

	send_to_char( "\n\r#R===== #yMemory Detail: Helps #R=====#n\n\r\n\r", ch );
	send_line( ch, "Instances:     %6d\n\r", r.count );
	send_line( ch, "Struct size:   sizeof(HELP_DATA) = %zu bytes\n\r\n\r", sizeof( HELP_DATA ) );

	format_bytes( r.struct_bytes, buf, sizeof( buf ) );
	send_line( ch, "Struct total:  %12zu  (%s)\n\r", r.struct_bytes, buf );
	format_bytes( r.string_bytes, buf, sizeof( buf ) );
	send_line( ch, "String total:  %12zu  (%s)\n\r", r.string_bytes, buf );
	send_line( ch, "  (keyword + text per entry)\n\r\n\r", r.string_bytes );

	format_bytes( category_total( &r ), buf, sizeof( buf ) );
	send_to_char( "------------------------------------------\n\r", ch );
	send_line( ch, "#CGrand total:       %10zu  (%s)#n\n\r", category_total( &r ), buf );
}

static void mem_show_areas( CHAR_DATA *ch ) {
	AREA_DATA *pArea;
	ROOM_INDEX_DATA *pRoom;
	char t_buf[32];
	size_t grand_total = 0;
	int total_rooms = 0, total_mobs = 0, total_objs = 0;
	int total_resets = 0, total_scripts = 0;

	send_to_char( "\n\r#R===== #yMemory Detail: Areas #R=====#n\n\r\n\r", ch );
	send_to_char( "#CArea                  Rooms  Mobs  Objs  Resets  Scripts    Total#n\n\r", ch );
	send_to_char( "--------------------  -----  ----  ----  ------  -------  ---------\n\r", ch );

	LIST_FOR_EACH( pArea, &g_areas, AREA_DATA, node ) {
		int rooms = 0, mobs = 0, objs = 0, resets = 0, scripts = 0;
		size_t area_bytes = sizeof( AREA_DATA );
		MOB_INDEX_DATA *pMob;
		OBJ_INDEX_DATA *pObj;
		int iHash;

		area_bytes += str_mem( pArea->name );
		area_bytes += str_mem( pArea->filename );
		area_bytes += str_mem( pArea->builders );

		/* Walk rooms in this area via the area room list */
		for ( pRoom = pArea->room_first; pRoom; pRoom = pRoom->next_in_area ) {
			int door;
			EXTRA_DESCR_DATA *ed;
			RESET_DATA *rst;
			SCRIPT_DATA *scr;

			rooms++;
			area_bytes += sizeof( ROOM_INDEX_DATA );
			area_bytes += str_mem( pRoom->name );
			area_bytes += str_mem( pRoom->description );
			if ( pRoom->dynamic ) {
				area_bytes += sizeof( ROOM_DYNAMIC_DATA );
				for ( door = 0; door < 5; door++ )
					area_bytes += str_mem( pRoom->dynamic->track[door] );
			}

			if ( pRoom->extras )
				area_bytes += sizeof( ROOM_EXTRAS );

			for ( door = 0; door < 6; door++ ) {
				if ( pRoom->exit[door] ) {
					area_bytes += sizeof( EXIT_DATA );
					area_bytes += str_mem( pRoom->exit[door]->keyword );
					area_bytes += str_mem( pRoom->exit[door]->description );
				}
			}

			LIST_FOR_EACH( ed, room_extra_descrs( pRoom ), EXTRA_DESCR_DATA, node ) {
				area_bytes += sizeof( EXTRA_DESCR_DATA );
				area_bytes += str_mem( ed->keyword );
				area_bytes += str_mem( ed->description );
			}

			LIST_FOR_EACH( rst, &pRoom->resets, RESET_DATA, node ) {
				resets++;
				area_bytes += sizeof( RESET_DATA );
			}

			LIST_FOR_EACH( scr, room_scripts( pRoom ), SCRIPT_DATA, node ) {
				scripts++;
				area_bytes += sizeof( SCRIPT_DATA );
				area_bytes += str_mem( scr->name );
				area_bytes += str_mem( scr->code );
				area_bytes += str_mem( scr->pattern );
				area_bytes += str_mem( scr->library_name );
			}
		}

		/* Count mob indexes in this area's vnum range */
		for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
			for ( pMob = mob_index_hash[iHash]; pMob; pMob = pMob->next ) {
				if ( pMob->area == pArea ) {
					SCRIPT_DATA *scr;
					mobs++;
					area_bytes += sizeof( MOB_INDEX_DATA );
					if ( pMob->pShop )
						area_bytes += sizeof( SHOP_DATA );

					LIST_FOR_EACH( scr, &pMob->scripts, SCRIPT_DATA, node ) {
						scripts++;
						area_bytes += sizeof( SCRIPT_DATA );
						area_bytes += str_mem( scr->code );
					}
				}
			}
		}

		/* Count obj indexes in this area's vnum range */
		for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
			for ( pObj = obj_index_hash[iHash]; pObj; pObj = pObj->next ) {
				if ( pObj->area == pArea ) {
					objs++;
					area_bytes += sizeof( OBJ_INDEX_DATA );
				}
			}
		}

		format_bytes( area_bytes, t_buf, sizeof( t_buf ) );
		send_line( ch, "%-20.20s  %5d  %4d  %4d  %6d  %7d  %s\n\r",
			pArea->name, rooms, mobs, objs, resets, scripts, t_buf );

		grand_total += area_bytes;
		total_rooms += rooms;
		total_mobs += mobs;
		total_objs += objs;
		total_resets += resets;
		total_scripts += scripts;
	}

	format_bytes( grand_total, t_buf, sizeof( t_buf ) );
	send_to_char( "--------------------  -----  ----  ----  ------  -------  ---------\n\r", ch );
	send_line( ch, "#CTOTAL                 %5d  %4d  %4d  %6d  %7d  %s#n\n\r",
		total_rooms, total_mobs, total_objs, total_resets, total_scripts, t_buf );
}

static void mem_show_scripts( CHAR_DATA *ch ) {
	MOB_INDEX_DATA *pMob;
	OBJ_INDEX_DATA *pObj;
	ROOM_INDEX_DATA *pRoom;
	int iHash;
	int mob_scripts = 0, obj_scripts = 0, rm_scripts = 0;
	size_t mob_bytes = 0, obj_bytes = 0, room_bytes = 0;
	size_t code_bytes = 0;
	char buf[32];

	send_to_char( "\n\r#R===== #yMemory Detail: Scripts #R=====#n\n\r\n\r", ch );
	send_line( ch, "Struct size:   sizeof(SCRIPT_DATA) = %zu bytes\n\r\n\r", sizeof( SCRIPT_DATA ) );

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
		for ( pMob = mob_index_hash[iHash]; pMob; pMob = pMob->next ) {
			SCRIPT_DATA *scr;
			LIST_FOR_EACH( scr, &pMob->scripts, SCRIPT_DATA, node ) {
				mob_scripts++;
				mob_bytes += sizeof( SCRIPT_DATA );
				mob_bytes += str_mem( scr->name );
				mob_bytes += str_mem( scr->code );
				mob_bytes += str_mem( scr->pattern );
				mob_bytes += str_mem( scr->library_name );
				code_bytes += str_mem( scr->code );
			}
		}
	}

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
		for ( pObj = obj_index_hash[iHash]; pObj; pObj = pObj->next ) {
			SCRIPT_DATA *scr;
			LIST_FOR_EACH( scr, &pObj->scripts, SCRIPT_DATA, node ) {
				obj_scripts++;
				obj_bytes += sizeof( SCRIPT_DATA );
				obj_bytes += str_mem( scr->name );
				obj_bytes += str_mem( scr->code );
				obj_bytes += str_mem( scr->pattern );
				obj_bytes += str_mem( scr->library_name );
				code_bytes += str_mem( scr->code );
			}
		}
	}

	{
		AREA_DATA *pArea;
		LIST_FOR_EACH( pArea, &g_areas, AREA_DATA, node ) {
		for ( pRoom = pArea->room_first; pRoom; pRoom = pRoom->next_in_area ) {
			SCRIPT_DATA *scr;
			LIST_FOR_EACH( scr, room_scripts( pRoom ), SCRIPT_DATA, node ) {
				rm_scripts++;
				room_bytes += sizeof( SCRIPT_DATA );
				room_bytes += str_mem( scr->name );
				room_bytes += str_mem( scr->code );
				room_bytes += str_mem( scr->pattern );
				room_bytes += str_mem( scr->library_name );
				code_bytes += str_mem( scr->code );
			}
		}
		}
	}

	send_to_char( "#CBy owner type:#n\n\r", ch );
	format_bytes( mob_bytes, buf, sizeof( buf ) );
	send_line( ch, "  Mob scripts:     %4d  %s\n\r", mob_scripts, buf );
	format_bytes( obj_bytes, buf, sizeof( buf ) );
	send_line( ch, "  Obj scripts:     %4d  %s\n\r", obj_scripts, buf );
	format_bytes( room_bytes, buf, sizeof( buf ) );
	send_line( ch, "  Room scripts:    %4d  %s\n\r\n\r", rm_scripts, buf );

	{
		int total = mob_scripts + obj_scripts + rm_scripts;
		size_t total_bytes = mob_bytes + obj_bytes + room_bytes;

		format_bytes( code_bytes, buf, sizeof( buf ) );
		send_line( ch, "Lua code bytes:          %s\n\r", buf );
		format_bytes( total_bytes, buf, sizeof( buf ) );
		send_to_char( "------------------------------------------\n\r", ch );
		send_line( ch, "#CTotal:             %4d  %s#n\n\r", total, buf );
	}
}

/* -----------------------------------------------------------------------
 * Command entry point
 * ----------------------------------------------------------------------- */

void do_memory( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		mem_show_summary( ch );
		return;
	}

	if ( !str_cmp( arg, "characters" ) || !str_cmp( arg, "chars" ) ) {
		mem_show_characters( ch );
		return;
	}

	if ( !str_cmp( arg, "objects" ) ) {
		mem_show_objects( ch );
		return;
	}

	if ( !str_cmp( arg, "rooms" ) ) {
		mem_show_rooms( ch );
		return;
	}

	if ( !str_cmp( arg, "mobs" ) ) {
		mem_show_mob_index( ch );
		return;
	}

	if ( !str_cmp( arg, "objs" ) ) {
		mem_show_obj_index( ch );
		return;
	}

	if ( !str_cmp( arg, "descriptors" ) || !str_cmp( arg, "desc" ) ) {
		mem_show_descriptors( ch );
		return;
	}

	if ( !str_cmp( arg, "helps" ) ) {
		mem_show_helps( ch );
		return;
	}

	if ( !str_cmp( arg, "areas" ) ) {
		mem_show_areas( ch );
		return;
	}

	if ( !str_cmp( arg, "scripts" ) ) {
		mem_show_scripts( ch );
		return;
	}

	send_to_char( "Syntax: memory [characters|objects|rooms|mobs|objs|descriptors|helps|areas|scripts]\n\r", ch );
}
