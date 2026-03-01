#!/usr/bin/env python3
"""
migrate_script_library.py — Centralize duplicated Lua scripts into a library.

Phase A: Extract the 21 unique tick_* script bodies from area databases
         and seed them into the script_library table in tables.db.

Phase B: Add library_name column to each area DB's scripts table, then
         convert matching scripts to library references (library_name set,
         code/trigger/chance cleared to defaults).

Custom one-off scripts (room triggers, speech handlers, etc.) are left
as inline scripts with library_name = NULL.

Usage:
    python migrate_script_library.py [--dry-run]
"""

import sqlite3
import os
import sys

TOOLS_DIR = os.path.dirname(__file__)
AREA_DIR = os.path.join(TOOLS_DIR, '..', '..', 'gamedata', 'db', 'areas')
TABLES_DB = os.path.join(TOOLS_DIR, '..', '..', 'gamedata', 'db', 'game', 'tables.db')

# Trigger bitmask values (must match script.h)
TRIG_TICK = 4  # (1 << 2)

# The 21 library script names (all tick_* scripts from spec_fun migration).
# These are the ones that get deduplicated into the library.
LIBRARY_NAMES = {
    'tick_cast_mage', 'tick_cast_cleric', 'tick_cast_undead',
    'tick_cast_judge', 'tick_cast_adept',
    'tick_breath_fire', 'tick_breath_acid', 'tick_breath_frost',
    'tick_breath_gas', 'tick_breath_lightning', 'tick_breath_any',
    'tick_thief', 'tick_poison', 'tick_guard',
    'tick_fido', 'tick_janitor', 'tick_mayor',
    'tick_executioner', 'tick_rogue', 'tick_eater', 'tick_gremlin',
}


def phase_a_seed_library(dry_run):
    """Extract unique script bodies and seed script_library in tables.db."""
    print("Phase A: Seeding script_library in tables.db")
    print("=" * 60)

    # Collect canonical script entries from area databases.
    # We take the first occurrence of each library name.
    library_entries = {}

    for fn in sorted(os.listdir(AREA_DIR)):
        if not fn.endswith('.db'):
            continue
        conn = sqlite3.connect(os.path.join(AREA_DIR, fn))
        tables = [r[0] for r in conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table'").fetchall()]
        if 'scripts' not in tables:
            conn.close()
            continue

        rows = conn.execute(
            'SELECT name, trigger, code, pattern, chance FROM scripts'
        ).fetchall()

        for name, trigger, code, pattern, chance in rows:
            if name in LIBRARY_NAMES and name not in library_entries:
                library_entries[name] = (name, trigger, code, pattern, chance)

        conn.close()

    if not library_entries:
        print("  No library scripts found in area databases!")
        return 0

    print(f"  Found {len(library_entries)} unique library scripts")

    if dry_run:
        for name in sorted(library_entries):
            print(f"    {name}")
        return len(library_entries)

    # Create table and insert entries
    conn = sqlite3.connect(TABLES_DB)

    conn.execute('''CREATE TABLE IF NOT EXISTS script_library (
        name    TEXT PRIMARY KEY,
        trigger INTEGER NOT NULL,
        code    TEXT NOT NULL,
        pattern TEXT DEFAULT NULL,
        chance  INTEGER DEFAULT 0
    )''')

    # Clear existing entries (idempotent re-run)
    conn.execute('DELETE FROM script_library')

    for name in sorted(library_entries):
        entry = library_entries[name]
        conn.execute(
            'INSERT INTO script_library (name, trigger, code, pattern, chance) '
            'VALUES (?, ?, ?, ?, ?)',
            entry
        )
        print(f"    Inserted: {name}")

    conn.commit()

    # Verify
    count = conn.execute('SELECT COUNT(*) FROM script_library').fetchone()[0]
    print(f"\n  script_library now has {count} entries")
    conn.close()

    return len(library_entries)


def phase_b_convert_areas(dry_run):
    """Add library_name column and convert matching scripts to references."""
    print("\nPhase B: Converting area scripts to library references")
    print("=" * 60)

    total_converted = 0
    total_skipped = 0

    for fn in sorted(os.listdir(AREA_DIR)):
        if not fn.endswith('.db'):
            continue
        db_path = os.path.join(AREA_DIR, fn)
        conn = sqlite3.connect(db_path)

        tables = [r[0] for r in conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table'").fetchall()]
        if 'scripts' not in tables:
            conn.close()
            continue

        # Check if library_name column already exists
        cols = [r[1] for r in conn.execute('PRAGMA table_info(scripts)').fetchall()]
        has_library_col = 'library_name' in cols

        if not has_library_col:
            if not dry_run:
                conn.execute(
                    'ALTER TABLE scripts ADD COLUMN library_name TEXT DEFAULT NULL'
                )
            # print(f"  {fn}: added library_name column")

        # Find scripts that match library names
        if has_library_col:
            rows = conn.execute(
                'SELECT id, name FROM scripts WHERE library_name IS NULL'
            ).fetchall()
        else:
            rows = conn.execute(
                'SELECT id, name FROM scripts'
            ).fetchall()

        converted = 0
        for script_id, name in rows:
            if name not in LIBRARY_NAMES:
                total_skipped += 1
                continue

            if not dry_run:
                conn.execute(
                    'UPDATE scripts SET library_name = ?, code = \'\', '
                    'trigger = 0, chance = 0, pattern = NULL '
                    'WHERE id = ?',
                    (name, script_id)
                )
            converted += 1

        if not dry_run and (converted > 0 or not has_library_col):
            conn.commit()

        if converted > 0:
            action = 'Would convert' if dry_run else 'Converted'
            print(f"  {fn}: {action} {converted} scripts to library references")
            total_converted += converted

        conn.close()

    print(f"\n  Total: {total_converted} scripts converted, "
          f"{total_skipped} custom scripts left inline")
    return total_converted


def main():
    dry_run = '--dry-run' in sys.argv

    if dry_run:
        print("DRY RUN — no changes will be made\n")

    seeded = phase_a_seed_library(dry_run)
    converted = phase_b_convert_areas(dry_run)

    print(f"\nDone: {seeded} library entries seeded, "
          f"{converted} area scripts converted to references.")


if __name__ == '__main__':
    main()
