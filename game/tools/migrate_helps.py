#!/usr/bin/env python3
"""
Migrate help.are into gamedata/db/game/help.db (SQLite).

Parses the Diku/Merc #HELPS format from help.are and inserts each entry
into the helps table.

Usage:
    python migrate_helps.py
"""

import sqlite3
import sys
from pathlib import Path


HELP_SCHEMA = """
CREATE TABLE IF NOT EXISTS helps (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    level      INTEGER NOT NULL DEFAULT 0,
    keyword    TEXT NOT NULL,
    text       TEXT NOT NULL DEFAULT ''
);
"""


def parse_help_are(filepath):
    """Parse a Diku/Merc #HELPS section from a .are file.

    Format:
        #HELPS
        <level> <keyword>~
        <text>~
        ...
        0 $~
    """
    helps = []
    with open(filepath, 'r', encoding='latin-1') as f:
        content = f.read()

    # Find the #HELPS section
    idx = content.find('#HELPS')
    if idx == -1:
        print("No #HELPS section found.")
        return helps

    # Skip past #HELPS and any whitespace
    idx += len('#HELPS')
    while idx < len(content) and content[idx] in '\r\n ':
        idx += 1

    while idx < len(content):
        # Read level number
        level_str = ''
        while idx < len(content) and content[idx] in ' \t\r\n':
            idx += 1
        while idx < len(content) and (content[idx].isdigit() or content[idx] == '-'):
            level_str += content[idx]
            idx += 1
        if not level_str:
            break
        level = int(level_str)

        # Skip whitespace between level and keyword
        while idx < len(content) and content[idx] in ' \t':
            idx += 1

        # Read keyword until ~
        keyword = ''
        while idx < len(content) and content[idx] != '~':
            keyword += content[idx]
            idx += 1
        keyword = keyword.strip()
        idx += 1  # skip ~

        # Check for end marker
        if keyword == '$':
            break

        # Skip newline after ~
        while idx < len(content) and content[idx] in '\r\n':
            idx += 1

        # Read text until ~
        text = ''
        while idx < len(content) and content[idx] != '~':
            text += content[idx]
            idx += 1
        idx += 1  # skip ~

        # Skip trailing whitespace
        while idx < len(content) and content[idx] in '\r\n ':
            idx += 1

        # Normalize line endings
        text = text.replace('\r\n', '\n').replace('\r', '\n')

        helps.append((level, keyword, text))

    return helps


def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    help_are = repo_root / 'gamedata' / 'area' / 'help.are'
    db_dir = repo_root / 'gamedata' / 'db' / 'game'

    if not help_are.exists():
        print(f"Error: {help_are} not found.")
        sys.exit(1)

    # Create game/ directory
    db_dir.mkdir(parents=True, exist_ok=True)
    db_path = db_dir / 'help.db'

    # Parse help.are
    helps = parse_help_are(str(help_are))
    if not helps:
        print("No help entries found in help.are.")
        return

    print(f"Parsed {len(helps)} help entries from help.are")

    # Create/open help.db
    conn = sqlite3.connect(str(db_path))
    conn.execute(HELP_SCHEMA)

    # Check if table already has data
    count = conn.execute("SELECT COUNT(*) FROM helps").fetchone()[0]
    if count > 0:
        print(f"help.db already has {count} entries. Clearing and re-importing.")
        conn.execute("DELETE FROM helps")

    # Insert all helps
    conn.execute("BEGIN TRANSACTION")
    for level, keyword, text in helps:
        conn.execute(
            "INSERT INTO helps (level, keyword, text) VALUES (?, ?, ?)",
            (level, keyword, text)
        )
    conn.commit()
    conn.close()

    print(f"Migrated {len(helps)} help entries to {db_path}")

    # Show a few examples
    for i, (level, keyword, text) in enumerate(helps[:5]):
        preview = text[:60].replace('\n', '\\n')
        print(f"  [{i+1}] level={level} keyword={keyword!r} text={preview}...")
    if len(helps) > 5:
        print(f"  ... and {len(helps) - 5} more")


if __name__ == '__main__':
    main()
