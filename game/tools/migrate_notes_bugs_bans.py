#!/usr/bin/env python3
"""
Migrate board notes, bug reports, bans, and disabled commands from text files
to SQLite (game.db).

Reads from:
  - gamedata/notes/<board_name>  (board notes)
  - gamedata/txt/bugs.txt        (bug reports)
  - gamedata/txt/ban.txt          (site bans)
  - gamedata/disabled.txt         (disabled commands)

Writes to:
  - gamedata/db/game/game.db      (notes, bugs, bans, disabled_commands tables)

Usage:
    python migrate_notes_bugs_bans.py [--gamedata PATH]
"""

import argparse
import os
import re
import sqlite3
import sys
from pathlib import Path


# Board names in order (must match boards[] array in board.c)
BOARD_NAMES = [
    "General", "Ideas", "Announce", "Bugs",
    "Personal", "Immortal", "Builder", "Kingdom"
]

# Schema for tables (created if not exists)
SCHEMA_SQL = """
CREATE TABLE IF NOT EXISTS notes (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  board_idx  INTEGER NOT NULL,
  sender     TEXT NOT NULL,
  date       TEXT NOT NULL,
  date_stamp INTEGER NOT NULL,
  expire     INTEGER NOT NULL,
  to_list    TEXT NOT NULL,
  subject    TEXT NOT NULL,
  text       TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS bugs (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  room_vnum  INTEGER NOT NULL DEFAULT 0,
  player     TEXT NOT NULL DEFAULT '',
  message    TEXT NOT NULL,
  timestamp  INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS bans (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  name       TEXT NOT NULL,
  reason     TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS disabled_commands (
  command_name TEXT PRIMARY KEY,
  level        INTEGER NOT NULL,
  disabled_by  TEXT NOT NULL
);
"""


def parse_note_file(filepath):
    """Parse a board note file, yielding (sender, date, stamp, expire, to, subject, text).

    File format:
        Sender  <name>~
        Date    <date string>~
        Stamp   <integer>
        Expire  <integer>
        To      <recipients>~
        Subject <subject>~
        Text
        <body>~
    """
    if not os.path.exists(filepath):
        return

    with open(filepath, 'r', encoding='latin-1') as fp:
        content = fp.read()

    # Split on tilde-terminated note boundaries by parsing sequentially
    pos = 0
    length = len(content)

    def skip_whitespace():
        nonlocal pos
        while pos < length and content[pos] in ' \t\r\n':
            pos += 1

    def read_line():
        nonlocal pos
        end = content.find('\n', pos)
        if end == -1:
            line = content[pos:]
            pos = length
        else:
            line = content[pos:end]
            pos = end + 1
        return line.strip()

    def read_tilde_field(line_content):
        """Extract value from 'Keyword  value~' format."""
        parts = line_content.split(None, 1)
        val = parts[1] if len(parts) > 1 else ''
        if val.endswith('~'):
            val = val[:-1]
        return val

    def read_tilde_text():
        """Read multi-line text until ~ delimiter."""
        nonlocal pos
        tilde_pos = content.find('~', pos)
        if tilde_pos == -1:
            text = content[pos:]
            pos = length
        else:
            text = content[pos:tilde_pos]
            pos = tilde_pos + 1
            # Skip past newline after tilde
            if pos < length and content[pos] == '\n':
                pos += 1
        return text

    while pos < length:
        skip_whitespace()
        if pos >= length:
            break

        line = read_line()
        if not line.lower().startswith('sender'):
            break
        sender = read_tilde_field(line)

        line = read_line()
        if not line.lower().startswith('date'):
            break
        date = read_tilde_field(line)

        line = read_line()
        if not line.lower().startswith('stamp'):
            break
        stamp = int(line.split(None, 1)[1])

        line = read_line()
        if not line.lower().startswith('expire'):
            break
        expire = int(line.split(None, 1)[1])

        line = read_line()
        if not line.lower().startswith('to'):
            break
        to_list = read_tilde_field(line)

        line = read_line()
        if not line.lower().startswith('subject'):
            break
        subject = read_tilde_field(line)

        line = read_line()
        if not line.lower().startswith('text'):
            break
        text = read_tilde_text()

        yield (sender, date, stamp, expire, to_list, subject, text)


def parse_bugs_file(filepath):
    """Parse bugs.txt, yielding (room_vnum, player, message, timestamp)."""
    if not os.path.exists(filepath):
        return

    # Format: [VNUM] NAME: MESSAGE
    # or:     [*****] BUG: MESSAGE (system bugs)
    pattern = re.compile(r'^\[([*\d\s]+)\]\s+(\S+):\s*(.*)')

    with open(filepath, 'r', encoding='latin-1') as fp:
        for line in fp:
            line = line.rstrip('\n')
            m = pattern.match(line)
            if m:
                vnum_str = m.group(1).strip()
                player = m.group(2)
                message = m.group(3)

                if '*' in vnum_str:
                    vnum = 0
                    player = "SYSTEM"
                    # Reconstruct full message for system bugs
                    message = line
                else:
                    try:
                        vnum = int(vnum_str)
                    except ValueError:
                        vnum = 0

                yield (vnum, player, message, 0)


def parse_ban_file(filepath):
    """Parse ban.txt, yielding (name, reason).

    Format: name\\nreason~\\n ... END\\n
    """
    if not os.path.exists(filepath):
        return

    with open(filepath, 'r', encoding='latin-1') as fp:
        content = fp.read()

    pos = 0
    length = len(content)

    while pos < length:
        # Read name line
        end = content.find('\n', pos)
        if end == -1:
            break
        name = content[pos:end].strip()
        pos = end + 1
        if not name or name == 'END':
            break

        # Read reason (tilde-terminated)
        tilde_pos = content.find('~', pos)
        if tilde_pos == -1:
            reason = content[pos:].strip()
            pos = length
        else:
            reason = content[pos:tilde_pos].strip()
            pos = tilde_pos + 1
            if pos < length and content[pos] == '\n':
                pos += 1

        yield (name, reason)


def parse_disabled_file(filepath):
    """Parse disabled.txt, yielding (command_name, level, disabled_by)."""
    if not os.path.exists(filepath):
        return

    with open(filepath, 'r', encoding='latin-1') as fp:
        for line in fp:
            line = line.strip()
            if not line or line == 'END':
                return
            parts = line.split()
            if len(parts) >= 3:
                yield (parts[0], int(parts[1]), parts[2])


def main():
    parser = argparse.ArgumentParser(description='Migrate text data to game.db')
    parser.add_argument('--gamedata', default=None,
                        help='Path to gamedata/ directory')
    args = parser.parse_args()

    if args.gamedata:
        gamedata = Path(args.gamedata)
    else:
        # Default: relative to this script
        gamedata = Path(__file__).resolve().parent.parent.parent / 'gamedata'

    db_path = gamedata / 'db' / 'game' / 'game.db'
    notes_dir = gamedata / 'notes'
    txt_dir = gamedata / 'txt'
    disabled_path = gamedata / 'disabled.txt'

    if not db_path.parent.exists():
        db_path.parent.mkdir(parents=True)

    print(f"Opening database: {db_path}")
    conn = sqlite3.connect(str(db_path))
    conn.executescript(SCHEMA_SQL)

    # Migrate notes
    total_notes = 0
    for idx, board_name in enumerate(BOARD_NAMES):
        filepath = notes_dir / board_name
        notes = list(parse_note_file(str(filepath)))
        if notes:
            conn.executemany(
                "INSERT INTO notes (board_idx, sender, date, date_stamp, expire, "
                "to_list, subject, text) VALUES (?,?,?,?,?,?,?,?)",
                [(idx, n[0], n[1], n[2], n[3], n[4], n[5], n[6]) for n in notes]
            )
            print(f"  Board {idx} ({board_name}): {len(notes)} notes")
            total_notes += len(notes)

        # Also try .old archive file
        filepath_old = notes_dir / f"{board_name}.old"
        notes_old = list(parse_note_file(str(filepath_old)))
        if notes_old:
            print(f"  Board {idx} ({board_name}.old): {len(notes_old)} archived notes (skipped)")

    print(f"  Total notes migrated: {total_notes}")

    # Migrate bugs
    bugs_path = txt_dir / 'bugs.txt'
    bugs = list(parse_bugs_file(str(bugs_path)))
    if bugs:
        conn.executemany(
            "INSERT INTO bugs (room_vnum, player, message, timestamp) VALUES (?,?,?,?)",
            bugs
        )
    print(f"  Bugs migrated: {len(bugs)}")

    # Migrate bans
    ban_path = txt_dir / 'ban.txt'
    bans = list(parse_ban_file(str(ban_path)))
    if bans:
        conn.executemany(
            "INSERT INTO bans (name, reason) VALUES (?,?)",
            bans
        )
    print(f"  Bans migrated: {len(bans)}")

    # Migrate disabled commands
    disabled = list(parse_disabled_file(str(disabled_path)))
    if disabled:
        conn.executemany(
            "INSERT OR REPLACE INTO disabled_commands (command_name, level, disabled_by) "
            "VALUES (?,?,?)",
            disabled
        )
    print(f"  Disabled commands migrated: {len(disabled)}")

    conn.commit()
    conn.close()
    print("Migration complete.")


if __name__ == '__main__':
    main()
