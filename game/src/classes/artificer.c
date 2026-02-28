/***************************************************************************
 *  Artificer class - technology-focused engineer with turrets and gadgets *
 *  Uses the "Power Cells" resource (ch->rage) that builds during combat.  *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"
#include "artificer.h"
#include "../db/db_class.h"

/* Forward declarations for class armor */
extern void do_classarmor_generic( CHAR_DATA *ch, char *argument, int class_id );

/*
 * Power cell status display - shared by Artificer and Mechanist
 */
void do_power( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int power_cap;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) && !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	power_cap = IS_CLASS( ch, CLASS_MECHANIST ) ? MECH_POWER_CAP : ART_POWER_CAP;
	if ( IS_CLASS( ch, CLASS_ARTIFICER ) && ch->pcdata->powers[ART_OVERCHARGE] > 0 )
		power_cap = ART_POWER_CAP_OVERCHARGE;

	snprintf( buf, sizeof( buf ),"%s[%s=#n Power Cell Status %s=%s]#n\n\r", ac, pc, pc, ac );
	send_to_char( buf, ch );
	snprintf( buf, sizeof( buf ),"Current Power: %s%d#n / %d\n\r", pc, ch->rage, power_cap );
	send_to_char( buf, ch );

	if ( IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		if ( ch->pcdata->powers[ART_OVERCHARGE] > 0 ) {
			snprintf( buf, sizeof( buf ),"#x208[#nOVERCHARGE: #C%d#n ticks remaining#x208]#n\n\r",
				ch->pcdata->powers[ART_OVERCHARGE] );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[ART_TURRET_COUNT] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nTurrets active: #C%d#n%s]#n\n\r",
				pc, ch->pcdata->powers[ART_TURRET_COUNT], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[ART_FORCEFIELD] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nForce field: #C%d#n HP, #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->stats[ART_STAT_FORCEFIELD_HP],
				ch->pcdata->powers[ART_FORCEFIELD], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[ART_ENERGYBLADE] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nEnergy blade: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[ART_ENERGYBLADE], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[ART_REPAIRBOT] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nRepair bot: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[ART_REPAIRBOT], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[ART_CLOAK] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nCloak: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[ART_CLOAK], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[ART_DECOY] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nDecoy: #C%d#n HP, #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->stats[ART_STAT_DECOY_HP],
				ch->pcdata->powers[ART_DECOY], pc );
			send_to_char( buf, ch );
		}
	}

	if ( IS_CLASS( ch, CLASS_MECHANIST ) ) {
		if ( ch->pcdata->powers[MECH_NEURAL_JACK] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nNeural Jack: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[MECH_NEURAL_JACK], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[MECH_SERVO_ARMS] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nServo Arms: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[MECH_SERVO_ARMS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[MECH_REACTIVE_PLATING] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nReactive Plating: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[MECH_REACTIVE_PLATING], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[MECH_DRONE_COUNT] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nCombat Drones: #C%d#n / %d%s]#n\n\r",
				pc, ch->pcdata->powers[MECH_DRONE_COUNT], MECH_MAX_DRONES, pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[MECH_BOMBER_ACTIVE] ) {
			snprintf( buf, sizeof( buf ),"%s[#nBomber drone deployed%s]#n\n\r", ac, ac );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[MECH_REPAIR_SWARM] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nRepair Swarm: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[MECH_REPAIR_SWARM], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[MECH_DRONE_ARMY] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nDrone Army: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[MECH_DRONE_ARMY], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[MECH_ORBITAL_CHARGE] > 0 ) {
			snprintf( buf, sizeof( buf ),"%s[#nORBITAL STRIKE CHARGING: round %d/3%s]#n\n\r",
				ac, ch->pcdata->powers[MECH_ORBITAL_CHARGE], ac );
			send_to_char( buf, ch );
		}
		/* Implant summary */
		if ( ch->pcdata->powers[MECH_NEURAL_IMPLANT] != IMPLANT_NEURAL_NONE ||
			ch->pcdata->powers[MECH_SERVO_IMPLANT] != IMPLANT_SERVO_NONE ||
			ch->pcdata->powers[MECH_CORE_IMPLANT] != IMPLANT_CORE_NONE ) {
			snprintf( buf, sizeof( buf ),"%sImplants:#n", pc );
			send_to_char( buf, ch );
			if ( ch->pcdata->powers[MECH_NEURAL_IMPLANT] == IMPLANT_NEURAL_COMBAT_PROC )
				send_to_char( " Neural:Combat", ch );
			else if ( ch->pcdata->powers[MECH_NEURAL_IMPLANT] == IMPLANT_NEURAL_TARGETING )
				send_to_char( " Neural:Targeting", ch );
			else if ( ch->pcdata->powers[MECH_NEURAL_IMPLANT] == IMPLANT_NEURAL_THREAT )
				send_to_char( " Neural:Threat", ch );
			if ( ch->pcdata->powers[MECH_SERVO_IMPLANT] == IMPLANT_SERVO_POWER_ARMS )
				send_to_char( " Servo:Power", ch );
			else if ( ch->pcdata->powers[MECH_SERVO_IMPLANT] == IMPLANT_SERVO_MULTI_TOOL )
				send_to_char( " Servo:Multi", ch );
			else if ( ch->pcdata->powers[MECH_SERVO_IMPLANT] == IMPLANT_SERVO_SHIELD_GEN )
				send_to_char( " Servo:Shield", ch );
			if ( ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_ARMORED )
				send_to_char( " Core:Armored", ch );
			else if ( ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_REGENERATOR )
				send_to_char( " Core:Regen", ch );
			else if ( ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_POWER )
				send_to_char( " Core:Power", ch );
			send_to_char( "\n\r", ch );
		}
	}
	return;
}

/*
 * Charge - out of combat power cell generation
 */
void do_powercharge( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int power_cap;
	int gain;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->fighting != NULL ) {
		send_to_char( "You can't manually charge while fighting - combat generates power automatically.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_CHARGE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana to charge your power cells.\n\r", ch );
		return;
	}

	power_cap = ( ch->pcdata->powers[ART_OVERCHARGE] > 0 ) ?
		ART_POWER_CAP_OVERCHARGE : ART_POWER_CAP;

	if ( ch->rage >= power_cap ) {
		send_to_char( "Your power cells are already at maximum charge.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, cfg( CFG_ABILITY_ARTIFICER_CHARGE_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_CHARGE_MANA_COST );

	gain = cfg( CFG_ABILITY_ARTIFICER_CHARGE_POWER_GAIN );
	ch->rage = UMIN( ch->rage + gain, power_cap );

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ),"You channel mana into your power cells, gaining %s%d#n power. [%d/%d]\n\r",
			pc, gain, ch->rage, power_cap );
	}
	send_to_char( buf, ch );
	act( "$n's equipment hums as energy flows through it.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Overcharge - temporarily exceed power cell limits, boosts damcap
 */
void do_overcharge( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_OVERCHARGE] > 0 ) {
		send_to_char( "Your systems are already overcharged!\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_OVERCHARGE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana to overcharge.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_OVERCHARGE_POWER_COST ) ) {
		snprintf( buf, sizeof( buf ),"You need at least %d power cells to overcharge.\n\r",
			cfg( CFG_ABILITY_ARTIFICER_OVERCHARGE_POWER_COST ) );
		send_to_char( buf, ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_OVERCHARGE_MANA_COST );
	ch->pcdata->powers[ART_OVERCHARGE] = cfg( CFG_ABILITY_ARTIFICER_OVERCHARGE_DURATION );

	send_to_char( "#x208WARNING:#n Systems overcharged! Power cap increased to 120, +damcap bonus active.\n\r", ch );
	send_to_char( "Power cells will decay faster while overcharged.\n\r", ch );
	act( "$n's equipment sparks and hums dangerously as systems overcharge!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Turret - deploy an automated stationary turret
 * Turrets attack the Artificer's fighting target each combat round.
 * Despawn when the Artificer leaves the room.
 * Max 2 active turrets (ART_MAX_TURRETS).
 */
void do_turret( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *turret;
	MOB_INDEX_DATA *pMobIndex;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_GADGET] < 1 ) {
		send_to_char( "You haven't trained Gadgetry yet. See #Rtechtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TURRET_COUNT] >= ART_MAX_TURRETS ) {
		snprintf( buf, sizeof( buf ), "You already have %d turrets deployed (max %d).\n\r",
			ch->pcdata->powers[ART_TURRET_COUNT], ART_MAX_TURRETS );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_TURRET_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_TURRET_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	if ( ( pMobIndex = get_mob_index( VNUM_ART_TURRET ) ) == NULL ) {
		send_to_char( "Turret template not found. Please contact an immortal.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_TURRET_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_TURRET_POWER_COST );

	turret = create_mobile( pMobIndex );

	/* Set turret stats */
	turret->level = ch->level;
	turret->hit = cfg( CFG_ABILITY_ARTIFICER_TURRET_HP );
	turret->max_hit = cfg( CFG_ABILITY_ARTIFICER_TURRET_HP );
	turret->hitroll = 50;
	turret->damroll = 50;
	turret->armor = -200;

	/* Customize appearance */
	snprintf( buf, sizeof( buf ), "%s's auto-turret", ch->name );
	free(turret->short_descr);
	turret->short_descr = str_dup( buf );
	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ), "%sA mechanical auto-turret belonging to %s hums with energy here.#n\n\r", pc, ch->name );
	}
	free(turret->long_descr);
	turret->long_descr = str_dup( buf );
	free(turret->name);
	turret->name = str_dup( "turret auto-turret" );

	/* Set turret flags */
	SET_BIT( turret->act, ACT_SENTINEL );
	SET_BIT( turret->act, ACT_NOEXP );
	turret->spec_fun = NULL;

	/* Link to owner */
	free(turret->lord);
	turret->lord = str_dup( ch->name );
	turret->wizard = ch;

	/* Place in room and set up follower relationship */
	char_to_room( turret, ch->in_room );
	add_follower( turret, ch );

	af.type = skill_lookup( "charm person" );
	af.duration = 666;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char( turret, &af );

	ch->pcdata->powers[ART_TURRET_COUNT]++;

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ),"%sYou deploy an auto-turret, which unfolds with mechanical precision!#n\n\r", pc );
		send_to_char( buf, ch );
	}
	act( "$n deploys an auto-turret, which unfolds with mechanical precision!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Decoy - holographic distraction that absorbs some incoming damage
 * 40% of incoming attacks hit the decoy instead of the Artificer.
 * Decoy has its own HP pool; destroyed when depleted or duration expires.
 */
void do_decoy( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int hp;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_GADGET] < 2 ) {
		send_to_char( "You need Gadgetry level 2. See #Rtechtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_DECOY] > 0 ) {
		snprintf( buf, sizeof( buf ), "Your decoy is already active (%d HP, %d ticks remaining).\n\r",
			ch->pcdata->stats[ART_STAT_DECOY_HP],
			ch->pcdata->powers[ART_DECOY] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_DECOY_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_DECOY_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_DECOY_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_DECOY_POWER_COST );

	hp = cfg( CFG_ABILITY_ARTIFICER_DECOY_HP );
	ch->pcdata->powers[ART_DECOY] = cfg( CFG_ABILITY_ARTIFICER_DECOY_DURATION );
	ch->pcdata->stats[ART_STAT_DECOY_HP] = hp;

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ),
			"%sYou project a holographic decoy! [%d HP, %d%% redirect]#n\n\r",
			pc, hp, cfg( CFG_ABILITY_ARTIFICER_DECOY_REDIRECT_PCT ) );
	}
	send_to_char( buf, ch );
	act( "A shimmering holographic copy of $n flickers into existence!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Grapple - dual-purpose gadget
 * Combat mode: grapple <target> - yank and damage, prevent flee, chance to disarm
 * Movement mode: grapple <direction> - rapid traversal to adjacent room
 */
void do_grapple( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	EXIT_DATA *pexit;
	AFFECT_DATA af;
	char arg[MAX_INPUT_LENGTH];
	int dam;
	int door = -1;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_GADGET] < 3 ) {
		send_to_char( "You need Gadgetry level 3. See #Rtechtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_GRAPPLE_CD] > 0 ) {
		send_to_char( "Your grapple hook is retracting. Wait for cooldown.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		char buf[MAX_STRING_LENGTH];
		send_to_char( "Syntax: grapple <target>     (yank and damage)\n\r", ch );
		snprintf( buf, sizeof(buf),
			"        grapple <direction>  (zip up to %d rooms, bypasses webs)\n\r",
			cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_RANGE ) );
		send_to_char( buf, ch );
		return;
	}

	/* Check if arg is a direction (movement mode) */
	if      ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = DIR_NORTH;
	else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )  door = DIR_EAST;
	else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = DIR_SOUTH;
	else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )  door = DIR_WEST;
	else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )    door = DIR_UP;
	else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )  door = DIR_DOWN;

	/* Movement mode - multi-room grapple traversal */
	if ( door >= 0 ) {
		ROOM_INDEX_DATA *dest_room;
		int range, rooms_traveled;
		char buf[MAX_STRING_LENGTH];

		if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_MANA_COST ) ) {
			send_to_char( "You don't have enough mana.\n\r", ch );
			return;
		}
		if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_POWER_COST ) ) {
			send_to_char( "You don't have enough power cells.\n\r", ch );
			return;
		}

		/* Full range at Gadgetry 3 */
		range = cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_RANGE );

		/* Walk the exit chain - bypasses walls, stops at closed doors and dead ends */
		dest_room = ch->in_room;
		for ( rooms_traveled = 0; rooms_traveled < range; rooms_traveled++ ) {
			pexit = dest_room->exit[door];
			if ( pexit == NULL || pexit->to_room == NULL )
				break;
			if ( IS_SET( pexit->exit_info, EX_CLOSED ) )
				break;
			dest_room = pexit->to_room;
		}

		if ( rooms_traveled == 0 ) {
			send_to_char( "Your grapple hook finds nothing to latch onto.\n\r", ch );
			return;
		}

		/* Pay costs */
		ch->mana -= cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_MANA_COST );
		ch->rage -= cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_POWER_COST );
		ch->pcdata->powers[ART_GRAPPLE_CD] = cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_COOLDOWN );

		/* Escape combat if fighting */
		if ( ch->fighting != NULL ) {
			act( "You fire your grapple hook and wrench free from combat!", ch, NULL, NULL, TO_CHAR );
			act( "$n fires a grapple hook and wrenches free from combat!", ch, NULL, NULL, TO_ROOM );
			stop_fighting( ch, TRUE );
		} else {
			act( "You fire your grapple hook and zip away!", ch, NULL, NULL, TO_CHAR );
			act( "$n fires a grapple hook and zips away!", ch, NULL, NULL, TO_ROOM );
		}

		/* Break free of webbing */
		if ( IS_AFFECTED( ch, AFF_WEBBED ) ) {
			REMOVE_BIT( ch->affected_by, AFF_WEBBED );
			send_to_char( "Your grapple tears through the webbing!\n\r", ch );
		}

		/* Despawn turrets before leaving room */
		if ( ch->pcdata->powers[ART_TURRET_COUNT] > 0 )
			artificer_despawn_turrets( ch );

		/* Teleport directly to destination - skips intermediate rooms (no mob aggro) */
		char_from_room( ch );
		char_to_room( ch, dest_room );

		if ( rooms_traveled > 1 ) {
			snprintf( buf, sizeof(buf), "Your grapple hook carries you %d rooms %s.\n\r",
				rooms_traveled, dir_name[door] );
			send_to_char( buf, ch );
		}

		act( "$n arrives in a blur, retracting a grapple hook.", ch, NULL, NULL, TO_ROOM );
		do_look( ch, "auto" );
		return;
	}

	/* Combat mode: grapple <target> */
	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}
	if ( victim == ch ) {
		send_to_char( "You can't grapple yourself!\n\r", ch );
		return;
	}
	if ( is_safe( ch, victim ) == TRUE ) return;

	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_POWER_COST );
	ch->pcdata->powers[ART_GRAPPLE_CD] = cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_COOLDOWN );

	dam = number_range( cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_DAM_MIN ),
		cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_DAM_MAX ) );

	act( "You fire your grapple hook at $N, yanking them violently!", ch, NULL, victim, TO_CHAR );
	act( "$n fires a grapple hook at you, yanking you violently!", ch, NULL, victim, TO_VICT );
	act( "$n fires a grapple hook at $N, yanking them violently!", ch, NULL, victim, TO_NOTVICT );

	/* Prevent flee for 2 ticks */
	if ( !IS_AFFECTED( victim, AFF_WEBBED ) ) {
		af.type = 0;
		af.duration = 2;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_WEBBED;
		affect_to_char( victim, &af );
		send_to_char( "The grapple hook tangles around you, preventing escape!\n\r", victim );
	}

	/* 20% chance to disarm */
	if ( number_percent() <= cfg( CFG_ABILITY_ARTIFICER_GRAPPLE_DISARM_PCT ) ) {
		disarm( ch, victim );
	}

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Energy Blade - summon an energy melee weapon (timed buff, boosts damcap + damage)
 */
void do_energyblade( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_WEAPON] < 1 ) {
		send_to_char( "You haven't trained Weaponsmithing yet. See #Rtechtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_ENERGYBLADE] > 0 ) {
		snprintf( buf, sizeof( buf ),"Your energy blade is already active (%d ticks remaining).\n\r",
			ch->pcdata->powers[ART_ENERGYBLADE] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_ENERGYBLADE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_ENERGYBLADE_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_ENERGYBLADE_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_ENERGYBLADE_POWER_COST );
	ch->pcdata->powers[ART_ENERGYBLADE] = cfg( CFG_ABILITY_ARTIFICER_ENERGYBLADE_DURATION );

	send_to_char( "A crackling blade of pure energy materializes in your hand!\n\r", ch );
	act( "A crackling blade of pure energy materializes in $n's hand!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Blaster - ranged energy attack, scales with power cells
 * Can target someone in the same room or an adjacent room.
 * Syntax: blaster <target> [direction]
 */
void do_blaster( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim = NULL;
	ROOM_INDEX_DATA *target_room = NULL;
	EXIT_DATA *pexit;
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int dam;
	int door = -1;
	bool ranged = FALSE;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_WEAPON] < 2 ) {
		send_to_char( "You need Weaponsmithing level 2. See #Rtechtrain#n.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );
	argument = one_argument( argument, arg2 );

	/* Check for ranged shot: blaster <target> <direction> */
	if ( arg2[0] != '\0' ) {
		if      ( !str_cmp( arg2, "n" ) || !str_cmp( arg2, "north" ) ) door = DIR_NORTH;
		else if ( !str_cmp( arg2, "e" ) || !str_cmp( arg2, "east" ) )  door = DIR_EAST;
		else if ( !str_cmp( arg2, "s" ) || !str_cmp( arg2, "south" ) ) door = DIR_SOUTH;
		else if ( !str_cmp( arg2, "w" ) || !str_cmp( arg2, "west" ) )  door = DIR_WEST;
		else if ( !str_cmp( arg2, "u" ) || !str_cmp( arg2, "up" ) )    door = DIR_UP;
		else if ( !str_cmp( arg2, "d" ) || !str_cmp( arg2, "down" ) )  door = DIR_DOWN;
		else {
			send_to_char( "Invalid direction. Use: north south east west up down\n\r", ch );
			return;
		}

		if ( ( pexit = ch->in_room->exit[door] ) == NULL ||
			( target_room = pexit->to_room ) == NULL ) {
			send_to_char( "You can't fire in that direction.\n\r", ch );
			return;
		}
		if ( IS_SET( pexit->exit_info, EX_CLOSED ) ) {
			send_to_char( "A closed door blocks your shot.\n\r", ch );
			return;
		}

		/* Temporarily move to target room to find victim */
		{
			ROOM_INDEX_DATA *original = ch->in_room;
			char_from_room( ch );
			char_to_room( ch, target_room );
			victim = get_char_room( ch, arg );
			char_from_room( ch );
			char_to_room( ch, original );
		}

		if ( victim == NULL ) {
			send_to_char( "You don't see them in that direction.\n\r", ch );
			return;
		}
		if ( is_safe( ch, victim ) == TRUE ) return;
		ranged = TRUE;
	}
	else if ( arg[0] != '\0' ) {
		/* Same room target */
		if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
			send_to_char( "They aren't here.\n\r", ch );
			return;
		}
		if ( victim == ch ) {
			send_to_char( "You can't blast yourself!\n\r", ch );
			return;
		}
		if ( is_safe( ch, victim ) == TRUE ) return;
	} else if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "Blast whom? Syntax: blaster <target> [direction]\n\r", ch );
		return;
	}

	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_BLASTER_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_BLASTER_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, cfg( CFG_ABILITY_ARTIFICER_BLASTER_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_BLASTER_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_BLASTER_POWER_COST );

	dam = number_range( cfg( CFG_ABILITY_ARTIFICER_BLASTER_DAM_MIN ),
		cfg( CFG_ABILITY_ARTIFICER_BLASTER_DAM_MAX ) );
	dam += ch->rage * 2;  /* Scales with remaining power */

	if ( ranged ) {
		/* Messages in attacker's room */
		snprintf( buf, sizeof( buf ),
			"You take aim and fire a searing bolt of energy %s!", dir_name[door] );
		act( buf, ch, NULL, NULL, TO_CHAR );
		snprintf( buf, sizeof( buf ),
			"$n takes aim and fires a searing bolt of energy %s!", dir_name[door] );
		act( buf, ch, NULL, NULL, TO_ROOM );

		/* Messages in victim's room */
		{
			const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
			const char *ac = br ? br->accent_color : "";
			snprintf( buf, sizeof( buf ), "%sA searing bolt of energy streaks in and strikes you!#n\n\r", ac );
			send_to_char( buf, victim );
		}
		act( "A searing bolt of energy streaks in and strikes $n!", victim, NULL, NULL, TO_ROOM );

		/* Ranged damage - no melee combat initiated */
		hurt_person( ch, victim, dam );
	} else {
		act( "You fire a searing bolt of energy at $N!", ch, NULL, victim, TO_CHAR );
		act( "$n fires a searing bolt of energy at you!", ch, NULL, victim, TO_VICT );
		act( "$n fires a searing bolt of energy at $N!", ch, NULL, victim, TO_NOTVICT );

		damage( ch, victim, dam, gsn_punch );
	}
	return;
}

/*
 * Grenade - AoE explosive damage to all enemies in room
 * Can target someone to initiate combat, or hits all current combatants
 */
void do_grenade( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim = NULL;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char arg[MAX_INPUT_LENGTH];
	int dam;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_WEAPON] < 3 ) {
		send_to_char( "You need Weaponsmithing level 3. See #Rtechtrain#n.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] != '\0' ) {
		if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
			send_to_char( "They aren't here.\n\r", ch );
			return;
		}
		if ( victim == ch ) {
			send_to_char( "You can't grenade yourself!\n\r", ch );
			return;
		}
		if ( is_safe( ch, victim ) == TRUE ) return;
	} else if ( ch->fighting == NULL ) {
		send_to_char( "Grenade whom?\n\r", ch );
		return;
	}

	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_GRENADE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_GRENADE_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, cfg( CFG_ABILITY_ARTIFICER_GRENADE_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_GRENADE_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_GRENADE_POWER_COST );

	act( "You hurl an explosive grenade into the fray!", ch, NULL, NULL, TO_CHAR );
	act( "$n hurls an explosive grenade into the fray!", ch, NULL, NULL, TO_ROOM );

	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) ) continue;  /* No PvP splash */
		if ( IS_NPC( vch ) && vch->fighting == NULL && vch != victim ) continue;  /* Hit target + combatants */
		if ( is_safe( ch, vch ) ) continue;

		dam = number_range( cfg( CFG_ABILITY_ARTIFICER_GRENADE_DAM_MIN ),
			cfg( CFG_ABILITY_ARTIFICER_GRENADE_DAM_MAX ) );
		dam += ch->rage;  /* Scales with power */

		act( "The explosion engulfs $N!", ch, NULL, vch, TO_CHAR );
		damage( ch, vch, dam, gsn_punch );
	}
	return;
}

/*
 * Force Field - energy shield that absorbs damage before HP loss
 */
void do_forcefield( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int absorb;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_DEFENSE] < 1 ) {
		send_to_char( "You haven't trained Defensive Tech yet. See #Rtechtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_FORCEFIELD] > 0 ) {
		snprintf( buf, sizeof( buf ),"Your force field is already active (%d HP, %d ticks remaining).\n\r",
			ch->pcdata->stats[ART_STAT_FORCEFIELD_HP],
			ch->pcdata->powers[ART_FORCEFIELD] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_FORCEFIELD_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_FORCEFIELD_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_FORCEFIELD_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_FORCEFIELD_POWER_COST );

	absorb = cfg( CFG_ABILITY_ARTIFICER_FORCEFIELD_ABSORB );
	ch->pcdata->powers[ART_FORCEFIELD] = cfg( CFG_ABILITY_ARTIFICER_FORCEFIELD_DURATION );
	ch->pcdata->stats[ART_STAT_FORCEFIELD_HP] = absorb;

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ),"A shimmering force field surrounds you! [%s%d#n HP absorption]\n\r", pc, absorb );
	}
	send_to_char( buf, ch );
	act( "A shimmering energy field materializes around $n!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Repair Bot - healing drone that also repairs equipment
 */
void do_repairbot( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_DEFENSE] < 2 ) {
		send_to_char( "You need Defensive Tech level 2. See #Rtechtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_REPAIRBOT] > 0 ) {
		snprintf( buf, sizeof( buf ),"Your repair bot is already active (%d ticks remaining).\n\r",
			ch->pcdata->powers[ART_REPAIRBOT] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_REPAIRBOT_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_REPAIRBOT_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_REPAIRBOT_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_REPAIRBOT_POWER_COST );
	ch->pcdata->powers[ART_REPAIRBOT] = cfg( CFG_ABILITY_ARTIFICER_REPAIRBOT_DURATION );

	snprintf( buf, sizeof( buf ),"A small repair bot deploys and begins scanning for damage. [%d HP/tick, %d ticks]\n\r",
		cfg( CFG_ABILITY_ARTIFICER_REPAIRBOT_HEAL_PER_TICK ),
		ch->pcdata->powers[ART_REPAIRBOT] );
	send_to_char( buf, ch );
	act( "A small hovering bot deploys from $n's equipment and begins repairs.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Tech Cloak - technological invisibility with sustained power drain
 */
void do_artcloak( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[ART_TRAIN_DEFENSE] < 3 ) {
		send_to_char( "You need Defensive Tech level 3. See #Rtechtrain#n.\n\r", ch );
		return;
	}

	/* Toggle off */
	if ( ch->pcdata->powers[ART_CLOAK] > 0 ) {
		ch->pcdata->powers[ART_CLOAK] = 0;
		REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
		REMOVE_BIT( ch->affected_by, AFF_SNEAK );
		send_to_char( "You deactivate your cloaking device.\n\r", ch );
		act( "$n shimmers into visibility.", ch, NULL, NULL, TO_ROOM );
		return;
	}

	if ( ch->fighting != NULL ) {
		send_to_char( "You can't activate your cloak while fighting!\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_ARTIFICER_TECHCLOAK_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_ARTIFICER_TECHCLOAK_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_ARTIFICER_TECHCLOAK_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_ARTIFICER_TECHCLOAK_POWER_COST );
	ch->pcdata->powers[ART_CLOAK] = cfg( CFG_ABILITY_ARTIFICER_TECHCLOAK_DURATION );
	SET_BIT( ch->affected_by, AFF_INVISIBLE );
	SET_BIT( ch->affected_by, AFF_SNEAK );

	send_to_char( "Your cloaking device activates - you shimmer and vanish!\n\r", ch );
	{
		char buf[MAX_STRING_LENGTH];
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ),"%sWarning:#n Drains power cells each tick. Deactivated by combat.\n\r", pc );
		send_to_char( buf, ch );
	}
	act( "$n shimmers and vanishes from sight.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Tech Training - improve Artificer skill categories
 */
void do_techtrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int *path;
	int max_level;
	int cost;
	const char *path_name;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ),"%s[%s=#n Artificer Training %s=%s]#n\n\r", ac, pc, pc, ac );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"Gadgetry:      Level %d/3 (turret, decoy, grapple)\n\r",
			ch->pcdata->powers[ART_TRAIN_GADGET] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"Weaponsmithing: Level %d/3 (energyblade, blaster, grenade)\n\r",
			ch->pcdata->powers[ART_TRAIN_WEAPON] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"Defensive Tech: Level %d/3 (forcefield, repairbot, techcloak)\n\r",
			ch->pcdata->powers[ART_TRAIN_DEFENSE] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: techtrain <gadgetry|weaponsmithing|defensive>\n\r", ch );
		return;
	}

	if ( !str_prefix( arg, "gadgetry" ) ) {
		path = &ch->pcdata->powers[ART_TRAIN_GADGET];
		max_level = 3;
		path_name = "Gadgetry";
	} else if ( !str_prefix( arg, "weaponsmithing" ) ) {
		path = &ch->pcdata->powers[ART_TRAIN_WEAPON];
		max_level = 3;
		path_name = "Weaponsmithing";
	} else if ( !str_prefix( arg, "defensive" ) ) {
		path = &ch->pcdata->powers[ART_TRAIN_DEFENSE];
		max_level = 3;
		path_name = "Defensive Tech";
	} else {
		send_to_char( "Valid paths: gadgetry, weaponsmithing, defensive\n\r", ch );
		return;
	}

	if ( *path >= max_level ) {
		snprintf( buf, sizeof( buf ),"Your %s training is already at maximum (%d).\n\r", path_name, max_level );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *path + 1 ) * 40;  /* 40 primal per level for Artificer */

	if ( ch->practice < cost ) {
		snprintf( buf, sizeof( buf ),"You need %d primal to advance %s to level %d.\n\r",
			cost, path_name, *path + 1 );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	(*path)++;

	snprintf( buf, sizeof( buf ),"You advance your %s training to level %d!\n\r", path_name, *path );
	send_to_char( buf, ch );

	/* Notify which abilities unlock */
	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		const char *ability = NULL;

		if ( !str_prefix( arg, "gadgetry" ) ) {
			if ( *path == 1 ) ability = "turret";
			else if ( *path == 2 ) ability = "decoy";
			else if ( *path == 3 ) ability = "grapple";
		} else if ( !str_prefix( arg, "weaponsmithing" ) ) {
			if ( *path == 1 ) ability = "energyblade";
			else if ( *path == 2 ) ability = "blaster";
			else if ( *path == 3 ) ability = "grenade";
		} else if ( !str_prefix( arg, "defensive" ) ) {
			if ( *path == 1 ) ability = "forcefield";
			else if ( *path == 2 ) ability = "repairbot";
			else if ( *path == 3 ) ability = "techcloak";
		}
		if ( ability ) {
			snprintf( buf, sizeof( buf ),"You have learned %s%s#n!\n\r", pc, ability );
			send_to_char( buf, ch );
		}
	}
	return;
}

/*
 * Artificer Armor - create class equipment
 */
void do_artificerarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_ARTIFICER );
}

/*
 * Update function called each tick from update.c
 */
void update_artificer( CHAR_DATA *ch ) {
	int power_cap;

	if ( !IS_CLASS( ch, CLASS_ARTIFICER ) )
		return;

	/* Power cell build (combat) or decay (out of combat) */
	if ( ch->position == POS_FIGHTING ) {
		power_cap = ( ch->pcdata->powers[ART_OVERCHARGE] > 0 ) ?
			ART_POWER_CAP_OVERCHARGE : ART_POWER_CAP;
		if ( ch->rage < power_cap )
			ch->rage = UMIN( ch->rage + 2, power_cap );
	} else if ( ch->rage > 0 && ch->rage < ART_POWER_CAP - 10 ) {
		/* Decay only if not near cap (stable at full charge) */
		ch->rage--;
		if ( ch->pcdata->powers[ART_OVERCHARGE] > 0 )
			ch->rage -= 2;  /* Extra decay while overcharged */
	}

	/* Overcharge duration countdown */
	if ( ch->pcdata->powers[ART_OVERCHARGE] > 0 ) {
		ch->pcdata->powers[ART_OVERCHARGE]--;
		if ( ch->pcdata->powers[ART_OVERCHARGE] <= 0 )
			send_to_char( "Your overcharge mode deactivates.\n\r", ch );
	}

	/* Force field duration countdown */
	if ( ch->pcdata->powers[ART_FORCEFIELD] > 0 ) {
		ch->pcdata->powers[ART_FORCEFIELD]--;
		if ( ch->pcdata->powers[ART_FORCEFIELD] <= 0 ) {
			ch->pcdata->stats[ART_STAT_FORCEFIELD_HP] = 0;
			send_to_char( "Your force field dissipates.\n\r", ch );
		}
	}

	/* Energy blade duration countdown */
	if ( ch->pcdata->powers[ART_ENERGYBLADE] > 0 ) {
		ch->pcdata->powers[ART_ENERGYBLADE]--;
		if ( ch->pcdata->powers[ART_ENERGYBLADE] <= 0 )
			send_to_char( "Your energy blade flickers and vanishes.\n\r", ch );
	}

	/* Repair bot healing + equipment repair */
	if ( ch->pcdata->powers[ART_REPAIRBOT] > 0 ) {
		/* Heal player */
		ch->hit = UMIN( ch->hit + cfg( CFG_ABILITY_ARTIFICER_REPAIRBOT_HEAL_PER_TICK ), ch->max_hit );

		/* Repair one damaged equipment item per tick */
		{
			OBJ_DATA *obj;
			OBJ_DATA *obj_next;
			bool repaired = FALSE;

			LIST_FOR_EACH_SAFE( obj, obj_next, &ch->carrying, OBJ_DATA, content_node ) {
				if ( repaired )
					break;
				if ( obj->condition < 100 && obj->wear_loc != WEAR_NONE ) {
					obj->condition = 100;
					act( "Your repair bot welds $p back together.", ch, obj, NULL, TO_CHAR );
					repaired = TRUE;
				}
			}
		}

		ch->pcdata->powers[ART_REPAIRBOT]--;
		if ( ch->pcdata->powers[ART_REPAIRBOT] <= 0 )
			send_to_char( "Your repair bot powers down.\n\r", ch );
	}

	/* Cloak duration and power drain */
	if ( ch->pcdata->powers[ART_CLOAK] > 0 ) {
		ch->rage -= cfg( CFG_ABILITY_ARTIFICER_TECHCLOAK_POWER_DRAIN );
		if ( ch->rage <= 0 ) {
			ch->rage = 0;
			ch->pcdata->powers[ART_CLOAK] = 0;
			REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
			REMOVE_BIT( ch->affected_by, AFF_SNEAK );
			send_to_char( "Your cloak deactivates - power depleted!\n\r", ch );
		} else {
			ch->pcdata->powers[ART_CLOAK]--;
			if ( ch->pcdata->powers[ART_CLOAK] <= 0 ) {
				REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
				REMOVE_BIT( ch->affected_by, AFF_SNEAK );
				send_to_char( "Your cloak deactivates.\n\r", ch );
			}
		}
	}

	/* Decoy duration countdown */
	if ( ch->pcdata->powers[ART_DECOY] > 0 ) {
		ch->pcdata->powers[ART_DECOY]--;
		if ( ch->pcdata->powers[ART_DECOY] <= 0 ) {
			ch->pcdata->stats[ART_STAT_DECOY_HP] = 0;
			send_to_char( "Your holographic decoy flickers out.\n\r", ch );
		}
	}

	/* Cooldown decrements */
	if ( ch->pcdata->powers[ART_GRAPPLE_CD] > 0 )
		ch->pcdata->powers[ART_GRAPPLE_CD]--;
	if ( ch->pcdata->powers[ART_BLASTER_CD] > 0 )
		ch->pcdata->powers[ART_BLASTER_CD]--;
	if ( ch->pcdata->powers[ART_GRENADE_CD] > 0 )
		ch->pcdata->powers[ART_GRENADE_CD]--;

	/* Track peak power */
	if ( ch->rage > ch->pcdata->stats[ART_STAT_PEAK_POWER] )
		ch->pcdata->stats[ART_STAT_PEAK_POWER] = ch->rage;

	return;
}

/*
 * Helper: Despawn all turrets when player leaves room
 * Called from move_char() in act_move.c before the character moves.
 */
void artificer_despawn_turrets( CHAR_DATA *ch ) {
	CHAR_DATA *turret;
	CHAR_DATA *turret_next;
	bool found = FALSE;

	LIST_FOR_EACH_SAFE( turret, turret_next, &g_characters, CHAR_DATA, char_node ) {

		if ( !IS_NPC( turret ) ) continue;
		if ( turret->pIndexData == NULL ) continue;
		if ( turret->pIndexData->vnum != VNUM_ART_TURRET ) continue;
		if ( turret->wizard != ch ) continue;

		act( "$n collapses and powers down.", turret, NULL, NULL, TO_ROOM );
		extract_char( turret, TRUE );
		found = TRUE;
	}

	if ( found )
		send_to_char( "Your turrets power down as you leave.\n\r", ch );

	ch->pcdata->powers[ART_TURRET_COUNT] = 0;
	return;
}

/*
 * Helper: Process turret attacks during violence update
 * Called from violence_update() in fight.c when the Artificer is fighting.
 * Each turret in the room attacks the Artificer's current target.
 */
void artificer_turret_attacks( CHAR_DATA *ch ) {
	CHAR_DATA *turret;
	CHAR_DATA *victim;
	int dam;

	if ( ( victim = ch->fighting ) == NULL ) return;
	if ( ch->in_room == NULL ) return;

	LIST_FOR_EACH(turret, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( !IS_NPC( turret ) ) continue;
		if ( turret->pIndexData == NULL ) continue;
		if ( turret->pIndexData->vnum != VNUM_ART_TURRET ) continue;
		if ( turret->wizard != ch ) continue;
		if ( !IS_AWAKE( turret ) ) continue;
		if ( turret->in_room != victim->in_room ) continue;

		dam = number_range( cfg( CFG_ABILITY_ARTIFICER_TURRET_DAM_MIN ),
			cfg( CFG_ABILITY_ARTIFICER_TURRET_DAM_MAX ) );

		act( "$n swivels and fires at $N!", turret, NULL, victim, TO_ROOM );
		damage( turret, victim, dam, gsn_punch );

		/* If victim died, stop processing */
		if ( victim->position == POS_DEAD || ch->fighting == NULL )
			break;
	}
	return;
}
