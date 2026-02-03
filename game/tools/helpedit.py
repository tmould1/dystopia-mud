#!/usr/bin/env python3
"""
Help file editor for Dystopia MUD.

Manages help entries in base_help.db with ANSI color preview,
full CRUD operations, and keyword/text search.

Usage:
    python helpedit.py list [--level N]
    python helpedit.py view <id_or_keyword>
    python helpedit.py raw <id_or_keyword>
    python helpedit.py add <keyword> [--level N]
    python helpedit.py edit <id_or_keyword>
    python helpedit.py delete <id>
    python helpedit.py search <term>
    python helpedit.py colors
"""

import argparse
import os
import platform
import re
import sqlite3
import subprocess
import sys
import tempfile
from pathlib import Path


# ---------------------------------------------------------------------------
# Path resolution
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
DB_PATH = REPO_ROOT / 'gamedata' / 'db' / 'game' / 'base_help.db'


# ---------------------------------------------------------------------------
# MUD color code table  (from comm.c:1618-1651)
# ---------------------------------------------------------------------------

COLOR_CODES = {
    # Numbers: bright colors
    '0': "\033[0;1;30m",   # Bright Black
    '1': "\033[0;1;31m",   # Bright Red
    '2': "\033[0;1;32m",   # Bright Green
    '3': "\033[0;1;33m",   # Bright Yellow
    '4': "\033[0;1;34m",   # Bright Blue
    '5': "\033[0;1;35m",   # Bright Purple
    '6': "\033[0;1;36m",   # Bright Cyan
    '7': "\033[0;0;37m",   # White
    '8': "\033[0;0;30m",   # Black
    '9': "\033[0;1;37m",   # Bright White
    # Lowercase: dark colors
    'r': "\033[0;0;31m",   # Red
    'g': "\033[0;0;32m",   # Green
    'o': "\033[0;0;33m",   # Yellow/Brown
    'l': "\033[0;0;34m",   # Blue
    'p': "\033[0;0;35m",   # Purple
    'c': "\033[0;0;36m",   # Cyan
    'y': "\033[0;1;33m",   # Bright Yellow
    # Uppercase: bright colors (aliases)
    'R': "\033[0;1;31m",   # Bright Red
    'G': "\033[0;1;32m",   # Bright Green
    'L': "\033[0;1;34m",   # Bright Blue
    'P': "\033[0;1;35m",   # Bright Purple
    'C': "\033[0;1;36m",   # Bright Cyan
    # Special formatting
    'n': "\033[0m",        # Reset
    'i': "\033[7m",        # Inverse
    'u': "\033[4m",        # Underline
}

RESET = "\033[0m"


def render_colors(text):
    """Convert MUD #X color codes to ANSI escape sequences."""
    out = []
    i = 0
    while i < len(text):
        if text[i] == '#' and i + 1 < len(text):
            nxt = text[i + 1]
            if nxt == '#':
                out.append('#')
                i += 2
            elif nxt == '-':
                out.append('~')
                i += 2
            elif nxt == '+':
                out.append('%')
                i += 2
            elif nxt == 'x' and i + 4 < len(text) and text[i+2:i+5].isdigit():
                code = int(text[i+2:i+5])
                out.append(f"\033[38;5;{code}m")
                i += 5
            elif nxt in COLOR_CODES:
                out.append(COLOR_CODES[nxt])
                i += 2
            else:
                out.append(text[i])
                i += 1
        else:
            out.append(text[i])
            i += 1
    out.append(RESET)
    return ''.join(out)


def strip_colors(text):
    """Remove MUD #X color codes, returning plain text."""
    out = []
    i = 0
    while i < len(text):
        if text[i] == '#' and i + 1 < len(text):
            nxt = text[i + 1]
            if nxt == '#':
                out.append('#')
                i += 2
            elif nxt == '-':
                out.append('~')
                i += 2
            elif nxt == '+':
                out.append('%')
                i += 2
            elif nxt == 'x' and i + 4 < len(text) and text[i+2:i+5].isdigit():
                i += 5
            elif nxt in COLOR_CODES:
                i += 2
            else:
                out.append(text[i])
                i += 1
        else:
            out.append(text[i])
            i += 1
    return ''.join(out)


# ---------------------------------------------------------------------------
# Database helpers
# ---------------------------------------------------------------------------

def get_db():
    """Open a connection to base_help.db."""
    if not DB_PATH.exists():
        print(f"Error: {DB_PATH} not found.")
        sys.exit(1)
    return sqlite3.connect(str(DB_PATH))


def find_help(conn, key):
    """Look up a help entry by ID (int) or keyword (string).

    Returns (id, level, keyword, text) or None.
    If multiple matches on keyword, prints them and returns None.
    """
    # Try as integer ID first
    try:
        help_id = int(key)
        row = conn.execute(
            "SELECT id, level, keyword, text FROM helps WHERE id = ?",
            (help_id,)
        ).fetchone()
        if row:
            return row
    except ValueError:
        pass

    # Keyword search (case-insensitive, match any word in keyword field)
    rows = conn.execute(
        "SELECT id, level, keyword, text FROM helps WHERE keyword LIKE ?",
        (f"%{key}%",)
    ).fetchall()

    if len(rows) == 1:
        return rows[0]
    elif len(rows) == 0:
        print(f"No help entry found for '{key}'.")
        return None
    else:
        print(f"Multiple matches for '{key}' - be more specific or use ID:")
        for r in rows:
            print(f"  [{r[0]:>3}] level={r[1]} keyword={r[2]}")
        return None


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_list(args):
    """List all help entries."""
    conn = get_db()
    if args.level is not None:
        rows = conn.execute(
            "SELECT id, level, keyword FROM helps WHERE level = ? ORDER BY id",
            (args.level,)
        ).fetchall()
    else:
        rows = conn.execute(
            "SELECT id, level, keyword FROM helps ORDER BY id"
        ).fetchall()
    conn.close()

    if not rows:
        print("No help entries found.")
        return

    print(f"{'ID':>4}  {'Lvl':>3}  Keyword")
    print(f"{'----':>4}  {'---':>3}  {'-' * 40}")
    for row in rows:
        print(f"{row[0]:>4}  {row[1]:>3}  {row[2]}")
    print(f"\n{len(rows)} entries.")


def cmd_view(args):
    """View a help entry with ANSI color rendering."""
    conn = get_db()
    row = find_help(conn, args.key)
    conn.close()
    if not row:
        return

    help_id, level, keyword, text = row
    print(f"{RESET}--- [{help_id}] {keyword} (level {level}) ---")
    print(render_colors(text))
    print(f"{RESET}--- end ---")


def cmd_raw(args):
    """View a help entry with raw color codes."""
    conn = get_db()
    row = find_help(conn, args.key)
    conn.close()
    if not row:
        return

    help_id, level, keyword, text = row
    print(f"--- [{help_id}] {keyword} (level {level}) ---")
    print(text)
    print("--- end ---")


def cmd_add(args):
    """Add a new help entry via $EDITOR."""
    keyword = args.keyword.upper()
    level = args.level or 0

    text = open_editor("")
    if text is None:
        print("Aborted.")
        return

    conn = get_db()
    conn.execute(
        "INSERT INTO helps (level, keyword, text) VALUES (?, ?, ?)",
        (level, keyword, text)
    )
    conn.commit()
    new_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]
    conn.close()

    print(f"Created help entry [{new_id}] keyword={keyword} level={level}")


def cmd_edit(args):
    """Edit an existing help entry via $EDITOR."""
    conn = get_db()
    row = find_help(conn, args.key)
    if not row:
        conn.close()
        return

    help_id, level, keyword, old_text = row

    new_text = open_editor(old_text)
    if new_text is None:
        print("Aborted (no changes).")
        conn.close()
        return

    if new_text == old_text:
        print("No changes made.")
        conn.close()
        return

    conn.execute(
        "UPDATE helps SET text = ? WHERE id = ?",
        (new_text, help_id)
    )
    conn.commit()
    conn.close()

    print(f"Updated help entry [{help_id}] {keyword}")


def cmd_delete(args):
    """Delete a help entry by ID."""
    try:
        help_id = int(args.id)
    except ValueError:
        print("Error: delete requires a numeric ID.")
        return

    conn = get_db()
    row = conn.execute(
        "SELECT id, level, keyword FROM helps WHERE id = ?",
        (help_id,)
    ).fetchone()
    if not row:
        print(f"No help entry with ID {help_id}.")
        conn.close()
        return

    print(f"Delete [{row[0]}] level={row[1]} keyword={row[2]}?")
    confirm = input("Type 'yes' to confirm: ").strip().lower()
    if confirm != 'yes':
        print("Aborted.")
        conn.close()
        return

    conn.execute("DELETE FROM helps WHERE id = ?", (help_id,))
    conn.commit()
    conn.close()
    print(f"Deleted help entry [{help_id}].")


def cmd_search(args):
    """Search help entries by keyword or text content."""
    term = args.term
    conn = get_db()

    rows = conn.execute(
        "SELECT id, level, keyword, text FROM helps "
        "WHERE keyword LIKE ? OR text LIKE ? ORDER BY id",
        (f"%{term}%", f"%{term}%")
    ).fetchall()
    conn.close()

    if not rows:
        print(f"No matches for '{term}'.")
        return

    print(f"{'ID':>4}  {'Lvl':>3}  {'Keyword':<30}  Preview")
    print(f"{'----':>4}  {'---':>3}  {'-' * 30}  {'-' * 40}")
    for r in rows:
        preview = strip_colors(r[3]).replace('\n', ' ').replace('\r', '')[:50]
        print(f"{r[0]:>4}  {r[1]:>3}  {r[2]:<30}  {preview}")
    print(f"\n{len(rows)} matches.")


def cmd_colors(args):
    """Print a color code reference chart with live ANSI preview."""
    print("Dystopia MUD Color Code Reference")
    print("=" * 50)
    print()

    sections = [
        ("Bright Colors (numbers)", [
            ('0', 'Bright Black'),  ('1', 'Bright Red'),
            ('2', 'Bright Green'),  ('3', 'Bright Yellow'),
            ('4', 'Bright Blue'),   ('5', 'Bright Purple'),
            ('6', 'Bright Cyan'),   ('7', 'White'),
            ('8', 'Black'),         ('9', 'Bright White'),
        ]),
        ("Dark Colors (lowercase)", [
            ('r', 'Dark Red'),    ('g', 'Dark Green'),
            ('o', 'Brown/Yellow'),('l', 'Dark Blue'),
            ('p', 'Dark Purple'), ('c', 'Dark Cyan'),
            ('y', 'Bright Yellow'),
        ]),
        ("Bright Aliases (uppercase)", [
            ('R', 'Bright Red'),  ('G', 'Bright Green'),
            ('L', 'Bright Blue'), ('P', 'Bright Purple'),
            ('C', 'Bright Cyan'),
        ]),
        ("Formatting", [
            ('n', 'Reset'),
            ('i', 'Inverse'),
            ('u', 'Underline'),
        ]),
    ]

    for title, codes in sections:
        print(f"  {title}:")
        for code, name in codes:
            ansi = COLOR_CODES[code]
            print(f"    #{code}  {ansi}Example Text{RESET}  {name}")
        print()

    print("  Special:")
    print("    ##   Literal # character")
    print("    #-   Literal ~ character")
    print("    #+   Literal % character")
    print("    #xNNN  256-color (e.g. #x128)")
    print()


# ---------------------------------------------------------------------------
# Editor integration
# ---------------------------------------------------------------------------

def get_editor():
    """Return the user's preferred editor command."""
    editor = os.environ.get('EDITOR') or os.environ.get('VISUAL')
    if editor:
        return editor
    if platform.system() == 'Windows':
        return 'notepad'
    return 'vi'


def open_editor(initial_text):
    """Open $EDITOR with initial_text, return edited text or None on abort."""
    editor = get_editor()

    with tempfile.NamedTemporaryFile(
        mode='w', suffix='.txt', delete=False, encoding='utf-8'
    ) as f:
        f.write(initial_text)
        tmppath = f.name

    try:
        # Get mtime before editing
        mtime_before = os.path.getmtime(tmppath)

        result = subprocess.run([editor, tmppath])
        if result.returncode != 0:
            return None

        # Check if file was actually modified
        mtime_after = os.path.getmtime(tmppath)
        if mtime_after == mtime_before:
            return None

        with open(tmppath, 'r', encoding='utf-8') as f:
            return f.read()
    finally:
        try:
            os.unlink(tmppath)
        except OSError:
            pass


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Dystopia MUD help file editor',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Database: base_help.db"
    )
    sub = parser.add_subparsers(dest='command', required=True)

    # list
    p = sub.add_parser('list', help='List all help entries')
    p.add_argument('--level', type=int, default=None,
                   help='Filter by access level')

    # view
    p = sub.add_parser('view', help='View entry with ANSI colors')
    p.add_argument('key', help='Entry ID or keyword')

    # raw
    p = sub.add_parser('raw', help='View entry with raw color codes')
    p.add_argument('key', help='Entry ID or keyword')

    # add
    p = sub.add_parser('add', help='Add a new help entry')
    p.add_argument('keyword', help='Keyword(s) for the entry')
    p.add_argument('--level', type=int, default=0,
                   help='Access level (default: 0)')

    # edit
    p = sub.add_parser('edit', help='Edit an existing entry')
    p.add_argument('key', help='Entry ID or keyword')

    # delete
    p = sub.add_parser('delete', help='Delete an entry by ID')
    p.add_argument('id', help='Entry ID to delete')

    # search
    p = sub.add_parser('search', help='Search entries by keyword or text')
    p.add_argument('term', help='Search term')

    # colors
    sub.add_parser('colors', help='Show color code reference chart')

    args = parser.parse_args()

    commands = {
        'list':   cmd_list,
        'view':   cmd_view,
        'raw':    cmd_raw,
        'add':    cmd_add,
        'edit':   cmd_edit,
        'delete': cmd_delete,
        'search': cmd_search,
        'colors': cmd_colors,
    }

    commands[args.command](args)


if __name__ == '__main__':
    main()
