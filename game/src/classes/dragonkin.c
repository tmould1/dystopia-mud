/***************************************************************************
 *  Dragonkin class - half-dragon humanoids with elemental attunement       *
 *  Uses "Draconic Essence" resource (ch->rage) that builds during combat.  *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "ability_config.h"
#include "dragonkin.h"

/* Forward declarations for update functions */
void update_dragonkin( CHAR_DATA *ch );

/*
 * Dragonkin status display - shows Essence and active buffs
 */
void do_dragonstatus( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	const char *attune_names[] = { "Fire", "Frost", "Storm", "Earth" };
	int attune;
	int essence_max;

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;
	attune = ch->pcdata->powers[DRAGON_ATTUNEMENT];
	if ( attune < 0 || attune > 3 ) attune = 0;

	sprintf( buf, "#x202~#x208[#n Dragonkin Status #x208]#x202~#n\n\r" );
	send_to_char( buf, ch );
	sprintf( buf, "Draconic Essence: #x208%d#n / %d\n\r", ch->rage, essence_max );
	send_to_char( buf, ch );
	sprintf( buf, "Elemental Attunement: #x208%s#n\n\r", attune_names[attune] );
	send_to_char( buf, ch );

	if ( ch->pcdata->powers[DRAGON_SCALESHIELD] > 0 ) {
		sprintf( buf, "#x208[#nScaleshield: #C%d#n HP remaining#x208]#n\n\r", ch->pcdata->stats[DRAGON_SCALESHIELD_HP] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DRAGON_DRAGONHIDE] ) {
		send_to_char( "#x208[#nDragonhide active#x208]#n\n\r", ch );
	}
	if ( ch->pcdata->powers[DRAGON_PRIMALWARDING] > 0 ) {
		sprintf( buf, "#x208[#nPrimal Warding: #C%d#n ticks remaining#x208]#n\n\r", ch->pcdata->powers[DRAGON_PRIMALWARDING] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[DRAGON_DRAKEWINGS] > 0 ) {
		sprintf( buf, "#x208[#nDrakewings: #C%d#n ticks remaining#x208]#n\n\r", ch->pcdata->powers[DRAGON_DRAKEWINGS] );
		send_to_char( buf, ch );
	}

	/* Training levels */
	send_to_char( "\n\r#x202Training:#n\n\r", ch );
	sprintf( buf, "  Draconic Breath: %d/3\n\r", ch->pcdata->powers[DRAGON_TRAIN_BREATH] );
	send_to_char( buf, ch );
	sprintf( buf, "  Dragon Scales:   %d/3\n\r", ch->pcdata->powers[DRAGON_TRAIN_SCALES] );
	send_to_char( buf, ch );
	sprintf( buf, "  Dragon Might:    %d/3\n\r", ch->pcdata->powers[DRAGON_TRAIN_MIGHT] );
	send_to_char( buf, ch );
	sprintf( buf, "  Essence Mastery: %d/2\n\r", ch->pcdata->powers[DRAGON_TRAIN_ESSENCE] );
	send_to_char( buf, ch );

	return;
}

/*
 * Attune - Switch elemental attunement (Fire/Frost/Storm/Earth)
 */
void do_attune( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	int new_attune = -1;
	const char *attune_names[] = { "Fire", "Frost", "Storm", "Earth" };

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_ESSENCE] < 1 ) {
		send_to_char( "You haven't trained Essence Mastery yet. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Attune to which element? (fire, frost, storm, earth)\n\r", ch );
		return;
	}

	if ( !str_prefix( arg, "fire" ) )       new_attune = ATTUNE_FIRE;
	else if ( !str_prefix( arg, "frost" ) ) new_attune = ATTUNE_FROST;
	else if ( !str_prefix( arg, "storm" ) ) new_attune = ATTUNE_STORM;
	else if ( !str_prefix( arg, "earth" ) ) new_attune = ATTUNE_EARTH;
	else {
		send_to_char( "Unknown element. Choose: fire, frost, storm, earth.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_ATTUNEMENT] == new_attune ) {
		send_to_char( "You are already attuned to that element.\n\r", ch );
		return;
	}

	if ( ch->fighting != NULL ) {
		send_to_char( "You cannot change attunement while in combat.\n\r", ch );
		return;
	}

	ch->pcdata->powers[DRAGON_ATTUNEMENT] = new_attune;
	act( "You attune your draconic essence to $t.", ch, attune_names[new_attune], NULL, TO_CHAR );
	act( "$n's scales shimmer as they attune to a new element.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Essencemeditate - Recover Essence out of combat
 */
void do_essencemeditate( CHAR_DATA *ch, char *argument ) {
	int gain;
	int essence_max;

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_ESSENCE] < 2 ) {
		send_to_char( "You need Essence Mastery level 2 to meditate. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ch->fighting != NULL ) {
		send_to_char( "You cannot meditate while in combat.\n\r", ch );
		return;
	}

	if ( ch->position != POS_RESTING && ch->position != POS_SITTING ) {
		send_to_char( "You must be resting or sitting to meditate.\n\r", ch );
		return;
	}

	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;
	if ( ch->rage >= essence_max ) {
		send_to_char( "Your draconic essence is already at its peak.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, acfg( ACFG_DRAGONKIN_MEDITATE_COOLDOWN ) );
	gain = IS_CLASS( ch, CLASS_WYRM ) ? 20 : acfg( ACFG_DRAGONKIN_MEDITATE_GAIN );
	ch->rage = UMIN( ch->rage + gain, essence_max );

	act( "You meditate, channeling draconic power within.", ch, NULL, NULL, TO_CHAR );
	act( "$n sits in meditation, scales faintly glowing.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Dragonbreath - Cone AoE breath attack, damage type based on attunement
 */
void do_dragonbreath( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char buf[MAX_STRING_LENGTH];
	int dam;
	int attune;
	int essence_max;
	const char *breath_types[] = { "fire", "frost", "lightning", "stone shards" };
	const char *breath_colors[] = { "#Rflames#n", "#Cfrost#n", "#Ylightning#n", "#ystone shards#n" };

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_BREATH] < 1 ) {
		send_to_char( "You haven't trained Draconic Breath yet. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_DRAGONBREATH_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	if ( ch->rage < acfg( ACFG_DRAGONKIN_DRAGONBREATH_ESSENCE_COST ) ) {
		sprintf( buf, "You need at least %d Essence to use dragonbreath.\n\r", acfg( ACFG_DRAGONKIN_DRAGONBREATH_ESSENCE_COST ) );
		send_to_char( buf, ch );
		return;
	}

	WAIT_STATE( ch, acfg( ACFG_DRAGONKIN_DRAGONBREATH_COOLDOWN ) );
	ch->mana -= acfg( ACFG_DRAGONKIN_DRAGONBREATH_MANA_COST );
	ch->rage -= acfg( ACFG_DRAGONKIN_DRAGONBREATH_ESSENCE_COST );
	if ( ch->rage < 0 ) ch->rage = 0;

	attune = ch->pcdata->powers[DRAGON_ATTUNEMENT];
	if ( attune < 0 || attune > 3 ) attune = 0;

	sprintf( buf, "You unleash a devastating cone of %s!", breath_colors[attune] );
	act( buf, ch, NULL, NULL, TO_CHAR );
	sprintf( buf, "$n unleashes a devastating cone of %s!", breath_colors[attune] );
	act( buf, ch, NULL, NULL, TO_ROOM );

	/* Build some Essence from the attack */
	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;
	ch->rage = UMIN( ch->rage + 5, essence_max );

	for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
		vch_next = vch->next_in_room;
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		dam = number_range( acfg( ACFG_DRAGONKIN_DRAGONBREATH_DAM_MIN ), acfg( ACFG_DRAGONKIN_DRAGONBREATH_DAM_MAX ) );
		dam += ch->rage * 2;
		dam += ch->pcdata->powers[DRAGON_TRAIN_BREATH] * 50;

		damage( ch, vch, dam, gsn_punch );
	}
	return;
}

/*
 * Searingblast - Single-target heavy elemental damage + DoT
 */
void do_searingblast( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	int dam;
	int attune;
	int essence_max;
	const char *blast_types[] = { "searing flames", "freezing cold", "crackling lightning", "crushing stone" };

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_BREATH] < 2 ) {
		send_to_char( "You need Draconic Breath level 2 for Searing Blast. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_SEARINGBLAST_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	if ( ch->rage < acfg( ACFG_DRAGONKIN_SEARINGBLAST_ESSENCE_COST ) ) {
		sprintf( buf, "You need at least %d Essence to use Searing Blast.\n\r", acfg( ACFG_DRAGONKIN_SEARINGBLAST_ESSENCE_COST ) );
		send_to_char( buf, ch );
		return;
	}

	WAIT_STATE( ch, acfg( ACFG_DRAGONKIN_SEARINGBLAST_COOLDOWN ) );
	ch->mana -= acfg( ACFG_DRAGONKIN_SEARINGBLAST_MANA_COST );
	ch->rage -= acfg( ACFG_DRAGONKIN_SEARINGBLAST_ESSENCE_COST );
	if ( ch->rage < 0 ) ch->rage = 0;

	attune = ch->pcdata->powers[DRAGON_ATTUNEMENT];
	if ( attune < 0 || attune > 3 ) attune = 0;

	dam = number_range( acfg( ACFG_DRAGONKIN_SEARINGBLAST_DAM_MIN ), acfg( ACFG_DRAGONKIN_SEARINGBLAST_DAM_MAX ) );
	dam += ch->rage * 3;
	dam += ch->pcdata->powers[DRAGON_TRAIN_BREATH] * 75;

	sprintf( buf, "You blast $N with %s!", blast_types[attune] );
	act( buf, ch, NULL, victim, TO_CHAR );
	sprintf( buf, "$n blasts you with %s!", blast_types[attune] );
	act( buf, ch, NULL, victim, TO_VICT );
	sprintf( buf, "$n blasts $N with %s!", blast_types[attune] );
	act( buf, ch, NULL, victim, TO_NOTVICT );

	/* Apply DoT - stack up to max */
	if ( ch->pcdata->powers[DRAGON_DOT_STACKS] < 5 ) {
		ch->pcdata->powers[DRAGON_DOT_STACKS]++;
		ch->pcdata->powers[DRAGON_DOT_TICKS] = acfg( ACFG_DRAGONKIN_SEARINGBLAST_DOT_DURATION );
		send_to_char( "The elemental energy lingers, burning your foe!\n\r", ch );
	}

	/* Build Essence */
	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;
	ch->rage = UMIN( ch->rage + 5, essence_max );

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Infernalstorm - Room-wide elemental devastation (high Essence cost)
 */
void do_infernalstorm( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char buf[MAX_STRING_LENGTH];
	int dam;
	int attune;
	int essence_max;
	const char *storm_types[] = { "firestorm", "blizzard", "lightning storm", "earthquake" };
	const char *storm_colors[] = { "#Rfirestorm#n", "#Cblizzard#n", "#Ylightning storm#n", "#yearthquake#n" };

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_BREATH] < 3 ) {
		send_to_char( "You need Draconic Breath level 3 for Infernal Storm. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_INFERNALSTORM_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	if ( ch->rage < acfg( ACFG_DRAGONKIN_INFERNALSTORM_ESSENCE_COST ) ) {
		sprintf( buf, "You need at least %d Essence to unleash Infernal Storm.\n\r", acfg( ACFG_DRAGONKIN_INFERNALSTORM_ESSENCE_COST ) );
		send_to_char( buf, ch );
		return;
	}

	WAIT_STATE( ch, acfg( ACFG_DRAGONKIN_INFERNALSTORM_COOLDOWN ) );
	ch->mana -= acfg( ACFG_DRAGONKIN_INFERNALSTORM_MANA_COST );
	ch->rage -= acfg( ACFG_DRAGONKIN_INFERNALSTORM_ESSENCE_COST );
	if ( ch->rage < 0 ) ch->rage = 0;

	attune = ch->pcdata->powers[DRAGON_ATTUNEMENT];
	if ( attune < 0 || attune > 3 ) attune = 0;

	sprintf( buf, "You call upon your draconic heritage, unleashing a devastating %s!", storm_colors[attune] );
	act( buf, ch, NULL, NULL, TO_CHAR );
	sprintf( buf, "$n calls upon draconic power, unleashing a devastating %s!", storm_colors[attune] );
	act( buf, ch, NULL, NULL, TO_ROOM );

	/* Build Essence from the destruction */
	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;
	ch->rage = UMIN( ch->rage + 10, essence_max );

	for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
		vch_next = vch->next_in_room;
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		dam = number_range( acfg( ACFG_DRAGONKIN_INFERNALSTORM_DAM_MIN ), acfg( ACFG_DRAGONKIN_INFERNALSTORM_DAM_MAX ) );
		dam += ch->rage * 4;
		dam += ch->pcdata->powers[DRAGON_TRAIN_BREATH] * 100;

		damage( ch, vch, dam, gsn_punch );
	}
	return;
}

/*
 * Scaleshield - Create damage absorption barrier
 */
void do_scaleshield( CHAR_DATA *ch, char *argument ) {
	int absorb_amount;

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_SCALES] < 1 ) {
		send_to_char( "You haven't trained Dragon Scales yet. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_SCALESHIELD] > 0 ) {
		send_to_char( "Your scaleshield is already active.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_SCALESHIELD_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= acfg( ACFG_DRAGONKIN_SCALESHIELD_MANA_COST );
	ch->pcdata->powers[DRAGON_SCALESHIELD] = acfg( ACFG_DRAGONKIN_SCALESHIELD_DURATION );

	/* Base absorb + bonus from training level */
	absorb_amount = acfg( ACFG_DRAGONKIN_SCALESHIELD_ABSORB );
	absorb_amount += ch->pcdata->powers[DRAGON_TRAIN_SCALES] * 500;
	ch->pcdata->stats[DRAGON_SCALESHIELD_HP] = absorb_amount;

	act( "Hardened dragon scales form a protective barrier around you!", ch, NULL, NULL, TO_CHAR );
	act( "Hardened dragon scales form a protective barrier around $n!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Dragonhide - Toggle: reduced damage taken, slight hitroll penalty
 */
void do_dragonhide( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_SCALES] < 2 ) {
		send_to_char( "You need Dragon Scales level 2 for Dragonhide. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	/* Toggle dragonhide on/off */
	if ( ch->pcdata->powers[DRAGON_DRAGONHIDE] ) {
		ch->pcdata->powers[DRAGON_DRAGONHIDE] = 0;
		act( "Your scales return to their normal hardness.", ch, NULL, NULL, TO_CHAR );
		act( "$n's scales lose their metallic sheen.", ch, NULL, NULL, TO_ROOM );
	} else {
		ch->pcdata->powers[DRAGON_DRAGONHIDE] = 1;
		act( "Your scales harden into impenetrable dragon hide!", ch, NULL, NULL, TO_CHAR );
		act( "$n's scales shimmer and harden with a metallic sheen!", ch, NULL, NULL, TO_ROOM );
	}
	return;
}

/*
 * Primalwarding - Group buff providing elemental resistance
 */
void do_primalwarding( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_SCALES] < 3 ) {
		send_to_char( "You need Dragon Scales level 3 for Primal Warding. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_PRIMALWARDING] > 0 ) {
		send_to_char( "You are already protected by primal warding.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_PRIMALWARDING_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= acfg( ACFG_DRAGONKIN_PRIMALWARDING_MANA_COST );
	ch->pcdata->powers[DRAGON_PRIMALWARDING] = acfg( ACFG_DRAGONKIN_PRIMALWARDING_DURATION );

	act( "You invoke the ancient warding of dragons, surrounding yourself with elemental protection!", ch, NULL, NULL, TO_CHAR );
	act( "$n invokes an ancient draconic ward, shimmering with elemental energy!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Dragonclaw - Enhanced melee attack + chance to apply attunement effect
 */
void do_dragonclaw( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	int dam;
	int attune;
	int essence_max;
	const char *claw_effects[] = { "searing", "freezing", "crackling", "crushing" };

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_MIGHT] < 1 ) {
		send_to_char( "You haven't trained Dragon Might yet. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_DRAGONCLAW_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, acfg( ACFG_DRAGONKIN_DRAGONCLAW_COOLDOWN ) );
	ch->mana -= acfg( ACFG_DRAGONKIN_DRAGONCLAW_MANA_COST );

	attune = ch->pcdata->powers[DRAGON_ATTUNEMENT];
	if ( attune < 0 || attune > 3 ) attune = 0;

	dam = number_range( acfg( ACFG_DRAGONKIN_DRAGONCLAW_DAM_MIN ), acfg( ACFG_DRAGONKIN_DRAGONCLAW_DAM_MAX ) );
	dam += ch->rage * 2;
	dam += ch->pcdata->powers[DRAGON_TRAIN_MIGHT] * 30;

	sprintf( buf, "You rake $N with %s dragon claws!", claw_effects[attune] );
	act( buf, ch, NULL, victim, TO_CHAR );
	sprintf( buf, "$n rakes you with %s dragon claws!", claw_effects[attune] );
	act( buf, ch, NULL, victim, TO_VICT );
	sprintf( buf, "$n rakes $N with %s dragon claws!", claw_effects[attune] );
	act( buf, ch, NULL, victim, TO_NOTVICT );

	/* Build Essence from combat */
	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;
	ch->rage = UMIN( ch->rage + 3, essence_max );

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Drakewings - Flight + movement speed buff
 */
void do_drakewings( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_MIGHT] < 2 ) {
		send_to_char( "You need Dragon Might level 2 for Drakewings. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_DRAKEWINGS] > 0 ) {
		send_to_char( "Your drakewings are already active.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_DRAKEWINGS_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= acfg( ACFG_DRAGONKIN_DRAKEWINGS_MANA_COST );
	ch->pcdata->powers[DRAGON_DRAKEWINGS] = acfg( ACFG_DRAGONKIN_DRAKEWINGS_DURATION );

	/* Grant flight */
	SET_BIT( ch->affected_by, AFF_FLYING );

	act( "Magnificent draconic wings sprout from your back!", ch, NULL, NULL, TO_CHAR );
	act( "Magnificent draconic wings sprout from $n's back!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Dragonrush - Charge attack with knockback + bonus damage
 */
void do_dragonrush( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	int dam;
	int essence_max;

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[DRAGON_TRAIN_MIGHT] < 3 ) {
		send_to_char( "You need Dragon Might level 3 for Dragonrush. See #Rdragontrain#n.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}

	if ( ch->mana < acfg( ACFG_DRAGONKIN_DRAGONRUSH_MANA_COST ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	if ( ch->rage < acfg( ACFG_DRAGONKIN_DRAGONRUSH_ESSENCE_COST ) ) {
		sprintf( buf, "You need at least %d Essence to use Dragonrush.\n\r", acfg( ACFG_DRAGONKIN_DRAGONRUSH_ESSENCE_COST ) );
		send_to_char( buf, ch );
		return;
	}

	WAIT_STATE( ch, acfg( ACFG_DRAGONKIN_DRAGONRUSH_COOLDOWN ) );
	ch->mana -= acfg( ACFG_DRAGONKIN_DRAGONRUSH_MANA_COST );
	ch->rage -= acfg( ACFG_DRAGONKIN_DRAGONRUSH_ESSENCE_COST );
	if ( ch->rage < 0 ) ch->rage = 0;

	dam = number_range( acfg( ACFG_DRAGONKIN_DRAGONRUSH_DAM_MIN ), acfg( ACFG_DRAGONKIN_DRAGONRUSH_DAM_MAX ) );
	dam += ch->rage * 3;
	dam += ch->pcdata->powers[DRAGON_TRAIN_MIGHT] * 60;

	act( "You charge forward with draconic fury, smashing into $N!", ch, NULL, victim, TO_CHAR );
	act( "$n charges forward with draconic fury, smashing into you!", ch, NULL, victim, TO_VICT );
	act( "$n charges forward with draconic fury, smashing into $N!", ch, NULL, victim, TO_NOTVICT );

	/* Build Essence from the charge */
	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;
	ch->rage = UMIN( ch->rage + 8, essence_max );

	/* Stun effect on victim */
	WAIT_STATE( victim, 4 );

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Dragontrain - Training command for Dragonkin abilities
 */
void do_dragontrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int cost;
	int *train_ptr = NULL;
	int max_level = 0;
	const char *tree_name = NULL;

	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "#x202~#x208[#n Dragonkin Training #x208]#x202~#n\n\r", ch );
		send_to_char( "Available training paths:\n\r", ch );
		sprintf( buf, "  #x208breath#n  - Draconic Breath (%d/3) - Offensive breath attacks\n\r", ch->pcdata->powers[DRAGON_TRAIN_BREATH] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x208scales#n  - Dragon Scales   (%d/3) - Defensive abilities\n\r", ch->pcdata->powers[DRAGON_TRAIN_SCALES] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x208might#n   - Dragon Might    (%d/3) - Combat enhancement\n\r", ch->pcdata->powers[DRAGON_TRAIN_MIGHT] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x208essence#n - Essence Mastery (%d/2) - Utility powers\n\r", ch->pcdata->powers[DRAGON_TRAIN_ESSENCE] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: dragontrain <path>\n\r", ch );
		return;
	}

	if ( !str_prefix( arg, "breath" ) ) {
		train_ptr = &ch->pcdata->powers[DRAGON_TRAIN_BREATH];
		max_level = 3;
		tree_name = "Draconic Breath";
	}
	else if ( !str_prefix( arg, "scales" ) ) {
		train_ptr = &ch->pcdata->powers[DRAGON_TRAIN_SCALES];
		max_level = 3;
		tree_name = "Dragon Scales";
	}
	else if ( !str_prefix( arg, "might" ) ) {
		train_ptr = &ch->pcdata->powers[DRAGON_TRAIN_MIGHT];
		max_level = 3;
		tree_name = "Dragon Might";
	}
	else if ( !str_prefix( arg, "essence" ) ) {
		train_ptr = &ch->pcdata->powers[DRAGON_TRAIN_ESSENCE];
		max_level = 2;
		tree_name = "Essence Mastery";
	}
	else {
		send_to_char( "Unknown training path. See #Rdragontrain#n for options.\n\r", ch );
		return;
	}

	if ( *train_ptr >= max_level ) {
		sprintf( buf, "You have already mastered %s.\n\r", tree_name );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *train_ptr + 1 ) * 40;

	if ( ch->practice < cost ) {
		sprintf( buf, "You need %d primal to train %s level %d.\n\r", cost, tree_name, *train_ptr + 1 );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	( *train_ptr )++;

	sprintf( buf, "You have trained %s to level %d.\n\r", tree_name, *train_ptr );
	send_to_char( buf, ch );
	return;
}

/*
 * Dragonarmor - Create class-specific armor
 */
void do_dragonarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_DRAGONKIN );
}

/*
 * Dragonkin tick update - called from update.c
 */
void update_dragonkin( CHAR_DATA *ch ) {
	int essence_max;

	if ( IS_NPC( ch ) )
		return;
	if ( !IS_CLASS( ch, CLASS_DRAGONKIN ) && !IS_CLASS( ch, CLASS_WYRM ) )
		return;

	essence_max = IS_CLASS( ch, CLASS_WYRM ) ? WYRM_ESSENCE_MAX : DRAGONKIN_ESSENCE_MAX;

	/* Combat: gain Essence */
	if ( ch->fighting != NULL ) {
		int gain = IS_CLASS( ch, CLASS_WYRM ) ? 4 : acfg( ACFG_DRAGONKIN_ESSENCE_COMBAT_GAIN );
		ch->rage = UMIN( ch->rage + gain, essence_max );
	}
	/* Out of combat: decay Essence */
	else if ( ch->rage > 0 ) {
		int decay = IS_CLASS( ch, CLASS_WYRM ) ? 1 : acfg( ACFG_DRAGONKIN_ESSENCE_DECAY_RATE );
		ch->rage = UMAX( ch->rage - decay, 0 );
	}

	/* Track peak Essence */
	if ( ch->rage > ch->pcdata->stats[DRAGON_ESSENCE_PEAK] )
		ch->pcdata->stats[DRAGON_ESSENCE_PEAK] = ch->rage;

	/* Decrement buff timers */
	if ( ch->pcdata->powers[DRAGON_SCALESHIELD] > 0 ) {
		ch->pcdata->powers[DRAGON_SCALESHIELD]--;
		if ( ch->pcdata->powers[DRAGON_SCALESHIELD] == 0 ) {
			ch->pcdata->stats[DRAGON_SCALESHIELD_HP] = 0;
			send_to_char( "Your scaleshield fades away.\n\r", ch );
		}
	}

	if ( ch->pcdata->powers[DRAGON_PRIMALWARDING] > 0 ) {
		ch->pcdata->powers[DRAGON_PRIMALWARDING]--;
		if ( ch->pcdata->powers[DRAGON_PRIMALWARDING] == 0 )
			send_to_char( "Your primal warding fades.\n\r", ch );
	}

	if ( ch->pcdata->powers[DRAGON_DRAKEWINGS] > 0 ) {
		ch->pcdata->powers[DRAGON_DRAKEWINGS]--;
		if ( ch->pcdata->powers[DRAGON_DRAKEWINGS] == 0 ) {
			REMOVE_BIT( ch->affected_by, AFF_FLYING );
			send_to_char( "Your drakewings dissipate.\n\r", ch );
		}
	}

	/* Process DoT */
	if ( ch->pcdata->powers[DRAGON_DOT_TICKS] > 0 ) {
		ch->pcdata->powers[DRAGON_DOT_TICKS]--;
		/* DoT damage would be applied to target, not self - this is tracked on victim */
	}

	return;
}
