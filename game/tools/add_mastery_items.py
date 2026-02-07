#!/usr/bin/env python3
"""
add_mastery_items.py - Add mastery items for Dragonkin/Wyrm classes
"""

import sqlite3
from pathlib import Path

# Mastery item vnums
DRAGONKIN_MASTERY_VNUM = 33415
WYRM_MASTERY_VNUM = 33435

CLASS_DRAGONKIN = 262144
CLASS_WYRM = 524288

# Wear flags
WEAR_TAKE = 1
WEAR_HOLD = 512

# Apply locations
APPLY_HITROLL = 18
APPLY_DAMROLL = 19

def main():
    script_dir = Path(__file__).parent

    # Add mastery items to classeq.db
    classeq_db = script_dir.parent.parent / "gamedata" / "db" / "areas" / "classeq.db"
    print(f"Using classeq.db: {classeq_db}")

    conn = sqlite3.connect(str(classeq_db))
    cursor = conn.cursor()

    # Dragonkin Mastery: Dragon Heart Shard
    cursor.execute('''INSERT OR REPLACE INTO objects
        (vnum, name, short_descr, description, item_type, extra_flags, wear_flags,
         value0, value1, value2, value3, weight, cost)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)''',
        (DRAGONKIN_MASTERY_VNUM, 'dragonkin heart shard dragon',
         '#x202<*#x220a pulsing dragon heart shard#x202*>#n',
         'A shard of crystallized dragon essence pulses with power here.',
         9, 0, WEAR_TAKE | WEAR_HOLD, 25, 0, 0, 0, 1, 0))

    cursor.execute('DELETE FROM object_affects WHERE obj_vnum = ?', (DRAGONKIN_MASTERY_VNUM,))
    cursor.execute('INSERT INTO object_affects (obj_vnum, location, modifier, sort_order) VALUES (?, ?, ?, ?)',
        (DRAGONKIN_MASTERY_VNUM, APPLY_HITROLL, 50, 0))
    cursor.execute('INSERT INTO object_affects (obj_vnum, location, modifier, sort_order) VALUES (?, ?, ?, ?)',
        (DRAGONKIN_MASTERY_VNUM, APPLY_DAMROLL, 50, 1))

    # Wyrm Mastery: Primordial Dragon Core
    cursor.execute('''INSERT OR REPLACE INTO objects
        (vnum, name, short_descr, description, item_type, extra_flags, wear_flags,
         value0, value1, value2, value3, weight, cost)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)''',
        (WYRM_MASTERY_VNUM, 'wyrm primordial dragon core',
         '#x202<#x220*a primordial dragon core*#x202>#n',
         'An ancient core of primordial dragon essence radiates immense power.',
         9, 0, WEAR_TAKE | WEAR_HOLD, 25, 0, 0, 0, 1, 0))

    cursor.execute('DELETE FROM object_affects WHERE obj_vnum = ?', (WYRM_MASTERY_VNUM,))
    cursor.execute('INSERT INTO object_affects (obj_vnum, location, modifier, sort_order) VALUES (?, ?, ?, ?)',
        (WYRM_MASTERY_VNUM, APPLY_HITROLL, 75, 0))
    cursor.execute('INSERT INTO object_affects (obj_vnum, location, modifier, sort_order) VALUES (?, ?, ?, ?)',
        (WYRM_MASTERY_VNUM, APPLY_DAMROLL, 75, 1))

    conn.commit()
    conn.close()
    print('Added mastery items to classeq.db')
    print(f'  Dragonkin: {DRAGONKIN_MASTERY_VNUM} - Dragon Heart Shard (+50/+50)')
    print(f'  Wyrm: {WYRM_MASTERY_VNUM} - Primordial Dragon Core (+75/+75)')

    # Update class.db with mastery vnums
    class_db = script_dir.parent.parent / "gamedata" / "db" / "game" / "class.db"
    print(f"Using class.db: {class_db}")

    conn = sqlite3.connect(str(class_db))
    cursor = conn.cursor()

    cursor.execute('UPDATE class_armor_config SET mastery_vnum = ? WHERE class_id = ?',
        (DRAGONKIN_MASTERY_VNUM, CLASS_DRAGONKIN))
    cursor.execute('UPDATE class_armor_config SET mastery_vnum = ? WHERE class_id = ?',
        (WYRM_MASTERY_VNUM, CLASS_WYRM))

    conn.commit()
    conn.close()
    print('Updated class.db mastery_vnum values')
    print('Done!')

if __name__ == "__main__":
    main()
