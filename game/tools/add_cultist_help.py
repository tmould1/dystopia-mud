#!/usr/bin/env python3
"""
add_cultist_help.py - Add help entries for Cultist/Voidborn classes

Usage:
    python add_cultist_help.py           # Add all help entries
    python add_cultist_help.py --dry-run # Preview without changes
"""

import sqlite3
import argparse
from pathlib import Path

# Cultist colors: #x064 (dark olive accent), #x120 (lime green primary)
# Voidborn colors: #x055 (dark violet accent), #x097 (eldritch purple primary)

CULTIST_HELP = """           #x064{~#x120 C U L T I S T #x064~}#n

#x120Devotees of the void who channel eldritch energies through forbidden
rituals. Cultists wield the dangerous Corruption resource - a double-edged
power that strengthens abilities but slowly consumes the wielder.#n

#x064-------------------#x120[ #nCORE SYSTEMS #x120]#x064-------------------#n
  #nCorruption#n       Risk/reward combat resource
  #nBuilding#n         Builds while in combat, abilities add corruption
  #nDecay#n            Decays out of combat
  #nSelf-Damage#n      High corruption burns you each tick

#x064-------------------#x120[ #nTRAINING PATHS #x120]#x064------------------#n
  #x120Forbidden Lore#n   Knowledge     (eldritchsight, whispers, unravel)
  #x120Tentacle Arts#n    Physical      (voidtendril, grasp, constrict)
  #x120Madness#n          Mental        (maddeninggaze, gibbering, insanity)
  Train with #Rvoidtrain#n - costs primal to advance each path.

#x064-------------------#x120[ #nKEY COMMANDS #x120]#x064-------------------#n
  #Rcorruption#n      Display current corruption level and effects
  #Rpurge#n           Emergency corruption dump at an HP cost
  #Reldritchsight#n   See hidden and invisible (Lore 1)
  #Rwhispers#n        Reveal weakness, apply Exposed debuff (Lore 2)
  #Runravel#n         Strip sanctuary and protective magic (Lore 3)
  #Rvoidtendril#n     Void-damage attack with corruption bonus (Tentacle 1)
  #Rgrasp#n           Prevent target from fleeing (Tentacle 2)
  #Rconstrict#n       Crushing DoT on grasped target (Tentacle 3)
  #Rmaddeninggaze#n   Sanity-shattering damage (Madness 1)
  #Rgibbering#n       AoE confusion, NPCs may flee (Madness 2)
  #Rinsanity#n        Ultimate madness - massive damage (Madness 3)
  #Rvoidtrain#n       Display/improve training paths
  #Rcultistarmor#n    Create class equipment

#x064-------------------#x120[ #nCORRUPTION TIERS #x120]#x064----------------#n
  #nLow#n             Safe - no self-damage
  #nModerate#n        Light void burn each tick
  #nHigh#n            Stronger burn, but enhanced abilities
  #nMaximum#n         Severe burn each tick, maximum power

See also: CORRUPTION, VOIDBORN"""

VOIDBORN_HELP = """          #x055*(#x097 V O I D B O R N #x055)*#n

#x097Beings who have surrendered to the void, becoming living conduits
of cosmic horror. Voidborn warp reality itself, summon alien entities,
and push Corruption beyond mortal limits. The ultimate evolution of
the Cultist.#n

#x055-------------------#x097[ #nCORE SYSTEMS #x097]#x055-------------------#n
  #nCorruption#n       Higher cap and partial self-damage resistance
  #nBuilding#n         Builds faster in combat than Cultist
  #nDecay#n            Decays slower out of combat than Cultist
  #nSelf-Damage#n      Same thresholds, but reduced damage taken

#x055-------------------#x097[ #nTRAINING PATHS #x097]#x055------------------#n
  #x097Reality Warp#n    Space/time    (phaseshift, dimensionalrend, unmake)
  #x097Elder Form#n      Transformation (voidshape, aberrantgrowth, finalform)
  #x097Cosmic Horror#n   Summoning      (summonthing, starspawn, entropy)
  Train with #Rvoidtrain#n - costs primal to advance each path.

#x055-------------------#x097[ #nKEY COMMANDS #x097]#x055-------------------#n
  #Rcorruption#n      Display corruption level (shared with Cultist)
  #Rpurge#n           Emergency corruption dump (shared with Cultist)
  #Rphaseshift#n      Become partially ethereal, dodge bonus (Warp 1)
  #Rdimensionalrend#n Damage + create rift hazard in room (Warp 2)
  #Runmake#n          Reality-shattering ultimate, requires high corruption (Warp 3)
  #Rvoidshape#n       Grow void tentacles for extra attacks (Form 1)
  #Raberrantgrowth#n  Damage/resistance mutation buff (Form 2)
  #Rfinalform#n       Full eldritch transformation, requires high corruption (Form 3)
  #Rsummonthing#n     Summon a void creature to fight for you (Horror 1)
  #Rstarspawn#n       AoE cosmic energy blast (Horror 2)
  #Rentropy#n         AoE decay aura, damages self too, requires high corruption (Horror 3)
  #Rvoidtrain#n       Display/improve training paths
  #Rvoidbornarmor#n   Create class equipment

#x055-------------------#x097[ #nCORRUPTION TIERS #x097]#x055----------------#n
  Voidborn has an additional tier beyond Cultist's maximum:
  #nLow#n             Safe - no self-damage
  #nModerate#n        Light void burn each tick
  #nHigh#n            Stronger burn, enhanced abilities
  #nSevere#n          Heavy burn each tick
  #nCatastrophic#n    Maximum power, dangerous self-damage

See also: CORRUPTION, CULTIST"""

CORRUPTION_HELP = """#x064-------------------#x120[ #nCORRUPTION #x120]#x064--------------------#n

Corruption is the combat resource used by Cultist and Voidborn classes.
It represents void energy slowly consuming the wielder's mortal form.

#x120Building Corruption:#n
  - Builds automatically while in combat (Voidborn builds faster)
  - Most abilities add corruption when used
  - Higher corruption increases damage output (damcap bonus)

#x120Corruption Decay:#n
  - Decays over time when not in combat
  - Voidborn corruption decays more slowly than Cultist

#x120Corruption Cap:#n
  - Voidborn have a higher corruption cap than Cultist

#x120Self-Damage:#n
  The void damages you when corruption exceeds certain thresholds.
  Higher corruption means more self-damage per tick. Multiple tiers
  exist, with each dealing progressively more damage.
  Voidborn take reduced self-damage from corruption.

#x120Emergency Purge:#n
  Use #Rpurge#n to dump corruption instantly at an HP cost.

#x120Viewing Corruption:#n
  Use the #Rcorruption#n command to see:
  - Current corruption level and cap
  - Active damage tier
  - Active ability effects and durations

#x120Prompt Variable:#n
  Add %R to your prompt to display corruption: prompt <%hhp %mm %RE>

See also: CULTIST, VOIDBORN"""


def add_help_entry(cursor, keyword, text, dry_run=False):
    """Add or update a help entry."""
    print(f"  Adding help: {keyword}")
    if not dry_run:
        cursor.execute("DELETE FROM helps WHERE keyword = ?", (keyword,))
        cursor.execute("INSERT INTO helps (keyword, text) VALUES (?, ?)",
                       (keyword, text))


def update_classes_help(cursor, dry_run=False):
    """Update the CLASSES help entry to include Cultist and Voidborn."""
    cursor.execute("SELECT text FROM helps WHERE keyword = 'CLASS CLASSES'")
    row = cursor.fetchone()
    if not row:
        print("  WARNING: CLASSES help entry not found!")
        return

    old_text = row[0]

    # Check if already added
    if 'Cultist' in old_text:
        print("  CLASSES help already includes Cultist, skipping update")
        return

    # Add Cultist before the UPGRADE CLASSES section (it's a base class)
    cultist_line = ("#x064{~#x120Cultist#x064~}#n        "
                    "Void devotees wielding dangerous Corruption power.\n")

    # Add Voidborn to upgrade section (after Mechanist)
    voidborn_line = ("#x055*(#x097Voidborn#x055)*#n         "
                     "Reality-warping beings who have surrendered to the void.\n")

    # Insert Cultist before UPGRADE CLASSES line
    new_text = old_text.replace(
        "\nUPGRADE CLASSES :",
        cultist_line + "\nUPGRADE CLASSES :"
    )

    # Insert Voidborn after Mechanist line if it exists, otherwise at end
    if 'Mechanist' in new_text:
        mechanist_line = ("Cybernetic war machines commanding drone armies.\n")
        new_text = new_text.replace(
            mechanist_line,
            mechanist_line + voidborn_line
        )
    elif 'Wyrm' in new_text:
        wyrm_line = "Ancient dragons of devastating power.\n"
        new_text = new_text.replace(
            wyrm_line,
            wyrm_line + voidborn_line
        )

    print("  Updating CLASSES help entry")
    if not dry_run:
        cursor.execute(
            "UPDATE helps SET text = ? WHERE keyword = 'CLASS CLASSES'",
            (new_text,))


def main():
    parser = argparse.ArgumentParser(
        description="Add Cultist/Voidborn help entries")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview without making changes")
    args = parser.parse_args()

    script_dir = Path(__file__).parent
    db_path = (script_dir.parent.parent / "gamedata" / "db" / "game"
               / "base_help.db")

    print(f"Using database: {db_path}")
    if args.dry_run:
        print("[DRY RUN MODE]")

    conn = sqlite3.connect(str(db_path))
    cursor = conn.cursor()

    print("\nAdding help entries:")
    add_help_entry(cursor, "CULTIST CULTISTS", CULTIST_HELP, args.dry_run)
    add_help_entry(cursor, "VOIDBORN", VOIDBORN_HELP, args.dry_run)
    add_help_entry(cursor, "CORRUPTION CORRUPTIONS",
                   CORRUPTION_HELP, args.dry_run)

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
