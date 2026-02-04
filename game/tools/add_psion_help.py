#!/usr/bin/env python3
"""
Add help entries for Psion and Mindflayer classes.

This script adds help entries to base_help.db.
Run from the game/tools directory:
    python add_psion_help.py
"""

import sqlite3
from pathlib import Path


HELP_ENTRIES = [
    # Psion class help
    {
        'level': 0,
        'keyword': 'PSION PSIONS',
        'text': '''          #x255<#x033|#n#x039[#n#x039]#x033|#x255>#n  #x039P S I O N#n  #x255<#x033|#n#x039[#n#x039]#x033|#x255>#n

#x250Masters of the mind who channel raw psychic energy into devastating
mental assaults. Psions build Focus through meditation, fueling increasingly
powerful abilities that bypass physical armor entirely.#n

#x033-------------------#x039[ #nCORE SYSTEMS #x039]#x033-------------------#n
  #nFocus#n           Builds via psimeditate, decays out of combat (-2/tick)
  #nFocus Cap#n       100 - higher focus = stronger mental abilities
  #nMental Damage#n   Bypasses armor, resisted only by Intelligence

#x033-------------------#x039[ #nTRAINING PATHS #x039]#x033------------------#n
  #x039Telepathy#n     Mind reading/defense (mindscan, thoughtshield, mentallink)
  #x039Telekinesis#n   Physical force        (forcepush, levitate, kineticbarrier)
  #x039Psi Combat#n    Mental attacks        (mindspike, psychicscream, brainburn)
  Train with #Rpsitrain#n - costs primal to advance each path.

#x033-------------------#x039[ #nKEY COMMANDS #x039]#x033-------------------#n
  #Rpsifocus#n         Display current focus and active buffs
  #Rpsimeditate#n      Channel to recover focus (+15 per use)
  #Rpsitrain#n         Display/improve training paths
  #Rmindscan#n         Detect hidden enemies via mental signature
  #Rthoughtshield#n    Absorb damage with mental barrier
  #Rmentallink#n       Share damage with willing ally
  #Rforcepush#n        Telekinetic blast + knockback
  #Rlevitate#n         Float above ground, avoiding some attacks
  #Rkineticbarrier#n   Shield absorbing physical damage
  #Rmindspike#n        Direct mental damage attack
  #Rpsychicscream#n    AoE mental blast, stun chance
  #Rbrainburn#n        Heavy mental DOT, intelligence drain
  #Rpsionarmor#n       Create class equipment (60 primal)

See also: FOCUS, MINDFLAYER, PSION TRAINING
'''
    },
    # Mindflayer class help
    {
        'level': 0,
        'keyword': 'MINDFLAYER MINDFLAYERS',
        'text': '''           #x135}#x035{#n#x035{#n#x035}#x035}#x135{#n  #x035M I N D F L A Y E R#n  #x135}#x035{#n#x035{#n#x035}#x035}#x135{#n

#x250Alien intellects who have transcended mortal limitations. Mindflayers
dominate lesser minds, consume intelligence, and unleash devastating psionic
storms. Their enhanced Focus cap of 150 enables apocalyptic abilities.#n

#x029-------------------#x035[ #nCORE SYSTEMS #x035]#x029-------------------#n
  #nFocus#n           Builds via psimeditate, decays slower (-1/tick)
  #nFocus Cap#n       150 - enables devastating finisher abilities
  #nThralls#n         Dominate up to 3 creatures to fight for you

#x029-------------------#x035[ #nTRAINING PATHS #x035]#x029------------------#n
  #x035Domination#n   Mind control       (enthral, puppet, hivemind, massdomination)
  #x035Consumption#n  Mental draining    (mindfeed, memorydrain, intellectdevour)
  #x035Psionic#n      Raw destruction    (psychicmaelstrom, psiblast, realityfracture)
  Train with #Rmindtrain#n - costs primal to advance each path.

#x029-------------------#x035[ #nKEY COMMANDS #x035]#x029-------------------#n
  #Rpsifocus#n          Display current focus and active effects
  #Rpsimeditate#n       Channel to recover focus (+20 per use)
  #Rmindtrain#n         Display/improve training paths
  #Renthral#n           Dominate creature as permanent thrall (max 3)
  #Rpuppet#n            Force thrall to attack specific target
  #Rhivemind#n          Link with thralls, share damage/coordinate
  #Rmassdomination#n    Attempt to enthral all enemies in room
  #Rmindfeed#n          Drain focus from victim's mind
  #Rmemorydrain#n       Steal victim's mana, add to your focus
  #Rintellectdevour#n   Massive mental damage, Int drain, instakill
  #Rpsychicmaelstrom#n  AoE psychic storm damaging all enemies
  #Rpsiblast#n          Focused mental lance, high single-target damage
  #Rrealityfracture#n   Tear reality with pure thought (100+ focus)
  #Rdismiss#n           Release a thrall from your control
  #Rmindflayerarmor#n   Create class equipment (60 primal)

See also: FOCUS, PSION, MINDFLAYER TRAINING
'''
    },
    # Focus system help
    {
        'level': 0,
        'keyword': 'FOCUS PSIONIC FOCUS',
        'text': '''                     #x039[ #nF O C U S #x039]#n

#x250Focus represents the concentrated mental energy of Psions and Mindflayers.
Unlike mana, Focus is built through meditation and concentration, making it
a resource that must be actively accumulated before combat.#n

#x033-------------------#x039[ #nBUILDING FOCUS #x039]#x033-------------------#n
  #Rpsimeditate#n      +15 focus (Psion) / +20 focus (Mindflayer)
  #nCombat tick#n      +2 focus per tick while fighting
  #nRest tick#n        Focus does not build while resting

#x033-------------------#x039[ #nFOCUS DECAY #x039]#x033-------------------#n
  #nPsion#n            -2 focus per tick when not in combat
  #nMindflayer#n       -1 focus per tick when not in combat
  Focus will not decay below 0.

#x033-------------------#x039[ #nFOCUS CAPS #x039]#x033-------------------#n
  #nPsion#n            Maximum 100 focus
  #nMindflayer#n       Maximum 150 focus (enhanced mental capacity)

#x033-------------------#x039[ #nCOMBAT SCALING #x039]#x033-------------------#n
  Focus adds to damcap:
    Psion:      +6 damcap per point of focus
    Mindflayer: +8 damcap per point of focus
  Many abilities require minimum focus levels to use.

See also: PSION, MINDFLAYER
'''
    },
]


def add_help_entries(db_path: Path) -> None:
    """Add help entries to the database."""
    if not db_path.exists():
        print(f"ERROR: Database not found: {db_path}")
        return

    conn = sqlite3.connect(str(db_path))
    cursor = conn.cursor()

    for entry in HELP_ENTRIES:
        keyword = entry['keyword']

        # Check if entry already exists
        cursor.execute("SELECT id FROM helps WHERE keyword = ?", (keyword,))
        existing = cursor.fetchone()

        if existing:
            print(f"  UPDATE: {keyword}")
            cursor.execute("""
                UPDATE helps SET level = ?, text = ?
                WHERE keyword = ?
            """, (entry['level'], entry['text'], keyword))
        else:
            print(f"  INSERT: {keyword}")
            cursor.execute("""
                INSERT INTO helps (level, keyword, text)
                VALUES (?, ?, ?)
            """, (entry['level'], keyword, entry['text']))

    # Update CLASSES help entry to include Psion
    cursor.execute("SELECT text FROM helps WHERE keyword = 'CLASS CLASSES'")
    row = cursor.fetchone()
    if row:
        classes_text = row[0]
        # Add Psion after Dirgesinger line
        if 'Psion' not in classes_text:
            new_text = classes_text.replace(
                '#x136~#x178[#x178Dirgesinger#x178]#x136~#n       Martial bards wielding the power of warsong.',
                '#x136~#x178[#x178Dirgesinger#x178]#x136~#n       Martial bards wielding the power of warsong.\n#x039<#x033|#x039Psion#x039|#x033>#n             Masters of the mind wielding psychic force.'
            )
            # Add Mindflayer to upgrade classes, after Siren
            new_text = new_text.replace(
                '#x039~#x147(#x147Siren#x147)#x039~#n             Supernatural voices of domination and sonic devastation.',
                '#x039~#x147(#x147Siren#x147)#x039~#n             Supernatural voices of domination and sonic devastation.\n#x035~#x029{#x035Mindflayer#x029}#x035~#n        Alien intellects who dominate minds and consume thought.'
            )
            cursor.execute("UPDATE helps SET text = ? WHERE keyword = 'CLASS CLASSES'", (new_text,))
            print("  UPDATE: CLASS CLASSES (added Psion and Mindflayer)")

    conn.commit()
    conn.close()
    print(f"\nSuccessfully added/updated help entries.")


def main():
    script_dir = Path(__file__).resolve().parent
    db_path = script_dir.parent.parent / 'gamedata' / 'db' / 'game' / 'base_help.db'

    print(f"Adding help entries to: {db_path}")
    add_help_entries(db_path)


if __name__ == '__main__':
    main()
