#!/usr/bin/env python3
"""
add_chronomancer_help.py - Add help entries for Chronomancer and Paradox classes

Usage:
    python add_chronomancer_help.py           # Add all help entries
    python add_chronomancer_help.py --dry-run # Preview without changes
"""

import sqlite3
import argparse
from pathlib import Path

# Chronomancer colors: #x130 (deep copper accent), #x215 (warm amber primary)
# Paradox colors: #x160 (deep crimson accent), #x210 (warm rose primary)

CHRONOMANCER_HELP = """         #x130[>#x215 C H R O N O M A N C E R #x130<]#n

#x215Temporal mages who manipulate the flow of time itself. Chronomancers
wield Temporal Flux - a balance between acceleration and deceleration
that shapes the power of their abilities.#n

#x130-------------------#x215[ #nCORE SYSTEMS #x215]#x130-------------------#n
  #nTemporal Flux#n     Balance resource (0-100, center 50)
  #nAcceleration#n      Push flux UP - speed buffs, extra attacks
  #nDeceleration#n      Push flux DOWN - enemy debuffs, control
  #nInstability#n       Random effects at extreme flux (0-10, 90-100)

#x130-------------------#x215[ #nTRAINING PATHS #x215]#x130------------------#n
  #x215Acceleration#n     Speed        (quicken, timeslip, blur)
  #x215Deceleration#n     Control      (slow, timetrap, stasis)
  #x215Temporal Sight#n   Perception   (foresight, hindsight, temporalecho)
  Train with #Rtimetrain#n - costs primal to advance each path.

#x130-------------------#x215[ #nKEY COMMANDS #x215]#x130-------------------#n
  #Rflux#n            Display current flux level and active effects
  #Rquicken#n         Haste buff, extra attacks (Accel 1)
  #Rtimeslip#n        Prepare evasive counter-attack (Accel 2)
  #Rblur#n            Extreme speed, massive attack/dodge bonus (Accel 3)
  #Rslow#n            Debuff enemy speed, reduce attacks (Decel 1)
  #Rtimetrap#n        AoE slow zone affecting all enemies in room (Decel 2)
  #Rstasis#n          Freeze target, store damage for release (Decel 3)
  #Rforesight#n       Dodge bonus from seeing the future (Sight 1)
  #Rhindsight#n       Stacking damage buff from learning (Sight 2)
  #Rtemporalecho#n    Attack that echoes again after a delay (Sight 3)
  #Rtimetrain#n       Display/improve training paths
  #Rchronoarmor#n     Create class equipment

#x130-------------------#x215[ #nFLUX ZONES #x215]#x130---------------------#n
  #x033Deep Slow#n       Deceleration abilities at maximum power
  #x039Slow#n            Deceleration enhanced, acceleration weakened
  #x215Balanced#n        Both schools at normal power
  #x208Fast#n            Acceleration enhanced, deceleration weakened
  #x196Deep Fast#n       Acceleration abilities at maximum power

See also: TEMPORAL FLUX"""

FLUX_HELP = """#x130-------------------#x215[ #nTEMPORAL FLUX #x215]#x130-------------------#n

Temporal Flux is the balance resource used by Chronomancers and Paradoxes.
It represents your position on the timeline between acceleration and
deceleration.

#x215Flux Balance:#n
  #x130Chronomancer#n: Ranges 0-100, center 50
  #x160Paradox#n:      Ranges 0-150, center 75 (expanded after upgrade)
  - Acceleration abilities push flux UP
  - Deceleration abilities push flux DOWN
  - Flux naturally drifts back toward center over time

#x215Power Scaling:#n
  Abilities scale based on flux position. Acceleration abilities grow
  stronger as flux rises, while deceleration abilities grow stronger
  as flux falls. At the center, both schools are at normal power.

#x215Instability:#n
  At extreme flux positions, temporal instability can occur. Random
  effects may trigger each tick including time skips, echoes, healing,
  or sudden acceleration. Paradoxes can trigger instability voluntarily
  with the #Rdestabilize#n command.

#x215Viewing Flux:#n
  Use the #Rflux#n command to see:
  - Current flux level and cap
  - Active zone and power modifiers
  - Active ability effects and durations

#x215Prompt Variable:#n
  Add %R to your prompt to display flux: prompt <%hhp %mm %RE>

See also: CHRONOMANCER, PARADOX"""


PARADOX_HELP = """           #x160>(#x210 P A R A D O X #x160)<#n

#x210Temporal anomalies who have transcended the normal flow of time.
Paradoxes wield an expanded Temporal Flux (0-150) and can rewind
damage, summon echoes, create time loops, and unleash entropy.#n
Upgrades from: #x130[>#x215Chronomancer#x130<]#n

#x160-------------------#x210[ #nCORE SYSTEMS #x210]#x160-------------------#n
  #nTemporal Flux#n     Expanded range (0-150, center 75)
  #nEcho Count#n        Temporal resonance from ability use (0-5)
  #nDestabilize#n       Voluntarily trigger instability effects
  #nSlower Drift#n      Flux drifts +/-1 per tick (vs +/-2)

#x160-------------------#x210[ #nTRAINING PATHS #x210]#x160------------------#n
  #x210Timeline#n         Manipulation  (rewind, splittimeline, convergence)
  #x210Temporal Combat#n  Combat        (futurestrike, pastself, timeloop,
                                    paradoxstrike)
  #x210Entropy#n          Ultimate      (age, temporalcollapse, eternity)
  Train with #Rparatrain#n - costs primal to advance each path.

#x160-------------------#x210[ #nKEY COMMANDS #x210]#x160-------------------#n
  #Rflux#n              Display current flux level and active effects
  #Rdestabilize#n       Trigger voluntary instability effect
  #Rrewind#n            Undo damage taken over last 5 ticks (Timeline 1)
  #Rsplittimeline#n     Snapshot HP/mana, choose keep or revert (Timeline 2)
  #Rconvergence#n       Collapse echo count for multiplied damage (Timeline 3)
  #Rfuturestrike#n      Delayed damage attack, adds echo (Combat 1)
  #Rpastself#n          Summon past echo for extra attacks (Combat 2)
  #Rtimeloop#n          Trap enemy, +dodge/+damage bonus (Combat 3)
  #Rparadoxstrike#n     Undodgeable attack scaling with flux (Combat 4)
  #Rage#n               Debuff target speed and damage (Entropy 1)
  #Rtemporalcollapse#n  AoE damage at extreme flux (Entropy 2)
  #Reternity#n          Immunity ultimate with aftermath stun (Entropy 3)
  #Rparatrain#n         Display/improve training paths
  #Rparadoxarmor#n      Create class equipment

#x160-------------------#x210[ #nFLUX ZONES #x210]#x160---------------------#n
  #x033Temporal Anchor#n  Immune to time effects, accel at max power
  #x039Slow#n              Deceleration enhanced, acceleration weakened
  #x210Balanced#n          All abilities at normal power
  #x208Fast#n              Acceleration enhanced, deceleration weakened
  #x196Temporal Storm#n   Extra random effects, decel at max power

See also: TEMPORAL FLUX, CHRONOMANCER"""


def add_help_entry(cursor, keyword, text, dry_run=False):
    """Add or update a help entry."""
    print(f"  Adding help: {keyword}")
    if not dry_run:
        cursor.execute("DELETE FROM helps WHERE keyword = ?", (keyword,))
        cursor.execute("INSERT INTO helps (keyword, text) VALUES (?, ?)",
                       (keyword, text))


def update_classes_help(cursor, dry_run=False):
    """Update the CLASSES help entry to include Chronomancer and Paradox."""
    cursor.execute("SELECT text FROM helps WHERE keyword = 'CLASS CLASSES'")
    row = cursor.fetchone()
    if not row:
        print("  WARNING: CLASSES help entry not found!")
        return

    text = row[0]
    changed = False

    # Add Chronomancer before the UPGRADE CLASSES section (it's a base class)
    if 'Chronomancer' not in text:
        chrono_line = ("#x130[>#x215Chronomancer#x130<]#n    "
                       "Temporal mages wielding the balance of Flux.\n")
        text = text.replace(
            "\nUPGRADE CLASSES :",
            chrono_line + "\nUPGRADE CLASSES :"
        )
        changed = True

    # Add Paradox to the upgrade classes section
    if 'Paradox' not in text:
        paradox_line = ("#x160>(#x210Paradox#x160)<#n         "
                        "Temporal anomalies beyond the flow of time."
                        "  (from Chronomancer)\n")
        # Insert after the last upgrade class entry (before closing section)
        # Find the end of the upgrade classes list
        if 'Voidborn' in text:
            # Add after Voidborn line
            lines = text.split('\n')
            for i, line in enumerate(lines):
                if 'Voidborn' in line:
                    lines.insert(i + 1, paradox_line.rstrip('\n'))
                    break
            text = '\n'.join(lines)
        else:
            # Fallback: add after UPGRADE CLASSES header
            text = text.replace(
                "UPGRADE CLASSES :\n",
                "UPGRADE CLASSES :\n" + paradox_line
            )
        changed = True

    if changed:
        print("  Updating CLASSES help entry")
        if not dry_run:
            cursor.execute(
                "UPDATE helps SET text = ? WHERE keyword = 'CLASS CLASSES'",
                (text,))
    else:
        print("  CLASSES help already includes Chronomancer and Paradox, skipping")


def main():
    parser = argparse.ArgumentParser(
        description="Add Chronomancer help entries")
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
    add_help_entry(cursor, "CHRONOMANCER CHRONOMANCERS", CHRONOMANCER_HELP, args.dry_run)
    add_help_entry(cursor, "PARADOX PARADOXES", PARADOX_HELP, args.dry_run)
    add_help_entry(cursor, "TEMPORAL FLUX", FLUX_HELP, args.dry_run)

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
