#!/usr/bin/env python3
"""
seed_tables_db.py - Extract hardcoded C data tables into tables.db

Parses C source files to extract social_table, slay_table, liq_table,
where_name/where_name_sr, and day_name/month_name arrays, then creates
(or recreates) gamedata/db/game/tables.db with that data.

Usage:
    python game/tools/seed_tables_db.py

Run from the repository root.  Idempotent -- safe to re-run.
"""

import os
import re
import sqlite3
import sys
from pathlib import Path


def find_repo_root() -> Path:
    """Find repository root relative to this script."""
    # Script is at game/tools/seed_tables_db.py
    return Path(__file__).resolve().parent.parent.parent


def parse_socials(src_dir: Path) -> list:
    """Parse social_table from socials.c."""
    path = src_dir / "commands" / "socials.c"
    text = path.read_text(encoding="utf-8", errors="replace")

    # Find the social_table array
    match = re.search(r'social_table\[\]\s*=\s*\{', text)
    if not match:
        print("ERROR: Could not find social_table in socials.c")
        return []

    # Extract entries -- each is { "name", "msg1", "msg2", ... }
    # We need to handle NULL values and multi-line entries
    socials = []
    pos = match.end()
    brace_depth = 1  # We're inside the outer {

    while brace_depth > 0 and pos < len(text):
        # Skip whitespace
        while pos < len(text) and text[pos] in ' \t\n\r':
            pos += 1

        if pos >= len(text):
            break

        if text[pos] == '}':
            brace_depth -= 1
            pos += 1
            continue

        if text[pos] == '{':
            # Start of an entry - collect until matching }
            entry_start = pos + 1
            depth = 1
            pos += 1
            while depth > 0 and pos < len(text):
                if text[pos] == '{':
                    depth += 1
                elif text[pos] == '}':
                    depth -= 1
                pos += 1
            entry_text = text[entry_start:pos - 1]

            # Parse the entry: comma-separated values that are either
            # "string literal" (possibly spanning multiple lines) or NULL
            fields = _parse_social_entry(entry_text)
            if fields and len(fields) == 8 and fields[0]:
                # Skip sentinel entry (empty name)
                if fields[0].strip():
                    socials.append(fields)
            continue

        # Skip commas and other characters between entries
        pos += 1

    return socials


def _parse_social_entry(entry_text: str) -> list:
    """Parse a single social entry into a list of 8 field values."""
    fields = []
    pos = 0
    text = entry_text.strip()

    while pos < len(text) and len(fields) < 8:
        # Skip whitespace and commas
        while pos < len(text) and text[pos] in ' \t\n\r,':
            pos += 1

        if pos >= len(text):
            break

        if text[pos:pos + 4] == 'NULL':
            fields.append(None)
            pos += 4
            continue

        if text[pos] == '"':
            # Collect possibly-concatenated string literals
            full_string = ""
            while pos < len(text) and text[pos] == '"':
                # Find the end of this string literal
                pos += 1  # skip opening "
                literal = ""
                while pos < len(text) and text[pos] != '"':
                    if text[pos] == '\\' and pos + 1 < len(text):
                        next_ch = text[pos + 1]
                        if next_ch == 'n':
                            literal += '\n'
                        elif next_ch == 'r':
                            literal += '\r'
                        elif next_ch == 't':
                            literal += '\t'
                        elif next_ch == '"':
                            literal += '"'
                        elif next_ch == '\\':
                            literal += '\\'
                        else:
                            literal += next_ch
                        pos += 2
                    else:
                        literal += text[pos]
                        pos += 1
                pos += 1  # skip closing "
                full_string += literal

                # Skip whitespace between concatenated strings
                while pos < len(text) and text[pos] in ' \t\n\r':
                    pos += 1

                # Check if next non-whitespace is another string (C concat)
                # If it's a comma or }, this string is done
                if pos < len(text) and text[pos] != '"':
                    break

            fields.append(full_string)
            continue

        # Unknown token - skip
        pos += 1

    return fields


def parse_slays(src_dir: Path) -> list:
    """Parse slay_table from const.c."""
    path = src_dir / "core" / "const.c"
    text = path.read_text(encoding="utf-8", errors="replace")

    match = re.search(r'slay_table\[.*?\]\s*=\s*\{', text)
    if not match:
        print("ERROR: Could not find slay_table in const.c")
        return []

    slays = []
    pos = match.end()
    brace_depth = 1

    while brace_depth > 0 and pos < len(text):
        while pos < len(text) and text[pos] in ' \t\n\r':
            pos += 1

        if pos >= len(text):
            break

        if text[pos] == '}':
            brace_depth -= 1
            pos += 1
            if brace_depth == 0:
                break
            continue

        if text[pos] == '{':
            entry_start = pos + 1
            depth = 1
            pos += 1
            while depth > 0 and pos < len(text):
                if text[pos] == '{':
                    depth += 1
                elif text[pos] == '}':
                    depth -= 1
                pos += 1
            entry_text = text[entry_start:pos - 1]

            fields = _parse_social_entry(entry_text)  # Same string parsing
            if fields and len(fields) >= 5:
                slays.append({
                    'owner': fields[0] or "",
                    'title': fields[1] or "",
                    'char_msg': fields[2] or "",
                    'vict_msg': fields[3] or "",
                    'room_msg': fields[4] or "",
                })
            continue

        pos += 1

    return slays


def parse_liquids(src_dir: Path) -> list:
    """Parse liq_table from const.c."""
    path = src_dir / "core" / "const.c"
    text = path.read_text(encoding="utf-8", errors="replace")

    match = re.search(r'liq_table\[.*?\]\s*=\s*\{', text)
    if not match:
        print("ERROR: Could not find liq_table in const.c")
        return []

    liquids = []
    # Match entries like: { "water", "clear", { 0, 1, 10 } }
    pattern = re.compile(
        r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*\{\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}\s*\}',
        re.DOTALL
    )
    for m in pattern.finditer(text, match.end()):
        liquids.append({
            'name': m.group(1),
            'color': m.group(2),
            'proof': int(m.group(3)),
            'full': int(m.group(4)),
            'thirst': int(m.group(5)),
        })
        if len(liquids) >= 16:
            break

    return liquids


def parse_wear_locations(src_dir: Path) -> list:
    """Parse where_name[] and where_name_sr[] from act_info.c."""
    path = src_dir / "commands" / "act_info.c"
    text = path.read_text(encoding="utf-8", errors="replace")

    def extract_string_array(array_name):
        pattern = re.compile(
            r'' + re.escape(array_name) + r'\[\]\s*=\s*\{(.*?)\};',
            re.DOTALL
        )
        m = pattern.search(text)
        if not m:
            return []
        body = m.group(1)
        strings = re.findall(r'"((?:[^"\\]|\\.)*)"', body)
        return strings

    display = extract_string_array('where_name')
    sr = extract_string_array('where_name_sr')

    # Wear slot names for reference (from merc.h WEAR_* defines)
    slot_names = [
        "light", "finger_l", "finger_r", "neck_1", "neck_2",
        "body", "head", "legs", "feet", "hands",
        "arms", "shield", "about", "waist", "wrist_l",
        "wrist_r", "wield", "hold", "third", "fourth",
        "face", "scabbard_l", "scabbard_r", "special", "float",
        "medal", "bodyart"
    ]

    locations = []
    count = min(len(display), len(sr), len(slot_names))
    for i in range(count):
        locations.append({
            'slot_id': i,
            'slot_name': slot_names[i],
            'display_text': display[i],
            'sr_text': sr[i],
        })

    return locations


def parse_calendar(src_dir: Path) -> tuple:
    """Parse day_name[] and month_name[] from act_info.c."""
    path = src_dir / "commands" / "act_info.c"
    text = path.read_text(encoding="utf-8", errors="replace")

    def extract_string_array(array_name):
        pattern = re.compile(
            r'' + re.escape(array_name) + r'\[\]\s*=\s*\{(.*?)\};',
            re.DOTALL
        )
        m = pattern.search(text)
        if not m:
            return []
        body = m.group(1)
        return re.findall(r'"((?:[^"\\]|\\.)*)"', body)

    days = extract_string_array('day_name')
    months = extract_string_array('month_name')
    return days, months


def create_database(db_path: Path, socials, slays, liquids, wear_locs, days, months):
    """Create tables.db and populate all tables."""
    # Ensure parent directory exists
    db_path.parent.mkdir(parents=True, exist_ok=True)

    # Remove existing database for clean recreation
    if db_path.exists():
        db_path.unlink()

    conn = sqlite3.connect(str(db_path))
    cur = conn.cursor()

    # --- Socials ---
    cur.execute("""
        CREATE TABLE socials (
            name          TEXT PRIMARY KEY,
            char_no_arg   TEXT,
            others_no_arg TEXT,
            char_found    TEXT,
            others_found  TEXT,
            vict_found    TEXT,
            char_auto     TEXT,
            others_auto   TEXT
        )
    """)

    for s in socials:
        cur.execute(
            "INSERT INTO socials VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
            (s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7])
        )
    print(f"  socials: {len(socials)} entries")

    # --- Slays ---
    cur.execute("""
        CREATE TABLE slays (
            id        INTEGER PRIMARY KEY,
            owner     TEXT NOT NULL DEFAULT '',
            title     TEXT NOT NULL,
            char_msg  TEXT NOT NULL,
            vict_msg  TEXT NOT NULL,
            room_msg  TEXT NOT NULL
        )
    """)

    for i, s in enumerate(slays):
        cur.execute(
            "INSERT INTO slays VALUES (?, ?, ?, ?, ?, ?)",
            (i, s['owner'], s['title'], s['char_msg'], s['vict_msg'], s['room_msg'])
        )
    print(f"  slays: {len(slays)} entries")

    # --- Liquids ---
    cur.execute("""
        CREATE TABLE liquids (
            id           INTEGER PRIMARY KEY,
            name         TEXT NOT NULL,
            color        TEXT NOT NULL,
            proof        INTEGER NOT NULL DEFAULT 0,
            full_effect  INTEGER NOT NULL DEFAULT 0,
            thirst       INTEGER NOT NULL DEFAULT 0
        )
    """)

    for i, liq in enumerate(liquids):
        cur.execute(
            "INSERT INTO liquids VALUES (?, ?, ?, ?, ?, ?)",
            (i, liq['name'], liq['color'], liq['proof'], liq['full'], liq['thirst'])
        )
    print(f"  liquids: {len(liquids)} entries")

    # --- Wear Locations ---
    cur.execute("""
        CREATE TABLE wear_locations (
            slot_id      INTEGER PRIMARY KEY,
            slot_name    TEXT NOT NULL,
            display_text TEXT NOT NULL,
            sr_text      TEXT NOT NULL
        )
    """)

    for loc in wear_locs:
        cur.execute(
            "INSERT INTO wear_locations VALUES (?, ?, ?, ?)",
            (loc['slot_id'], loc['slot_name'], loc['display_text'], loc['sr_text'])
        )
    print(f"  wear_locations: {len(wear_locs)} entries")

    # --- Calendar ---
    cur.execute("""
        CREATE TABLE calendar (
            type  TEXT NOT NULL,
            idx   INTEGER NOT NULL,
            name  TEXT NOT NULL,
            PRIMARY KEY (type, idx)
        )
    """)

    for i, name in enumerate(days):
        cur.execute("INSERT INTO calendar VALUES ('day', ?, ?)", (i, name))
    for i, name in enumerate(months):
        cur.execute("INSERT INTO calendar VALUES ('month', ?, ?)", (i, name))
    print(f"  calendar: {len(days)} days, {len(months)} months")

    conn.commit()
    conn.close()


def main():
    repo_root = find_repo_root()
    src_dir = repo_root / "game" / "src"
    db_path = repo_root / "gamedata" / "db" / "game" / "tables.db"

    print(f"Repository root: {repo_root}")
    print(f"Source directory: {src_dir}")
    print(f"Output database: {db_path}")
    print()

    # Verify source directory exists
    if not src_dir.exists():
        print(f"ERROR: Source directory not found: {src_dir}")
        sys.exit(1)

    print("Parsing C source files...")
    socials = parse_socials(src_dir)
    slays = parse_slays(src_dir)
    liquids = parse_liquids(src_dir)
    wear_locs = parse_wear_locations(src_dir)
    days, months = parse_calendar(src_dir)

    print(f"\nExtracted:")
    print(f"  {len(socials)} socials")
    print(f"  {len(slays)} slays")
    print(f"  {len(liquids)} liquids")
    print(f"  {len(wear_locs)} wear locations")
    print(f"  {len(days)} days, {len(months)} months")

    # Validate
    if len(socials) < 10:
        print("WARNING: Fewer than 10 socials parsed - check parser")
    if len(slays) < 1:
        print("WARNING: No slays parsed - check parser")
    if len(liquids) != 16:
        print(f"WARNING: Expected 16 liquids, got {len(liquids)}")
    if len(wear_locs) != 27:
        print(f"WARNING: Expected 27 wear locations, got {len(wear_locs)}")
    if len(days) != 7:
        print(f"WARNING: Expected 7 days, got {len(days)}")
    if len(months) != 17:
        print(f"WARNING: Expected 17 months, got {len(months)}")

    print(f"\nCreating {db_path}...")
    create_database(db_path, socials, slays, liquids, wear_locs, days, months)
    print("\nDone!")


if __name__ == "__main__":
    main()
