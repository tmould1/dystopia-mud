/***************************************************************************
 *  Shaman class - spiritual conduits who straddle material and spirit     *
 *  worlds. Uses "Spirit Tether" resource (ch->rage) with balance          *
 *  mechanic. Places totems for zone control, channels spirits for         *
 *  offense/defense, and communes with ancestors for healing/utility.      *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"
#include "shaman.h"
#include "../db/db_class.h"

/* Forward declarations for class armor */
extern void do_classarmor_generic( CHAR_DATA *ch, char *argument, int class_id );

/* =========================================================================
 * TETHER SCALING HELPER
 *
 * Returns a percentage modifier (50-130) based on tether position.
 * Material abilities are stronger at low tether (grounded).
 * Spirit abilities are stronger at high tether (unmoored).
 * ========================================================================= */

int get_shaman_power_mod( CHAR_DATA *ch, bool is_material )
{
	int tether = ch->rage;

	if ( is_material )
	{
		if ( tether <= SHAMAN_GROUNDED_MAX )    return 130;  /* +30% */
		if ( tether <= SHAMAN_EARTHBOUND_MAX )  return 115;  /* +15% */
		if ( tether <= SHAMAN_BALANCED_MAX )    return 100;  /* normal */
		if ( tether <= SHAMAN_SPIRITTOUCH_MAX ) return 75;   /* -25% */
		return 50;                                            /* -50% */
	}
	else /* spirit */
	{
		if ( tether <= SHAMAN_GROUNDED_MAX )    return 50;   /* -50% */
		if ( tether <= SHAMAN_EARTHBOUND_MAX )  return 75;   /* -25% */
		if ( tether <= SHAMAN_BALANCED_MAX )    return 100;  /* normal */
		if ( tether <= SHAMAN_SPIRITTOUCH_MAX ) return 115;  /* +15% */
		return 130;                                           /* +30% */
	}
}

/* =========================================================================
 * TETHER - Display current tether position and status
 *
 * Shared between Shaman and Spirit Lord. Shows tether bar, zone name,
 * power modifiers, and active effects.
 * ========================================================================= */

void do_tether( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int tether_cap;
	const char *zone_name;
	bool is_sl;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) && !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	is_sl = IS_CLASS( ch, CLASS_SPIRITLORD );
	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	tether_cap = is_sl ? SL_TETHER_CAP : SHAMAN_TETHER_CAP;

	/* Header */
	if ( is_sl )
		snprintf( buf, sizeof(buf), "%s{{%s(#n Spirit Tether %s)%s}}#n\n\r", ac, pc, pc, ac );
	else
		snprintf( buf, sizeof(buf), "%s|~%s(#n Spirit Tether %s)%s~|#n\n\r", ac, pc, pc, ac );
	send_to_char( buf, ch );

	/* Current tether */
	snprintf( buf, sizeof(buf), "Tether: %s%d#n / %d\n\r", pc, ch->rage, tether_cap );
	send_to_char( buf, ch );

	/* Zone name */
	if ( is_sl )
	{
		if ( ch->rage <= SL_ANCHORED_MAX )         zone_name = "#GAnchor#ned";
		else if ( ch->rage <= SL_EARTHBOUND_MAX )   zone_name = "#gEarth#nbound";
		else if ( ch->rage <= SL_BALANCED_MAX )     zone_name = "#CBalan#nced";
		else if ( ch->rage <= SL_SPIRITTOUCH_MAX )  zone_name = "#bSpirit#n-Touched";
		else                                         zone_name = "#BAscen#nded";
	}
	else
	{
		if ( ch->rage <= SHAMAN_GROUNDED_MAX )         zone_name = "#GGround#ned";
		else if ( ch->rage <= SHAMAN_EARTHBOUND_MAX )   zone_name = "#gEarth#nbound";
		else if ( ch->rage <= SHAMAN_BALANCED_MAX )     zone_name = "#CBalan#nced";
		else if ( ch->rage <= SHAMAN_SPIRITTOUCH_MAX )  zone_name = "#bSpirit#n-Touched";
		else                                             zone_name = "#BUnmoor#ned";
	}
	snprintf( buf, sizeof(buf), "State: %s\n\r", zone_name );
	send_to_char( buf, ch );

	/* Power modifiers */
	if ( is_sl )
		snprintf( buf, sizeof(buf), "Material power: %s%d%%#n    Spirit power: %s%d%%#n\n\r",
			pc, get_sl_power_mod( ch, TRUE ),
			pc, get_sl_power_mod( ch, FALSE ) );
	else
		snprintf( buf, sizeof(buf), "Material power: %s%d%%#n    Spirit power: %s%d%%#n\n\r",
			pc, get_shaman_power_mod( ch, TRUE ),
			pc, get_shaman_power_mod( ch, FALSE ) );
	send_to_char( buf, ch );

	/* Active effects - Shaman */
	if ( IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		switch ( ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] )
		{
			case TOTEM_WARD:
				snprintf( buf, sizeof(buf), "%s[#nWard Totem: #C%d#n ticks%s]#n\n\r",
					pc, ch->pcdata->powers[SHAMAN_WARD_TICKS], pc );
				send_to_char( buf, ch );
				break;
			case TOTEM_WRATH:
				snprintf( buf, sizeof(buf), "%s[#nWrath Totem: #C%d#n ticks%s]#n\n\r",
					pc, ch->pcdata->powers[SHAMAN_WRATH_TICKS], pc );
				send_to_char( buf, ch );
				break;
			case TOTEM_SPIRIT:
				snprintf( buf, sizeof(buf), "%s[#nSpirit Totem: #C%d#n ticks%s]#n\n\r",
					pc, ch->pcdata->powers[SHAMAN_SPIRIT_TOTEM_TICKS], pc );
				send_to_char( buf, ch );
				break;
		}
		if ( ch->pcdata->powers[SHAMAN_SPIRIT_WARD_CHARGES] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nSpirit Ward: #C%d#n charges%s]#n\n\r",
				pc, ch->pcdata->powers[SHAMAN_SPIRIT_WARD_CHARGES], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SHAMAN_SPIRIT_WALK_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nSpirit Walk: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[SHAMAN_SPIRIT_WALK_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SHAMAN_SPIRIT_SIGHT_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nSpirit Sight: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[SHAMAN_SPIRIT_SIGHT_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SHAMAN_SOUL_LINK_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nSoul Link: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[SHAMAN_SOUL_LINK_TICKS], pc );
			send_to_char( buf, ch );
		}
	}

	/* Active effects - Spirit Lord */
	if ( IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		switch ( ch->pcdata->powers[SL_EMBODY_TYPE] )
		{
			case AURA_WARD:
				snprintf( buf, sizeof(buf), "%s[#nWard Aura: #CActive#n%s]#n\n\r", pc, pc );
				send_to_char( buf, ch );
				break;
			case AURA_WRATH:
				snprintf( buf, sizeof(buf), "%s[#nWrath Aura: #CActive#n%s]#n\n\r", pc, pc );
				send_to_char( buf, ch );
				break;
			case AURA_SPIRIT:
				snprintf( buf, sizeof(buf), "%s[#nSpirit Aura: #CActive#n%s]#n\n\r", pc, pc );
				send_to_char( buf, ch );
				break;
		}
		if ( ch->pcdata->powers[SL_ANCESTRAL_FORM_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nAncestral Form: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[SL_ANCESTRAL_FORM_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nSpirit Fusion: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SL_SPIRIT_ARMY_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nSpirit Army: #C%d#n ticks, #C%d#n warriors%s]#n\n\r",
				pc, ch->pcdata->powers[SL_SPIRIT_ARMY_TICKS],
				ch->pcdata->stats[SL_STAT_ARMY_COUNT], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SL_POSSESS_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nPossess: #C%d#n rounds%s]#n\n\r",
				pc, ch->pcdata->powers[SL_POSSESS_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SL_WISDOM_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nAncestral Wisdom: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[SL_WISDOM_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SL_ASCENSION_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nAscension: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[SL_ASCENSION_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[SL_ASCENSION_AFTERMATH] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nRe-materializing: #R%d#n rounds%s]#n\n\r",
				pc, ch->pcdata->powers[SL_ASCENSION_AFTERMATH], pc );
			send_to_char( buf, ch );
		}
	}

	/* Training levels */
	send_to_char( "\n\r", ch );
	if ( IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		snprintf( buf, sizeof(buf), "Training: Totems %s%d#n  Spirits %s%d#n  Communion %s%d#n\n\r",
			pc, ch->pcdata->powers[SHAMAN_TRAIN_TOTEM],
			pc, ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT],
			pc, ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] );
	}
	else
	{
		snprintf( buf, sizeof(buf), "Training: Embodiment %s%d#n  Dominion %s%d#n  Transcendence %s%d#n\n\r",
			pc, ch->pcdata->powers[SL_TRAIN_EMBODY],
			pc, ch->pcdata->powers[SL_TRAIN_DOMINION],
			pc, ch->pcdata->powers[SL_TRAIN_TRANSCEND] );
	}
	send_to_char( buf, ch );

	return;
}

/* =========================================================================
 * SPIRITTRAIN - Train Shaman ability categories
 * ========================================================================= */

void do_spirittrain( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	if ( argument[0] == '\0' )
	{
		snprintf( buf, sizeof(buf), "%s|~%s(#n Spirit Training %s)%s~|#n\n\r", ac, pc, pc, ac );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[SHAMAN_TRAIN_TOTEM] + 1 ) * cfg( CFG_SHAMAN_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nTotems:    Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[SHAMAN_TRAIN_TOTEM], cost, pc );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT] + 1 ) * cfg( CFG_SHAMAN_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nSpirits:   Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT], cost, pc );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] + 1 ) * cfg( CFG_SHAMAN_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nCommunion: Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE], cost, pc );
		send_to_char( buf, ch );

		send_to_char( "\n\rSyntax: spirittrain <totems|spirits|communion>\n\r", ch );
		return;
	}

	if ( !str_prefix( argument, "totems" ) )
	{
		if ( ch->pcdata->powers[SHAMAN_TRAIN_TOTEM] >= 3 )
		{
			send_to_char( "You have mastered Totems.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[SHAMAN_TRAIN_TOTEM] + 1 ) * cfg( CFG_SHAMAN_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Totems.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[SHAMAN_TRAIN_TOTEM]++;
		snprintf( buf, sizeof(buf),
			"#GThe spirit of the earth answers... Totems is now level %d.#n\n\r",
			ch->pcdata->powers[SHAMAN_TRAIN_TOTEM] );
		send_to_char( buf, ch );
		return;
	}

	if ( !str_prefix( argument, "spirits" ) )
	{
		if ( ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT] >= 3 )
		{
			send_to_char( "You have mastered Spirits.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT] + 1 ) * cfg( CFG_SHAMAN_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Spirits.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT]++;
		snprintf( buf, sizeof(buf),
			"#BSpirits swirl around you... Spirits is now level %d.#n\n\r",
			ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT] );
		send_to_char( buf, ch );
		return;
	}

	if ( !str_prefix( argument, "communion" ) )
	{
		if ( ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] >= 3 )
		{
			send_to_char( "You have mastered Communion.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] + 1 ) * cfg( CFG_SHAMAN_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Communion.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE]++;
		snprintf( buf, sizeof(buf),
			"#CWhispers of the ancestors fill your mind... Communion is now level %d.#n\n\r",
			ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] );
		send_to_char( buf, ch );
		return;
	}

	send_to_char( "Syntax: spirittrain <totems|spirits|communion>\n\r", ch );
	return;
}

/* =========================================================================
 * TOTEM HELPERS
 *
 * Totems are mob entities placed in the room. Only one active at a time.
 * When a new totem is placed, the old one is despawned first.
 * ========================================================================= */

static void shaman_despawn_totem( CHAR_DATA *ch )
{
	CHAR_DATA *mob;
	CHAR_DATA *mob_next;

	LIST_FOR_EACH_SAFE( mob, mob_next, &g_characters, CHAR_DATA, char_node )
	{

		if ( !IS_NPC( mob ) ) continue;
		if ( mob->pIndexData == NULL ) continue;
		if ( mob->pIndexData->vnum != VNUM_SHAMAN_TOTEM ) continue;
		if ( mob->wizard != ch ) continue;

		act( "$n crumbles as the spirits depart.", mob, NULL, NULL, TO_ROOM );
		extract_char( mob, TRUE );
	}

	ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] = TOTEM_NONE;
	ch->pcdata->powers[SHAMAN_WARD_TICKS] = 0;
	ch->pcdata->powers[SHAMAN_WRATH_TICKS] = 0;
	ch->pcdata->powers[SHAMAN_SPIRIT_TOTEM_TICKS] = 0;
	ch->pcdata->stats[SHAMAN_STAT_TOTEM_HP] = 0;
}

static CHAR_DATA *shaman_create_totem( CHAR_DATA *ch, int totem_type )
{
	CHAR_DATA *totem;
	MOB_INDEX_DATA *pMobIndex;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];
	const char *totem_name;
	int totem_hp;

	if ( ( pMobIndex = get_mob_index( VNUM_SHAMAN_TOTEM ) ) == NULL )
	{
		send_to_char( "Totem template not found. Please contact an immortal.\n\r", ch );
		return NULL;
	}

	switch ( totem_type )
	{
		case TOTEM_WARD:   totem_name = "Ward";   break;
		case TOTEM_WRATH:  totem_name = "Wrath";  break;
		case TOTEM_SPIRIT: totem_name = "Spirit";  break;
		default:           totem_name = "Unknown"; break;
	}

	totem = create_mobile( pMobIndex );
	totem_hp = cfg( CFG_SHAMAN_TOTEM_HP );

	totem->level  = ch->level;
	totem->hit    = totem_hp;
	totem->max_hit = totem_hp;
	totem->hitroll = 0;
	totem->damroll = 0;
	totem->armor  = 0;

	snprintf( buf, sizeof(buf), "%s's %s Totem", ch->name, totem_name );
	free(totem->short_descr);
	totem->short_descr = str_dup( buf );

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf),
			"%sA glowing %s Totem belonging to %s pulses with spirit energy here.#n\n\r",
			pc, totem_name, ch->name );
	}
	free(totem->long_descr);
	totem->long_descr = str_dup( buf );

	snprintf( buf, sizeof(buf), "totem %s", totem_name );
	free(totem->name);
	totem->name = str_dup( buf );

	SET_BIT( totem->act, ACT_SENTINEL );
	SET_BIT( totem->act, ACT_NOEXP );
	totem->spec_fun = NULL;

	free(totem->lord);
	totem->lord = str_dup( ch->name );
	totem->wizard = ch;

	char_to_room( totem, ch->in_room );
	add_follower( totem, ch );

	af.type      = skill_lookup( "charm person" );
	af.duration  = 666;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char( totem, &af );

	ch->pcdata->stats[SHAMAN_STAT_TOTEM_HP] = totem_hp;

	return totem;
}

/* =========================================================================
 * TOTEM ABILITIES (Material - Decreases Tether)
 * ========================================================================= */

void do_wardtotem( CHAR_DATA *ch, char *argument )
{
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_TOTEM] < 1 )
	{
		send_to_char( "You must train Totems to level 1 first.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_WARDTOTEM_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	/* Remove existing totem if any */
	if ( ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] != TOTEM_NONE )
	{
		send_to_char( "Your previous totem crumbles as you channel new spirits.\n\r", ch );
		shaman_despawn_totem( ch );
	}

	ch->mana -= mana_cost;

	if ( shaman_create_totem( ch, TOTEM_WARD ) == NULL )
		return;

	ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] = TOTEM_WARD;
	ch->pcdata->powers[SHAMAN_WARD_TICKS] = cfg( CFG_SHAMAN_WARDTOTEM_DURATION );

	/* Shift tether toward material */
	ch->rage = UMAX( ch->rage - cfg( CFG_SHAMAN_WARDTOTEM_TETHER_CHANGE ), 0 );

	send_to_char( "#GA Ward Totem rises from the earth, protective spirits swirling around it.#n\n\r", ch );
	act( "$n plants a glowing totem in the ground, and protective spirits emerge.",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

void do_wrathtotem( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_TOTEM] < 2 )
	{
		send_to_char( "You must train Totems to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_WRATHTOTEM_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Wrath Totem is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SHAMAN_WRATHTOTEM_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_WRATHTOTEM_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	/* Remove existing totem if any */
	if ( ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] != TOTEM_NONE )
	{
		send_to_char( "Your previous totem crumbles as you channel new spirits.\n\r", ch );
		shaman_despawn_totem( ch );
	}

	ch->mana -= mana_cost;

	if ( shaman_create_totem( ch, TOTEM_WRATH ) == NULL )
		return;

	ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] = TOTEM_WRATH;
	ch->pcdata->powers[SHAMAN_WRATH_TICKS] = cfg( CFG_SHAMAN_WRATHTOTEM_DURATION );
	ch->pcdata->powers[SHAMAN_WRATHTOTEM_CD] = cfg( CFG_SHAMAN_WRATHTOTEM_COOLDOWN );

	/* Shift tether toward material */
	ch->rage = UMAX( ch->rage - cfg( CFG_SHAMAN_WRATHTOTEM_TETHER_CHANGE ), 0 );

	send_to_char( "#RA Wrath Totem erupts from the ground, angry spirits shrieking around it!#n\n\r", ch );
	act( "$n plants a totem crackling with fury, and vengeful spirits shriek forth!",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

void do_spirittotem( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_TOTEM] < 3 )
	{
		send_to_char( "You must train Totems to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_SPIRITTOTEM_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Spirit Totem is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SHAMAN_SPIRITTOTEM_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_SPIRITTOTEM_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	/* Remove existing totem if any */
	if ( ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] != TOTEM_NONE )
	{
		send_to_char( "Your previous totem crumbles as you channel new spirits.\n\r", ch );
		shaman_despawn_totem( ch );
	}

	ch->mana -= mana_cost;

	if ( shaman_create_totem( ch, TOTEM_SPIRIT ) == NULL )
		return;

	ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] = TOTEM_SPIRIT;
	ch->pcdata->powers[SHAMAN_SPIRIT_TOTEM_TICKS] = cfg( CFG_SHAMAN_SPIRITTOTEM_DURATION );
	ch->pcdata->powers[SHAMAN_SPIRITTOTEM_CD] = cfg( CFG_SHAMAN_SPIRITTOTEM_COOLDOWN );

	/* Shift tether toward material */
	ch->rage = UMAX( ch->rage - cfg( CFG_SHAMAN_SPIRITTOTEM_TETHER_CHANGE ), 0 );

	send_to_char( "#CThe Spirit Totem manifests, shimmering between worlds.#n\n\r", ch );
	act( "$n plants a totem that shimmers between planes, spirit energy pulsing outward.",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

/* =========================================================================
 * SPIRIT ABILITIES (Spirit - Increases Tether)
 * ========================================================================= */

void do_spiritbolt( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	int mana_cost;
	int dam;
	int power_mod;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT] < 1 )
	{
		send_to_char( "You must train Spirits to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to call a spirit bolt.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[SHAMAN_SPIRITBOLT_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Spirit Bolt is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SHAMAN_SPIRITBOLT_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_SPIRITBOLT_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	/* Calculate damage: base + tether scaling */
	dam = number_range( cfg( CFG_SHAMAN_SPIRITBOLT_BASE_MIN ),
	                    cfg( CFG_SHAMAN_SPIRITBOLT_BASE_MAX ) );
	dam += ch->rage * 2;

	/* Apply spirit power modifier */
	power_mod = get_shaman_power_mod( ch, FALSE );
	dam = dam * power_mod / 100;

	/* Shift tether toward spirit */
	ch->rage = UMIN( ch->rage + cfg( CFG_SHAMAN_SPIRITBOLT_TETHER_CHANGE ), SHAMAN_TETHER_CAP );

	/* Set cooldown */
	ch->pcdata->powers[SHAMAN_SPIRITBOLT_CD] = cfg( CFG_SHAMAN_SPIRITBOLT_COOLDOWN );

	act( "#BA vengeful spirit tears through the veil and strikes $N!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#B$n calls a spirit from beyond the veil to strike you!#n",
		ch, NULL, victim, TO_VICT );
	act( "#B$n calls a spirit from beyond the veil to strike $N!#n",
		ch, NULL, victim, TO_NOTVICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );
	WAIT_STATE( ch, 12 );

	return;
}

void do_spiritward( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;
	int charges;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT] < 2 )
	{
		send_to_char( "You must train Spirits to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_SPIRIT_WARD_CHARGES] > 0 )
	{
		send_to_char( "Your spirit ward is already active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_SPIRITWARD_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	charges = cfg( CFG_SHAMAN_SPIRITWARD_CHARGES );

	/* Tether scaling: gain 1 bonus charge in spirit-touched or unmoored zone */
	if ( ch->rage > SHAMAN_BALANCED_MAX )
		charges++;

	ch->pcdata->powers[SHAMAN_SPIRIT_WARD_CHARGES] = charges;

	/* Shift tether toward spirit */
	ch->rage = UMIN( ch->rage + cfg( CFG_SHAMAN_SPIRITWARD_TETHER_CHANGE ), SHAMAN_TETHER_CAP );

	snprintf( buf, sizeof(buf),
		"#BProtective spirits encircle you! (%d charges)#n\n\r", charges );
	send_to_char( buf, ch );
	act( "$n is surrounded by shimmering #Bprotective spirits#n.", ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

void do_spiritwalk( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_SPIRIT] < 3 )
	{
		send_to_char( "You must train Spirits to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_SPIRIT_WALK_TICKS] > 0 )
	{
		send_to_char( "You are already walking between worlds.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_SPIRITWALK_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Spirit Walk is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SHAMAN_SPIRITWALK_CD] );
		send_to_char( buf, ch );
		return;
	}

	/* Cannot activate when already too spirit-heavy */
	if ( ch->rage >= 70 )
	{
		send_to_char( "Your tether is too spirit-heavy to walk between worlds safely.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_SPIRITWALK_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	ch->pcdata->powers[SHAMAN_SPIRIT_WALK_TICKS] = cfg( CFG_SHAMAN_SPIRITWALK_DURATION );
	ch->pcdata->powers[SHAMAN_SPIRITWALK_CD] = cfg( CFG_SHAMAN_SPIRITWALK_COOLDOWN );

	/* Shift tether toward spirit */
	ch->rage = UMIN( ch->rage + cfg( CFG_SHAMAN_SPIRITWALK_TETHER_CHANGE ), SHAMAN_TETHER_CAP );

	send_to_char( "#BYou step partially into the spirit plane, your form becoming translucent.#n\n\r", ch );
	act( "$n's form shimmers and becomes partially transparent, one foot in another world.",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

/* =========================================================================
 * COMMUNION ABILITIES (Utility - Neutral Tether)
 * ========================================================================= */

void do_ancestorcall( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;
	int heal;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] < 1 )
	{
		send_to_char( "You must train Communion to level 1 first.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_ANCESTORCALL_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	heal = number_range( cfg( CFG_SHAMAN_ANCESTORCALL_HEAL_MIN ),
	                     cfg( CFG_SHAMAN_ANCESTORCALL_HEAL_MAX ) );

	/* Balance bonus: extra healing at perfect balance (tether 45-55) */
	if ( ch->rage >= 45 && ch->rage <= 55 )
	{
		heal = heal * cfg( CFG_SHAMAN_ANCESTORCALL_BALANCE_BONUS ) / 100;
		send_to_char( "#CThe ancestors smile upon your harmony.#n\n\r", ch );
	}

	heal_char( ch, heal );

	snprintf( buf, sizeof(buf),
		"#CYou call upon your ancestors, and warmth floods through you for #t5EC4B0%d#C health.#n\n\r", heal );
	send_to_char( buf, ch );
	act( "$n is surrounded by a gentle #Cspirit glow#n as ancestral whispers fill the air.",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );

	return;
}

void do_spiritsight( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int duration;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] < 2 )
	{
		send_to_char( "You must train Communion to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_SPIRIT_SIGHT_TICKS] > 0 )
	{
		send_to_char( "Your spirit sight is already active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_SPIRITSIGHT_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	duration = cfg( CFG_SHAMAN_SPIRITSIGHT_DURATION );
	ch->pcdata->powers[SHAMAN_SPIRIT_SIGHT_TICKS] = duration;

	send_to_char( "#CYour eyes shimmer as the spirit world overlays your vision.#n\n\r", ch );
	act( "$n's eyes take on an eerie #Cspirit glow#n.", ch, NULL, NULL, TO_ROOM );

	return;
}

void do_soullink( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SHAMAN ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_TRAIN_COMMUNE] < 3 )
	{
		send_to_char( "You must train Communion to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to forge a soul link.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[SHAMAN_SOUL_LINK_TICKS] > 0 )
	{
		send_to_char( "Your soul is already linked.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SHAMAN_SOULLINK_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Soul Link is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SHAMAN_SOULLINK_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SHAMAN_SOULLINK_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	ch->pcdata->powers[SHAMAN_SOUL_LINK_TICKS] = cfg( CFG_SHAMAN_SOULLINK_DURATION );
	ch->pcdata->powers[SHAMAN_SOUL_LINK_TYPE] = 0;  /* 0 = enemy link via ch->fighting */
	ch->pcdata->powers[SHAMAN_SOULLINK_CD] = cfg( CFG_SHAMAN_SOULLINK_COOLDOWN );

	act( "#CYou forge a spirit link with $N - your fates are now intertwined!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#C$n forges a spirit link with you - a spectral thread binds your souls!#n",
		ch, NULL, victim, TO_VICT );
	act( "#CA spectral thread connects $n and $N, binding their spirits together.#n",
		ch, NULL, victim, TO_NOTVICT );

	WAIT_STATE( ch, 12 );
	return;
}

/* =========================================================================
 * SHAMANARMOR - Class equipment creation wrapper
 * ========================================================================= */

void do_shamanarmor( CHAR_DATA *ch, char *argument )
{
	do_classarmor_generic( ch, argument, CLASS_SHAMAN );
}

/* =========================================================================
 * UPDATE_SHAMAN - Per-tick processing
 *
 * Called from update.c for each Shaman character. Handles:
 * - Tether drift toward center
 * - Spirit manifestation at extremes
 * - Buff durations and cooldown decrements
 * - Totem tick effects (damage, drain, defense)
 * ========================================================================= */

void update_shaman( CHAR_DATA *ch )
{
	char buf[MAX_STRING_LENGTH];

	/* ---- Tether drift toward center (+/-2 per tick) ---- */
	if ( ch->rage > SHAMAN_TETHER_CENTER )
		ch->rage = UMAX( SHAMAN_TETHER_CENTER, ch->rage - 2 );
	else if ( ch->rage < SHAMAN_TETHER_CENTER )
		ch->rage = UMIN( SHAMAN_TETHER_CENTER, ch->rage + 2 );

	/* ---- Spirit manifestation at extremes (10% chance) ---- */
	if ( ch->rage <= SHAMAN_MANIFEST_LOW || ch->rage >= SHAMAN_MANIFEST_HIGH )
	{
		if ( number_range( 1, 100 ) <= 10 )
		{
			ch->pcdata->stats[SHAMAN_STAT_MANIFEST_CNT]++;

			switch ( number_range( 1, 3 ) )
			{
			case 1: /* Ancestor whisper - small heal */
				{
					int heal = ch->max_hit / 20;
					ch->hit = UMIN( ch->max_hit, ch->hit + heal );
					snprintf( buf, sizeof(buf),
						"#CSpirit manifestation! An ancestor whispers, restoring %d health.#n\n\r", heal );
					send_to_char( buf, ch );
				}
				break;
			case 2: /* Spirit surge - brief stun */
				send_to_char( "#CSpirit manifestation! Spirits overwhelm your senses briefly.#n\n\r", ch );
				WAIT_STATE( ch, 12 );
				break;
			case 3: /* Tether snap - move toward center */
				ch->rage = ( ch->rage < SHAMAN_TETHER_CENTER )
					? UMIN( ch->rage + 10, SHAMAN_TETHER_CENTER )
					: UMAX( ch->rage - 10, SHAMAN_TETHER_CENTER );
				send_to_char( "#CSpirit manifestation! Your tether snaps back toward balance.#n\n\r", ch );
				break;
			}
		}
	}

	/* ---- Totem duration countdowns ---- */
	if ( ch->pcdata->powers[SHAMAN_WARD_TICKS] > 0 )
	{
		ch->pcdata->powers[SHAMAN_WARD_TICKS]--;
		if ( ch->pcdata->powers[SHAMAN_WARD_TICKS] == 0 )
		{
			send_to_char( "#GYour Ward Totem crumbles, its protective spirits dispersing.#n\n\r", ch );
			shaman_despawn_totem( ch );
		}
	}

	if ( ch->pcdata->powers[SHAMAN_WRATH_TICKS] > 0 )
	{
		ch->pcdata->powers[SHAMAN_WRATH_TICKS]--;

		/* Wrath Totem tick damage to fighting target */
		if ( ch->fighting != NULL && ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] == TOTEM_WRATH )
		{
			int dam = number_range( cfg( CFG_SHAMAN_WRATHTOTEM_TICK_MIN ),
			                        cfg( CFG_SHAMAN_WRATHTOTEM_TICK_MAX ) );
			int power_mod = get_shaman_power_mod( ch, TRUE );
			dam = dam * power_mod / 100;

			act( "#RThe Wrath Totem lashes $N with vengeful spirits!#n",
				ch, NULL, ch->fighting, TO_CHAR );
			act( "#R$n's Wrath Totem lashes you with vengeful spirits!#n",
				ch, NULL, ch->fighting, TO_VICT );
			damage( ch, ch->fighting, dam, TYPE_UNDEFINED );
		}

		if ( ch->pcdata->powers[SHAMAN_WRATH_TICKS] == 0 )
		{
			send_to_char( "#RYour Wrath Totem shatters, its fury spent.#n\n\r", ch );
			shaman_despawn_totem( ch );
		}
	}

	if ( ch->pcdata->powers[SHAMAN_SPIRIT_TOTEM_TICKS] > 0 )
	{
		ch->pcdata->powers[SHAMAN_SPIRIT_TOTEM_TICKS]--;

		/* Spirit Totem mana drain from target, restore to self */
		if ( ch->fighting != NULL && ch->pcdata->powers[SHAMAN_ACTIVE_TOTEM] == TOTEM_SPIRIT )
		{
			int drain = cfg( CFG_SHAMAN_SPIRITTOTEM_MANA_DRAIN );
			int restore = cfg( CFG_SHAMAN_SPIRITTOTEM_MANA_RESTORE );

			if ( !IS_NPC( ch->fighting ) && ch->fighting->mana >= drain )
				ch->fighting->mana -= drain;

			ch->mana = UMIN( ch->mana + restore, ch->max_mana );

			act( "#CThe Spirit Totem drains mystical energy from $N!#n",
				ch, NULL, ch->fighting, TO_CHAR );
		}

		if ( ch->pcdata->powers[SHAMAN_SPIRIT_TOTEM_TICKS] == 0 )
		{
			send_to_char( "#CYour Spirit Totem fades back between worlds.#n\n\r", ch );
			shaman_despawn_totem( ch );
		}
	}

	/* ---- Buff duration countdowns ---- */
	if ( ch->pcdata->powers[SHAMAN_SPIRIT_WALK_TICKS] > 0 )
	{
		ch->pcdata->powers[SHAMAN_SPIRIT_WALK_TICKS]--;
		if ( ch->pcdata->powers[SHAMAN_SPIRIT_WALK_TICKS] == 0 )
			send_to_char( "#BYou solidify as you return fully to the material plane.#n\n\r", ch );
	}

	if ( ch->pcdata->powers[SHAMAN_SPIRIT_SIGHT_TICKS] > 0 )
	{
		ch->pcdata->powers[SHAMAN_SPIRIT_SIGHT_TICKS]--;
		if ( ch->pcdata->powers[SHAMAN_SPIRIT_SIGHT_TICKS] == 0 )
			send_to_char( "#CYour spirit sight fades - the veil closes once more.#n\n\r", ch );
	}

	if ( ch->pcdata->powers[SHAMAN_SOUL_LINK_TICKS] > 0 )
	{
		ch->pcdata->powers[SHAMAN_SOUL_LINK_TICKS]--;
		if ( ch->pcdata->powers[SHAMAN_SOUL_LINK_TICKS] == 0 )
		{
			ch->pcdata->powers[SHAMAN_SOUL_LINK_TYPE] = 0;
			send_to_char( "#CThe soul link dissolves - your spirit is your own again.#n\n\r", ch );
		}
	}

	/* ---- Cooldown decrements ---- */
	if ( ch->pcdata->powers[SHAMAN_WRATHTOTEM_CD] > 0 )
		ch->pcdata->powers[SHAMAN_WRATHTOTEM_CD]--;
	if ( ch->pcdata->powers[SHAMAN_SPIRITTOTEM_CD] > 0 )
		ch->pcdata->powers[SHAMAN_SPIRITTOTEM_CD]--;
	if ( ch->pcdata->powers[SHAMAN_SPIRITWALK_CD] > 0 )
		ch->pcdata->powers[SHAMAN_SPIRITWALK_CD]--;
	if ( ch->pcdata->powers[SHAMAN_SOULLINK_CD] > 0 )
		ch->pcdata->powers[SHAMAN_SOULLINK_CD]--;
	if ( ch->pcdata->powers[SHAMAN_SPIRITBOLT_CD] > 0 )
		ch->pcdata->powers[SHAMAN_SPIRITBOLT_CD]--;
}
