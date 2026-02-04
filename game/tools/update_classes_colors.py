#!/usr/bin/env python3
"""
Update CLASSES help entry with new Psion/Mindflayer color schemes.
"""

import sqlite3
from pathlib import Path

def main():
    script_dir = Path(__file__).resolve().parent
    db_path = script_dir.parent.parent / 'gamedata' / 'db' / 'game' / 'base_help.db'

    print(f"Updating: {db_path}")

    conn = sqlite3.connect(str(db_path))
    c = conn.cursor()

    c.execute("SELECT text FROM helps WHERE keyword = ?", ('CLASS CLASSES',))
    row = c.fetchone()

    if not row:
        print("ERROR: CLASS CLASSES help entry not found!")
        conn.close()
        return

    text = row[0]

    # Update Psion colors (bright white chevrons, blue pipes, BRIGHT CYAN name)
    old_psion = '#W<#x033|#CPsion#x033|#W>'
    new_psion = '#x255<#x033|#CPsion#x033|#x255>'

    # Update Mindflayer colors (magenta outer, teal inner, BRIGHT GREEN name)
    old_mind = '#P}#x035{#GMindflayer#x035}#P{'
    new_mind = '#x135}#x035{#GMindflayer#x035}#x135{'

    # Update Siren colors (bright white outer, cyan inner, bright white name)
    old_siren = '#W)#x147(#WSiren#x147)#W('
    new_siren = '#x255)#x147(#x255Siren#x147)#x255('

    print(f"Psion pattern found: {old_psion in text}")
    print(f"Mindflayer pattern found: {old_mind in text}")
    print(f"Siren pattern found: {old_siren in text}")

    if old_psion in text:
        text = text.replace(old_psion, new_psion)
        print("  Replaced Psion colors")

    if old_mind in text:
        text = text.replace(old_mind, new_mind)
        print("  Replaced Mindflayer colors")

    if old_siren in text:
        text = text.replace(old_siren, new_siren)
        print("  Replaced Siren colors")

    c.execute("UPDATE helps SET text = ? WHERE keyword = ?", (text, 'CLASS CLASSES'))
    conn.commit()
    conn.close()

    print("Done!")

if __name__ == '__main__':
    main()
