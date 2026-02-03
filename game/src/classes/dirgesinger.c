/***************************************************************************
 *  Dirgesinger class - martial bard with battle chants and sonic damage   *
 *  Uses the "Resonance" resource (ch->rage) that builds during combat.    *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "ability_config.h"
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

	sprintf( buf, "#x136~#x178[#n Resonance Status #x178]#x136~#n\n\r" );
	send_to_char( buf, ch );
	sprintf( buf, "Current Resonance: #x178%d#n / %d\n\r", ch->rage,
		IS_CLASS( ch, CLASS_SIREN ) ? 150 : 100 );
	send_to_char( buf, ch );

	if ( ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] )
		send_to_char( "#x178[#nWarsong active#x178]#n\n\r", ch );
	if ( ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] > 0 ) {
		sprintf( buf, "#x178[#nBattlehymn: #C%d#n ticks remaining#x178]#n\n\r", ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE] > 0 ) {
		sprintf( buf, "#x178[#nIronsong barrier: #C%d#n HP remaining#x178]#n\n\r", ch->pcdata->stats[DIRGE_ARMOR_BONUS] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] > 0 ) {
		sprintf( buf, "#x178[#nCadence: #C%d#n ticks remaining#x178]#n\n\r", ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DIRGE_DIRGE_TICKS] > 0 ) {
		sprintf( buf, "#x178[#nDirge DOT: #C%d#n stacks, #C%d#n ticks remaining#x178]#n\n\r",
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
	if ( ch->mana < acfg( "dirgesinger.warcry.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "dirgesinger.warcry.cooldown" ) );
	ch->mana -= acfg( "dirgesinger.warcry.mana_cost" );

	dam = number_range( acfg( "dirgesinger.warcry.dam_bonus_min" ), acfg( "dirgesinger.warcry.dam_bonus_max" ) );
	dam += ch->rage * 2;

	act( "You unleash a thunderous warcry at $N!", ch, NULL, victim, TO_CHAR );
	act( "$n unleashes a thunderous warcry at you!", ch, NULL, victim, TO_VICT );
	act( "$n unleashes a thunderous warcry at $N!", ch, NULL, victim, TO_NOTVICT );

	/* Build resonance */
	resonance_cap = IS_CLASS( ch, CLASS_SIREN ) ? 150 : 100;
	ch->rage = UMIN( ch->rage + acfg( "dirgesinger.warcry.resonance_gain" ), resonance_cap );

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
	if ( ch->mana < acfg( "dirgesinger.shatter.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "dirgesinger.shatter.resonance_req" ) ) {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "You need at least %d resonance to use shatter.\n\r", acfg( "dirgesinger.shatter.resonance_req" ) );
		send_to_char( buf, ch );
		return;
	}
	WAIT_STATE( ch, acfg( "dirgesinger.shatter.cooldown" ) );
	ch->mana -= acfg( "dirgesinger.shatter.mana_cost" );
	ch->rage -= acfg( "dirgesinger.shatter.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	dam = number_range( 200, 400 ) + ch->rage * 3;

	act( "You release a devastating sonic burst at $N!", ch, NULL, victim, TO_CHAR );
	act( "$n releases a devastating sonic burst at you!", ch, NULL, victim, TO_VICT );
	act( "$n releases a devastating sonic burst at $N!", ch, NULL, victim, TO_NOTVICT );

	damage( ch, victim, dam, gsn_punch );

	/* Disarm chance - knock weapon to ground, never destroy */
	if ( number_percent() < acfg( "dirgesinger.shatter.disarm_chance" ) ) {
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
	if ( ch->mana < acfg( "dirgesinger.battlehymn.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	ch->mana -= acfg( "dirgesinger.battlehymn.mana_cost" );
	ch->pcdata->powers[DIRGE_BATTLEHYMN_ACTIVE] = acfg( "dirgesinger.battlehymn.duration" );

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
	if ( ch->mana < acfg( "dirgesinger.dirge.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_DIRGE_STACKS] >= acfg( "dirgesinger.dirge.max_stacks" ) ) {
		send_to_char( "Your dirge has reached maximum intensity.\n\r", ch );
		return;
	}
	ch->mana -= acfg( "dirgesinger.dirge.mana_cost" );
	ch->pcdata->powers[DIRGE_DIRGE_STACKS]++;
	ch->pcdata->powers[DIRGE_DIRGE_TICKS] = acfg( "dirgesinger.dirge.duration" );

	/* Build resonance */
	resonance_cap = IS_CLASS( ch, CLASS_SIREN ) ? 150 : 100;
	ch->rage = UMIN( ch->rage + 3, resonance_cap );

	if ( ch->pcdata->powers[DIRGE_DIRGE_STACKS] == 1 ) {
		act( "You begin a mournful dirge, sapping $N's strength!", ch, NULL, victim, TO_CHAR );
		act( "$n begins a mournful dirge, and you feel your strength waning!", ch, NULL, victim, TO_VICT );
	} else {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "Your dirge intensifies to %d stacks!\n\r", ch->pcdata->powers[DIRGE_DIRGE_STACKS] );
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
	if ( ch->mana < acfg( "dirgesinger.thunderclap.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "dirgesinger.thunderclap.resonance_req" ) ) {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "You need at least %d resonance to use thunderclap.\n\r", acfg( "dirgesinger.thunderclap.resonance_req" ) );
		send_to_char( buf, ch );
		return;
	}
	WAIT_STATE( ch, acfg( "dirgesinger.thunderclap.cooldown" ) );
	ch->mana -= acfg( "dirgesinger.thunderclap.mana_cost" );
	ch->rage -= acfg( "dirgesinger.thunderclap.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You clap your hands together, unleashing a devastating sonic shockwave!", ch, NULL, NULL, TO_CHAR );
	act( "$n claps $s hands together, unleashing a devastating sonic shockwave!", ch, NULL, NULL, TO_ROOM );

	for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
		vch_next = vch->next_in_room;
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
	if ( ch->mana < acfg( "dirgesinger.ironsong.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	ch->mana -= acfg( "dirgesinger.ironsong.mana_cost" );
	ch->pcdata->powers[DIRGE_IRONSONG_ACTIVE] = acfg( "dirgesinger.ironsong.duration" );
	ch->pcdata->stats[DIRGE_ARMOR_BONUS] = acfg( "dirgesinger.ironsong.absorb_amount" );

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
	if ( ch->mana < acfg( "dirgesinger.cadence.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	ch->mana -= acfg( "dirgesinger.cadence.mana_cost" );
	ch->pcdata->powers[DIRGE_CADENCE_ACTIVE] = acfg( "dirgesinger.cadence.duration" );

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
	if ( ch->mana < acfg( "dirgesinger.dissonance.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "dirgesinger.dissonance.cooldown" ) );
	ch->mana -= acfg( "dirgesinger.dissonance.mana_cost" );
	ch->pcdata->powers[DIRGE_DISSONANCE_TICKS] = acfg( "dirgesinger.dissonance.duration" );

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
	if ( ch->mana < acfg( "dirgesinger.rally.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "dirgesinger.rally.resonance_req" ) ) {
		sprintf( buf, "You need at least %d resonance to rally your allies.\n\r", acfg( "dirgesinger.rally.resonance_req" ) );
		send_to_char( buf, ch );
		return;
	}
	ch->mana -= acfg( "dirgesinger.rally.mana_cost" );
	ch->rage -= acfg( "dirgesinger.rally.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	heal = acfg( "dirgesinger.rally.heal_amount" );

	act( "You raise your voice in an inspiring rally, healing wounds around you!", ch, NULL, NULL, TO_CHAR );
	act( "$n raises $s voice in an inspiring rally!", ch, NULL, NULL, TO_ROOM );

	for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room ) {
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
	if ( ch->mana < acfg( "dirgesinger.warsong.mana_drain_per_tick" ) * 2 ) {
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
		sprintf( buf, "  #x178War Chants#n    [%d/3]  Offensive sonic attacks\n\r", ch->pcdata->powers[DIRGE_TRAIN_WARCHANTS] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x178Battle Songs#n  [%d/3]  Combat buffs\n\r", ch->pcdata->powers[DIRGE_TRAIN_BATTLESONGS] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x178Dirges#n        [%d/2]  Debuffs and DOTs\n\r", ch->pcdata->powers[DIRGE_TRAIN_DIRGES] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x178Iron Voice#n    [%d/2]  Defense and support\n\r", ch->pcdata->powers[DIRGE_TRAIN_IRONVOICE] );
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
		sprintf( buf, "Your %s training is already at maximum (%d).\n\r", path_name, max_level );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *path + 1 ) * 40;

	if ( ch->practice < cost ) {
		sprintf( buf, "You need %d primal to advance %s to level %d.\n\r", cost, path_name, *path + 1 );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	(*path)++;

	sprintf( buf, "You advance your #x178%s#n training to level %d!\n\r", path_name, *path );
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
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;
	char arg[MAX_INPUT_LENGTH];
	int vnum = 0;

	if ( !IS_CLASS( ch, CLASS_DIRGESINGER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Please specify which piece of armor to create.\n\r", ch );
		send_to_char( "Options: warhorn ring collar battleplate warhelm greaves warboots gauntlets vambraces warcape belt bracer warmask\n\r", ch );
		return;
	}

	if ( ch->practice < acfg( "dirgesinger.armor.primal_cost" ) ) {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "You need %d primal to create dirgesinger armor.\n\r", acfg( "dirgesinger.armor.primal_cost" ) );
		send_to_char( buf, ch );
		return;
	}

	if ( !str_cmp( arg, "warhorn" ) )          vnum = 33320;
	else if ( !str_cmp( arg, "ring" ) )        vnum = 33321;
	else if ( !str_cmp( arg, "collar" ) )      vnum = 33322;
	else if ( !str_cmp( arg, "battleplate" ) ) vnum = 33323;
	else if ( !str_cmp( arg, "warhelm" ) )     vnum = 33324;
	else if ( !str_cmp( arg, "greaves" ) )     vnum = 33325;
	else if ( !str_cmp( arg, "warboots" ) )    vnum = 33326;
	else if ( !str_cmp( arg, "gauntlets" ) )   vnum = 33327;
	else if ( !str_cmp( arg, "vambraces" ) )   vnum = 33328;
	else if ( !str_cmp( arg, "warcape" ) )     vnum = 33329;
	else if ( !str_cmp( arg, "belt" ) )        vnum = 33330;
	else if ( !str_cmp( arg, "bracer" ) )      vnum = 33331;
	else if ( !str_cmp( arg, "warmask" ) )     vnum = 33332;
	else { do_dirgesingerarmor( ch, "" ); return; }

	if ( vnum == 0 || ( pObjIndex = get_obj_index( vnum ) ) == NULL ) {
		send_to_char( "Missing object, please inform an immortal.\n\r", ch );
		return;
	}

	ch->practice -= acfg( "dirgesinger.armor.primal_cost" );
	obj = create_object( pObjIndex, 50 );
	obj->questowner = str_dup( ch->pcdata->switchname );
	obj_to_char( obj, ch );

	act( "You shape sonic energy into $p!", ch, obj, NULL, TO_CHAR );
	act( "$n shapes sonic energy into $p!", ch, obj, NULL, TO_ROOM );
	return;
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
			int dot_dam = acfg( "dirgesinger.dirge.tick_damage" ) * ch->pcdata->powers[DIRGE_DIRGE_STACKS];
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
		int drain = acfg( "dirgesinger.warsong.mana_drain_per_tick" );
		if ( ch->mana >= drain )
			ch->mana -= drain;
		else {
			ch->pcdata->powers[DIRGE_WARSONG_ACTIVE] = 0;
			send_to_char( "Your warsong fades as your mana is depleted.\n\r", ch );
		}
	}
}
