#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "../db/db_game.h"

BALANCE_DATA balance;

/*
 * Map of balance keys to struct fields.
 * Used by load, save, and the admin command to avoid 80+ if/else chains.
 * Not static — also referenced by db_game.c for load/save.
 */
typedef struct balance_entry {
	const char *key;
	int        *field;
} balance_entry_t;

balance_entry_t balance_map[] = {
	/* Global combat */
	{ "base_damcap",              &balance.base_damcap },
	{ "builder_damcap",           &balance.builder_damcap },
	{ "pvp_damage_divisor",       &balance.pvp_damage_divisor },
	{ "sleep_damage_multiplier",  &balance.sleep_damage_multiplier },

	/* Upgrade level bonuses */
	{ "upgrade_damcap_per_level", &balance.upgrade_damcap_per_level },
	{ "upgrade_dmg_1",            &balance.upgrade_dmg[0] },
	{ "upgrade_dmg_2",            &balance.upgrade_dmg[1] },
	{ "upgrade_dmg_3",            &balance.upgrade_dmg[2] },
	{ "upgrade_dmg_4",            &balance.upgrade_dmg[3] },
	{ "upgrade_dmg_5",            &balance.upgrade_dmg[4] },

	/* Generation bonuses */
	{ "gen_damcap_0",             &balance.gen_damcap[0] },
	{ "gen_damcap_1",             &balance.gen_damcap[1] },
	{ "gen_damcap_2",             &balance.gen_damcap[2] },
	{ "gen_damcap_3",             &balance.gen_damcap[3] },
	{ "gen_damcap_4",             &balance.gen_damcap[4] },

	/* Per-class damage caps */
	{ "damcap_mage",              &balance.damcap_mage },
	{ "damcap_mage_beast",        &balance.damcap_mage_beast },
	{ "damcap_tanarri_per_rank",  &balance.damcap_tanarri_per_rank },
	{ "damcap_lich_base",         &balance.damcap_lich_base },
	{ "damcap_lich_lore",         &balance.damcap_lich_lore },
	{ "damcap_angel_per_power",   &balance.damcap_angel_per_power },
	{ "damcap_shape_base",        &balance.damcap_shape_base },
	{ "damcap_shape_tiger",       &balance.damcap_shape_tiger },
	{ "damcap_shape_hydra",       &balance.damcap_shape_hydra },
	{ "damcap_shape_bull",        &balance.damcap_shape_bull },
	{ "damcap_shape_faerie",      &balance.damcap_shape_faerie },
	{ "damcap_drow_base",         &balance.damcap_drow_base },
	{ "damcap_drow_hate",         &balance.damcap_drow_hate },
	{ "damcap_drow_form",         &balance.damcap_drow_form },
	{ "damcap_demon_base",        &balance.damcap_demon_base },
	{ "damcap_demon_soul_mult",   &balance.damcap_demon_soul_mult },
	{ "damcap_demon_soul_cap",    &balance.damcap_demon_soul_cap },
	{ "damcap_demon_hell",        &balance.damcap_demon_hell },
	{ "damcap_demon_power_mult",  &balance.damcap_demon_power_mult },
	{ "damcap_droid_per_limb",    &balance.damcap_droid_per_limb },
	{ "damcap_monk_combat_mult",  &balance.damcap_monk_combat_mult },
	{ "damcap_monk_chi_mult",     &balance.damcap_monk_chi_mult },
	{ "damcap_vamp_rage_mult",    &balance.damcap_vamp_rage_mult },
	{ "damcap_vamp_pote_mult",    &balance.damcap_vamp_pote_mult },
	{ "damcap_vamp_trueblood",    &balance.damcap_vamp_trueblood },
	{ "damcap_vamp_lamagra",      &balance.damcap_vamp_lamagra },
	{ "damcap_vamp_methuselah",   &balance.damcap_vamp_methuselah },
	{ "damcap_vamp_elder",        &balance.damcap_vamp_elder },
	{ "damcap_vamp_ancilla",      &balance.damcap_vamp_ancilla },
	{ "damcap_ninja_rage_mult",   &balance.damcap_ninja_rage_mult },
	{ "damcap_ninja_chikyu_high", &balance.damcap_ninja_chikyu_high },
	{ "damcap_ninja_chikyu_low",  &balance.damcap_ninja_chikyu_low },
	{ "damcap_ww_rage_mult",      &balance.damcap_ww_rage_mult },
	{ "damcap_ww_high_rage",      &balance.damcap_ww_high_rage },
	{ "damcap_ww_pain",           &balance.damcap_ww_pain },
	{ "damcap_uk_wpn_mult",       &balance.damcap_uk_wpn_mult },
	{ "damcap_samurai_per_wpn",   &balance.damcap_samurai_per_wpn },
	{ "damcap_samurai_base",      &balance.damcap_samurai_base },
	{ "damcap_dirgesinger_base",      &balance.damcap_dirgesinger_base },
	{ "damcap_dirgesinger_res_mult",  &balance.damcap_dirgesinger_res_mult },
	{ "damcap_dirgesinger_battlehymn",&balance.damcap_dirgesinger_battlehymn },
	{ "damcap_siren_res_mult",        &balance.damcap_siren_res_mult },
	{ "damcap_siren_echoshield",      &balance.damcap_siren_echoshield },
	{ "damcap_artifact",          &balance.damcap_artifact },

	/* Stance damcap bonuses */
	{ "damcap_stance_bull",       &balance.damcap_stance_bull },
	{ "damcap_stance_dragon",     &balance.damcap_stance_dragon },
	{ "damcap_stance_wolf",       &balance.damcap_stance_wolf },
	{ "damcap_stance_tiger",      &balance.damcap_stance_tiger },
	{ "damcap_superstance_3",     &balance.damcap_superstance[0] },
	{ "damcap_superstance_2",     &balance.damcap_superstance[1] },
	{ "damcap_superstance_1",     &balance.damcap_superstance[2] },
	{ "damcap_rev_superstance_3", &balance.damcap_rev_superstance[0] },
	{ "damcap_rev_superstance_2", &balance.damcap_rev_superstance[1] },
	{ "damcap_rev_superstance_1", &balance.damcap_rev_superstance[2] },
	{ "damcap_stance_crab",       &balance.damcap_stance_crab },
	{ "damcap_stance_dragon_def", &balance.damcap_stance_dragon_def },
	{ "damcap_stance_swallow",    &balance.damcap_stance_swallow },
	{ "damcap_vamp_fort_mult",    &balance.damcap_vamp_fort_mult },

	/* Weapon skill caps per class */
	{ "wpn_cap_default",          &balance.wpn_cap_default },
	{ "wpn_cap_drow",             &balance.wpn_cap_drow },
	{ "wpn_cap_werewolf",         &balance.wpn_cap_werewolf },
	{ "wpn_cap_monk",             &balance.wpn_cap_monk },
	{ "wpn_cap_angel",            &balance.wpn_cap_angel },
	{ "wpn_cap_lich",             &balance.wpn_cap_lich },
	{ "wpn_cap_droid",            &balance.wpn_cap_droid },
	{ "wpn_cap_samurai",          &balance.wpn_cap_samurai },
	{ "wpn_cap_shape",            &balance.wpn_cap_shape },
	{ "wpn_cap_hard_cap",         &balance.wpn_cap_hard_cap },
	{ "wpn_cap_low_level",        &balance.wpn_cap_low_level },
	{ "wpn_gen2_bonus",           &balance.wpn_gen2_bonus },
	{ "wpn_gen1_bonus",           &balance.wpn_gen1_bonus },

	/* Damage multipliers (percentage x100) */
	{ "dmg_mult_tanarri_might",   &balance.dmg_mult_tanarri_might },
	{ "dmg_mult_demon_might",     &balance.dmg_mult_demon_might },
	{ "dmg_mult_uk_vs_shape",     &balance.dmg_mult_uk_vs_shape },
	{ "dmg_mult_demon_strongarms",&balance.dmg_mult_demon_strongarms },
	{ "dmg_mult_ninja_belt_1",    &balance.dmg_mult_ninja_belt[0] },
	{ "dmg_mult_ninja_belt_2",    &balance.dmg_mult_ninja_belt[1] },
	{ "dmg_mult_ninja_belt_3",    &balance.dmg_mult_ninja_belt[2] },
	{ "dmg_mult_ninja_belt_4",    &balance.dmg_mult_ninja_belt[3] },
	{ "dmg_mult_ninja_belt_5",    &balance.dmg_mult_ninja_belt[4] },
	{ "dmg_mult_ninja_belt_6",    &balance.dmg_mult_ninja_belt[5] },
	{ "dmg_mult_ninja_belt_7",    &balance.dmg_mult_ninja_belt[6] },
	{ "dmg_mult_ninja_belt_8",    &balance.dmg_mult_ninja_belt[7] },
	{ "dmg_mult_ninja_belt_9",    &balance.dmg_mult_ninja_belt[8] },
	{ "dmg_mult_ninja_belt_10",   &balance.dmg_mult_ninja_belt[9] },

	/* Weapon skill damage formula */
	{ "wpn_dam_skill_cap",        &balance.wpn_dam_skill_cap },
	{ "wpn_dam_divisor",          &balance.wpn_dam_divisor },

	{ NULL, NULL }
};

void load_balance( void ) {
	/* Set defaults */
	balance.base_damcap             = 1000;
	balance.builder_damcap          = 30000;
	balance.pvp_damage_divisor      = 2;
	balance.sleep_damage_multiplier = 2;

	balance.upgrade_damcap_per_level = 500;
	balance.upgrade_dmg[0] = 105;
	balance.upgrade_dmg[1] = 110;
	balance.upgrade_dmg[2] = 115;
	balance.upgrade_dmg[3] = 120;
	balance.upgrade_dmg[4] = 125;

	balance.gen_damcap[0] = 250;
	balance.gen_damcap[1] = 200;
	balance.gen_damcap[2] = 150;
	balance.gen_damcap[3] = 100;
	balance.gen_damcap[4] = 50;

	balance.damcap_mage              = 500;
	balance.damcap_mage_beast        = 750;
	balance.damcap_tanarri_per_rank  = 375;
	balance.damcap_lich_base         = 450;
	balance.damcap_lich_lore         = 350;
	balance.damcap_angel_per_power   = 125;
	balance.damcap_shape_base        = 800;
	balance.damcap_shape_tiger       = 325;
	balance.damcap_shape_hydra       = 350;
	balance.damcap_shape_bull        = 300;
	balance.damcap_shape_faerie      = 275;
	balance.damcap_drow_base         = 500;
	balance.damcap_drow_hate         = 650;
	balance.damcap_drow_form         = 650;
	balance.damcap_demon_base        = 500;
	balance.damcap_demon_soul_mult   = 70;
	balance.damcap_demon_soul_cap    = 350;
	balance.damcap_demon_hell        = 6000;
	balance.damcap_demon_power_mult  = 2;
	balance.damcap_droid_per_limb    = 450;
	balance.damcap_monk_combat_mult  = 100;
	balance.damcap_monk_chi_mult     = 200;
	balance.damcap_vamp_rage_mult    = 5;
	balance.damcap_vamp_pote_mult    = 50;
	balance.damcap_vamp_trueblood    = 600;
	balance.damcap_vamp_lamagra      = 500;
	balance.damcap_vamp_methuselah   = 400;
	balance.damcap_vamp_elder        = 250;
	balance.damcap_vamp_ancilla      = 100;
	balance.damcap_ninja_rage_mult   = 4;
	balance.damcap_ninja_chikyu_high = 500;
	balance.damcap_ninja_chikyu_low  = 500;
	balance.damcap_ww_rage_mult      = 1;
	balance.damcap_ww_high_rage      = 400;
	balance.damcap_ww_pain           = 750;
	balance.damcap_uk_wpn_mult       = 275;
	balance.damcap_samurai_per_wpn   = 175;
	balance.damcap_samurai_base      = 375;
	balance.damcap_dirgesinger_base      = 200;
	balance.damcap_dirgesinger_res_mult  = 7;
	balance.damcap_dirgesinger_battlehymn = 200;
	balance.damcap_siren_res_mult        = 7;
	balance.damcap_siren_echoshield      = 150;
	balance.damcap_artifact          = 500;

	balance.damcap_stance_bull       = 200;
	balance.damcap_stance_dragon     = 250;
	balance.damcap_stance_wolf       = 250;
	balance.damcap_stance_tiger      = 200;
	balance.damcap_superstance[0]    = 550;
	balance.damcap_superstance[1]    = 400;
	balance.damcap_superstance[2]    = 250;
	balance.damcap_rev_superstance[0] = 550;
	balance.damcap_rev_superstance[1] = 400;
	balance.damcap_rev_superstance[2] = 250;
	balance.damcap_stance_crab       = 250;
	balance.damcap_stance_dragon_def = 250;
	balance.damcap_stance_swallow    = 250;
	balance.damcap_vamp_fort_mult    = 50;

	balance.wpn_cap_default  = 200;
	balance.wpn_cap_drow     = 300;
	balance.wpn_cap_werewolf = 350;
	balance.wpn_cap_monk     = 800;
	balance.wpn_cap_angel    = 500;
	balance.wpn_cap_lich     = 350;
	balance.wpn_cap_droid    = 450;
	balance.wpn_cap_samurai  = 1000;
	balance.wpn_cap_shape    = 400;
	balance.wpn_cap_hard_cap = 1100;
	balance.wpn_cap_low_level = 200;
	balance.wpn_gen2_bonus   = 10;
	balance.wpn_gen1_bonus   = 20;

	balance.dmg_mult_tanarri_might    = 150;
	balance.dmg_mult_demon_might      = 110;
	balance.dmg_mult_uk_vs_shape      = 120;
	balance.dmg_mult_demon_strongarms = 120;
	balance.dmg_mult_ninja_belt[0] = 110;
	balance.dmg_mult_ninja_belt[1] = 120;
	balance.dmg_mult_ninja_belt[2] = 130;
	balance.dmg_mult_ninja_belt[3] = 140;
	balance.dmg_mult_ninja_belt[4] = 150;
	balance.dmg_mult_ninja_belt[5] = 160;
	balance.dmg_mult_ninja_belt[6] = 170;
	balance.dmg_mult_ninja_belt[7] = 180;
	balance.dmg_mult_ninja_belt[8] = 190;
	balance.dmg_mult_ninja_belt[9] = 200;

	balance.wpn_dam_skill_cap = 350;
	balance.wpn_dam_divisor   = 60;

	/* Load from game.db (overwrites defaults for any keys present) */
	db_game_load_balance();
}

void save_balance( void ) {
	db_game_save_balance();
}

void do_balance( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int i;

	argument = one_argument( argument, arg );
	one_argument( argument, arg2 );

	if ( arg[0] == '\0' ) {
		send_to_char( "Combat Balance Configuration:\n\r", ch );
		send_to_char( "----------------------------\n\r", ch );

		for ( i = 0; balance_map[i].key != NULL; i++ ) {
			snprintf( buf, sizeof( buf ), "  %-28s %d\n\r",
				balance_map[i].key, *balance_map[i].field );
			send_to_char( buf, ch );
		}

		send_to_char( "\n\rSyntax: balance <key> <value>\n\r", ch );
		send_to_char( "        balance reload\n\r", ch );
		return;
	}

	if ( !str_cmp( arg, "reload" ) ) {
		load_balance();
		send_to_char( "Balance configuration reloaded from database.\n\r", ch );
		return;
	}

	/* Find the key */
	for ( i = 0; balance_map[i].key != NULL; i++ ) {
		if ( !str_cmp( arg, balance_map[i].key ) )
			break;
	}

	if ( balance_map[i].key == NULL ) {
		send_to_char( "Unknown balance key.  Type 'balance' for a list.\n\r", ch );
		return;
	}

	if ( arg2[0] == '\0' ) {
		snprintf( buf, sizeof( buf ), "%s = %d\n\r",
			balance_map[i].key, *balance_map[i].field );
		send_to_char( buf, ch );
		return;
	}

	int old_val = *balance_map[i].field;
	*balance_map[i].field = atoi( arg2 );
	save_balance();

	snprintf( buf, sizeof( buf ), "%s changed from %d to %d.\n\r",
		balance_map[i].key, old_val, *balance_map[i].field );
	send_to_char( buf, ch );
}
