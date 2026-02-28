/***************************************************************************
 *  Mechanist class - cyborg warrior with drones and heavy ordnance        *
 *  Upgrade class from Artificer with enhanced Power Cells and implants.   *
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
 * Neural Jack - neural augmentation for combat advantage
 * +20% dodge, reduced cooldowns by 1 pulse
 */
void do_neuraljack( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_CYBER] < 1 ) {
		send_to_char( "You haven't trained Cybernetics yet. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_NEURAL_JACK] > 0 ) {
		snprintf( buf, sizeof(buf), "Neural jack already active (%d ticks remaining).\n\r",
			ch->pcdata->powers[MECH_NEURAL_JACK] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_NEURALJACK_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_NEURALJACK_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_NEURALJACK_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_NEURALJACK_POWER_COST );
	ch->pcdata->powers[MECH_NEURAL_JACK] = cfg( CFG_ABILITY_MECHANIST_NEURALJACK_DURATION );

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf), "%sYour neural jack interfaces with your combat systems!#n\n\r", pc );
		send_to_char( buf, ch );
		snprintf( buf, sizeof(buf), "%s[#n+%d%% dodge, cooldowns reduced%s]#n\n\r",
			ac, cfg( CFG_ABILITY_MECHANIST_NEURALJACK_DODGE_PCT ), ac );
	}
	send_to_char( buf, ch );
	act( "$n's eyes glow briefly as neural implants activate.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Servo Arms - strength augmentation for enhanced damage
 * +50 damage per melee hit, +1 extra attack per round
 */
void do_servoarms( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_CYBER] < 2 ) {
		send_to_char( "You need Cybernetics level 2. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_SERVO_ARMS] > 0 ) {
		snprintf( buf, sizeof(buf), "Servo arms already active (%d ticks remaining).\n\r",
			ch->pcdata->powers[MECH_SERVO_ARMS] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_SERVOARMS_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_SERVOARMS_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_SERVOARMS_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_SERVOARMS_POWER_COST );
	ch->pcdata->powers[MECH_SERVO_ARMS] = cfg( CFG_ABILITY_MECHANIST_SERVOARMS_DURATION );

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf), "%sServo-assisted arms engage with a hydraulic hiss!#n\n\r", pc );
		send_to_char( buf, ch );
		snprintf( buf, sizeof(buf), "%s[#n+%d melee damage, +1 extra attack%s]#n\n\r",
			ac, cfg( CFG_ABILITY_MECHANIST_SERVOARMS_DAM_BONUS ), ac );
	}
	send_to_char( buf, ch );
	act( "$n's arms whir and lock into combat configuration.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Reactive Plating - armor augmentation for defense
 * 15% damage resistance, -75 AC, reflects 10% melee damage
 */
void do_reactiveplating( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_CYBER] < 3 ) {
		send_to_char( "You need Cybernetics level 3. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_REACTIVE_PLATING] > 0 ) {
		snprintf( buf, sizeof(buf), "Reactive plating already active (%d ticks remaining).\n\r",
			ch->pcdata->powers[MECH_REACTIVE_PLATING] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_REACTIVE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_REACTIVE_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_REACTIVE_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_REACTIVE_POWER_COST );
	ch->pcdata->powers[MECH_REACTIVE_PLATING] = cfg( CFG_ABILITY_MECHANIST_REACTIVE_DURATION );

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf), "%sReactive armor plating slides into place!#n\n\r", pc );
		send_to_char( buf, ch );
		snprintf( buf, sizeof(buf), "%s[#n-%d%% damage taken, %d%% melee reflect, -75 AC%s]#n\n\r",
			ac, cfg( CFG_ABILITY_MECHANIST_REACTIVE_RESIST_PCT ),
			cfg( CFG_ABILITY_MECHANIST_REACTIVE_REFLECT_PCT ), ac );
	}
	send_to_char( buf, ch );
	act( "Armored plates shift and lock across $n's body.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Combat Drone - deploy an armed attack drone
 * Drones follow the Mechanist and attack their fighting target.
 * Max 4 combat drones (MECH_MAX_DRONES).
 */
void do_combatdrone( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *drone;
	MOB_INDEX_DATA *pMobIndex;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_DRONE] < 1 ) {
		send_to_char( "You haven't trained Drone Swarm yet. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_DRONE_COUNT] >= MECH_MAX_DRONES ) {
		snprintf( buf, sizeof(buf), "You already have %d drones deployed (max %d).\n\r",
			ch->pcdata->powers[MECH_DRONE_COUNT], MECH_MAX_DRONES );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	if ( ( pMobIndex = get_mob_index( VNUM_MECH_COMBAT_DRONE ) ) == NULL ) {
		send_to_char( "Combat drone template not found. Please contact an immortal.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_POWER_COST );

	drone = create_mobile( pMobIndex );

	/* Set drone stats */
	drone->level = ch->level;
	drone->hit = cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_HP );
	drone->max_hit = cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_HP );
	drone->hitroll = 50;
	drone->damroll = 50;
	drone->armor = -200;

	/* Customize appearance */
	snprintf( buf, sizeof(buf), "%s's combat drone", ch->name );
	free(drone->short_descr);
	drone->short_descr = str_dup( buf );
	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf), "%sA sleek combat drone belonging to %s hovers here, weapons ready.#n\n\r", pc, ch->name );
		free(drone->long_descr);
		drone->long_descr = str_dup( buf );
	}
	free(drone->name);
	drone->name = str_dup( "drone combat-drone" );

	/* Set drone flags - NO ACT_SENTINEL so drones follow */
	SET_BIT( drone->act, ACT_NOEXP );
	drone->spec_fun = NULL;

	/* Link to owner */
	free(drone->lord);
	drone->lord = str_dup( ch->name );
	drone->wizard = ch;

	/* Place in room and set up follower relationship */
	char_to_room( drone, ch->in_room );
	add_follower( drone, ch );

	af.type = skill_lookup( "charm person" );
	af.duration = 666;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char( drone, &af );

	ch->pcdata->powers[MECH_DRONE_COUNT]++;
	ch->pcdata->stats[MECH_STAT_DRONES_TOTAL]++;

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf), "%sA combat drone unfolds from your back and takes flight!#n\n\r", pc );
		send_to_char( buf, ch );
	}
	act( "A combat drone unfolds from $n's back and takes flight!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Repair Swarm - healing nanobots that heal player and drones
 * Heals 100 HP/tick to player, 50 HP/tick to all active drones.
 */
void do_repairswarm( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_DRONE] < 2 ) {
		send_to_char( "You need Drone Swarm level 2. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_REPAIR_SWARM] > 0 ) {
		snprintf( buf, sizeof(buf), "Repair swarm already active (%d ticks remaining).\n\r",
			ch->pcdata->powers[MECH_REPAIR_SWARM] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_POWER_COST );
	ch->pcdata->powers[MECH_REPAIR_SWARM] = cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_DURATION );

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf), "%sA cloud of repair nanobots swarms from your chassis!#n\n\r", pc );
		send_to_char( buf, ch );
		snprintf( buf, sizeof(buf), "%s[#n+%d HP/tick to you, +%d HP/tick to drones%s]#n\n\r",
			ac, cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_HEAL ),
			cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_DRONE_HEAL ), ac );
	}
	send_to_char( buf, ch );
	act( "A swarm of tiny repair nanobots erupts from $n's chassis.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Bomber Drone - deploy an explosive drone
 * Does not attack automatically. Use 'detonate' to trigger AoE explosion.
 * Only one bomber drone active at a time, separate from combat drone count.
 */
void do_bomberdrone( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *drone;
	MOB_INDEX_DATA *pMobIndex;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_DRONE] < 3 ) {
		send_to_char( "You need Drone Swarm level 3. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_BOMBER_ACTIVE] ) {
		send_to_char( "You already have a bomber drone deployed. Use #Rdetonate#n.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	if ( ( pMobIndex = get_mob_index( VNUM_MECH_BOMBER_DRONE ) ) == NULL ) {
		send_to_char( "Bomber drone template not found. Please contact an immortal.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_POWER_COST );

	drone = create_mobile( pMobIndex );

	/* Set drone stats - lower HP since it's meant to explode */
	drone->level = ch->level;
	drone->hit = cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_HP );
	drone->max_hit = cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_HP );
	drone->hitroll = 0;
	drone->damroll = 0;
	drone->armor = -100;

	/* Customize appearance */
	snprintf( buf, sizeof(buf), "%s's bomber drone", ch->name );
	free(drone->short_descr);
	drone->short_descr = str_dup( buf );
	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		snprintf( buf, sizeof(buf), "%sA volatile bomber drone belonging to %s hovers here, blinking ominously.#n\n\r", ac, ch->name );
	}
	free(drone->long_descr);
	drone->long_descr = str_dup( buf );
	free(drone->name);
	drone->name = str_dup( "drone bomber-drone" );

	/* Set drone flags - follows owner, no combat */
	SET_BIT( drone->act, ACT_NOEXP );
	drone->spec_fun = NULL;

	/* Link to owner */
	free(drone->lord);
	drone->lord = str_dup( ch->name );
	drone->wizard = ch;

	/* Place in room and set up follower relationship */
	char_to_room( drone, ch->in_room );
	add_follower( drone, ch );

	af.type = skill_lookup( "charm person" );
	af.duration = 666;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char( drone, &af );

	ch->pcdata->powers[MECH_BOMBER_ACTIVE] = 1;
	ch->pcdata->stats[MECH_STAT_DRONES_TOTAL]++;

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		snprintf( buf, sizeof(buf), "%sA bomber drone deploys from your chassis, blinking with explosive charge!#n\n\r", ac );
		send_to_char( buf, ch );
	}
	act( "A volatile-looking drone deploys from $n's chassis, blinking ominously.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Detonate - trigger bomber drone explosion
 * AoE damage to all enemies in the bomber drone's room.
 * Destroys the bomber drone in the process.
 */
void do_artdetonate( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *drone;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( !ch->pcdata->powers[MECH_BOMBER_ACTIVE] ) {
		send_to_char( "You don't have a bomber drone deployed.\n\r", ch );
		return;
	}

	/* Find the bomber drone */
	drone = NULL;
	LIST_FOR_EACH( vch, &g_characters, CHAR_DATA, char_node ) {
		if ( !IS_NPC( vch ) ) continue;
		if ( vch->pIndexData == NULL ) continue;
		if ( vch->pIndexData->vnum != VNUM_MECH_BOMBER_DRONE ) continue;
		if ( vch->wizard != ch ) continue;
		drone = vch;
		break;
	}

	if ( drone == NULL ) {
		send_to_char( "Your bomber drone could not be found! Resetting.\n\r", ch );
		ch->pcdata->powers[MECH_BOMBER_ACTIVE] = 0;
		return;
	}

	{
		char buf[MAX_STRING_LENGTH];
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		snprintf( buf, sizeof(buf), "%sYou trigger your bomber drone's explosive charge!#n", ac );
		act( buf, ch, NULL, NULL, TO_CHAR );
		snprintf( buf, sizeof(buf), "%s$n's bomber drone begins to glow white-hot!#n", ac );
		act( buf, ch, NULL, NULL, TO_ROOM );
	}

	/* AoE explosion in the drone's room */
	LIST_FOR_EACH_SAFE(vch, vch_next, &drone->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( vch == drone ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) ) continue;  /* No PvP splash */
		if ( IS_NPC( vch ) && vch->wizard == ch ) continue;  /* Don't hurt own drones */
		if ( is_safe( ch, vch ) ) continue;

		dam = number_range( cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_DAM_MIN ),
			cfg( CFG_ABILITY_MECHANIST_BOMBERDRONE_DAM_MAX ) );
		dam += ch->rage * 3;  /* Scales with power */

		act( "The explosion engulfs $N!", ch, NULL, vch, TO_CHAR );
		act( "You are engulfed by the explosion!", ch, NULL, vch, TO_VICT );
		damage( ch, vch, dam, gsn_punch );

		if ( vch->position == POS_DEAD || ch->fighting == NULL )
			break;
	}

	/* Destroy the bomber drone */
	extract_char( drone, TRUE );
	ch->pcdata->powers[MECH_BOMBER_ACTIVE] = 0;
	return;
}

/*
 * Drone Army - mass deployment: instantly spawns to max drones
 * and grants all drones +50% damage and +200 HP for duration.
 */
void do_dronearmy( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *drone;
	MOB_INDEX_DATA *pMobIndex;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];
	int spawned = 0;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_DRONE] < 4 ) {
		send_to_char( "You need Drone Swarm level 4. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_DRONE_ARMY] > 0 ) {
		snprintf( buf, sizeof(buf), "Drone army buff already active (%d ticks remaining).\n\r",
			ch->pcdata->powers[MECH_DRONE_ARMY] );
		send_to_char( buf, ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_DRONEARMY_CD] > 0 ) {
		send_to_char( "Your drone army systems are still recharging.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_DRONEARMY_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_DRONEARMY_POWER_REQ ) ) {
		snprintf( buf, sizeof(buf), "You need at least %d power cells for mass deployment.\n\r",
			cfg( CFG_ABILITY_MECHANIST_DRONEARMY_POWER_REQ ) );
		send_to_char( buf, ch );
		return;
	}

	if ( ( pMobIndex = get_mob_index( VNUM_MECH_COMBAT_DRONE ) ) == NULL ) {
		send_to_char( "Combat drone template not found. Please contact an immortal.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_DRONEARMY_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_DRONEARMY_POWER_COST );
	ch->pcdata->powers[MECH_DRONEARMY_CD] = cfg( CFG_ABILITY_MECHANIST_DRONEARMY_COOLDOWN );
	ch->pcdata->powers[MECH_DRONE_ARMY] = cfg( CFG_ABILITY_MECHANIST_DRONEARMY_DURATION );

	/* Spawn drones up to max */
	while ( ch->pcdata->powers[MECH_DRONE_COUNT] < MECH_MAX_DRONES ) {
		drone = create_mobile( pMobIndex );

		drone->level = ch->level;
		drone->hit = cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_HP ) + cfg( CFG_ABILITY_MECHANIST_DRONEARMY_HP_BONUS );
		drone->max_hit = drone->hit;
		drone->hitroll = 50;
		drone->damroll = 50;
		drone->armor = -200;

		snprintf( buf, sizeof(buf), "%s's combat drone", ch->name );
		free(drone->short_descr);
		drone->short_descr = str_dup( buf );
		{
			const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
			const char *pc = br ? br->primary_color : "";
			snprintf( buf, sizeof(buf), "%sA sleek combat drone belonging to %s hovers here, weapons ready.#n\n\r", pc, ch->name );
		}
		free(drone->long_descr);
		drone->long_descr = str_dup( buf );
		free(drone->name);
		drone->name = str_dup( "drone combat-drone" );

		SET_BIT( drone->act, ACT_NOEXP );
		drone->spec_fun = NULL;

		free(drone->lord);
		drone->lord = str_dup( ch->name );
		drone->wizard = ch;

		char_to_room( drone, ch->in_room );
		add_follower( drone, ch );

		af.type = skill_lookup( "charm person" );
		af.duration = 666;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_CHARM;
		affect_to_char( drone, &af );

		ch->pcdata->powers[MECH_DRONE_COUNT]++;
		ch->pcdata->stats[MECH_STAT_DRONES_TOTAL]++;
		spawned++;
	}

	/* Also grant HP bonus to existing drones */
	if ( spawned < MECH_MAX_DRONES ) {
		CHAR_DATA *vch;
		LIST_FOR_EACH( vch, &g_characters, CHAR_DATA, char_node ) {
			if ( !IS_NPC( vch ) ) continue;
			if ( vch->pIndexData == NULL ) continue;
			if ( vch->pIndexData->vnum != VNUM_MECH_COMBAT_DRONE ) continue;
			if ( vch->wizard != ch ) continue;
			vch->max_hit += cfg( CFG_ABILITY_MECHANIST_DRONEARMY_HP_BONUS );
			vch->hit = UMIN( vch->hit + cfg( CFG_ABILITY_MECHANIST_DRONEARMY_HP_BONUS ), vch->max_hit );
		}
	}

	{
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof(buf), "%sYou activate mass drone deployment! %d drones launched!#n\n\r", ac, spawned );
		send_to_char( buf, ch );
		snprintf( buf, sizeof(buf), "%s[#nAll drones: +%d%% damage, +%d HP for %d ticks%s]#n\n\r",
			pc, cfg( CFG_ABILITY_MECHANIST_DRONEARMY_DAM_BONUS_PCT ),
			cfg( CFG_ABILITY_MECHANIST_DRONEARMY_HP_BONUS ),
			cfg( CFG_ABILITY_MECHANIST_DRONEARMY_DURATION ), pc );
	}
	send_to_char( buf, ch );
	act( "An entire swarm of combat drones erupts from $n's chassis!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Railgun - heavy single target damage
 * High damage with power scaling, armor pierce (50% ignore AC)
 */
void do_railgun( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_ORDNANCE] < 1 ) {
		send_to_char( "You haven't trained Heavy Ordnance yet. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_RAILGUN_CD] > 0 ) {
		send_to_char( "Your railgun is recharging.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_RAILGUN_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_RAILGUN_POWER_REQ ) ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof(buf), "You need at least %d power cells to fire.\n\r",
			cfg( CFG_ABILITY_MECHANIST_RAILGUN_POWER_REQ ) );
		send_to_char( buf, ch );
		return;
	}

	WAIT_STATE( ch, cfg( CFG_ABILITY_MECHANIST_RAILGUN_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_MECHANIST_RAILGUN_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_RAILGUN_POWER_COST );
	ch->pcdata->powers[MECH_RAILGUN_CD] = cfg( CFG_ABILITY_MECHANIST_RAILGUN_COOLDOWN );

	dam = number_range( cfg( CFG_ABILITY_MECHANIST_RAILGUN_DAM_MIN ),
		cfg( CFG_ABILITY_MECHANIST_RAILGUN_DAM_MAX ) );
	dam += ch->rage * 4;  /* Scales with remaining power */

	{
		char buf[MAX_STRING_LENGTH];
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		snprintf( buf, sizeof(buf), "%sYou charge and fire your railgun at $N with devastating force!#n", ac );
		act( buf, ch, NULL, victim, TO_CHAR );
		snprintf( buf, sizeof(buf), "$n aims a massive weapon and fires - a %sthunderclap#n shakes the room!", ac );
		act( buf, ch, NULL, victim, TO_VICT );
		snprintf( buf, sizeof(buf), "$n aims a massive weapon at $N and fires - a %sthunderclap#n shakes the room!", ac );
		act( buf, ch, NULL, victim, TO_NOTVICT );
	}

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * EMP Burst - AoE damage + tech disruption
 * Damages all enemies in room, strips force fields and tech shields
 */
void do_empburst( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_ORDNANCE] < 2 ) {
		send_to_char( "You need Heavy Ordnance level 2. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_EMP_CD] > 0 ) {
		send_to_char( "Your EMP emitter is recharging.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_EMPBURST_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_EMPBURST_POWER_COST ) ) {
		send_to_char( "You don't have enough power cells.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, cfg( CFG_ABILITY_MECHANIST_EMPBURST_COOLDOWN ) );
	ch->mana -= cfg( CFG_ABILITY_MECHANIST_EMPBURST_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_EMPBURST_POWER_COST );
	ch->pcdata->powers[MECH_EMP_CD] = cfg( CFG_ABILITY_MECHANIST_EMPBURST_COOLDOWN );

	{
		char buf[MAX_STRING_LENGTH];
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		snprintf( buf, sizeof(buf), "%sYou release a massive electromagnetic pulse!#n", ac );
		act( buf, ch, NULL, NULL, TO_CHAR );
		snprintf( buf, sizeof(buf), "%s$n releases a blinding electromagnetic pulse!#n", ac );
		act( buf, ch, NULL, NULL, TO_ROOM );
	}

	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) ) continue;  /* No PvP splash */
		if ( IS_NPC( vch ) && vch->fighting == NULL ) continue;
		if ( is_safe( ch, vch ) ) continue;

		dam = number_range( cfg( CFG_ABILITY_MECHANIST_EMPBURST_DAM_MIN ),
			cfg( CFG_ABILITY_MECHANIST_EMPBURST_DAM_MAX ) );
		dam += ch->rage * 2;  /* Scales with power */

		/* Strip Artificer force fields */
		if ( !IS_NPC( vch ) && IS_CLASS( vch, CLASS_ARTIFICER ) &&
			vch->pcdata->powers[ART_FORCEFIELD] > 0 ) {
			vch->pcdata->powers[ART_FORCEFIELD] = 0;
			vch->pcdata->stats[ART_STAT_FORCEFIELD_HP] = 0;
			send_to_char( "The EMP shatters your force field!\n\r", vch );
			dam += dam / 2;  /* +50% vs tech targets */
		}

		act( "The electromagnetic pulse surges through $N!", ch, NULL, vch, TO_CHAR );
		damage( ch, vch, dam, gsn_punch );

		if ( vch->position == POS_DEAD || ch->fighting == NULL )
			break;
	}
	return;
}

/*
 * Orbital Strike - ultimate delayed AoE attack
 */
void do_orbitalstrike( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_TRAIN_ORDNANCE] < 3 ) {
		send_to_char( "You need Heavy Ordnance level 3. See #Rcybtrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( !IS_OUTSIDE( ch ) ) {
		send_to_char( "You need to be outdoors to call an orbital strike!\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_ORBITAL_CHARGE] > 0 ) {
		send_to_char( "An orbital strike is already charging!\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MECH_ORBITAL_CD] > 0 ) {
		send_to_char( "Your orbital targeting systems are still recharging.\n\r", ch );
		return;
	}
	if ( ch->mana < cfg( CFG_ABILITY_MECHANIST_ORBITAL_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < cfg( CFG_ABILITY_MECHANIST_ORBITAL_POWER_REQ ) ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof(buf), "You need at least %d power cells for orbital targeting.\n\r",
			cfg( CFG_ABILITY_MECHANIST_ORBITAL_POWER_REQ ) );
		send_to_char( buf, ch );
		return;
	}

	ch->mana -= cfg( CFG_ABILITY_MECHANIST_ORBITAL_MANA_COST );
	ch->rage -= cfg( CFG_ABILITY_MECHANIST_ORBITAL_POWER_COST );
	ch->pcdata->powers[MECH_ORBITAL_CD] = cfg( CFG_ABILITY_MECHANIST_ORBITAL_COOLDOWN );
	ch->pcdata->powers[MECH_ORBITAL_CHARGE] = 1;

	{
		char buf[MAX_STRING_LENGTH];
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *ac = br ? br->accent_color : "";
		snprintf( buf, sizeof(buf), "%sYou activate orbital targeting systems - strike incoming in 2 rounds!#n\n\r", ac );
		send_to_char( buf, ch );
		snprintf( buf, sizeof(buf), "%s$n activates a targeting beacon - something is being called down from above!#n", ac );
		act( buf, ch, NULL, NULL, TO_ROOM );
	}
	return;
}

/*
 * Implant management - select and swap cybernetic implants
 */
void do_mechimplant( CHAR_DATA *ch, char *argument ) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	if ( arg1[0] == '\0' || !str_cmp( arg1, "list" ) ) {
		snprintf( buf, sizeof( buf ),"%s>%s/#n Implant Status %s\\%s<#n\n\r", ac, pc, pc, ac );
		send_to_char( buf, ch );

		/* Neural slot */
		if ( ch->pcdata->powers[MECH_TRAIN_CYBER] >= 1 ) {
			snprintf( buf, sizeof( buf ),"Neural: %s\n\r",
				ch->pcdata->powers[MECH_NEURAL_IMPLANT] == IMPLANT_NEURAL_COMBAT_PROC ? "Combat Processor" :
				ch->pcdata->powers[MECH_NEURAL_IMPLANT] == IMPLANT_NEURAL_TARGETING ? "Targeting Suite" :
				ch->pcdata->powers[MECH_NEURAL_IMPLANT] == IMPLANT_NEURAL_THREAT ? "Threat Analyzer" : "None" );
			send_to_char( buf, ch );
		}

		/* Servo slot */
		if ( ch->pcdata->powers[MECH_TRAIN_CYBER] >= 2 ) {
			snprintf( buf, sizeof( buf ),"Servo:  %s\n\r",
				ch->pcdata->powers[MECH_SERVO_IMPLANT] == IMPLANT_SERVO_POWER_ARMS ? "Power Arms" :
				ch->pcdata->powers[MECH_SERVO_IMPLANT] == IMPLANT_SERVO_MULTI_TOOL ? "Multi-Tool" :
				ch->pcdata->powers[MECH_SERVO_IMPLANT] == IMPLANT_SERVO_SHIELD_GEN ? "Shield Generator" : "None" );
			send_to_char( buf, ch );
		}

		/* Core slot */
		if ( ch->pcdata->powers[MECH_TRAIN_CYBER] >= 3 ) {
			snprintf( buf, sizeof( buf ),"Core:   %s\n\r",
				ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_ARMORED ? "Armored Chassis" :
				ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_REGENERATOR ? "Regenerator" :
				ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_POWER ? "Power Core" : "None" );
			send_to_char( buf, ch );
		}

		send_to_char( "\n\rSyntax: cybimplant <neural|servo|core> <type>\n\r", ch );
		return;
	}

	if ( ch->fighting != NULL ) {
		send_to_char( "You can't swap implants while fighting!\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[MECH_IMPLANT_CD] > 0 ) {
		send_to_char( "Your implant systems are still recalibrating.\n\r", ch );
		return;
	}

	/* Neural implant swapping */
	if ( !str_prefix( arg1, "neural" ) ) {
		if ( ch->pcdata->powers[MECH_TRAIN_CYBER] < 1 ) {
			send_to_char( "You need Cybernetics level 1 to use neural implants.\n\r", ch );
			return;
		}
		if ( arg2[0] == '\0' ) {
			send_to_char( "Available neural implants:\n\r", ch );
			snprintf( buf, sizeof( buf ),"  %scombat#n    - Combat Processor: +15%% dodge, -1 pulse cooldowns\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %stargeting#n  - Targeting Suite: +20 hitroll, +20%% railgun damage\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %sthreat#n     - Threat Analyzer: see enemy HP/buffs\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %snone#n       - Remove implant\n\r", pc ); send_to_char( buf, ch );
			return;
		}
		if ( !str_prefix( arg2, "combat" ) ) {
			ch->pcdata->powers[MECH_NEURAL_IMPLANT] = IMPLANT_NEURAL_COMBAT_PROC;
			snprintf( buf, sizeof( buf ),"%sCombat Processor neural implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "targeting" ) ) {
			ch->pcdata->powers[MECH_NEURAL_IMPLANT] = IMPLANT_NEURAL_TARGETING;
			snprintf( buf, sizeof( buf ),"%sTargeting Suite neural implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "threat" ) ) {
			ch->pcdata->powers[MECH_NEURAL_IMPLANT] = IMPLANT_NEURAL_THREAT;
			snprintf( buf, sizeof( buf ),"%sThreat Analyzer neural implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "none" ) ) {
			ch->pcdata->powers[MECH_NEURAL_IMPLANT] = IMPLANT_NEURAL_NONE;
			send_to_char( "Neural implant removed.\n\r", ch );
		} else {
			send_to_char( "Invalid neural implant. Options: combat, targeting, threat, none\n\r", ch );
			return;
		}
	}
	/* Servo implant swapping */
	else if ( !str_prefix( arg1, "servo" ) ) {
		if ( ch->pcdata->powers[MECH_TRAIN_CYBER] < 2 ) {
			send_to_char( "You need Cybernetics level 2 to use servo implants.\n\r", ch );
			return;
		}
		if ( arg2[0] == '\0' ) {
			send_to_char( "Available servo implants:\n\r", ch );
			snprintf( buf, sizeof( buf ),"  %spower#n     - Power Arms: +30 damroll, +50 melee damage\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %smulti#n     - Multi-Tool: +1 extra attack\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %sshield#n    - Shield Generator: +200 damcap\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %snone#n      - Remove implant\n\r", pc ); send_to_char( buf, ch );
			return;
		}
		if ( !str_prefix( arg2, "power" ) ) {
			ch->pcdata->powers[MECH_SERVO_IMPLANT] = IMPLANT_SERVO_POWER_ARMS;
			snprintf( buf, sizeof( buf ),"%sPower Arms servo implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "multi" ) ) {
			ch->pcdata->powers[MECH_SERVO_IMPLANT] = IMPLANT_SERVO_MULTI_TOOL;
			snprintf( buf, sizeof( buf ),"%sMulti-Tool servo implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "shield" ) ) {
			ch->pcdata->powers[MECH_SERVO_IMPLANT] = IMPLANT_SERVO_SHIELD_GEN;
			snprintf( buf, sizeof( buf ),"%sShield Generator servo implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "none" ) ) {
			ch->pcdata->powers[MECH_SERVO_IMPLANT] = IMPLANT_SERVO_NONE;
			send_to_char( "Servo implant removed.\n\r", ch );
		} else {
			send_to_char( "Invalid servo implant. Options: power, multi, shield, none\n\r", ch );
			return;
		}
	}
	/* Core implant swapping */
	else if ( !str_prefix( arg1, "core" ) ) {
		if ( ch->pcdata->powers[MECH_TRAIN_CYBER] < 3 ) {
			send_to_char( "You need Cybernetics level 3 to use core implants.\n\r", ch );
			return;
		}
		if ( arg2[0] == '\0' ) {
			send_to_char( "Available core implants:\n\r", ch );
			snprintf( buf, sizeof( buf ),"  %sarmored#n   - Armored Chassis: -50 AC, 10%% damage resistance\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %sregen#n     - Regenerator: +100 HP/tick regen\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %spower#n     - Power Core: +25 max power, +1 power/tick\n\r", pc ); send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ),"  %snone#n      - Remove implant\n\r", pc ); send_to_char( buf, ch );
			return;
		}
		if ( !str_prefix( arg2, "armored" ) ) {
			ch->pcdata->powers[MECH_CORE_IMPLANT] = IMPLANT_CORE_ARMORED;
			snprintf( buf, sizeof( buf ),"%sArmored Chassis core implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "regen" ) ) {
			ch->pcdata->powers[MECH_CORE_IMPLANT] = IMPLANT_CORE_REGENERATOR;
			snprintf( buf, sizeof( buf ),"%sRegenerator core implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "power" ) ) {
			ch->pcdata->powers[MECH_CORE_IMPLANT] = IMPLANT_CORE_POWER;
			snprintf( buf, sizeof( buf ),"%sPower Core implant installed.#n\n\r", pc ); send_to_char( buf, ch );
		} else if ( !str_prefix( arg2, "none" ) ) {
			ch->pcdata->powers[MECH_CORE_IMPLANT] = IMPLANT_CORE_NONE;
			send_to_char( "Core implant removed.\n\r", ch );
		} else {
			send_to_char( "Invalid core implant. Options: armored, regen, power, none\n\r", ch );
			return;
		}
	} else {
		send_to_char( "Valid slots: neural, servo, core\n\r", ch );
		return;
	}

	ch->pcdata->powers[MECH_IMPLANT_CD] = cfg( CFG_ABILITY_MECHANIST_IMPLANT_SWAP_CD );
	act( "$n's cybernetics whir and reconfigure.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Cybernetic Training - improve Mechanist skill categories
 */
void do_cybtrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int *path;
	int max_level;
	int cost;
	const char *path_name;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		{
			const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
			const char *ac = br ? br->accent_color : "";
			const char *pc = br ? br->primary_color : "";
			snprintf( buf, sizeof( buf ),"%s>%s/#n Mechanist Training %s\\%s<#n\n\r", ac, pc, pc, ac );
			send_to_char( buf, ch );
		}
		snprintf( buf, sizeof( buf ),"Cybernetics:   Level %d/3 (neuraljack, servoarms, reactiveplating)\n\r",
			ch->pcdata->powers[MECH_TRAIN_CYBER] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"Drone Swarm:   Level %d/4 (combatdrone, repairswarm, bomberdrone, dronearmy)\n\r",
			ch->pcdata->powers[MECH_TRAIN_DRONE] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ),"Heavy Ordnance: Level %d/3 (railgun, empburst, orbitalstrike)\n\r",
			ch->pcdata->powers[MECH_TRAIN_ORDNANCE] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: cybtrain <cybernetics|drones|ordnance>\n\r", ch );
		return;
	}

	if ( !str_prefix( arg, "cybernetics" ) ) {
		path = &ch->pcdata->powers[MECH_TRAIN_CYBER];
		max_level = 3;
		path_name = "Cybernetics";
	} else if ( !str_prefix( arg, "drones" ) ) {
		path = &ch->pcdata->powers[MECH_TRAIN_DRONE];
		max_level = 4;
		path_name = "Drone Swarm";
	} else if ( !str_prefix( arg, "ordnance" ) ) {
		path = &ch->pcdata->powers[MECH_TRAIN_ORDNANCE];
		max_level = 3;
		path_name = "Heavy Ordnance";
	} else {
		send_to_char( "Valid paths: cybernetics, drones, ordnance\n\r", ch );
		return;
	}

	if ( *path >= max_level ) {
		snprintf( buf, sizeof( buf ),"Your %s training is already at maximum (%d).\n\r", path_name, max_level );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *path + 1 ) * 50;  /* 50 primal per level for Mechanist */

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
		const char *extra = NULL;

		if ( !str_prefix( arg, "cybernetics" ) ) {
			if ( *path == 1 ) { ability = "neuraljack"; extra = " Neural implants unlocked."; }
			else if ( *path == 2 ) { ability = "servoarms"; extra = " Servo implants unlocked."; }
			else if ( *path == 3 ) { ability = "reactiveplating"; extra = " Core implants unlocked."; }
		} else if ( !str_prefix( arg, "drones" ) ) {
			if ( *path == 1 ) ability = "combatdrone";
			else if ( *path == 2 ) ability = "repairswarm";
			else if ( *path == 3 ) ability = "bomberdrone";
			else if ( *path == 4 ) ability = "dronearmy";
		} else if ( !str_prefix( arg, "ordnance" ) ) {
			if ( *path == 1 ) ability = "railgun";
			else if ( *path == 2 ) ability = "empburst";
			else if ( *path == 3 ) ability = "orbitalstrike";
		}
		if ( ability ) {
			snprintf( buf, sizeof( buf ),"You have learned %s%s#n!%s\n\r", pc, ability, extra ? extra : "" );
			send_to_char( buf, ch );
		}
	}
	return;
}

/*
 * Mechanist Armor - create class equipment
 */
void do_mechanistarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_MECHANIST );
}

/*
 * Drone Status - show all drone HP and status
 */
void do_dronestatus( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int num = 0;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	snprintf( buf, sizeof( buf ),"%s>%s/#n Drone Status %s\\%s<#n\n\r", ac, pc, pc, ac );
	send_to_char( buf, ch );
	snprintf( buf, sizeof(buf), "Combat Drones: %d / %d\n\r",
		ch->pcdata->powers[MECH_DRONE_COUNT], MECH_MAX_DRONES );
	send_to_char( buf, ch );

	/* Show individual combat drone HP */
	LIST_FOR_EACH( vch, &g_characters, CHAR_DATA, char_node ) {
		if ( !IS_NPC( vch ) ) continue;
		if ( vch->pIndexData == NULL ) continue;
		if ( vch->wizard != ch ) continue;
		if ( vch->pIndexData->vnum == VNUM_MECH_COMBAT_DRONE ) {
			num++;
			snprintf( buf, sizeof(buf), "  %sDrone %d#n: %d/%d HP%s\n\r", pc,
				num, vch->hit, vch->max_hit,
				vch->fighting != NULL ? " #R(fighting)#n" : "" );
			send_to_char( buf, ch );
		}
	}

	/* Show bomber drone */
	if ( ch->pcdata->powers[MECH_BOMBER_ACTIVE] ) {
		LIST_FOR_EACH( vch, &g_characters, CHAR_DATA, char_node ) {
			if ( !IS_NPC( vch ) ) continue;
			if ( vch->pIndexData == NULL ) continue;
			if ( vch->pIndexData->vnum != VNUM_MECH_BOMBER_DRONE ) continue;
			if ( vch->wizard != ch ) continue;
			snprintf( buf, sizeof(buf), "  %sBomber#n: %d/%d HP - ready to #Rdetonate#n\n\r", ac,
				vch->hit, vch->max_hit );
			send_to_char( buf, ch );
			break;
		}
	}

	if ( ch->pcdata->powers[MECH_DRONE_ARMY] > 0 ) {
		snprintf( buf, sizeof(buf), "%s[#nDrone Army buff: #C%d#n ticks, +%d%% damage%s]#n\n\r", ac,
			ch->pcdata->powers[MECH_DRONE_ARMY],
			cfg( CFG_ABILITY_MECHANIST_DRONEARMY_DAM_BONUS_PCT ), ac );
		send_to_char( buf, ch );
	}

	if ( ch->pcdata->powers[MECH_REPAIR_SWARM] > 0 ) {
		snprintf( buf, sizeof(buf), "%s[#nRepair Swarm: #C%d#n ticks remaining%s]#n\n\r", ac,
			ch->pcdata->powers[MECH_REPAIR_SWARM], ac );
		send_to_char( buf, ch );
	}

	snprintf( buf, sizeof(buf), "Total drones deployed (lifetime): %d\n\r",
		ch->pcdata->stats[MECH_STAT_DRONES_TOTAL] );
	send_to_char( buf, ch );
	return;
}

/*
 * Drone Recall - despawn all drones (combat + bomber)
 */
void do_dronerecall( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if ( !IS_CLASS( ch, CLASS_MECHANIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[MECH_DRONE_COUNT] == 0 && !ch->pcdata->powers[MECH_BOMBER_ACTIVE] ) {
		send_to_char( "You don't have any drones deployed.\n\r", ch );
		return;
	}

	/* Find and extract all drone mobs */
	LIST_FOR_EACH_SAFE( vch, vch_next, &g_characters, CHAR_DATA, char_node ) {

		if ( !IS_NPC( vch ) ) continue;
		if ( vch->pIndexData == NULL ) continue;
		if ( vch->wizard != ch ) continue;
		if ( vch->pIndexData->vnum != VNUM_MECH_COMBAT_DRONE &&
			vch->pIndexData->vnum != VNUM_MECH_BOMBER_DRONE ) continue;

		act( "$n folds up and returns to $S master.", vch, NULL, ch, TO_ROOM );
		extract_char( vch, TRUE );
	}

	ch->pcdata->powers[MECH_DRONE_COUNT] = 0;
	ch->pcdata->powers[MECH_BOMBER_ACTIVE] = 0;

	{
		char buf[MAX_STRING_LENGTH];
		const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
		const char *pc = br ? br->primary_color : "";
		snprintf( buf, sizeof( buf ),"%sYou recall all your drones to storage compartments.#n\n\r", pc );
		send_to_char( buf, ch );
	}
	act( "$n's drones fly back and fold into storage compartments.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Update function called each tick from update.c
 */
void update_mechanist( CHAR_DATA *ch ) {
	if ( !IS_CLASS( ch, CLASS_MECHANIST ) )
		return;

	/* Power cell generation - Mechanist has no decay */
	{
		int power_cap = MECH_POWER_CAP;
		int bonus_tick = 0;

		/* Power Core implant: +25 max power, +1 power/tick */
		if ( ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_POWER ) {
			power_cap += cfg( CFG_ABILITY_MECHANIST_IMPLANT_CORE_POWER_MAX );
			bonus_tick = cfg( CFG_ABILITY_MECHANIST_IMPLANT_CORE_POWER_TICK );
		}

		if ( ch->position == POS_FIGHTING ) {
			if ( ch->rage < power_cap )
				ch->rage = UMIN( ch->rage + 3 + bonus_tick, power_cap );
		} else {
			if ( ch->rage < power_cap )
				ch->rage = UMIN( ch->rage + 1 + bonus_tick, power_cap );
		}
	}

	/* Implant passive tick effects */
	if ( ch->pcdata->powers[MECH_CORE_IMPLANT] == IMPLANT_CORE_REGENERATOR )
		ch->hit = UMIN( ch->hit + cfg( CFG_ABILITY_MECHANIST_IMPLANT_CORE_REGEN_HP ), ch->max_hit );

	/* Neural Jack duration countdown */
	if ( ch->pcdata->powers[MECH_NEURAL_JACK] > 0 ) {
		ch->pcdata->powers[MECH_NEURAL_JACK]--;
		if ( ch->pcdata->powers[MECH_NEURAL_JACK] <= 0 )
			send_to_char( "Your neural enhancement subsides.\n\r", ch );
	}

	/* Servo Arms duration countdown */
	if ( ch->pcdata->powers[MECH_SERVO_ARMS] > 0 ) {
		ch->pcdata->powers[MECH_SERVO_ARMS]--;
		if ( ch->pcdata->powers[MECH_SERVO_ARMS] <= 0 )
			send_to_char( "Your servo arms power down.\n\r", ch );
	}

	/* Reactive Plating duration countdown */
	if ( ch->pcdata->powers[MECH_REACTIVE_PLATING] > 0 ) {
		ch->pcdata->powers[MECH_REACTIVE_PLATING]--;
		if ( ch->pcdata->powers[MECH_REACTIVE_PLATING] <= 0 )
			send_to_char( "Your reactive plating deactivates.\n\r", ch );
	}

	/* Repair Swarm healing */
	if ( ch->pcdata->powers[MECH_REPAIR_SWARM] > 0 ) {
		/* Heal player */
		ch->hit = UMIN( ch->hit + cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_HEAL ), ch->max_hit );

		/* Heal all active drones */
		if ( ch->pcdata->powers[MECH_DRONE_COUNT] > 0 || ch->pcdata->powers[MECH_BOMBER_ACTIVE] ) {
			CHAR_DATA *vch;
			int dheal = cfg( CFG_ABILITY_MECHANIST_REPAIRSWARM_DRONE_HEAL );
			LIST_FOR_EACH( vch, &g_characters, CHAR_DATA, char_node ) {
				if ( !IS_NPC( vch ) ) continue;
				if ( vch->pIndexData == NULL ) continue;
				if ( vch->wizard != ch ) continue;
				if ( vch->pIndexData->vnum != VNUM_MECH_COMBAT_DRONE &&
					vch->pIndexData->vnum != VNUM_MECH_BOMBER_DRONE ) continue;
				vch->hit = UMIN( vch->hit + dheal, vch->max_hit );
			}
		}

		ch->pcdata->powers[MECH_REPAIR_SWARM]--;
		if ( ch->pcdata->powers[MECH_REPAIR_SWARM] <= 0 )
			send_to_char( "Your repair nanobots disperse.\n\r", ch );
	}

	/* Drone Army buff countdown */
	if ( ch->pcdata->powers[MECH_DRONE_ARMY] > 0 ) {
		ch->pcdata->powers[MECH_DRONE_ARMY]--;
		if ( ch->pcdata->powers[MECH_DRONE_ARMY] <= 0 )
			send_to_char( "Your drone army buff expires.\n\r", ch );
	}

	/* Orbital Strike charge processing */
	if ( ch->pcdata->powers[MECH_ORBITAL_CHARGE] > 0 && ch->fighting != NULL ) {
		ch->pcdata->powers[MECH_ORBITAL_CHARGE]++;
		if ( ch->pcdata->powers[MECH_ORBITAL_CHARGE] == 2 ) {
			char buf[MAX_STRING_LENGTH];
			const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
			const char *ac = br ? br->accent_color : "";
			snprintf( buf, sizeof(buf), "%sThe sky begins to glow ominously overhead...#n", ac );
			act( buf, ch, NULL, NULL, TO_CHAR );
			act( buf, ch, NULL, NULL, TO_ROOM );
		} else if ( ch->pcdata->powers[MECH_ORBITAL_CHARGE] >= 3 ) {
			CHAR_DATA *victim = ch->fighting;
			CHAR_DATA *vch;
			CHAR_DATA *vch_next;
			int dam;

			act( "#R*** ORBITAL STRIKE INCOMING! ***#n", ch, NULL, NULL, TO_CHAR );
			act( "#R*** ORBITAL STRIKE INCOMING! ***#n", ch, NULL, NULL, TO_ROOM );

			/* Primary target: full damage */
			dam = number_range( cfg( CFG_ABILITY_MECHANIST_ORBITAL_DAM_MIN ),
				cfg( CFG_ABILITY_MECHANIST_ORBITAL_DAM_MAX ) );
			dam += ch->rage * 6;  /* Heavy power scaling */

			{
				char abuf[MAX_STRING_LENGTH];
				const CLASS_BRACKET *br = db_class_get_bracket( ch->class );
				const char *ac = br ? br->accent_color : "";
				snprintf( abuf, sizeof(abuf), "%sThe orbital lance slams into $N with blinding force!#n", ac );
				act( abuf, ch, NULL, victim, TO_CHAR );
				snprintf( abuf, sizeof(abuf), "%sAn orbital lance slams into you from the sky!#n", ac );
				act( abuf, ch, NULL, victim, TO_VICT );
				snprintf( abuf, sizeof(abuf), "%sAn orbital lance slams into $N from the sky!#n", ac );
				act( abuf, ch, NULL, victim, TO_NOTVICT );
			}
			damage( ch, victim, dam, gsn_punch );

			/* AoE splash: hit other enemies for reduced damage */
			if ( victim->position != POS_DEAD && ch->fighting != NULL ) {
				int aoe_pct = cfg( CFG_ABILITY_MECHANIST_ORBITAL_AOE_PCT );
				LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
					if ( vch == ch ) continue;
					if ( vch == victim ) continue;
					if ( !IS_NPC( vch ) && !IS_NPC( ch ) ) continue;
					if ( IS_NPC( vch ) && vch->wizard == ch ) continue;
					if ( is_safe( ch, vch ) ) continue;
					if ( IS_NPC( vch ) && vch->fighting == NULL ) continue;

					dam = number_range( cfg( CFG_ABILITY_MECHANIST_ORBITAL_DAM_MIN ),
						cfg( CFG_ABILITY_MECHANIST_ORBITAL_DAM_MAX ) );
					dam += ch->rage * 6;
					dam = dam * aoe_pct / 100;

					act( "The shockwave blasts $N!", ch, NULL, vch, TO_CHAR );
					damage( ch, vch, dam, gsn_punch );

					if ( vch->position == POS_DEAD || ch->fighting == NULL )
						break;
				}
			}

			ch->pcdata->powers[MECH_ORBITAL_CHARGE] = 0;
		}
	}
	/* Cancel orbital charge if not fighting */
	if ( ch->pcdata->powers[MECH_ORBITAL_CHARGE] > 0 && ch->fighting == NULL ) {
		ch->pcdata->powers[MECH_ORBITAL_CHARGE] = 0;
		send_to_char( "Orbital targeting lost - strike cancelled.\n\r", ch );
	}

	/* Cooldown decrements */
	if ( ch->pcdata->powers[MECH_RAILGUN_CD] > 0 )
		ch->pcdata->powers[MECH_RAILGUN_CD]--;
	if ( ch->pcdata->powers[MECH_EMP_CD] > 0 )
		ch->pcdata->powers[MECH_EMP_CD]--;
	if ( ch->pcdata->powers[MECH_ORBITAL_CD] > 0 )
		ch->pcdata->powers[MECH_ORBITAL_CD]--;
	if ( ch->pcdata->powers[MECH_IMPLANT_CD] > 0 )
		ch->pcdata->powers[MECH_IMPLANT_CD]--;
	if ( ch->pcdata->powers[MECH_DRONEARMY_CD] > 0 )
		ch->pcdata->powers[MECH_DRONEARMY_CD]--;

	/* Track peak power */
	if ( ch->rage > ch->pcdata->stats[MECH_STAT_PEAK_POWER] )
		ch->pcdata->stats[MECH_STAT_PEAK_POWER] = ch->rage;

	return;
}

/*
 * Helper: Move drones when player teleports (non-directional movement)
 * For normal directional movement, drones follow via the follower system.
 * This handles teleport, recall, portal, grapple, etc.
 */
void mechanist_move_drones( CHAR_DATA *ch, ROOM_INDEX_DATA *to_room ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if ( to_room == NULL ) return;

	LIST_FOR_EACH_SAFE( vch, vch_next, &g_characters, CHAR_DATA, char_node ) {

		if ( !IS_NPC( vch ) ) continue;
		if ( vch->pIndexData == NULL ) continue;
		if ( vch->wizard != ch ) continue;
		if ( vch->pIndexData->vnum != VNUM_MECH_COMBAT_DRONE &&
			vch->pIndexData->vnum != VNUM_MECH_BOMBER_DRONE ) continue;

		if ( vch->fighting != NULL )
			stop_fighting( vch, TRUE );

		char_from_room( vch );
		char_to_room( vch, to_room );
	}
	return;
}

/*
 * Helper: Process drone attacks during violence update
 * Called from violence_update() in fight.c when the Mechanist is fighting.
 * Each combat drone in the room attacks the Mechanist's current target.
 * Bomber drones do NOT attack (they're meant to be detonated).
 */
void mechanist_drone_attacks( CHAR_DATA *ch ) {
	CHAR_DATA *drone;
	CHAR_DATA *victim;
	int dam;
	int bonus_pct;

	if ( ( victim = ch->fighting ) == NULL ) return;
	if ( ch->in_room == NULL ) return;

	bonus_pct = ( ch->pcdata->powers[MECH_DRONE_ARMY] > 0 )
		? cfg( CFG_ABILITY_MECHANIST_DRONEARMY_DAM_BONUS_PCT ) : 0;

	LIST_FOR_EACH(drone, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( !IS_NPC( drone ) ) continue;
		if ( drone->pIndexData == NULL ) continue;
		if ( drone->pIndexData->vnum != VNUM_MECH_COMBAT_DRONE ) continue;
		if ( drone->wizard != ch ) continue;
		if ( !IS_AWAKE( drone ) ) continue;
		if ( drone->in_room != victim->in_room ) continue;

		dam = number_range( cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_DAM_MIN ),
			cfg( CFG_ABILITY_MECHANIST_COMBATDRONE_DAM_MAX ) );

		/* Drone Army damage bonus */
		if ( bonus_pct > 0 )
			dam += dam * bonus_pct / 100;

		act( "$n targets $N and opens fire!", drone, NULL, victim, TO_ROOM );
		damage( drone, victim, dam, gsn_punch );

		/* If victim died, stop processing */
		if ( victim->position == POS_DEAD || ch->fighting == NULL )
			break;
	}
	return;
}
