#!/usr/bin/env python3
"""
Migrate hidden.lst into per-area .db files (sets is_hidden column).

Reads gamedata/area/hidden.lst and updates the corresponding .db files
in gamedata/db/areas/ to set is_hidden = 1.

Usage:
    python migrate_hidden.py
"""

import sqlite3
import sys
from pathlib import Path


def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    hidden_lst = repo_root / 'gamedata' / 'area' / 'hidden.lst'
    db_dir = repo_root / 'gamedata' / 'db' / 'areas'

    if not hidden_lst.exists():
        print("No hidden.lst found, nothing to migrate.")
        return

    if not db_dir.exists():
        print(f"Error: db directory not found: {db_dir}")
        sys.exit(1)

    # Read hidden area filenames
    hidden_areas = []
    with open(hidden_lst, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('$'):
                break
            hidden_areas.append(line)

    if not hidden_areas:
        print("No hidden areas found in hidden.lst.")
        return

    print(f"Found {len(hidden_areas)} hidden areas: {', '.join(hidden_areas)}")

    # First, add is_hidden column to all .db files that don't have it yet
    for db_file in sorted(db_dir.glob('*.db')):
        conn = sqlite3.connect(str(db_file))
        # Check if column exists
        cursor = conn.execute("PRAGMA table_info(area)")
        columns = [row[1] for row in cursor.fetchall()]
        if 'is_hidden' not in columns:
            conn.execute("ALTER TABLE area ADD COLUMN is_hidden INTEGER NOT NULL DEFAULT 0")
            conn.commit()
        conn.close()

    # Set is_hidden = 1 for hidden areas
    updated = 0
    for area_filename in hidden_areas:
        # area_filename is like "kavir.are", db file is "kavir.db"
        stem = Path(area_filename).stem
        db_path = db_dir / f"{stem}.db"

        if not db_path.exists():
            print(f"  SKIP  {area_filename} (no .db file)")
            continue

        conn = sqlite3.connect(str(db_path))
        conn.execute("UPDATE area SET is_hidden = 1")
        conn.commit()
        conn.close()
        print(f"  OK    {area_filename} -> {stem}.db is_hidden=1")
        updated += 1

    print(f"\nMigrated {updated} hidden area flags.")


if __name__ == '__main__':
    main()
