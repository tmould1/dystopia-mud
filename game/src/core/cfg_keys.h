/***************************************************************************
 *  cfg_keys.h - Unified configuration keys (auto-generated)
 *
 *  This file is the SINGLE SOURCE OF TRUTH for all game config entries.
 *  The enum and table are both generated from the CFG_ENTRIES macro.
 *
 *  Categories:
 *    CFG_COMBAT_*      - Global combat parameters
 *    CFG_PROGRESSION_* - Upgrade/generation bonuses
 *    CFG_WORLD_*       - Time, weather, world settings
 *    CFG_ABILITY_*     - Per-class ability parameters
 *
 *  DO NOT EDIT MANUALLY - regenerate using:
 *      python game/tools/generate_cfg_keys.py
 ***************************************************************************/

#ifndef CFG_KEYS_H
#define CFG_KEYS_H

/*
 * X-Macro definitions for config keys.
 * Format: CFG_X(ENUM_NAME, "string.key", default_value)
 */
#define CFG_ENTRIES \
    \
    /* =========== COMBAT =========== */ \
    CFG_X(COMBAT_BASE_DAMCAP                                     , "combat.base_damcap",       1000) \
    CFG_X(COMBAT_BUILDER_DAMCAP                                  , "combat.builder_damcap",      30000) \
    CFG_X(COMBAT_PVP_DAMAGE_DIVISOR                              , "combat.pvp_damage_divisor",          2) \
    CFG_X(COMBAT_SLEEP_DAMAGE_MULTIPLIER                         , "combat.sleep_damage_multiplier",          2) \
    CFG_X(COMBAT_WPN_DAM_DIVISOR                                 , "combat.wpn_dam_divisor",         60) \
    CFG_X(COMBAT_WPN_DAM_SKILL_CAP                               , "combat.wpn_dam_skill_cap",        350) \
    CFG_X(COMBAT_WPN_GEN1_BONUS                                  , "combat.wpn_gen1_bonus",         20) \
    CFG_X(COMBAT_WPN_GEN2_BONUS                                  , "combat.wpn_gen2_bonus",         10) \
    \
    /* =========== COMBAT - DAMCAP =========== */ \
    CFG_X(COMBAT_DAMCAP_ARTIFACT                                 , "combat.damcap.artifact",        500) \
    CFG_X(COMBAT_DAMCAP_MAGE                                     , "combat.damcap.mage",        500) \
    \
    /* =========== COMBAT - DAMCAP - ANGEL =========== */ \
    CFG_X(COMBAT_DAMCAP_ANGEL_PER_POWER                          , "combat.damcap.angel.per_power",        125) \
    \
    /* =========== COMBAT - DAMCAP - DEMON =========== */ \
    CFG_X(COMBAT_DAMCAP_DEMON_BASE                               , "combat.damcap.demon.base",        500) \
    CFG_X(COMBAT_DAMCAP_DEMON_HELL                               , "combat.damcap.demon.hell",       6000) \
    CFG_X(COMBAT_DAMCAP_DEMON_POWER_MULT                         , "combat.damcap.demon.power_mult",          2) \
    CFG_X(COMBAT_DAMCAP_DEMON_SOUL_CAP                           , "combat.damcap.demon.soul_cap",        350) \
    CFG_X(COMBAT_DAMCAP_DEMON_SOUL_MULT                          , "combat.damcap.demon.soul_mult",         70) \
    \
    /* =========== COMBAT - DAMCAP - DIRGESINGER =========== */ \
    CFG_X(COMBAT_DAMCAP_DIRGESINGER_BASE                         , "combat.damcap.dirgesinger.base",        200) \
    CFG_X(COMBAT_DAMCAP_DIRGESINGER_BATTLEHYMN                   , "combat.damcap.dirgesinger.battlehymn",        200) \
    CFG_X(COMBAT_DAMCAP_DIRGESINGER_RES_MULT                     , "combat.damcap.dirgesinger.res_mult",          7) \
    \
    /* =========== COMBAT - DAMCAP - DRAGONKIN =========== */ \
    CFG_X(COMBAT_DAMCAP_DRAGONKIN_BASE                           , "combat.damcap.dragonkin.base",        200) \
    CFG_X(COMBAT_DAMCAP_DRAGONKIN_DRAGONHIDE                     , "combat.damcap.dragonkin.dragonhide",        150) \
    CFG_X(COMBAT_DAMCAP_DRAGONKIN_ESSENCE_MULT                   , "combat.damcap.dragonkin.essence_mult",          5) \
    \
    /* =========== COMBAT - DAMCAP - DROID =========== */ \
    CFG_X(COMBAT_DAMCAP_DROID_PER_LIMB                           , "combat.damcap.droid.per_limb",        450) \
    \
    /* =========== COMBAT - DAMCAP - DROW =========== */ \
    CFG_X(COMBAT_DAMCAP_DROW_BASE                                , "combat.damcap.drow.base",        500) \
    CFG_X(COMBAT_DAMCAP_DROW_FORM                                , "combat.damcap.drow.form",        650) \
    CFG_X(COMBAT_DAMCAP_DROW_HATE                                , "combat.damcap.drow.hate",        650) \
    \
    /* =========== COMBAT - DAMCAP - LICH =========== */ \
    CFG_X(COMBAT_DAMCAP_LICH_BASE                                , "combat.damcap.lich.base",        450) \
    CFG_X(COMBAT_DAMCAP_LICH_LORE                                , "combat.damcap.lich.lore",        350) \
    \
    /* =========== COMBAT - DAMCAP - MAGE =========== */ \
    CFG_X(COMBAT_DAMCAP_MAGE_BEAST                               , "combat.damcap.mage.beast",        750) \
    \
    /* =========== COMBAT - DAMCAP - MONK =========== */ \
    CFG_X(COMBAT_DAMCAP_MONK_CHI_MULT                            , "combat.damcap.monk.chi_mult",        200) \
    CFG_X(COMBAT_DAMCAP_MONK_COMBAT_MULT                         , "combat.damcap.monk.combat_mult",        100) \
    \
    /* =========== COMBAT - DAMCAP - NINJA =========== */ \
    CFG_X(COMBAT_DAMCAP_NINJA_CHIKYU_HIGH                        , "combat.damcap.ninja.chikyu_high",        500) \
    CFG_X(COMBAT_DAMCAP_NINJA_CHIKYU_LOW                         , "combat.damcap.ninja.chikyu_low",        500) \
    CFG_X(COMBAT_DAMCAP_NINJA_RAGE_MULT                          , "combat.damcap.ninja.rage_mult",          4) \
    \
    /* =========== COMBAT - DAMCAP - REV_SUPERSTANCE =========== */ \
    CFG_X(COMBAT_DAMCAP_REV_SUPERSTANCE_1                        , "combat.damcap.rev_superstance.1",        250) \
    CFG_X(COMBAT_DAMCAP_REV_SUPERSTANCE_2                        , "combat.damcap.rev_superstance.2",        400) \
    CFG_X(COMBAT_DAMCAP_REV_SUPERSTANCE_3                        , "combat.damcap.rev_superstance.3",        550) \
    \
    /* =========== COMBAT - DAMCAP - SAMURAI =========== */ \
    CFG_X(COMBAT_DAMCAP_SAMURAI_BASE                             , "combat.damcap.samurai.base",        375) \
    CFG_X(COMBAT_DAMCAP_SAMURAI_PER_WPN                          , "combat.damcap.samurai.per_wpn",        175) \
    \
    /* =========== COMBAT - DAMCAP - SHAPE =========== */ \
    CFG_X(COMBAT_DAMCAP_SHAPE_BASE                               , "combat.damcap.shape.base",        800) \
    CFG_X(COMBAT_DAMCAP_SHAPE_BULL                               , "combat.damcap.shape.bull",        300) \
    CFG_X(COMBAT_DAMCAP_SHAPE_FAERIE                             , "combat.damcap.shape.faerie",        275) \
    CFG_X(COMBAT_DAMCAP_SHAPE_HYDRA                              , "combat.damcap.shape.hydra",        350) \
    CFG_X(COMBAT_DAMCAP_SHAPE_TIGER                              , "combat.damcap.shape.tiger",        325) \
    \
    /* =========== COMBAT - DAMCAP - SIREN =========== */ \
    CFG_X(COMBAT_DAMCAP_SIREN_ECHOSHIELD                         , "combat.damcap.siren.echoshield",        150) \
    CFG_X(COMBAT_DAMCAP_SIREN_RES_MULT                           , "combat.damcap.siren.res_mult",          7) \
    \
    /* =========== COMBAT - DAMCAP - STANCE =========== */ \
    CFG_X(COMBAT_DAMCAP_STANCE_BULL                              , "combat.damcap.stance.bull",        200) \
    CFG_X(COMBAT_DAMCAP_STANCE_CRAB                              , "combat.damcap.stance.crab",        250) \
    CFG_X(COMBAT_DAMCAP_STANCE_DRAGON                            , "combat.damcap.stance.dragon",        250) \
    CFG_X(COMBAT_DAMCAP_STANCE_DRAGON_DEF                        , "combat.damcap.stance.dragon_def",        250) \
    CFG_X(COMBAT_DAMCAP_STANCE_SWALLOW                           , "combat.damcap.stance.swallow",        250) \
    CFG_X(COMBAT_DAMCAP_STANCE_TIGER                             , "combat.damcap.stance.tiger",        200) \
    CFG_X(COMBAT_DAMCAP_STANCE_WOLF                              , "combat.damcap.stance.wolf",        250) \
    \
    /* =========== COMBAT - DAMCAP - SUPERSTANCE =========== */ \
    CFG_X(COMBAT_DAMCAP_SUPERSTANCE_1                            , "combat.damcap.superstance.1",        250) \
    CFG_X(COMBAT_DAMCAP_SUPERSTANCE_2                            , "combat.damcap.superstance.2",        400) \
    CFG_X(COMBAT_DAMCAP_SUPERSTANCE_3                            , "combat.damcap.superstance.3",        550) \
    \
    /* =========== COMBAT - DAMCAP - TANARRI =========== */ \
    CFG_X(COMBAT_DAMCAP_TANARRI_PER_RANK                         , "combat.damcap.tanarri.per_rank",        375) \
    \
    /* =========== COMBAT - DAMCAP - UK =========== */ \
    CFG_X(COMBAT_DAMCAP_UK_WPN_MULT                              , "combat.damcap.uk.wpn_mult",        275) \
    \
    /* =========== COMBAT - DAMCAP - VAMP =========== */ \
    CFG_X(COMBAT_DAMCAP_VAMP_ANCILLA                             , "combat.damcap.vamp.ancilla",        100) \
    CFG_X(COMBAT_DAMCAP_VAMP_ELDER                               , "combat.damcap.vamp.elder",        250) \
    CFG_X(COMBAT_DAMCAP_VAMP_FORT_MULT                           , "combat.damcap.vamp.fort_mult",         50) \
    CFG_X(COMBAT_DAMCAP_VAMP_LAMAGRA                             , "combat.damcap.vamp.lamagra",        500) \
    CFG_X(COMBAT_DAMCAP_VAMP_METHUSELAH                          , "combat.damcap.vamp.methuselah",        400) \
    CFG_X(COMBAT_DAMCAP_VAMP_POTE_MULT                           , "combat.damcap.vamp.pote_mult",         50) \
    CFG_X(COMBAT_DAMCAP_VAMP_RAGE_MULT                           , "combat.damcap.vamp.rage_mult",          5) \
    CFG_X(COMBAT_DAMCAP_VAMP_TRUEBLOOD                           , "combat.damcap.vamp.trueblood",        600) \
    \
    /* =========== COMBAT - DAMCAP - WW =========== */ \
    CFG_X(COMBAT_DAMCAP_WW_HIGH_RAGE                             , "combat.damcap.ww.high_rage",        400) \
    CFG_X(COMBAT_DAMCAP_WW_PAIN                                  , "combat.damcap.ww.pain",        750) \
    CFG_X(COMBAT_DAMCAP_WW_RAGE_MULT                             , "combat.damcap.ww.rage_mult",          1) \
    \
    /* =========== COMBAT - DAMCAP - WYRM =========== */ \
    CFG_X(COMBAT_DAMCAP_WYRM_ESSENCE_MULT                        , "combat.damcap.wyrm.essence_mult",          7) \
    CFG_X(COMBAT_DAMCAP_WYRM_WYRMFORM                            , "combat.damcap.wyrm.wyrmform",        300) \
    \
    /* =========== COMBAT - DMG_MULT =========== */ \
    CFG_X(COMBAT_DMG_MULT_DEMON_MIGHT                            , "combat.dmg_mult.demon_might",        110) \
    CFG_X(COMBAT_DMG_MULT_DEMON_STRONGARMS                       , "combat.dmg_mult.demon_strongarms",        120) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_1                           , "combat.dmg_mult.ninja_belt_1",        110) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_10                          , "combat.dmg_mult.ninja_belt_10",        200) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_2                           , "combat.dmg_mult.ninja_belt_2",        120) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_3                           , "combat.dmg_mult.ninja_belt_3",        130) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_4                           , "combat.dmg_mult.ninja_belt_4",        140) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_5                           , "combat.dmg_mult.ninja_belt_5",        150) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_6                           , "combat.dmg_mult.ninja_belt_6",        160) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_7                           , "combat.dmg_mult.ninja_belt_7",        170) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_8                           , "combat.dmg_mult.ninja_belt_8",        180) \
    CFG_X(COMBAT_DMG_MULT_NINJA_BELT_9                           , "combat.dmg_mult.ninja_belt_9",        190) \
    CFG_X(COMBAT_DMG_MULT_TANARRI_MIGHT                          , "combat.dmg_mult.tanarri_might",        150) \
    CFG_X(COMBAT_DMG_MULT_UK_VS_SHAPE                            , "combat.dmg_mult.uk_vs_shape",        120) \
    \
    /* =========== COMBAT - WPN_CAP =========== */ \
    CFG_X(COMBAT_WPN_CAP_ANGEL                                   , "combat.wpn_cap.angel",        500) \
    CFG_X(COMBAT_WPN_CAP_DEFAULT                                 , "combat.wpn_cap.default",        200) \
    CFG_X(COMBAT_WPN_CAP_DROID                                   , "combat.wpn_cap.droid",        450) \
    CFG_X(COMBAT_WPN_CAP_DROW                                    , "combat.wpn_cap.drow",        300) \
    CFG_X(COMBAT_WPN_CAP_HARD_CAP                                , "combat.wpn_cap.hard_cap",       1100) \
    CFG_X(COMBAT_WPN_CAP_LICH                                    , "combat.wpn_cap.lich",        350) \
    CFG_X(COMBAT_WPN_CAP_LOW_LEVEL                               , "combat.wpn_cap.low_level",        200) \
    CFG_X(COMBAT_WPN_CAP_MONK                                    , "combat.wpn_cap.monk",        800) \
    CFG_X(COMBAT_WPN_CAP_SAMURAI                                 , "combat.wpn_cap.samurai",       1000) \
    CFG_X(COMBAT_WPN_CAP_SHAPE                                   , "combat.wpn_cap.shape",        400) \
    CFG_X(COMBAT_WPN_CAP_WEREWOLF                                , "combat.wpn_cap.werewolf",        350) \
    \
    /* =========== PROGRESSION =========== */ \
    CFG_X(PROGRESSION_GEN_DAMCAP_0                               , "progression.gen_damcap_0",        250) \
    CFG_X(PROGRESSION_GEN_DAMCAP_1                               , "progression.gen_damcap_1",        200) \
    CFG_X(PROGRESSION_GEN_DAMCAP_2                               , "progression.gen_damcap_2",        150) \
    CFG_X(PROGRESSION_GEN_DAMCAP_3                               , "progression.gen_damcap_3",        100) \
    CFG_X(PROGRESSION_GEN_DAMCAP_4                               , "progression.gen_damcap_4",         50) \
    CFG_X(PROGRESSION_UPGRADE_DAMCAP_PER_LEVEL                   , "progression.upgrade_damcap_per_level",        500) \
    CFG_X(PROGRESSION_UPGRADE_DMG_1                              , "progression.upgrade_dmg_1",        105) \
    CFG_X(PROGRESSION_UPGRADE_DMG_2                              , "progression.upgrade_dmg_2",        110) \
    CFG_X(PROGRESSION_UPGRADE_DMG_3                              , "progression.upgrade_dmg_3",        115) \
    CFG_X(PROGRESSION_UPGRADE_DMG_4                              , "progression.upgrade_dmg_4",        120) \
    CFG_X(PROGRESSION_UPGRADE_DMG_5                              , "progression.upgrade_dmg_5",        125) \
    \
    /* =========== WORLD =========== */ \
    CFG_X(WORLD_TIME_SCALE                                       , "world.time_scale",          5) \
    \
    /* =========== ABILITY - ANGEL =========== */ \
    CFG_X(ABILITY_ANGEL_ANGELICARMOR_PRACTICE_COST               , "ability.angel.angelicarmor.practice_cost",        150) \
    CFG_X(ABILITY_ANGEL_ANGELICAURA_LEVEL_REQ                    , "ability.angel.angelicaura.level_req",          2) \
    CFG_X(ABILITY_ANGEL_AWINGS_LEVEL_REQ                         , "ability.angel.awings.level_req",          1) \
    CFG_X(ABILITY_ANGEL_EYEFORANEYE_LEVEL_REQ                    , "ability.angel.eyeforaneye.level_req",          5) \
    CFG_X(ABILITY_ANGEL_FORGIVNESS_COOLDOWN                      , "ability.angel.forgivness.cooldown",         16) \
    CFG_X(ABILITY_ANGEL_FORGIVNESS_HEAL_MAX                      , "ability.angel.forgivness.heal_max",       1500) \
    CFG_X(ABILITY_ANGEL_FORGIVNESS_HEAL_MIN                      , "ability.angel.forgivness.heal_min",       1000) \
    CFG_X(ABILITY_ANGEL_FORGIVNESS_LEVEL_REQ                     , "ability.angel.forgivness.level_req",          3) \
    CFG_X(ABILITY_ANGEL_GBANISH_COOLDOWN                         , "ability.angel.gbanish.cooldown",         18) \
    CFG_X(ABILITY_ANGEL_GBANISH_DAM_EVIL                         , "ability.angel.gbanish.dam_evil",       1500) \
    CFG_X(ABILITY_ANGEL_GBANISH_DAM_GOOD                         , "ability.angel.gbanish.dam_good",        500) \
    CFG_X(ABILITY_ANGEL_GBANISH_DAM_NEUTRAL                      , "ability.angel.gbanish.dam_neutral",       1000) \
    CFG_X(ABILITY_ANGEL_GBANISH_LEVEL_REQ                        , "ability.angel.gbanish.level_req",          3) \
    CFG_X(ABILITY_ANGEL_GFAVOR_DAMROLL_BONUS                     , "ability.angel.gfavor.damroll_bonus",        400) \
    CFG_X(ABILITY_ANGEL_GFAVOR_HITROLL_BONUS                     , "ability.angel.gfavor.hitroll_bonus",        400) \
    CFG_X(ABILITY_ANGEL_GFAVOR_LEVEL_REQ                         , "ability.angel.gfavor.level_req",          2) \
    CFG_X(ABILITY_ANGEL_GFAVOR_MANA_COST                         , "ability.angel.gfavor.mana_cost",       2000) \
    CFG_X(ABILITY_ANGEL_GFAVOR_MOVE_COST                         , "ability.angel.gfavor.move_cost",       2000) \
    CFG_X(ABILITY_ANGEL_GPEACE_LEVEL_REQ                         , "ability.angel.gpeace.level_req",          1) \
    CFG_X(ABILITY_ANGEL_GSENSES_LEVEL_REQ                        , "ability.angel.gsenses.level_req",          1) \
    CFG_X(ABILITY_ANGEL_HALO_LEVEL_REQ                           , "ability.angel.halo.level_req",          2) \
    CFG_X(ABILITY_ANGEL_HARMONY_COOLDOWN                         , "ability.angel.harmony.cooldown",         12) \
    CFG_X(ABILITY_ANGEL_HARMONY_LEVEL_REQ                        , "ability.angel.harmony.level_req",          5) \
    CFG_X(ABILITY_ANGEL_HOUSEOFGOD_COOLDOWN                      , "ability.angel.houseofgod.cooldown",         24) \
    CFG_X(ABILITY_ANGEL_HOUSEOFGOD_LEVEL_REQ                     , "ability.angel.houseofgod.level_req",          5) \
    CFG_X(ABILITY_ANGEL_HOUSEOFGOD_PEACE_COUNTER                 , "ability.angel.houseofgod.peace_counter",         50) \
    CFG_X(ABILITY_ANGEL_INNERPEACE_COOLDOWN                      , "ability.angel.innerpeace.cooldown",         18) \
    CFG_X(ABILITY_ANGEL_INNERPEACE_HEAL_MULTIPLIER               , "ability.angel.innerpeace.heal_multiplier",        500) \
    CFG_X(ABILITY_ANGEL_INNERPEACE_LEVEL_REQ                     , "ability.angel.innerpeace.level_req",          3) \
    CFG_X(ABILITY_ANGEL_INNERPEACE_MANA_COST                     , "ability.angel.innerpeace.mana_cost",       1500) \
    CFG_X(ABILITY_ANGEL_MARTYR_COOLDOWN                          , "ability.angel.martyr.cooldown",          6) \
    CFG_X(ABILITY_ANGEL_MARTYR_LEVEL_REQ                         , "ability.angel.martyr.level_req",          5) \
    CFG_X(ABILITY_ANGEL_SINSOFTHEPAST_COOLDOWN                   , "ability.angel.sinsofthepast.cooldown",         12) \
    CFG_X(ABILITY_ANGEL_SINSOFTHEPAST_LEVEL_REQ                  , "ability.angel.sinsofthepast.level_req",          3) \
    CFG_X(ABILITY_ANGEL_SPIRITFORM_LEVEL_REQ                     , "ability.angel.spiritform.level_req",          2) \
    CFG_X(ABILITY_ANGEL_SWOOP_LEVEL_REQ                          , "ability.angel.swoop.level_req",          1) \
    CFG_X(ABILITY_ANGEL_SWOOP_MOVE_COST                          , "ability.angel.swoop.move_cost",        500) \
    CFG_X(ABILITY_ANGEL_TOUCHOFGOD_COOLDOWN                      , "ability.angel.touchofgod.cooldown",         18) \
    CFG_X(ABILITY_ANGEL_TOUCHOFGOD_DAM_MAX                       , "ability.angel.touchofgod.dam_max",        200) \
    CFG_X(ABILITY_ANGEL_TOUCHOFGOD_DAM_MIN                       , "ability.angel.touchofgod.dam_min",        100) \
    CFG_X(ABILITY_ANGEL_TOUCHOFGOD_LEVEL_REQ                     , "ability.angel.touchofgod.level_req",          4) \
    \
    /* =========== ABILITY - CLAN =========== */ \
    CFG_X(ABILITY_CLAN_ACID_BLOOD_REQ                            , "ability.clan.acid.blood_req",        500) \
    CFG_X(ABILITY_CLAN_ACID_LEVEL_REQ                            , "ability.clan.acid.level_req",          4) \
    CFG_X(ABILITY_CLAN_AWE_LEVEL_REQ                             , "ability.clan.awe.level_req",          1) \
    CFG_X(ABILITY_CLAN_BEASTLIKE_BEAST_CHANCE_DIVISOR            , "ability.clan.beastlike.beast_chance_divisor",        500) \
    CFG_X(ABILITY_CLAN_CALM_DEMON_COOLDOWN                       , "ability.clan.calm_demon.cooldown",          8) \
    CFG_X(ABILITY_CLAN_CALM_DEMON_LEVEL_REQ                      , "ability.clan.calm_demon.level_req",          4) \
    CFG_X(ABILITY_CLAN_CALM_NINJA_COOLDOWN                       , "ability.clan.calm_ninja.cooldown",         12) \
    CFG_X(ABILITY_CLAN_CALM_WW_COOLDOWN                          , "ability.clan.calm_ww.cooldown",         12) \
    CFG_X(ABILITY_CLAN_CALM_WW_LEVEL_REQ                         , "ability.clan.calm_ww.level_req",          3) \
    CFG_X(ABILITY_CLAN_CALM_WW_RAGE_LOSS_MAX                     , "ability.clan.calm_ww.rage_loss_max",         90) \
    CFG_X(ABILITY_CLAN_CALM_WW_RAGE_LOSS_MIN                     , "ability.clan.calm_ww.rage_loss_min",         60) \
    CFG_X(ABILITY_CLAN_CALM_WW_UNSHIFT_THRESHOLD                 , "ability.clan.calm_ww.unshift_threshold",        100) \
    CFG_X(ABILITY_CLAN_CHANGE_LEVEL_REQ                          , "ability.clan.change.level_req",          3) \
    CFG_X(ABILITY_CLAN_CHANGE_BAT_BLOOD_COST_MAX                 , "ability.clan.change_bat.blood_cost_max",         50) \
    CFG_X(ABILITY_CLAN_CHANGE_BAT_BLOOD_COST_MIN                 , "ability.clan.change_bat.blood_cost_min",         40) \
    CFG_X(ABILITY_CLAN_CHANGE_BAT_BLOOD_REQ                      , "ability.clan.change_bat.blood_req",         50) \
    CFG_X(ABILITY_CLAN_CHANGE_MIST_BLOOD_COST_MAX                , "ability.clan.change_mist.blood_cost_max",         50) \
    CFG_X(ABILITY_CLAN_CHANGE_MIST_BLOOD_COST_MIN                , "ability.clan.change_mist.blood_cost_min",         40) \
    CFG_X(ABILITY_CLAN_CHANGE_MIST_BLOOD_REQ                     , "ability.clan.change_mist.blood_req",         50) \
    CFG_X(ABILITY_CLAN_CHANGE_WOLF_BLOOD_COST_MAX                , "ability.clan.change_wolf.blood_cost_max",         50) \
    CFG_X(ABILITY_CLAN_CHANGE_WOLF_BLOOD_COST_MIN                , "ability.clan.change_wolf.blood_cost_min",         40) \
    CFG_X(ABILITY_CLAN_CHANGE_WOLF_BLOOD_REQ                     , "ability.clan.change_wolf.blood_req",         50) \
    CFG_X(ABILITY_CLAN_CHANGE_WOLF_MOD_STR                       , "ability.clan.change_wolf.mod_str",         10) \
    CFG_X(ABILITY_CLAN_CLANDISC_FORTITUDE_MAX                    , "ability.clan.clandisc.fortitude_max",         12) \
    CFG_X(ABILITY_CLAN_CLANDISC_MAX_LEVEL                        , "ability.clan.clandisc.max_level",         10) \
    CFG_X(ABILITY_CLAN_CLASS_VAMPIRE_DEFAULT_BEAST               , "ability.clan.class_vampire.default_beast",         30) \
    CFG_X(ABILITY_CLAN_CLAWS_LEVEL_REQ_DEMON                     , "ability.clan.claws.level_req_demon",          1) \
    CFG_X(ABILITY_CLAN_CLAWS_LEVEL_REQ_VAMPIRE                   , "ability.clan.claws.level_req_vampire",          2) \
    CFG_X(ABILITY_CLAN_CLAWS_LEVEL_REQ_WEREWOLF                  , "ability.clan.claws.level_req_werewolf",          1) \
    CFG_X(ABILITY_CLAN_COIL_LEVEL_REQ                            , "ability.clan.coil.level_req",          8) \
    CFG_X(ABILITY_CLAN_DARKHEART_BLOOD_COST                      , "ability.clan.darkheart.blood_cost",        100) \
    CFG_X(ABILITY_CLAN_DARKHEART_LEVEL_REQ                       , "ability.clan.darkheart.level_req",          1) \
    CFG_X(ABILITY_CLAN_DARKHEART_SELF_DAMAGE_MAX                 , "ability.clan.darkheart.self_damage_max",         20) \
    CFG_X(ABILITY_CLAN_DARKHEART_SELF_DAMAGE_MIN                 , "ability.clan.darkheart.self_damage_min",         10) \
    CFG_X(ABILITY_CLAN_DEATH_BLOOD_COST                          , "ability.clan.death.blood_cost",        300) \
    CFG_X(ABILITY_CLAN_DEATH_LEVEL_REQ                           , "ability.clan.death.level_req",          5) \
    CFG_X(ABILITY_CLAN_DEATH_SILENCE_DURATION                    , "ability.clan.death.silence_duration",         10) \
    CFG_X(ABILITY_CLAN_DEMONFORM_ARMOR_BONUS                     , "ability.clan.demonform.armor_bonus",        300) \
    CFG_X(ABILITY_CLAN_DEMONFORM_DAMROLL_BONUS                   , "ability.clan.demonform.damroll_bonus",        200) \
    CFG_X(ABILITY_CLAN_DEMONFORM_HITROLL_BONUS                   , "ability.clan.demonform.hitroll_bonus",        200) \
    CFG_X(ABILITY_CLAN_DEMONFORM_MOD_DEX                         , "ability.clan.demonform.mod_dex",         15) \
    CFG_X(ABILITY_CLAN_DEMONFORM_MOD_STR                         , "ability.clan.demonform.mod_str",         15) \
    CFG_X(ABILITY_CLAN_DRAGON_BLOOD_COST                         , "ability.clan.dragon.blood_cost",          6) \
    CFG_X(ABILITY_CLAN_DRAGON_COOLDOWN                           , "ability.clan.dragon.cooldown",         12) \
    CFG_X(ABILITY_CLAN_DRAGON_LEVEL_REQ                          , "ability.clan.dragon.level_req",          8) \
    CFG_X(ABILITY_CLAN_EARTHMELD_LEVEL_REQ                       , "ability.clan.earthmeld.level_req",          4) \
    CFG_X(ABILITY_CLAN_FANGS_LEVEL_REQ_DEMON                     , "ability.clan.fangs.level_req_demon",          2) \
    CFG_X(ABILITY_CLAN_FANGS_LEVEL_REQ_WEREWOLF                  , "ability.clan.fangs.level_req_werewolf",          2) \
    CFG_X(ABILITY_CLAN_FCOMMAND_LEVEL_REQ                        , "ability.clan.fcommand.level_req",          3) \
    CFG_X(ABILITY_CLAN_FLASH_BLOOD_COST                          , "ability.clan.flash.blood_cost",        200) \
    CFG_X(ABILITY_CLAN_FLASH_CELE_BONUS                          , "ability.clan.flash.cele_bonus",          2) \
    CFG_X(ABILITY_CLAN_FLASH_LEVEL_REQ                           , "ability.clan.flash.level_req",          9) \
    CFG_X(ABILITY_CLAN_FLEX_COOLDOWN                             , "ability.clan.flex.cooldown",         12) \
    CFG_X(ABILITY_CLAN_FORGET_BLOOD_COST                         , "ability.clan.forget.blood_cost",        250) \
    CFG_X(ABILITY_CLAN_FORGET_COOLDOWN                           , "ability.clan.forget.cooldown",         12) \
    CFG_X(ABILITY_CLAN_FORGET_LEVEL_REQ                          , "ability.clan.forget.level_req",          8) \
    CFG_X(ABILITY_CLAN_FORGET_PRIMAL_COST                        , "ability.clan.forget.primal_cost",         25) \
    CFG_X(ABILITY_CLAN_FORGET_SUCCESS_CHANCE                     , "ability.clan.forget.success_chance",         10) \
    CFG_X(ABILITY_CLAN_HORN_LEVEL_REQ                            , "ability.clan.horn.level_req",          4) \
    CFG_X(ABILITY_CLAN_HUMANITY_BEAST_CHANCE_DIVISOR             , "ability.clan.humanity.beast_chance_divisor",        300) \
    CFG_X(ABILITY_CLAN_HUMANITY_GOLCONDA_EXP_REWARD              , "ability.clan.humanity.golconda_exp_reward",    1000000) \
    CFG_X(ABILITY_CLAN_INCONNU_EXP_COST                          , "ability.clan.inconnu.exp_cost",    1000000) \
    CFG_X(ABILITY_CLAN_KORYOU_LEVEL_REQ                          , "ability.clan.koryou.level_req",          2) \
    CFG_X(ABILITY_CLAN_MAJESTY_BLOOD_COST_MAX                    , "ability.clan.majesty.blood_cost_max",         45) \
    CFG_X(ABILITY_CLAN_MAJESTY_BLOOD_COST_MIN                    , "ability.clan.majesty.blood_cost_min",         35) \
    CFG_X(ABILITY_CLAN_MAJESTY_BLOOD_REQ                         , "ability.clan.majesty.blood_req",         45) \
    CFG_X(ABILITY_CLAN_MAJESTY_LEVEL_REQ                         , "ability.clan.majesty.level_req",          5) \
    CFG_X(ABILITY_CLAN_MASK_BLOOD_COST_MAX                       , "ability.clan.mask.blood_cost_max",         40) \
    CFG_X(ABILITY_CLAN_MASK_BLOOD_COST_MIN                       , "ability.clan.mask.blood_cost_min",         30) \
    CFG_X(ABILITY_CLAN_MASK_BLOOD_REQ                            , "ability.clan.mask.blood_req",         40) \
    CFG_X(ABILITY_CLAN_MASK_LEVEL_REQ_SHAPESHIFTER               , "ability.clan.mask.level_req_shapeshifter",          4) \
    CFG_X(ABILITY_CLAN_MASK_LEVEL_REQ_VAMPIRE                    , "ability.clan.mask.level_req_vampire",          2) \
    CFG_X(ABILITY_CLAN_MITSUKERU_LEVEL_REQ                       , "ability.clan.mitsukeru.level_req",          1) \
    CFG_X(ABILITY_CLAN_MORTAL_LEVEL_REQ                          , "ability.clan.mortal.level_req",          4) \
    CFG_X(ABILITY_CLAN_NIGHTSIGHT_LEVEL_REQ_MONK                 , "ability.clan.nightsight.level_req_monk",          1) \
    CFG_X(ABILITY_CLAN_NIGHTSIGHT_LEVEL_REQ_VAMPIRE_OBTE         , "ability.clan.nightsight.level_req_vampire_obte",          3) \
    CFG_X(ABILITY_CLAN_NIGHTSIGHT_LEVEL_REQ_VAMPIRE_PROT         , "ability.clan.nightsight.level_req_vampire_prot",          1) \
    CFG_X(ABILITY_CLAN_NIGHTSIGHT_LEVEL_REQ_WEREWOLF             , "ability.clan.nightsight.level_req_werewolf",          1) \
    CFG_X(ABILITY_CLAN_PLASMA_LEVEL_REQ                          , "ability.clan.plasma.level_req",          5) \
    CFG_X(ABILITY_CLAN_POISON_BLOOD_COST_MAX                     , "ability.clan.poison.blood_cost_max",         15) \
    CFG_X(ABILITY_CLAN_POISON_BLOOD_COST_MIN                     , "ability.clan.poison.blood_cost_min",          5) \
    CFG_X(ABILITY_CLAN_POISON_BLOOD_REQ                          , "ability.clan.poison.blood_req",         15) \
    CFG_X(ABILITY_CLAN_POISON_LEVEL_REQ                          , "ability.clan.poison.level_req",          3) \
    CFG_X(ABILITY_CLAN_POISON_TIMER_MAX                          , "ability.clan.poison.timer_max",         20) \
    CFG_X(ABILITY_CLAN_POISON_TIMER_MIN                          , "ability.clan.poison.timer_min",         10) \
    CFG_X(ABILITY_CLAN_POSSESSION_BLOOD_COST_VAMPIRE             , "ability.clan.possession.blood_cost_vampire",         50) \
    CFG_X(ABILITY_CLAN_POSSESSION_LEVEL_REQ_DEMON                , "ability.clan.possession.level_req_demon",         10) \
    CFG_X(ABILITY_CLAN_POSSESSION_LEVEL_REQ_VAMPIRE              , "ability.clan.possession.level_req_vampire",          3) \
    CFG_X(ABILITY_CLAN_POSSESSION_MOVE_COST_DEMON                , "ability.clan.possession.move_cost_demon",        500) \
    CFG_X(ABILITY_CLAN_RAGE_DEMON_BEAST_MAX                      , "ability.clan.rage_demon.beast_max",        100) \
    CFG_X(ABILITY_CLAN_RAGE_DEMON_COOLDOWN                       , "ability.clan.rage_demon.cooldown",         12) \
    CFG_X(ABILITY_CLAN_RAGE_DEMON_LEVEL_REQ                      , "ability.clan.rage_demon.level_req",          3) \
    CFG_X(ABILITY_CLAN_RAGE_DEMON_RAGE_CAP                       , "ability.clan.rage_demon.rage_cap",        125) \
    CFG_X(ABILITY_CLAN_RAGE_DEMON_RAGE_GAIN_MAX                  , "ability.clan.rage_demon.rage_gain_max",         25) \
    CFG_X(ABILITY_CLAN_RAGE_WW_COOLDOWN                          , "ability.clan.rage_ww.cooldown",         12) \
    CFG_X(ABILITY_CLAN_RAGE_WW_RAGE_GAIN_MAX                     , "ability.clan.rage_ww.rage_gain_max",         60) \
    CFG_X(ABILITY_CLAN_RAGE_WW_RAGE_GAIN_MIN                     , "ability.clan.rage_ww.rage_gain_min",         40) \
    CFG_X(ABILITY_CLAN_RAGE_WW_TRANSFORM_THRESHOLD               , "ability.clan.rage_ww.transform_threshold",        100) \
    CFG_X(ABILITY_CLAN_READAURA_LEVEL_REQ_DROID                  , "ability.clan.readaura.level_req_droid",          5) \
    CFG_X(ABILITY_CLAN_READAURA_LEVEL_REQ_LICH                   , "ability.clan.readaura.level_req_lich",          1) \
    CFG_X(ABILITY_CLAN_READAURA_LEVEL_REQ_MONK                   , "ability.clan.readaura.level_req_monk",          2) \
    CFG_X(ABILITY_CLAN_READAURA_LEVEL_REQ_VAMPIRE                , "ability.clan.readaura.level_req_vampire",          3) \
    CFG_X(ABILITY_CLAN_REGENERATE_BLOOD_COST_MAX                 , "ability.clan.regenerate.blood_cost_max",         20) \
    CFG_X(ABILITY_CLAN_REGENERATE_BLOOD_COST_MIN                 , "ability.clan.regenerate.blood_cost_min",         10) \
    CFG_X(ABILITY_CLAN_REGENERATE_BLOOD_REQ                      , "ability.clan.regenerate.blood_req",          5) \
    CFG_X(ABILITY_CLAN_REGENERATE_COOLDOWN_DOWNED                , "ability.clan.regenerate.cooldown_downed",         24) \
    CFG_X(ABILITY_CLAN_REGENERATE_COOLDOWN_NORMAL                , "ability.clan.regenerate.cooldown_normal",          8) \
    CFG_X(ABILITY_CLAN_REGENERATE_HEAL_AMOUNT                    , "ability.clan.regenerate.heal_amount",        500) \
    CFG_X(ABILITY_CLAN_RESET_SPELL_CAP                           , "ability.clan.reset_spell.cap",        200) \
    CFG_X(ABILITY_CLAN_RESET_WEAPON_CAP                          , "ability.clan.reset_weapon.cap",        200) \
    CFG_X(ABILITY_CLAN_ROT_COOLDOWN                              , "ability.clan.rot.cooldown",         12) \
    CFG_X(ABILITY_CLAN_ROT_LEVEL_REQ                             , "ability.clan.rot.level_req",          2) \
    CFG_X(ABILITY_CLAN_ROT_SUCCESS_CHANCE                        , "ability.clan.rot.success_chance",         50) \
    CFG_X(ABILITY_CLAN_SCRY_BLOOD_COST_MAX                       , "ability.clan.scry.blood_cost_max",         25) \
    CFG_X(ABILITY_CLAN_SCRY_BLOOD_COST_MIN                       , "ability.clan.scry.blood_cost_min",         15) \
    CFG_X(ABILITY_CLAN_SCRY_BLOOD_REQ                            , "ability.clan.scry.blood_req",         25) \
    CFG_X(ABILITY_CLAN_SCRY_LEVEL_REQ_DROID                      , "ability.clan.scry.level_req_droid",          5) \
    CFG_X(ABILITY_CLAN_SCRY_LEVEL_REQ_MAGE                       , "ability.clan.scry.level_req_mage",          3) \
    CFG_X(ABILITY_CLAN_SCRY_LEVEL_REQ_MONK                       , "ability.clan.scry.level_req_monk",          3) \
    CFG_X(ABILITY_CLAN_SCRY_LEVEL_REQ_VAMPIRE                    , "ability.clan.scry.level_req_vampire",          2) \
    CFG_X(ABILITY_CLAN_SERENITY_COOLDOWN                         , "ability.clan.serenity.cooldown",         12) \
    CFG_X(ABILITY_CLAN_SERENITY_LEVEL_REQ                        , "ability.clan.serenity.level_req",          2) \
    CFG_X(ABILITY_CLAN_SERPENT_BLOOD_COST_MAX                    , "ability.clan.serpent.blood_cost_max",         50) \
    CFG_X(ABILITY_CLAN_SERPENT_BLOOD_COST_MIN                    , "ability.clan.serpent.blood_cost_min",         40) \
    CFG_X(ABILITY_CLAN_SERPENT_BLOOD_REQ                         , "ability.clan.serpent.blood_req",         50) \
    CFG_X(ABILITY_CLAN_SERPENT_LEVEL_REQ                         , "ability.clan.serpent.level_req",          2) \
    CFG_X(ABILITY_CLAN_SERPENT_MOD_STR                           , "ability.clan.serpent.mod_str",         10) \
    CFG_X(ABILITY_CLAN_SHADOWPLANE_BLOOD_COST_MAX                , "ability.clan.shadowplane.blood_cost_max",         75) \
    CFG_X(ABILITY_CLAN_SHADOWPLANE_BLOOD_COST_MIN                , "ability.clan.shadowplane.blood_cost_min",         65) \
    CFG_X(ABILITY_CLAN_SHADOWPLANE_BLOOD_REQ                     , "ability.clan.shadowplane.blood_req",         75) \
    CFG_X(ABILITY_CLAN_SHADOWPLANE_LEVEL_REQ_VAMPIRE             , "ability.clan.shadowplane.level_req_vampire",          3) \
    CFG_X(ABILITY_CLAN_SHADOWPLANE_LEVEL_REQ_WEREWOLF            , "ability.clan.shadowplane.level_req_werewolf",          3) \
    CFG_X(ABILITY_CLAN_SHADOWSIGHT_BLOOD_COST_MAX                , "ability.clan.shadowsight.blood_cost_max",         10) \
    CFG_X(ABILITY_CLAN_SHADOWSIGHT_BLOOD_COST_MIN                , "ability.clan.shadowsight.blood_cost_min",          5) \
    CFG_X(ABILITY_CLAN_SHADOWSIGHT_BLOOD_REQ                     , "ability.clan.shadowsight.blood_req",         10) \
    CFG_X(ABILITY_CLAN_SHADOWSIGHT_LEVEL_REQ_MONK                , "ability.clan.shadowsight.level_req_monk",          2) \
    CFG_X(ABILITY_CLAN_SHADOWSIGHT_LEVEL_REQ_VAMPIRE             , "ability.clan.shadowsight.level_req_vampire",          2) \
    CFG_X(ABILITY_CLAN_SHADOWSIGHT_LEVEL_REQ_WEREWOLF            , "ability.clan.shadowsight.level_req_werewolf",          2) \
    CFG_X(ABILITY_CLAN_SHADOWSTEP_LEVEL_REQ                      , "ability.clan.shadowstep.level_req",          4) \
    CFG_X(ABILITY_CLAN_SHIELD_LEVEL_REQ_DROID                    , "ability.clan.shield.level_req_droid",          4) \
    CFG_X(ABILITY_CLAN_SHIELD_LEVEL_REQ_MONK                     , "ability.clan.shield.level_req_monk",          2) \
    CFG_X(ABILITY_CLAN_SHIELD_LEVEL_REQ_VAMP_DOMI                , "ability.clan.shield.level_req_vamp_domi",          2) \
    CFG_X(ABILITY_CLAN_SHIELD_LEVEL_REQ_VAMP_OBFU                , "ability.clan.shield.level_req_vamp_obfu",          3) \
    CFG_X(ABILITY_CLAN_SHIELD_LEVEL_REQ_WEREWOLF                 , "ability.clan.shield.level_req_werewolf",          2) \
    CFG_X(ABILITY_CLAN_SMOTHER_SPARK_CHANCE                      , "ability.clan.smother.spark_chance",         98) \
    CFG_X(ABILITY_CLAN_STAKE_EXP_REWARD                          , "ability.clan.stake.exp_reward",       1000) \
    CFG_X(ABILITY_CLAN_TASTE_LEVEL_REQ                           , "ability.clan.taste.level_req",          1) \
    CFG_X(ABILITY_CLAN_THEFT_ACID_BLOOD_DRAIN                    , "ability.clan.theft.acid_blood_drain",         30) \
    CFG_X(ABILITY_CLAN_THEFT_ACID_DAMAGE                         , "ability.clan.theft.acid_damage",        300) \
    CFG_X(ABILITY_CLAN_THEFT_BLOOD_GAIN_MAX                      , "ability.clan.theft.blood_gain_max",         15) \
    CFG_X(ABILITY_CLAN_THEFT_BLOOD_GAIN_MIN                      , "ability.clan.theft.blood_gain_min",         10) \
    CFG_X(ABILITY_CLAN_THEFT_LEVEL_REQ                           , "ability.clan.theft.level_req",          4) \
    CFG_X(ABILITY_CLAN_THEFT_MOB_DRAIN_MAX                       , "ability.clan.theft.mob_drain_max",         40) \
    CFG_X(ABILITY_CLAN_THEFT_MOB_DRAIN_MIN                       , "ability.clan.theft.mob_drain_min",         30) \
    CFG_X(ABILITY_CLAN_THEFT_PLAYER_DRAIN_MAX                    , "ability.clan.theft.player_drain_max",         40) \
    CFG_X(ABILITY_CLAN_THEFT_PLAYER_DRAIN_MIN                    , "ability.clan.theft.player_drain_min",         30) \
    CFG_X(ABILITY_CLAN_TIDE_LEVEL_REQ                            , "ability.clan.tide.level_req",          5) \
    CFG_X(ABILITY_CLAN_TIDE_PRIMAL_COST                          , "ability.clan.tide.primal_cost",         10) \
    CFG_X(ABILITY_CLAN_TRUESIGHT_LEVEL_REQ_MONK                  , "ability.clan.truesight.level_req_monk",          3) \
    CFG_X(ABILITY_CLAN_TRUESIGHT_LEVEL_REQ_SHAPESHIFTER          , "ability.clan.truesight.level_req_shapeshifter",          1) \
    CFG_X(ABILITY_CLAN_TRUESIGHT_LEVEL_REQ_VAMPIRE               , "ability.clan.truesight.level_req_vampire",          1) \
    CFG_X(ABILITY_CLAN_TRUESIGHT_LEVEL_REQ_WEREWOLF              , "ability.clan.truesight.level_req_werewolf",          3) \
    CFG_X(ABILITY_CLAN_UNWEREWOLF_DAMROLL_LOSS                   , "ability.clan.unwerewolf.damroll_loss",         50) \
    CFG_X(ABILITY_CLAN_UNWEREWOLF_HITROLL_LOSS                   , "ability.clan.unwerewolf.hitroll_loss",         50) \
    CFG_X(ABILITY_CLAN_UNWEREWOLF_RAGE_LOSS                      , "ability.clan.unwerewolf.rage_loss",         25) \
    CFG_X(ABILITY_CLAN_VANISH_LEVEL_REQ_VAMPIRE                  , "ability.clan.vanish.level_req_vampire",          1) \
    CFG_X(ABILITY_CLAN_VANISH_LEVEL_REQ_WEREWOLF                 , "ability.clan.vanish.level_req_werewolf",          1) \
    CFG_X(ABILITY_CLAN_WEB_COOLDOWN                              , "ability.clan.web.cooldown",         12) \
    CFG_X(ABILITY_CLAN_WEB_LEVEL_REQ_TANARRI                     , "ability.clan.web.level_req_tanarri",          3) \
    CFG_X(ABILITY_CLAN_WEB_LEVEL_REQ_WEREWOLF                    , "ability.clan.web.level_req_werewolf",          2) \
    CFG_X(ABILITY_CLAN_WEREWOLF_DAMROLL_BONUS                    , "ability.clan.werewolf.damroll_bonus",         50) \
    CFG_X(ABILITY_CLAN_WEREWOLF_HITROLL_BONUS                    , "ability.clan.werewolf.hitroll_bonus",         50) \
    CFG_X(ABILITY_CLAN_WEREWOLF_RAGE_INITIAL                     , "ability.clan.werewolf.rage_initial",         25) \
    CFG_X(ABILITY_CLAN_WEREWOLF_RAGE_MAX                         , "ability.clan.werewolf.rage_max",        300) \
    CFG_X(ABILITY_CLAN_WEREWOLF_RAGE_WOLF4_BONUS                 , "ability.clan.werewolf.rage_wolf4_bonus",        100) \
    CFG_X(ABILITY_CLAN_ZULOFORM_BLOOD_COST_MAX                   , "ability.clan.zuloform.blood_cost_max",        200) \
    CFG_X(ABILITY_CLAN_ZULOFORM_BLOOD_COST_MIN                   , "ability.clan.zuloform.blood_cost_min",        100) \
    CFG_X(ABILITY_CLAN_ZULOFORM_BLOOD_REQ                        , "ability.clan.zuloform.blood_req",        200) \
    CFG_X(ABILITY_CLAN_ZULOFORM_DAMROLL_BONUS                    , "ability.clan.zuloform.damroll_bonus",        150) \
    CFG_X(ABILITY_CLAN_ZULOFORM_HITROLL_BONUS                    , "ability.clan.zuloform.hitroll_bonus",        150) \
    CFG_X(ABILITY_CLAN_ZULOFORM_LEVEL_REQ                        , "ability.clan.zuloform.level_req",          2) \
    CFG_X(ABILITY_CLAN_ZULOFORM_MOD_DEX                          , "ability.clan.zuloform.mod_dex",         15) \
    CFG_X(ABILITY_CLAN_ZULOFORM_MOD_STR                          , "ability.clan.zuloform.mod_str",         15) \
    \
    /* =========== ABILITY - DEMON =========== */ \
    CFG_X(ABILITY_DEMON_CONE_COOLDOWN                            , "ability.demon.cone.cooldown",         10) \
    CFG_X(ABILITY_DEMON_CONE_MANA_COST                           , "ability.demon.cone.mana_cost",        100) \
    CFG_X(ABILITY_DEMON_DEMONARMOUR_PRIMAL_COST                  , "ability.demon.demonarmour.primal_cost",         60) \
    CFG_X(ABILITY_DEMON_DSTAKE_PRIMAL_COST                       , "ability.demon.dstake.primal_cost",         60) \
    CFG_X(ABILITY_DEMON_HORNS_LEVEL_REQ                          , "ability.demon.horns.level_req",          4) \
    CFG_X(ABILITY_DEMON_INPART_COST_BLINK                        , "ability.demon.inpart.cost.blink",      15000) \
    CFG_X(ABILITY_DEMON_INPART_COST_CAUST                        , "ability.demon.inpart.cost.caust",       3000) \
    CFG_X(ABILITY_DEMON_INPART_COST_CLAWS                        , "ability.demon.inpart.cost.claws",       2500) \
    CFG_X(ABILITY_DEMON_INPART_COST_DEMONFORM                    , "ability.demon.inpart.cost.demonform",      25000) \
    CFG_X(ABILITY_DEMON_INPART_COST_ENTOMB                       , "ability.demon.inpart.cost.entomb",      20000) \
    CFG_X(ABILITY_DEMON_INPART_COST_FANGS                        , "ability.demon.inpart.cost.fangs",       2500) \
    CFG_X(ABILITY_DEMON_INPART_COST_FREEZEWEAPON                 , "ability.demon.inpart.cost.freezeweapon",       3000) \
    CFG_X(ABILITY_DEMON_INPART_COST_GRAFT                        , "ability.demon.inpart.cost.graft",      20000) \
    CFG_X(ABILITY_DEMON_INPART_COST_HOOVES                       , "ability.demon.inpart.cost.hooves",       1500) \
    CFG_X(ABILITY_DEMON_INPART_COST_HORNS                        , "ability.demon.inpart.cost.horns",       2500) \
    CFG_X(ABILITY_DEMON_INPART_COST_IMMOLATE                     , "ability.demon.inpart.cost.immolate",       2500) \
    CFG_X(ABILITY_DEMON_INPART_COST_INFERNO                      , "ability.demon.inpart.cost.inferno",      20000) \
    CFG_X(ABILITY_DEMON_INPART_COST_LEAP                         , "ability.demon.inpart.cost.leap",        500) \
    CFG_X(ABILITY_DEMON_INPART_COST_LEECH                        , "ability.demon.inpart.cost.leech",      15000) \
    CFG_X(ABILITY_DEMON_INPART_COST_LIFESPAN                     , "ability.demon.inpart.cost.lifespan",        100) \
    CFG_X(ABILITY_DEMON_INPART_COST_MAGIC                        , "ability.demon.inpart.cost.magic",       1000) \
    CFG_X(ABILITY_DEMON_INPART_COST_MIGHT                        , "ability.demon.inpart.cost.might",       7500) \
    CFG_X(ABILITY_DEMON_INPART_COST_MOVE                         , "ability.demon.inpart.cost.move",        500) \
    CFG_X(ABILITY_DEMON_INPART_COST_NIGHTSIGHT                   , "ability.demon.inpart.cost.nightsight",       3000) \
    CFG_X(ABILITY_DEMON_INPART_COST_SCRY                         , "ability.demon.inpart.cost.scry",       7500) \
    CFG_X(ABILITY_DEMON_INPART_COST_SHIELD                       , "ability.demon.inpart.cost.shield",      20000) \
    CFG_X(ABILITY_DEMON_INPART_COST_SPEED                        , "ability.demon.inpart.cost.speed",       7500) \
    CFG_X(ABILITY_DEMON_INPART_COST_TAIL                         , "ability.demon.inpart.cost.tail",       5000) \
    CFG_X(ABILITY_DEMON_INPART_COST_TOUGHNESS                    , "ability.demon.inpart.cost.toughness",       7500) \
    CFG_X(ABILITY_DEMON_INPART_COST_TRAVEL                       , "ability.demon.inpart.cost.travel",       1500) \
    CFG_X(ABILITY_DEMON_INPART_COST_TRUESIGHT                    , "ability.demon.inpart.cost.truesight",       7500) \
    CFG_X(ABILITY_DEMON_INPART_COST_UNNERVE                      , "ability.demon.inpart.cost.unnerve",       5000) \
    CFG_X(ABILITY_DEMON_INPART_COST_WINGS                        , "ability.demon.inpart.cost.wings",       1000) \
    CFG_X(ABILITY_DEMON_INPART_NONCLASS_PRIMAL_COST              , "ability.demon.inpart.nonclass_primal_cost",        100) \
    CFG_X(ABILITY_DEMON_INPART_OTHER_MULTIPLIER                  , "ability.demon.inpart.other_multiplier",         25) \
    CFG_X(ABILITY_DEMON_OBTAIN_DEMON_POINTS_COST                 , "ability.demon.obtain.demon_points_cost",      15000) \
    CFG_X(ABILITY_DEMON_OBTAIN_MAX_WARPS                         , "ability.demon.obtain.max_warps",         18) \
    CFG_X(ABILITY_DEMON_WINGS_LEVEL_REQ                          , "ability.demon.wings.level_req",          5) \
    \
    /* =========== ABILITY - DIRGESINGER =========== */ \
    CFG_X(ABILITY_DIRGESINGER_ARMOR_PRIMAL_COST                  , "ability.dirgesinger.armor.primal_cost",         60) \
    CFG_X(ABILITY_DIRGESINGER_BATTLEHYMN_DAM_BONUS               , "ability.dirgesinger.battlehymn.dam_bonus",         50) \
    CFG_X(ABILITY_DIRGESINGER_BATTLEHYMN_DURATION                , "ability.dirgesinger.battlehymn.duration",         10) \
    CFG_X(ABILITY_DIRGESINGER_BATTLEHYMN_MANA_COST               , "ability.dirgesinger.battlehymn.mana_cost",         60) \
    CFG_X(ABILITY_DIRGESINGER_CADENCE_DURATION                   , "ability.dirgesinger.cadence.duration",          8) \
    CFG_X(ABILITY_DIRGESINGER_CADENCE_EXTRA_ATTACKS              , "ability.dirgesinger.cadence.extra_attacks",          2) \
    CFG_X(ABILITY_DIRGESINGER_CADENCE_MANA_COST                  , "ability.dirgesinger.cadence.mana_cost",         60) \
    CFG_X(ABILITY_DIRGESINGER_DIRGE_DURATION                     , "ability.dirgesinger.dirge.duration",          6) \
    CFG_X(ABILITY_DIRGESINGER_DIRGE_MANA_COST                    , "ability.dirgesinger.dirge.mana_cost",         50) \
    CFG_X(ABILITY_DIRGESINGER_DIRGE_MAX_STACKS                   , "ability.dirgesinger.dirge.max_stacks",          5) \
    CFG_X(ABILITY_DIRGESINGER_DIRGE_TICK_DAMAGE                  , "ability.dirgesinger.dirge.tick_damage",         50) \
    CFG_X(ABILITY_DIRGESINGER_DISSONANCE_COOLDOWN                , "ability.dirgesinger.dissonance.cooldown",          8) \
    CFG_X(ABILITY_DIRGESINGER_DISSONANCE_DURATION                , "ability.dirgesinger.dissonance.duration",         10) \
    CFG_X(ABILITY_DIRGESINGER_DISSONANCE_MANA_COST               , "ability.dirgesinger.dissonance.mana_cost",         60) \
    CFG_X(ABILITY_DIRGESINGER_IRONSONG_ABSORB_AMOUNT             , "ability.dirgesinger.ironsong.absorb_amount",       1000) \
    CFG_X(ABILITY_DIRGESINGER_IRONSONG_DURATION                  , "ability.dirgesinger.ironsong.duration",         12) \
    CFG_X(ABILITY_DIRGESINGER_IRONSONG_MANA_COST                 , "ability.dirgesinger.ironsong.mana_cost",         70) \
    CFG_X(ABILITY_DIRGESINGER_RALLY_HEAL_AMOUNT                  , "ability.dirgesinger.rally.heal_amount",        500) \
    CFG_X(ABILITY_DIRGESINGER_RALLY_MANA_COST                    , "ability.dirgesinger.rally.mana_cost",        120) \
    CFG_X(ABILITY_DIRGESINGER_RALLY_RESONANCE_COST               , "ability.dirgesinger.rally.resonance_cost",         30) \
    CFG_X(ABILITY_DIRGESINGER_RALLY_RESONANCE_REQ                , "ability.dirgesinger.rally.resonance_req",         50) \
    CFG_X(ABILITY_DIRGESINGER_SHATTER_COOLDOWN                   , "ability.dirgesinger.shatter.cooldown",          8) \
    CFG_X(ABILITY_DIRGESINGER_SHATTER_DISARM_CHANCE              , "ability.dirgesinger.shatter.disarm_chance",         30) \
    CFG_X(ABILITY_DIRGESINGER_SHATTER_MANA_COST                  , "ability.dirgesinger.shatter.mana_cost",         80) \
    CFG_X(ABILITY_DIRGESINGER_SHATTER_RESONANCE_COST             , "ability.dirgesinger.shatter.resonance_cost",         10) \
    CFG_X(ABILITY_DIRGESINGER_SHATTER_RESONANCE_REQ              , "ability.dirgesinger.shatter.resonance_req",         25) \
    CFG_X(ABILITY_DIRGESINGER_THUNDERCLAP_COOLDOWN               , "ability.dirgesinger.thunderclap.cooldown",         10) \
    CFG_X(ABILITY_DIRGESINGER_THUNDERCLAP_MANA_COST              , "ability.dirgesinger.thunderclap.mana_cost",        100) \
    CFG_X(ABILITY_DIRGESINGER_THUNDERCLAP_RESONANCE_COST         , "ability.dirgesinger.thunderclap.resonance_cost",         20) \
    CFG_X(ABILITY_DIRGESINGER_THUNDERCLAP_RESONANCE_REQ          , "ability.dirgesinger.thunderclap.resonance_req",         40) \
    CFG_X(ABILITY_DIRGESINGER_WARCRY_COOLDOWN                    , "ability.dirgesinger.warcry.cooldown",          4) \
    CFG_X(ABILITY_DIRGESINGER_WARCRY_DAM_BONUS_MAX               , "ability.dirgesinger.warcry.dam_bonus_max",        120) \
    CFG_X(ABILITY_DIRGESINGER_WARCRY_DAM_BONUS_MIN               , "ability.dirgesinger.warcry.dam_bonus_min",         50) \
    CFG_X(ABILITY_DIRGESINGER_WARCRY_MANA_COST                   , "ability.dirgesinger.warcry.mana_cost",         50) \
    CFG_X(ABILITY_DIRGESINGER_WARCRY_RESONANCE_GAIN              , "ability.dirgesinger.warcry.resonance_gain",          5) \
    CFG_X(ABILITY_DIRGESINGER_WARSONG_MANA_DRAIN_PER_TICK        , "ability.dirgesinger.warsong.mana_drain_per_tick",         25) \
    \
    /* =========== ABILITY - DRAGONKIN =========== */ \
    CFG_X(ABILITY_DRAGONKIN_DRAGONARMOR_PRACTICE_COST            , "ability.dragonkin.dragonarmor.practice_cost",         60) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONBREATH_COOLDOWN                , "ability.dragonkin.dragonbreath.cooldown",          8) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONBREATH_DAM_MAX                 , "ability.dragonkin.dragonbreath.dam_max",        300) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONBREATH_DAM_MIN                 , "ability.dragonkin.dragonbreath.dam_min",        150) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONBREATH_ESSENCE_COST            , "ability.dragonkin.dragonbreath.essence_cost",         20) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONBREATH_MANA_COST               , "ability.dragonkin.dragonbreath.mana_cost",         60) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONCLAW_COOLDOWN                  , "ability.dragonkin.dragonclaw.cooldown",          4) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONCLAW_DAM_MAX                   , "ability.dragonkin.dragonclaw.dam_max",        200) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONCLAW_DAM_MIN                   , "ability.dragonkin.dragonclaw.dam_min",        100) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONCLAW_MANA_COST                 , "ability.dragonkin.dragonclaw.mana_cost",         40) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONHIDE_DAM_REDUCTION             , "ability.dragonkin.dragonhide.dam_reduction",         15) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONRUSH_COOLDOWN                  , "ability.dragonkin.dragonrush.cooldown",          8) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONRUSH_DAM_MAX                   , "ability.dragonkin.dragonrush.dam_max",        350) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONRUSH_DAM_MIN                   , "ability.dragonkin.dragonrush.dam_min",        150) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONRUSH_ESSENCE_COST              , "ability.dragonkin.dragonrush.essence_cost",         25) \
    CFG_X(ABILITY_DRAGONKIN_DRAGONRUSH_MANA_COST                 , "ability.dragonkin.dragonrush.mana_cost",         60) \
    CFG_X(ABILITY_DRAGONKIN_DRAKEWINGS_DURATION                  , "ability.dragonkin.drakewings.duration",         30) \
    CFG_X(ABILITY_DRAGONKIN_DRAKEWINGS_MANA_COST                 , "ability.dragonkin.drakewings.mana_cost",         50) \
    CFG_X(ABILITY_DRAGONKIN_ESSENCE_COMBAT_GAIN                  , "ability.dragonkin.essence.combat_gain",          3) \
    CFG_X(ABILITY_DRAGONKIN_ESSENCE_DECAY_RATE                   , "ability.dragonkin.essence.decay_rate",          2) \
    CFG_X(ABILITY_DRAGONKIN_INFERNALSTORM_COOLDOWN               , "ability.dragonkin.infernalstorm.cooldown",         15) \
    CFG_X(ABILITY_DRAGONKIN_INFERNALSTORM_DAM_MAX                , "ability.dragonkin.infernalstorm.dam_max",        600) \
    CFG_X(ABILITY_DRAGONKIN_INFERNALSTORM_DAM_MIN                , "ability.dragonkin.infernalstorm.dam_min",        300) \
    CFG_X(ABILITY_DRAGONKIN_INFERNALSTORM_ESSENCE_COST           , "ability.dragonkin.infernalstorm.essence_cost",         60) \
    CFG_X(ABILITY_DRAGONKIN_INFERNALSTORM_MANA_COST              , "ability.dragonkin.infernalstorm.mana_cost",        150) \
    CFG_X(ABILITY_DRAGONKIN_MEDITATE_COOLDOWN                    , "ability.dragonkin.meditate.cooldown",          8) \
    CFG_X(ABILITY_DRAGONKIN_MEDITATE_GAIN                        , "ability.dragonkin.meditate.gain",         15) \
    CFG_X(ABILITY_DRAGONKIN_PRIMALWARDING_DURATION               , "ability.dragonkin.primalwarding.duration",         20) \
    CFG_X(ABILITY_DRAGONKIN_PRIMALWARDING_MANA_COST              , "ability.dragonkin.primalwarding.mana_cost",        100) \
    CFG_X(ABILITY_DRAGONKIN_SCALESHIELD_ABSORB                   , "ability.dragonkin.scaleshield.absorb",       2000) \
    CFG_X(ABILITY_DRAGONKIN_SCALESHIELD_DURATION                 , "ability.dragonkin.scaleshield.duration",         15) \
    CFG_X(ABILITY_DRAGONKIN_SCALESHIELD_MANA_COST                , "ability.dragonkin.scaleshield.mana_cost",         70) \
    CFG_X(ABILITY_DRAGONKIN_SEARINGBLAST_COOLDOWN                , "ability.dragonkin.searingblast.cooldown",         10) \
    CFG_X(ABILITY_DRAGONKIN_SEARINGBLAST_DAM_MAX                 , "ability.dragonkin.searingblast.dam_max",        400) \
    CFG_X(ABILITY_DRAGONKIN_SEARINGBLAST_DAM_MIN                 , "ability.dragonkin.searingblast.dam_min",        200) \
    CFG_X(ABILITY_DRAGONKIN_SEARINGBLAST_DOT_DAMAGE              , "ability.dragonkin.searingblast.dot_damage",         50) \
    CFG_X(ABILITY_DRAGONKIN_SEARINGBLAST_DOT_DURATION            , "ability.dragonkin.searingblast.dot_duration",          5) \
    CFG_X(ABILITY_DRAGONKIN_SEARINGBLAST_ESSENCE_COST            , "ability.dragonkin.searingblast.essence_cost",         30) \
    CFG_X(ABILITY_DRAGONKIN_SEARINGBLAST_MANA_COST               , "ability.dragonkin.searingblast.mana_cost",         80) \
    \
    /* =========== ABILITY - DROW =========== */ \
    CFG_X(ABILITY_DROW_CHAOSBLAST_COOLDOWN                       , "ability.drow.chaosblast.cooldown",         12) \
    CFG_X(ABILITY_DROW_CHAOSBLAST_MANA_COST                      , "ability.drow.chaosblast.mana_cost",        750) \
    CFG_X(ABILITY_DROW_CONFUSE_COOLDOWN                          , "ability.drow.confuse.cooldown",         16) \
    CFG_X(ABILITY_DROW_CONFUSE_FAIL_THRESHOLD                    , "ability.drow.confuse.fail_threshold",         25) \
    CFG_X(ABILITY_DROW_CONFUSE_MOVE_COST                         , "ability.drow.confuse.move_cost",         75) \
    CFG_X(ABILITY_DROW_DARKNESS_MANA_COST                        , "ability.drow.darkness.mana_cost",        500) \
    CFG_X(ABILITY_DROW_DROWCREATE_PRIMAL_COST                    , "ability.drow.drowcreate.primal_cost",         60) \
    CFG_X(ABILITY_DROW_DROWFIRE_COOLDOWN                         , "ability.drow.drowfire.cooldown",         12) \
    CFG_X(ABILITY_DROW_DROWFIRE_MANA_COST                        , "ability.drow.drowfire.mana_cost",        100) \
    CFG_X(ABILITY_DROW_EARTHSHATTER_COOLDOWN                     , "ability.drow.earthshatter.cooldown",         12) \
    CFG_X(ABILITY_DROW_EARTHSHATTER_MANA_COST                    , "ability.drow.earthshatter.mana_cost",        150) \
    CFG_X(ABILITY_DROW_GRANT_CONFUSE_COST                        , "ability.drow.grant.confuse_cost",       2500) \
    CFG_X(ABILITY_DROW_GRANT_DARKNESS_COST                       , "ability.drow.grant.darkness_cost",       7500) \
    CFG_X(ABILITY_DROW_GRANT_DARKTENDRILS_COST                   , "ability.drow.grant.darktendrils_cost",      25000) \
    CFG_X(ABILITY_DROW_GRANT_DGAROTTE_COST                       , "ability.drow.grant.dgarotte_cost",       2500) \
    CFG_X(ABILITY_DROW_GRANT_DROWFIRE_COST                       , "ability.drow.grant.drowfire_cost",       2500) \
    CFG_X(ABILITY_DROW_GRANT_DROWHATE_COST                       , "ability.drow.grant.drowhate_cost",      20000) \
    CFG_X(ABILITY_DROW_GRANT_DROWPOISON_COST                     , "ability.drow.grant.drowpoison_cost",       2500) \
    CFG_X(ABILITY_DROW_GRANT_DROWSHIELD_COST                     , "ability.drow.grant.drowshield_cost",       5000) \
    CFG_X(ABILITY_DROW_GRANT_DROWSIGHT_COST                      , "ability.drow.grant.drowsight_cost",       5000) \
    CFG_X(ABILITY_DROW_GRANT_EARTHSHATTER_COST                   , "ability.drow.grant.earthshatter_cost",       7500) \
    CFG_X(ABILITY_DROW_GRANT_FIGHTDANCE_COST                     , "ability.drow.grant.fightdance_cost",      10000) \
    CFG_X(ABILITY_DROW_GRANT_GAROTTE_COST                        , "ability.drow.grant.garotte_cost",       5000) \
    CFG_X(ABILITY_DROW_GRANT_GLAMOUR_COST                        , "ability.drow.grant.glamour_cost",       5000) \
    CFG_X(ABILITY_DROW_GRANT_LEVITATION_COST                     , "ability.drow.grant.levitation_cost",       1000) \
    CFG_X(ABILITY_DROW_GRANT_OTHER_MULTIPLIER                    , "ability.drow.grant.other_multiplier",          5) \
    CFG_X(ABILITY_DROW_GRANT_SHADOWWALK_COST                     , "ability.drow.grant.shadowwalk_cost",      10000) \
    CFG_X(ABILITY_DROW_GRANT_SPEED_COST                          , "ability.drow.grant.speed_cost",       7500) \
    CFG_X(ABILITY_DROW_GRANT_SPIDERARMS_COST                     , "ability.drow.grant.spiderarms_cost",      25000) \
    CFG_X(ABILITY_DROW_GRANT_SPIDERFORM_COST                     , "ability.drow.grant.spiderform_cost",      25000) \
    CFG_X(ABILITY_DROW_GRANT_TOUGHSKIN_COST                      , "ability.drow.grant.toughskin_cost",       7500) \
    CFG_X(ABILITY_DROW_GRANT_WEB_COST                            , "ability.drow.grant.web_cost",       5000) \
    CFG_X(ABILITY_DROW_HEAL_COOLDOWN                             , "ability.drow.heal.cooldown",         12) \
    CFG_X(ABILITY_DROW_HEAL_MANA_COST                            , "ability.drow.heal.mana_cost",        750) \
    CFG_X(ABILITY_DROW_SHADOWWALK_MOVE_COST                      , "ability.drow.shadowwalk.move_cost",        250) \
    CFG_X(ABILITY_DROW_SPIDERFORM_ARMOR_BONUS                    , "ability.drow.spiderform.armor_bonus",       1000) \
    CFG_X(ABILITY_DROW_SPIDERFORM_DAMROLL_BONUS                  , "ability.drow.spiderform.damroll_bonus",        400) \
    CFG_X(ABILITY_DROW_SPIDERFORM_HITROLL_BONUS                  , "ability.drow.spiderform.hitroll_bonus",        400) \
    CFG_X(ABILITY_DROW_SPIDERFORM_REVERT_COOLDOWN                , "ability.drow.spiderform.revert_cooldown",          7) \
    \
    /* =========== ABILITY - LICH =========== */ \
    CFG_X(ABILITY_LICH_CHAOSSHIELD_LEVEL_REQ                     , "ability.lich.chaosshield.level_req",          5) \
    CFG_X(ABILITY_LICH_CHILLHAND_COOLDOWN                        , "ability.lich.chillhand.cooldown",          8) \
    CFG_X(ABILITY_LICH_CHILLHAND_DAMAGE_MAX                      , "ability.lich.chillhand.damage_max",       1250) \
    CFG_X(ABILITY_LICH_CHILLHAND_DAMAGE_MIN                      , "ability.lich.chillhand.damage_min",        750) \
    CFG_X(ABILITY_LICH_CHILLHAND_DEBUFF_DURATION                 , "ability.lich.chillhand.debuff_duration",          6) \
    CFG_X(ABILITY_LICH_CHILLHAND_LEVEL_REQ                       , "ability.lich.chillhand.level_req",          3) \
    CFG_X(ABILITY_LICH_CHILLHAND_PVP_DAMAGE_DIVISOR              , "ability.lich.chillhand.pvp_damage_divisor",          3) \
    CFG_X(ABILITY_LICH_CHILLHAND_STR_MODIFIER                    , "ability.lich.chillhand.str_modifier",         -3) \
    CFG_X(ABILITY_LICH_CREEPINGDOOM_COOLDOWN                     , "ability.lich.creepingdoom.cooldown",         24) \
    CFG_X(ABILITY_LICH_CREEPINGDOOM_LEVEL_REQ                    , "ability.lich.creepingdoom.level_req",          5) \
    CFG_X(ABILITY_LICH_CREEPINGDOOM_MANA_COST                    , "ability.lich.creepingdoom.mana_cost",       5000) \
    CFG_X(ABILITY_LICH_EARTHSWALLOW_COOLDOWN                     , "ability.lich.earthswallow.cooldown",          8) \
    CFG_X(ABILITY_LICH_EARTHSWALLOW_LEVEL_REQ                    , "ability.lich.earthswallow.level_req",          3) \
    CFG_X(ABILITY_LICH_EARTHSWALLOW_MANA_COST                    , "ability.lich.earthswallow.mana_cost",       5000) \
    CFG_X(ABILITY_LICH_LICHARMOR_PRACTICE_COST                   , "ability.lich.licharmor.practice_cost",        150) \
    CFG_X(ABILITY_LICH_PAINWRECK_COOLDOWN                        , "ability.lich.painwreck.cooldown",         18) \
    CFG_X(ABILITY_LICH_PAINWRECK_DAMAGE_MAX                      , "ability.lich.painwreck.damage_max",        200) \
    CFG_X(ABILITY_LICH_PAINWRECK_DAMAGE_MIN                      , "ability.lich.painwreck.damage_min",        100) \
    CFG_X(ABILITY_LICH_PAINWRECK_LEVEL_REQ                       , "ability.lich.painwreck.level_req",          4) \
    CFG_X(ABILITY_LICH_PAINWRECK_STUN_CHANCE                     , "ability.lich.painwreck.stun_chance",          3) \
    CFG_X(ABILITY_LICH_PENTAGRAM_COOLDOWN                        , "ability.lich.pentagram.cooldown",          8) \
    CFG_X(ABILITY_LICH_PENTAGRAM_LEVEL_REQ                       , "ability.lich.pentagram.level_req",          5) \
    CFG_X(ABILITY_LICH_PENTAGRAM_MANA_COST                       , "ability.lich.pentagram.mana_cost",       1500) \
    CFG_X(ABILITY_LICH_PLANARSTORM_COOLDOWN                      , "ability.lich.planarstorm.cooldown",         24) \
    CFG_X(ABILITY_LICH_PLANARSTORM_DAMAGE_MAX                    , "ability.lich.planarstorm.damage_max",      12500) \
    CFG_X(ABILITY_LICH_PLANARSTORM_DAMAGE_MIN                    , "ability.lich.planarstorm.damage_min",       7500) \
    CFG_X(ABILITY_LICH_PLANARSTORM_LEVEL_REQ                     , "ability.lich.planarstorm.level_req",          4) \
    CFG_X(ABILITY_LICH_PLANARSTORM_MANA_COST                     , "ability.lich.planarstorm.mana_cost",       5000) \
    CFG_X(ABILITY_LICH_PLANARTRAVEL_DURATION_MAX                 , "ability.lich.planartravel.duration_max",          3) \
    CFG_X(ABILITY_LICH_PLANARTRAVEL_DURATION_MIN                 , "ability.lich.planartravel.duration_min",          2) \
    CFG_X(ABILITY_LICH_PLANARTRAVEL_LEVEL_REQ                    , "ability.lich.planartravel.level_req",          3) \
    CFG_X(ABILITY_LICH_PLANARTRAVEL_MANA_COST                    , "ability.lich.planartravel.mana_cost",        500) \
    CFG_X(ABILITY_LICH_PLANESHIFT_LEVEL_REQ                      , "ability.lich.planeshift.level_req",          5) \
    CFG_X(ABILITY_LICH_POLARITY_COOLDOWN                         , "ability.lich.polarity.cooldown",         12) \
    CFG_X(ABILITY_LICH_POLARITY_DRAIN_MAX                        , "ability.lich.polarity.drain_max",       4000) \
    CFG_X(ABILITY_LICH_POLARITY_DRAIN_MIN                        , "ability.lich.polarity.drain_min",       2000) \
    CFG_X(ABILITY_LICH_POLARITY_HEAL_DIVISOR                     , "ability.lich.polarity.heal_divisor",          2) \
    CFG_X(ABILITY_LICH_POLARITY_LEVEL_REQ                        , "ability.lich.polarity.level_req",          5) \
    CFG_X(ABILITY_LICH_POWERTRANSFER_COOLDOWN                    , "ability.lich.powertransfer.cooldown",         18) \
    CFG_X(ABILITY_LICH_POWERTRANSFER_HEAL_MAX                    , "ability.lich.powertransfer.heal_max",      10000) \
    CFG_X(ABILITY_LICH_POWERTRANSFER_HEAL_MIN                    , "ability.lich.powertransfer.heal_min",       7500) \
    CFG_X(ABILITY_LICH_POWERTRANSFER_LEVEL_REQ                   , "ability.lich.powertransfer.level_req",          4) \
    CFG_X(ABILITY_LICH_POWERTRANSFER_MANA_COST                   , "ability.lich.powertransfer.mana_cost",       5000) \
    CFG_X(ABILITY_LICH_SOULSUCK_COOLDOWN                         , "ability.lich.soulsuck.cooldown",         10) \
    CFG_X(ABILITY_LICH_SOULSUCK_DAMAGE_MAX                       , "ability.lich.soulsuck.damage_max",       1000) \
    CFG_X(ABILITY_LICH_SOULSUCK_DAMAGE_MIN                       , "ability.lich.soulsuck.damage_min",        250) \
    CFG_X(ABILITY_LICH_SOULSUCK_LEVEL_REQ                        , "ability.lich.soulsuck.level_req",          3) \
    CFG_X(ABILITY_LICH_STUDYLORE_EXP_PER_LEVEL                   , "ability.lich.studylore.exp_per_level",   10000000) \
    CFG_X(ABILITY_LICH_STUDYLORE_MAX_LEVEL                       , "ability.lich.studylore.max_level",          5) \
    CFG_X(ABILITY_LICH_SUMMONGOLEM_CHARM_DURATION                , "ability.lich.summongolem.charm_duration",        666) \
    CFG_X(ABILITY_LICH_SUMMONGOLEM_LEVEL_REQ                     , "ability.lich.summongolem.level_req",          4) \
    \
    /* =========== ABILITY - MAGE =========== */ \
    CFG_X(ABILITY_MAGE_BEWITCH_COOLDOWN                          , "ability.mage.bewitch.cooldown",          2) \
    CFG_X(ABILITY_MAGE_CHANT_BLESS_COOLDOWN                      , "ability.mage.chant_bless.cooldown",          6) \
    CFG_X(ABILITY_MAGE_CHANT_BLESS_MANA_COST                     , "ability.mage.chant_bless.mana_cost",       2500) \
    CFG_X(ABILITY_MAGE_CHANT_CURSE_COOLDOWN                      , "ability.mage.chant_curse.cooldown",          6) \
    CFG_X(ABILITY_MAGE_CHANT_CURSE_MANA_COST                     , "ability.mage.chant_curse.mana_cost",       2500) \
    CFG_X(ABILITY_MAGE_CHANT_DAMAGE_COOLDOWN                     , "ability.mage.chant_damage.cooldown",          8) \
    CFG_X(ABILITY_MAGE_CHANT_DAMAGE_MANA_COST                    , "ability.mage.chant_damage.mana_cost",       1000) \
    CFG_X(ABILITY_MAGE_CHANT_DAMAGE_PVP_CAP_RANGE_HIGH           , "ability.mage.chant_damage.pvp_cap_range_high",       1050) \
    CFG_X(ABILITY_MAGE_CHANT_DAMAGE_PVP_CAP_RANGE_LOW            , "ability.mage.chant_damage.pvp_cap_range_low",        950) \
    CFG_X(ABILITY_MAGE_CHANT_DAMAGE_PVP_DAM_CAP                  , "ability.mage.chant_damage.pvp_dam_cap",       1000) \
    CFG_X(ABILITY_MAGE_CHANT_HEAL_COOLDOWN                       , "ability.mage.chant_heal.cooldown",          8) \
    CFG_X(ABILITY_MAGE_CHANT_HEAL_MANA_COST                      , "ability.mage.chant_heal.mana_cost",       1500) \
    CFG_X(ABILITY_MAGE_CHAOSMAGIC_COOLDOWN                       , "ability.mage.chaosmagic.cooldown",          6) \
    CFG_X(ABILITY_MAGE_CHAOSMAGIC_MANA_COST                      , "ability.mage.chaosmagic.mana_cost",       1500) \
    CFG_X(ABILITY_MAGE_DISCHARGE_ENTROPY_BONUS                   , "ability.mage.discharge.entropy_bonus",        100) \
    CFG_X(ABILITY_MAGE_DISCHARGE_LEVEL_REQ                       , "ability.mage.discharge.level_req",          4) \
    CFG_X(ABILITY_MAGE_INVOKE_ALL_LEVEL_REQ                      , "ability.mage.invoke_all.level_req",          9) \
    CFG_X(ABILITY_MAGE_INVOKE_BEAST_LEVEL_REQ                    , "ability.mage.invoke_beast.level_req",          9) \
    CFG_X(ABILITY_MAGE_INVOKE_BEAST_PRACTICE_COST                , "ability.mage.invoke_beast.practice_cost",         10) \
    CFG_X(ABILITY_MAGE_INVOKE_DEFLECTOR_LEVEL_REQ                , "ability.mage.invoke_deflector.level_req",          5) \
    CFG_X(ABILITY_MAGE_INVOKE_DEFLECTOR_PRACTICE_COST            , "ability.mage.invoke_deflector.practice_cost",          5) \
    CFG_X(ABILITY_MAGE_INVOKE_ILLUSIONS_LEVEL_REQ                , "ability.mage.invoke_illusions.level_req",          8) \
    CFG_X(ABILITY_MAGE_INVOKE_ILLUSIONS_PRACTICE_COST            , "ability.mage.invoke_illusions.practice_cost",          5) \
    CFG_X(ABILITY_MAGE_INVOKE_LEARN_COST_MULTIPLIER              , "ability.mage.invoke_learn.cost_multiplier",         20) \
    CFG_X(ABILITY_MAGE_INVOKE_LEARN_MAX_LEVEL                    , "ability.mage.invoke_learn.max_level",         10) \
    CFG_X(ABILITY_MAGE_INVOKE_MAGESHIELD_LEVEL_REQ               , "ability.mage.invoke_mageshield.level_req",          2) \
    CFG_X(ABILITY_MAGE_INVOKE_MAGESHIELD_PRACTICE_COST           , "ability.mage.invoke_mageshield.practice_cost",         25) \
    CFG_X(ABILITY_MAGE_INVOKE_STEELSHIELD_LEVEL_REQ              , "ability.mage.invoke_steelshield.level_req",          6) \
    CFG_X(ABILITY_MAGE_INVOKE_STEELSHIELD_PRACTICE_COST          , "ability.mage.invoke_steelshield.practice_cost",          5) \
    CFG_X(ABILITY_MAGE_MAGEARMOR_PRACTICE_COST                   , "ability.mage.magearmor.practice_cost",         60) \
    CFG_X(ABILITY_MAGE_REVEAL_MANA_COST                          , "ability.mage.reveal.mana_cost",       5000) \
    CFG_X(ABILITY_MAGE_TELEPORT_LEVEL_REQ                        , "ability.mage.teleport.level_req",          1) \
    CFG_X(ABILITY_MAGE_TELEPORT_MANA_COST                        , "ability.mage.teleport.mana_cost",        250) \
    \
    /* =========== ABILITY - MINDFLAYER =========== */ \
    CFG_X(ABILITY_MINDFLAYER_DAMCAP_FOCUS_MULT                   , "ability.mindflayer.damcap.focus_mult",          8) \
    CFG_X(ABILITY_MINDFLAYER_DAMCAP_HIVEMIND                     , "ability.mindflayer.damcap.hivemind",        200) \
    CFG_X(ABILITY_MINDFLAYER_ENTHRAL_DURATION                    , "ability.mindflayer.enthral.duration",         60) \
    CFG_X(ABILITY_MINDFLAYER_ENTHRAL_FOCUS_COST                  , "ability.mindflayer.enthral.focus_cost",         40) \
    CFG_X(ABILITY_MINDFLAYER_ENTHRAL_FOCUS_REQ                   , "ability.mindflayer.enthral.focus_req",         50) \
    CFG_X(ABILITY_MINDFLAYER_ENTHRAL_MANA_COST                   , "ability.mindflayer.enthral.mana_cost",        120) \
    CFG_X(ABILITY_MINDFLAYER_ENTHRAL_MAX_THRALLS                 , "ability.mindflayer.enthral.max_thralls",          3) \
    CFG_X(ABILITY_MINDFLAYER_FOCUS_DECAY_RATE                    , "ability.mindflayer.focus.decay_rate",          1) \
    CFG_X(ABILITY_MINDFLAYER_FOCUS_MAX                           , "ability.mindflayer.focus.max",        150) \
    CFG_X(ABILITY_MINDFLAYER_HIVEMIND_DURATION                   , "ability.mindflayer.hivemind.duration",         20) \
    CFG_X(ABILITY_MINDFLAYER_HIVEMIND_FOCUS_COST                 , "ability.mindflayer.hivemind.focus_cost",         50) \
    CFG_X(ABILITY_MINDFLAYER_HIVEMIND_MANA_COST                  , "ability.mindflayer.hivemind.mana_cost",        100) \
    CFG_X(ABILITY_MINDFLAYER_INTELLECTDEVOUR_COOLDOWN            , "ability.mindflayer.intellectdevour.cooldown",         15) \
    CFG_X(ABILITY_MINDFLAYER_INTELLECTDEVOUR_FOCUS_COST          , "ability.mindflayer.intellectdevour.focus_cost",         60) \
    CFG_X(ABILITY_MINDFLAYER_INTELLECTDEVOUR_FOCUS_REQ           , "ability.mindflayer.intellectdevour.focus_req",         80) \
    CFG_X(ABILITY_MINDFLAYER_INTELLECTDEVOUR_INSTAKILL_THRESHOLD , "ability.mindflayer.intellectdevour.instakill_threshold",        200) \
    CFG_X(ABILITY_MINDFLAYER_INTELLECTDEVOUR_MANA_COST           , "ability.mindflayer.intellectdevour.mana_cost",        150) \
    CFG_X(ABILITY_MINDFLAYER_INTELLECTDEVOUR_MAXHP_REDUCTION     , "ability.mindflayer.intellectdevour.maxhp_reduction",         10) \
    CFG_X(ABILITY_MINDFLAYER_MASSDOMINATION_COOLDOWN             , "ability.mindflayer.massdomination.cooldown",         30) \
    CFG_X(ABILITY_MINDFLAYER_MASSDOMINATION_FOCUS_COST           , "ability.mindflayer.massdomination.focus_cost",        100) \
    CFG_X(ABILITY_MINDFLAYER_MASSDOMINATION_FOCUS_REQ            , "ability.mindflayer.massdomination.focus_req",        100) \
    CFG_X(ABILITY_MINDFLAYER_MASSDOMINATION_MANA_COST            , "ability.mindflayer.massdomination.mana_cost",        200) \
    CFG_X(ABILITY_MINDFLAYER_MASSDOMINATION_SUCCESS_RATE         , "ability.mindflayer.massdomination.success_rate",         50) \
    CFG_X(ABILITY_MINDFLAYER_MEMORYDRAIN_COOLDOWN                , "ability.mindflayer.memorydrain.cooldown",          8) \
    CFG_X(ABILITY_MINDFLAYER_MEMORYDRAIN_FOCUS_COST              , "ability.mindflayer.memorydrain.focus_cost",         20) \
    CFG_X(ABILITY_MINDFLAYER_MEMORYDRAIN_FOCUS_GAIN              , "ability.mindflayer.memorydrain.focus_gain",         15) \
    CFG_X(ABILITY_MINDFLAYER_MEMORYDRAIN_FOCUS_REQ               , "ability.mindflayer.memorydrain.focus_req",         30) \
    CFG_X(ABILITY_MINDFLAYER_MEMORYDRAIN_MANA_COST               , "ability.mindflayer.memorydrain.mana_cost",         80) \
    CFG_X(ABILITY_MINDFLAYER_MEMORYDRAIN_MANA_DRAIN              , "ability.mindflayer.memorydrain.mana_drain",        500) \
    CFG_X(ABILITY_MINDFLAYER_MINDBLAST_COOLDOWN                  , "ability.mindflayer.mindblast.cooldown",          8) \
    CFG_X(ABILITY_MINDFLAYER_MINDBLAST_FOCUS_COST                , "ability.mindflayer.mindblast.focus_cost",         30) \
    CFG_X(ABILITY_MINDFLAYER_MINDBLAST_FOCUS_REQ                 , "ability.mindflayer.mindblast.focus_req",         40) \
    CFG_X(ABILITY_MINDFLAYER_MINDBLAST_MANA_COST                 , "ability.mindflayer.mindblast.mana_cost",         80) \
    CFG_X(ABILITY_MINDFLAYER_MINDBLAST_STUN_DURATION             , "ability.mindflayer.mindblast.stun_duration",          2) \
    CFG_X(ABILITY_MINDFLAYER_MINDFEED_COOLDOWN                   , "ability.mindflayer.mindfeed.cooldown",          6) \
    CFG_X(ABILITY_MINDFLAYER_MINDFEED_FOCUS_DRAIN                , "ability.mindflayer.mindfeed.focus_drain",         20) \
    CFG_X(ABILITY_MINDFLAYER_MINDFEED_FOCUS_GAIN                 , "ability.mindflayer.mindfeed.focus_gain",         10) \
    CFG_X(ABILITY_MINDFLAYER_MINDFEED_HEAL_PCT                   , "ability.mindflayer.mindfeed.heal_pct",         30) \
    CFG_X(ABILITY_MINDFLAYER_MINDFEED_MANA_COST                  , "ability.mindflayer.mindfeed.mana_cost",         60) \
    CFG_X(ABILITY_MINDFLAYER_MINDFLAYERARMOR_PRIMAL_COST         , "ability.mindflayer.mindflayerarmor.primal_cost",        150) \
    CFG_X(ABILITY_MINDFLAYER_PSIBLAST_COOLDOWN                   , "ability.mindflayer.psiblast.cooldown",          8) \
    CFG_X(ABILITY_MINDFLAYER_PSIBLAST_DAM_MAX                    , "ability.mindflayer.psiblast.dam_max",        500) \
    CFG_X(ABILITY_MINDFLAYER_PSIBLAST_DAM_MIN                    , "ability.mindflayer.psiblast.dam_min",        300) \
    CFG_X(ABILITY_MINDFLAYER_PSIBLAST_FOCUS_COST                 , "ability.mindflayer.psiblast.focus_cost",         50) \
    CFG_X(ABILITY_MINDFLAYER_PSIBLAST_MANA_COST                  , "ability.mindflayer.psiblast.mana_cost",        120) \
    CFG_X(ABILITY_MINDFLAYER_PSIMEDITATE_FOCUS_GAIN              , "ability.mindflayer.psimeditate.focus_gain",         20) \
    CFG_X(ABILITY_MINDFLAYER_PSYCHICMAELSTROM_CONFUSE_CHANCE     , "ability.mindflayer.psychicmaelstrom.confuse_chance",         30) \
    CFG_X(ABILITY_MINDFLAYER_PSYCHICMAELSTROM_COOLDOWN           , "ability.mindflayer.psychicmaelstrom.cooldown",         10) \
    CFG_X(ABILITY_MINDFLAYER_PSYCHICMAELSTROM_FOCUS_COST         , "ability.mindflayer.psychicmaelstrom.focus_cost",         40) \
    CFG_X(ABILITY_MINDFLAYER_PSYCHICMAELSTROM_MANA_COST          , "ability.mindflayer.psychicmaelstrom.mana_cost",        100) \
    CFG_X(ABILITY_MINDFLAYER_PUPPET_FOCUS_COST                   , "ability.mindflayer.puppet.focus_cost",         15) \
    CFG_X(ABILITY_MINDFLAYER_PUPPET_MANA_COST                    , "ability.mindflayer.puppet.mana_cost",         50) \
    CFG_X(ABILITY_MINDFLAYER_REALITYFRACTURE_COOLDOWN            , "ability.mindflayer.realityfracture.cooldown",         20) \
    CFG_X(ABILITY_MINDFLAYER_REALITYFRACTURE_DAM_MAX             , "ability.mindflayer.realityfracture.dam_max",       1000) \
    CFG_X(ABILITY_MINDFLAYER_REALITYFRACTURE_DAM_MIN             , "ability.mindflayer.realityfracture.dam_min",        500) \
    CFG_X(ABILITY_MINDFLAYER_REALITYFRACTURE_FOCUS_COST          , "ability.mindflayer.realityfracture.focus_cost",         80) \
    CFG_X(ABILITY_MINDFLAYER_REALITYFRACTURE_FOCUS_REQ           , "ability.mindflayer.realityfracture.focus_req",        100) \
    CFG_X(ABILITY_MINDFLAYER_REALITYFRACTURE_MANA_COST           , "ability.mindflayer.realityfracture.mana_cost",        200) \
    \
    /* =========== ABILITY - MONK =========== */ \
    CFG_X(ABILITY_MONK_ADAMANTIUM_LEVEL_REQ                      , "ability.monk.adamantium.level_req",          1) \
    CFG_X(ABILITY_MONK_BACKFIST_COOLDOWN                         , "ability.monk.backfist.cooldown",         11) \
    CFG_X(ABILITY_MONK_CELESTIAL_LEVEL_REQ                       , "ability.monk.celestial.level_req",         10) \
    CFG_X(ABILITY_MONK_CELESTIAL_MOVE_COST                       , "ability.monk.celestial.move_cost",        250) \
    CFG_X(ABILITY_MONK_CHANDS_LEVEL_REQ                          , "ability.monk.chands.level_req",          9) \
    CFG_X(ABILITY_MONK_CHI_COOLDOWN                              , "ability.monk.chi.cooldown",         12) \
    CFG_X(ABILITY_MONK_CHI_MOVE_COST_BASE                        , "ability.monk.chi.move_cost_base",        500) \
    CFG_X(ABILITY_MONK_CHI_MOVE_COST_PER_LEVEL                   , "ability.monk.chi.move_cost_per_level",         20) \
    CFG_X(ABILITY_MONK_CLOAK_LEVEL_REQ                           , "ability.monk.cloak.level_req",         11) \
    CFG_X(ABILITY_MONK_CLOAK_MOVE_COST                           , "ability.monk.cloak.move_cost",       1000) \
    CFG_X(ABILITY_MONK_DARKBLAZE_BLIND_DURATION                  , "ability.monk.darkblaze.blind_duration",         60) \
    CFG_X(ABILITY_MONK_DARKBLAZE_COOLDOWN                        , "ability.monk.darkblaze.cooldown",         18) \
    CFG_X(ABILITY_MONK_DARKBLAZE_LEVEL_REQ                       , "ability.monk.darkblaze.level_req",          8) \
    CFG_X(ABILITY_MONK_DEATHTOUCH_COOLDOWN                       , "ability.monk.deathtouch.cooldown",         12) \
    CFG_X(ABILITY_MONK_DEATHTOUCH_LEVEL_REQ                      , "ability.monk.deathtouch.level_req",          4) \
    CFG_X(ABILITY_MONK_ELBOW_COOLDOWN                            , "ability.monk.elbow.cooldown",         11) \
    CFG_X(ABILITY_MONK_FIND_DAM_PVP_DAM_CAP_MAX                  , "ability.monk.find_dam.pvp_dam_cap_max",       1020) \
    CFG_X(ABILITY_MONK_FIND_DAM_PVP_DAM_CAP_MIN                  , "ability.monk.find_dam.pvp_dam_cap_min",        940) \
    CFG_X(ABILITY_MONK_FIND_DAM_PVP_DAM_THRESHOLD                , "ability.monk.find_dam.pvp_dam_threshold",       1000) \
    CFG_X(ABILITY_MONK_FLAMINGHANDS_LEVEL_REQ                    , "ability.monk.flaminghands.level_req",          5) \
    CFG_X(ABILITY_MONK_GHOLD_LEVEL_REQ                           , "ability.monk.ghold.level_req",         13) \
    CFG_X(ABILITY_MONK_GODSBLESS_COOLDOWN                        , "ability.monk.godsbless.cooldown",         12) \
    CFG_X(ABILITY_MONK_GODSBLESS_LEVEL_REQ                       , "ability.monk.godsbless.level_req",          7) \
    CFG_X(ABILITY_MONK_GODSBLESS_MANA_COST                       , "ability.monk.godsbless.mana_cost",       3000) \
    CFG_X(ABILITY_MONK_GODSEYE_LEVEL_REQ                         , "ability.monk.godseye.level_req",          1) \
    CFG_X(ABILITY_MONK_GODSFAVOR_COOLDOWN                        , "ability.monk.godsfavor.cooldown",          4) \
    CFG_X(ABILITY_MONK_GODSFAVOR_LEVEL_REQ                       , "ability.monk.godsfavor.level_req",          8) \
    CFG_X(ABILITY_MONK_GODSFAVOR_MOVE_COST                       , "ability.monk.godsfavor.move_cost",       1500) \
    CFG_X(ABILITY_MONK_GODSHEAL_COOLDOWN_FIGHTING                , "ability.monk.godsheal.cooldown_fighting",         12) \
    CFG_X(ABILITY_MONK_GODSHEAL_COOLDOWN_NONFIGHTING             , "ability.monk.godsheal.cooldown_nonfighting",          8) \
    CFG_X(ABILITY_MONK_GODSHEAL_HEAL_FIGHTING                    , "ability.monk.godsheal.heal_fighting",        150) \
    CFG_X(ABILITY_MONK_GODSHEAL_HEAL_NONFIGHTING                 , "ability.monk.godsheal.heal_nonfighting",        500) \
    CFG_X(ABILITY_MONK_GODSHEAL_LEVEL_REQ                        , "ability.monk.godsheal.level_req",         12) \
    CFG_X(ABILITY_MONK_GODSHEAL_MANA_CHECK                       , "ability.monk.godsheal.mana_check",        300) \
    CFG_X(ABILITY_MONK_GODSHEAL_MANA_COST                        , "ability.monk.godsheal.mana_cost",        400) \
    CFG_X(ABILITY_MONK_GUIDE_EXP_COST                            , "ability.monk.guide.exp_cost",      50000) \
    CFG_X(ABILITY_MONK_HEALINGTOUCH_COOLDOWN                     , "ability.monk.healingtouch.cooldown",         12) \
    CFG_X(ABILITY_MONK_HEALINGTOUCH_LEVEL_REQ                    , "ability.monk.healingtouch.level_req",          3) \
    CFG_X(ABILITY_MONK_KNEE_COOLDOWN                             , "ability.monk.knee.cooldown",         11) \
    CFG_X(ABILITY_MONK_LEARN_ABILITY_COST                        , "ability.monk.learn.ability_cost",     500000) \
    CFG_X(ABILITY_MONK_LEARN_ABILITY_MAX_RANK                    , "ability.monk.learn.ability_max_rank",          4) \
    CFG_X(ABILITY_MONK_LEARN_CHI_COST_MULTIPLIER                 , "ability.monk.learn.chi_cost_multiplier",    1000000) \
    CFG_X(ABILITY_MONK_LEARN_CHI_MAX_RANK                        , "ability.monk.learn.chi_max_rank",          6) \
    CFG_X(ABILITY_MONK_LEARN_FIGHT_COST                          , "ability.monk.learn.fight_cost",      50000) \
    CFG_X(ABILITY_MONK_LEARN_TECHNIQUE_COST                      , "ability.monk.learn.technique_cost",     200000) \
    CFG_X(ABILITY_MONK_MANTRA_COST_MULTIPLIER                    , "ability.monk.mantra.cost_multiplier",         10) \
    CFG_X(ABILITY_MONK_MANTRA_MAX_LEVEL                          , "ability.monk.mantra.max_level",         14) \
    CFG_X(ABILITY_MONK_MONKARMOR_PRIMAL_COST                     , "ability.monk.monkarmor.primal_cost",         60) \
    CFG_X(ABILITY_MONK_PALMSTRIKE_COOLDOWN                       , "ability.monk.palmstrike.cooldown",         11) \
    CFG_X(ABILITY_MONK_RELAX_COOLDOWN                            , "ability.monk.relax.cooldown",         12) \
    CFG_X(ABILITY_MONK_SACREDINVIS_LEVEL_REQ                     , "ability.monk.sacredinvis.level_req",          3) \
    CFG_X(ABILITY_MONK_SACREDINVIS_MOVE_COST                     , "ability.monk.sacredinvis.move_cost",        500) \
    CFG_X(ABILITY_MONK_SHINKICK_COOLDOWN                         , "ability.monk.shinkick.cooldown",         12) \
    CFG_X(ABILITY_MONK_SPINKICK_COOLDOWN                         , "ability.monk.spinkick.cooldown",         11) \
    CFG_X(ABILITY_MONK_SPINKICK_LIGHTNING_MAX_HITS               , "ability.monk.spinkick.lightning_max_hits",          6) \
    CFG_X(ABILITY_MONK_SPIRITPOWER_DAMROLL_BONUS                 , "ability.monk.spiritpower.damroll_bonus",        200) \
    CFG_X(ABILITY_MONK_SPIRITPOWER_HITROLL_BONUS                 , "ability.monk.spiritpower.hitroll_bonus",        200) \
    CFG_X(ABILITY_MONK_SPIRITPOWER_LEVEL_REQ                     , "ability.monk.spiritpower.level_req",          3) \
    CFG_X(ABILITY_MONK_SPIRITPOWER_MOVE_CHECK                    , "ability.monk.spiritpower.move_check",        100) \
    CFG_X(ABILITY_MONK_SPIRITPOWER_MOVE_COST                     , "ability.monk.spiritpower.move_cost",         25) \
    CFG_X(ABILITY_MONK_STEELSKIN_LEVEL_REQ                       , "ability.monk.steelskin.level_req",          6) \
    CFG_X(ABILITY_MONK_SWEEP_COMBO_PARALYSE_DURATION             , "ability.monk.sweep.combo_paralyse_duration",         30) \
    CFG_X(ABILITY_MONK_SWEEP_COOLDOWN                            , "ability.monk.sweep.cooldown",         11) \
    CFG_X(ABILITY_MONK_THRUSTKICK_COOLDOWN                       , "ability.monk.thrustkick.cooldown",         11) \
    CFG_X(ABILITY_MONK_WRATHOFGOD_COOLDOWN                       , "ability.monk.wrathofgod.cooldown",          8) \
    CFG_X(ABILITY_MONK_WRATHOFGOD_LEVEL_REQ                      , "ability.monk.wrathofgod.level_req",          4) \
    CFG_X(ABILITY_MONK_WRATHOFGOD_NUM_HITS                       , "ability.monk.wrathofgod.num_hits",          4) \
    \
    /* =========== ABILITY - NINJA =========== */ \
    CFG_X(ABILITY_NINJA_BOMUZITE_COOLDOWN                        , "ability.ninja.bomuzite.cooldown",          1) \
    CFG_X(ABILITY_NINJA_BOMUZITE_LEVEL_REQ                       , "ability.ninja.bomuzite.level_req",          6) \
    CFG_X(ABILITY_NINJA_BOMUZITE_MOVE_COST                       , "ability.ninja.bomuzite.move_cost",        500) \
    CFG_X(ABILITY_NINJA_HARA_KIRI_LEVEL_REQ                      , "ability.ninja.hara_kiri.level_req",          6) \
    CFG_X(ABILITY_NINJA_HARA_KIRI_MIN_DURATION                   , "ability.ninja.hara_kiri.min_duration",          5) \
    CFG_X(ABILITY_NINJA_KAKUSU_LEVEL_REQ                         , "ability.ninja.kakusu.level_req",          3) \
    CFG_X(ABILITY_NINJA_KAKUSU_MOVE_COST                         , "ability.ninja.kakusu.move_cost",        500) \
    CFG_X(ABILITY_NINJA_KANZUITE_LEVEL_REQ                       , "ability.ninja.kanzuite.level_req",          5) \
    CFG_X(ABILITY_NINJA_KANZUITE_MOVE_COST                       , "ability.ninja.kanzuite.move_cost",        500) \
    CFG_X(ABILITY_NINJA_MICHI_COOLDOWN                           , "ability.ninja.michi.cooldown",         12) \
    CFG_X(ABILITY_NINJA_MICHI_MOVE_COST                          , "ability.ninja.michi.move_cost",        500) \
    CFG_X(ABILITY_NINJA_MICHI_RAGE_GAIN                          , "ability.ninja.michi.rage_gain",        100) \
    CFG_X(ABILITY_NINJA_MICHI_RAGE_THRESHOLD                     , "ability.ninja.michi.rage_threshold",        100) \
    CFG_X(ABILITY_NINJA_MIENAKU_LEVEL_REQ                        , "ability.ninja.mienaku.level_req",          3) \
    CFG_X(ABILITY_NINJA_MIENAKU_MOVE_COST                        , "ability.ninja.mienaku.move_cost",        200) \
    CFG_X(ABILITY_NINJA_NINJAARMOR_PRIMAL_COST                   , "ability.ninja.ninjaarmor.primal_cost",         60) \
    CFG_X(ABILITY_NINJA_PRINCIPLES_CHIKYU_MAX                    , "ability.ninja.principles.chikyu_max",          6) \
    CFG_X(ABILITY_NINJA_PRINCIPLES_NINGENNO_MAX                  , "ability.ninja.principles.ningenno_max",          6) \
    CFG_X(ABILITY_NINJA_PRINCIPLES_SORA_MAX                      , "ability.ninja.principles.sora_max",          6) \
    CFG_X(ABILITY_NINJA_STALK_MOVE_COST                          , "ability.ninja.stalk.move_cost",        500) \
    CFG_X(ABILITY_NINJA_STRANGLE_COOLDOWN_FAIL                   , "ability.ninja.strangle.cooldown_fail",         24) \
    CFG_X(ABILITY_NINJA_STRANGLE_COOLDOWN_SUCCESS                , "ability.ninja.strangle.cooldown_success",         12) \
    CFG_X(ABILITY_NINJA_STRANGLE_FAIL_THRESHOLD                  , "ability.ninja.strangle.fail_threshold",         15) \
    CFG_X(ABILITY_NINJA_STRANGLE_HITS_SUCCESS                    , "ability.ninja.strangle.hits_success",          4) \
    CFG_X(ABILITY_NINJA_STRANGLE_LEVEL_REQ                       , "ability.ninja.strangle.level_req",          2) \
    CFG_X(ABILITY_NINJA_TSUME_LEVEL_REQ                          , "ability.ninja.tsume.level_req",          1) \
    \
    /* =========== ABILITY - PSION =========== */ \
    CFG_X(ABILITY_PSION_BRAINBURN_COOLDOWN                       , "ability.psion.brainburn.cooldown",         10) \
    CFG_X(ABILITY_PSION_BRAINBURN_DOT_DURATION                   , "ability.psion.brainburn.dot_duration",          5) \
    CFG_X(ABILITY_PSION_BRAINBURN_FOCUS_COST                     , "ability.psion.brainburn.focus_cost",         30) \
    CFG_X(ABILITY_PSION_BRAINBURN_FOCUS_REQ                      , "ability.psion.brainburn.focus_req",         35) \
    CFG_X(ABILITY_PSION_BRAINBURN_INT_DRAIN                      , "ability.psion.brainburn.int_drain",          2) \
    CFG_X(ABILITY_PSION_BRAINBURN_MANA_COST                      , "ability.psion.brainburn.mana_cost",        100) \
    CFG_X(ABILITY_PSION_BRAINBURN_STUN_CHANCE                    , "ability.psion.brainburn.stun_chance",         20) \
    CFG_X(ABILITY_PSION_DAMCAP_BASE                              , "ability.psion.damcap.base",        150) \
    CFG_X(ABILITY_PSION_DAMCAP_FOCUS_MULT                        , "ability.psion.damcap.focus_mult",          6) \
    CFG_X(ABILITY_PSION_DAMCAP_THOUGHTSHIELD                     , "ability.psion.damcap.thoughtshield",        100) \
    CFG_X(ABILITY_PSION_FOCUS_COMBAT_GAIN                        , "ability.psion.focus.combat_gain",          2) \
    CFG_X(ABILITY_PSION_FOCUS_DECAY_RATE                         , "ability.psion.focus.decay_rate",          2) \
    CFG_X(ABILITY_PSION_FOCUS_MAX                                , "ability.psion.focus.max",        100) \
    CFG_X(ABILITY_PSION_FORCEPUSH_COOLDOWN                       , "ability.psion.forcepush.cooldown",          6) \
    CFG_X(ABILITY_PSION_FORCEPUSH_DAM_BONUS                      , "ability.psion.forcepush.dam_bonus",         80) \
    CFG_X(ABILITY_PSION_FORCEPUSH_DISARM_CHANCE                  , "ability.psion.forcepush.disarm_chance",         15) \
    CFG_X(ABILITY_PSION_FORCEPUSH_FOCUS_COST                     , "ability.psion.forcepush.focus_cost",         15) \
    CFG_X(ABILITY_PSION_FORCEPUSH_KNOCKBACK_CHANCE               , "ability.psion.forcepush.knockback_chance",         25) \
    CFG_X(ABILITY_PSION_FORCEPUSH_MANA_COST                      , "ability.psion.forcepush.mana_cost",         40) \
    CFG_X(ABILITY_PSION_KINETICBARRIER_ABSORB_AMOUNT             , "ability.psion.kineticbarrier.absorb_amount",       2000) \
    CFG_X(ABILITY_PSION_KINETICBARRIER_DURATION                  , "ability.psion.kineticbarrier.duration",         12) \
    CFG_X(ABILITY_PSION_KINETICBARRIER_FOCUS_COST                , "ability.psion.kineticbarrier.focus_cost",         25) \
    CFG_X(ABILITY_PSION_KINETICBARRIER_FOCUS_REQ                 , "ability.psion.kineticbarrier.focus_req",         30) \
    CFG_X(ABILITY_PSION_KINETICBARRIER_MANA_COST                 , "ability.psion.kineticbarrier.mana_cost",         70) \
    CFG_X(ABILITY_PSION_LEVITATE_DURATION                        , "ability.psion.levitate.duration",         10) \
    CFG_X(ABILITY_PSION_LEVITATE_FOCUS_COST                      , "ability.psion.levitate.focus_cost",         20) \
    CFG_X(ABILITY_PSION_LEVITATE_MANA_COST                       , "ability.psion.levitate.mana_cost",         50) \
    CFG_X(ABILITY_PSION_MEDITATE_COOLDOWN                        , "ability.psion.meditate.cooldown",          8) \
    CFG_X(ABILITY_PSION_MENTALLINK_DURATION                      , "ability.psion.mentallink.duration",         12) \
    CFG_X(ABILITY_PSION_MENTALLINK_FOCUS_COST                    , "ability.psion.mentallink.focus_cost",         25) \
    CFG_X(ABILITY_PSION_MENTALLINK_MANA_COST                     , "ability.psion.mentallink.mana_cost",         50) \
    CFG_X(ABILITY_PSION_MENTALLINK_SHARE_PCT                     , "ability.psion.mentallink.share_pct",         30) \
    CFG_X(ABILITY_PSION_MINDSCAN_DURATION                        , "ability.psion.mindscan.duration",         10) \
    CFG_X(ABILITY_PSION_MINDSCAN_FOCUS_REQ                       , "ability.psion.mindscan.focus_req",         20) \
    CFG_X(ABILITY_PSION_MINDSCAN_MANA_COST                       , "ability.psion.mindscan.mana_cost",         30) \
    CFG_X(ABILITY_PSION_MINDSPIKE_COOLDOWN                       , "ability.psion.mindspike.cooldown",          4) \
    CFG_X(ABILITY_PSION_MINDSPIKE_DAM_MAX                        , "ability.psion.mindspike.dam_max",        200) \
    CFG_X(ABILITY_PSION_MINDSPIKE_DAM_MIN                        , "ability.psion.mindspike.dam_min",        100) \
    CFG_X(ABILITY_PSION_MINDSPIKE_FOCUS_COST                     , "ability.psion.mindspike.focus_cost",         10) \
    CFG_X(ABILITY_PSION_MINDSPIKE_FOCUS_GAIN                     , "ability.psion.mindspike.focus_gain",          5) \
    CFG_X(ABILITY_PSION_MINDSPIKE_MANA_COST                      , "ability.psion.mindspike.mana_cost",         50) \
    CFG_X(ABILITY_PSION_PSIMEDITATE_FOCUS_GAIN                   , "ability.psion.psimeditate.focus_gain",         15) \
    CFG_X(ABILITY_PSION_PSIONARMOR_PRIMAL_COST                   , "ability.psion.psionarmor.primal_cost",         60) \
    CFG_X(ABILITY_PSION_PSYCHICSCREAM_COOLDOWN                   , "ability.psion.psychicscream.cooldown",          8) \
    CFG_X(ABILITY_PSION_PSYCHICSCREAM_FOCUS_COST                 , "ability.psion.psychicscream.focus_cost",         20) \
    CFG_X(ABILITY_PSION_PSYCHICSCREAM_FOCUS_REQ                  , "ability.psion.psychicscream.focus_req",         25) \
    CFG_X(ABILITY_PSION_PSYCHICSCREAM_MANA_COST                  , "ability.psion.psychicscream.mana_cost",         80) \
    CFG_X(ABILITY_PSION_PSYCHICSCREAM_STUN_CHANCE                , "ability.psion.psychicscream.stun_chance",         25) \
    CFG_X(ABILITY_PSION_THOUGHTSHIELD_ABSORB_AMOUNT              , "ability.psion.thoughtshield.absorb_amount",       1500) \
    CFG_X(ABILITY_PSION_THOUGHTSHIELD_DURATION                   , "ability.psion.thoughtshield.duration",         15) \
    CFG_X(ABILITY_PSION_THOUGHTSHIELD_MANA_COST                  , "ability.psion.thoughtshield.mana_cost",         60) \
    \
    /* =========== ABILITY - SAMURAI =========== */ \
    CFG_X(ABILITY_SAMURAI_BLADESPIN_WPN_SKILL_REQ                , "ability.samurai.bladespin.wpn_skill_req",       1000) \
    CFG_X(ABILITY_SAMURAI_BLOCK_COOLDOWN                         , "ability.samurai.block.cooldown",         12) \
    CFG_X(ABILITY_SAMURAI_BLOCK_FOCUS_COST                       , "ability.samurai.block.focus_cost",          4) \
    CFG_X(ABILITY_SAMURAI_BLOCK_FOCUS_MAX                        , "ability.samurai.block.focus_max",         40) \
    CFG_X(ABILITY_SAMURAI_COMBO_VICTIM_MIN_HP                    , "ability.samurai.combo.victim_min_hp",       1000) \
    CFG_X(ABILITY_SAMURAI_COMBO_10_HITS                          , "ability.samurai.combo_10.hits",          3) \
    CFG_X(ABILITY_SAMURAI_COMBO_15_FAIL_RANGE_MAX                , "ability.samurai.combo_15.fail_range_max",          3) \
    CFG_X(ABILITY_SAMURAI_COMBO_20_VICTIM_COOLDOWN               , "ability.samurai.combo_20.victim_cooldown",         24) \
    CFG_X(ABILITY_SAMURAI_COMBO_25_FAIL_RANGE_MAX                , "ability.samurai.combo_25.fail_range_max",          3) \
    CFG_X(ABILITY_SAMURAI_COMBO_30_HEAL_MAX                      , "ability.samurai.combo_30.heal_max",       4000) \
    CFG_X(ABILITY_SAMURAI_COMBO_30_HEAL_MIN                      , "ability.samurai.combo_30.heal_min",       2000) \
    CFG_X(ABILITY_SAMURAI_COMBO_35_HITS                          , "ability.samurai.combo_35.hits",          5) \
    CFG_X(ABILITY_SAMURAI_COUNTERMOVE_COOLDOWN                   , "ability.samurai.countermove.cooldown",         12) \
    CFG_X(ABILITY_SAMURAI_COUNTERMOVE_FOCUS_COST                 , "ability.samurai.countermove.focus_cost",          8) \
    CFG_X(ABILITY_SAMURAI_COUNTERMOVE_FOCUS_MAX                  , "ability.samurai.countermove.focus_max",         40) \
    CFG_X(ABILITY_SAMURAI_FOCUS_COOLDOWN                         , "ability.samurai.focus.cooldown",          8) \
    CFG_X(ABILITY_SAMURAI_HOLOGRAMTRANSFER_MOVE_COST             , "ability.samurai.hologramtransfer.move_cost",       1000) \
    CFG_X(ABILITY_SAMURAI_KATANA_PRIMAL_COST                     , "ability.samurai.katana.primal_cost",        250) \
    CFG_X(ABILITY_SAMURAI_KATANA_WEAPON_DICE_COUNT               , "ability.samurai.katana.weapon_dice_count",         65) \
    CFG_X(ABILITY_SAMURAI_KATANA_WEAPON_DICE_SIZE                , "ability.samurai.katana.weapon_dice_size",        115) \
    CFG_X(ABILITY_SAMURAI_MARTIAL_EXP_COST                       , "ability.samurai.martial.exp_cost",  150000000) \
    CFG_X(ABILITY_SAMURAI_SIDESTEP_COOLDOWN                      , "ability.samurai.sidestep.cooldown",         12) \
    CFG_X(ABILITY_SAMURAI_SIDESTEP_FOCUS_COST                    , "ability.samurai.sidestep.focus_cost",          2) \
    CFG_X(ABILITY_SAMURAI_SIDESTEP_FOCUS_MAX                     , "ability.samurai.sidestep.focus_max",         40) \
    CFG_X(ABILITY_SAMURAI_SLIDE_COOLDOWN                         , "ability.samurai.slide.cooldown",         12) \
    CFG_X(ABILITY_SAMURAI_SLIDE_FOCUS_COST                       , "ability.samurai.slide.focus_cost",          1) \
    CFG_X(ABILITY_SAMURAI_SLIDE_FOCUS_MAX                        , "ability.samurai.slide.focus_max",         40) \
    \
    /* =========== ABILITY - SHAPESHIFTER =========== */ \
    CFG_X(ABILITY_SHAPESHIFTER_BREATH_COOLDOWN                   , "ability.shapeshifter.breath.cooldown",         24) \
    CFG_X(ABILITY_SHAPESHIFTER_CAMOUFLAGE_LEVEL_REQ              , "ability.shapeshifter.camouflage.level_req",          1) \
    CFG_X(ABILITY_SHAPESHIFTER_CHARGE_COOLDOWN                   , "ability.shapeshifter.charge.cooldown",         18) \
    CFG_X(ABILITY_SHAPESHIFTER_CHARGE_LEVEL_REQ                  , "ability.shapeshifter.charge.level_req",          4) \
    CFG_X(ABILITY_SHAPESHIFTER_CHARGE_MOVE_COST                  , "ability.shapeshifter.charge.move_cost",       2000) \
    CFG_X(ABILITY_SHAPESHIFTER_FAERIEBLINK_COOLDOWN              , "ability.shapeshifter.faerieblink.cooldown",         18) \
    CFG_X(ABILITY_SHAPESHIFTER_FAERIEBLINK_LEVEL_REQ             , "ability.shapeshifter.faerieblink.level_req",          5) \
    CFG_X(ABILITY_SHAPESHIFTER_FAERIEBLINK_MANA_COST             , "ability.shapeshifter.faerieblink.mana_cost",       2500) \
    CFG_X(ABILITY_SHAPESHIFTER_FAERIECURSE_COOLDOWN              , "ability.shapeshifter.faeriecurse.cooldown",         12) \
    CFG_X(ABILITY_SHAPESHIFTER_FAERIECURSE_LEVEL_REQ             , "ability.shapeshifter.faeriecurse.level_req",          4) \
    CFG_X(ABILITY_SHAPESHIFTER_FAERIECURSE_MANA_COST             , "ability.shapeshifter.faeriecurse.mana_cost",       1000) \
    CFG_X(ABILITY_SHAPESHIFTER_FAERIECURSE_MOVE_COST             , "ability.shapeshifter.faeriecurse.move_cost",        500) \
    CFG_X(ABILITY_SHAPESHIFTER_FORMLEARN_COST_MULTIPLIER         , "ability.shapeshifter.formlearn.cost_multiplier",         80) \
    CFG_X(ABILITY_SHAPESHIFTER_FORMLEARN_MAX_LEVEL               , "ability.shapeshifter.formlearn.max_level",          5) \
    CFG_X(ABILITY_SHAPESHIFTER_HATFORM_LEVEL_REQ                 , "ability.shapeshifter.hatform.level_req",          3) \
    CFG_X(ABILITY_SHAPESHIFTER_MISTWALK_LEVEL_REQ                , "ability.shapeshifter.mistwalk.level_req",          2) \
    CFG_X(ABILITY_SHAPESHIFTER_MISTWALK_MOVE_COST                , "ability.shapeshifter.mistwalk.move_cost",        250) \
    CFG_X(ABILITY_SHAPESHIFTER_PHASE_DURATION                    , "ability.shapeshifter.phase.duration",         10) \
    CFG_X(ABILITY_SHAPESHIFTER_PHASE_LEVEL_REQ                   , "ability.shapeshifter.phase.level_req",          5) \
    CFG_X(ABILITY_SHAPESHIFTER_SHAPEARMOR_PRIMAL_COST            , "ability.shapeshifter.shapearmor.primal_cost",        150) \
    CFG_X(ABILITY_SHAPESHIFTER_SHAPEROAR_COOLDOWN                , "ability.shapeshifter.shaperoar.cooldown",         16) \
    CFG_X(ABILITY_SHAPESHIFTER_SHAPEROAR_LEVEL_REQ               , "ability.shapeshifter.shaperoar.level_req",          3) \
    CFG_X(ABILITY_SHAPESHIFTER_SHAPESHIFT_LEVEL_REQ              , "ability.shapeshifter.shapeshift.level_req",          4) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_BULL_ARMOR                  , "ability.shapeshifter.shift.bull_armor",        -50) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_BULL_DAMROLL                , "ability.shapeshifter.shift.bull_damroll",        400) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_BULL_HITROLL                , "ability.shapeshifter.shift.bull_hitroll",        400) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_COUNTER_INCREMENT           , "ability.shapeshifter.shift.counter_increment",         10) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_COUNTER_MAX                 , "ability.shapeshifter.shift.counter_max",         35) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_FAERIE_ARMOR                , "ability.shapeshifter.shift.faerie_armor",       -500) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_FAERIE_DAMROLL              , "ability.shapeshifter.shift.faerie_damroll",        250) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_FAERIE_HITROLL              , "ability.shapeshifter.shift.faerie_hitroll",        250) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_HEAL_CAP                    , "ability.shapeshifter.shift.heal_cap",       5000) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_HYDRA_ARMOR                 , "ability.shapeshifter.shift.hydra_armor",        -50) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_HYDRA_DAMROLL               , "ability.shapeshifter.shift.hydra_damroll",        450) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_HYDRA_HITROLL               , "ability.shapeshifter.shift.hydra_hitroll",        450) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_TIGER_ARMOR                 , "ability.shapeshifter.shift.tiger_armor",       -200) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_TIGER_DAMROLL               , "ability.shapeshifter.shift.tiger_damroll",        300) \
    CFG_X(ABILITY_SHAPESHIFTER_SHIFT_TIGER_HITROLL               , "ability.shapeshifter.shift.tiger_hitroll",        450) \
    CFG_X(ABILITY_SHAPESHIFTER_STOMP_COOLDOWN                    , "ability.shapeshifter.stomp.cooldown",         24) \
    CFG_X(ABILITY_SHAPESHIFTER_STOMP_DAMAGE                      , "ability.shapeshifter.stomp.damage",        500) \
    CFG_X(ABILITY_SHAPESHIFTER_STOMP_LEVEL_REQ                   , "ability.shapeshifter.stomp.level_req",          5) \
    \
    /* =========== ABILITY - SIREN =========== */ \
    CFG_X(ABILITY_SIREN_ARIAOFUNMAKING_COOLDOWN                  , "ability.siren.ariaofunmaking.cooldown",         15) \
    CFG_X(ABILITY_SIREN_ARIAOFUNMAKING_MANA_COST                 , "ability.siren.ariaofunmaking.mana_cost",        150) \
    CFG_X(ABILITY_SIREN_ARIAOFUNMAKING_PVP_RESIST_PCT            , "ability.siren.ariaofunmaking.pvp_resist_pct",         50) \
    CFG_X(ABILITY_SIREN_ARIAOFUNMAKING_RESONANCE_COST            , "ability.siren.ariaofunmaking.resonance_cost",         60) \
    CFG_X(ABILITY_SIREN_BANSHEEWAIL_COOLDOWN                     , "ability.siren.bansheewail.cooldown",         15) \
    CFG_X(ABILITY_SIREN_BANSHEEWAIL_INSTAKILL_THRESHOLD          , "ability.siren.bansheewail.instakill_threshold",        500) \
    CFG_X(ABILITY_SIREN_BANSHEEWAIL_MANA_COST                    , "ability.siren.bansheewail.mana_cost",        150) \
    CFG_X(ABILITY_SIREN_BANSHEEWAIL_RESONANCE_COST               , "ability.siren.bansheewail.resonance_cost",         60) \
    CFG_X(ABILITY_SIREN_BANSHEEWAIL_RESONANCE_REQ                , "ability.siren.bansheewail.resonance_req",         80) \
    CFG_X(ABILITY_SIREN_CACOPHONY_COOLDOWN                       , "ability.siren.cacophony.cooldown",         12) \
    CFG_X(ABILITY_SIREN_CACOPHONY_MANA_COST                      , "ability.siren.cacophony.mana_cost",        120) \
    CFG_X(ABILITY_SIREN_CACOPHONY_RESONANCE_COST                 , "ability.siren.cacophony.resonance_cost",         40) \
    CFG_X(ABILITY_SIREN_COMMANDVOICE_COOLDOWN                    , "ability.siren.commandvoice.cooldown",         10) \
    CFG_X(ABILITY_SIREN_COMMANDVOICE_MANA_COST                   , "ability.siren.commandvoice.mana_cost",        100) \
    CFG_X(ABILITY_SIREN_COMMANDVOICE_RESONANCE_COST              , "ability.siren.commandvoice.resonance_cost",         30) \
    CFG_X(ABILITY_SIREN_CRESCENDO_FINALE_DAM_MULT                , "ability.siren.crescendo.finale_dam_mult",          2) \
    CFG_X(ABILITY_SIREN_CRESCENDO_MANA_PER_STAGE                 , "ability.siren.crescendo.mana_per_stage",         50) \
    CFG_X(ABILITY_SIREN_ECHOSHIELD_DURATION                      , "ability.siren.echoshield.duration",         10) \
    CFG_X(ABILITY_SIREN_ECHOSHIELD_MANA_COST                     , "ability.siren.echoshield.mana_cost",         80) \
    CFG_X(ABILITY_SIREN_ECHOSHIELD_REFLECT_PCT                   , "ability.siren.echoshield.reflect_pct",         20) \
    CFG_X(ABILITY_SIREN_ENTHRALL_DURATION                        , "ability.siren.enthrall.duration",         30) \
    CFG_X(ABILITY_SIREN_ENTHRALL_MANA_COST                       , "ability.siren.enthrall.mana_cost",        140) \
    CFG_X(ABILITY_SIREN_ENTHRALL_MAX_FOLLOWERS                   , "ability.siren.enthrall.max_followers",          2) \
    CFG_X(ABILITY_SIREN_ENTHRALL_RESONANCE_COST                  , "ability.siren.enthrall.resonance_cost",         50) \
    CFG_X(ABILITY_SIREN_MESMERIZE_COOLDOWN                       , "ability.siren.mesmerize.cooldown",         12) \
    CFG_X(ABILITY_SIREN_MESMERIZE_MANA_COST                      , "ability.siren.mesmerize.mana_cost",        100) \
    CFG_X(ABILITY_SIREN_MESMERIZE_RESONANCE_COST                 , "ability.siren.mesmerize.resonance_cost",         35) \
    CFG_X(ABILITY_SIREN_MESMERIZE_STUN_DURATION                  , "ability.siren.mesmerize.stun_duration",          3) \
    CFG_X(ABILITY_SIREN_SIRENARMOR_PRIMAL_COST                   , "ability.siren.sirenarmor.primal_cost",         60) \
    CFG_X(ABILITY_SIREN_SIRENSONG_COOLDOWN                       , "ability.siren.sirensong.cooldown",         20) \
    CFG_X(ABILITY_SIREN_SIRENSONG_MANA_COST                      , "ability.siren.sirensong.mana_cost",        200) \
    CFG_X(ABILITY_SIREN_SIRENSONG_RESONANCE_COST                 , "ability.siren.sirensong.resonance_cost",         80) \
    CFG_X(ABILITY_SIREN_SOULREND_BYPASS_PCT                      , "ability.siren.soulrend.bypass_pct",         40) \
    CFG_X(ABILITY_SIREN_SOULREND_COOLDOWN                        , "ability.siren.soulrend.cooldown",          8) \
    CFG_X(ABILITY_SIREN_SOULREND_MANA_COST                       , "ability.siren.soulrend.mana_cost",        100) \
    CFG_X(ABILITY_SIREN_SOULREND_RESONANCE_COST                  , "ability.siren.soulrend.resonance_cost",         20) \
    \
    /* =========== ABILITY - SPIDERDROID =========== */ \
    CFG_X(ABILITY_SPIDERDROID_CUBEFORM_BODY_LEVEL_REQ            , "ability.spiderdroid.cubeform.body_level_req",          5) \
    CFG_X(ABILITY_SPIDERDROID_CUBEFORM_DAMROLL_BONUS             , "ability.spiderdroid.cubeform.damroll_bonus",        250) \
    CFG_X(ABILITY_SPIDERDROID_CUBEFORM_HITROLL_BONUS             , "ability.spiderdroid.cubeform.hitroll_bonus",        250) \
    CFG_X(ABILITY_SPIDERDROID_CUBEFORM_MANA_COST                 , "ability.spiderdroid.cubeform.mana_cost",       2000) \
    CFG_X(ABILITY_SPIDERDROID_CUBEFORM_MOVE_COST                 , "ability.spiderdroid.cubeform.move_cost",       2000) \
    CFG_X(ABILITY_SPIDERDROID_DRIDEREQ_PRIMAL_COST               , "ability.spiderdroid.dridereq.primal_cost",        150) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_BODY_MAX                   , "ability.spiderdroid.implant.body_max",          6) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_COST_LEVEL0                , "ability.spiderdroid.implant.cost_level0",      12500) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_COST_LEVEL1                , "ability.spiderdroid.implant.cost_level1",      25000) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_COST_LEVEL2                , "ability.spiderdroid.implant.cost_level2",      50000) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_COST_LEVEL3                , "ability.spiderdroid.implant.cost_level3",     100000) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_COST_LEVEL4                , "ability.spiderdroid.implant.cost_level4",     200000) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_COST_LEVEL5                , "ability.spiderdroid.implant.cost_level5",     400000) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_FACE_MAX                   , "ability.spiderdroid.implant.face_max",          5) \
    CFG_X(ABILITY_SPIDERDROID_IMPLANT_LEGS_MAX                   , "ability.spiderdroid.implant.legs_max",          5) \
    CFG_X(ABILITY_SPIDERDROID_INFRAVISION_FACE_LEVEL_REQ         , "ability.spiderdroid.infravision.face_level_req",          1) \
    CFG_X(ABILITY_SPIDERDROID_STUNTUBES_BODY_LEVEL_REQ           , "ability.spiderdroid.stuntubes.body_level_req",          5) \
    CFG_X(ABILITY_SPIDERDROID_STUNTUBES_COOLDOWN                 , "ability.spiderdroid.stuntubes.cooldown",         12) \
    CFG_X(ABILITY_SPIDERDROID_STUNTUBES_LEGS_LEVEL_REQ           , "ability.spiderdroid.stuntubes.legs_level_req",          5) \
    CFG_X(ABILITY_SPIDERDROID_STUNTUBES_MOVE_COST                , "ability.spiderdroid.stuntubes.move_cost",       1000) \
    \
    /* =========== ABILITY - TANARRI =========== */ \
    CFG_X(ABILITY_TANARRI_BOOMING_COOLDOWN                       , "ability.tanarri.booming.cooldown",         12) \
    CFG_X(ABILITY_TANARRI_CHAOSGATE_MOVE_COST                    , "ability.tanarri.chaosgate.move_cost",       1000) \
    CFG_X(ABILITY_TANARRI_CHAOSSURGE_COOLDOWN                    , "ability.tanarri.chaossurge.cooldown",         12) \
    CFG_X(ABILITY_TANARRI_CHAOSSURGE_DAMAGE_GOOD                 , "ability.tanarri.chaossurge.damage_good",       1500) \
    CFG_X(ABILITY_TANARRI_CHAOSSURGE_DAMAGE_MILDEVIL             , "ability.tanarri.chaossurge.damage_mildevil",        500) \
    CFG_X(ABILITY_TANARRI_CHAOSSURGE_DAMAGE_NEUTRAL              , "ability.tanarri.chaossurge.damage_neutral",       1000) \
    CFG_X(ABILITY_TANARRI_EARTHQUAKE_COOLDOWN                    , "ability.tanarri.earthquake.cooldown",         12) \
    CFG_X(ABILITY_TANARRI_EARTHQUAKE_MANA_COST                   , "ability.tanarri.earthquake.mana_cost",       1000) \
    CFG_X(ABILITY_TANARRI_ENMITY_COOLDOWN                        , "ability.tanarri.enmity.cooldown",         24) \
    CFG_X(ABILITY_TANARRI_ENRAGE_COOLDOWN                        , "ability.tanarri.enrage.cooldown",         18) \
    CFG_X(ABILITY_TANARRI_FURY_DAMROLL_BONUS                     , "ability.tanarri.fury.damroll_bonus",        250) \
    CFG_X(ABILITY_TANARRI_FURY_HITROLL_BONUS                     , "ability.tanarri.fury.hitroll_bonus",        250) \
    CFG_X(ABILITY_TANARRI_INFERNAL_COOLDOWN                      , "ability.tanarri.infernal.cooldown",         12) \
    CFG_X(ABILITY_TANARRI_INFERNAL_MANA_COST                     , "ability.tanarri.infernal.mana_cost",       2000) \
    CFG_X(ABILITY_TANARRI_LAVABLAST_COOLDOWN                     , "ability.tanarri.lavablast.cooldown",         18) \
    CFG_X(ABILITY_TANARRI_LAVABLAST_MANA_COST                    , "ability.tanarri.lavablast.mana_cost",       1000) \
    CFG_X(ABILITY_TANARRI_LAVABLAST_MOVE_COST                    , "ability.tanarri.lavablast.move_cost",       1000) \
    CFG_X(ABILITY_TANARRI_TANEQ_PRIMAL_COST                      , "ability.tanarri.taneq.primal_cost",        150) \
    CFG_X(ABILITY_TANARRI_TORNADO_COOLDOWN                       , "ability.tanarri.tornado.cooldown",         12) \
    CFG_X(ABILITY_TANARRI_TORNADO_MANA_COST                      , "ability.tanarri.tornado.mana_cost",       1500) \
    \
    /* =========== ABILITY - UNDEAD_KNIGHT =========== */ \
    CFG_X(ABILITY_UNDEAD_KNIGHT_AURA_BOG_LEVEL_REQ               , "ability.undead_knight.aura_bog.level_req",          6) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_AURA_DEATH_LEVEL_REQ             , "ability.undead_knight.aura_death.level_req",          2) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_AURA_FEAR_LEVEL_REQ              , "ability.undead_knight.aura_fear.level_req",          9) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_AURA_MIGHT_DAMROLL_BONUS         , "ability.undead_knight.aura_might.damroll_bonus",        300) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_AURA_MIGHT_HITROLL_BONUS         , "ability.undead_knight.aura_might.hitroll_bonus",        300) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_AURA_MIGHT_LEVEL_REQ             , "ability.undead_knight.aura_might.level_req",          4) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_GAIN_INVOCATION_COST_MULTIPLIER  , "ability.undead_knight.gain_invocation.cost_multiplier",         60) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_GAIN_INVOCATION_MAX_LEVEL        , "ability.undead_knight.gain_invocation.max_level",          5) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_GAIN_NECROMANCY_COST_MULTIPLIER  , "ability.undead_knight.gain_necromancy.cost_multiplier",         60) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_GAIN_NECROMANCY_MAX_LEVEL        , "ability.undead_knight.gain_necromancy.max_level",         10) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_GAIN_SPIRIT_COST_MULTIPLIER      , "ability.undead_knight.gain_spirit.cost_multiplier",         60) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_GAIN_SPIRIT_MAX_LEVEL            , "ability.undead_knight.gain_spirit.max_level",         10) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_KNIGHTARMOR_PRIMAL_COST          , "ability.undead_knight.knightarmor.primal_cost",        150) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_BLIND_COOLDOWN         , "ability.undead_knight.powerword_blind.cooldown",         12) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_BLIND_DURATION         , "ability.undead_knight.powerword_blind.duration",         60) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_BLIND_LEVEL_REQ        , "ability.undead_knight.powerword_blind.level_req",          1) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_FLAMES_COOLDOWN        , "ability.undead_knight.powerword_flames.cooldown",         12) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_FLAMES_LEVEL_REQ       , "ability.undead_knight.powerword_flames.level_req",          4) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_KILL_LEVEL_REQ         , "ability.undead_knight.powerword_kill.level_req",          3) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_KILL_NPC_DAM_CAP       , "ability.undead_knight.powerword_kill.npc_dam_cap",       5000) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_KILL_PC_DAM_CAP        , "ability.undead_knight.powerword_kill.pc_dam_cap",       1500) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_STUN_CASTER_COOLDOWN   , "ability.undead_knight.powerword_stun.caster_cooldown",          8) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_STUN_LEVEL_REQ         , "ability.undead_knight.powerword_stun.level_req",          5) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_POWERWORD_STUN_VICTIM_COOLDOWN   , "ability.undead_knight.powerword_stun.victim_cooldown",         24) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_RIDE_MOVE_COST                   , "ability.undead_knight.ride.move_cost",        600) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_UNHOLYRITE_COOLDOWN              , "ability.undead_knight.unholyrite.cooldown",         18) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_UNHOLYRITE_HEAL_MAX              , "ability.undead_knight.unholyrite.heal_max",       1000) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_UNHOLYRITE_HEAL_MIN              , "ability.undead_knight.unholyrite.heal_min",        500) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_UNHOLYRITE_MANA_COST             , "ability.undead_knight.unholyrite.mana_cost",        500) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_WEAPONPRACTICE_COST_MULTIPLIER   , "ability.undead_knight.weaponpractice.cost_multiplier",         60) \
    CFG_X(ABILITY_UNDEAD_KNIGHT_WEAPONPRACTICE_MAX_LEVEL         , "ability.undead_knight.weaponpractice.max_level",         10) \
    \
    /* =========== ABILITY - VAMPIRE =========== */ \
    CFG_X(ABILITY_VAMPIRE_ASSASSINATE_BASE_CHANCE                , "ability.vampire.assassinate.base_chance",          5) \
    CFG_X(ABILITY_VAMPIRE_ASSASSINATE_COOLDOWN                   , "ability.vampire.assassinate.cooldown",         15) \
    CFG_X(ABILITY_VAMPIRE_ASSASSINATE_DAM_BONUS_MAX              , "ability.vampire.assassinate.dam_bonus_max",         20) \
    CFG_X(ABILITY_VAMPIRE_ASSASSINATE_DAM_BONUS_MIN              , "ability.vampire.assassinate.dam_bonus_min",          1) \
    CFG_X(ABILITY_VAMPIRE_ASSASSINATE_LEVEL_REQ                  , "ability.vampire.assassinate.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_ASTRALWALK_LEVEL_REQ                   , "ability.vampire.astralwalk.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_BAAL_COOLDOWN                          , "ability.vampire.baal.cooldown",         20) \
    CFG_X(ABILITY_VAMPIRE_BAAL_DISC_BONUS                        , "ability.vampire.baal.disc_bonus",          2) \
    CFG_X(ABILITY_VAMPIRE_BAAL_LEVEL_REQ                         , "ability.vampire.baal.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_BECKON_LEVEL_REQ                       , "ability.vampire.beckon.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_BECKON_MAX_FOLLOWERS                   , "ability.vampire.beckon.max_followers",          4) \
    CFG_X(ABILITY_VAMPIRE_BLOODAGONY_LEVEL_REQ                   , "ability.vampire.bloodagony.level_req",          3) \
    CFG_X(ABILITY_VAMPIRE_BLOODROD_LEVEL_REQ                     , "ability.vampire.bloodrod.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_BLOODROD_PRACTICE_COST                 , "ability.vampire.bloodrod.practice_cost",         10) \
    CFG_X(ABILITY_VAMPIRE_BLOODWALL_COOLDOWN                     , "ability.vampire.bloodwall.cooldown",         25) \
    CFG_X(ABILITY_VAMPIRE_BLOODWALL_LEVEL_REQ                    , "ability.vampire.bloodwall.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_BLOODWALL_MAX_BLOOD_INPUT              , "ability.vampire.bloodwall.max_blood_input",          5) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_CHAIN_CHANCE_DENOM          , "ability.vampire.bloodwater.chain_chance_denom",          4) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_CHAIN_VICTIM_MIN_HP         , "ability.vampire.bloodwater.chain_victim_min_hp",        100) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_COOLDOWN_NONVAMP            , "ability.vampire.bloodwater.cooldown_nonvamp",         12) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_COOLDOWN_VAMP               , "ability.vampire.bloodwater.cooldown_vamp",          6) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_DAMAGE_NPC                  , "ability.vampire.bloodwater.damage_npc",       2000) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_DAMAGE_PC_MAX               , "ability.vampire.bloodwater.damage_pc_max",        600) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_DAMAGE_PC_MIN               , "ability.vampire.bloodwater.damage_pc_min",        300) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_LEVEL_REQ                   , "ability.vampire.bloodwater.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_BLOODWATER_VAMP_BLOOD_DRAIN            , "ability.vampire.bloodwater.vamp_blood_drain",        150) \
    CFG_X(ABILITY_VAMPIRE_BONEMOD_LEVEL_REQ                      , "ability.vampire.bonemod.level_req",          3) \
    CFG_X(ABILITY_VAMPIRE_CAULDRON_COOLDOWN                      , "ability.vampire.cauldron.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_CAULDRON_LEVEL_REQ                     , "ability.vampire.cauldron.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_CAULDRON_MAX_BLOOD_INPUT               , "ability.vampire.cauldron.max_blood_input",        200) \
    CFG_X(ABILITY_VAMPIRE_CONCEAL_LEVEL_REQ                      , "ability.vampire.conceal.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_CONTROL_LEVEL_REQ                      , "ability.vampire.control.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_CSERPENT_LEVEL_REQ                     , "ability.vampire.cserpent.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_CSERPENT_MAX_FOLLOWERS                 , "ability.vampire.cserpent.max_followers",          5) \
    CFG_X(ABILITY_VAMPIRE_DIABLERISE_COOLDOWN                    , "ability.vampire.diablerise.cooldown",         15) \
    CFG_X(ABILITY_VAMPIRE_DRAGONFORM_BLOOD_COST_CHECK            , "ability.vampire.dragonform.blood_cost_check",        100) \
    CFG_X(ABILITY_VAMPIRE_DRAGONFORM_BLOOD_COST_MAX              , "ability.vampire.dragonform.blood_cost_max",        400) \
    CFG_X(ABILITY_VAMPIRE_DRAGONFORM_BLOOD_COST_MIN              , "ability.vampire.dragonform.blood_cost_min",        200) \
    CFG_X(ABILITY_VAMPIRE_DRAGONFORM_DAMROLL_BONUS               , "ability.vampire.dragonform.damroll_bonus",        100) \
    CFG_X(ABILITY_VAMPIRE_DRAGONFORM_HITROLL_BONUS               , "ability.vampire.dragonform.hitroll_bonus",        100) \
    CFG_X(ABILITY_VAMPIRE_DRAGONFORM_LEVEL_REQ                   , "ability.vampire.dragonform.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_COOLDOWN                         , "ability.vampire.drain.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_LEVEL_REQ                        , "ability.vampire.drain.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_MAX_OVERHEAL                     , "ability.vampire.drain.max_overheal",       1000) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_NPC_HEAL_MAX                     , "ability.vampire.drain.npc_heal_max",        550) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_NPC_HEAL_MIN                     , "ability.vampire.drain.npc_heal_min",        450) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_PC_HEAL_MAX                      , "ability.vampire.drain.pc_heal_max",        450) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_PC_HEAL_MIN                      , "ability.vampire.drain.pc_heal_min",        350) \
    CFG_X(ABILITY_VAMPIRE_DRAIN_VICTIM_MIN_HP                    , "ability.vampire.drain.victim_min_hp",        500) \
    CFG_X(ABILITY_VAMPIRE_DUB_LEVEL_REQ                          , "ability.vampire.dub.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_EMBRACE_COOLDOWN                       , "ability.vampire.embrace.cooldown",         15) \
    CFG_X(ABILITY_VAMPIRE_ENTRANCE_COOLDOWN                      , "ability.vampire.entrance.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_ENTRANCE_LEVEL_REQ                     , "ability.vampire.entrance.level_req",          3) \
    CFG_X(ABILITY_VAMPIRE_FACADE_LEVEL_REQ                       , "ability.vampire.facade.level_req",          6) \
    CFG_X(ABILITY_VAMPIRE_FEAR_COOLDOWN                          , "ability.vampire.fear.cooldown",         16) \
    CFG_X(ABILITY_VAMPIRE_FEAR_LEVEL_REQ                         , "ability.vampire.fear.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_FEAR_PC_FAIL_RANGE                     , "ability.vampire.fear.pc_fail_range",          4) \
    CFG_X(ABILITY_VAMPIRE_FEAR_PC_LEVEL_REQ                      , "ability.vampire.fear.pc_level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_FLAMEHANDS_LEVEL_REQ                   , "ability.vampire.flamehands.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_FLESHCRAFT_BLOOD_COST_CHECK            , "ability.vampire.fleshcraft.blood_cost_check",         40) \
    CFG_X(ABILITY_VAMPIRE_FLESHCRAFT_BLOOD_COST_MAX              , "ability.vampire.fleshcraft.blood_cost_max",         40) \
    CFG_X(ABILITY_VAMPIRE_FLESHCRAFT_BLOOD_COST_MIN              , "ability.vampire.fleshcraft.blood_cost_min",         30) \
    CFG_X(ABILITY_VAMPIRE_FLESHCRAFT_LEVEL_REQ                   , "ability.vampire.fleshcraft.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_FORMILLUSION_BLOOD_COST                , "ability.vampire.formillusion.blood_cost",         30) \
    CFG_X(ABILITY_VAMPIRE_FORMILLUSION_CLONE_LEVEL               , "ability.vampire.formillusion.clone_level",        200) \
    CFG_X(ABILITY_VAMPIRE_FORMILLUSION_LEVEL_REQ                 , "ability.vampire.formillusion.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_FORMILLUSION_MAX_FOLLOWERS             , "ability.vampire.formillusion.max_followers",          4) \
    CFG_X(ABILITY_VAMPIRE_FRENZY_COOLDOWN                        , "ability.vampire.frenzy.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_FRENZY_LEVEL_REQ                       , "ability.vampire.frenzy.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_FRENZY_RAGE_GAIN_MAX                   , "ability.vampire.frenzy.rage_gain_max",         30) \
    CFG_X(ABILITY_VAMPIRE_FRENZY_RAGE_GAIN_MIN                   , "ability.vampire.frenzy.rage_gain_min",         20) \
    CFG_X(ABILITY_VAMPIRE_GATE_LEVEL_REQ                         , "ability.vampire.gate.level_req",          3) \
    CFG_X(ABILITY_VAMPIRE_GATE_PORTAL_TIMER                      , "ability.vampire.gate.portal_timer",          5) \
    CFG_X(ABILITY_VAMPIRE_GOURGE_BLOOD_GAIN_MAX                  , "ability.vampire.gourge.blood_gain_max",        200) \
    CFG_X(ABILITY_VAMPIRE_GOURGE_BLOOD_GAIN_MIN                  , "ability.vampire.gourge.blood_gain_min",        100) \
    CFG_X(ABILITY_VAMPIRE_GOURGE_COOLDOWN                        , "ability.vampire.gourge.cooldown",         15) \
    CFG_X(ABILITY_VAMPIRE_GOURGE_LEVEL_REQ                       , "ability.vampire.gourge.level_req",          8) \
    CFG_X(ABILITY_VAMPIRE_GOURGE_MOB_LEVEL_MAX                   , "ability.vampire.gourge.mob_level_max",         30) \
    CFG_X(ABILITY_VAMPIRE_GRAB_COOLDOWN                          , "ability.vampire.grab.cooldown",          8) \
    CFG_X(ABILITY_VAMPIRE_GRAB_LEVEL_REQ                         , "ability.vampire.grab.level_req",          8) \
    CFG_X(ABILITY_VAMPIRE_GUARDIAN_CHARM_DURATION                , "ability.vampire.guardian.charm_duration",        666) \
    CFG_X(ABILITY_VAMPIRE_GUARDIAN_LEVEL_REQ                     , "ability.vampire.guardian.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_GUARDIAN_MAX_FOLLOWERS                 , "ability.vampire.guardian.max_followers",          5) \
    CFG_X(ABILITY_VAMPIRE_GUARDIAN_MOB_ARMOR                     , "ability.vampire.guardian.mob_armor",        300) \
    CFG_X(ABILITY_VAMPIRE_GUARDIAN_MOB_DAMROLL                   , "ability.vampire.guardian.mob_damroll",         50) \
    CFG_X(ABILITY_VAMPIRE_GUARDIAN_MOB_HITROLL                   , "ability.vampire.guardian.mob_hitroll",         50) \
    CFG_X(ABILITY_VAMPIRE_GUARDIAN_MOB_HP                        , "ability.vampire.guardian.mob_hp",       5000) \
    CFG_X(ABILITY_VAMPIRE_HAGSWRINKLES_BLOOD_COST_CHECK          , "ability.vampire.hagswrinkles.blood_cost_check",         40) \
    CFG_X(ABILITY_VAMPIRE_HAGSWRINKLES_BLOOD_COST_MAX            , "ability.vampire.hagswrinkles.blood_cost_max",         40) \
    CFG_X(ABILITY_VAMPIRE_HAGSWRINKLES_BLOOD_COST_MIN            , "ability.vampire.hagswrinkles.blood_cost_min",         30) \
    CFG_X(ABILITY_VAMPIRE_HAGSWRINKLES_LEVEL_REQ                 , "ability.vampire.hagswrinkles.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_ILLUSION_LEVEL_REQ                     , "ability.vampire.illusion.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_ILLUSION_MAX_FOLLOWERS                 , "ability.vampire.illusion.max_followers",          5) \
    CFG_X(ABILITY_VAMPIRE_INFERNO_BLOOD_COST                     , "ability.vampire.inferno.blood_cost",        100) \
    CFG_X(ABILITY_VAMPIRE_INFERNO_LEVEL_REQ                      , "ability.vampire.inferno.level_req",          6) \
    CFG_X(ABILITY_VAMPIRE_INFIRMITY_COOLDOWN                     , "ability.vampire.infirmity.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_INFIRMITY_LEVEL_REQ                    , "ability.vampire.infirmity.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_LAMPREY_BLOOD_GAIN_MAX                 , "ability.vampire.lamprey.blood_gain_max",         50) \
    CFG_X(ABILITY_VAMPIRE_LAMPREY_BLOOD_GAIN_MIN                 , "ability.vampire.lamprey.blood_gain_min",         40) \
    CFG_X(ABILITY_VAMPIRE_LAMPREY_COOLDOWN                       , "ability.vampire.lamprey.cooldown",          5) \
    CFG_X(ABILITY_VAMPIRE_LAMPREY_DAM_BONUS_MAX                  , "ability.vampire.lamprey.dam_bonus_max",         30) \
    CFG_X(ABILITY_VAMPIRE_LAMPREY_DAM_BONUS_MIN                  , "ability.vampire.lamprey.dam_bonus_min",          1) \
    CFG_X(ABILITY_VAMPIRE_LAMPREY_LEVEL_REQ                      , "ability.vampire.lamprey.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_MINDBLAST_COOLDOWN                     , "ability.vampire.mindblast.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_MINDBLAST_LEVEL_REQ                    , "ability.vampire.mindblast.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_MINDBLAST_SUCCESS_CHANCE               , "ability.vampire.mindblast.success_chance",         50) \
    CFG_X(ABILITY_VAMPIRE_MIRROR_BLOOD_COST                      , "ability.vampire.mirror.blood_cost",         20) \
    CFG_X(ABILITY_VAMPIRE_MIRROR_CLONE_HP                        , "ability.vampire.mirror.clone_hp",       2000) \
    CFG_X(ABILITY_VAMPIRE_MIRROR_LEVEL_REQ                       , "ability.vampire.mirror.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_MIRROR_MAX_FOLLOWERS                   , "ability.vampire.mirror.max_followers",          4) \
    CFG_X(ABILITY_VAMPIRE_OBJ_LEVEL_REQ                          , "ability.vampire.obj.level_req",         10) \
    CFG_X(ABILITY_VAMPIRE_OBJMASK_BLOOD_COST_CHECK               , "ability.vampire.objmask.blood_cost_check",         50) \
    CFG_X(ABILITY_VAMPIRE_OBJMASK_BLOOD_COST_MAX                 , "ability.vampire.objmask.blood_cost_max",         50) \
    CFG_X(ABILITY_VAMPIRE_OBJMASK_BLOOD_COST_MIN                 , "ability.vampire.objmask.blood_cost_min",         40) \
    CFG_X(ABILITY_VAMPIRE_OBJMASK_LEVEL_REQ                      , "ability.vampire.objmask.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_PIGEON_LEVEL_REQ                       , "ability.vampire.pigeon.level_req",          3) \
    CFG_X(ABILITY_VAMPIRE_PRESERVE_LEVEL_REQ                     , "ability.vampire.preserve.level_req",          2) \
    CFG_X(ABILITY_VAMPIRE_PURIFICATION_COOLDOWN_BASE             , "ability.vampire.purification.cooldown_base",         30) \
    CFG_X(ABILITY_VAMPIRE_PURIFICATION_LEVEL_REQ                 , "ability.vampire.purification.level_req",          7) \
    CFG_X(ABILITY_VAMPIRE_PURIFICATION_MOVE_COST                 , "ability.vampire.purification.move_cost",       5000) \
    CFG_X(ABILITY_VAMPIRE_SCALES_ARMOR_BONUS                     , "ability.vampire.scales.armor_bonus",        100) \
    CFG_X(ABILITY_VAMPIRE_SCALES_LEVEL_REQ                       , "ability.vampire.scales.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_SCREAM_BLOOD_COST_CHECK                , "ability.vampire.scream.blood_cost_check",         50) \
    CFG_X(ABILITY_VAMPIRE_SCREAM_LEVEL_REQ                       , "ability.vampire.scream.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_SERVANT_CHARM_DURATION                 , "ability.vampire.servant.charm_duration",        666) \
    CFG_X(ABILITY_VAMPIRE_SERVANT_LEVEL_REQ                      , "ability.vampire.servant.level_req",          8) \
    CFG_X(ABILITY_VAMPIRE_SERVANT_MAX_FOLLOWERS                  , "ability.vampire.servant.max_followers",          5) \
    CFG_X(ABILITY_VAMPIRE_SERVANT_MOB_ARMOR                      , "ability.vampire.servant.mob_armor",        700) \
    CFG_X(ABILITY_VAMPIRE_SERVANT_MOB_DAMROLL                    , "ability.vampire.servant.mob_damroll",        100) \
    CFG_X(ABILITY_VAMPIRE_SERVANT_MOB_HITROLL                    , "ability.vampire.servant.mob_hitroll",        100) \
    CFG_X(ABILITY_VAMPIRE_SERVANT_MOB_HP                         , "ability.vampire.servant.mob_hp",      30000) \
    CFG_X(ABILITY_VAMPIRE_SHADOWGAZE_COOLDOWN                    , "ability.vampire.shadowgaze.cooldown",          8) \
    CFG_X(ABILITY_VAMPIRE_SHADOWGAZE_LEVEL_REQ                   , "ability.vampire.shadowgaze.level_req",         10) \
    CFG_X(ABILITY_VAMPIRE_SHARE_BLOOD_COST                       , "ability.vampire.share.blood_cost",         25) \
    CFG_X(ABILITY_VAMPIRE_SHARE_LEVEL_REQ                        , "ability.vampire.share.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_SHARPEN_LEVEL_REQ                      , "ability.vampire.sharpen.level_req",          7) \
    CFG_X(ABILITY_VAMPIRE_SHROUD_LEVEL_REQ                       , "ability.vampire.shroud.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_SPEW_BLOOD_COST_CHECK                  , "ability.vampire.spew.blood_cost_check",         20) \
    CFG_X(ABILITY_VAMPIRE_SPEW_BLOOD_COST_MAX                    , "ability.vampire.spew.blood_cost_max",         22) \
    CFG_X(ABILITY_VAMPIRE_SPEW_BLOOD_COST_MIN                    , "ability.vampire.spew.blood_cost_min",         18) \
    CFG_X(ABILITY_VAMPIRE_SPEW_COOLDOWN                          , "ability.vampire.spew.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_SPEW_LEVEL_REQ                         , "ability.vampire.spew.level_req",          6) \
    CFG_X(ABILITY_VAMPIRE_SPIRITGATE_BLOOD_COST                  , "ability.vampire.spiritgate.blood_cost",         65) \
    CFG_X(ABILITY_VAMPIRE_SPIRITGATE_BLOOD_COST_CHECK            , "ability.vampire.spiritgate.blood_cost_check",         66) \
    CFG_X(ABILITY_VAMPIRE_SPIRITGATE_LEVEL_REQ                   , "ability.vampire.spiritgate.level_req",          3) \
    CFG_X(ABILITY_VAMPIRE_SPIRITGUARD_LEVEL_REQ                  , "ability.vampire.spiritguard.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_SPIT_BLOOD_COST                        , "ability.vampire.spit.blood_cost",          5) \
    CFG_X(ABILITY_VAMPIRE_SPIT_COOLDOWN                          , "ability.vampire.spit.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_SPIT_DAM_BONUS_MAX                     , "ability.vampire.spit.dam_bonus_max",         30) \
    CFG_X(ABILITY_VAMPIRE_SPIT_DAM_BONUS_MIN                     , "ability.vampire.spit.dam_bonus_min",          1) \
    CFG_X(ABILITY_VAMPIRE_SPIT_LEVEL_REQ                         , "ability.vampire.spit.level_req",          1) \
    CFG_X(ABILITY_VAMPIRE_SUMMON_COOLDOWN                        , "ability.vampire.summon.cooldown",         10) \
    CFG_X(ABILITY_VAMPIRE_SUMMON_LEVEL_REQ                       , "ability.vampire.summon.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_SUN_DAMAGE_MAX                         , "ability.vampire.sun.damage_max",          2) \
    CFG_X(ABILITY_VAMPIRE_SUN_DAMAGE_MIN                         , "ability.vampire.sun.damage_min",          1) \
    CFG_X(ABILITY_VAMPIRE_SUN_DAMAGE_SERPENT                     , "ability.vampire.sun.damage_serpent",          1) \
    CFG_X(ABILITY_VAMPIRE_TENDRILS_COOLDOWN                      , "ability.vampire.tendrils.cooldown",         12) \
    CFG_X(ABILITY_VAMPIRE_TENDRILS_LEVEL_REQ                     , "ability.vampire.tendrils.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_TESTEMB_COOLDOWN                       , "ability.vampire.testemb.cooldown",         15) \
    CFG_X(ABILITY_VAMPIRE_TONGUE_COOLDOWN                        , "ability.vampire.tongue.cooldown",          5) \
    CFG_X(ABILITY_VAMPIRE_TONGUE_DAM_BONUS_MAX                   , "ability.vampire.tongue.dam_bonus_max",         30) \
    CFG_X(ABILITY_VAMPIRE_TONGUE_DAM_BONUS_MIN                   , "ability.vampire.tongue.dam_bonus_min",          1) \
    CFG_X(ABILITY_VAMPIRE_TONGUE_LEVEL_REQ                       , "ability.vampire.tongue.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_UNVEIL_LEVEL_REQ                       , "ability.vampire.unveil.level_req",          3) \
    CFG_X(ABILITY_VAMPIRE_VAMPDARKNESS_BLOOD_COST                , "ability.vampire.vampdarkness.blood_cost",        300) \
    CFG_X(ABILITY_VAMPIRE_VAMPDARKNESS_LEVEL_REQ                 , "ability.vampire.vampdarkness.level_req",          6) \
    CFG_X(ABILITY_VAMPIRE_VAMPIREARMOR_PRACTICE_COST             , "ability.vampire.vampirearmor.practice_cost",         60) \
    CFG_X(ABILITY_VAMPIRE_VTWIST_LEVEL_REQ                       , "ability.vampire.vtwist.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_WALL_BLOOD_COST                        , "ability.vampire.wall.blood_cost",         50) \
    CFG_X(ABILITY_VAMPIRE_WALL_COOLDOWN                          , "ability.vampire.wall.cooldown",         25) \
    CFG_X(ABILITY_VAMPIRE_WALL_LEVEL_REQ                         , "ability.vampire.wall.level_req",          7) \
    CFG_X(ABILITY_VAMPIRE_WALL_TIMER                             , "ability.vampire.wall.timer",          3) \
    CFG_X(ABILITY_VAMPIRE_WITHERING_COOLDOWN                     , "ability.vampire.withering.cooldown",         35) \
    CFG_X(ABILITY_VAMPIRE_WITHERING_DISMEMBER_CHANCE             , "ability.vampire.withering.dismember_chance",         15) \
    CFG_X(ABILITY_VAMPIRE_WITHERING_LEVEL_REQ                    , "ability.vampire.withering.level_req",          4) \
    CFG_X(ABILITY_VAMPIRE_ZOMBIE_CHARM_DURATION                  , "ability.vampire.zombie.charm_duration",        666) \
    CFG_X(ABILITY_VAMPIRE_ZOMBIE_COOLDOWN                        , "ability.vampire.zombie.cooldown",         10) \
    CFG_X(ABILITY_VAMPIRE_ZOMBIE_LEVEL_REQ                       , "ability.vampire.zombie.level_req",          5) \
    CFG_X(ABILITY_VAMPIRE_ZOMBIE_MAX_FOLLOWERS                   , "ability.vampire.zombie.max_followers",          5) \
    \
    /* =========== ABILITY - WEREWOLF =========== */ \
    CFG_X(ABILITY_WEREWOLF_BURROW_LEVEL_REQ                      , "ability.werewolf.burrow.level_req",          6) \
    CFG_X(ABILITY_WEREWOLF_COCOON_LEVEL_REQ                      , "ability.werewolf.cocoon.level_req",          8) \
    CFG_X(ABILITY_WEREWOLF_DEVOUR_HEAL_MAX                       , "ability.werewolf.devour.heal_max",        250) \
    CFG_X(ABILITY_WEREWOLF_DEVOUR_HEAL_MIN                       , "ability.werewolf.devour.heal_min",        100) \
    CFG_X(ABILITY_WEREWOLF_DEVOUR_LEVEL_REQ                      , "ability.werewolf.devour.level_req",          5) \
    CFG_X(ABILITY_WEREWOLF_DISQUIET_COOLDOWN                     , "ability.werewolf.disquiet.cooldown",          4) \
    CFG_X(ABILITY_WEREWOLF_DISQUIET_LEVEL_REQ                    , "ability.werewolf.disquiet.level_req",          6) \
    CFG_X(ABILITY_WEREWOLF_FLAMECLAWS_LEVEL_REQ                  , "ability.werewolf.flameclaws.level_req",          1) \
    CFG_X(ABILITY_WEREWOLF_GMOTHERSTOUCH_COOLDOWN_COMBAT         , "ability.werewolf.gmotherstouch.cooldown_combat",         16) \
    CFG_X(ABILITY_WEREWOLF_GMOTHERSTOUCH_COOLDOWN_NONCOMBAT      , "ability.werewolf.gmotherstouch.cooldown_noncombat",          6) \
    CFG_X(ABILITY_WEREWOLF_GMOTHERSTOUCH_HEAL_COMBAT             , "ability.werewolf.gmotherstouch.heal_combat",        200) \
    CFG_X(ABILITY_WEREWOLF_GMOTHERSTOUCH_HEAL_NONCOMBAT          , "ability.werewolf.gmotherstouch.heal_noncombat",        600) \
    CFG_X(ABILITY_WEREWOLF_GMOTHERSTOUCH_LEVEL_REQ               , "ability.werewolf.gmotherstouch.level_req",          4) \
    CFG_X(ABILITY_WEREWOLF_GMOTHERSTOUCH_MANA_COST_COMBAT        , "ability.werewolf.gmotherstouch.mana_cost_combat",        100) \
    CFG_X(ABILITY_WEREWOLF_GMOTHERSTOUCH_MANA_COST_NONCOMBAT     , "ability.werewolf.gmotherstouch.mana_cost_noncombat",        300) \
    CFG_X(ABILITY_WEREWOLF_JAWLOCK_LEVEL_REQ                     , "ability.werewolf.jawlock.level_req",          8) \
    CFG_X(ABILITY_WEREWOLF_KLAIVE_PRIMAL_COST                    , "ability.werewolf.klaive.primal_cost",         60) \
    CFG_X(ABILITY_WEREWOLF_MOONARMOUR_LEVEL_REQ                  , "ability.werewolf.moonarmour.level_req",          2) \
    CFG_X(ABILITY_WEREWOLF_MOONARMOUR_PRIMAL_COST                , "ability.werewolf.moonarmour.primal_cost",         60) \
    CFG_X(ABILITY_WEREWOLF_MOONBEAM_COOLDOWN                     , "ability.werewolf.moonbeam.cooldown",         12) \
    CFG_X(ABILITY_WEREWOLF_MOONBEAM_DAMAGE_EVIL                  , "ability.werewolf.moonbeam.damage_evil",       1000) \
    CFG_X(ABILITY_WEREWOLF_MOONBEAM_DAMAGE_GOOD                  , "ability.werewolf.moonbeam.damage_good",        500) \
    CFG_X(ABILITY_WEREWOLF_MOONBEAM_DAMAGE_NEUTRAL               , "ability.werewolf.moonbeam.damage_neutral",        750) \
    CFG_X(ABILITY_WEREWOLF_MOONBEAM_LEVEL_REQ                    , "ability.werewolf.moonbeam.level_req",          8) \
    CFG_X(ABILITY_WEREWOLF_MOONBEAM_MANA_COST                    , "ability.werewolf.moonbeam.mana_cost",        500) \
    CFG_X(ABILITY_WEREWOLF_MOONGATE_GATE_TIMER                   , "ability.werewolf.moongate.gate_timer",          5) \
    CFG_X(ABILITY_WEREWOLF_MOONGATE_LEVEL_REQ                    , "ability.werewolf.moongate.level_req",          6) \
    CFG_X(ABILITY_WEREWOLF_MOTHERSTOUCH_COOLDOWN_COMBAT          , "ability.werewolf.motherstouch.cooldown_combat",         16) \
    CFG_X(ABILITY_WEREWOLF_MOTHERSTOUCH_COOLDOWN_NONCOMBAT       , "ability.werewolf.motherstouch.cooldown_noncombat",          8) \
    CFG_X(ABILITY_WEREWOLF_MOTHERSTOUCH_HEAL_COMBAT              , "ability.werewolf.motherstouch.heal_combat",        100) \
    CFG_X(ABILITY_WEREWOLF_MOTHERSTOUCH_HEAL_NONCOMBAT           , "ability.werewolf.motherstouch.heal_noncombat",        500) \
    CFG_X(ABILITY_WEREWOLF_MOTHERSTOUCH_LEVEL_REQ                , "ability.werewolf.motherstouch.level_req",          3) \
    CFG_X(ABILITY_WEREWOLF_MOTHERSTOUCH_MANA_COST_COMBAT         , "ability.werewolf.motherstouch.mana_cost_combat",         50) \
    CFG_X(ABILITY_WEREWOLF_MOTHERSTOUCH_MANA_COST_NONCOMBAT      , "ability.werewolf.motherstouch.mana_cost_noncombat",        250) \
    CFG_X(ABILITY_WEREWOLF_PERCEPTION_LEVEL_REQ                  , "ability.werewolf.perception.level_req",          3) \
    CFG_X(ABILITY_WEREWOLF_QUILLS_LEVEL_REQ                      , "ability.werewolf.quills.level_req",          5) \
    CFG_X(ABILITY_WEREWOLF_RAZORCLAWS_LEVEL_REQ                  , "ability.werewolf.razorclaws.level_req",          4) \
    CFG_X(ABILITY_WEREWOLF_REND_LEVEL_REQ                        , "ability.werewolf.rend.level_req",          7) \
    CFG_X(ABILITY_WEREWOLF_RESHAPE_LEVEL_REQ                     , "ability.werewolf.reshape.level_req",          7) \
    CFG_X(ABILITY_WEREWOLF_ROAR_COOLDOWN_FAIL                    , "ability.werewolf.roar.cooldown_fail",         12) \
    CFG_X(ABILITY_WEREWOLF_ROAR_COOLDOWN_SUCCESS                 , "ability.werewolf.roar.cooldown_success",         18) \
    CFG_X(ABILITY_WEREWOLF_ROAR_LEVEL_REQ                        , "ability.werewolf.roar.level_req",          6) \
    CFG_X(ABILITY_WEREWOLF_SCLAWS_LEVEL_REQ                      , "ability.werewolf.sclaws.level_req",          5) \
    CFG_X(ABILITY_WEREWOLF_SHRED_BASE_HITS                       , "ability.werewolf.shred.base_hits",          2) \
    CFG_X(ABILITY_WEREWOLF_SHRED_COOLDOWN                        , "ability.werewolf.shred.cooldown",         24) \
    CFG_X(ABILITY_WEREWOLF_SHRED_LEVEL_REQ                       , "ability.werewolf.shred.level_req",          7) \
    CFG_X(ABILITY_WEREWOLF_SKIN_ARMOR_BONUS                      , "ability.werewolf.skin.armor_bonus",        100) \
    CFG_X(ABILITY_WEREWOLF_SKIN_LEVEL_REQ                        , "ability.werewolf.skin.level_req",          7) \
    CFG_X(ABILITY_WEREWOLF_SLAM_LEVEL_REQ                        , "ability.werewolf.slam.level_req",          8) \
    CFG_X(ABILITY_WEREWOLF_STAREDOWN_COOLDOWN                    , "ability.werewolf.staredown.cooldown",         16) \
    CFG_X(ABILITY_WEREWOLF_STAREDOWN_LEVEL_REQ                   , "ability.werewolf.staredown.level_req",          5) \
    CFG_X(ABILITY_WEREWOLF_TALONS_COOLDOWN                       , "ability.werewolf.talons.cooldown",         12) \
    CFG_X(ABILITY_WEREWOLF_TALONS_DAMAGE_NPC_MAX                 , "ability.werewolf.talons.damage_npc_max",       4000) \
    CFG_X(ABILITY_WEREWOLF_TALONS_DAMAGE_NPC_MIN                 , "ability.werewolf.talons.damage_npc_min",       2000) \
    CFG_X(ABILITY_WEREWOLF_TALONS_DAMAGE_PC_MAX                  , "ability.werewolf.talons.damage_pc_max",        600) \
    CFG_X(ABILITY_WEREWOLF_TALONS_DAMAGE_PC_MIN                  , "ability.werewolf.talons.damage_pc_min",        400) \
    CFG_X(ABILITY_WEREWOLF_TALONS_LEVEL_REQ                      , "ability.werewolf.talons.level_req",         10) \
    CFG_X(ABILITY_WEREWOLF_WITHER_COOLDOWN                       , "ability.werewolf.wither.cooldown",         18) \
    CFG_X(ABILITY_WEREWOLF_WITHER_LEVEL_REQ                      , "ability.werewolf.wither.level_req",          7) \
    CFG_X(ABILITY_WEREWOLF_WITHER_PC_CHANCE                      , "ability.werewolf.wither.pc_chance",         15) \
    \
    /* =========== ABILITY - WYRM =========== */ \
    CFG_X(ABILITY_WYRM_ANCIENTWRATH_DAMAGE_BONUS                 , "ability.wyrm.ancientwrath.damage_bonus",         25) \
    CFG_X(ABILITY_WYRM_ANCIENTWRATH_DURATION                     , "ability.wyrm.ancientwrath.duration",         10) \
    CFG_X(ABILITY_WYRM_ANCIENTWRATH_ESSENCE_COST                 , "ability.wyrm.ancientwrath.essence_cost",         25) \
    CFG_X(ABILITY_WYRM_ANNIHILATE_BASE_DAMAGE                    , "ability.wyrm.annihilate.base_damage",       1200) \
    CFG_X(ABILITY_WYRM_ANNIHILATE_ESSENCE_COST                   , "ability.wyrm.annihilate.essence_cost",         35) \
    CFG_X(ABILITY_WYRM_ANNIHILATE_EXECUTE_BONUS                  , "ability.wyrm.annihilate.execute_bonus",         50) \
    CFG_X(ABILITY_WYRM_ANNIHILATE_EXECUTE_THRESHOLD              , "ability.wyrm.annihilate.execute_threshold",         25) \
    CFG_X(ABILITY_WYRM_APOCALYPSE_BASE_DAMAGE                    , "ability.wyrm.apocalypse.base_damage",       2000) \
    CFG_X(ABILITY_WYRM_APOCALYPSE_COOLDOWN                       , "ability.wyrm.apocalypse.cooldown",         30) \
    CFG_X(ABILITY_WYRM_APOCALYPSE_ESSENCE_COST                   , "ability.wyrm.apocalypse.essence_cost",         60) \
    CFG_X(ABILITY_WYRM_CATACLYSM_BASE_DAMAGE                     , "ability.wyrm.cataclysm.base_damage",        600) \
    CFG_X(ABILITY_WYRM_CATACLYSM_ESSENCE_COST                    , "ability.wyrm.cataclysm.essence_cost",         40) \
    CFG_X(ABILITY_WYRM_CATACLYSM_FIELD_DAMAGE                    , "ability.wyrm.cataclysm.field_damage",        200) \
    CFG_X(ABILITY_WYRM_CATACLYSM_FIELD_DURATION                  , "ability.wyrm.cataclysm.field_duration",          3) \
    CFG_X(ABILITY_WYRM_DAMAGE_REDUCTION                          , "ability.wyrm.damage_reduction",         30) \
    CFG_X(ABILITY_WYRM_DAMCAP_BASE                               , "ability.wyrm.damcap.base",        300) \
    CFG_X(ABILITY_WYRM_DAMCAP_ESSENCE_MULT                       , "ability.wyrm.damcap.essence_mult",          7) \
    CFG_X(ABILITY_WYRM_DRAGONFEAR_DURATION                       , "ability.wyrm.dragonfear.duration",          3) \
    CFG_X(ABILITY_WYRM_DRAGONFEAR_ESSENCE_COST                   , "ability.wyrm.dragonfear.essence_cost",         20) \
    CFG_X(ABILITY_WYRM_DRAGONFEAR_STUN_CHANCE                    , "ability.wyrm.dragonfear.stun_chance",         30) \
    CFG_X(ABILITY_WYRM_DRAGONLORD_DURATION                       , "ability.wyrm.dragonlord.duration",         30) \
    CFG_X(ABILITY_WYRM_DRAGONLORD_ESSENCE_COST                   , "ability.wyrm.dragonlord.essence_cost",         25) \
    CFG_X(ABILITY_WYRM_PRIMORDIAL_DAMAGE_MULT                    , "ability.wyrm.primordial.damage_mult",         10) \
    CFG_X(ABILITY_WYRM_PRIMORDIAL_ESSENCE_REGEN                  , "ability.wyrm.primordial.essence_regen",          5) \
    CFG_X(ABILITY_WYRM_TERRAINSHIFT_DAMAGE_BONUS                 , "ability.wyrm.terrainshift.damage_bonus",         15) \
    CFG_X(ABILITY_WYRM_TERRAINSHIFT_DURATION                     , "ability.wyrm.terrainshift.duration",         10) \
    CFG_X(ABILITY_WYRM_TERRAINSHIFT_ESSENCE_COST                 , "ability.wyrm.terrainshift.essence_cost",         30) \
    CFG_X(ABILITY_WYRM_WYRMARMOR_PRACTICE_COST                   , "ability.wyrm.wyrmarmor.practice_cost",         60) \
    CFG_X(ABILITY_WYRM_WYRMBREATH_BASE_DAMAGE                    , "ability.wyrm.wyrmbreath.base_damage",        800) \
    CFG_X(ABILITY_WYRM_WYRMBREATH_ESSENCE_COST                   , "ability.wyrm.wyrmbreath.essence_cost",         25) \
    CFG_X(ABILITY_WYRM_WYRMBREATH_ESSENCE_MULT                   , "ability.wyrm.wyrmbreath.essence_mult",         12) \
    CFG_X(ABILITY_WYRM_WYRMFORM_DAMCAP_BONUS                     , "ability.wyrm.wyrmform.damcap_bonus",        300) \
    CFG_X(ABILITY_WYRM_WYRMFORM_DURATION                         , "ability.wyrm.wyrmform.duration",         20) \
    CFG_X(ABILITY_WYRM_WYRMFORM_ESSENCE_COST                     , "ability.wyrm.wyrmform.essence_cost",         40) \
    CFG_X(ABILITY_WYRM_WYRMFORM_EXTRA_ATTACKS                    , "ability.wyrm.wyrmform.extra_attacks",          2) \
    /* ABILITY - ARTIFICER - CHARGE */ \
    CFG_X(ABILITY_ARTIFICER_CHARGE_MANA_COST                    , "ability.artificer.charge.mana_cost",            30) \
    CFG_X(ABILITY_ARTIFICER_CHARGE_COOLDOWN                     , "ability.artificer.charge.cooldown",              4) \
    CFG_X(ABILITY_ARTIFICER_CHARGE_POWER_GAIN                   , "ability.artificer.charge.power_gain",           15) \
    /* ABILITY - ARTIFICER - OVERCHARGE */ \
    CFG_X(ABILITY_ARTIFICER_OVERCHARGE_MANA_COST                , "ability.artificer.overcharge.mana_cost",        80) \
    CFG_X(ABILITY_ARTIFICER_OVERCHARGE_POWER_COST               , "ability.artificer.overcharge.power_cost",       80) \
    CFG_X(ABILITY_ARTIFICER_OVERCHARGE_DURATION                 , "ability.artificer.overcharge.duration",          8) \
    /* ABILITY - ARTIFICER - ENERGYBLADE */ \
    CFG_X(ABILITY_ARTIFICER_ENERGYBLADE_MANA_COST               , "ability.artificer.energyblade.mana_cost",       40) \
    CFG_X(ABILITY_ARTIFICER_ENERGYBLADE_POWER_COST              , "ability.artificer.energyblade.power_cost",      15) \
    CFG_X(ABILITY_ARTIFICER_ENERGYBLADE_DURATION                , "ability.artificer.energyblade.duration",        10) \
    /* ABILITY - ARTIFICER - BLASTER */ \
    CFG_X(ABILITY_ARTIFICER_BLASTER_MANA_COST                   , "ability.artificer.blaster.mana_cost",           30) \
    CFG_X(ABILITY_ARTIFICER_BLASTER_POWER_COST                  , "ability.artificer.blaster.power_cost",          10) \
    CFG_X(ABILITY_ARTIFICER_BLASTER_COOLDOWN                    , "ability.artificer.blaster.cooldown",             4) \
    CFG_X(ABILITY_ARTIFICER_BLASTER_DAM_MIN                     , "ability.artificer.blaster.dam_min",            100) \
    CFG_X(ABILITY_ARTIFICER_BLASTER_DAM_MAX                     , "ability.artificer.blaster.dam_max",            200) \
    /* ABILITY - ARTIFICER - GRENADE */ \
    CFG_X(ABILITY_ARTIFICER_GRENADE_MANA_COST                   , "ability.artificer.grenade.mana_cost",           50) \
    CFG_X(ABILITY_ARTIFICER_GRENADE_POWER_COST                  , "ability.artificer.grenade.power_cost",          20) \
    CFG_X(ABILITY_ARTIFICER_GRENADE_COOLDOWN                    , "ability.artificer.grenade.cooldown",             6) \
    CFG_X(ABILITY_ARTIFICER_GRENADE_DAM_MIN                     , "ability.artificer.grenade.dam_min",            150) \
    CFG_X(ABILITY_ARTIFICER_GRENADE_DAM_MAX                     , "ability.artificer.grenade.dam_max",            300) \
    /* ABILITY - ARTIFICER - FORCEFIELD */ \
    CFG_X(ABILITY_ARTIFICER_FORCEFIELD_MANA_COST                , "ability.artificer.forcefield.mana_cost",        50) \
    CFG_X(ABILITY_ARTIFICER_FORCEFIELD_POWER_COST               , "ability.artificer.forcefield.power_cost",       20) \
    CFG_X(ABILITY_ARTIFICER_FORCEFIELD_DURATION                 , "ability.artificer.forcefield.duration",          8) \
    CFG_X(ABILITY_ARTIFICER_FORCEFIELD_ABSORB                   , "ability.artificer.forcefield.absorb",          500) \
    /* ABILITY - ARTIFICER - REPAIRBOT */ \
    CFG_X(ABILITY_ARTIFICER_REPAIRBOT_MANA_COST                 , "ability.artificer.repairbot.mana_cost",         40) \
    CFG_X(ABILITY_ARTIFICER_REPAIRBOT_POWER_COST                , "ability.artificer.repairbot.power_cost",        15) \
    CFG_X(ABILITY_ARTIFICER_REPAIRBOT_DURATION                  , "ability.artificer.repairbot.duration",          10) \
    CFG_X(ABILITY_ARTIFICER_REPAIRBOT_HEAL_PER_TICK             , "ability.artificer.repairbot.heal_per_tick",    100) \
    /* ABILITY - ARTIFICER - TECHCLOAK */ \
    CFG_X(ABILITY_ARTIFICER_TECHCLOAK_MANA_COST                 , "ability.artificer.techcloak.mana_cost",         60) \
    CFG_X(ABILITY_ARTIFICER_TECHCLOAK_POWER_COST                , "ability.artificer.techcloak.power_cost",        25) \
    CFG_X(ABILITY_ARTIFICER_TECHCLOAK_DURATION                  , "ability.artificer.techcloak.duration",          12) \
    CFG_X(ABILITY_ARTIFICER_TECHCLOAK_POWER_DRAIN               , "ability.artificer.techcloak.power_drain",        3) \
    /* ABILITY - ARTIFICER - TURRET */ \
    CFG_X(ABILITY_ARTIFICER_TURRET_MANA_COST                    , "ability.artificer.turret.mana_cost",            60) \
    CFG_X(ABILITY_ARTIFICER_TURRET_POWER_COST                   , "ability.artificer.turret.power_cost",           20) \
    CFG_X(ABILITY_ARTIFICER_TURRET_DURATION                     , "ability.artificer.turret.duration",             12) \
    CFG_X(ABILITY_ARTIFICER_TURRET_HP                           , "ability.artificer.turret.hp",                  500) \
    CFG_X(ABILITY_ARTIFICER_TURRET_DAM_MIN                      , "ability.artificer.turret.dam_min",              50) \
    CFG_X(ABILITY_ARTIFICER_TURRET_DAM_MAX                      , "ability.artificer.turret.dam_max",              80) \
    /* ABILITY - ARTIFICER - DECOY */ \
    CFG_X(ABILITY_ARTIFICER_DECOY_MANA_COST                     , "ability.artificer.decoy.mana_cost",             50) \
    CFG_X(ABILITY_ARTIFICER_DECOY_POWER_COST                    , "ability.artificer.decoy.power_cost",            15) \
    CFG_X(ABILITY_ARTIFICER_DECOY_DURATION                      , "ability.artificer.decoy.duration",               6) \
    CFG_X(ABILITY_ARTIFICER_DECOY_HP                            , "ability.artificer.decoy.hp",                   200) \
    CFG_X(ABILITY_ARTIFICER_DECOY_REDIRECT_PCT                  , "ability.artificer.decoy.redirect_pct",          40) \
    /* ABILITY - ARTIFICER - GRAPPLE */ \
    CFG_X(ABILITY_ARTIFICER_GRAPPLE_MANA_COST                   , "ability.artificer.grapple.mana_cost",           40) \
    CFG_X(ABILITY_ARTIFICER_GRAPPLE_POWER_COST                  , "ability.artificer.grapple.power_cost",          10) \
    CFG_X(ABILITY_ARTIFICER_GRAPPLE_COOLDOWN                    , "ability.artificer.grapple.cooldown",             6) \
    CFG_X(ABILITY_ARTIFICER_GRAPPLE_DAM_MIN                     , "ability.artificer.grapple.dam_min",             50) \
    CFG_X(ABILITY_ARTIFICER_GRAPPLE_DAM_MAX                     , "ability.artificer.grapple.dam_max",            100) \
    CFG_X(ABILITY_ARTIFICER_GRAPPLE_DISARM_PCT                  , "ability.artificer.grapple.disarm_pct",          20) \
    CFG_X(ABILITY_ARTIFICER_GRAPPLE_RANGE                       , "ability.artificer.grapple.range",                5) \
    /* DAMCAP - ARTIFICER */ \
    CFG_X(COMBAT_DAMCAP_ARTIFICER_BASE                          , "combat.damcap.artificer.base",                 150) \
    CFG_X(COMBAT_DAMCAP_ARTIFICER_POWER_MULT                    , "combat.damcap.artificer.power_mult",             5) \
    CFG_X(COMBAT_DAMCAP_ARTIFICER_BLADE                         , "combat.damcap.artificer.blade",                150) \
    CFG_X(COMBAT_DAMCAP_ARTIFICER_OVERCHARGE                    , "combat.damcap.artificer.overcharge",            100) \
    /* DAMCAP - MECHANIST */ \
    CFG_X(COMBAT_DAMCAP_MECHANIST_POWER_MULT                    , "combat.damcap.mechanist.power_mult",             7) \
    CFG_X(COMBAT_DAMCAP_MECHANIST_SERVO                         , "combat.damcap.mechanist.servo",                200) \
    CFG_X(COMBAT_DAMCAP_MECHANIST_ARMY                          , "combat.damcap.mechanist.army",                 150) \
    CFG_X(COMBAT_DAMCAP_MECHANIST_PER_DRONE                     , "combat.damcap.mechanist.per_drone",             30) \
    /* ABILITY - MECHANIST - NEURALJACK */ \
    CFG_X(ABILITY_MECHANIST_NEURALJACK_MANA_COST                , "ability.mechanist.neuraljack.mana_cost",        80) \
    CFG_X(ABILITY_MECHANIST_NEURALJACK_POWER_COST               , "ability.mechanist.neuraljack.power_cost",       30) \
    CFG_X(ABILITY_MECHANIST_NEURALJACK_DURATION                 , "ability.mechanist.neuraljack.duration",         12) \
    CFG_X(ABILITY_MECHANIST_NEURALJACK_DODGE_PCT                , "ability.mechanist.neuraljack.dodge_pct",        20) \
    /* ABILITY - MECHANIST - SERVOARMS */ \
    CFG_X(ABILITY_MECHANIST_SERVOARMS_MANA_COST                 , "ability.mechanist.servoarms.mana_cost",         80) \
    CFG_X(ABILITY_MECHANIST_SERVOARMS_POWER_COST                , "ability.mechanist.servoarms.power_cost",        30) \
    CFG_X(ABILITY_MECHANIST_SERVOARMS_DURATION                  , "ability.mechanist.servoarms.duration",          12) \
    CFG_X(ABILITY_MECHANIST_SERVOARMS_DAM_BONUS                 , "ability.mechanist.servoarms.dam_bonus",         50) \
    /* ABILITY - MECHANIST - REACTIVEPLATING */ \
    CFG_X(ABILITY_MECHANIST_REACTIVE_MANA_COST                  , "ability.mechanist.reactive.mana_cost",          80) \
    CFG_X(ABILITY_MECHANIST_REACTIVE_POWER_COST                 , "ability.mechanist.reactive.power_cost",         30) \
    CFG_X(ABILITY_MECHANIST_REACTIVE_DURATION                   , "ability.mechanist.reactive.duration",           12) \
    CFG_X(ABILITY_MECHANIST_REACTIVE_RESIST_PCT                 , "ability.mechanist.reactive.resist_pct",         15) \
    CFG_X(ABILITY_MECHANIST_REACTIVE_REFLECT_PCT                , "ability.mechanist.reactive.reflect_pct",        10) \
    /* ABILITY - MECHANIST - RAILGUN */ \
    CFG_X(ABILITY_MECHANIST_RAILGUN_MANA_COST                   , "ability.mechanist.railgun.mana_cost",          100) \
    CFG_X(ABILITY_MECHANIST_RAILGUN_POWER_REQ                   , "ability.mechanist.railgun.power_req",           50) \
    CFG_X(ABILITY_MECHANIST_RAILGUN_POWER_COST                  , "ability.mechanist.railgun.power_cost",          40) \
    CFG_X(ABILITY_MECHANIST_RAILGUN_COOLDOWN                    , "ability.mechanist.railgun.cooldown",             8) \
    CFG_X(ABILITY_MECHANIST_RAILGUN_DAM_MIN                     , "ability.mechanist.railgun.dam_min",            400) \
    CFG_X(ABILITY_MECHANIST_RAILGUN_DAM_MAX                     , "ability.mechanist.railgun.dam_max",            600) \
    /* ABILITY - MECHANIST - EMPBURST */ \
    CFG_X(ABILITY_MECHANIST_EMPBURST_MANA_COST                  , "ability.mechanist.empburst.mana_cost",         120) \
    CFG_X(ABILITY_MECHANIST_EMPBURST_POWER_COST                 , "ability.mechanist.empburst.power_cost",         50) \
    CFG_X(ABILITY_MECHANIST_EMPBURST_COOLDOWN                   , "ability.mechanist.empburst.cooldown",           15) \
    CFG_X(ABILITY_MECHANIST_EMPBURST_DAM_MIN                    , "ability.mechanist.empburst.dam_min",           200) \
    CFG_X(ABILITY_MECHANIST_EMPBURST_DAM_MAX                    , "ability.mechanist.empburst.dam_max",           350) \
    /* ABILITY - MECHANIST - COMBATDRONE */ \
    CFG_X(ABILITY_MECHANIST_COMBATDRONE_MANA_COST              , "ability.mechanist.combatdrone.mana_cost",       70) \
    CFG_X(ABILITY_MECHANIST_COMBATDRONE_POWER_COST             , "ability.mechanist.combatdrone.power_cost",      25) \
    CFG_X(ABILITY_MECHANIST_COMBATDRONE_HP                     , "ability.mechanist.combatdrone.hp",             400) \
    CFG_X(ABILITY_MECHANIST_COMBATDRONE_DAM_MIN                , "ability.mechanist.combatdrone.dam_min",         60) \
    CFG_X(ABILITY_MECHANIST_COMBATDRONE_DAM_MAX                , "ability.mechanist.combatdrone.dam_max",        100) \
    /* ABILITY - MECHANIST - REPAIRSWARM */ \
    CFG_X(ABILITY_MECHANIST_REPAIRSWARM_MANA_COST              , "ability.mechanist.repairswarm.mana_cost",       90) \
    CFG_X(ABILITY_MECHANIST_REPAIRSWARM_POWER_COST             , "ability.mechanist.repairswarm.power_cost",      35) \
    CFG_X(ABILITY_MECHANIST_REPAIRSWARM_DURATION               , "ability.mechanist.repairswarm.duration",        10) \
    CFG_X(ABILITY_MECHANIST_REPAIRSWARM_HEAL                   , "ability.mechanist.repairswarm.heal",           100) \
    CFG_X(ABILITY_MECHANIST_REPAIRSWARM_DRONE_HEAL             , "ability.mechanist.repairswarm.drone_heal",      50) \
    /* ABILITY - MECHANIST - BOMBERDRONE */ \
    CFG_X(ABILITY_MECHANIST_BOMBERDRONE_MANA_COST              , "ability.mechanist.bomberdrone.mana_cost",      100) \
    CFG_X(ABILITY_MECHANIST_BOMBERDRONE_POWER_COST             , "ability.mechanist.bomberdrone.power_cost",      40) \
    CFG_X(ABILITY_MECHANIST_BOMBERDRONE_HP                     , "ability.mechanist.bomberdrone.hp",             200) \
    CFG_X(ABILITY_MECHANIST_BOMBERDRONE_DAM_MIN                , "ability.mechanist.bomberdrone.dam_min",        300) \
    CFG_X(ABILITY_MECHANIST_BOMBERDRONE_DAM_MAX                , "ability.mechanist.bomberdrone.dam_max",        500) \
    /* ABILITY - MECHANIST - DRONEARMY */ \
    CFG_X(ABILITY_MECHANIST_DRONEARMY_MANA_COST               , "ability.mechanist.dronearmy.mana_cost",       150) \
    CFG_X(ABILITY_MECHANIST_DRONEARMY_POWER_REQ                , "ability.mechanist.dronearmy.power_req",       100) \
    CFG_X(ABILITY_MECHANIST_DRONEARMY_POWER_COST               , "ability.mechanist.dronearmy.power_cost",       80) \
    CFG_X(ABILITY_MECHANIST_DRONEARMY_COOLDOWN                 , "ability.mechanist.dronearmy.cooldown",         30) \
    CFG_X(ABILITY_MECHANIST_DRONEARMY_DURATION                 , "ability.mechanist.dronearmy.duration",         10) \
    CFG_X(ABILITY_MECHANIST_DRONEARMY_DAM_BONUS_PCT            , "ability.mechanist.dronearmy.dam_bonus_pct",    50) \
    CFG_X(ABILITY_MECHANIST_DRONEARMY_HP_BONUS                 , "ability.mechanist.dronearmy.hp_bonus",        200) \
    /* ABILITY - MECHANIST - ORBITAL STRIKE */ \
    CFG_X(ABILITY_MECHANIST_ORBITAL_MANA_COST                  , "ability.mechanist.orbital.mana_cost",         200) \
    CFG_X(ABILITY_MECHANIST_ORBITAL_POWER_REQ                  , "ability.mechanist.orbital.power_req",         120) \
    CFG_X(ABILITY_MECHANIST_ORBITAL_POWER_COST                 , "ability.mechanist.orbital.power_cost",        100) \
    CFG_X(ABILITY_MECHANIST_ORBITAL_COOLDOWN                   , "ability.mechanist.orbital.cooldown",           45) \
    CFG_X(ABILITY_MECHANIST_ORBITAL_DAM_MIN                    , "ability.mechanist.orbital.dam_min",           600) \
    CFG_X(ABILITY_MECHANIST_ORBITAL_DAM_MAX                    , "ability.mechanist.orbital.dam_max",          1000) \
    CFG_X(ABILITY_MECHANIST_ORBITAL_AOE_PCT                    , "ability.mechanist.orbital.aoe_pct",            50) \
    /* ABILITY - MECHANIST - IMPLANT SWAP */ \
    CFG_X(ABILITY_MECHANIST_IMPLANT_SWAP_CD                    , "ability.mechanist.implant.swap_cd",            10) \
    /* ABILITY - MECHANIST - NEURAL IMPLANT PASSIVES */ \
    CFG_X(ABILITY_MECHANIST_IMPLANT_NEURAL_COMBAT_DODGE        , "ability.mechanist.implant.neural.combat_dodge",       15) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_NEURAL_TARGETING_HITROLL   , "ability.mechanist.implant.neural.targeting_hitroll",   20) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_NEURAL_TARGETING_DAM_PCT   , "ability.mechanist.implant.neural.targeting_dam_pct",   20) \
    /* ABILITY - MECHANIST - SERVO IMPLANT PASSIVES */ \
    CFG_X(ABILITY_MECHANIST_IMPLANT_SERVO_POWER_DAMROLL        , "ability.mechanist.implant.servo.power_damroll",        30) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_SERVO_POWER_DAM            , "ability.mechanist.implant.servo.power_dam",            50) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_SERVO_SHIELD_DAMCAP        , "ability.mechanist.implant.servo.shield_damcap",       200) \
    /* ABILITY - MECHANIST - CORE IMPLANT PASSIVES */ \
    CFG_X(ABILITY_MECHANIST_IMPLANT_CORE_ARMORED_AC            , "ability.mechanist.implant.core.armored_ac",            50) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_CORE_ARMORED_RESIST        , "ability.mechanist.implant.core.armored_resist",        10) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_CORE_REGEN_HP              , "ability.mechanist.implant.core.regen_hp",             100) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_CORE_POWER_MAX             , "ability.mechanist.implant.core.power_max",             25) \
    CFG_X(ABILITY_MECHANIST_IMPLANT_CORE_POWER_TICK            , "ability.mechanist.implant.core.power_tick",             1) \
    /* ABILITY - ARTIFICER - ARMOR */ \
    CFG_X(ABILITY_ARTIFICER_ARTIFICERARMOR_PRACTICE_COST      , "ability.artificer.artificerarmor.practice_cost",        60) \
    /* ABILITY - MECHANIST - ARMOR */ \
    CFG_X(ABILITY_MECHANIST_MECHANISTARMOR_PRACTICE_COST      , "ability.mechanist.mechanistarmor.practice_cost",        60) \
    /* COMBAT - CULTIST - DAMCAP */ \
    CFG_X(COMBAT_DAMCAP_CULTIST_BASE                           , "combat.damcap.cultist.base",                         150) \
    CFG_X(COMBAT_DAMCAP_CULTIST_CORRUPT_MULT                   , "combat.damcap.cultist.corrupt_mult",                   5) \
    CFG_X(COMBAT_DAMCAP_CULTIST_HIGH_CORRUPT                   , "combat.damcap.cultist.high_corrupt",                 150) \
    /* COMBAT - VOIDBORN - DAMCAP */ \
    CFG_X(COMBAT_DAMCAP_VOIDBORN_CORRUPT_MULT                  , "combat.damcap.voidborn.corrupt_mult",                  7) \
    CFG_X(COMBAT_DAMCAP_VOIDBORN_FINALFORM                     , "combat.damcap.voidborn.finalform",                   300) \
    CFG_X(COMBAT_DAMCAP_VOIDBORN_ABERRANT                      , "combat.damcap.voidborn.aberrant",                    150) \
    /* ABILITY - CULTIST - GENERAL */ \
    CFG_X(CULTIST_TRAIN_COST_MULT                              , "ability.cultist.train.cost_mult",                     40) \
    CFG_X(CULTIST_PURGE_HP_COST                                , "ability.cultist.purge.hp_cost",                        5) \
    CFG_X(CULTIST_PURGE_CORRUPTION_REMOVED                     , "ability.cultist.purge.corruption_removed",            25) \
    /* ABILITY - CULTIST - FORBIDDEN LORE */ \
    CFG_X(CULTIST_ELDRITCHSIGHT_MANA_COST                      , "ability.cultist.eldritchsight.mana_cost",             30) \
    CFG_X(CULTIST_ELDRITCHSIGHT_CORRUPT_GAIN                   , "ability.cultist.eldritchsight.corrupt_gain",           3) \
    CFG_X(CULTIST_ELDRITCHSIGHT_DURATION                       , "ability.cultist.eldritchsight.duration",              10) \
    CFG_X(CULTIST_WHISPERS_MANA_COST                           , "ability.cultist.whispers.mana_cost",                  50) \
    CFG_X(CULTIST_WHISPERS_CORRUPT_GAIN                        , "ability.cultist.whispers.corrupt_gain",                5) \
    CFG_X(CULTIST_WHISPERS_DEBUFF_DURATION                     , "ability.cultist.whispers.debuff_duration",             8) \
    CFG_X(CULTIST_UNRAVEL_MANA_COST                            , "ability.cultist.unravel.mana_cost",                   80) \
    CFG_X(CULTIST_UNRAVEL_CORRUPT_GAIN                         , "ability.cultist.unravel.corrupt_gain",                10) \
    CFG_X(CULTIST_UNRAVEL_MAX_REMOVES                          , "ability.cultist.unravel.max_removes",                  3) \
    CFG_X(CULTIST_UNRAVEL_MAX_REMOVES_HIGH                     , "ability.cultist.unravel.max_removes_high",             5) \
    /* ABILITY - CULTIST - TENTACLE ARTS */ \
    CFG_X(CULTIST_VOIDTENDRIL_MANA_COST                        , "ability.cultist.voidtendril.mana_cost",               40) \
    CFG_X(CULTIST_VOIDTENDRIL_CORRUPT_GAIN                     , "ability.cultist.voidtendril.corrupt_gain",             5) \
    CFG_X(CULTIST_GRASP_MANA_COST                              , "ability.cultist.grasp.mana_cost",                     60) \
    CFG_X(CULTIST_GRASP_CORRUPT_GAIN                           , "ability.cultist.grasp.corrupt_gain",                   8) \
    CFG_X(CULTIST_GRASP_DURATION                               , "ability.cultist.grasp.duration",                       4) \
    CFG_X(CULTIST_GRASP_DURATION_HIGH                          , "ability.cultist.grasp.duration_high",                  6) \
    CFG_X(CULTIST_GRASP_TICK_DAMAGE                            , "ability.cultist.grasp.tick_damage",                   30) \
    CFG_X(CULTIST_CONSTRICT_MANA_COST                          , "ability.cultist.constrict.mana_cost",                 80) \
    CFG_X(CULTIST_CONSTRICT_CORRUPT_GAIN                       , "ability.cultist.constrict.corrupt_gain",              12) \
    CFG_X(CULTIST_CONSTRICT_DURATION                           , "ability.cultist.constrict.duration",                   5) \
    /* ABILITY - CULTIST - MADNESS */ \
    CFG_X(CULTIST_MADDENINGGAZE_MANA_COST                      , "ability.cultist.maddeninggaze.mana_cost",             50) \
    CFG_X(CULTIST_MADDENINGGAZE_CORRUPT_GAIN                   , "ability.cultist.maddeninggaze.corrupt_gain",           7) \
    CFG_X(CULTIST_GIBBERING_MANA_COST                          , "ability.cultist.gibbering.mana_cost",                 90) \
    CFG_X(CULTIST_GIBBERING_CORRUPT_GAIN                       , "ability.cultist.gibbering.corrupt_gain",              15) \
    CFG_X(CULTIST_GIBBERING_CONFUSION_DURATION                 , "ability.cultist.gibbering.confusion_duration",         3) \
    CFG_X(CULTIST_GIBBERING_FLEE_CHANCE                        , "ability.cultist.gibbering.flee_chance",               30) \
    CFG_X(CULTIST_INSANITY_MANA_COST                           , "ability.cultist.insanity.mana_cost",                 120) \
    CFG_X(CULTIST_INSANITY_CORRUPT_GAIN                        , "ability.cultist.insanity.corrupt_gain",               20) \
    CFG_X(CULTIST_INSANITY_SELFATTACK_CHANCE                   , "ability.cultist.insanity.selfattack_chance",           20) \
    /* ABILITY - CULTIST - ARMOR */ \
    CFG_X(CULTIST_CULTISTARMOR_PRACTICE_COST                   , "ability.cultist.cultistarmor.practice_cost",           60) \
    /* ABILITY - VOIDBORN - GENERAL */ \
    CFG_X(VOIDBORN_TRAIN_COST_MULT                             , "ability.voidborn.train.cost_mult",                    50) \
    /* ABILITY - VOIDBORN - REALITY WARP */ \
    CFG_X(VOIDBORN_PHASESHIFT_MANA_COST                        , "ability.voidborn.phaseshift.mana_cost",               70) \
    CFG_X(VOIDBORN_PHASESHIFT_CORRUPT_GAIN                     , "ability.voidborn.phaseshift.corrupt_gain",            10) \
    CFG_X(VOIDBORN_PHASESHIFT_DURATION                         , "ability.voidborn.phaseshift.duration",                 8) \
    CFG_X(VOIDBORN_PHASESHIFT_DODGE_BONUS                      , "ability.voidborn.phaseshift.dodge_bonus",             25) \
    CFG_X(VOIDBORN_DIMREND_MANA_COST                           , "ability.voidborn.dimrend.mana_cost",                 100) \
    CFG_X(VOIDBORN_DIMREND_CORRUPT_GAIN                        , "ability.voidborn.dimrend.corrupt_gain",               15) \
    CFG_X(VOIDBORN_DIMREND_COOLDOWN                            , "ability.voidborn.dimrend.cooldown",                   10) \
    CFG_X(VOIDBORN_DIMREND_HAZARD_DURATION                     , "ability.voidborn.dimrend.hazard_duration",             5) \
    CFG_X(VOIDBORN_DIMREND_HAZARD_DAMAGE                       , "ability.voidborn.dimrend.hazard_damage",              50) \
    CFG_X(VOIDBORN_UNMAKE_MANA_COST                            , "ability.voidborn.unmake.mana_cost",                  180) \
    CFG_X(VOIDBORN_UNMAKE_CORRUPT_REQ                          , "ability.voidborn.unmake.corrupt_req",                100) \
    CFG_X(VOIDBORN_UNMAKE_CORRUPT_COST                         , "ability.voidborn.unmake.corrupt_cost",                50) \
    CFG_X(VOIDBORN_UNMAKE_COOLDOWN                             , "ability.voidborn.unmake.cooldown",                    25) \
    CFG_X(VOIDBORN_UNMAKE_INSTAKILL_THRESHOLD                  , "ability.voidborn.unmake.instakill_threshold",       1000) \
    /* ABILITY - VOIDBORN - ELDER FORM */ \
    CFG_X(VOIDBORN_VOIDSHAPE_MANA_COST                         , "ability.voidborn.voidshape.mana_cost",                80) \
    CFG_X(VOIDBORN_VOIDSHAPE_CORRUPT_GAIN                      , "ability.voidborn.voidshape.corrupt_gain",             12) \
    CFG_X(VOIDBORN_VOIDSHAPE_DURATION                          , "ability.voidborn.voidshape.duration",                 10) \
    CFG_X(VOIDBORN_VOIDSHAPE_EXTRA_ATTACKS                     , "ability.voidborn.voidshape.extra_attacks",             2) \
    CFG_X(VOIDBORN_ABERRANT_MANA_COST                          , "ability.voidborn.aberrant.mana_cost",                100) \
    CFG_X(VOIDBORN_ABERRANT_CORRUPT_GAIN                       , "ability.voidborn.aberrant.corrupt_gain",              18) \
    CFG_X(VOIDBORN_ABERRANT_DURATION                           , "ability.voidborn.aberrant.duration",                   8) \
    CFG_X(VOIDBORN_ABERRANT_DAMAGE_BONUS                       , "ability.voidborn.aberrant.damage_bonus",              25) \
    CFG_X(VOIDBORN_ABERRANT_RESISTANCE                         , "ability.voidborn.aberrant.resistance",                15) \
    CFG_X(VOIDBORN_FINALFORM_MANA_COST                         , "ability.voidborn.finalform.mana_cost",               150) \
    CFG_X(VOIDBORN_FINALFORM_CORRUPT_REQ                       , "ability.voidborn.finalform.corrupt_req",              75) \
    CFG_X(VOIDBORN_FINALFORM_CORRUPT_GAIN                      , "ability.voidborn.finalform.corrupt_gain",             30) \
    CFG_X(VOIDBORN_FINALFORM_DURATION                          , "ability.voidborn.finalform.duration",                  6) \
    CFG_X(VOIDBORN_FINALFORM_COOLDOWN                          , "ability.voidborn.finalform.cooldown",                 30) \
    /* ABILITY - VOIDBORN - COSMIC HORROR */ \
    CFG_X(VOIDBORN_SUMMONTHING_MANA_COST                       , "ability.voidborn.summonthing.mana_cost",             120) \
    CFG_X(VOIDBORN_SUMMONTHING_CORRUPT_GAIN                    , "ability.voidborn.summonthing.corrupt_gain",           20) \
    CFG_X(VOIDBORN_SUMMONTHING_DURATION                        , "ability.voidborn.summonthing.duration",               15) \
    CFG_X(VOIDBORN_STARSPAWN_MANA_COST                         , "ability.voidborn.starspawn.mana_cost",               140) \
    CFG_X(VOIDBORN_STARSPAWN_CORRUPT_GAIN                      , "ability.voidborn.starspawn.corrupt_gain",             25) \
    CFG_X(VOIDBORN_STARSPAWN_COOLDOWN                          , "ability.voidborn.starspawn.cooldown",                 12) \
    CFG_X(VOIDBORN_STARSPAWN_AOE_PCT                           , "ability.voidborn.starspawn.aoe_pct",                  50) \
    CFG_X(VOIDBORN_ENTROPY_MANA_COST                           , "ability.voidborn.entropy.mana_cost",                 200) \
    CFG_X(VOIDBORN_ENTROPY_CORRUPT_REQ                         , "ability.voidborn.entropy.corrupt_req",               125) \
    CFG_X(VOIDBORN_ENTROPY_CORRUPT_COST                        , "ability.voidborn.entropy.corrupt_cost",               75) \
    CFG_X(VOIDBORN_ENTROPY_COOLDOWN                            , "ability.voidborn.entropy.cooldown",                   30) \
    CFG_X(VOIDBORN_ENTROPY_DURATION                            , "ability.voidborn.entropy.duration",                    5) \
    /* ABILITY - VOIDBORN - ARMOR */ \
    CFG_X(VOIDBORN_VOIDBORNARMOR_PRACTICE_COST                 , "ability.voidborn.voidbornarmor.practice_cost",         60) \
    /* COMBAT - CHRONOMANCER - DAMCAP */ \
    CFG_X(COMBAT_DAMCAP_CHRONO_BASE                            , "combat.damcap.chrono.base",                          150) \
    CFG_X(COMBAT_DAMCAP_CHRONO_EXTREME_MULT                    , "combat.damcap.chrono.extreme_mult",                    4) \
    CFG_X(COMBAT_DAMCAP_CHRONO_QUICKEN                         , "combat.damcap.chrono.quicken",                       100) \
    CFG_X(COMBAT_DAMCAP_CHRONO_BLUR                            , "combat.damcap.chrono.blur",                          150) \
    /* ABILITY - CHRONOMANCER - GENERAL */ \
    CFG_X(CHRONO_TRAIN_COST_MULT                               , "ability.chrono.train.cost_mult",                      40) \
    /* ABILITY - CHRONOMANCER - ACCELERATION */ \
    CFG_X(CHRONO_QUICKEN_MANA_COST                             , "ability.chrono.quicken.mana_cost",                    60) \
    CFG_X(CHRONO_QUICKEN_FLUX_CHANGE                           , "ability.chrono.quicken.flux_change",                  15) \
    CFG_X(CHRONO_QUICKEN_DURATION                              , "ability.chrono.quicken.duration",                      8) \
    CFG_X(CHRONO_QUICKEN_EXTRA_ATTACKS                         , "ability.chrono.quicken.extra_attacks",                 2) \
    CFG_X(CHRONO_QUICKEN_EXTRA_ATTACKS_BONUS                   , "ability.chrono.quicken.extra_attacks_bonus",           3) \
    CFG_X(CHRONO_TIMESLIP_MANA_COST                            , "ability.chrono.timeslip.mana_cost",                   50) \
    CFG_X(CHRONO_TIMESLIP_FLUX_CHANGE                          , "ability.chrono.timeslip.flux_change",                 10) \
    CFG_X(CHRONO_TIMESLIP_COOLDOWN                             , "ability.chrono.timeslip.cooldown",                     6) \
    CFG_X(CHRONO_BLUR_MANA_COST                                , "ability.chrono.blur.mana_cost",                      100) \
    CFG_X(CHRONO_BLUR_FLUX_CHANGE                              , "ability.chrono.blur.flux_change",                     25) \
    CFG_X(CHRONO_BLUR_DURATION                                 , "ability.chrono.blur.duration",                         5) \
    CFG_X(CHRONO_BLUR_COOLDOWN                                 , "ability.chrono.blur.cooldown",                        15) \
    CFG_X(CHRONO_BLUR_EXTRA_ATTACKS                            , "ability.chrono.blur.extra_attacks",                    4) \
    CFG_X(CHRONO_BLUR_DODGE_BONUS                              , "ability.chrono.blur.dodge_bonus",                     40) \
    CFG_X(CHRONO_BLUR_MISS_CHANCE                              , "ability.chrono.blur.miss_chance",                     25) \
    /* ABILITY - CHRONOMANCER - DECELERATION */ \
    CFG_X(CHRONO_SLOW_MANA_COST                                , "ability.chrono.slow.mana_cost",                       50) \
    CFG_X(CHRONO_SLOW_FLUX_CHANGE                              , "ability.chrono.slow.flux_change",                     12) \
    CFG_X(CHRONO_SLOW_DURATION                                 , "ability.chrono.slow.duration",                         8) \
    CFG_X(CHRONO_SLOW_DURATION_BONUS                           , "ability.chrono.slow.duration_bonus",                  12) \
    CFG_X(CHRONO_SLOW_ATTACKS_LOST                             , "ability.chrono.slow.attacks_lost",                     2) \
    CFG_X(CHRONO_SLOW_DODGE_REDUCTION                          , "ability.chrono.slow.dodge_reduction",                 20) \
    CFG_X(CHRONO_TIMETRAP_MANA_COST                            , "ability.chrono.timetrap.mana_cost",                   80) \
    CFG_X(CHRONO_TIMETRAP_FLUX_CHANGE                          , "ability.chrono.timetrap.flux_change",                 18) \
    CFG_X(CHRONO_TIMETRAP_DURATION                             , "ability.chrono.timetrap.duration",                     6) \
    CFG_X(CHRONO_TIMETRAP_COOLDOWN                             , "ability.chrono.timetrap.cooldown",                    12) \
    CFG_X(CHRONO_STASIS_MANA_COST                              , "ability.chrono.stasis.mana_cost",                    120) \
    CFG_X(CHRONO_STASIS_FLUX_CHANGE                            , "ability.chrono.stasis.flux_change",                   25) \
    CFG_X(CHRONO_STASIS_DURATION                               , "ability.chrono.stasis.duration",                       3) \
    CFG_X(CHRONO_STASIS_COOLDOWN                               , "ability.chrono.stasis.cooldown",                      20) \
    CFG_X(CHRONO_STASIS_MAX_STORED                             , "ability.chrono.stasis.max_stored",                  2000) \
    /* ABILITY - CHRONOMANCER - TEMPORAL SIGHT */ \
    CFG_X(CHRONO_FORESIGHT_MANA_COST                           , "ability.chrono.foresight.mana_cost",                  40) \
    CFG_X(CHRONO_FORESIGHT_DURATION                            , "ability.chrono.foresight.duration",                   10) \
    CFG_X(CHRONO_FORESIGHT_DODGE_BONUS                         , "ability.chrono.foresight.dodge_bonus",                20) \
    CFG_X(CHRONO_HINDSIGHT_MANA_COST                           , "ability.chrono.hindsight.mana_cost",                  50) \
    CFG_X(CHRONO_HINDSIGHT_DURATION                            , "ability.chrono.hindsight.duration",                    8) \
    CFG_X(CHRONO_HINDSIGHT_STACK_BONUS                         , "ability.chrono.hindsight.stack_bonus",                 5) \
    CFG_X(CHRONO_HINDSIGHT_MAX_STACKS                          , "ability.chrono.hindsight.max_stacks",                  5) \
    CFG_X(CHRONO_ECHO_MANA_COST                                , "ability.chrono.temporalecho.mana_cost",               70) \
    CFG_X(CHRONO_ECHO_COOLDOWN                                 , "ability.chrono.temporalecho.cooldown",                 8) \
    CFG_X(CHRONO_ECHO_DELAY                                    , "ability.chrono.temporalecho.delay",                    2) \
    CFG_X(CHRONO_ECHO_DAMAGE_PCT                               , "ability.chrono.temporalecho.damage_pct",             75) \
    /* ABILITY - CHRONOMANCER - ARMOR */ \
    CFG_X(CHRONO_CHRONOARMOR_PRACTICE_COST                     , "ability.chrono.chronoarmor.practice_cost",            60) \
    /* ======================================================================= */ \
    /* COMBAT - PARADOX - DAMCAP */ \
    CFG_X(COMBAT_DAMCAP_PARA_BASE                              , "combat.damcap.para.base",                           200) \
    CFG_X(COMBAT_DAMCAP_PARA_EXTREME_MULT                      , "combat.damcap.para.extreme_mult",                     5) \
    CFG_X(COMBAT_DAMCAP_PARA_PASTSELF                           , "combat.damcap.para.pastself",                       100) \
    CFG_X(COMBAT_DAMCAP_PARA_TIMELOOP                           , "combat.damcap.para.timeloop",                       150) \
    CFG_X(COMBAT_DAMCAP_PARA_ETERNITY_REDUCTION                 , "combat.damcap.para.eternity_reduction",              50) \
    /* GENERAL - PARADOX - TRAINING */ \
    CFG_X(PARA_TRAIN_COST_MULT                                 , "ability.para.train.cost_mult",                       45) \
    /* ABILITY - PARADOX - DESTABILIZE */ \
    CFG_X(PARA_DESTABILIZE_MANA                                , "ability.para.destabilize.mana",                      60) \
    CFG_X(PARA_DESTABILIZE_FLUX                                , "ability.para.destabilize.flux",                      20) \
    CFG_X(PARA_DESTABILIZE_CD                                  , "ability.para.destabilize.cd",                        10) \
    /* ABILITY - PARADOX - TIMELINE: REWIND */ \
    CFG_X(PARA_REWIND_MANA                                     , "ability.para.rewind.mana",                          100) \
    CFG_X(PARA_REWIND_CD                                       , "ability.para.rewind.cd",                             20) \
    CFG_X(PARA_REWIND_MAX_HEAL                                 , "ability.para.rewind.max_heal",                     1500) \
    /* ABILITY - PARADOX - TIMELINE: SPLIT TIMELINE */ \
    CFG_X(PARA_SPLIT_MANA                                      , "ability.para.split.mana",                           120) \
    CFG_X(PARA_SPLIT_CD                                        , "ability.para.split.cd",                              30) \
    CFG_X(PARA_SPLIT_ROUNDS                                    , "ability.para.split.rounds",                           3) \
    /* ABILITY - PARADOX - TIMELINE: CONVERGENCE */ \
    CFG_X(PARA_CONVERGENCE_MANA                                , "ability.para.convergence.mana",                     180) \
    CFG_X(PARA_CONVERGENCE_FLUX                                , "ability.para.convergence.flux",                      50) \
    CFG_X(PARA_CONVERGENCE_CD                                  , "ability.para.convergence.cd",                        35) \
    CFG_X(PARA_CONVERGENCE_BASE_MIN                            , "ability.para.convergence.base_min",                 500) \
    CFG_X(PARA_CONVERGENCE_BASE_MAX                            , "ability.para.convergence.base_max",                 800) \
    /* ABILITY - PARADOX - TEMPORAL COMBAT: FUTURE STRIKE */ \
    CFG_X(PARA_FUTURESTRIKE_MANA                               , "ability.para.futurestrike.mana",                     60) \
    CFG_X(PARA_FUTURESTRIKE_FLUX                               , "ability.para.futurestrike.flux",                     15) \
    CFG_X(PARA_FUTURESTRIKE_CD                                 , "ability.para.futurestrike.cd",                        6) \
    CFG_X(PARA_FUTURESTRIKE_BASE_MIN                           , "ability.para.futurestrike.base_min",                200) \
    CFG_X(PARA_FUTURESTRIKE_BASE_MAX                           , "ability.para.futurestrike.base_max",                350) \
    CFG_X(PARA_FUTURESTRIKE_DELAY                              , "ability.para.futurestrike.delay",                     2) \
    /* ABILITY - PARADOX - TEMPORAL COMBAT: PAST SELF */ \
    CFG_X(PARA_PASTSELF_MANA                                   , "ability.para.pastself.mana",                        100) \
    CFG_X(PARA_PASTSELF_FLUX                                   , "ability.para.pastself.flux",                         20) \
    CFG_X(PARA_PASTSELF_CD                                     , "ability.para.pastself.cd",                           25) \
    CFG_X(PARA_PASTSELF_DURATION                               , "ability.para.pastself.duration",                      8) \
    CFG_X(PARA_PASTSELF_EXTRA_ATTACKS                          , "ability.para.pastself.extra_attacks",                  2) \
    /* ABILITY - PARADOX - TEMPORAL COMBAT: TIME LOOP */ \
    CFG_X(PARA_TIMELOOP_MANA                                   , "ability.para.timeloop.mana",                         90) \
    CFG_X(PARA_TIMELOOP_CD                                     , "ability.para.timeloop.cd",                           20) \
    CFG_X(PARA_TIMELOOP_ROUNDS                                 , "ability.para.timeloop.rounds",                        3) \
    CFG_X(PARA_TIMELOOP_DODGE                                  , "ability.para.timeloop.dodge",                        30) \
    CFG_X(PARA_TIMELOOP_DAMAGE_BONUS                           , "ability.para.timeloop.damage_bonus",                 20) \
    /* ABILITY - PARADOX - TEMPORAL COMBAT: PARADOX STRIKE */ \
    CFG_X(PARA_PARADOXSTRIKE_MANA                              , "ability.para.paradoxstrike.mana",                   120) \
    CFG_X(PARA_PARADOXSTRIKE_FLUX                              , "ability.para.paradoxstrike.flux",                    30) \
    CFG_X(PARA_PARADOXSTRIKE_CD                                , "ability.para.paradoxstrike.cd",                      12) \
    CFG_X(PARA_PARADOXSTRIKE_BASE_MIN                          , "ability.para.paradoxstrike.base_min",               350) \
    CFG_X(PARA_PARADOXSTRIKE_BASE_MAX                          , "ability.para.paradoxstrike.base_max",               550) \
    /* ABILITY - PARADOX - ENTROPY: AGE */ \
    CFG_X(PARA_AGE_MANA                                        , "ability.para.age.mana",                              80) \
    CFG_X(PARA_AGE_FLUX                                        , "ability.para.age.flux",                              25) \
    CFG_X(PARA_AGE_CD                                          , "ability.para.age.cd",                                15) \
    CFG_X(PARA_AGE_DURATION                                    , "ability.para.age.duration",                          10) \
    CFG_X(PARA_AGE_DAMAGE_REDUCTION                            , "ability.para.age.damage_reduction",                  20) \
    CFG_X(PARA_AGE_DODGE_REDUCTION                             , "ability.para.age.dodge_reduction",                   15) \
    /* ABILITY - PARADOX - ENTROPY: TEMPORAL COLLAPSE */ \
    CFG_X(PARA_COLLAPSE_MANA                                   , "ability.para.collapse.mana",                        160) \
    CFG_X(PARA_COLLAPSE_CD                                     , "ability.para.collapse.cd",                           30) \
    CFG_X(PARA_COLLAPSE_BASE_MIN                               , "ability.para.collapse.base_min",                    400) \
    CFG_X(PARA_COLLAPSE_BASE_MAX                               , "ability.para.collapse.base_max",                    700) \
    CFG_X(PARA_COLLAPSE_FLUX_RESTORE                           , "ability.para.collapse.flux_restore",                 40) \
    /* ABILITY - PARADOX - ENTROPY: ETERNITY */ \
    CFG_X(PARA_ETERNITY_MANA                                   , "ability.para.eternity.mana",                        250) \
    CFG_X(PARA_ETERNITY_CD                                     , "ability.para.eternity.cd",                           60) \
    CFG_X(PARA_ETERNITY_DURATION                               , "ability.para.eternity.duration",                      6) \
    CFG_X(PARA_ETERNITY_AFTERMATH                              , "ability.para.eternity.aftermath",                     2) \
    /* ABILITY - PARADOX - ARMOR */ \
    CFG_X(PARA_PARADOXARMOR_PRACTICE_COST                      , "ability.para.paradoxarmor.practice_cost",            60)


/* Generate enum from X-macro */
typedef enum {
#define CFG_X(name, key, def) CFG_##name,
    CFG_ENTRIES
#undef CFG_X
    CFG_COUNT  /* Total count, also serves as invalid sentinel */
} cfg_key_t;

#endif /* CFG_KEYS_H */
