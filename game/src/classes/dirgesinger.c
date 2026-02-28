/***************************************************************************
 *  Dirgesinger class - martial bard with battle chants and sonic damage   *
 *  Uses the "Resonance" resource (ch->rage) that builds during combat.    *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"
#include "dirgesinger.h"

/* Forward declarations for update functions */
void update_dirgesinger( CHAR_DATA *ch );

/*
 * Resonance status display
 */
void do_resonance( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) && !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	snprintf( buf, sizeof( buf ),"#x136~#x178[#n Resonance Status #x178]#x136~#n\n\r" );
	send_to_char( buf, ch );
	snprintf( buf, sizeof( buf ),"Current Resonance: #x178%d#n / %d\n\r", ch->rage,
		IS_CLASS( ch, CLASS_SIREN ) ? 150 : 100 );
	send_to_char( buf, ch );

	if ( ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] )
		send_to_char( "#x178[#nWarsong active#x178]#n\n\r", ch );
	if ( ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] > 0 ) {
		snprintf( buf, sizeof( buf ),"#x178[#nBattlehymn: #C%d#n ticks remaining#x178]#n\n\r", ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE] > 0 ) {
		snprintf( buf, sizeof( buf ),"#x178[#nIronsong barrier: #C%d#n HP remaining#x178]#n\n\r", ch->pcdata->stats[DIRGE_ARMOR_BONUS] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] > 0 ) {
		snprintf( buf, sizeof( buf ),"#x178[#nCadence: #C%d#n ticks remaining#x178]#n\n\r", ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DIRGE_DIRGE_TICKS] > 0 ) {
		snprintf( buf, sizeof( buf ),"#x178[#nDirge DOT: #C%d#n stacks, #C%d#n ticks remaining#x178]#n\n\r",
			ch->pcdata->powers[DIRGE_DIRGE_STACKS], ch->pcdata->powers[DIRGE_DIRGE_TICKS] );
		send_to_char( buf, ch );
	}
	return;
}

/*
 * Warcry - basic sonic damage attack. Builds resonance.
 */
void do_warcry( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;
	int resonance_cap;

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_WARCHANTS] < 1 ) {
		send_to_char( "You haven't trained War Chants yet. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_WARCRY_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, cfg( CFG_ABILITY_DIRGESINGER_WARCRY_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_WARCRY_MANA_COST );

	dam = number_range( cfg( CFG_ABILITY_DIRGESINGER_WARCRY_DAM_BONUS_MIN ), cfg( CFG_ABILITY_DIRGESINGER_WARCRY_DAM_BONUS_MAX ) );
	dam += ch->rage * 2;

	act( "You unleash a thunderous warcry at $N!", ch, NULL, victim, TO_CHAR );
	act( "$n unleashes a thunderous warcry at you!", ch, NULL, victim, TO_VICT );
	act( "$n unleashes a thunderous warcry at $N!", ch, NULL, victim, TO_NOTVICT );

	/* Build resonance */
	resonance_cap = IS_CLASS( ch, CLASS_SIREN ) ? 150 : 100;
	ch->rage = UMIN( ch->rage + cfg( CFG_ABILITY_DIRGESINGER_WARCRY_RESONANCE_GAIN ), resonance_cap );

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Shatter - heavy single-target sonic burst + disarm chance.
 * Never destroys equipment.
 */
void do_shatter( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int dam;

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_WARCHANTS] < 2 ) {
		send_to_char( "You need War Chants level 2 to use shatter. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_SHATTER_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_DIRGESINGER_SHATTER_RESONANCE_REQ ) ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),"You need at least %d resonance to use shatter.\n\r", cfg( CFG_ABILITY_DIRGESINGER_SHATTER_RESONANCE_REQ ) );
		send_to_char( buf, ch );
		return;
	}
	WAIT_STATE( ch, cfg( CFG_ABILITY_DIRGESINGER_SHATTER_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_SHATTER_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_DIRGESINGER_SHATTER_RESONANCE_COST );
	if ( ch->rage < 0 ) ch->rage = 0;

	dam = number_range( 200, 400 ) + ch->rage * 3;

	act( "You release a devastating sonic burst at $N!", ch, NULL, victim, TO_CHAR );
	act( "$n releases a devastating sonic burst at you!", ch, NULL, victim, TO_VICT );
	act( "$n releases a devastating sonic burst at $N!", ch, NULL, victim, TO_NOTVICT );

	damage( ch, victim, dam, gsn_punch );

	/* Disarm chance - knock weapon to ground, never destroy */
	if ( number_percent() < cfg( CFG_ABILITY_DIRGESINGER_SHATTER_DISARM_CHANCE ) ) {
		if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) != NULL ) {
			act( "The sonic blast knocks $p from $N's grasp!", ch, obj, victim, TO_CHAR );
			act( "The sonic blast knocks $p from your grasp!", ch, obj, victim, TO_VICT );
			obj_from_char( obj );
			obj_to_room( obj, victim->in_room );
		}
	}
	return;
}

/*
 * Battlehymn - self-buff adding sonic damage to melee attacks
 */
void do_battlehymn( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_BATTLESONGS] < 1 ) {
		send_to_char( "You need Battle Songs level 1 to use battlehymn. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] > 0 ) {
		send_to_char( "Your battle hymn is already resonating.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_BATTLEHYMN_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_BATTLEHYMN_MANA_COST );
	ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] = cfg( CFG_ABILITY_DIRGESINGER_BATTLEHYMN_DURATION );

	act( "You begin a fierce battle hymn, sonic energy crackling around your fists!", ch, NULL, NULL, TO_CHAR );
	act( "$n begins a fierce battle hymn, sonic energy crackling around $s fists!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Dirge - stacking DOT that progressively weakens target
 */
void do_dirge( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int resonance_cap;

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_DIRGES] < 1 ) {
		send_to_char( "You need Dirges level 1 to use dirge. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_DIRGE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_DIRGE_STACKS] >= cfg( CFG_ABILITY_DIRGESINGER_DIRGE_MAX_STACKS ) ) {
		send_to_char( "Your dirge has reached maximum intensity.\n\r", ch );
		return;
	}
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_DIRGE_MANA_COST );
	ch->pcdata->powers[DIRGE_DIRGE_STACKS]++;
	ch->pcdata->powers[DIRGE_DIRGE_TICKS] = cfg( CFG_ABILITY_DIRGESINGER_DIRGE_DURATION );

	/* Build resonance */
	resonance_cap = IS_CLASS( ch, CLASS_SIREN ) ? 150 : 100;
	ch->rage = UMIN( ch->rage + 3, resonance_cap );

	if ( ch->pcdata->powers[DIRGE_DIRGE_STACKS] == 1 ) {
		act( "You begin a mournful dirge, sapping $N's strength!", ch, NULL, victim, TO_CHAR );
		act( "$n begins a mournful dirge, and you feel your strength waning!", ch, NULL, victim, TO_VICT );
	} else {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),"Your dirge intensifies to %d stacks!\n\r", ch->pcdata->powers[DIRGE_DIRGE_STACKS] );
		send_to_char( buf, ch );
	}
	return;
}

/*
 * Thunderclap - AoE sonic blast hitting all enemies in room
 */
void do_thunderclap( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_WARCHANTS] < 3 ) {
		send_to_char( "You need War Chants level 3 to use thunderclap. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_THUNDERCLAP_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_DIRGESINGER_THUNDERCLAP_RESONANCE_REQ ) ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),"You need at least %d resonance to use thunderclap.\n\r", cfg( CFG_ABILITY_DIRGESINGER_THUNDERCLAP_RESONANCE_REQ ) );
		send_to_char( buf, ch );
		return;
	}
	WAIT_STATE( ch, cfg( CFG_ABILITY_DIRGESINGER_THUNDERCLAP_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_THUNDERCLAP_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_DIRGESINGER_THUNDERCLAP_RESONANCE_COST );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You clap your hands together, unleashing a devastating sonic shockwave!", ch, NULL, NULL, TO_CHAR );
	act( "$n claps $s hands together, unleashing a devastating sonic shockwave!", ch, NULL, NULL, TO_ROOM );

	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		dam = number_range( 150, 300 ) + ch->rage * 2;
		damage( ch, vch, dam, gsn_punch );
	}
	return;
}

/*
 * Ironsong - defensive chant creating a sonic barrier
 */
void do_ironsong( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_IRONVOICE] < 1 ) {
		send_to_char( "You need Iron Voice level 1 to use ironsong. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE] > 0 ) {
		send_to_char( "Your ironsong barrier is already active.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_IRONSONG_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_IRONSONG_MANA_COST );
	ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE] = cfg( CFG_ABILITY_DIRGESINGER_IRONSONG_DURATION );
	ch->pcdata->stats[DIRGE_ARMOR_BONUS] = cfg( CFG_ABILITY_DIRGESINGER_IRONSONG_ABSORB_AMOUNT );

	act( "You weave a protective song of iron, a shimmering sonic barrier surrounding you!", ch, NULL, NULL, TO_CHAR );
	act( "$n weaves a protective song, a shimmering sonic barrier surrounding $m!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Cadence - haste/movement buff granting extra attacks
 */
void do_cadence( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_BATTLESONGS] < 2 ) {
		send_to_char( "You need Battle Songs level 2 to use cadence. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] > 0 ) {
		send_to_char( "You are already moving to an accelerated cadence.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_CADENCE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_CADENCE_MANA_COST );
	ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] = cfg( CFG_ABILITY_DIRGESINGER_CADENCE_DURATION );

	act( "You begin a rapid cadence, your movements blurring with speed!", ch, NULL, NULL, TO_CHAR );
	act( "$n begins a rapid cadence, $s movements blurring with speed!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Dissonance - debuff reducing enemy attack effectiveness
 */
void do_dissonance( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_DIRGES] < 2 ) {
		send_to_char( "You need Dirges level 2 to use dissonance. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_DISSONANCE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, cfg( CFG_ABILITY_DIRGESINGER_DISSONANCE_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_DISSONANCE_MANA_COST );
	ch->pcdata->powers[DIRGE_DISSONANCE_TICKS] = cfg( CFG_ABILITY_DIRGESINGER_DISSONANCE_DURATION );

	act( "You weave a discordant melody, scrambling $N's concentration!", ch, NULL, victim, TO_CHAR );
	act( "$n weaves a discordant melody, scrambling your concentration!", ch, NULL, victim, TO_VICT );
	act( "$n weaves a discordant melody at $N!", ch, NULL, victim, TO_NOTVICT );
	return;
}

/*
 * Rally - group heal/buff through inspiring war-song
 */
void do_rally( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *gch;
	char buf[MAX_STRING_LENGTH];
	int heal;

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_IRONVOICE] < 2 ) {
		send_to_char( "You need Iron Voice level 2 to use rally. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_RALLY_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_DIRGESINGER_RALLY_RESONANCE_REQ ) ) {
		snprintf( buf, sizeof( buf ),"You need at least %d resonance to rally your allies.\n\r", cfg( CFG_ABILITY_DIRGESINGER_RALLY_RESONANCE_REQ ) );
		send_to_char( buf, ch );
		return;
	}
	ch->mana -= cfg( CFG_ABILITY_DIRGESINGER_RALLY_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_DIRGESINGER_RALLY_RESONANCE_COST );
	if ( ch->rage < 0 ) ch->rage = 0;

	heal = cfg( CFG_ABILITY_DIRGESINGER_RALLY_HEAL_AMOUNT );

	act( "You raise your voice in an inspiring rally, healing wounds around you!", ch, NULL, NULL, TO_CHAR );
	act( "$n raises $s voice in an inspiring rally!", ch, NULL, NULL, TO_ROOM );

	LIST_FOR_EACH(gch, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( !is_same_group( gch, ch ) ) continue;
		gch->hit = UMIN( gch->hit + heal, gch->max_hit );
		send_to_char( "You feel invigorated by the rallying cry!\n\r", gch );
	}
	return;
}

/*
 * Warsong - toggle aura giving combat bonuses to bard and group
 */
void do_warsong( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_TRAIN_BATTLESONGS] < 3 ) {
		send_to_char( "You need Battle Songs level 3 to use warsong. See #Rsongtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] ) {
		ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] = 0;
		act( "Your warsong fades into silence.", ch, NULL, NULL, TO_CHAR );
		act( "$n's warsong fades into silence.", ch, NULL, NULL, TO_ROOM );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_DIRGESINGER_WARSONG_MANA_DRAIN_PER_TICK ) * 2 ) {
		send_to_char( "You don't have enough mana to sustain a warsong.\n\r", ch );
		return;
	}
	ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] = 1;
	act( "You begin a powerful warsong, empowering yourself and your allies!", ch, NULL, NULL, TO_CHAR );
	act( "$n begins a powerful warsong, the air thrumming with energy!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Song Training - display and improve training category levels
 */
void do_songtrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int *path = NULL;
	int max_level = 0;
	int cost;
	const char *path_name = NULL;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "#x136~#x178[#n Song Training #x178]#x136~#n\n\r", ch );
		snprintf( buf, sizeof( buf ),"  #x178War Chants#n    [%d/3]  Offensive sonic attacks\n\r", ch->pcdata->powers[DIRGE_TRAIN_WARCHANTS] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"  #x178Battle Songs#n  [%d/3]  Combat buffs\n\r", ch->pcdata->powers[DIRGE_TRAIN_BATTLESONGS] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"  #x178Dirges#n        [%d/2]  Debuffs and DOTs\n\r", ch->pcdata->powers[DIRGE_TRAIN_DIRGES] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"  #x178Iron Voice#n    [%d/2]  Defense and support\n\r", ch->pcdata->powers[DIRGE_TRAIN_IRONVOICE] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: songtrain <warchants|battlesongs|dirges|ironvoice>\n\r", ch );

		send_to_char( "\n\r#x178War Chants:#n   1:warcry  2:shatter  3:thunderclap\n\r", ch );
		send_to_char( "#x178Battle Songs:#n 1:battlehymn  2:cadence  3:warsong\n\r", ch );
		send_to_char( "#x178Dirges:#n       1:dirge  2:dissonance\n\r", ch );
		send_to_char( "#x178Iron Voice:#n   1:ironsong  2:rally\n\r", ch );
		return;
	}

	if ( !str_cmp( arg, "warchants" ) ) {
		path = &ch->pcdata->powers[DIRGE_TRAIN_WARCHANTS];
		max_level = 3;
		path_name = "War Chants";
	} else if ( !str_cmp( arg, "battlesongs" ) ) {
		path = &ch->pcdata->powers[DIRGE_TRAIN_BATTLESONGS];
		max_level = 3;
		path_name = "Battle Songs";
	} else if ( !str_cmp( arg, "dirges" ) ) {
		path = &ch->pcdata->powers[DIRGE_TRAIN_DIRGES];
		max_level = 2;
		path_name = "Dirges";
	} else if ( !str_cmp( arg, "ironvoice" ) ) {
		path = &ch->pcdata->powers[DIRGE_TRAIN_IRONVOICE];
		max_level = 2;
		path_name = "Iron Voice";
	} else {
		send_to_char( "Valid paths: warchants, battlesongs, dirges, ironvoice\n\r", ch );
		return;
	}

	if ( *path >= max_level ) {
		snprintf( buf, sizeof( buf ),"Your %s training is already at maximum (%d).\n\r", path_name, max_level );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *path + 1 ) * 40;

	if ( ch->practice < cost ) {
		snprintf( buf, sizeof( buf ),"You need %d primal to advance %s to level %d.\n\r", cost, path_name, *path + 1 );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	(*path)++;

	snprintf( buf, sizeof( buf ),"You advance your #x178%s#n training to level %d!\n\r", path_name, *path );
	send_to_char( buf, ch );

	if ( !str_cmp( arg, "warchants" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #Rwarcry#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #Rshatter#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #Rthunderclap#n!\n\r", ch );
	} else if ( !str_cmp( arg, "battlesongs" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #Rbattlehymn#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #Rcadence#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #Rwarsong#n!\n\r", ch );
	} else if ( !str_cmp( arg, "dirges" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #Rdirge#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #Rdissonance#n!\n\r", ch );
	} else if ( !str_cmp( arg, "ironvoice" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #Rironsong#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #Rrally#n!\n\r", ch );
	}
	return;
}

/*
 * Dirgesinger armor - creates class-specific equipment
 */
void do_dirgesingerarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_DIRGESINGER );
}

/*
 * Tick update for Dirgesinger - called from update.c
 */
void update_dirgesinger( CHAR_DATA *ch ) {
	/* Resonance build/decay */
	if ( ch->position == POS_FIGHTING ) {
		if ( ch->rage < 100 )
			ch->rage = UMIN( ch->rage + 2, 100 );
	} else if ( ch->rage > 0 ) {
		ch->rage -= 3;
		if ( ch->rage < 0 ) ch->rage = 0;
	}

	/* Track peak resonance */
	if ( ch->rage > ch->pcdata->stats[DIRGE_RESONANCE_PEAK] )
		ch->pcdata->stats[DIRGE_RESONANCE_PEAK] = ch->rage;

	/* Dirge DOT processing */
	if ( ch->pcdata->powers[DIRGE_DIRGE_TICKS] > 0 ) {
		if ( ch->fighting != NULL ) {
			int dot_dam = cfg( CFG_ABILITY_DIRGESINGER_DIRGE_TICK_DAMAGE ) * ch->pcdata->powers[DIRGE_DIRGE_STACKS];
			damage( ch, ch->fighting, dot_dam, gsn_punch );
		}
		ch->pcdata->powers[DIRGE_DIRGE_TICKS]--;
		if ( ch->pcdata->powers[DIRGE_DIRGE_TICKS] <= 0 ) {
			ch->pcdata->powers[DIRGE_DIRGE_STACKS] = 0;
			send_to_char( "Your dirge fades away.\n\r", ch );
		}
	}

	/* Buff duration countdowns */
	if ( ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] > 0 ) {
		ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE]--;
		if ( ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] <= 0 )
			send_to_char( "Your battle hymn fades.\n\r", ch );
	}
	if ( ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE] > 0 ) {
		ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE]--;
		if ( ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE] <= 0 ) {
			ch->pcdata->stats[DIRGE_ARMOR_BONUS] = 0;
			send_to_char( "Your ironsong barrier dissipates.\n\r", ch );
		}
	}
	if ( ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] > 0 ) {
		ch->pcdata->powers[DIRGE_CADENCE_ACTIVE]--;
		if ( ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] <= 0 )
			send_to_char( "Your accelerated cadence slows to normal.\n\r", ch );
	}
	if ( ch->pcdata->powers[DIRGE_DISSONANCE_TICKS] > 0 )
		ch->pcdata->powers[DIRGE_DISSONANCE_TICKS]--;

	/* Warsong sustained mana drain */
	if ( ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] == 1 ) {
		int drain = cfg( CFG_ABILITY_DIRGESINGER_WARSONG_MANA_DRAIN_PER_TICK );
		if ( ch->mana >= drain )
			ch->mana -= drain;
		else {
			ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] = 0;
			send_to_char( "Your warsong fades as your mana is depleted.\n\r", ch );
		}
	}
}
