/***************************************************************************
 *  Wyrm class - upgrade from Dragonkin, full dragon transformation         *
 *  Enhanced Essence cap (150), dragon form, devastating abilities          *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"
#include "dragonkin.h"

/* Forward declarations for update functions */
void update_wyrm( CHAR_DATA *ch );

/* Helper: Get attunement name for messages */
static const char *wyrm_attune_name( int attune ) {
	switch ( attune ) {
		case ATTUNE_FIRE:  return "infernal flames";
		case ATTUNE_FROST: return "freezing winds";
		case ATTUNE_STORM: return "crackling lightning";
		case ATTUNE_EARTH: return "crushing stone shards";
		default:           return "elemental fury";
	}
}

/*
 * Wyrmbreath - Enhanced breath, larger area, higher damage
 */
void do_wyrmbreath( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char buf[MAX_STRING_LENGTH];
	int dam, cost, attune;
	const char *attack_name;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_DEVASTATION] < 1 ) {
		send_to_char( "You haven't trained Devastation yet. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_WYRMBREATH_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for wyrmbreath.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	attune = ch->pcdata->powers[DRAGON_ATTUNEMENT];
	attack_name = wyrm_attune_name( attune );

	ch->rage -= cost;
	dam = cfg( CFG_ABILITY_WYRM_WYRMBREATH_BASE_DAMAGE ) +
	      ( ch->rage * cfg( CFG_ABILITY_WYRM_WYRMBREATH_ESSENCE_MULT ) );

	/* Primordial bonus */
	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] >= 3 )
		dam = dam * ( 100 + cfg( CFG_ABILITY_WYRM_PRIMORDIAL_DAMAGE_MULT ) ) / 100;

	act( "You rear back and unleash a devastating torrent of $t!", ch, attack_name, NULL, TO_CHAR );
	act( "$n rears back and unleashes a devastating torrent of $t!", ch, attack_name, NULL, TO_ROOM );

	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		damage( ch, vch, dam, gsn_punch );
	}
	WAIT_STATE( ch, 12 );
}

/*
 * Cataclysm - Massive AoE, leaves lingering damage field
 */
void do_cataclysm( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char buf[MAX_STRING_LENGTH];
	int dam, cost;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_DEVASTATION] < 2 ) {
		send_to_char( "You need Devastation level 2 for Cataclysm. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_CATACLYSM_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for cataclysm.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	ch->rage -= cost;
	dam = cfg( CFG_ABILITY_WYRM_CATACLYSM_BASE_DAMAGE );

	/* Primordial bonus */
	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] >= 3 )
		dam = dam * ( 100 + cfg( CFG_ABILITY_WYRM_PRIMORDIAL_DAMAGE_MULT ) ) / 100;

	send_to_char( "#RYou call forth a cataclysm of elemental destruction!#n\n\r", ch );
	act( "$n calls forth a cataclysm of elemental destruction!", ch, NULL, NULL, TO_ROOM );

	/* Set terrain effect for lingering damage */
	ch->pcdata->powers[DRAGON_TERRAIN_TYPE] = ch->pcdata->powers[DRAGON_ATTUNEMENT] + 1;

	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		damage( ch, vch, dam, gsn_punch );
	}
	WAIT_STATE( ch, 18 );
}

/*
 * Annihilate - Single-target execute ability (bonus damage at low HP)
 */
void do_annihilate( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int dam, cost;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_DEVASTATION] < 3 ) {
		send_to_char( "You need Devastation level 3 for Annihilate. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	one_argument( argument, arg );
	if ( arg[0] == '\0' && ch->fighting == NULL ) {
		send_to_char( "Annihilate whom?\n\r", ch );
		return;
	}

	if ( arg[0] == '\0' )
		victim = ch->fighting;
	else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( is_safe( ch, victim ) ) return;

	cost = cfg( CFG_ABILITY_WYRM_ANNIHILATE_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for annihilate.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	ch->rage -= cost;
	dam = cfg( CFG_ABILITY_WYRM_ANNIHILATE_BASE_DAMAGE );

	/* Execute bonus: more damage when target is low HP */
	if ( victim->hit * 100 / victim->max_hit <= cfg( CFG_ABILITY_WYRM_ANNIHILATE_EXECUTE_THRESHOLD ) ) {
		dam = dam * ( 100 + cfg( CFG_ABILITY_WYRM_ANNIHILATE_EXECUTE_BONUS ) ) / 100;
		send_to_char( "#RYou sense your prey's weakness and strike to annihilate!#n\n\r", ch );
	}

	/* Primordial bonus */
	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] >= 3 )
		dam = dam * ( 100 + cfg( CFG_ABILITY_WYRM_PRIMORDIAL_DAMAGE_MULT ) ) / 100;

	act( "You focus your draconic fury and strike to annihilate $N!", ch, NULL, victim, TO_CHAR );
	act( "$n focuses draconic fury and strikes to annihilate you!", ch, NULL, victim, TO_VICT );
	act( "$n focuses draconic fury and strikes to annihilate $N!", ch, NULL, victim, TO_NOTVICT );

	damage( ch, victim, dam, gsn_punch );
	WAIT_STATE( ch, 12 );
}

/*
 * Apocalypse - Ultimate: room-wide devastation + terrain effect
 */
void do_apocalypse( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char buf[MAX_STRING_LENGTH];
	int dam, cost;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_DEVASTATION] < 4 ) {
		send_to_char( "You need Devastation level 4 for Apocalypse. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_APOCALYPSE_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for apocalypse.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	ch->rage -= cost;
	dam = cfg( CFG_ABILITY_WYRM_APOCALYPSE_BASE_DAMAGE );

	/* Primordial bonus */
	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] >= 3 )
		dam = dam * ( 100 + cfg( CFG_ABILITY_WYRM_PRIMORDIAL_DAMAGE_MULT ) ) / 100;

	send_to_char( "#x160You unleash the APOCALYPSE!#n\n\r", ch );
	act( "#x160$n unleashes the APOCALYPSE!#n", ch, NULL, NULL, TO_ROOM );

	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		damage( ch, vch, dam, gsn_punch );
	}
	WAIT_STATE( ch, 24 );
}

/*
 * Dragonfear - AoE fear effect, chance to flee/stun
 */
void do_dragonfear( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char buf[MAX_STRING_LENGTH];
	int cost, stun_chance;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_DOMINION] < 1 ) {
		send_to_char( "You haven't trained Dominion yet. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_DRAGONFEAR_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for dragonfear.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	ch->rage -= cost;
	stun_chance = cfg( CFG_ABILITY_WYRM_DRAGONFEAR_STUN_CHANCE );

	send_to_char( "#x160You unleash your terrifying draconic presence!#n\n\r", ch );
	act( "$n unleashes a wave of terrifying draconic presence!", ch, NULL, NULL, TO_ROOM );

	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		if ( number_percent() < stun_chance ) {
			act( "$N is stunned by your fearsome presence!", ch, NULL, vch, TO_CHAR );
			act( "You are stunned by $n's fearsome presence!", ch, NULL, vch, TO_VICT );
			WAIT_STATE( vch, 12 );
		} else {
			act( "$N cowers in fear!", ch, NULL, vch, TO_CHAR );
			act( "You cower in fear before $n!", ch, NULL, vch, TO_VICT );
			do_flee( vch, "" );
		}
	}
	WAIT_STATE( ch, 12 );
}

/*
 * Terrainshift - Change room terrain to match attunement (combat effects)
 */
void do_terrainshift( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int cost, attune;
	const char *terrain_name;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_DOMINION] < 2 ) {
		send_to_char( "You need Dominion level 2 for Terrainshift. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_TERRAINSHIFT_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for terrainshift.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	attune = ch->pcdata->powers[DRAGON_ATTUNEMENT];
	switch ( attune ) {
		case ATTUNE_FIRE:  terrain_name = "scorched earth"; break;
		case ATTUNE_FROST: terrain_name = "frozen wasteland"; break;
		case ATTUNE_STORM: terrain_name = "crackling storm field"; break;
		case ATTUNE_EARTH: terrain_name = "jagged stone spires"; break;
		default:           terrain_name = "elemental chaos"; break;
	}

	ch->rage -= cost;
	ch->pcdata->powers[DRAGON_TERRAIN_TYPE] = attune + 1;

	snprintf( buf, sizeof( buf ), "You reshape the terrain into %s!\n\r", terrain_name );
	send_to_char( buf, ch );
	act( "$n reshapes the terrain with draconic power!", ch, NULL, NULL, TO_ROOM );
	WAIT_STATE( ch, 8 );
}

/*
 * Dragonlord - Charm/dominate dragon-type mobs
 */
void do_dragonlord( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int cost;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_DOMINION] < 3 ) {
		send_to_char( "You need Dominion level 3 for Dragonlord. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	one_argument( argument, arg );
	if ( arg[0] == '\0' ) {
		send_to_char( "Command which creature?\n\r", ch );
		return;
	}

	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( !IS_NPC( victim ) ) {
		send_to_char( "Dragonlord only works on creatures.\n\r", ch );
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_DRAGONLORD_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for dragonlord.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	ch->rage -= cost;

	/* Attempt to charm - check if already charmed */
	if ( IS_AFFECTED( victim, AFF_CHARM ) ) {
		send_to_char( "That creature is already under someone's control.\n\r", ch );
		return;
	}

	/* Set charm */
	SET_BIT( victim->affected_by, AFF_CHARM );
	victim->master = ch;
	victim->leader = ch;

	act( "You assert your draconic dominance over $N!", ch, NULL, victim, TO_CHAR );
	act( "$n asserts draconic dominance over $N!", ch, NULL, victim, TO_ROOM );
	WAIT_STATE( ch, 8 );
}

/*
 * Wyrmform - Dragon transformation: stat bonuses, unlocks abilities
 */
void do_wyrmform( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int cost, duration;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] < 1 ) {
		send_to_char( "You haven't trained Ascension yet. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	/* Toggle off */
	if ( ch->pcdata->powers[DRAGON_WYRMFORM] > 0 ) {
		send_to_char( "Your draconic form shimmers and fades.\n\r", ch );
		act( "$n's massive dragon form shrinks back to humanoid size.", ch, NULL, NULL, TO_ROOM );
		ch->pcdata->powers[DRAGON_WYRMFORM] = 0;
		ch->pcdata->powers[DRAGON_ANCIENTWRATH] = 0;  /* Ancientwrath ends with form */
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_WYRMFORM_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence to transform.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	ch->rage -= cost;
	duration = cfg( CFG_ABILITY_WYRM_WYRMFORM_DURATION );
	ch->pcdata->powers[DRAGON_WYRMFORM] = duration;

	send_to_char( "#x220Your body surges with primordial power as you transform into a massive dragon!#n\n\r", ch );
	act( "#x220$n's body explodes in draconic energy, transforming into a massive wyrm!#n", ch, NULL, NULL, TO_ROOM );
	WAIT_STATE( ch, 8 );
}

/*
 * Ancientwrath - While in form: bonus elemental damage on all attacks
 */
void do_ancientwrath( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int cost, duration;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] < 2 ) {
		send_to_char( "You need Ascension level 2 for Ancient Wrath. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_WYRMFORM] == 0 ) {
		send_to_char( "You must be in wyrmform to use Ancient Wrath.\n\r", ch );
		return;
	}

	/* Toggle off */
	if ( ch->pcdata->powers[DRAGON_ANCIENTWRATH] > 0 ) {
		send_to_char( "You calm your ancient wrath.\n\r", ch );
		ch->pcdata->powers[DRAGON_ANCIENTWRATH] = 0;
		return;
	}

	cost = cfg( CFG_ABILITY_WYRM_ANCIENTWRATH_ESSENCE_COST );
	if ( ch->rage < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d Essence for ancient wrath.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	ch->rage -= cost;
	duration = cfg( CFG_ABILITY_WYRM_ANCIENTWRATH_DURATION );
	ch->pcdata->powers[DRAGON_ANCIENTWRATH] = duration;

	send_to_char( "#RYour ancient draconic fury ignites!#n\n\r", ch );
	act( "$n's eyes blaze with ancient draconic fury!", ch, NULL, NULL, TO_ROOM );
	WAIT_STATE( ch, 4 );
}

/*
 * Primordial - Permanent passive improvements to all abilities
 */
void do_primordial( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] < 3 ) {
		send_to_char( "You need Ascension level 3 for Primordial. See #Rwyrmtrain#n.\n\r", ch );
		return;
	}

	/* Primordial is passive - display status */
	send_to_char( "#x220Primordial Dragon Power:#n\n\r", ch );
	send_to_char( "  - All abilities deal 10% bonus damage\n\r", ch );
	send_to_char( "  - Passive Essence regeneration increased\n\r", ch );
	send_to_char( "This power is always active.\n\r", ch );
}

/*
 * Wyrmtrain - Training command for Wyrm abilities
 */
void do_wyrmtrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int cost;
	int *train_ptr = NULL;
	int max_level = 0;
	const char *tree_name = NULL;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "#x202~#x220[#n Wyrm Training #x220]#x202~#n\n\r", ch );
		send_to_char( "Available training paths:\n\r", ch );
		snprintf( buf, sizeof( buf ), "  #x220devastation#n - Devastation (%d/4) - Ultimate destruction\n\r", ch->pcdata->powers[WYRM_TRAIN_DEVASTATION] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  #x220dominion#n    - Dominion    (%d/3) - Control and fear\n\r", ch->pcdata->powers[WYRM_TRAIN_DOMINION] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  #x220ascension#n   - Ascension   (%d/3) - Dragon transformation\n\r", ch->pcdata->powers[WYRM_TRAIN_ASCENSION] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: wyrmtrain <path>\n\r", ch );
		return;
	}

	if ( !str_prefix( arg, "devastation" ) ) {
		train_ptr = &ch->pcdata->powers[WYRM_TRAIN_DEVASTATION];
		max_level = 4;
		tree_name = "Devastation";
	}
	else if ( !str_prefix( arg, "dominion" ) ) {
		train_ptr = &ch->pcdata->powers[WYRM_TRAIN_DOMINION];
		max_level = 3;
		tree_name = "Dominion";
	}
	else if ( !str_prefix( arg, "ascension" ) ) {
		train_ptr = &ch->pcdata->powers[WYRM_TRAIN_ASCENSION];
		max_level = 3;
		tree_name = "Ascension";
	}
	else {
		send_to_char( "Unknown training path. See #Rwyrmtrain#n for options.\n\r", ch );
		return;
	}

	if ( *train_ptr >= max_level ) {
		snprintf( buf, sizeof( buf ), "You have already mastered %s.\n\r", tree_name );
		send_to_char( buf, ch );
		return;
	}

	/* Wyrm training costs more: (level + 1) * 50 */
	cost = ( *train_ptr + 1 ) * 50;

	if ( ch->practice < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d primal to train %s level %d.\n\r", cost, tree_name, *train_ptr + 1 );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	( *train_ptr )++;

	snprintf( buf, sizeof( buf ), "You have trained %s to level %d.\n\r", tree_name, *train_ptr );
	send_to_char( buf, ch );
}

/*
 * Wyrmarmor - Create class-specific armor
 */
void do_wyrmarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_WYRM );
}

/*
 * Wyrm tick update - called from update.c
 * Note: Essence handling is in update_dragonkin() since Wyrm shares rage field
 */
void update_wyrm( CHAR_DATA *ch ) {
	if ( IS_NPC( ch ) )
		return;
	if ( !IS_CLASS( ch, CLASS_WYRM ) )
		return;

	/* Primordial passive: bonus essence regen */
	if ( ch->pcdata->powers[WYRM_TRAIN_ASCENSION] >= 3 ) {
		int regen = cfg( CFG_ABILITY_WYRM_PRIMORDIAL_ESSENCE_REGEN );
		if ( ch->rage < WYRM_ESSENCE_MAX )
			ch->rage = UMIN( ch->rage + regen, WYRM_ESSENCE_MAX );
	}

	/* Wyrm-specific buff timers */
	if ( ch->pcdata->powers[DRAGON_WYRMFORM] > 0 ) {
		ch->pcdata->powers[DRAGON_WYRMFORM]--;
		if ( ch->pcdata->powers[DRAGON_WYRMFORM] == 0 ) {
			send_to_char( "Your wyrmform fades, returning you to humanoid shape.\n\r", ch );
			act( "$n's draconic form shimmers and shrinks back to humanoid size.", ch, NULL, NULL, TO_ROOM );
			ch->pcdata->powers[DRAGON_ANCIENTWRATH] = 0;  /* Ancientwrath ends with form */
		}
	}

	if ( ch->pcdata->powers[DRAGON_ANCIENTWRATH] > 0 ) {
		ch->pcdata->powers[DRAGON_ANCIENTWRATH]--;
		if ( ch->pcdata->powers[DRAGON_ANCIENTWRATH] == 0 )
			send_to_char( "Your ancient wrath subsides.\n\r", ch );
	}

	/* Terrain shift decay */
	if ( ch->pcdata->powers[DRAGON_TERRAIN_TYPE] > 0 ) {
		/* Terrain effects can persist - decayed elsewhere or manually cleared */
	}
}
