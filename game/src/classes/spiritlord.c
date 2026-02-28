/***************************************************************************
 *  Spirit Lord class - upgrade from Shaman. Transcends the boundary       *
 *  between material and spirit. Internalizes totems as personal auras,    *
 *  commands spirit armies, and can shed physical form entirely.            *
 *  Uses expanded Spirit Tether (0-150) via ch->rage.                      *
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
 * TETHER SCALING HELPER (Spirit Lord)
 *
 * Returns a percentage modifier (40-140) based on tether position.
 * Material abilities are stronger at low tether (anchored).
 * Spirit abilities are stronger at high tether (ascended).
 * ========================================================================= */

int get_sl_power_mod( CHAR_DATA *ch, bool is_material )
{
	int tether = ch->rage;

	if ( is_material )
	{
		if ( tether <= SL_ANCHORED_MAX )     return 140;  /* +40% */
		if ( tether <= SL_EARTHBOUND_MAX )   return 120;  /* +20% */
		if ( tether <= SL_BALANCED_MAX )     return 100;  /* normal */
		if ( tether <= SL_SPIRITTOUCH_MAX )  return 70;   /* -30% */
		return 40;                                         /* -60% */
	}
	else /* spirit */
	{
		if ( tether <= SL_ANCHORED_MAX )     return 40;   /* -60% */
		if ( tether <= SL_EARTHBOUND_MAX )   return 70;   /* -30% */
		if ( tether <= SL_BALANCED_MAX )     return 100;  /* normal */
		if ( tether <= SL_SPIRITTOUCH_MAX )  return 120;  /* +20% */
		return 140;                                        /* +40% */
	}
}

/* =========================================================================
 * LORDTRAIN - Train Spirit Lord ability categories
 * ========================================================================= */

void do_lordtrain( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	if ( argument[0] == '\0' )
	{
		snprintf( buf, sizeof(buf), "%s{{%s(#n Lord Training %s)%s}}#n\n\r", ac, pc, pc, ac );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[SL_TRAIN_EMBODY] + 1 ) * cfg( CFG_SL_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nEmbodiment:    Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[SL_TRAIN_EMBODY], cost, pc );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[SL_TRAIN_DOMINION] + 1 ) * cfg( CFG_SL_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nDominion:      Level %d/4  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[SL_TRAIN_DOMINION], cost, pc );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[SL_TRAIN_TRANSCEND] + 1 ) * cfg( CFG_SL_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nTranscendence: Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[SL_TRAIN_TRANSCEND], cost, pc );
		send_to_char( buf, ch );

		send_to_char( "\n\rSyntax: lordtrain <embodiment|dominion|transcendence>\n\r", ch );
		return;
	}

	if ( !str_prefix( argument, "embodiment" ) )
	{
		if ( ch->pcdata->powers[SL_TRAIN_EMBODY] >= 3 )
		{
			send_to_char( "You have mastered Embodiment.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[SL_TRAIN_EMBODY] + 1 ) * cfg( CFG_SL_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Embodiment.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[SL_TRAIN_EMBODY]++;
		snprintf( buf, sizeof(buf),
			"#GSpiritual energy coalesces within you... Embodiment is now level %d.#n\n\r",
			ch->pcdata->powers[SL_TRAIN_EMBODY] );
		send_to_char( buf, ch );
		return;
	}

	if ( !str_prefix( argument, "dominion" ) )
	{
		if ( ch->pcdata->powers[SL_TRAIN_DOMINION] >= 4 )
		{
			send_to_char( "You have mastered Dominion.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[SL_TRAIN_DOMINION] + 1 ) * cfg( CFG_SL_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Dominion.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[SL_TRAIN_DOMINION]++;
		snprintf( buf, sizeof(buf),
			"#BThe spirits bend to your will... Dominion is now level %d.#n\n\r",
			ch->pcdata->powers[SL_TRAIN_DOMINION] );
		send_to_char( buf, ch );
		return;
	}

	if ( !str_prefix( argument, "transcendence" ) )
	{
		if ( ch->pcdata->powers[SL_TRAIN_TRANSCEND] >= 3 )
		{
			send_to_char( "You have mastered Transcendence.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[SL_TRAIN_TRANSCEND] + 1 ) * cfg( CFG_SL_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Transcendence.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[SL_TRAIN_TRANSCEND]++;
		snprintf( buf, sizeof(buf),
			"#CThe boundary between worlds thins... Transcendence is now level %d.#n\n\r",
			ch->pcdata->powers[SL_TRAIN_TRANSCEND] );
		send_to_char( buf, ch );
		return;
	}

	send_to_char( "Syntax: lordtrain <embodiment|dominion|transcendence>\n\r", ch );
	return;
}

/* =========================================================================
 * EMBODIMENT ABILITIES (Internalized Totems - Decreases Tether)
 * ========================================================================= */

void do_embody( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int aura_type;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_EMBODY] < 1 )
	{
		send_to_char( "You must train Embodiment to level 1 first.\n\r", ch );
		return;
	}

	if ( argument[0] == '\0' )
	{
		send_to_char( "Syntax: embody <ward|wrath|spirit|none>\n\r", ch );
		return;
	}

	/* Toggle off */
	if ( !str_prefix( argument, "none" ) || !str_prefix( argument, "off" ) )
	{
		if ( ch->pcdata->powers[SL_EMBODY_TYPE] == AURA_NONE )
		{
			send_to_char( "You have no aura active.\n\r", ch );
			return;
		}
		ch->pcdata->powers[SL_EMBODY_TYPE] = AURA_NONE;
		send_to_char( "#CYour aura fades as you release the internalized totem.#n\n\r", ch );
		act( "$n's aura fades.", ch, NULL, NULL, TO_ROOM );
		return;
	}

	/* Determine aura type */
	if ( !str_prefix( argument, "ward" ) )
		aura_type = AURA_WARD;
	else if ( !str_prefix( argument, "wrath" ) )
		aura_type = AURA_WRATH;
	else if ( !str_prefix( argument, "spirit" ) )
		aura_type = AURA_SPIRIT;
	else
	{
		send_to_char( "Syntax: embody <ward|wrath|spirit|none>\n\r", ch );
		return;
	}

	/* If already this aura, toggle off */
	if ( ch->pcdata->powers[SL_EMBODY_TYPE] == aura_type )
	{
		ch->pcdata->powers[SL_EMBODY_TYPE] = AURA_NONE;
		send_to_char( "#CYour aura fades as you release the internalized totem.#n\n\r", ch );
		act( "$n's aura fades.", ch, NULL, NULL, TO_ROOM );
		return;
	}

	/* Spirit Fusion overrides individual embody */
	if ( ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] > 0 )
	{
		send_to_char( "Spirit Fusion is active - all three auras are already merged.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SL_EMBODY_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->pcdata->powers[SL_EMBODY_TYPE] = aura_type;

	/* Shift tether toward material */
	ch->rage = UMAX( ch->rage - cfg( CFG_SL_EMBODY_TETHER_CHANGE ), 0 );

	switch ( aura_type )
	{
		case AURA_WARD:
			send_to_char( "#GProtective spirits settle into your very being as a Ward Aura.#n\n\r", ch );
			act( "$n's form shimmers with a protective #Gward aura#n.", ch, NULL, NULL, TO_ROOM );
			break;
		case AURA_WRATH:
			send_to_char( "#RVengeful spirits merge with your essence as a Wrath Aura!#n\n\r", ch );
			act( "$n's form crackles with a furious #Rwrath aura#n.", ch, NULL, NULL, TO_ROOM );
			break;
		case AURA_SPIRIT:
			send_to_char( "#CSpiritual energy suffuses your being as a Spirit Aura.#n\n\r", ch );
			act( "$n's form glows with a shimmering #Cspirit aura#n.", ch, NULL, NULL, TO_ROOM );
			break;
	}

	return;
}

void do_ancestralform( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_EMBODY] < 2 )
	{
		send_to_char( "You must train Embodiment to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_ANCESTRAL_FORM_TICKS] > 0 )
	{
		send_to_char( "You are already in Ancestral Form.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_ANCESTRALFORM_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Ancestral Form is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SL_ANCESTRALFORM_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SL_ANCESTRALFORM_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	ch->pcdata->powers[SL_ANCESTRAL_FORM_TICKS] = cfg( CFG_SL_ANCESTRALFORM_DURATION );
	ch->pcdata->powers[SL_ANCESTRALFORM_CD] = cfg( CFG_SL_ANCESTRALFORM_COOLDOWN );

	/* Shift tether toward material */
	ch->rage = UMAX( ch->rage - cfg( CFG_SL_ANCESTRALFORM_TETHER_CHANGE ), 0 );

	send_to_char( "#GYou channel the essence of your ancestors, your form becoming spectral and war-like!#n\n\r", ch );
	act( "$n transforms into a spectral warrior, ancestral spirits merging with $s form!",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

void do_spiritfusion( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_EMBODY] < 3 )
	{
		send_to_char( "You must train Embodiment to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] > 0 )
	{
		send_to_char( "Spirit Fusion is already active.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_SPIRITFUSION_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Spirit Fusion is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SL_SPIRITFUSION_CD] );
		send_to_char( buf, ch );
		return;
	}

	/* Tether requirement: must not be too grounded */
	if ( ch->rage <= 30 )
	{
		send_to_char( "Your tether is too grounded to merge all three auras.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SL_SPIRITFUSION_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] = cfg( CFG_SL_SPIRITFUSION_DURATION );
	ch->pcdata->powers[SL_SPIRITFUSION_CD] = cfg( CFG_SL_SPIRITFUSION_COOLDOWN );

	/* Shift tether toward material (major cost) */
	ch->rage = UMAX( ch->rage - cfg( CFG_SL_SPIRITFUSION_TETHER_CHANGE ), 0 );

	send_to_char( "#t90B8E0All three totem spirits merge into your being - Ward, Wrath, and Spirit fuse as one!#n\n\r", ch );
	act( "$n erupts with blinding spirit energy as three auras merge into a single devastating force!",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

/* =========================================================================
 * DOMINION ABILITIES (Spirit Command - Increases Tether)
 * ========================================================================= */

void do_compel( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	int mana_cost;
	int dam;
	int power_mod;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_DOMINION] < 1 )
	{
		send_to_char( "You must train Dominion to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to compel the spirits.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[SL_COMPEL_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Compel is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SL_COMPEL_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SL_COMPEL_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	/* Calculate damage */
	dam = number_range( cfg( CFG_SL_COMPEL_BASE_MIN ), cfg( CFG_SL_COMPEL_BASE_MAX ) );

	/* Apply spirit power modifier */
	power_mod = get_sl_power_mod( ch, FALSE );
	dam = dam * power_mod / 100;

	/* Tether scaling: +50% damage above tether 90 */
	if ( ch->rage > SL_BALANCED_MAX )
		dam = dam * 150 / 100;

	/* Shift tether toward spirit */
	ch->rage = UMIN( ch->rage + cfg( CFG_SL_COMPEL_TETHER_CHANGE ), SL_TETHER_CAP );

	/* Set cooldown */
	ch->pcdata->powers[SL_COMPEL_CD] = cfg( CFG_SL_COMPEL_COOLDOWN );

	act( "#BYou compel the spirits around you to assault $N with overwhelming force!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#BSpirits surge from the ether and tear into you at $n's command!#n",
		ch, NULL, victim, TO_VICT );
	act( "#B$n commands nearby spirits to assault $N in a terrifying display of dominion!#n",
		ch, NULL, victim, TO_NOTVICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );
	WAIT_STATE( ch, 12 );

	return;
}

void do_possess( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_DOMINION] < 2 )
	{
		send_to_char( "You must train Dominion to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_POSSESS_TICKS] > 0 )
	{
		send_to_char( "You are already possessing a target.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_POSSESS_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Possess is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SL_POSSESS_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( argument[0] == '\0' )
	{
		send_to_char( "Possess whom?\n\r", ch );
		return;
	}

	if ( ( victim = get_char_room( ch, argument ) ) == NULL )
	{
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( !IS_NPC( victim ) )
	{
		send_to_char( "You can only possess NPCs.\n\r", ch );
		return;
	}

	if ( IS_AFFECTED( victim, AFF_CHARM ) )
	{
		send_to_char( "That creature is already charmed.\n\r", ch );
		return;
	}

	if ( victim->level > ch->level + 10 )
	{
		send_to_char( "That creature's will is too strong to dominate.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SL_POSSESS_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	/* Stop victim from fighting and make it a follower */
	if ( victim->fighting != NULL )
		stop_fighting( victim, TRUE );

	add_follower( victim, ch );

	af.type      = skill_lookup( "charm person" );
	af.duration  = cfg( CFG_SL_POSSESS_DURATION );
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char( victim, &af );

	ch->pcdata->powers[SL_POSSESS_TICKS] = cfg( CFG_SL_POSSESS_DURATION );
	ch->pcdata->powers[SL_POSSESS_CD] = cfg( CFG_SL_POSSESS_COOLDOWN );

	/* Shift tether toward spirit */
	ch->rage = UMIN( ch->rage + cfg( CFG_SL_POSSESS_TETHER_CHANGE ), SL_TETHER_CAP );

	act( "#BYou project your spirit into $N, overwhelming their will!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#B$n's eyes glow as $e projects $s spirit into $N, taking control!#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#BA foreign will overwhelms your mind - you are no longer your own!#n",
		ch, NULL, victim, TO_VICT );

	WAIT_STATE( ch, 12 );
	return;
}

void do_spiritarmy( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	AFFECT_DATA af;
	int mana_cost;
	int i, count;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_DOMINION] < 3 )
	{
		send_to_char( "You must train Dominion to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_SPIRIT_ARMY_TICKS] > 0 )
	{
		send_to_char( "Your spirit army is already manifested.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_SPIRITARMY_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Spirit Army is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SL_SPIRITARMY_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SL_SPIRITARMY_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	if ( ( pMobIndex = get_mob_index( VNUM_SL_SPIRIT_WARRIOR ) ) == NULL )
	{
		send_to_char( "Spirit warrior template not found. Please contact an immortal.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	count = cfg( CFG_SL_SPIRITARMY_COUNT );

	for ( i = 0; i < count; i++ )
	{
		CHAR_DATA *warrior;

		warrior = create_mobile( pMobIndex );

		warrior->level   = ch->level;
		warrior->hit     = ch->max_hit / 5;
		warrior->max_hit = ch->max_hit / 5;
		warrior->hitroll = ch->hitroll / 3;
		warrior->damroll = ch->damroll / 3;
		warrior->armor   = -200;

		snprintf( buf, sizeof(buf), "%s's spirit warrior", ch->name );
		free_string( warrior->short_descr );
		warrior->short_descr = str_dup( buf );

		{
			const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
			const char *pc = br ? br->primary_color : "";
			snprintf( buf, sizeof(buf),
				"%sA spectral warrior bound to %s hovers here, weapons ready.#n\n\r",
				pc, ch->name );
		}
		free_string( warrior->long_descr );
		warrior->long_descr = str_dup( buf );

		free_string( warrior->name );
		warrior->name = str_dup( "spirit warrior spectral" );

		SET_BIT( warrior->act, ACT_SENTINEL );
		SET_BIT( warrior->act, ACT_NOEXP );
		warrior->spec_fun = NULL;

		free_string( warrior->lord );
		warrior->lord = str_dup( ch->name );
		warrior->wizard = ch;

		char_to_room( warrior, ch->in_room );
		add_follower( warrior, ch );

		af.type      = skill_lookup( "charm person" );
		af.duration  = 666;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = AFF_CHARM;
		affect_to_char( warrior, &af );
	}

	ch->pcdata->stats[SL_STAT_ARMY_COUNT] = count;
	ch->pcdata->powers[SL_SPIRIT_ARMY_TICKS] = cfg( CFG_SL_SPIRITARMY_DURATION );
	ch->pcdata->powers[SL_SPIRITARMY_CD] = cfg( CFG_SL_SPIRITARMY_COOLDOWN );

	/* Shift tether toward spirit */
	ch->rage = UMIN( ch->rage + cfg( CFG_SL_SPIRITARMY_TETHER_CHANGE ), SL_TETHER_CAP );

	snprintf( buf, sizeof(buf),
		"#B%d spectral warriors materialize from the spirit realm at your command!#n\n\r", count );
	send_to_char( buf, ch );
	act( "Spectral warriors materialize from the spirit realm at $n's command!",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

void do_soulstorm( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int mana_cost;
	int dam;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_DOMINION] < 4 )
	{
		send_to_char( "You must train Dominion to level 4 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to unleash a soul storm.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_SOULSTORM_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Soul Storm is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[SL_SOULSTORM_CD] );
		send_to_char( buf, ch );
		return;
	}

	/* Tether requirement: deep in spirit territory */
	if ( ch->rage <= 100 )
	{
		send_to_char( "Your tether must be above 100 to unleash a soul storm.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_SL_SOULSTORM_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	/* Calculate base damage: 400-700 + (tether - center) * 5 */
	dam = number_range( cfg( CFG_SL_SOULSTORM_BASE_MIN ), cfg( CFG_SL_SOULSTORM_BASE_MAX ) );
	dam += ( ch->rage - SL_TETHER_CENTER ) * 5;

	ch->pcdata->powers[SL_SOULSTORM_CD] = cfg( CFG_SL_SOULSTORM_COOLDOWN );

	send_to_char( "#t90B8E0You release all accumulated spirit energy in a devastating soul storm!#n\n\r", ch );
	act( "#t90B8E0$n unleashes a devastating storm of spirit energy that engulfs the area!#n",
		ch, NULL, NULL, TO_ROOM );

	/* AoE: hit everyone fighting us */
	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node)
	{

		if ( vch->fighting != ch )
			continue;

		damage( ch, vch, dam, TYPE_UNDEFINED );
	}

	/* Tether aftermath: move 30 points toward center */
	if ( ch->rage > SL_TETHER_CENTER )
		ch->rage = UMAX( ch->rage - 30, SL_TETHER_CENTER );
	else
		ch->rage = UMIN( ch->rage + 30, SL_TETHER_CENTER );

	send_to_char( "#CThe spiritual exhaustion drags your tether back toward balance.#n\n\r", ch );

	WAIT_STATE( ch, 12 );
	return;
}

/* =========================================================================
 * TRANSCENDENCE ABILITIES (Ultimate Powers - Neutral Tether)
 * ========================================================================= */

void do_ancestralwisdom( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_TRANSCEND] < 1 )
	{
		send_to_char( "You must train Transcendence to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_WISDOM_TICKS] > 0 )
	{
		send_to_char( "Ancestral Wisdom is already active.\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[SL_STAT_WISDOM_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Ancestral Wisdom is on cooldown for %d more ticks.\n\r",
			ch->pcdata->stats[SL_STAT_WISDOM_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SL_ANCESTRALWISDOM_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	ch->pcdata->powers[SL_WISDOM_TICKS] = cfg( CFG_SL_ANCESTRALWISDOM_DURATION );
	ch->pcdata->stats[SL_STAT_WISDOM_CD] = cfg( CFG_SL_ANCESTRALWISDOM_COOLDOWN );

	/* Balance bonus message */
	if ( ch->rage >= 70 && ch->rage <= 80 )
		send_to_char( "#CThe ancestors recognize your perfect balance - wisdom flows freely!#n\n\r", ch );

	send_to_char( "#CThe combined wisdom of your ancestors floods your mind, sharpening every sense.#n\n\r", ch );
	act( "$n's eyes glow with the accumulated wisdom of generations.",
		ch, NULL, NULL, TO_ROOM );

	return;
}

void do_spiritcleanse( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;
	int heal;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_TRANSCEND] < 2 )
	{
		send_to_char( "You must train Transcendence to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[SL_STAT_CLEANSE_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Spirit Cleanse is on cooldown for %d more ticks.\n\r",
			ch->pcdata->stats[SL_STAT_CLEANSE_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SL_SPIRITCLEANSE_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	/* Heal 20% max HP */
	heal = ch->max_hit * cfg( CFG_SL_SPIRITCLEANSE_HEAL_PCT ) / 100;
	heal_char( ch, heal );

	/* Reset tether to center */
	ch->rage = SL_TETHER_CENTER;

	ch->pcdata->stats[SL_STAT_CLEANSE_CD] = cfg( CFG_SL_SPIRITCLEANSE_COOLDOWN );

	snprintf( buf, sizeof(buf),
		"#t90B8E0Pure ancestral energy surges through you, purifying body and spirit for #C%d#t90B8E0 health!#n\n\r", heal );
	send_to_char( buf, ch );
	send_to_char( "#CYour tether snaps back to perfect balance.#n\n\r", ch );
	act( "$n is engulfed in pure #t90B8E0white light#n as ancestral power purifies $s being!",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

void do_ascension( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int mana_cost;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_SPIRITLORD ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_TRAIN_TRANSCEND] < 3 )
	{
		send_to_char( "You must train Transcendence to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_ASCENSION_TICKS] > 0 )
	{
		send_to_char( "You have already ascended.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[SL_ASCENSION_AFTERMATH] > 0 )
	{
		send_to_char( "You are still re-materializing from your last ascension.\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[SL_STAT_ASCENSION_CD] > 0 )
	{
		snprintf( buf, sizeof(buf), "Ascension is on cooldown for %d more ticks.\n\r",
			ch->pcdata->stats[SL_STAT_ASCENSION_CD] );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_SL_ASCENSION_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	ch->pcdata->powers[SL_ASCENSION_TICKS] = cfg( CFG_SL_ASCENSION_DURATION );
	ch->pcdata->stats[SL_STAT_ASCENSION_CD] = cfg( CFG_SL_ASCENSION_COOLDOWN );
	ch->pcdata->stats[SL_STAT_ASCENSIONS_USED]++;

	/* Reset tether to center */
	ch->rage = SL_TETHER_CENTER;

	send_to_char( "#t90B8E0You shed your physical form and ASCEND to pure spirit!#n\n\r", ch );
	send_to_char( "#t90B8E0All physical constraints fall away - you are spirit incarnate!#n\n\r", ch );
	act( "$n's physical form dissolves into blinding #t90B8E0spiritual light#n - $e has ASCENDED!",
		ch, NULL, NULL, TO_ROOM );

	WAIT_STATE( ch, 12 );
	return;
}

/* =========================================================================
 * LORDARMOR - Class equipment creation wrapper
 * ========================================================================= */

void do_lordarmor( CHAR_DATA *ch, char *argument )
{
	do_classarmor_generic( ch, argument, CLASS_SPIRITLORD );
}

/* =========================================================================
 * UPDATE_SPIRITLORD - Per-tick processing
 *
 * Called from update.c for each Spirit Lord character. Handles:
 * - Tether drift toward center (slower than Shaman)
 * - Spirit manifestation at extremes
 * - Aura tick effects (damage, drain, defense)
 * - Buff durations and cooldown decrements
 * - Spirit army warrior management
 * - Ascension auto-bolt and aftermath
 * ========================================================================= */

void update_spiritlord( CHAR_DATA *ch )
{
	char buf[MAX_STRING_LENGTH];

	/* ---- Tether drift toward center (+/-1 per tick, slower than Shaman) ---- */
	if ( ch->rage > SL_TETHER_CENTER )
		ch->rage = UMAX( SL_TETHER_CENTER, ch->rage - 1 );
	else if ( ch->rage < SL_TETHER_CENTER )
		ch->rage = UMIN( SL_TETHER_CENTER, ch->rage + 1 );

	/* ---- Spirit manifestation at extremes (10% chance) ---- */
	if ( ch->rage <= SL_MANIFEST_LOW || ch->rage >= SL_MANIFEST_HIGH )
	{
		if ( number_range( 1, 100 ) <= 10 )
		{
			switch ( number_range( 1, 3 ) )
			{
			case 1: /* Ancestor whisper - heal */
				{
					int heal = ch->max_hit / 15;
					ch->hit = UMIN( ch->max_hit, ch->hit + heal );
					snprintf( buf, sizeof(buf),
						"#t90B8E0Spirit manifestation! Ancient ancestors surge through you, restoring %d health.#n\n\r", heal );
					send_to_char( buf, ch );
				}
				break;
			case 2: /* Spirit surge - brief stun */
				send_to_char( "#t90B8E0Spirit manifestation! The boundary between worlds tears at your mind!#n\n\r", ch );
				WAIT_STATE( ch, 12 );
				break;
			case 3: /* Tether snap toward center */
				ch->rage = ( ch->rage < SL_TETHER_CENTER )
					? UMIN( ch->rage + 15, SL_TETHER_CENTER )
					: UMAX( ch->rage - 15, SL_TETHER_CENTER );
				send_to_char( "#t90B8E0Spirit manifestation! Your tether snaps violently toward balance.#n\n\r", ch );
				break;
			}
		}
	}

	/* ---- Ascension processing (highest priority - overrides auras) ---- */
	if ( ch->pcdata->powers[SL_ASCENSION_TICKS] > 0 )
	{
		ch->pcdata->powers[SL_ASCENSION_TICKS]--;

		/* Auto spirit bolt if fighting */
		if ( ch->fighting != NULL )
		{
			int dam = number_range( cfg( CFG_SL_ASCENSION_BOLT_MIN ),
			                        cfg( CFG_SL_ASCENSION_BOLT_MAX ) );
			int power_mod = get_sl_power_mod( ch, FALSE );
			dam = dam * power_mod / 100;

			act( "#t90B8E0Pure spirit energy lashes out from your ascended form at $N!#n",
				ch, NULL, ch->fighting, TO_CHAR );
			act( "#t90B8E0$n's ascended form lashes $N with pure spirit energy!#n",
				ch, NULL, ch->fighting, TO_NOTVICT );
			damage( ch, ch->fighting, dam, TYPE_UNDEFINED );
		}

		if ( ch->pcdata->powers[SL_ASCENSION_TICKS] == 0 )
		{
			ch->pcdata->powers[SL_ASCENSION_AFTERMATH] = cfg( CFG_SL_ASCENSION_AFTERMATH );
			send_to_char( "#t90B8E0Your ascension ends - you crash back into physical form!#n\n\r", ch );
			send_to_char( "#RThe aftermath of ascension leaves you weakened and disoriented.#n\n\r", ch );
			act( "$n's spiritual form suddenly solidifies as $e crashes back to the material plane!",
				ch, NULL, NULL, TO_ROOM );
		}
	}

	/* ---- Ascension aftermath (stunned state) ---- */
	if ( ch->pcdata->powers[SL_ASCENSION_AFTERMATH] > 0 )
	{
		ch->pcdata->powers[SL_ASCENSION_AFTERMATH]--;
		WAIT_STATE( ch, 24 );

		if ( ch->pcdata->powers[SL_ASCENSION_AFTERMATH] == 0 )
			send_to_char( "#CThe aftermath of ascension passes. You feel whole again.#n\n\r", ch );
	}

	/* ---- Embody aura tick effects ---- */
	if ( ch->pcdata->powers[SL_ASCENSION_TICKS] == 0
	&&   ch->pcdata->powers[SL_EMBODY_TYPE] != AURA_NONE )
	{
		int aura = ch->pcdata->powers[SL_EMBODY_TYPE];

		/* Wrath aura: damage fighting target each tick */
		if ( ( aura == AURA_WRATH || ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] > 0 )
		&&   ch->fighting != NULL )
		{
			int dam = number_range( cfg( CFG_SL_EMBODY_WRATH_TICK_MIN ),
			                        cfg( CFG_SL_EMBODY_WRATH_TICK_MAX ) );
			int power_mod = get_sl_power_mod( ch, TRUE );
			dam = dam * power_mod / 100;

			act( "#RYour wrath aura sears $N with spirit fury!#n",
				ch, NULL, ch->fighting, TO_CHAR );
			act( "#R$n's wrath aura sears you with spirit fury!#n",
				ch, NULL, ch->fighting, TO_VICT );
			damage( ch, ch->fighting, dam, TYPE_UNDEFINED );
		}

		/* Spirit aura: mana drain from fighting target */
		if ( ( aura == AURA_SPIRIT || ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] > 0 )
		&&   ch->fighting != NULL )
		{
			int drain   = cfg( CFG_SL_EMBODY_SPIRIT_MANA_DRAIN );
			int restore = cfg( CFG_SL_EMBODY_SPIRIT_MANA_RESTORE );

			if ( !IS_NPC( ch->fighting ) && ch->fighting->mana >= drain )
				ch->fighting->mana -= drain;

			ch->mana = UMIN( ch->mana + restore, ch->max_mana );

			act( "#CYour spirit aura drains mystical energy from $N!#n",
				ch, NULL, ch->fighting, TO_CHAR );
		}
	}

	/* ---- Spirit Fusion tick processing ---- */
	if ( ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] > 0 )
	{
		int mana_tick = cfg( CFG_SL_SPIRITFUSION_MANA_TICK );
		ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS]--;

		/* Mana cost per tick to sustain triple aura */
		if ( ch->mana >= mana_tick )
		{
			ch->mana -= mana_tick;
		}
		else
		{
			/* Not enough mana - fusion collapses */
			ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] = 0;
			send_to_char( "#RYour mana is exhausted! Spirit Fusion collapses!#n\n\r", ch );
		}

		if ( ch->pcdata->powers[SL_SPIRIT_FUSION_TICKS] == 0 )
			send_to_char( "#BSpirit Fusion fades - you return to a single aura.#n\n\r", ch );
	}

	/* ---- Ancestral Form duration countdown ---- */
	if ( ch->pcdata->powers[SL_ANCESTRAL_FORM_TICKS] > 0 )
	{
		ch->pcdata->powers[SL_ANCESTRAL_FORM_TICKS]--;
		if ( ch->pcdata->powers[SL_ANCESTRAL_FORM_TICKS] == 0 )
		{
			send_to_char( "#CYour ancestral form fades - the spirits return to rest.#n\n\r", ch );
			act( "$n's ancestral form shimmers and fades back to normal.",
				ch, NULL, NULL, TO_ROOM );
		}
	}

	/* ---- Spirit Army duration countdown ---- */
	if ( ch->pcdata->powers[SL_SPIRIT_ARMY_TICKS] > 0 )
	{
		ch->pcdata->powers[SL_SPIRIT_ARMY_TICKS]--;
		if ( ch->pcdata->powers[SL_SPIRIT_ARMY_TICKS] == 0 )
		{
			CHAR_DATA *vch;
			CHAR_DATA *vch_next;

			/* Despawn all spirit warriors */
			LIST_FOR_EACH_SAFE( vch, vch_next, &g_characters, CHAR_DATA, char_node )
			{

				if ( IS_NPC( vch )
				&&   vch->wizard == ch
				&&   vch->pIndexData
				&&   vch->pIndexData->vnum == VNUM_SL_SPIRIT_WARRIOR )
				{
					act( "$n fades back into the spirit realm.", vch, NULL, NULL, TO_ROOM );
					extract_char( vch, TRUE );
				}
			}

			ch->pcdata->stats[SL_STAT_ARMY_COUNT] = 0;
			send_to_char( "#BYour spirit army fades back beyond the veil.#n\n\r", ch );
		}
	}

	/* ---- Possess duration countdown ---- */
	if ( ch->pcdata->powers[SL_POSSESS_TICKS] > 0 )
	{
		ch->pcdata->powers[SL_POSSESS_TICKS]--;
		if ( ch->pcdata->powers[SL_POSSESS_TICKS] == 0 )
		{
			ch->pcdata->powers[SL_POSSESS_TARGET] = 0;
			send_to_char( "#CYour possession fades - the spirit releases its grip.#n\n\r", ch );
		}
	}

	/* ---- Ancestral Wisdom duration countdown + HP regen ---- */
	if ( ch->pcdata->powers[SL_WISDOM_TICKS] > 0 )
	{
		int regen = ch->max_hit * cfg( CFG_SL_ANCESTRALWISDOM_REGEN_PCT ) / 100;
		ch->hit = UMIN( ch->max_hit, ch->hit + regen );

		ch->pcdata->powers[SL_WISDOM_TICKS]--;
		if ( ch->pcdata->powers[SL_WISDOM_TICKS] == 0 )
			send_to_char( "#CThe wisdom of your ancestors fades from your mind.#n\n\r", ch );
	}

	/* ---- Cooldown decrements (powers[14-19]) ---- */
	if ( ch->pcdata->powers[SL_ANCESTRALFORM_CD] > 0 )
		ch->pcdata->powers[SL_ANCESTRALFORM_CD]--;
	if ( ch->pcdata->powers[SL_SPIRITFUSION_CD] > 0 )
		ch->pcdata->powers[SL_SPIRITFUSION_CD]--;
	if ( ch->pcdata->powers[SL_COMPEL_CD] > 0 )
		ch->pcdata->powers[SL_COMPEL_CD]--;
	if ( ch->pcdata->powers[SL_POSSESS_CD] > 0 )
		ch->pcdata->powers[SL_POSSESS_CD]--;
	if ( ch->pcdata->powers[SL_SPIRITARMY_CD] > 0 )
		ch->pcdata->powers[SL_SPIRITARMY_CD]--;
	if ( ch->pcdata->powers[SL_SOULSTORM_CD] > 0 )
		ch->pcdata->powers[SL_SOULSTORM_CD]--;

	/* ---- Cooldown decrements (stats[]) ---- */
	if ( ch->pcdata->stats[SL_STAT_WISDOM_CD] > 0 )
		ch->pcdata->stats[SL_STAT_WISDOM_CD]--;
	if ( ch->pcdata->stats[SL_STAT_CLEANSE_CD] > 0 )
		ch->pcdata->stats[SL_STAT_CLEANSE_CD]--;
	if ( ch->pcdata->stats[SL_STAT_ASCENSION_CD] > 0 )
		ch->pcdata->stats[SL_STAT_ASCENSION_CD]--;
}
