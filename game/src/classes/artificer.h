/***************************************************************************
 *  Artificer / Mechanist Class Header                                     *
 *  Technology-focused classes with turrets, drones, and power cells       *
 ***************************************************************************/

#ifndef ARTIFICER_H
#define ARTIFICER_H

/*
 * Class bit values (power-of-2)
 * These are also defined in class.h for global access
 */
#define CLASS_ARTIFICER     1048576   /* 2^20 - Base class */
#define CLASS_MECHANIST     2097152   /* 2^21 - Upgrade class */

/*
 * Power Cell caps
 */
#define ART_POWER_CAP               100
#define ART_POWER_CAP_OVERCHARGE    120
#define MECH_POWER_CAP              150

/*
 * Maximum turrets/drones
 */
#define ART_MAX_TURRETS     2
#define MECH_MAX_DRONES     4

/* =========================================================================
 * ARTIFICER pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) */
#define ART_TURRET_COUNT    0   /* Number of active turrets (0-2) */
#define ART_FORCEFIELD      1   /* Force field ticks remaining */
#define ART_ENERGYBLADE     2   /* Energy blade ticks remaining */
#define ART_REPAIRBOT       3   /* Repair bot ticks remaining */
#define ART_CLOAK           4   /* Cloak ticks remaining */
#define ART_DECOY           5   /* Decoy ticks remaining */
#define ART_OVERCHARGE      6   /* Overcharge ticks remaining */
#define ART_GRAPPLE_CD      7   /* Grapple cooldown pulses */
#define ART_BLASTER_CD      8   /* Blaster cooldown pulses */
#define ART_GRENADE_CD      9   /* Grenade cooldown pulses */

/* Training levels (10+) */
#define ART_TRAIN_GADGET    10  /* Gadgetry level (0-3) */
#define ART_TRAIN_WEAPON    11  /* Weaponsmithing level (0-3) */
#define ART_TRAIN_DEFENSE   12  /* Defensive Tech level (0-3) */

/* pcdata->stats[] indices */
#define ART_STAT_FORCEFIELD_HP  0   /* Current force field HP remaining */
#define ART_STAT_DECOY_HP       1   /* Current decoy HP remaining */
#define ART_STAT_PEAK_POWER     2   /* Highest power reached this session */

/* =========================================================================
 * MECHANIST pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) - REPLACES Artificer states on upgrade */
#define MECH_NEURAL_JACK        0   /* Neural Jack ticks remaining */
#define MECH_SERVO_ARMS         1   /* Servo Arms ticks remaining */
#define MECH_REACTIVE_PLATING   2   /* Reactive Plating ticks remaining */
#define MECH_DRONE_COUNT        3   /* Number of active combat drones (0-4) */
#define MECH_REPAIR_SWARM       4   /* Repair Swarm ticks remaining */
#define MECH_BOMBER_ACTIVE      5   /* Bomber drone deployed (0/1) */
#define MECH_DRONE_ARMY         6   /* Drone Army buff ticks remaining */
#define MECH_ORBITAL_CHARGE     7   /* Orbital Strike charge rounds (0-2) */
#define MECH_NEURAL_IMPLANT     8   /* Current neural implant type (0-3) */
#define MECH_SERVO_IMPLANT      9   /* Current servo implant type (0-3) */

/* Training levels (10+) - REUSES same indices as Artificer */
#define MECH_TRAIN_CYBER        10  /* Cybernetics level (0-3) */
#define MECH_TRAIN_DRONE        11  /* Drone Swarm level (0-4) */
#define MECH_TRAIN_ORDNANCE     12  /* Heavy Ordnance level (0-3) */
#define MECH_CORE_IMPLANT       13  /* Current core implant type (0-3) */

/* Cooldowns (14+) */
#define MECH_RAILGUN_CD         14  /* Railgun cooldown pulses */
#define MECH_EMP_CD             15  /* EMP Burst cooldown pulses */
#define MECH_ORBITAL_CD         16  /* Orbital Strike cooldown pulses */
#define MECH_IMPLANT_CD         17  /* Implant swap cooldown pulses */
#define MECH_DRONEARMY_CD       18  /* Drone Army cooldown pulses */

/* pcdata->stats[] indices */
#define MECH_STAT_PEAK_POWER    0   /* Highest power reached this session */
#define MECH_STAT_DRONES_TOTAL  1   /* Total drones deployed (lifetime) */

/* =========================================================================
 * Implant type constants
 * ========================================================================= */

/* Neural implants (MECH_NEURAL_IMPLANT values) */
#define IMPLANT_NEURAL_NONE         0
#define IMPLANT_NEURAL_COMBAT_PROC  1   /* +15% dodge, -1 pulse cooldowns */
#define IMPLANT_NEURAL_TARGETING    2   /* +20 hitroll, +20% blaster/railgun */
#define IMPLANT_NEURAL_THREAT       3   /* See enemy HP/buffs, attack warning */

/* Servo implants (MECH_SERVO_IMPLANT values) */
#define IMPLANT_SERVO_NONE          0
#define IMPLANT_SERVO_POWER_ARMS    1   /* +30 damroll, +50 melee damage */
#define IMPLANT_SERVO_MULTI_TOOL    2   /* Extra attack +1, grapple in combat */
#define IMPLANT_SERVO_SHIELD_GEN    3   /* +200 damcap, force field regen */

/* Core implants (MECH_CORE_IMPLANT values) */
#define IMPLANT_CORE_NONE           0
#define IMPLANT_CORE_ARMORED        1   /* -50 AC, 10% damage resistance */
#define IMPLANT_CORE_REGENERATOR    2   /* +100 HP/tick, double repair bot */
#define IMPLANT_CORE_POWER          3   /* +25 max power, +1 power/tick */

/* =========================================================================
 * Turret/Drone mob vnums
 * ========================================================================= */

#define VNUM_ART_TURRET         33400
#define VNUM_ART_DECOY          33401
#define VNUM_ART_REPAIRBOT      33402
#define VNUM_MECH_COMBAT_DRONE  33405
#define VNUM_MECH_BOMBER_DRONE  33406

/* =========================================================================
 * Function prototypes
 * ========================================================================= */

/* Artificer abilities */
void do_power           args( ( CHAR_DATA *ch, char *argument ) );
void do_powercharge     args( ( CHAR_DATA *ch, char *argument ) );
void do_overcharge      args( ( CHAR_DATA *ch, char *argument ) );
void do_turret          args( ( CHAR_DATA *ch, char *argument ) );
void do_decoy           args( ( CHAR_DATA *ch, char *argument ) );
void do_grapple         args( ( CHAR_DATA *ch, char *argument ) );
void do_energyblade     args( ( CHAR_DATA *ch, char *argument ) );
void do_blaster         args( ( CHAR_DATA *ch, char *argument ) );
void do_grenade         args( ( CHAR_DATA *ch, char *argument ) );
void do_forcefield      args( ( CHAR_DATA *ch, char *argument ) );
void do_repairbot       args( ( CHAR_DATA *ch, char *argument ) );
void do_artcloak        args( ( CHAR_DATA *ch, char *argument ) );
void do_techtrain       args( ( CHAR_DATA *ch, char *argument ) );
void do_artificerarmor  args( ( CHAR_DATA *ch, char *argument ) );

/* Mechanist abilities */
void do_neuraljack      args( ( CHAR_DATA *ch, char *argument ) );
void do_servoarms       args( ( CHAR_DATA *ch, char *argument ) );
void do_reactiveplating args( ( CHAR_DATA *ch, char *argument ) );
void do_combatdrone     args( ( CHAR_DATA *ch, char *argument ) );
void do_repairswarm     args( ( CHAR_DATA *ch, char *argument ) );
void do_bomberdrone     args( ( CHAR_DATA *ch, char *argument ) );
void do_artdetonate     args( ( CHAR_DATA *ch, char *argument ) );
void do_dronearmy       args( ( CHAR_DATA *ch, char *argument ) );
void do_railgun         args( ( CHAR_DATA *ch, char *argument ) );
void do_empburst        args( ( CHAR_DATA *ch, char *argument ) );
void do_orbitalstrike   args( ( CHAR_DATA *ch, char *argument ) );
void do_mechimplant     args( ( CHAR_DATA *ch, char *argument ) );
void do_cybtrain        args( ( CHAR_DATA *ch, char *argument ) );
void do_mechanistarmor  args( ( CHAR_DATA *ch, char *argument ) );
void do_dronestatus     args( ( CHAR_DATA *ch, char *argument ) );
void do_dronerecall     args( ( CHAR_DATA *ch, char *argument ) );

/* Update functions (called from update.c) */
void update_artificer   args( ( CHAR_DATA *ch ) );
void update_mechanist   args( ( CHAR_DATA *ch ) );

/* Helper functions */
void artificer_despawn_turrets  args( ( CHAR_DATA *ch ) );
void mechanist_move_drones      args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *to_room ) );
void artificer_turret_attacks   args( ( CHAR_DATA *ch ) );
void mechanist_drone_attacks    args( ( CHAR_DATA *ch ) );

#endif /* ARTIFICER_H */
