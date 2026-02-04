#!/usr/bin/env python3
"""
Migrate ability config defaults from C source to SQLite.

Parses game/src/core/ability_config.c and extracts the acfg_table[]
entries, then inserts them into gamedata/db/game/game.db.

Usage:
    python migrate_ability_config.py
"""

import re
import sqlite3
from pathlib import Path


def parse_ability_config(source_path: Path) -> list[tuple[str, int]]:
    """Parse acfg_table[] from ability_config.c, return list of (key, value)."""
    content = source_path.read_text(encoding='utf-8')

    # Match: { "key", value, default_value }
    # We use default_value (3rd field) as the initial database value
    pattern = r'\{\s*"([^"]+)"\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}'

    entries = []
    for match in re.finditer(pattern, content):
        key = match.group(1)
        default_value = int(match.group(3))  # Use default_value column
        entries.append((key, default_value))

    return entries


def migrate(game_db: Path, entries: list[tuple[str, int]]):
    """Insert entries into ability_config table."""
    conn = sqlite3.connect(game_db)
    cursor = conn.cursor()

    # Clear existing entries
    cursor.execute("DELETE FROM ability_config")

    # Insert all entries
    cursor.executemany(
        "INSERT INTO ability_config (key, value) VALUES (?, ?)",
        entries
    )

    conn.commit()
    print(f"Inserted {len(entries)} ability config entries")
    conn.close()


def main():
    script_dir = Path(__file__).parent
    source_file = script_dir.parent / 'src' / 'core' / 'ability_config.c'
    game_db = script_dir.parent.parent / 'gamedata' / 'db' / 'game' / 'game.db'

    if not source_file.exists():
        print(f"ERROR: {source_file} not found")
        return 1

    if not game_db.exists():
        print(f"ERROR: {game_db} not found")
        return 1

    entries = parse_ability_config(source_file)
    print(f"Parsed {len(entries)} entries from {source_file.name}")

    migrate(game_db, entries)
    return 0


if __name__ == '__main__':
    exit(main())
