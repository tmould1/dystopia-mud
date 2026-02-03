/***************************************************************************
 *  ability_config.c - Per-ability balance configuration
 *
 *  Stores per-ability tuning constants (cooldowns, costs, damage, etc.)
 *  in an ability_config table in game.db.  Values are loaded at boot and
 *  can be viewed/modified at runtime via the 'ability' admin command.
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "ability_config.h"
#include "../db/db_game.h"


/*
 * Master table of all per-ability balance constants.
 * Keys use dotted notation: <class>.<ability>.<param>
 * Format: { "key", current_value, default_value }
 */
static acfg_entry_t acfg_table[] = {

	/* ================================================================
	 * VAMPIRE  (vamp.c)
	 * ================================================================ */
	{ "vampire.vampirearmor.practice_cost",       60,       60 },
	{ "vampire.preserve.level_req",                2,        2 },
	{ "vampire.spiritguard.level_req",             4,        4 },
	{ "vampire.spiritgate.level_req",              3,        3 },
	{ "vampire.spiritgate.blood_cost_check",      66,       66 },
	{ "vampire.spiritgate.blood_cost",            65,       65 },
	{ "vampire.purification.level_req",            7,        7 },
	{ "vampire.purification.move_cost",         5000,     5000 },
	{ "vampire.purification.cooldown_base",       30,       30 },
	{ "vampire.scream.level_req",                  1,        1 },
	{ "vampire.scream.blood_cost_check",          50,       50 },
	{ "vampire.testemb.cooldown",                 15,       15 },
	{ "vampire.conceal.level_req",                 5,        5 },
	{ "vampire.fear.level_req",                    2,        2 },
	{ "vampire.fear.cooldown",                    16,       16 },
	{ "vampire.fear.pc_level_req",                 5,        5 },
	{ "vampire.fear.pc_fail_range",                4,        4 },
	{ "vampire.vtwist.level_req",                  5,        5 },
	{ "vampire.dub.level_req",                     4,        4 },
	{ "vampire.sharpen.level_req",                 7,        7 },
	{ "vampire.gourge.level_req",                  8,        8 },
	{ "vampire.gourge.mob_level_max",             30,       30 },
	{ "vampire.gourge.cooldown",                  15,       15 },
	{ "vampire.gourge.blood_gain_min",           100,      100 },
	{ "vampire.gourge.blood_gain_max",           200,      200 },
	{ "vampire.bloodwater.level_req",              5,        5 },
	{ "vampire.bloodwater.damage_npc",          2000,     2000 },
	{ "vampire.bloodwater.damage_pc_min",        300,      300 },
	{ "vampire.bloodwater.damage_pc_max",        600,      600 },
	{ "vampire.bloodwater.cooldown_nonvamp",      12,       12 },
	{ "vampire.bloodwater.cooldown_vamp",          6,        6 },
	{ "vampire.bloodwater.vamp_blood_drain",     150,      150 },
	{ "vampire.bloodwater.chain_chance_denom",     4,        4 },
	{ "vampire.bloodwater.chain_victim_min_hp",  100,      100 },
	{ "vampire.spew.level_req",                    6,        6 },
	{ "vampire.spew.blood_cost_check",            20,       20 },
	{ "vampire.spew.blood_cost_min",              18,       18 },
	{ "vampire.spew.blood_cost_max",              22,       22 },
	{ "vampire.spew.cooldown",                    12,       12 },
	{ "vampire.vampdarkness.level_req",            6,        6 },
	{ "vampire.vampdarkness.blood_cost",         300,      300 },
	{ "vampire.dragonform.level_req",              4,        4 },
	{ "vampire.dragonform.blood_cost_check",     100,      100 },
	{ "vampire.dragonform.blood_cost_min",       200,      200 },
	{ "vampire.dragonform.blood_cost_max",       400,      400 },
	{ "vampire.dragonform.damroll_bonus",        100,      100 },
	{ "vampire.dragonform.hitroll_bonus",        100,      100 },
	{ "vampire.obj.level_req",                    10,       10 },
	{ "vampire.baal.level_req",                    5,        5 },
	{ "vampire.baal.cooldown",                    20,       20 },
	{ "vampire.baal.disc_bonus",                   2,        2 },
	{ "vampire.facade.level_req",                  6,        6 },
	{ "vampire.wall.level_req",                    7,        7 },
	{ "vampire.wall.blood_cost",                  50,       50 },
	{ "vampire.wall.cooldown",                    25,       25 },
	{ "vampire.wall.timer",                        3,        3 },
	{ "vampire.inferno.level_req",                 6,        6 },
	{ "vampire.inferno.blood_cost",              100,      100 },
	{ "vampire.zombie.level_req",                  5,        5 },
	{ "vampire.zombie.max_followers",              5,        5 },
	{ "vampire.zombie.charm_duration",           666,      666 },
	{ "vampire.zombie.cooldown",                  10,       10 },
	{ "vampire.fleshcraft.level_req",              2,        2 },
	{ "vampire.fleshcraft.blood_cost_check",      40,       40 },
	{ "vampire.fleshcraft.blood_cost_min",        30,       30 },
	{ "vampire.fleshcraft.blood_cost_max",        40,       40 },
	{ "vampire.entrance.level_req",                3,        3 },
	{ "vampire.entrance.cooldown",                12,       12 },
	{ "vampire.tendrils.level_req",                4,        4 },
	{ "vampire.tendrils.cooldown",                12,       12 },
	{ "vampire.lamprey.level_req",                 5,        5 },
	{ "vampire.lamprey.cooldown",                  5,        5 },
	{ "vampire.lamprey.dam_bonus_min",             1,        1 },
	{ "vampire.lamprey.dam_bonus_max",            30,       30 },
	{ "vampire.lamprey.blood_gain_min",           40,       40 },
	{ "vampire.lamprey.blood_gain_max",           50,       50 },
	{ "vampire.assassinate.level_req",             4,        4 },
	{ "vampire.assassinate.base_chance",           5,        5 },
	{ "vampire.assassinate.cooldown",             15,       15 },
	{ "vampire.assassinate.dam_bonus_min",         1,        1 },
	{ "vampire.assassinate.dam_bonus_max",        20,       20 },
	{ "vampire.mindblast.level_req",               2,        2 },
	{ "vampire.mindblast.cooldown",               12,       12 },
	{ "vampire.mindblast.success_chance",         50,       50 },
	{ "vampire.tongue.level_req",                  4,        4 },
	{ "vampire.tongue.cooldown",                   5,        5 },
	{ "vampire.tongue.dam_bonus_min",              1,        1 },
	{ "vampire.tongue.dam_bonus_max",             30,       30 },
	{ "vampire.objmask.level_req",                 5,        5 },
	{ "vampire.objmask.blood_cost_check",         50,       50 },
	{ "vampire.objmask.blood_cost_min",           40,       40 },
	{ "vampire.objmask.blood_cost_max",           50,       50 },
	{ "vampire.mirror.level_req",                  1,        1 },
	{ "vampire.mirror.blood_cost",                20,       20 },
	{ "vampire.mirror.max_followers",              4,        4 },
	{ "vampire.mirror.clone_hp",                2000,     2000 },
	{ "vampire.control.level_req",                 4,        4 },
	{ "vampire.formillusion.level_req",            2,        2 },
	{ "vampire.formillusion.blood_cost",          30,       30 },
	{ "vampire.formillusion.max_followers",        4,        4 },
	{ "vampire.formillusion.clone_level",        200,      200 },
	{ "vampire.unveil.level_req",                  3,        3 },
	{ "vampire.astralwalk.level_req",              4,        4 },
	{ "vampire.hagswrinkles.level_req",            1,        1 },
	{ "vampire.hagswrinkles.blood_cost_check",    40,       40 },
	{ "vampire.hagswrinkles.blood_cost_min",      30,       30 },
	{ "vampire.hagswrinkles.blood_cost_max",      40,       40 },
	{ "vampire.gate.level_req",                    3,        3 },
	{ "vampire.gate.portal_timer",                 5,        5 },
	{ "vampire.pigeon.level_req",                  3,        3 },
	{ "vampire.bloodagony.level_req",              3,        3 },
	{ "vampire.diablerise.cooldown",              15,       15 },
	{ "vampire.embrace.cooldown",                 15,       15 },
	{ "vampire.withering.level_req",               4,        4 },
	{ "vampire.withering.cooldown",               35,       35 },
	{ "vampire.withering.dismember_chance",       15,       15 },
	{ "vampire.infirmity.level_req",               2,        2 },
	{ "vampire.infirmity.cooldown",               12,       12 },
	{ "vampire.guardian.level_req",                1,        1 },
	{ "vampire.guardian.max_followers",            5,        5 },
	{ "vampire.guardian.mob_hp",                5000,     5000 },
	{ "vampire.guardian.mob_hitroll",             50,       50 },
	{ "vampire.guardian.mob_damroll",             50,       50 },
	{ "vampire.guardian.mob_armor",              300,      300 },
	{ "vampire.guardian.charm_duration",         666,      666 },
	{ "vampire.servant.level_req",                 8,        8 },
	{ "vampire.servant.max_followers",             5,        5 },
	{ "vampire.servant.mob_hp",               30000,    30000 },
	{ "vampire.servant.mob_hitroll",             100,      100 },
	{ "vampire.servant.mob_damroll",             100,      100 },
	{ "vampire.servant.mob_armor",               700,      700 },
	{ "vampire.servant.charm_duration",          666,      666 },
	{ "vampire.beckon.level_req",                  1,        1 },
	{ "vampire.beckon.max_followers",              4,        4 },
	{ "vampire.spit.level_req",                    1,        1 },
	{ "vampire.spit.blood_cost",                   5,        5 },
	{ "vampire.spit.cooldown",                    12,       12 },
	{ "vampire.spit.dam_bonus_min",                1,        1 },
	{ "vampire.spit.dam_bonus_max",               30,       30 },
	{ "vampire.scales.level_req",                  5,        5 },
	{ "vampire.scales.armor_bonus",              100,      100 },
	{ "vampire.cserpent.level_req",                4,        4 },
	{ "vampire.cserpent.max_followers",            5,        5 },
	{ "vampire.illusion.level_req",                1,        1 },
	{ "vampire.illusion.max_followers",            5,        5 },
	{ "vampire.bloodwall.level_req",               2,        2 },
	{ "vampire.bloodwall.cooldown",               25,       25 },
	{ "vampire.bloodwall.max_blood_input",         5,        5 },
	{ "vampire.bloodrod.level_req",                2,        2 },
	{ "vampire.bloodrod.practice_cost",           10,       10 },
	{ "vampire.shadowgaze.level_req",             10,       10 },
	{ "vampire.shadowgaze.cooldown",               8,        8 },
	{ "vampire.grab.level_req",                    8,        8 },
	{ "vampire.grab.cooldown",                     8,        8 },
	{ "vampire.share.level_req",                   4,        4 },
	{ "vampire.share.blood_cost",                 25,       25 },
	{ "vampire.frenzy.level_req",                  5,        5 },
	{ "vampire.frenzy.rage_gain_min",             20,       20 },
	{ "vampire.frenzy.rage_gain_max",             30,       30 },
	{ "vampire.frenzy.cooldown",                  12,       12 },
	{ "vampire.shroud.level_req",                  1,        1 },
	{ "vampire.summon.level_req",                  4,        4 },
	{ "vampire.summon.cooldown",                  10,       10 },
	{ "vampire.drain.level_req",                   5,        5 },
	{ "vampire.drain.victim_min_hp",             500,      500 },
	{ "vampire.drain.npc_heal_min",              450,      450 },
	{ "vampire.drain.npc_heal_max",              550,      550 },
	{ "vampire.drain.pc_heal_min",               350,      350 },
	{ "vampire.drain.pc_heal_max",               450,      450 },
	{ "vampire.drain.max_overheal",             1000,     1000 },
	{ "vampire.drain.cooldown",                   12,       12 },
	{ "vampire.flamehands.level_req",              5,        5 },
	{ "vampire.cauldron.level_req",                2,        2 },
	{ "vampire.cauldron.max_blood_input",        200,      200 },
	{ "vampire.cauldron.cooldown",                12,       12 },
	{ "vampire.bonemod.level_req",                 3,        3 },

	/* ================================================================
	 * CLAN  (clan.c)
	 * ================================================================ */
	{ "clan.smother.spark_chance",                98,       98 },
	{ "clan.coil.level_req",                       8,        8 },
	{ "clan.tide.level_req",                       5,        5 },
	{ "clan.tide.primal_cost",                    10,       10 },
	{ "clan.flash.level_req",                      9,        9 },
	{ "clan.flash.blood_cost",                   200,      200 },
	{ "clan.flash.cele_bonus",                     2,        2 },
	{ "clan.death.level_req",                      5,        5 },
	{ "clan.death.blood_cost",                   300,      300 },
	{ "clan.death.silence_duration",              10,       10 },
	{ "clan.acid.level_req",                       4,        4 },
	{ "clan.acid.blood_req",                     500,      500 },
	{ "clan.forget.level_req",                     8,        8 },
	{ "clan.forget.blood_cost",                  250,      250 },
	{ "clan.forget.primal_cost",                  25,       25 },
	{ "clan.forget.cooldown",                     12,       12 },
	{ "clan.forget.success_chance",               10,       10 },
	{ "clan.rot.level_req",                        2,        2 },
	{ "clan.rot.cooldown",                        12,       12 },
	{ "clan.rot.success_chance",                  50,       50 },
	{ "clan.dragon.level_req",                     8,        8 },
	{ "clan.dragon.blood_cost",                    6,        6 },
	{ "clan.dragon.cooldown",                     12,       12 },
	{ "clan.awe.level_req",                        1,        1 },
	{ "clan.plasma.level_req",                     5,        5 },
	{ "clan.taste.level_req",                      1,        1 },
	{ "clan.shadowstep.level_req",                 4,        4 },
	{ "clan.earthmeld.level_req",                  4,        4 },
	{ "clan.serenity.level_req",                   2,        2 },
	{ "clan.serenity.cooldown",                   12,       12 },
	{ "clan.theft.level_req",                      4,        4 },
	{ "clan.theft.blood_gain_min",                10,       10 },
	{ "clan.theft.blood_gain_max",                15,       15 },
	{ "clan.theft.mob_drain_min",                 30,       30 },
	{ "clan.theft.mob_drain_max",                 40,       40 },
	{ "clan.theft.acid_damage",                  300,      300 },
	{ "clan.theft.acid_blood_drain",              30,       30 },
	{ "clan.theft.player_drain_min",              30,       30 },
	{ "clan.theft.player_drain_max",              40,       40 },
	{ "clan.demonform.damroll_bonus",            200,      200 },
	{ "clan.demonform.hitroll_bonus",            200,      200 },
	{ "clan.demonform.armor_bonus",              300,      300 },
	{ "clan.demonform.mod_str",                   15,       15 },
	{ "clan.demonform.mod_dex",                   15,       15 },
	{ "clan.zuloform.level_req",                   2,        2 },
	{ "clan.zuloform.blood_req",                 200,      200 },
	{ "clan.zuloform.blood_cost_min",            100,      100 },
	{ "clan.zuloform.blood_cost_max",            200,      200 },
	{ "clan.zuloform.damroll_bonus",             150,      150 },
	{ "clan.zuloform.hitroll_bonus",             150,      150 },
	{ "clan.zuloform.mod_str",                    15,       15 },
	{ "clan.zuloform.mod_dex",                    15,       15 },
	{ "clan.claws.level_req_werewolf",             1,        1 },
	{ "clan.claws.level_req_demon",                1,        1 },
	{ "clan.claws.level_req_vampire",              2,        2 },
	{ "clan.fangs.level_req_werewolf",             2,        2 },
	{ "clan.fangs.level_req_demon",                2,        2 },
	{ "clan.nightsight.level_req_werewolf",        1,        1 },
	{ "clan.nightsight.level_req_monk",            1,        1 },
	{ "clan.nightsight.level_req_vampire_prot",    1,        1 },
	{ "clan.nightsight.level_req_vampire_obte",    3,        3 },
	{ "clan.inconnu.exp_cost",               1000000,  1000000 },
	{ "clan.shadowsight.level_req_werewolf",       2,        2 },
	{ "clan.shadowsight.level_req_monk",           2,        2 },
	{ "clan.shadowsight.level_req_vampire",        2,        2 },
	{ "clan.shadowsight.blood_req",               10,       10 },
	{ "clan.shadowsight.blood_cost_min",           5,        5 },
	{ "clan.shadowsight.blood_cost_max",          10,       10 },
	{ "clan.mask.level_req_vampire",               2,        2 },
	{ "clan.mask.level_req_shapeshifter",          4,        4 },
	{ "clan.mask.blood_req",                      40,       40 },
	{ "clan.mask.blood_cost_min",                 30,       30 },
	{ "clan.mask.blood_cost_max",                 40,       40 },
	{ "clan.change.level_req",                     3,        3 },
	{ "clan.change_bat.blood_req",                50,       50 },
	{ "clan.change_bat.blood_cost_min",           40,       40 },
	{ "clan.change_bat.blood_cost_max",           50,       50 },
	{ "clan.change_wolf.blood_req",               50,       50 },
	{ "clan.change_wolf.blood_cost_min",          40,       40 },
	{ "clan.change_wolf.blood_cost_max",          50,       50 },
	{ "clan.change_wolf.mod_str",                 10,       10 },
	{ "clan.change_mist.blood_req",               50,       50 },
	{ "clan.change_mist.blood_cost_min",          40,       40 },
	{ "clan.change_mist.blood_cost_max",          50,       50 },
	{ "clan.clandisc.max_level",                  10,       10 },
	{ "clan.clandisc.fortitude_max",              12,       12 },
	{ "clan.shadowplane.level_req_vampire",        3,        3 },
	{ "clan.shadowplane.level_req_werewolf",       3,        3 },
	{ "clan.shadowplane.blood_req",               75,       75 },
	{ "clan.shadowplane.blood_cost_min",          65,       65 },
	{ "clan.shadowplane.blood_cost_max",          75,       75 },
	{ "clan.darkheart.level_req",                  1,        1 },
	{ "clan.darkheart.blood_cost",               100,      100 },
	{ "clan.darkheart.self_damage_min",           10,       10 },
	{ "clan.darkheart.self_damage_max",           20,       20 },
	{ "clan.truesight.level_req_vampire",          1,        1 },
	{ "clan.truesight.level_req_shapeshifter",     1,        1 },
	{ "clan.truesight.level_req_werewolf",         3,        3 },
	{ "clan.truesight.level_req_monk",             3,        3 },
	{ "clan.majesty.level_req",                    5,        5 },
	{ "clan.majesty.blood_req",                   45,       45 },
	{ "clan.majesty.blood_cost_min",              35,       35 },
	{ "clan.majesty.blood_cost_max",              45,       45 },
	{ "clan.scry.level_req_vampire",               2,        2 },
	{ "clan.scry.level_req_monk",                  3,        3 },
	{ "clan.scry.level_req_mage",                  3,        3 },
	{ "clan.scry.level_req_droid",                 5,        5 },
	{ "clan.scry.blood_req",                      25,       25 },
	{ "clan.scry.blood_cost_min",                 15,       15 },
	{ "clan.scry.blood_cost_max",                 25,       25 },
	{ "clan.readaura.level_req_vampire",           3,        3 },
	{ "clan.readaura.level_req_lich",              1,        1 },
	{ "clan.readaura.level_req_monk",              2,        2 },
	{ "clan.readaura.level_req_droid",             5,        5 },
	{ "clan.mortal.level_req",                     4,        4 },
	{ "clan.shield.level_req_werewolf",            2,        2 },
	{ "clan.shield.level_req_monk",                2,        2 },
	{ "clan.shield.level_req_droid",               4,        4 },
	{ "clan.shield.level_req_vamp_obfu",           3,        3 },
	{ "clan.shield.level_req_vamp_domi",           2,        2 },
	{ "clan.serpent.level_req",                    2,        2 },
	{ "clan.serpent.blood_req",                   50,       50 },
	{ "clan.serpent.blood_cost_min",              40,       40 },
	{ "clan.serpent.blood_cost_max",              50,       50 },
	{ "clan.serpent.mod_str",                     10,       10 },
	{ "clan.poison.level_req",                     3,        3 },
	{ "clan.poison.blood_req",                    15,       15 },
	{ "clan.poison.blood_cost_min",                5,        5 },
	{ "clan.poison.blood_cost_max",               15,       15 },
	{ "clan.poison.timer_min",                    10,       10 },
	{ "clan.poison.timer_max",                    20,       20 },
	{ "clan.regenerate.blood_req",                 5,        5 },
	{ "clan.regenerate.blood_cost_min",           10,       10 },
	{ "clan.regenerate.blood_cost_max",           20,       20 },
	{ "clan.regenerate.heal_amount",             500,      500 },
	{ "clan.regenerate.cooldown_normal",           8,        8 },
	{ "clan.regenerate.cooldown_downed",          24,       24 },
	{ "clan.humanity.beast_chance_divisor",      300,      300 },
	{ "clan.humanity.golconda_exp_reward",   1000000,  1000000 },
	{ "clan.beastlike.beast_chance_divisor",     500,      500 },
	{ "clan.werewolf.rage_initial",               25,       25 },
	{ "clan.werewolf.hitroll_bonus",              50,       50 },
	{ "clan.werewolf.damroll_bonus",              50,       50 },
	{ "clan.werewolf.rage_wolf4_bonus",          100,      100 },
	{ "clan.werewolf.rage_max",                  300,      300 },
	{ "clan.unwerewolf.rage_loss",                25,       25 },
	{ "clan.unwerewolf.hitroll_loss",             50,       50 },
	{ "clan.unwerewolf.damroll_loss",             50,       50 },
	{ "clan.possession.level_req_vampire",         3,        3 },
	{ "clan.possession.level_req_demon",          10,       10 },
	{ "clan.possession.blood_cost_vampire",       50,       50 },
	{ "clan.possession.move_cost_demon",         500,      500 },
	{ "clan.fcommand.level_req",                   3,        3 },
	{ "clan.vanish.level_req_werewolf",            1,        1 },
	{ "clan.vanish.level_req_vampire",             1,        1 },
	{ "clan.flex.cooldown",                       12,       12 },
	{ "clan.rage_ww.rage_gain_min",               40,       40 },
	{ "clan.rage_ww.rage_gain_max",               60,       60 },
	{ "clan.rage_ww.transform_threshold",        100,      100 },
	{ "clan.rage_ww.cooldown",                    12,       12 },
	{ "clan.rage_demon.level_req",                 3,        3 },
	{ "clan.rage_demon.rage_cap",                125,      125 },
	{ "clan.rage_demon.rage_gain_max",            25,       25 },
	{ "clan.rage_demon.beast_max",               100,      100 },
	{ "clan.rage_demon.cooldown",                 12,       12 },
	{ "clan.calm_ww.level_req",                    3,        3 },
	{ "clan.calm_ww.rage_loss_min",               60,       60 },
	{ "clan.calm_ww.rage_loss_max",               90,       90 },
	{ "clan.calm_ww.unshift_threshold",          100,      100 },
	{ "clan.calm_ww.cooldown",                    12,       12 },
	{ "clan.calm_ninja.cooldown",                 12,       12 },
	{ "clan.calm_demon.level_req",                 4,        4 },
	{ "clan.calm_demon.cooldown",                  8,        8 },
	{ "clan.web.level_req_werewolf",               2,        2 },
	{ "clan.web.level_req_tanarri",                3,        3 },
	{ "clan.web.cooldown",                        12,       12 },
	{ "clan.mitsukeru.level_req",                  1,        1 },
	{ "clan.koryou.level_req",                     2,        2 },
	{ "clan.stake.exp_reward",                  1000,     1000 },
	{ "clan.horn.level_req",                       4,        4 },
	{ "clan.reset_weapon.cap",                   200,      200 },
	{ "clan.reset_spell.cap",                    200,      200 },
	{ "clan.class_vampire.default_beast",         30,       30 },

	/* ================================================================
	 * DEMON  (demon.c)
	 * ================================================================ */
	{ "demon.obtain.demon_points_cost",        15000,    15000 },
	{ "demon.obtain.max_warps",                   18,       18 },
	{ "demon.inpart.cost.fangs",                2500,     2500 },
	{ "demon.inpart.cost.claws",                2500,     2500 },
	{ "demon.inpart.cost.horns",                2500,     2500 },
	{ "demon.inpart.cost.immolate",             2500,     2500 },
	{ "demon.inpart.cost.caust",                3000,     3000 },
	{ "demon.inpart.cost.freezeweapon",         3000,     3000 },
	{ "demon.inpart.cost.nightsight",           3000,     3000 },
	{ "demon.inpart.cost.unnerve",              5000,     5000 },
	{ "demon.inpart.cost.tail",                 5000,     5000 },
	{ "demon.inpart.cost.might",                7500,     7500 },
	{ "demon.inpart.cost.toughness",            7500,     7500 },
	{ "demon.inpart.cost.speed",                7500,     7500 },
	{ "demon.inpart.cost.scry",                 7500,     7500 },
	{ "demon.inpart.cost.truesight",            7500,     7500 },
	{ "demon.inpart.cost.hooves",               1500,     1500 },
	{ "demon.inpart.cost.travel",               1500,     1500 },
	{ "demon.inpart.cost.move",                  500,      500 },
	{ "demon.inpart.cost.leap",                  500,      500 },
	{ "demon.inpart.cost.magic",                1000,     1000 },
	{ "demon.inpart.cost.wings",                1000,     1000 },
	{ "demon.inpart.cost.shield",              20000,    20000 },
	{ "demon.inpart.cost.graft",               20000,    20000 },
	{ "demon.inpart.cost.inferno",             20000,    20000 },
	{ "demon.inpart.cost.entomb",              20000,    20000 },
	{ "demon.inpart.cost.leech",               15000,    15000 },
	{ "demon.inpart.cost.blink",               15000,    15000 },
	{ "demon.inpart.cost.demonform",           25000,    25000 },
	{ "demon.inpart.cost.lifespan",              100,      100 },
	{ "demon.inpart.other_multiplier",            25,       25 },
	{ "demon.inpart.nonclass_primal_cost",       100,      100 },
	{ "demon.demonarmour.primal_cost",            60,       60 },
	{ "demon.horns.level_req",                     4,        4 },
	{ "demon.wings.level_req",                     5,        5 },
	{ "demon.cone.mana_cost",                    100,      100 },
	{ "demon.cone.cooldown",                      10,       10 },
	{ "demon.dstake.primal_cost",                 60,       60 },

	/* ================================================================
	 * ANGEL  (angel.c)
	 * ================================================================ */
	{ "angel.spiritform.level_req",                2,        2 },
	{ "angel.gpeace.level_req",                    1,        1 },
	{ "angel.innerpeace.level_req",                3,        3 },
	{ "angel.innerpeace.mana_cost",             1500,     1500 },
	{ "angel.innerpeace.heal_multiplier",        500,      500 },
	{ "angel.innerpeace.cooldown",                18,       18 },
	{ "angel.houseofgod.level_req",                5,        5 },
	{ "angel.houseofgod.peace_counter",           50,       50 },
	{ "angel.houseofgod.cooldown",                24,       24 },
	{ "angel.angelicaura.level_req",               2,        2 },
	{ "angel.gbanish.level_req",                   3,        3 },
	{ "angel.gbanish.dam_good",                  500,      500 },
	{ "angel.gbanish.dam_neutral",              1000,     1000 },
	{ "angel.gbanish.dam_evil",                 1500,     1500 },
	{ "angel.gbanish.cooldown",                   18,       18 },
	{ "angel.harmony.level_req",                   5,        5 },
	{ "angel.harmony.cooldown",                   12,       12 },
	{ "angel.gsenses.level_req",                   1,        1 },
	{ "angel.gfavor.level_req",                    2,        2 },
	{ "angel.gfavor.mana_cost",                 2000,     2000 },
	{ "angel.gfavor.move_cost",                 2000,     2000 },
	{ "angel.gfavor.damroll_bonus",              400,      400 },
	{ "angel.gfavor.hitroll_bonus",              400,      400 },
	{ "angel.forgivness.level_req",                3,        3 },
	{ "angel.forgivness.heal_min",              1000,     1000 },
	{ "angel.forgivness.heal_max",              1500,     1500 },
	{ "angel.forgivness.cooldown",                16,       16 },
	{ "angel.martyr.level_req",                    5,        5 },
	{ "angel.martyr.cooldown",                     6,        6 },
	{ "angel.swoop.level_req",                     1,        1 },
	{ "angel.swoop.move_cost",                   500,      500 },
	{ "angel.touchofgod.level_req",                4,        4 },
	{ "angel.touchofgod.dam_min",                100,      100 },
	{ "angel.touchofgod.dam_max",                200,      200 },
	{ "angel.touchofgod.cooldown",                18,       18 },
	{ "angel.awings.level_req",                    1,        1 },
	{ "angel.halo.level_req",                      2,        2 },
	{ "angel.sinsofthepast.level_req",             3,        3 },
	{ "angel.sinsofthepast.cooldown",             12,       12 },
	{ "angel.eyeforaneye.level_req",               5,        5 },
	{ "angel.angelicarmor.practice_cost",        150,      150 },

	/* ================================================================
	 * LICH  (lich.c)
	 * ================================================================ */
	{ "lich.planeshift.level_req",                 5,        5 },
	{ "lich.pentagram.level_req",                  5,        5 },
	{ "lich.pentagram.mana_cost",               1500,     1500 },
	{ "lich.pentagram.cooldown",                   8,        8 },
	{ "lich.soulsuck.level_req",                   3,        3 },
	{ "lich.soulsuck.damage_min",                250,      250 },
	{ "lich.soulsuck.damage_max",               1000,     1000 },
	{ "lich.soulsuck.cooldown",                   10,       10 },
	{ "lich.licharmor.practice_cost",            150,      150 },
	{ "lich.earthswallow.level_req",               3,        3 },
	{ "lich.earthswallow.mana_cost",            5000,     5000 },
	{ "lich.earthswallow.cooldown",                8,        8 },
	{ "lich.painwreck.level_req",                  4,        4 },
	{ "lich.painwreck.damage_min",               100,      100 },
	{ "lich.painwreck.damage_max",               200,      200 },
	{ "lich.painwreck.cooldown",                  18,       18 },
	{ "lich.painwreck.stun_chance",                3,        3 },
	{ "lich.creepingdoom.level_req",               5,        5 },
	{ "lich.creepingdoom.mana_cost",            5000,     5000 },
	{ "lich.creepingdoom.cooldown",               24,       24 },
	{ "lich.chillhand.level_req",                  3,        3 },
	{ "lich.chillhand.damage_min",               750,      750 },
	{ "lich.chillhand.damage_max",              1250,     1250 },
	{ "lich.chillhand.pvp_damage_divisor",         3,        3 },
	{ "lich.chillhand.cooldown",                   8,        8 },
	{ "lich.chillhand.debuff_duration",            6,        6 },
	{ "lich.chillhand.str_modifier",              -3,       -3 },
	{ "lich.polarity.level_req",                   5,        5 },
	{ "lich.polarity.drain_min",                2000,     2000 },
	{ "lich.polarity.drain_max",                4000,     4000 },
	{ "lich.polarity.heal_divisor",                2,        2 },
	{ "lich.polarity.cooldown",                   12,       12 },
	{ "lich.powertransfer.level_req",              4,        4 },
	{ "lich.powertransfer.mana_cost",           5000,     5000 },
	{ "lich.powertransfer.heal_min",            7500,     7500 },
	{ "lich.powertransfer.heal_max",           10000,    10000 },
	{ "lich.powertransfer.cooldown",              18,       18 },
	{ "lich.planarstorm.level_req",                4,        4 },
	{ "lich.planarstorm.mana_cost",             5000,     5000 },
	{ "lich.planarstorm.damage_min",            7500,     7500 },
	{ "lich.planarstorm.damage_max",           12500,    12500 },
	{ "lich.planarstorm.cooldown",                24,       24 },
	{ "lich.planartravel.level_req",               3,        3 },
	{ "lich.planartravel.mana_cost",             500,      500 },
	{ "lich.planartravel.duration_min",            2,        2 },
	{ "lich.planartravel.duration_max",            3,        3 },
	{ "lich.chaosshield.level_req",                5,        5 },
	{ "lich.studylore.max_level",                  5,        5 },
	{ "lich.studylore.exp_per_level",       10000000, 10000000 },
	{ "lich.summongolem.level_req",                4,        4 },
	{ "lich.summongolem.charm_duration",         666,      666 },

	/* ================================================================
	 * MAGE  (mage.c)
	 * ================================================================ */
	{ "mage.reveal.mana_cost",                  5000,     5000 },
	{ "mage.magearmor.practice_cost",             60,       60 },
	{ "mage.chaosmagic.mana_cost",              1500,     1500 },
	{ "mage.chaosmagic.cooldown",                  6,        6 },
	{ "mage.chant_heal.mana_cost",              1500,     1500 },
	{ "mage.chant_heal.cooldown",                  8,        8 },
	{ "mage.chant_damage.mana_cost",            1000,     1000 },
	{ "mage.chant_damage.cooldown",                8,        8 },
	{ "mage.chant_damage.pvp_dam_cap",          1000,     1000 },
	{ "mage.chant_damage.pvp_cap_range_low",     950,      950 },
	{ "mage.chant_damage.pvp_cap_range_high",   1050,     1050 },
	{ "mage.chant_bless.mana_cost",             2500,     2500 },
	{ "mage.chant_bless.cooldown",                 6,        6 },
	{ "mage.chant_curse.mana_cost",             2500,     2500 },
	{ "mage.chant_curse.cooldown",                 6,        6 },
	{ "mage.invoke_learn.cost_multiplier",        20,       20 },
	{ "mage.invoke_learn.max_level",              10,       10 },
	{ "mage.invoke_mageshield.practice_cost",     25,       25 },
	{ "mage.invoke_mageshield.level_req",          2,        2 },
	{ "mage.invoke_steelshield.practice_cost",     5,        5 },
	{ "mage.invoke_steelshield.level_req",         6,        6 },
	{ "mage.invoke_beast.practice_cost",          10,       10 },
	{ "mage.invoke_beast.level_req",               9,        9 },
	{ "mage.invoke_illusions.practice_cost",       5,        5 },
	{ "mage.invoke_illusions.level_req",           8,        8 },
	{ "mage.invoke_deflector.practice_cost",       5,        5 },
	{ "mage.invoke_deflector.level_req",           5,        5 },
	{ "mage.invoke_all.level_req",                 9,        9 },
	{ "mage.discharge.level_req",                  4,        4 },
	{ "mage.discharge.entropy_bonus",            100,      100 },
	{ "mage.teleport.mana_cost",                 250,      250 },
	{ "mage.teleport.level_req",                   1,        1 },
	{ "mage.bewitch.cooldown",                     2,        2 },

	/* ================================================================
	 * MONK  (monk.c + monk2.c)
	 * ================================================================ */
	{ "monk.monkarmor.primal_cost",               60,       60 },
	{ "monk.deathtouch.level_req",                 4,        4 },
	{ "monk.deathtouch.cooldown",                 12,       12 },
	{ "monk.healingtouch.level_req",               3,        3 },
	{ "monk.healingtouch.cooldown",               12,       12 },
	{ "monk.spiritpower.level_req",                3,        3 },
	{ "monk.spiritpower.damroll_bonus",          200,      200 },
	{ "monk.spiritpower.hitroll_bonus",          200,      200 },
	{ "monk.spiritpower.move_check",             100,      100 },
	{ "monk.spiritpower.move_cost",               25,       25 },
	{ "monk.relax.cooldown",                      12,       12 },
	{ "monk.chi.cooldown",                        12,       12 },
	{ "monk.chi.move_cost_base",                 500,      500 },
	{ "monk.chi.move_cost_per_level",             20,       20 },
	{ "monk.chands.level_req",                     9,        9 },
	{ "monk.guide.exp_cost",                   50000,    50000 },
	{ "monk.mantra.max_level",                    14,       14 },
	{ "monk.mantra.cost_multiplier",              10,       10 },
	{ "monk.cloak.level_req",                     11,       11 },
	{ "monk.cloak.move_cost",                   1000,     1000 },
	{ "monk.sacredinvis.level_req",                3,        3 },
	{ "monk.sacredinvis.move_cost",              500,      500 },
	{ "monk.flaminghands.level_req",               5,        5 },
	{ "monk.adamantium.level_req",                 1,        1 },
	{ "monk.celestial.level_req",                 10,       10 },
	{ "monk.celestial.move_cost",                250,      250 },
	{ "monk.godseye.level_req",                    1,        1 },
	{ "monk.steelskin.level_req",                  6,        6 },
	{ "monk.godsbless.level_req",                  7,        7 },
	{ "monk.godsbless.mana_cost",               3000,     3000 },
	{ "monk.godsbless.cooldown",                  12,       12 },
	{ "monk.wrathofgod.level_req",                 4,        4 },
	{ "monk.wrathofgod.num_hits",                  4,        4 },
	{ "monk.wrathofgod.cooldown",                  8,        8 },
	{ "monk.godsfavor.level_req",                  8,        8 },
	{ "monk.godsfavor.move_cost",               1500,     1500 },
	{ "monk.godsfavor.cooldown",                   4,        4 },
	{ "monk.darkblaze.level_req",                  8,        8 },
	{ "monk.darkblaze.blind_duration",            60,       60 },
	{ "monk.darkblaze.cooldown",                  18,       18 },
	{ "monk.godsheal.level_req",                  12,       12 },
	{ "monk.godsheal.mana_check",                300,      300 },
	{ "monk.godsheal.mana_cost",                 400,      400 },
	{ "monk.godsheal.heal_fighting",             150,      150 },
	{ "monk.godsheal.heal_nonfighting",          500,      500 },
	{ "monk.godsheal.cooldown_fighting",          12,       12 },
	{ "monk.godsheal.cooldown_nonfighting",        8,        8 },
	{ "monk.ghold.level_req",                     13,       13 },
	{ "monk.find_dam.pvp_dam_threshold",        1000,     1000 },
	{ "monk.find_dam.pvp_dam_cap_min",           940,      940 },
	{ "monk.find_dam.pvp_dam_cap_max",          1020,     1020 },
	{ "monk.shinkick.cooldown",                   12,       12 },
	{ "monk.palmstrike.cooldown",                 11,       11 },
	{ "monk.knee.cooldown",                       11,       11 },
	{ "monk.sweep.cooldown",                      11,       11 },
	{ "monk.sweep.combo_paralyse_duration",       30,       30 },
	{ "monk.elbow.cooldown",                      11,       11 },
	{ "monk.thrustkick.cooldown",                 11,       11 },
	{ "monk.spinkick.cooldown",                   11,       11 },
	{ "monk.spinkick.lightning_max_hits",          6,        6 },
	{ "monk.backfist.cooldown",                   11,       11 },
	{ "monk.learn.fight_cost",                 50000,    50000 },
	{ "monk.learn.technique_cost",            200000,   200000 },
	{ "monk.learn.ability_cost",              500000,   500000 },
	{ "monk.learn.ability_max_rank",               4,        4 },
	{ "monk.learn.chi_cost_multiplier",      1000000,  1000000 },
	{ "monk.learn.chi_max_rank",                   6,        6 },

	/* ================================================================
	 * WEREWOLF  (ww.c)
	 * ================================================================ */
	{ "werewolf.klaive.primal_cost",              60,       60 },
	{ "werewolf.sclaws.level_req",                 5,        5 },
	{ "werewolf.moonbeam.level_req",               8,        8 },
	{ "werewolf.moonbeam.mana_cost",             500,      500 },
	{ "werewolf.moonbeam.damage_good",           500,      500 },
	{ "werewolf.moonbeam.damage_evil",          1000,     1000 },
	{ "werewolf.moonbeam.damage_neutral",        750,      750 },
	{ "werewolf.moonbeam.cooldown",               12,       12 },
	{ "werewolf.moongate.level_req",               6,        6 },
	{ "werewolf.moongate.gate_timer",              5,        5 },
	{ "werewolf.gmotherstouch.level_req",          4,        4 },
	{ "werewolf.gmotherstouch.mana_cost_combat", 100,      100 },
	{ "werewolf.gmotherstouch.heal_combat",      200,      200 },
	{ "werewolf.gmotherstouch.cooldown_combat",   16,       16 },
	{ "werewolf.gmotherstouch.mana_cost_noncombat", 300,   300 },
	{ "werewolf.gmotherstouch.heal_noncombat",   600,      600 },
	{ "werewolf.gmotherstouch.cooldown_noncombat", 6,        6 },
	{ "werewolf.motherstouch.level_req",           3,        3 },
	{ "werewolf.motherstouch.mana_cost_combat",   50,       50 },
	{ "werewolf.motherstouch.heal_combat",       100,      100 },
	{ "werewolf.motherstouch.cooldown_combat",    16,       16 },
	{ "werewolf.motherstouch.mana_cost_noncombat", 250,    250 },
	{ "werewolf.motherstouch.heal_noncombat",    500,      500 },
	{ "werewolf.motherstouch.cooldown_noncombat",  8,        8 },
	{ "werewolf.flameclaws.level_req",             1,        1 },
	{ "werewolf.moonarmour.level_req",             2,        2 },
	{ "werewolf.moonarmour.primal_cost",          60,       60 },
	{ "werewolf.rend.level_req",                   7,        7 },
	{ "werewolf.skin.level_req",                   7,        7 },
	{ "werewolf.skin.armor_bonus",               100,      100 },
	{ "werewolf.jawlock.level_req",                8,        8 },
	{ "werewolf.perception.level_req",             3,        3 },
	{ "werewolf.roar.level_req",                   6,        6 },
	{ "werewolf.roar.cooldown_success",           18,       18 },
	{ "werewolf.roar.cooldown_fail",              12,       12 },
	{ "werewolf.slam.level_req",                   8,        8 },
	{ "werewolf.shred.level_req",                  7,        7 },
	{ "werewolf.shred.cooldown",                  24,       24 },
	{ "werewolf.shred.base_hits",                  2,        2 },
	{ "werewolf.talons.level_req",                10,       10 },
	{ "werewolf.talons.cooldown",                 12,       12 },
	{ "werewolf.talons.damage_pc_min",           400,      400 },
	{ "werewolf.talons.damage_pc_max",           600,      600 },
	{ "werewolf.talons.damage_npc_min",         2000,     2000 },
	{ "werewolf.talons.damage_npc_max",         4000,     4000 },
	{ "werewolf.devour.level_req",                 5,        5 },
	{ "werewolf.devour.heal_min",                100,      100 },
	{ "werewolf.devour.heal_max",                250,      250 },
	{ "werewolf.staredown.level_req",              5,        5 },
	{ "werewolf.staredown.cooldown",              16,       16 },
	{ "werewolf.disquiet.level_req",               6,        6 },
	{ "werewolf.disquiet.cooldown",                4,        4 },
	{ "werewolf.reshape.level_req",                7,        7 },
	{ "werewolf.cocoon.level_req",                 8,        8 },
	{ "werewolf.quills.level_req",                 5,        5 },
	{ "werewolf.burrow.level_req",                 6,        6 },
	{ "werewolf.wither.level_req",                 7,        7 },
	{ "werewolf.wither.cooldown",                 18,       18 },
	{ "werewolf.wither.pc_chance",                15,       15 },
	{ "werewolf.razorclaws.level_req",             4,        4 },

	/* ================================================================
	 * NINJA  (ninja.c)
	 * ================================================================ */
	{ "ninja.stalk.move_cost",                   500,      500 },
	{ "ninja.principles.sora_max",                 6,        6 },
	{ "ninja.principles.chikyu_max",               6,        6 },
	{ "ninja.principles.ningenno_max",             6,        6 },
	{ "ninja.michi.rage_threshold",              100,      100 },
	{ "ninja.michi.rage_gain",                   100,      100 },
	{ "ninja.michi.move_cost",                   500,      500 },
	{ "ninja.michi.cooldown",                     12,       12 },
	{ "ninja.kakusu.level_req",                    3,        3 },
	{ "ninja.kakusu.move_cost",                  500,      500 },
	{ "ninja.kanzuite.level_req",                  5,        5 },
	{ "ninja.kanzuite.move_cost",                500,      500 },
	{ "ninja.mienaku.level_req",                   3,        3 },
	{ "ninja.mienaku.move_cost",                 200,      200 },
	{ "ninja.bomuzite.level_req",                  6,        6 },
	{ "ninja.bomuzite.move_cost",                500,      500 },
	{ "ninja.bomuzite.cooldown",                   1,        1 },
	{ "ninja.tsume.level_req",                     1,        1 },
	{ "ninja.hara_kiri.level_req",                 6,        6 },
	{ "ninja.hara_kiri.min_duration",              5,        5 },
	{ "ninja.ninjaarmor.primal_cost",             60,       60 },
	{ "ninja.strangle.level_req",                  2,        2 },
	{ "ninja.strangle.fail_threshold",            15,       15 },
	{ "ninja.strangle.cooldown_success",          12,       12 },
	{ "ninja.strangle.cooldown_fail",             24,       24 },
	{ "ninja.strangle.hits_success",               4,        4 },

	/* ================================================================
	 * SAMURAI  (samurai.c)
	 * ================================================================ */
	{ "samurai.katana.primal_cost",              250,      250 },
	{ "samurai.katana.weapon_dice_count",         65,       65 },
	{ "samurai.katana.weapon_dice_size",         115,      115 },
	{ "samurai.bladespin.wpn_skill_req",        1000,     1000 },
	{ "samurai.hologramtransfer.move_cost",     1000,     1000 },
	{ "samurai.focus.cooldown",                    8,        8 },
	{ "samurai.slide.focus_cost",                  1,        1 },
	{ "samurai.slide.focus_max",                  40,       40 },
	{ "samurai.slide.cooldown",                   12,       12 },
	{ "samurai.sidestep.focus_cost",               2,        2 },
	{ "samurai.sidestep.focus_max",               40,       40 },
	{ "samurai.sidestep.cooldown",                12,       12 },
	{ "samurai.block.focus_cost",                  4,        4 },
	{ "samurai.block.focus_max",                  40,       40 },
	{ "samurai.block.cooldown",                   12,       12 },
	{ "samurai.countermove.focus_cost",             8,        8 },
	{ "samurai.countermove.focus_max",             40,       40 },
	{ "samurai.countermove.cooldown",              12,       12 },
	{ "samurai.combo.victim_min_hp",            1000,     1000 },
	{ "samurai.combo_10.hits",                     3,        3 },
	{ "samurai.combo_15.fail_range_max",           3,        3 },
	{ "samurai.combo_20.victim_cooldown",         24,       24 },
	{ "samurai.combo_25.fail_range_max",           3,        3 },
	{ "samurai.combo_30.heal_min",              2000,     2000 },
	{ "samurai.combo_30.heal_max",              4000,     4000 },
	{ "samurai.combo_35.hits",                     5,        5 },
	{ "samurai.martial.exp_cost",          150000000, 150000000 },

	/* ================================================================
	 * DROW  (drow.c)
	 * ================================================================ */
	{ "drow.grant.other_multiplier",               5,        5 },
	{ "drow.grant.drowfire_cost",               2500,     2500 },
	{ "drow.grant.darkness_cost",               7500,     7500 },
	{ "drow.grant.drowsight_cost",              5000,     5000 },
	{ "drow.grant.spiderarms_cost",            25000,    25000 },
	{ "drow.grant.web_cost",                    5000,     5000 },
	{ "drow.grant.spiderform_cost",            25000,    25000 },
	{ "drow.grant.drowhate_cost",              20000,    20000 },
	{ "drow.grant.drowshield_cost",             5000,     5000 },
	{ "drow.grant.levitation_cost",             1000,     1000 },
	{ "drow.grant.shadowwalk_cost",            10000,    10000 },
	{ "drow.grant.garotte_cost",                5000,     5000 },
	{ "drow.grant.dgarotte_cost",               2500,     2500 },
	{ "drow.grant.drowpoison_cost",             2500,     2500 },
	{ "drow.grant.glamour_cost",                5000,     5000 },
	{ "drow.grant.confuse_cost",                2500,     2500 },
	{ "drow.grant.earthshatter_cost",           7500,     7500 },
	{ "drow.grant.speed_cost",                  7500,     7500 },
	{ "drow.grant.toughskin_cost",              7500,     7500 },
	{ "drow.grant.darktendrils_cost",          25000,    25000 },
	{ "drow.grant.fightdance_cost",            10000,    10000 },
	{ "drow.chaosblast.mana_cost",               750,      750 },
	{ "drow.chaosblast.cooldown",                 12,       12 },
	{ "drow.drowcreate.primal_cost",              60,       60 },
	{ "drow.drowfire.mana_cost",                 100,      100 },
	{ "drow.drowfire.cooldown",                   12,       12 },
	{ "drow.heal.mana_cost",                     750,      750 },
	{ "drow.heal.cooldown",                       12,       12 },
	{ "drow.shadowwalk.move_cost",               250,      250 },
	{ "drow.spiderform.hitroll_bonus",           400,      400 },
	{ "drow.spiderform.damroll_bonus",           400,      400 },
	{ "drow.spiderform.armor_bonus",            1000,     1000 },
	{ "drow.spiderform.revert_cooldown",           7,        7 },
	{ "drow.darkness.mana_cost",                 500,      500 },
	{ "drow.confuse.move_cost",                   75,       75 },
	{ "drow.confuse.fail_threshold",              25,       25 },
	{ "drow.confuse.cooldown",                    16,       16 },
	{ "drow.earthshatter.mana_cost",             150,      150 },
	{ "drow.earthshatter.cooldown",               12,       12 },

	/* ================================================================
	 * SHAPESHIFTER  (shapeshifter.c)
	 * ================================================================ */
	{ "shapeshifter.stomp.level_req",              5,        5 },
	{ "shapeshifter.stomp.damage",               500,      500 },
	{ "shapeshifter.stomp.cooldown",              24,       24 },
	{ "shapeshifter.faeriecurse.level_req",        4,        4 },
	{ "shapeshifter.faeriecurse.mana_cost",     1000,     1000 },
	{ "shapeshifter.faeriecurse.move_cost",      500,      500 },
	{ "shapeshifter.faeriecurse.cooldown",        12,       12 },
	{ "shapeshifter.breath.cooldown",             24,       24 },
	{ "shapeshifter.phase.level_req",              5,        5 },
	{ "shapeshifter.phase.duration",              10,       10 },
	{ "shapeshifter.shapearmor.primal_cost",     150,      150 },
	{ "shapeshifter.mistwalk.level_req",           2,        2 },
	{ "shapeshifter.mistwalk.move_cost",         250,      250 },
	{ "shapeshifter.hatform.level_req",            3,        3 },
	{ "shapeshifter.shift.counter_max",           35,       35 },
	{ "shapeshifter.shift.counter_increment",     10,       10 },
	{ "shapeshifter.shift.heal_cap",            5000,     5000 },
	{ "shapeshifter.shift.tiger_damroll",        300,      300 },
	{ "shapeshifter.shift.tiger_hitroll",        450,      450 },
	{ "shapeshifter.shift.tiger_armor",         -200,     -200 },
	{ "shapeshifter.shift.hydra_damroll",        450,      450 },
	{ "shapeshifter.shift.hydra_hitroll",        450,      450 },
	{ "shapeshifter.shift.hydra_armor",          -50,      -50 },
	{ "shapeshifter.shift.bull_damroll",         400,      400 },
	{ "shapeshifter.shift.bull_hitroll",         400,      400 },
	{ "shapeshifter.shift.bull_armor",           -50,      -50 },
	{ "shapeshifter.shift.faerie_damroll",       250,      250 },
	{ "shapeshifter.shift.faerie_hitroll",       250,      250 },
	{ "shapeshifter.shift.faerie_armor",        -500,     -500 },
	{ "shapeshifter.formlearn.cost_multiplier",   80,       80 },
	{ "shapeshifter.formlearn.max_level",          5,        5 },
	{ "shapeshifter.faerieblink.level_req",        5,        5 },
	{ "shapeshifter.faerieblink.mana_cost",     2500,     2500 },
	{ "shapeshifter.faerieblink.cooldown",        18,       18 },
	{ "shapeshifter.camouflage.level_req",         1,        1 },
	{ "shapeshifter.shapeshift.level_req",         4,        4 },
	{ "shapeshifter.shaperoar.level_req",          3,        3 },
	{ "shapeshifter.shaperoar.cooldown",          16,       16 },
	{ "shapeshifter.charge.level_req",             4,        4 },
	{ "shapeshifter.charge.move_cost",          2000,     2000 },
	{ "shapeshifter.charge.cooldown",             18,       18 },

	/* ================================================================
	 * TANARRI  (tanarri.c)
	 * ================================================================ */
	{ "tanarri.taneq.primal_cost",               150,      150 },
	{ "tanarri.chaossurge.damage_good",         1500,     1500 },
	{ "tanarri.chaossurge.damage_neutral",      1000,     1000 },
	{ "tanarri.chaossurge.damage_mildevil",      500,      500 },
	{ "tanarri.chaossurge.cooldown",              12,       12 },
	{ "tanarri.enmity.cooldown",                  24,       24 },
	{ "tanarri.enrage.cooldown",                  18,       18 },
	{ "tanarri.lavablast.mana_cost",            1000,     1000 },
	{ "tanarri.lavablast.move_cost",            1000,     1000 },
	{ "tanarri.lavablast.cooldown",               18,       18 },
	{ "tanarri.chaosgate.move_cost",            1000,     1000 },
	{ "tanarri.booming.cooldown",                 12,       12 },
	{ "tanarri.fury.damroll_bonus",              250,      250 },
	{ "tanarri.fury.hitroll_bonus",              250,      250 },
	{ "tanarri.earthquake.mana_cost",           1000,     1000 },
	{ "tanarri.earthquake.cooldown",              12,       12 },
	{ "tanarri.tornado.mana_cost",              1500,     1500 },
	{ "tanarri.tornado.cooldown",                 12,       12 },
	{ "tanarri.infernal.mana_cost",             2000,     2000 },
	{ "tanarri.infernal.cooldown",                12,       12 },

	/* ================================================================
	 * UNDEAD KNIGHT  (undead_knight.c)
	 * ================================================================ */
	{ "undead_knight.ride.move_cost",            600,      600 },
	{ "undead_knight.knightarmor.primal_cost",   150,      150 },
	{ "undead_knight.unholyrite.mana_cost",      500,      500 },
	{ "undead_knight.unholyrite.heal_min",       500,      500 },
	{ "undead_knight.unholyrite.heal_max",      1000,     1000 },
	{ "undead_knight.unholyrite.cooldown",        18,       18 },
	{ "undead_knight.aura_might.level_req",        4,        4 },
	{ "undead_knight.aura_might.damroll_bonus",  300,      300 },
	{ "undead_knight.aura_might.hitroll_bonus",  300,      300 },
	{ "undead_knight.aura_death.level_req",        2,        2 },
	{ "undead_knight.aura_fear.level_req",         9,        9 },
	{ "undead_knight.aura_bog.level_req",          6,        6 },
	{ "undead_knight.gain_necromancy.max_level",  10,       10 },
	{ "undead_knight.gain_necromancy.cost_multiplier", 60,  60 },
	{ "undead_knight.gain_invocation.max_level",   5,        5 },
	{ "undead_knight.gain_invocation.cost_multiplier", 60,  60 },
	{ "undead_knight.gain_spirit.max_level",      10,       10 },
	{ "undead_knight.gain_spirit.cost_multiplier", 60,      60 },
	{ "undead_knight.weaponpractice.max_level",   10,       10 },
	{ "undead_knight.weaponpractice.cost_multiplier", 60,   60 },
	{ "undead_knight.powerword_stun.level_req",    5,        5 },
	{ "undead_knight.powerword_stun.victim_cooldown", 24,   24 },
	{ "undead_knight.powerword_stun.caster_cooldown",  8,    8 },
	{ "undead_knight.powerword_blind.level_req",   1,        1 },
	{ "undead_knight.powerword_blind.duration",   60,       60 },
	{ "undead_knight.powerword_blind.cooldown",   12,       12 },
	{ "undead_knight.powerword_kill.level_req",    3,        3 },
	{ "undead_knight.powerword_kill.npc_dam_cap", 5000,   5000 },
	{ "undead_knight.powerword_kill.pc_dam_cap",  1500,   1500 },
	{ "undead_knight.powerword_flames.level_req",  4,        4 },
	{ "undead_knight.powerword_flames.cooldown",  12,       12 },

	/* ================================================================
	 * SPIDER DROID  (spiderdroid.c)
	 * ================================================================ */
	{ "spiderdroid.implant.face_max",              5,        5 },
	{ "spiderdroid.implant.legs_max",              5,        5 },
	{ "spiderdroid.implant.body_max",              6,        6 },
	{ "spiderdroid.implant.cost_level0",       12500,    12500 },
	{ "spiderdroid.implant.cost_level1",       25000,    25000 },
	{ "spiderdroid.implant.cost_level2",       50000,    50000 },
	{ "spiderdroid.implant.cost_level3",      100000,   100000 },
	{ "spiderdroid.implant.cost_level4",      200000,   200000 },
	{ "spiderdroid.implant.cost_level5",      400000,   400000 },
	{ "spiderdroid.stuntubes.body_level_req",      5,        5 },
	{ "spiderdroid.stuntubes.legs_level_req",      5,        5 },
	{ "spiderdroid.stuntubes.move_cost",        1000,     1000 },
	{ "spiderdroid.stuntubes.cooldown",           12,       12 },
	{ "spiderdroid.cubeform.body_level_req",       5,        5 },
	{ "spiderdroid.cubeform.mana_cost",         2000,     2000 },
	{ "spiderdroid.cubeform.move_cost",         2000,     2000 },
	{ "spiderdroid.cubeform.damroll_bonus",      250,      250 },
	{ "spiderdroid.cubeform.hitroll_bonus",      250,      250 },
	{ "spiderdroid.infravision.face_level_req",    1,        1 },
	{ "spiderdroid.dridereq.primal_cost",        150,      150 },

	/* ================================================================
	 * DIRGESINGER  (dirgesinger.c)
	 * ================================================================ */
	{ "dirgesinger.warcry.mana_cost",            20,       20 },
	{ "dirgesinger.warcry.resonance_gain",        5,        5 },
	{ "dirgesinger.warcry.cooldown",              4,        4 },
	{ "dirgesinger.warcry.dam_bonus_min",        50,       50 },
	{ "dirgesinger.warcry.dam_bonus_max",       120,      120 },
	{ "dirgesinger.shatter.mana_cost",           40,       40 },
	{ "dirgesinger.shatter.resonance_req",       25,       25 },
	{ "dirgesinger.shatter.resonance_cost",      10,       10 },
	{ "dirgesinger.shatter.disarm_chance",       30,       30 },
	{ "dirgesinger.shatter.cooldown",             8,        8 },
	{ "dirgesinger.battlehymn.mana_cost",        30,       30 },
	{ "dirgesinger.battlehymn.duration",         10,       10 },
	{ "dirgesinger.battlehymn.dam_bonus",        50,       50 },
	{ "dirgesinger.dirge.mana_cost",             25,       25 },
	{ "dirgesinger.dirge.max_stacks",             5,        5 },
	{ "dirgesinger.dirge.tick_damage",           50,       50 },
	{ "dirgesinger.dirge.duration",               6,        6 },
	{ "dirgesinger.thunderclap.mana_cost",       50,       50 },
	{ "dirgesinger.thunderclap.resonance_req",   40,       40 },
	{ "dirgesinger.thunderclap.resonance_cost",  20,       20 },
	{ "dirgesinger.thunderclap.cooldown",        10,       10 },
	{ "dirgesinger.ironsong.mana_cost",          35,       35 },
	{ "dirgesinger.ironsong.absorb_amount",    1000,     1000 },
	{ "dirgesinger.ironsong.duration",           12,       12 },
	{ "dirgesinger.cadence.mana_cost",           20,       20 },
	{ "dirgesinger.cadence.duration",             8,        8 },
	{ "dirgesinger.cadence.extra_attacks",        2,        2 },
	{ "dirgesinger.dissonance.mana_cost",        30,       30 },
	{ "dirgesinger.dissonance.cooldown",          8,        8 },
	{ "dirgesinger.dissonance.duration",         10,       10 },
	{ "dirgesinger.rally.mana_cost",             60,       60 },
	{ "dirgesinger.rally.resonance_req",         50,       50 },
	{ "dirgesinger.rally.resonance_cost",        30,       30 },
	{ "dirgesinger.rally.heal_amount",          500,      500 },
	{ "dirgesinger.warsong.mana_drain_per_tick", 10,       10 },
	{ "dirgesinger.armor.primal_cost",           60,       60 },

	/* ================================================================
	 * SIREN  (siren.c)
	 * ================================================================ */
	{ "siren.bansheewail.mana_cost",             80,       80 },
	{ "siren.bansheewail.resonance_req",         80,       80 },
	{ "siren.bansheewail.resonance_cost",        60,       60 },
	{ "siren.bansheewail.instakill_threshold",  500,      500 },
	{ "siren.bansheewail.cooldown",              15,       15 },
	{ "siren.soulrend.mana_cost",               50,       50 },
	{ "siren.soulrend.resonance_cost",           20,       20 },
	{ "siren.soulrend.cooldown",                  8,        8 },
	{ "siren.soulrend.bypass_pct",               40,       40 },
	{ "siren.crescendo.mana_per_stage",          25,       25 },
	{ "siren.crescendo.finale_dam_mult",          2,        2 },
	{ "siren.cacophony.mana_cost",               60,       60 },
	{ "siren.cacophony.resonance_cost",          40,       40 },
	{ "siren.cacophony.cooldown",                12,       12 },
	{ "siren.enthrall.mana_cost",               70,       70 },
	{ "siren.enthrall.resonance_cost",           50,       50 },
	{ "siren.enthrall.max_followers",             2,        2 },
	{ "siren.enthrall.duration",                 30,       30 },
	{ "siren.sirensong.mana_cost",             100,      100 },
	{ "siren.sirensong.resonance_cost",          80,       80 },
	{ "siren.sirensong.cooldown",                20,       20 },
	{ "siren.commandvoice.mana_cost",           40,       40 },
	{ "siren.commandvoice.resonance_cost",       30,       30 },
	{ "siren.commandvoice.cooldown",             10,       10 },
	{ "siren.mesmerize.mana_cost",              50,       50 },
	{ "siren.mesmerize.resonance_cost",          35,       35 },
	{ "siren.mesmerize.cooldown",                12,       12 },
	{ "siren.mesmerize.stun_duration",            3,        3 },
	{ "siren.echoshield.mana_cost",             40,       40 },
	{ "siren.echoshield.duration",              10,       10 },
	{ "siren.echoshield.reflect_pct",           20,       20 },
	{ "siren.ariaofunmaking.mana_cost",         70,       70 },
	{ "siren.ariaofunmaking.resonance_cost",    60,       60 },
	{ "siren.ariaofunmaking.cooldown",          15,       15 },
	{ "siren.ariaofunmaking.pvp_resist_pct",    50,       50 },

	/* Sentinel */
	{ NULL, 0, 0 }
};


/* ================================================================
 * Lookup functions
 * ================================================================ */

int acfg( const char *key ) {
	int i;

	for ( i = 0; acfg_table[i].key != NULL; i++ ) {
		if ( !str_cmp( key, acfg_table[i].key ) )
			return acfg_table[i].value;
	}

	{
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),
			"acfg: unknown key '%s'", key );
		log_string( buf );
	}

	return 0;
}

bool acfg_set( const char *key, int value ) {
	int i;

	for ( i = 0; acfg_table[i].key != NULL; i++ ) {
		if ( !str_cmp( key, acfg_table[i].key ) ) {
			acfg_table[i].value = value;
			return TRUE;
		}
	}

	return FALSE;
}

acfg_entry_t *acfg_find( const char *key ) {
	int i;

	for ( i = 0; acfg_table[i].key != NULL; i++ ) {
		if ( !str_cmp( key, acfg_table[i].key ) )
			return &acfg_table[i];
	}

	return NULL;
}

acfg_entry_t *acfg_find_by_index( int index ) {
	if ( index < 0 || index >= acfg_count() )
		return NULL;
	return &acfg_table[index];
}

int acfg_count( void ) {
	static int cached = -1;
	int i;

	if ( cached >= 0 )
		return cached;

	for ( i = 0; acfg_table[i].key != NULL; i++ )
		;

	cached = i;
	return cached;
}


/* ================================================================
 * Load / Save wrappers
 * ================================================================ */

void load_ability_config( void ) {
	char buf[MAX_STRING_LENGTH];

	/* Defaults are already baked into the table initializer.
	 * Just load overrides from the DB. */
	db_game_load_ability_config();

	snprintf( buf, sizeof( buf ),
		"  Ability config: %d entries registered", acfg_count() );
	log_string( buf );
}

void save_ability_config( void ) {
	db_game_save_ability_config();
}


/* ================================================================
 * Admin command: do_ability
 * ================================================================ */

/*
 * Extract the class prefix from a dotted key (everything before the
 * first dot).  Returns a pointer to a static buffer.
 */
static const char *acfg_class_prefix( const char *key ) {
	static char buf[64];
	const char *dot = strchr( key, '.' );

	if ( !dot ) {
		snprintf( buf, sizeof( buf ), "%s", key );
		return buf;
	}

	{
		size_t len = (size_t)( dot - key );
		if ( len >= sizeof( buf ) )
			len = sizeof( buf ) - 1;
		memcpy( buf, key, len );
		buf[len] = '\0';
	}
	return buf;
}


void do_ability( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char output[MAX_STRING_LENGTH * 4];
	int i, total, count;

	argument = one_argument( argument, arg );
	argument = one_argument( argument, arg2 );
	total = acfg_count();

	/* No args: show summary by class */
	if ( arg[0] == '\0' ) {
		const char *last_class = "";
		int class_count = 0;
		int total_classes = 0;

		output[0] = '\0';
		snprintf( buf, sizeof( buf ),
			"#yAbility Config#n  (%d total entries)\r\n"
			"#y---------------------------------------------#n\r\n",
			total );
		strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

		for ( i = 0; i < total; i++ ) {
			const char *cls = acfg_class_prefix( acfg_table[i].key );
			if ( str_cmp( cls, last_class ) ) {
				if ( last_class[0] != '\0' ) {
					snprintf( buf, sizeof( buf ),
						"  #g%-20s#n  %d entries\r\n",
						last_class, class_count );
					strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
					total_classes++;
				}
				last_class = cls;
				class_count = 1;
			} else {
				class_count++;
			}
		}
		if ( last_class[0] != '\0' ) {
			snprintf( buf, sizeof( buf ),
				"  #g%-20s#n  %d entries\r\n",
				last_class, class_count );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
			total_classes++;
		}

		snprintf( buf, sizeof( buf ),
			"#y---------------------------------------------#n\r\n"
			"%d classes.  Use #yability <class>#n for details.\r\n"
			"#yability <key> <value>#n to modify.  "
			"#yability reload#n to reload from DB.\r\n",
			total_classes );
		strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

		send_to_char( output, ch );
		return;
	}

	/* "ability reload" - reload from DB */
	if ( !str_cmp( arg, "reload" ) ) {
		/* Reset all to defaults first */
		for ( i = 0; i < total; i++ )
			acfg_table[i].value = acfg_table[i].default_value;
		db_game_load_ability_config();
		send_to_char( "Ability config reloaded from database.\r\n", ch );
		return;
	}

	/* "ability defaults" - reset all to defaults */
	if ( !str_cmp( arg, "defaults" ) ) {
		for ( i = 0; i < total; i++ )
			acfg_table[i].value = acfg_table[i].default_value;
		db_game_save_ability_config();
		send_to_char( "All ability config values reset to defaults.\r\n", ch );
		return;
	}

	/* "ability reset <key>" - reset single key */
	if ( !str_cmp( arg, "reset" ) ) {
		acfg_entry_t *e;

		if ( arg2[0] == '\0' ) {
			send_to_char( "Reset which key?  Usage: ability reset <key>\r\n", ch );
			return;
		}

		e = acfg_find( arg2 );
		if ( !e ) {
			send_to_char( "Unknown ability config key.\r\n", ch );
			return;
		}

		e->value = e->default_value;
		db_game_save_ability_config();
		snprintf( buf, sizeof( buf ),
			"Reset #g%s#n to default: #y%d#n\r\n",
			e->key, e->default_value );
		send_to_char( buf, ch );
		return;
	}

	/* "ability <key> <value>" - set a value */
	if ( arg2[0] != '\0' ) {
		acfg_entry_t *e = acfg_find( arg );
		int val;

		if ( !e ) {
			send_to_char( "Unknown ability config key.\r\n", ch );
			return;
		}

		val = atoi( arg2 );
		e->value = val;
		db_game_save_ability_config();
		snprintf( buf, sizeof( buf ),
			"Set #g%s#n = #y%d#n (default: %d)\r\n",
			e->key, e->value, e->default_value );
		send_to_char( buf, ch );
		return;
	}

	/* "ability <prefix>" - show matching entries */
	output[0] = '\0';
	count = 0;

	for ( i = 0; i < total; i++ ) {
		if ( !str_prefix( arg, acfg_table[i].key ) ) {
			const char *modified =
				( acfg_table[i].value != acfg_table[i].default_value )
				? " #r*#n" : "";

			snprintf( buf, sizeof( buf ),
				"  #g%-50s#n  #y%d#n  (default: %d)%s\r\n",
				acfg_table[i].key,
				acfg_table[i].value,
				acfg_table[i].default_value,
				modified );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
			count++;
		}
	}

	if ( count == 0 ) {
		send_to_char( "No matching ability config keys found.\r\n", ch );
		return;
	}

	snprintf( buf, sizeof( buf ),
		"#y--- %d matching entries ---#n\r\n", count );
	strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

	send_to_char( output, ch );
}
