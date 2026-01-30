#!/usr/bin/env python3
"""
Migrate .are area files to per-area SQLite databases.

Reads all areas from gamedata/area/ via area_parser and writes each
to gamedata/db/<basename>.db using the same schema as db_sql.c.

Usage:
    python migrate_to_sqlite.py [--area-dir PATH] [--db-dir PATH]

By default, paths are derived relative to the repository root.
"""

import argparse
import os
import sqlite3
import sys
from pathlib import Path

# Add parent so we can import mudlib
sys.path.insert(0, str(Path(__file__).resolve().parent))

from mudlib.area_parser import parse_area_file
from mudlib.models import Area


SCHEMA_SQL = """
CREATE TABLE IF NOT EXISTS area (
    name        TEXT NOT NULL,
    builders    TEXT DEFAULT '',
    lvnum       INTEGER NOT NULL,
    uvnum       INTEGER NOT NULL,
    security    INTEGER DEFAULT 3,
    recall      INTEGER DEFAULT 0,
    area_flags  INTEGER DEFAULT 0,
    is_hidden   INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS mobiles (
    vnum        INTEGER PRIMARY KEY,
    player_name TEXT, short_descr TEXT, long_descr TEXT, description TEXT,
    act         INTEGER, affected_by INTEGER, alignment INTEGER,
    level       INTEGER, hitroll INTEGER, ac INTEGER,
    hitnodice   INTEGER, hitsizedice INTEGER, hitplus INTEGER,
    damnodice   INTEGER, damsizedice INTEGER, damplus INTEGER,
    gold        INTEGER, sex INTEGER
);

CREATE TABLE IF NOT EXISTS objects (
    vnum        INTEGER PRIMARY KEY,
    name TEXT, short_descr TEXT, description TEXT,
    item_type INTEGER, extra_flags INTEGER, wear_flags INTEGER,
    value0 INTEGER, value1 INTEGER, value2 INTEGER, value3 INTEGER,
    weight INTEGER, cost INTEGER,
    chpoweron TEXT, chpoweroff TEXT, chpoweruse TEXT,
    victpoweron TEXT, victpoweroff TEXT, victpoweruse TEXT,
    spectype INTEGER DEFAULT 0, specpower INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS object_affects (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    obj_vnum   INTEGER NOT NULL REFERENCES objects(vnum),
    location   INTEGER NOT NULL,
    modifier   INTEGER NOT NULL,
    sort_order INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS extra_descriptions (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_type TEXT NOT NULL,
    owner_vnum INTEGER NOT NULL,
    keyword    TEXT NOT NULL,
    description TEXT NOT NULL,
    sort_order INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS rooms (
    vnum        INTEGER PRIMARY KEY,
    name TEXT, description TEXT,
    room_flags INTEGER, sector_type INTEGER
);

CREATE TABLE IF NOT EXISTS exits (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    room_vnum   INTEGER NOT NULL REFERENCES rooms(vnum),
    direction   INTEGER NOT NULL,
    description TEXT DEFAULT '', keyword TEXT DEFAULT '',
    exit_info   INTEGER DEFAULT 0,
    key_vnum    INTEGER DEFAULT -1,
    to_vnum     INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS room_texts (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    room_vnum  INTEGER NOT NULL REFERENCES rooms(vnum),
    input TEXT, output TEXT, choutput TEXT, name TEXT,
    type INTEGER, power INTEGER, mob INTEGER,
    sort_order INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS resets (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    command    TEXT NOT NULL,
    arg1 INTEGER, arg2 INTEGER, arg3 INTEGER,
    sort_order INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS shops (
    keeper_vnum INTEGER PRIMARY KEY,
    buy_type0 INTEGER, buy_type1 INTEGER, buy_type2 INTEGER,
    buy_type3 INTEGER, buy_type4 INTEGER,
    profit_buy INTEGER, profit_sell INTEGER,
    open_hour INTEGER, close_hour INTEGER
);

CREATE TABLE IF NOT EXISTS specials (
    mob_vnum      INTEGER PRIMARY KEY,
    spec_fun_name TEXT NOT NULL
);
"""


def write_area_db(area: Area, db_path: Path) -> dict:
    """Write a single Area to a SQLite database. Returns counts dict."""
    if db_path.exists():
        db_path.unlink()

    conn = sqlite3.connect(str(db_path))
    conn.executescript(SCHEMA_SQL)

    counts = {
        'mobiles': 0, 'objects': 0, 'rooms': 0, 'exits': 0,
        'resets': 0, 'shops': 0, 'specials': 0,
        'extra_descs': 0, 'obj_affects': 0, 'room_texts': 0,
    }

    # Area metadata
    conn.execute(
        "INSERT INTO area (name, builders, lvnum, uvnum, security, recall, area_flags) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)",
        (area.name, area.builders, area.lvnum, area.uvnum,
         area.security, area.recall, area.area_flags)
    )

    # Mobiles
    for vnum, mob in area.mobiles.items():
        conn.execute(
            "INSERT INTO mobiles (vnum, player_name, short_descr, long_descr, description, "
            "act, affected_by, alignment, level, hitroll, ac, "
            "hitnodice, hitsizedice, hitplus, damnodice, damsizedice, damplus, "
            "gold, sex) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (vnum, mob.name, mob.short_descr, mob.long_descr, mob.description,
             mob.act_flags, mob.affected_by, mob.alignment,
             mob.level, mob.hitroll, mob.ac,
             mob.hit_dice[0], mob.hit_dice[1], mob.hit_dice[2],
             mob.dam_dice[0], mob.dam_dice[1], mob.dam_dice[2],
             mob.gold, mob.sex)
        )
        counts['mobiles'] += 1

    # Objects
    for vnum, obj in area.objects.items():
        conn.execute(
            "INSERT INTO objects (vnum, name, short_descr, description, "
            "item_type, extra_flags, wear_flags, "
            "value0, value1, value2, value3, weight, cost, "
            "chpoweron, chpoweroff, chpoweruse, "
            "victpoweron, victpoweroff, victpoweruse, "
            "spectype, specpower) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (vnum, obj.name, obj.short_descr, obj.long_descr,
             obj.item_type, obj.extra_flags, obj.wear_flags,
             obj.value[0], obj.value[1], obj.value[2], obj.value[3],
             obj.weight, obj.cost,
             obj.chpoweron, obj.chpoweroff, obj.chpoweruse,
             obj.victpoweron, obj.victpoweroff, obj.victpoweruse,
             obj.spectype, obj.specpower)
        )
        counts['objects'] += 1

        # Object affects
        for sort_idx, aff in enumerate(obj.affects):
            conn.execute(
                "INSERT INTO object_affects (obj_vnum, location, modifier, sort_order) "
                "VALUES (?, ?, ?, ?)",
                (vnum, aff.apply_type, aff.modifier, sort_idx)
            )
            counts['obj_affects'] += 1

        # Object extra descriptions
        for sort_idx, ed in enumerate(obj.extra_descs):
            conn.execute(
                "INSERT INTO extra_descriptions (owner_type, owner_vnum, keyword, description, sort_order) "
                "VALUES ('object', ?, ?, ?, ?)",
                (vnum, ed.keyword, ed.description, sort_idx)
            )
            counts['extra_descs'] += 1

    # Rooms
    for vnum, room in area.rooms.items():
        conn.execute(
            "INSERT INTO rooms (vnum, name, description, room_flags, sector_type) "
            "VALUES (?, ?, ?, ?, ?)",
            (vnum, room.name, room.description, room.room_flags, room.sector_type)
        )
        counts['rooms'] += 1

        # Exits
        for direction, exit_obj in room.exits.items():
            conn.execute(
                "INSERT INTO exits (room_vnum, direction, description, keyword, "
                "exit_info, key_vnum, to_vnum) VALUES (?, ?, ?, ?, ?, ?, ?)",
                (vnum, direction, exit_obj.description, exit_obj.keyword,
                 exit_obj.door_flags, exit_obj.key_vnum, exit_obj.destination_vnum)
            )
            counts['exits'] += 1

        # Room extra descriptions
        for sort_idx, ed in enumerate(room.extra_descs):
            conn.execute(
                "INSERT INTO extra_descriptions (owner_type, owner_vnum, keyword, description, sort_order) "
                "VALUES ('room', ?, ?, ?, ?)",
                (vnum, ed.keyword, ed.description, sort_idx)
            )
            counts['extra_descs'] += 1

        # Room texts
        for sort_idx, rt in enumerate(room.room_texts):
            conn.execute(
                "INSERT INTO room_texts (room_vnum, input, output, choutput, name, "
                "type, power, mob, sort_order) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                (vnum, rt.input, rt.output, rt.choutput, rt.name,
                 rt.type, rt.power, rt.mob, sort_idx)
            )
            counts['room_texts'] += 1

    # Resets
    for sort_idx, reset in enumerate(area.resets):
        conn.execute(
            "INSERT INTO resets (command, arg1, arg2, arg3, sort_order) "
            "VALUES (?, ?, ?, ?, ?)",
            (reset.command, reset.arg1, reset.arg2, reset.arg3, sort_idx)
        )
        counts['resets'] += 1

    # Shops
    for shop in area.shops:
        bt = shop.buy_types + [0] * (5 - len(shop.buy_types))
        conn.execute(
            "INSERT INTO shops (keeper_vnum, buy_type0, buy_type1, buy_type2, "
            "buy_type3, buy_type4, profit_buy, profit_sell, open_hour, close_hour) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (shop.keeper_vnum, bt[0], bt[1], bt[2], bt[3], bt[4],
             shop.profit_buy, shop.profit_sell, shop.open_hour, shop.close_hour)
        )
        counts['shops'] += 1

    # Specials
    for mob_vnum, spec_fun in area.specials.items():
        conn.execute(
            "INSERT INTO specials (mob_vnum, spec_fun_name) VALUES (?, ?)",
            (mob_vnum, spec_fun)
        )
        counts['specials'] += 1

    conn.commit()
    conn.close()
    return counts


def main():
    parser = argparse.ArgumentParser(description="Migrate .are files to SQLite databases")
    parser.add_argument('--area-dir', type=Path, default=None,
                        help="Path to area/ directory (default: gamedata/area/)")
    parser.add_argument('--db-dir', type=Path, default=None,
                        help="Path to db/ output directory (default: gamedata/db/areas/)")
    args = parser.parse_args()

    # Derive paths relative to repo root
    repo_root = Path(__file__).resolve().parent.parent.parent
    area_dir = args.area_dir or repo_root / 'gamedata' / 'area'
    db_dir = args.db_dir or repo_root / 'gamedata' / 'db' / 'areas'

    if not area_dir.exists():
        print(f"Error: area directory not found: {area_dir}")
        sys.exit(1)

    db_dir.mkdir(parents=True, exist_ok=True)

    area_list = area_dir / 'area.lst'
    if not area_list.exists():
        print(f"Error: area.lst not found in {area_dir}")
        sys.exit(1)

    with open(area_list, 'r') as f:
        area_files = [line.strip() for line in f if line.strip() and not line.startswith('$')]

    total_counts = {
        'areas': 0, 'mobiles': 0, 'objects': 0, 'rooms': 0, 'exits': 0,
        'resets': 0, 'shops': 0, 'specials': 0,
        'extra_descs': 0, 'obj_affects': 0, 'room_texts': 0,
    }
    errors = []

    for area_file in area_files:
        filepath = area_dir / area_file
        if not filepath.exists():
            print(f"  SKIP  {area_file} (not found)")
            continue

        basename = filepath.stem
        db_path = db_dir / f"{basename}.db"

        try:
            area = parse_area_file(filepath)
            if area is None:
                errors.append(f"{area_file}: parser returned None")
                print(f"  FAIL  {area_file}")
                continue

            counts = write_area_db(area, db_path)
            total_counts['areas'] += 1
            for k, v in counts.items():
                total_counts[k] += v

            parts = []
            if counts['mobiles']:
                parts.append(f"{counts['mobiles']}m")
            if counts['objects']:
                parts.append(f"{counts['objects']}o")
            if counts['rooms']:
                parts.append(f"{counts['rooms']}r")
            if counts['resets']:
                parts.append(f"{counts['resets']}R")
            if counts['shops']:
                parts.append(f"{counts['shops']}S")
            if counts['specials']:
                parts.append(f"{counts['specials']}sp")
            summary = " ".join(parts) if parts else "empty"
            print(f"  OK    {area_file:30s} -> {basename}.db  [{summary}]")

        except Exception as e:
            errors.append(f"{area_file}: {e}")
            print(f"  FAIL  {area_file}: {e}")

    print()
    print(f"Migration complete: {total_counts['areas']} areas")
    print(f"  Mobiles: {total_counts['mobiles']}")
    print(f"  Objects: {total_counts['objects']} ({total_counts['obj_affects']} affects)")
    print(f"  Rooms:   {total_counts['rooms']} ({total_counts['exits']} exits)")
    print(f"  Resets:  {total_counts['resets']}")
    print(f"  Shops:   {total_counts['shops']}")
    print(f"  Specials:{total_counts['specials']}")
    print(f"  Extra descriptions: {total_counts['extra_descs']}")
    print(f"  Room texts: {total_counts['room_texts']}")

    if errors:
        print(f"\n{len(errors)} error(s):")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
