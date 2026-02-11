#!/usr/bin/env python3
"""
add_artificer_help.py - Add help entries for Artificer/Mechanist classes

Usage:
    python add_artificer_help.py           # Add all help entries
    python add_artificer_help.py --dry-run # Preview without changes
"""

import sqlite3
import argparse
from pathlib import Path

# Artificer colors: #x037 (dark teal accent), #x117 (sky blue primary)
# Mechanist colors: #x127 (dark magenta accent), #x177 (violet primary)

ARTIFICER_HELP = """         #x037[=#x117 A R T I F I C E R #x037=]#n

#x117Technology-wielding inventors who build gadgets, deploy turrets, and
charge Power Cells through combat. Their versatile arsenal of energy
weapons and defensive tech makes them formidable on any battlefield.#n

#x037-------------------#x117[ #nCORE SYSTEMS #x117]#x037-------------------#n
  #nPower Cells#n      Stored in combat resource (cap 100, 120 overcharged)
  #nBuilding#n         +5/tick in combat, powercharge for burst generation
  #nDecay#n            Power decays out of combat (-1/tick)
  #nOvercharge#n       Temporarily exceed cap to 120 for boosted abilities

#x037-------------------#x117[ #nTRAINING PATHS #x117]#x037------------------#n
  #x117Gadgetry#n        Utility devices    (turret, decoy, grapple)
  #x117Weaponsmithing#n  Combat gear        (energyblade, blaster, grenade)
  #x117Defensive Tech#n  Protection         (forcefield, repairbot, artcloak)
  Train with #Rtechtrain#n - costs primal to advance each path.

#x037-------------------#x117[ #nKEY COMMANDS #x117]#x037-------------------#n
  #Rpower#n            Display current power cells and active gadgets
  #Rpowercharge#n      Generate burst of power cells (out of combat)
  #Rovercharge#n       Temporarily exceed power cell cap
  #Rturret#n           Deploy an automated turret (Gadgetry 1)
  #Rdecoy#n            Create a holographic distraction (Gadgetry 2)
  #Rgrapple#n          Grappling hook for mobility and combat (Gadgetry 3)
  #Renergyblade#n      Summon an energy melee weapon (Weaponsmithing 1)
  #Rblaster#n          Ranged energy attack (Weaponsmithing 2)
  #Rgrenade#n          Explosive AoE damage (Weaponsmithing 3)
  #Rforcefield#n       Energy shield absorbs damage (Defensive Tech 1)
  #Rrepairbot#n        Deploy a healing drone (Defensive Tech 2)
  #Rartcloak#n         Technological invisibility (Defensive Tech 3)
  #Rtechtrain#n        Display/improve training paths
  #Rartificerarmor#n   Create class equipment (60 primal)

See also: POWER CELLS, MECHANIST"""

MECHANIST_HELP = """         #x127>/#x177 M E C H A N I S T #x127\\<#n

#x177Cyborg warriors who have integrated technology directly into their
bodies. Mechanists command drone swarms, fire devastating heavy weapons,
and augment themselves with cybernetic implants. The ultimate evolution
of the Artificer.#n

#x127-------------------#x177[ #nCORE SYSTEMS #x177]#x127-------------------#n
  #nPower Cells#n      Cap 150 (175 with Power Core implant), no decay
  #nCombat Drones#n    Up to 4 combat drones + 1 bomber drone active
  #nImplant System#n   3 slots (Neural/Servo/Core) for passive bonuses

#x127-------------------#x177[ #nTRAINING PATHS #x177]#x127------------------#n
  #x177Cybernetics#n     Self-augmentation  (neuraljack, servoarms, reactiveplating)
  #x177Drone Swarm#n     Minion control     (combatdrone, repairswarm, bomberdrone, dronearmy)
  #x177Heavy Ordnance#n  Devastation        (railgun, empburst, orbitalstrike)
  Train with #Rcybtrain#n - costs primal to advance each path.

#x127-------------------#x177[ #nKEY COMMANDS #x177]#x127-------------------#n
  #Rpower#n            Display current power cells and active systems
  #Rneuraljack#n       Neural augmentation for dodge and reflexes (Cyber 1)
  #Rservoarms#n        Strength augmentation for melee damage (Cyber 2)
  #Rreactiveplating#n  Armor augmentation with damage reflect (Cyber 3)
  #Rcombatdrone#n      Deploy an armed combat drone (Drone 1)
  #Rrepairswarm#n      Release healing nanobots (Drone 2)
  #Rbomberdrone#n      Deploy explosive drone, detonate on command (Drone 3)
  #Rdronearmy#n        Mass drone deployment with empowerment (Drone 4)
  #Rrailgun#n          Devastating single-target heavy weapon (Ordnance 1)
  #Rempburst#n         AoE electromagnetic disruption (Ordnance 2)
  #Rorbitalstrike#n    Ultimate: orbital weapons platform strike (Ordnance 3)
  #Rmechimplant#n      Install/swap cybernetic implants
  #Rdronestatus#n      View all active drone HP and status
  #Rdronerecall#n      Recall and despawn all drones
  #Rcybtrain#n         Display/improve training paths
  #Rmechanistarmor#n   Create class equipment (60 primal)

#x127-------------------#x177[ #nIMPLANT SLOTS #x177]#x127------------------#n
  #nNeural#n           Combat Processor / Targeting Suite / Threat Analyzer
  #nServo#n            Power Arms / Multi-Tool / Shield Generator
  #nCore#n             Armored Chassis / Regenerator / Power Core

See also: POWER CELLS, ARTIFICER"""

POWER_CELLS_HELP = """#x037-------------------#x117[ #nPOWER CELLS #x117]#x037-------------------#n

Power Cells are the combat resource used by Artificer and Mechanist classes.
They represent stored technological energy that fuels abilities and gadgets.

#x117Building Power Cells:#n
  - Artificer: +5 per tick while in combat
  - Mechanist: +7 per tick while in combat
  - #Rpowercharge#n generates a burst of power cells (out of combat)
  - Some abilities generate bonus power on successful use

#x117Power Cell Cap:#n
  - Artificer: 100 (120 while overcharged)
  - Mechanist: 150 (175 with Power Core implant)

#x117Power Cell Decay:#n
  - Artificer: Loses power out of combat (-1 per tick)
  - Mechanist: No decay (internal reactor maintains charge)

#x117Viewing Power Cells:#n
  Use the #Rpower#n command to see:
  - Current power cells and maximum
  - Overcharge status (Artificer)
  - Active gadgets, drones, and implants

#x117Prompt Variable:#n
  Add %R to your prompt to display power cells: prompt <%hhp %mm %RE>

See also: ARTIFICER, MECHANIST"""

def add_help_entry(cursor, keyword, text, dry_run=False):
    """Add or update a help entry."""
    print(f"  Adding help: {keyword}")
    if not dry_run:
        cursor.execute("DELETE FROM helps WHERE keyword = ?", (keyword,))
        cursor.execute("INSERT INTO helps (keyword, text) VALUES (?, ?)", (keyword, text))

def update_classes_help(cursor, dry_run=False):
    """Update the CLASSES help entry to include Artificer and Mechanist."""
    cursor.execute("SELECT text FROM helps WHERE keyword = 'CLASS CLASSES'")
    row = cursor.fetchone()
    if not row:
        print("  WARNING: CLASSES help entry not found!")
        return

    old_text = row[0]

    # Check if already added
    if 'Artificer' in old_text:
        print("  CLASSES help already includes Artificer, skipping update")
        return

    # Add Artificer before the UPGRADE CLASSES section (it's a base class)
    artificer_line = "#x037[=#x117Artificer#x037=]#n       Technology-wielding inventors with turrets and gadgets.\n"

    # Add Mechanist to upgrade section (after Wyrm if present, otherwise after Mindflayer)
    mechanist_line = "#x127>/#x177Mechanist#x127\\<#n          Cybernetic war machines commanding drone armies.\n"

    # Insert Artificer before UPGRADE CLASSES line
    new_text = old_text.replace(
        "\nUPGRADE CLASSES :",
        artificer_line + "\nUPGRADE CLASSES :"
    )

    # Insert Mechanist after Wyrm line if it exists, otherwise after Mindflayer
    if 'Wyrm' in new_text:
        wyrm_line = "#x202<#x220*Wyrm*#x202>#n              Ancient dragons of devastating power.\n"
        new_text = new_text.replace(
            wyrm_line,
            wyrm_line + mechanist_line
        )
    else:
        new_text = new_text.replace(
            "#x135}#x035{#GMindflayer#x035}#x135{#n        Alien intellects who dominate minds and consume thought.\n",
            "#x135}#x035{#GMindflayer#x035}#x135{#n        Alien intellects who dominate minds and consume thought.\n" + mechanist_line
        )

    print("  Updating CLASSES help entry")
    if not dry_run:
        cursor.execute("UPDATE helps SET text = ? WHERE keyword = 'CLASS CLASSES'", (new_text,))

def main():
    parser = argparse.ArgumentParser(description="Add Artificer/Mechanist help entries")
    parser.add_argument("--dry-run", action="store_true", help="Preview without making changes")
    args = parser.parse_args()

    script_dir = Path(__file__).parent
    db_path = script_dir.parent.parent / "gamedata" / "db" / "game" / "base_help.db"

    print(f"Using database: {db_path}")
    if args.dry_run:
        print("[DRY RUN MODE]")

    conn = sqlite3.connect(str(db_path))
    cursor = conn.cursor()

    print("\nAdding help entries:")
    add_help_entry(cursor, "ARTIFICER ARTIFICERS", ARTIFICER_HELP, args.dry_run)
    add_help_entry(cursor, "MECHANIST MECHANISTS", MECHANIST_HELP, args.dry_run)
    add_help_entry(cursor, "POWER CELLS POWERCELLS POWER_CELLS", POWER_CELLS_HELP, args.dry_run)

    print("\nUpdating existing entries:")
    update_classes_help(cursor, args.dry_run)

    if not args.dry_run:
        conn.commit()
        print("\nDone! All help entries added.")
    else:
        print("\n[DRY RUN] No changes made.")

    conn.close()

if __name__ == "__main__":
    main()
