#!/usr/bin/env python3
"""
Migrate game data text files into gamedata/db/game/game.db (SQLite).

Migrates:
  - gamedata/txt/gameconfig.txt -> gameconfig table
  - gamedata/txt/topboard.txt   -> topboard table
  - gamedata/txt/leader.txt     -> leaderboard table
  - gamedata/txt/kingdoms.txt   -> kingdoms table

Usage:
    python migrate_gamedata.py
"""

import sqlite3
import sys
from pathlib import Path


GAME_SCHEMA = """
CREATE TABLE IF NOT EXISTS gameconfig (
    key        TEXT PRIMARY KEY,
    value      TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS topboard (
    rank       INTEGER PRIMARY KEY,
    name       TEXT NOT NULL DEFAULT 'Empty',
    pkscore    INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS leaderboard (
    category   TEXT PRIMARY KEY,
    name       TEXT NOT NULL DEFAULT 'Nobody',
    value      INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS kingdoms (
    id         INTEGER PRIMARY KEY,
    name       TEXT NOT NULL DEFAULT 'None',
    whoname    TEXT NOT NULL DEFAULT 'None',
    leader     TEXT NOT NULL DEFAULT 'None',
    general    TEXT NOT NULL DEFAULT 'None',
    kills      INTEGER NOT NULL DEFAULT 0,
    deaths     INTEGER NOT NULL DEFAULT 0,
    qps        INTEGER NOT NULL DEFAULT 0,
    req_hit    INTEGER NOT NULL DEFAULT 0,
    req_move   INTEGER NOT NULL DEFAULT 0,
    req_mana   INTEGER NOT NULL DEFAULT 0,
    req_qps    INTEGER NOT NULL DEFAULT 0
);
"""

MAX_TOP_PLAYERS = 20
MAX_KINGDOM = 5


def read_tilde_string(lines, idx):
    """Read a tilde-terminated string from lines. Returns (string, next_idx)."""
    s = lines[idx].rstrip('\n\r')
    if s.endswith('~'):
        return s[:-1], idx + 1
    return s, idx + 1


def migrate_gameconfig(conn, txt_dir):
    """Migrate gameconfig.txt -> gameconfig table."""
    filepath = txt_dir / 'gameconfig.txt'
    if not filepath.exists():
        print(f"  SKIP  gameconfig.txt (not found)")
        return

    config = {}
    with open(filepath, 'r', encoding='latin-1') as f:
        for line in f:
            line = line.strip()
            if not line or ':' not in line:
                continue
            key, _, value = line.partition(':')
            config[key.strip()] = value.strip()

    if not config:
        print(f"  SKIP  gameconfig.txt (empty)")
        return

    for key, value in config.items():
        conn.execute(
            "INSERT OR REPLACE INTO gameconfig (key, value) VALUES (?, ?)",
            (key, value)
        )
    print(f"  OK    gameconfig.txt ({len(config)} keys)")


def migrate_topboard(conn, txt_dir):
    """Migrate topboard.txt -> topboard table."""
    filepath = txt_dir / 'topboard.txt'
    if not filepath.exists():
        print(f"  SKIP  topboard.txt (not found)")
        return

    with open(filepath, 'r', encoding='latin-1') as f:
        lines = f.readlines()

    idx = 0
    count = 0
    for rank in range(1, MAX_TOP_PLAYERS + 1):
        if idx >= len(lines):
            break
        name, idx = read_tilde_string(lines, idx)
        if idx >= len(lines):
            break
        pkscore = int(lines[idx].strip())
        idx += 1
        conn.execute(
            "INSERT OR REPLACE INTO topboard (rank, name, pkscore) VALUES (?, ?, ?)",
            (rank, name, pkscore)
        )
        count += 1

    print(f"  OK    topboard.txt ({count} entries)")


def migrate_leaderboard(conn, txt_dir):
    """Migrate leader.txt -> leaderboard table."""
    filepath = txt_dir / 'leader.txt'
    if not filepath.exists():
        print(f"  SKIP  leader.txt (not found)")
        return

    categories = ['bestpk', 'pk', 'pd', 'mk', 'md', 'tt', 'qc']

    with open(filepath, 'r', encoding='latin-1') as f:
        lines = f.readlines()

    idx = 0
    count = 0
    for cat in categories:
        if idx >= len(lines):
            break
        name, idx = read_tilde_string(lines, idx)
        if idx >= len(lines):
            break
        value = int(lines[idx].strip())
        idx += 1
        conn.execute(
            "INSERT OR REPLACE INTO leaderboard (category, name, value) VALUES (?, ?, ?)",
            (cat, name, value)
        )
        count += 1

    print(f"  OK    leader.txt ({count} categories)")


def migrate_kingdoms(conn, txt_dir):
    """Migrate kingdoms.txt -> kingdoms table."""
    filepath = txt_dir / 'kingdoms.txt'
    if not filepath.exists():
        print(f"  SKIP  kingdoms.txt (not found)")
        return

    with open(filepath, 'r', encoding='latin-1') as f:
        lines = f.readlines()

    idx = 0
    count = 0
    for kid in range(1, MAX_KINGDOM + 1):
        if idx + 5 >= len(lines):
            break
        name, idx = read_tilde_string(lines, idx)
        whoname, idx = read_tilde_string(lines, idx)
        leader, idx = read_tilde_string(lines, idx)
        general, idx = read_tilde_string(lines, idx)

        parts1 = lines[idx].strip().split()
        idx += 1
        kills, deaths, qps = int(parts1[0]), int(parts1[1]), int(parts1[2])

        parts2 = lines[idx].strip().split()
        idx += 1
        req_hit, req_move, req_mana, req_qps = (
            int(parts2[0]), int(parts2[1]), int(parts2[2]), int(parts2[3])
        )

        conn.execute(
            "INSERT OR REPLACE INTO kingdoms "
            "(id, name, whoname, leader, general, kills, deaths, qps, "
            "req_hit, req_move, req_mana, req_qps) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (kid, name, whoname, leader, general, kills, deaths, qps,
             req_hit, req_move, req_mana, req_qps)
        )
        count += 1

    print(f"  OK    kingdoms.txt ({count} kingdoms)")


def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    txt_dir = repo_root / 'gamedata' / 'txt'
    db_dir = repo_root / 'gamedata' / 'db' / 'game'

    db_dir.mkdir(parents=True, exist_ok=True)
    db_path = db_dir / 'game.db'

    print(f"Migrating game data to {db_path}")

    conn = sqlite3.connect(str(db_path))

    # Create schema (multiple statements need executescript)
    conn.executescript(GAME_SCHEMA)

    conn.execute("BEGIN TRANSACTION")

    migrate_gameconfig(conn, txt_dir)
    migrate_topboard(conn, txt_dir)
    migrate_leaderboard(conn, txt_dir)
    migrate_kingdoms(conn, txt_dir)

    conn.commit()
    conn.close()

    print("\nMigration complete.")


if __name__ == '__main__':
    main()
