#!/usr/bin/env python3
"""
add_shaman_help.py - Add help entries for Shaman and Spirit Lord classes

Usage:
    python add_shaman_help.py           # Add all help entries
    python add_shaman_help.py --dry-run # Preview without changes
"""

import sqlite3
import argparse
from pathlib import Path

# Shaman colors: #t1A7A6A (deep teal accent), #t5EC4B0 (jade green primary)
# Spirit Lord colors: #t4060A0 (spectral indigo accent), #t90B8E0 (luminous sky primary)

SHAMAN_HELP = """            #t1A7A6A~(#t5EC4B0 S H A M A N #t1A7A6A)~#n

#t5EC4B0Spirit world conduits who walk between the material and ethereal
planes. Shamans wield the Spirit Tether - a balance between the
grounded material world and the unmoored spirit realm.#n

#t1A7A6A-------------------#t5EC4B0[ #nCORE SYSTEMS #t5EC4B0]#t1A7A6A-------------------#n
  #nSpirit Tether#n     Balance resource (0-100, center 50)
  #nMaterial#n           Totems, physical defense (decrease tether)
  #nSpirit#n             Spirit attacks, phasing (increase tether)
  #nManifestation#n      Random effects at extreme tether (0-10, 90-100)

#t1A7A6A-------------------#t5EC4B0[ #nTRAINING PATHS #t5EC4B0]#t1A7A6A------------------#n
  #t5EC4B0Totems#n            Defense     (wardtotem, wrathtotem, spirittotem)
  #t5EC4B0Spirits#n           Offense     (spiritbolt, spiritward, spiritwalk)
  #t5EC4B0Communion#n         Utility     (ancestorcall, spiritsight, soullink)
  Train with #Rspiritttrain#n - costs primal to advance each path.

#t1A7A6A-------------------#t5EC4B0[ #nKEY COMMANDS #t5EC4B0]#t1A7A6A-------------------#n
  #Rtether#n          Display current tether level and active effects
  #Rwardtotem#n       Defensive totem, reduces damage (Totems 1)
  #Rwrathtotem#n      Offensive totem, deals tick damage (Totems 2)
  #Rspirittotem#n     Mana drain totem, steals mana (Totems 3)
  #Rspiritbolt#n      Spirit damage scaling with tether (Spirits 1)
  #Rspiritward#n      Charge-based damage absorption (Spirits 2)
  #Rspiritwalk#n      Phase shift, increased dodge (Spirits 3)
  #Rancestorcall#n    Heal with balance bonus at center (Communion 1)
  #Rspiritsight#n     Enhanced vision buff (Communion 2)
  #Rsoullink#n        Share damage with fighting target (Communion 3)
  #Rspiritttrain#n    Display/improve training paths
  #Rshamanarmor#n     Create class equipment

#t1A7A6A-------------------#t5EC4B0[ #nTETHER ZONES #t5EC4B0]#t1A7A6A-------------------#n
  #x033Grounded#n        Material +30%, Spirit -50%
  #x039Earthbound#n      Material +15%, Spirit -25%
  #t5EC4B0Balanced#n         All abilities at normal power
  #x208Spirit-Touched#n  Spirit +15%, Material -25%
  #x196Unmoored#n        Spirit +30%, Material -50%

See also: SPIRIT TETHER"""

TETHER_HELP = """#t1A7A6A-------------------#t5EC4B0[ #nSPIRIT TETHER #t5EC4B0]#t1A7A6A-------------------#n

Spirit Tether is the balance resource used by Shamans and Spirit Lords.
It represents your connection between the material and spirit worlds.

#t5EC4B0Tether Balance:#n
  #t1A7A6AShaman#n:       Ranges 0-100, center 50
  #t4060A0Spirit Lord#n:  Ranges 0-150, center 75 (expanded after upgrade)
  - Material abilities (totems) push tether DOWN
  - Spirit abilities push tether UP
  - Tether naturally drifts back toward center over time

#t5EC4B0Power Scaling:#n
  Abilities scale based on tether position. Material abilities grow
  stronger at low tether, while spirit abilities grow stronger at high
  tether. At the center, both schools are at normal power.

#t5EC4B0Manifestation:#n
  At extreme tether positions, spirit manifestation can occur. Random
  effects may trigger each tick including ancestor whispers (heal),
  spirit surges (stun), or tether snaps back toward center.

#t5EC4B0Viewing Tether:#n
  Use the #Rtether#n command to see:
  - Current tether level and cap
  - Active zone and power modifiers
  - Active ability effects and durations

#t5EC4B0Prompt Variable:#n
  Add %R to your prompt to display tether: prompt <%hhp %mm %RE>

See also: SHAMAN, SPIRIT LORD"""


SPIRITLORD_HELP = """         #t4060A0+(#t90B8E0 S P I R I T   L O R D #t4060A0)+#n

#t90B8E0Ascended spirit conduits who have transcended the boundary between
material and spirit. Spirit Lords internalize totems as personal auras,
command spirit armies, and can shed physical form entirely.#n
Upgrades from: #t1A7A6A~(#t5EC4B0Shaman#t1A7A6A)~#n

#t4060A0-------------------#t90B8E0[ #nCORE SYSTEMS #t90B8E0]#t4060A0-------------------#n
  #nSpirit Tether#n     Expanded range (0-150, center 75)
  #nAura System#n        Internalized totems (embody ward/wrath/spirit)
  #nSpirit Fusion#n      Triple aura mode (all three at once)
  #nSlower Drift#n       Tether drifts +/-1 per tick (vs +/-2)

#t4060A0-------------------#t90B8E0[ #nTRAINING PATHS #t90B8E0]#t4060A0------------------#n
  #t90B8E0Embodiment#n        Auras         (embody, ancestralform, spiritfusion)
  #t90B8E0Dominion#n          Combat        (compel, possess, spiritarmy,
                                    soulstorm)
  #t90B8E0Transcendence#n     Ultimate      (ancestralwisdom, spiritcleanse,
                                    ascension)
  Train with #Rlordtrain#n - costs primal to advance each path.

#t4060A0-------------------#t90B8E0[ #nKEY COMMANDS #t90B8E0]#t4060A0-------------------#n
  #Rtether#n            Display current tether level and active effects
  #Rembody#n            Toggle personal aura: ward, wrath, or spirit
  #Rancestralform#n     Transform with extra attacks + dodge (Embody 2)
  #Rspiritfusion#n      Activate all three auras at once (Embody 3)
  #Rcompel#n            Direct spirit damage, undodgeable (Dominion 1)
  #Rpossess#n           Charm an NPC via spirit possession (Dominion 2)
  #Rspiritarmy#n        Summon 3 spectral warriors (Dominion 3)
  #Rsoulstorm#n         AoE spirit damage, requires tether >100 (Dominion 4)
  #Rancestralwisdom#n   Multi-stat buff with HP regen (Transcend 1)
  #Rspiritcleanse#n     Heal 20% HP, reset tether to center (Transcend 2)
  #Rascension#n         Shed physical form, auto-bolt, aftermath stun (Transcend 3)
  #Rlordtrain#n         Display/improve training paths
  #Rlordarmor#n         Create class equipment

#t4060A0-------------------#t90B8E0[ #nTETHER ZONES #t90B8E0]#t4060A0-------------------#n
  #x033Anchored#n        Material +40%, Spirit -60%, immune to spirit
  #x039Earthbound#n      Material +20%, Spirit -30%
  #t90B8E0Balanced#n          All abilities normal, can use Embodiment
  #x208Spirit-Touched#n  Spirit +20%, Material -30%
  #x196Ascended#n        Spirit +40%, Material -60%, phase through walls

See also: SPIRIT TETHER, SHAMAN"""


def add_help_entry(cursor, keyword, text, dry_run=False):
    """Add or update a help entry."""
    print(f"  Adding help: {keyword}")
    if not dry_run:
        cursor.execute("DELETE FROM helps WHERE keyword = ?", (keyword,))
        cursor.execute("INSERT INTO helps (keyword, text) VALUES (?, ?)",
                       (keyword, text))


def update_classes_help(cursor, dry_run=False):
    """Update the CLASSES help entry to include Shaman and Spirit Lord."""
    cursor.execute("SELECT text FROM helps WHERE keyword = 'CLASS CLASSES'")
    row = cursor.fetchone()
    if not row:
        print("  WARNING: CLASSES help entry not found!")
        return

    text = row[0]
    changed = False

    # Add Shaman before the UPGRADE CLASSES section (it's a base class)
    if 'Shaman' not in text:
        shaman_line = ("#t1A7A6A~(#t5EC4B0Shaman#t1A7A6A)~#n          "
                       "Spirit conduits walking between worlds.\n")
        text = text.replace(
            "\nUPGRADE CLASSES :",
            shaman_line + "\nUPGRADE CLASSES :"
        )
        changed = True

    # Add Spirit Lord to the upgrade classes section
    if 'Spirit Lord' not in text:
        sl_line = ("#t4060A0+(#t90B8E0Spirit Lord#t4060A0)+#n     "
                   "Ascended spirit masters beyond the veil."
                   "  (from Shaman)\n")
        # Insert after the last upgrade class entry
        if 'Paradox' in text:
            lines = text.split('\n')
            for i, line in enumerate(lines):
                if 'Paradox' in line:
                    lines.insert(i + 1, sl_line.rstrip('\n'))
                    break
            text = '\n'.join(lines)
        else:
            text = text.replace(
                "UPGRADE CLASSES :\n",
                "UPGRADE CLASSES :\n" + sl_line
            )
        changed = True

    # Update existing entries to use true color (handle old xterm and broken #W)
    old_new = [
        ("#x026~(#x116Shaman#x026)~", "#t1A7A6A~(#t5EC4B0Shaman#t1A7A6A)~"),
        ("#x135+(#WSpirit Lord#x135)+",
         "#t4060A0+(#t90B8E0Spirit Lord#t4060A0)+"),
        ("#x135+(#9Spirit Lord#x135)+",
         "#t4060A0+(#t90B8E0Spirit Lord#t4060A0)+"),
    ]
    for old, new in old_new:
        if old in text:
            text = text.replace(old, new)
            changed = True

    if changed:
        print("  Updating CLASSES help entry")
        if not dry_run:
            cursor.execute(
                "UPDATE helps SET text = ? WHERE keyword = 'CLASS CLASSES'",
                (text,))
    else:
        print("  CLASSES help already includes Shaman and Spirit Lord, skipping")


def main():
    parser = argparse.ArgumentParser(
        description="Add Shaman/Spirit Lord help entries")
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
    add_help_entry(cursor, "SHAMAN SHAMANS", SHAMAN_HELP, args.dry_run)
    add_help_entry(cursor, "SPIRIT LORD SPIRITLORD SPIRITLORDS", SPIRITLORD_HELP, args.dry_run)
    add_help_entry(cursor, "SPIRIT TETHER", TETHER_HELP, args.dry_run)

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
