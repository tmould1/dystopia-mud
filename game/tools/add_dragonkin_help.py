#!/usr/bin/env python3
"""
add_dragonkin_help.py - Add help entries for Dragonkin/Wyrm classes

Usage:
    python add_dragonkin_help.py           # Add all help entries
    python add_dragonkin_help.py --dry-run # Preview without changes
"""

import sqlite3
import argparse
from pathlib import Path

# Dragonkin colors: #x202 (dark orange accent), #x220 (gold primary)
# Wyrm colors: Same family but slightly different decorations

DRAGONKIN_HELP = """         #x202<*#x220[#n D R A G O N K I N #x220]#x202*>#n

#x250Half-dragon warriors who channel elemental draconic power. Dragonkin
build Draconic Essence through combat, fueling devastating breath attacks
and protective scales. Their elemental attunement shapes their abilities.#n

#x202-------------------#x220[ #nCORE SYSTEMS #x220]#x202-------------------#n
  #nDraconic Essence#n  Builds in combat (+3/tick), decays out (-2/tick)
  #nEssence Cap#n       100 - higher essence = stronger abilities
  #nAttunement#n        Fire/Frost/Storm/Earth - changes breath element

#x202-------------------#x220[ #nTRAINING PATHS #x220]#x202------------------#n
  #x220Draconic Breath#n  Breath attacks       (dragonbreath, searingblast)
  #x220Dragon Scales#n    Defensive abilities  (scaleshield, dragonhide)
  #x220Dragon Might#n     Combat enhancement   (drakewings, dragonrage)
  #x220Essence Mastery#n  Resource management  (essenceburst, primalwarding)
  Train with #Rdragontrain#n - costs primal to advance each path.

#x202-------------------#x220[ #nKEY COMMANDS #x220]#x202-------------------#n
  #Ressence#n           Display current essence and active buffs
  #Rattune#n            Switch elemental attunement (fire/frost/storm/earth)
  #Rdragontrain#n       Display/improve training paths
  #Rdragonbreath#n      Elemental breath attack (costs essence)
  #Rsearingblast#n      Stacking DoT breath attack (costs essence)
  #Rscaleshield#n       Absorb damage with dragon scales
  #Rdragonhide#n        Toggle passive damage reduction
  #Rdrakewings#n        Gain flight and extra attacks
  #Rdragonrage#n        Temporary damage boost (costs essence)
  #Ressenceburst#n      Burst of essence generation
  #Rprimalwarding#n     Protective aura for group
  #Rdragonarmor#n       Create class equipment (60 primal)

#x202-------------------#x220[ #nATTUNEMENT EFFECTS #x220]#x202---------------#n
  #RFire#n              Bonus damage, burning DoT on breath
  #CFrost#n             Slow enemies, chance to freeze
  #YStorm#n             Chain lightning, shock procs
  #yEarth#n             Increased defense, knockback

See also: ESSENCE, ATTUNEMENT, WYRM"""

WYRM_HELP = """         #x202<#x220*#n W Y R M #x220*#x202>#n

#x250Ancient dragons who have fully awakened their draconic heritage. Wyrms
command devastating power, able to transform into true dragon form and
unleash apocalyptic destruction. They are the ultimate evolution of Dragonkin.#n

#x202-------------------#x220[ #nCORE SYSTEMS #x220]#x202-------------------#n
  #nDraconic Essence#n  Builds in combat, max 150 (vs 100 for Dragonkin)
  #nWyrm Form#n         Transform into massive dragon (+damcap, +attacks)
  #nPrimordial#n        Passive bonus damage and essence regeneration

#x202-------------------#x220[ #nTRAINING PATHS #x220]#x202------------------#n
  #x220Devastation#n      Mass destruction   (wyrmbreath, cataclysm, annihilate, apocalypse)
  #x220Dominion#n         Control abilities  (dragonfear, terrainshift, dragonlord)
  #x220Ascension#n        Dragon form        (wyrmform, ancientwrath, primordial)
  Train with #Rwyrmtrain#n - costs primal to advance each path.

#x202-------------------#x220[ #nKEY COMMANDS #x220]#x202-------------------#n
  #Ressence#n           Display current essence and active buffs
  #Rattune#n            Switch elemental attunement
  #Rwyrmtrain#n         Display/improve training paths
  #Rwyrmbreath#n        Devastating AoE breath attack
  #Rcataclysm#n         Massive AoE + lingering damage field
  #Rannihilate#n        Execute attack (bonus damage at low HP)
  #Rapocalypse#n        Ultimate: room-wide devastation
  #Rdragonfear#n        AoE fear effect, chance to stun
  #Rterrainshift#n      Set room terrain to your attunement
  #Rdragonlord#n        Charm dragon-type creatures
  #Rwyrmform#n          Transform into massive wyrm
  #Rancientwrath#n      Damage boost while in wyrmform
  #Rwyrmarmor#n         Create class equipment (75 primal)

#x202-------------------#x220[ #nWYRM FORM BONUSES #x220]#x202----------------#n
  #n+300 damcap#n        Massive damage increase
  #n+2 attacks#n         Extra attacks per round
  #n+20% DR#n            Additional damage reduction
  #nAncient Wrath#n      Only usable while transformed

See also: ESSENCE, ATTUNEMENT, DRAGONKIN"""

ESSENCE_HELP = """#x202-------------------#x220[ #nDRACONIC ESSENCE #x220]#x202-----------------#n

Draconic Essence is the combat resource used by Dragonkin and Wyrm classes.
It represents the raw draconic power flowing through your veins.

#x220Building Essence:#n
  - Automatically builds while in combat (+3 per tick)
  - Some abilities generate bonus essence (dragonbreath, essenceburst)
  - Wyrms have higher max (150) vs Dragonkin (100)

#x220Essence Decay:#n
  - Decays slowly when not in combat (-2 per tick)
  - Higher training in Essence Mastery slows decay

#x220Using Essence:#n
  - Many abilities require minimum essence to use
  - Powerful abilities consume essence when activated
  - Higher essence increases damcap and ability damage

#x220Viewing Essence:#n
  Use the #Ressence#n command to see:
  - Current essence level
  - Active buffs and their durations
  - Current elemental attunement

#x220Prompt Variable:#n
  Add %R to your prompt to display essence: prompt <%hhp %mm %RE>

See also: DRAGONKIN, WYRM, ATTUNEMENT"""

ATTUNEMENT_HELP = """#x202-------------------#x220[ #nELEMENTAL ATTUNEMENT #x220]#x202---------------#n

Dragonkin and Wyrms channel one of four elemental attunements that modify
their breath attacks and grant unique combat effects.

#x220Switching Attunement:#n
  Use #Rattune <element>#n to change your current attunement.
  Elements: fire, frost, storm, earth

#x202-------------------#x220[ #nATTUNEMENT TYPES #x220]#x202-----------------#n

#RFire Attunement:#n
  - Breath deals fire damage with burning DoT
  - Bonus damage against vulnerable enemies
  - Best for raw damage output

#CFrost Attunement:#n
  - Breath deals cold damage, slows enemies
  - Chance to freeze targets in place
  - Best for control and kiting

#YStorm Attunement:#n
  - Breath deals lightning damage, chains between targets
  - Chance to shock (interrupt/stun)
  - Best for multi-target fights

#yEarth Attunement:#n
  - Breath deals physical damage with knockback
  - Increased defensive bonuses
  - Best for tanking and survivability

#x220Terrain Shift (Wyrm):#n
  Wyrms can use #Rterrainshift#n to infuse a room with their attunement,
  gaining bonus damage while fighting in that terrain.

See also: DRAGONKIN, WYRM, ESSENCE"""

def add_help_entry(cursor, keyword, text, dry_run=False):
    """Add or update a help entry."""
    print(f"  Adding help: {keyword}")
    if not dry_run:
        cursor.execute("DELETE FROM helps WHERE keyword = ?", (keyword,))
        cursor.execute("INSERT INTO helps (keyword, text) VALUES (?, ?)", (keyword, text))

def update_classes_help(cursor, dry_run=False):
    """Update the CLASSES help entry to include Dragonkin and Wyrm."""
    cursor.execute("SELECT text FROM helps WHERE keyword = 'CLASS CLASSES'")
    row = cursor.fetchone()
    if not row:
        print("  WARNING: CLASSES help entry not found!")
        return

    old_text = row[0]

    # Check if already added
    if 'Dragonkin' in old_text:
        print("  CLASSES help already includes Dragonkin, skipping update")
        return

    # Add Dragonkin before the UPGRADE CLASSES section
    dragonkin_line = "#x202<*#x220Dragonkin#x202*>#n       Half-dragon warriors wielding elemental draconic power.\n"

    # Add Wyrm to upgrade section (after Mindflayer)
    wyrm_line = "#x202<#x220*Wyrm*#x202>#n              Ancient dragons of devastating power.\n"

    # Insert Dragonkin before UPGRADE CLASSES line
    new_text = old_text.replace(
        "\nUPGRADE CLASSES :",
        dragonkin_line + "\nUPGRADE CLASSES :"
    )

    # Insert Wyrm after Mindflayer line
    new_text = new_text.replace(
        "#x135}#x035{#GMindflayer#x035}#x135{#n        Alien intellects who dominate minds and consume thought.\n",
        "#x135}#x035{#GMindflayer#x035}#x135{#n        Alien intellects who dominate minds and consume thought.\n" + wyrm_line
    )

    print("  Updating CLASSES help entry")
    if not dry_run:
        cursor.execute("UPDATE helps SET text = ? WHERE keyword = 'CLASS CLASSES'", (new_text,))

def main():
    parser = argparse.ArgumentParser(description="Add Dragonkin/Wyrm help entries")
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
    add_help_entry(cursor, "DRAGONKIN DRAGONKINS", DRAGONKIN_HELP, args.dry_run)
    add_help_entry(cursor, "WYRM WYRMS", WYRM_HELP, args.dry_run)
    add_help_entry(cursor, "ESSENCE DRACONIC ESSENCE", ESSENCE_HELP, args.dry_run)
    add_help_entry(cursor, "ATTUNEMENT ATTUNE ATTUNEMENTS", ATTUNEMENT_HELP, args.dry_run)

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
