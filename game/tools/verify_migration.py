#!/usr/bin/env python3
"""
Verify that each .db SQLite database matches its corresponding .are source file.

For every area listed in area.lst (except help.are), this script:
  1. Parses the .are file with mudlib.area_parser.parse_area_file
  2. Opens the matching .db file with sqlite3
  3. Compares counts AND field-level data for every table

Usage:
    python verify_migration.py [--area-dir PATH] [--db-dir PATH]
"""

import argparse
import os
import sqlite3
import sys
from pathlib import Path

# Ensure we can import mudlib
sys.path.insert(0, str(Path(__file__).resolve().parent))

from mudlib.area_parser import parse_area_file
from mudlib.models import Area


def compare_values(label, expected, actual):
    """Return a mismatch string if values differ, else None."""
    if expected != actual:
        return f"  {label}: expected {expected!r}, got {actual!r}"
    return None


def verify_area_table(conn, area):
    """Verify the area metadata table."""
    mismatches = []
    row = conn.execute("SELECT name, builders, lvnum, uvnum, security, recall, area_flags FROM area").fetchone()
    if row is None:
        return ["  area table: no row found"]

    fields = [
        ("name", area.name, row[0]),
        ("builders", area.builders, row[1]),
        ("lvnum", area.lvnum, row[2]),
        ("uvnum", area.uvnum, row[3]),
        ("security", area.security, row[4]),
        ("recall", area.recall, row[5]),
        ("area_flags", area.area_flags, row[6]),
    ]
    for name, expected, actual in fields:
        m = compare_values(f"area.{name}", expected, actual)
        if m:
            mismatches.append(m)
    return mismatches


def verify_mobiles(conn, area):
    """Verify mobiles table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT vnum, player_name, short_descr, level, act, affected_by, "
        "alignment, hitroll, ac, hitnodice, hitsizedice, hitplus, "
        "damnodice, damsizedice, damplus, gold, sex FROM mobiles ORDER BY vnum"
    ).fetchall()
    db_by_vnum = {r[0]: r for r in db_rows}

    expected_count = len(area.mobiles)
    actual_count = len(db_rows)
    if expected_count != actual_count:
        mismatches.append(f"  mobiles count: expected {expected_count}, got {actual_count}")

    for vnum, mob in area.mobiles.items():
        row = db_by_vnum.get(vnum)
        if row is None:
            mismatches.append(f"  mobile vnum {vnum}: missing from db")
            continue
        checks = [
            ("player_name", mob.name, row[1]),
            ("short_descr", mob.short_descr, row[2]),
            ("level", mob.level, row[3]),
            ("act", mob.act_flags, row[4]),
            ("affected_by", mob.affected_by, row[5]),
            ("alignment", mob.alignment, row[6]),
            ("hitroll", mob.hitroll, row[7]),
            ("ac", mob.ac, row[8]),
            ("hitnodice", mob.hit_dice[0], row[9]),
            ("hitsizedice", mob.hit_dice[1], row[10]),
            ("hitplus", mob.hit_dice[2], row[11]),
            ("damnodice", mob.dam_dice[0], row[12]),
            ("damsizedice", mob.dam_dice[1], row[13]),
            ("damplus", mob.dam_dice[2], row[14]),
            ("gold", mob.gold, row[15]),
            ("sex", mob.sex, row[16]),
        ]
        for name, expected, actual in checks:
            m = compare_values(f"mobile {vnum}.{name}", expected, actual)
            if m:
                mismatches.append(m)

    # Check for extra vnums in db
    for vnum in db_by_vnum:
        if vnum not in area.mobiles:
            mismatches.append(f"  mobile vnum {vnum}: in db but not in .are")

    return mismatches


def verify_objects(conn, area):
    """Verify objects table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT vnum, name, short_descr, description, item_type, extra_flags, wear_flags, "
        "value0, value1, value2, value3, weight, cost, "
        "chpoweron, chpoweroff, chpoweruse, "
        "victpoweron, victpoweroff, victpoweruse, "
        "spectype, specpower FROM objects ORDER BY vnum"
    ).fetchall()
    db_by_vnum = {r[0]: r for r in db_rows}

    expected_count = len(area.objects)
    actual_count = len(db_rows)
    if expected_count != actual_count:
        mismatches.append(f"  objects count: expected {expected_count}, got {actual_count}")

    for vnum, obj in area.objects.items():
        row = db_by_vnum.get(vnum)
        if row is None:
            mismatches.append(f"  object vnum {vnum}: missing from db")
            continue
        checks = [
            ("name", obj.name, row[1]),
            ("short_descr", obj.short_descr, row[2]),
            ("description", obj.long_descr, row[3]),
            ("item_type", obj.item_type, row[4]),
            ("extra_flags", obj.extra_flags, row[5]),
            ("wear_flags", obj.wear_flags, row[6]),
            ("value0", obj.value[0], row[7]),
            ("value1", obj.value[1], row[8]),
            ("value2", obj.value[2], row[9]),
            ("value3", obj.value[3], row[10]),
            ("weight", obj.weight, row[11]),
            ("cost", obj.cost, row[12]),
            ("chpoweron", obj.chpoweron, row[13]),
            ("chpoweroff", obj.chpoweroff, row[14]),
            ("chpoweruse", obj.chpoweruse, row[15]),
            ("victpoweron", obj.victpoweron, row[16]),
            ("victpoweroff", obj.victpoweroff, row[17]),
            ("victpoweruse", obj.victpoweruse, row[18]),
            ("spectype", obj.spectype, row[19]),
            ("specpower", obj.specpower, row[20]),
        ]
        for name, expected, actual in checks:
            m = compare_values(f"object {vnum}.{name}", expected, actual)
            if m:
                mismatches.append(m)

    for vnum in db_by_vnum:
        if vnum not in area.objects:
            mismatches.append(f"  object vnum {vnum}: in db but not in .are")

    return mismatches


def verify_object_affects(conn, area):
    """Verify object_affects table - compare (location, modifier) lists per object."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT obj_vnum, location, modifier FROM object_affects ORDER BY obj_vnum, sort_order"
    ).fetchall()

    # Group by obj_vnum
    db_by_vnum = {}
    for obj_vnum, location, modifier in db_rows:
        db_by_vnum.setdefault(obj_vnum, []).append((location, modifier))

    for vnum, obj in area.objects.items():
        expected = [(a.apply_type, a.modifier) for a in obj.affects]
        actual = db_by_vnum.get(vnum, [])
        if expected != actual:
            mismatches.append(
                f"  object_affects vnum {vnum}: expected {expected}, got {actual}"
            )

    # Check for affects on objects not in .are
    for vnum in db_by_vnum:
        if vnum not in area.objects:
            mismatches.append(f"  object_affects vnum {vnum}: in db but object not in .are")

    return mismatches


def verify_extra_descriptions(conn, area):
    """Verify extra_descriptions table for both objects and rooms."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT owner_type, owner_vnum, keyword, description "
        "FROM extra_descriptions ORDER BY owner_type, owner_vnum, sort_order"
    ).fetchall()

    # Group by (owner_type, owner_vnum)
    db_grouped = {}
    for owner_type, owner_vnum, keyword, description in db_rows:
        db_grouped.setdefault((owner_type, owner_vnum), []).append((keyword, description))

    # Check object extra descs
    for vnum, obj in area.objects.items():
        expected = [(ed.keyword, ed.description) for ed in obj.extra_descs]
        actual = db_grouped.get(("object", vnum), [])
        if expected != actual:
            mismatches.append(
                f"  extra_desc object {vnum}: expected {len(expected)} entries, got {len(actual)}"
            )
            # Show first difference
            for i in range(max(len(expected), len(actual))):
                e = expected[i] if i < len(expected) else None
                a = actual[i] if i < len(actual) else None
                if e != a:
                    mismatches.append(f"    [{i}] expected {e!r}, got {a!r}")
                    break

    # Check room extra descs
    for vnum, room in area.rooms.items():
        expected = [(ed.keyword, ed.description) for ed in room.extra_descs]
        actual = db_grouped.get(("room", vnum), [])
        if expected != actual:
            mismatches.append(
                f"  extra_desc room {vnum}: expected {len(expected)} entries, got {len(actual)}"
            )
            for i in range(max(len(expected), len(actual))):
                e = expected[i] if i < len(expected) else None
                a = actual[i] if i < len(actual) else None
                if e != a:
                    mismatches.append(f"    [{i}] expected {e!r}, got {a!r}")
                    break

    return mismatches


def verify_rooms(conn, area):
    """Verify rooms table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT vnum, name, description, room_flags, sector_type FROM rooms ORDER BY vnum"
    ).fetchall()
    db_by_vnum = {r[0]: r for r in db_rows}

    expected_count = len(area.rooms)
    actual_count = len(db_rows)
    if expected_count != actual_count:
        mismatches.append(f"  rooms count: expected {expected_count}, got {actual_count}")

    for vnum, room in area.rooms.items():
        row = db_by_vnum.get(vnum)
        if row is None:
            mismatches.append(f"  room vnum {vnum}: missing from db")
            continue
        checks = [
            ("name", room.name, row[1]),
            ("description", room.description, row[2]),
            ("room_flags", room.room_flags, row[3]),
            ("sector_type", room.sector_type, row[4]),
        ]
        for name, expected, actual in checks:
            m = compare_values(f"room {vnum}.{name}", expected, actual)
            if m:
                mismatches.append(m)

    for vnum in db_by_vnum:
        if vnum not in area.rooms:
            mismatches.append(f"  room vnum {vnum}: in db but not in .are")

    return mismatches


def verify_exits(conn, area):
    """Verify exits table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT room_vnum, direction, description, keyword, exit_info, key_vnum, to_vnum "
        "FROM exits ORDER BY room_vnum, direction"
    ).fetchall()

    # Group by room_vnum
    db_by_room = {}
    for room_vnum, direction, description, keyword, exit_info, key_vnum, to_vnum in db_rows:
        db_by_room.setdefault(room_vnum, {})[direction] = (description, keyword, exit_info, key_vnum, to_vnum)

    for vnum, room in area.rooms.items():
        db_exits = db_by_room.get(vnum, {})

        for direction, exit_obj in room.exits.items():
            db_exit = db_exits.get(direction)
            if db_exit is None:
                mismatches.append(f"  exit room {vnum} dir {direction}: missing from db")
                continue
            checks = [
                ("description", exit_obj.description, db_exit[0]),
                ("keyword", exit_obj.keyword, db_exit[1]),
                ("exit_info", exit_obj.door_flags, db_exit[2]),
                ("key_vnum", exit_obj.key_vnum, db_exit[3]),
                ("to_vnum", exit_obj.destination_vnum, db_exit[4]),
            ]
            for name, expected, actual in checks:
                m = compare_values(f"exit room {vnum} dir {direction}.{name}", expected, actual)
                if m:
                    mismatches.append(m)

        # Check for extra exits in db
        for direction in db_exits:
            if direction not in room.exits:
                mismatches.append(f"  exit room {vnum} dir {direction}: in db but not in .are")

    return mismatches


def verify_room_texts(conn, area):
    """Verify room_texts table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT room_vnum, input, output, choutput, name, type, power, mob "
        "FROM room_texts ORDER BY room_vnum, sort_order"
    ).fetchall()

    # Group by room_vnum
    db_by_room = {}
    for row in db_rows:
        db_by_room.setdefault(row[0], []).append(row[1:])

    for vnum, room in area.rooms.items():
        expected = [(rt.input, rt.output, rt.choutput, rt.name, rt.type, rt.power, rt.mob)
                    for rt in room.room_texts]
        actual = db_by_room.get(vnum, [])

        if len(expected) != len(actual):
            mismatches.append(
                f"  room_texts room {vnum}: expected {len(expected)}, got {len(actual)}"
            )

        for i in range(min(len(expected), len(actual))):
            e = expected[i]
            a = actual[i]
            field_names = ["input", "output", "choutput", "name", "type", "power", "mob"]
            for j, fname in enumerate(field_names):
                m = compare_values(f"room_text room {vnum}[{i}].{fname}", e[j], a[j])
                if m:
                    mismatches.append(m)

    return mismatches


def verify_resets(conn, area):
    """Verify resets table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT command, arg1, arg2, arg3 FROM resets ORDER BY sort_order"
    ).fetchall()

    expected_count = len(area.resets)
    actual_count = len(db_rows)
    if expected_count != actual_count:
        mismatches.append(f"  resets count: expected {expected_count}, got {actual_count}")

    for i in range(min(expected_count, actual_count)):
        reset = area.resets[i]
        row = db_rows[i]
        checks = [
            ("command", reset.command, row[0]),
            ("arg1", reset.arg1, row[1]),
            ("arg2", reset.arg2, row[2]),
            ("arg3", reset.arg3, row[3]),
        ]
        for name, expected, actual in checks:
            m = compare_values(f"reset[{i}].{name}", expected, actual)
            if m:
                mismatches.append(m)

    return mismatches


def verify_shops(conn, area):
    """Verify shops table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT keeper_vnum, buy_type0, buy_type1, buy_type2, buy_type3, buy_type4, "
        "profit_buy, profit_sell, open_hour, close_hour FROM shops ORDER BY keeper_vnum"
    ).fetchall()
    db_by_keeper = {r[0]: r for r in db_rows}

    expected_count = len(area.shops)
    actual_count = len(db_rows)
    if expected_count != actual_count:
        mismatches.append(f"  shops count: expected {expected_count}, got {actual_count}")

    for shop in area.shops:
        row = db_by_keeper.get(shop.keeper_vnum)
        if row is None:
            mismatches.append(f"  shop keeper {shop.keeper_vnum}: missing from db")
            continue

        # Pad buy_types to 5 with zeros
        bt = shop.buy_types + [0] * (5 - len(shop.buy_types))
        checks = [
            ("buy_type0", bt[0], row[1]),
            ("buy_type1", bt[1], row[2]),
            ("buy_type2", bt[2], row[3]),
            ("buy_type3", bt[3], row[4]),
            ("buy_type4", bt[4], row[5]),
            ("profit_buy", shop.profit_buy, row[6]),
            ("profit_sell", shop.profit_sell, row[7]),
            ("open_hour", shop.open_hour, row[8]),
            ("close_hour", shop.close_hour, row[9]),
        ]
        for name, expected, actual in checks:
            m = compare_values(f"shop {shop.keeper_vnum}.{name}", expected, actual)
            if m:
                mismatches.append(m)

    return mismatches


def verify_specials(conn, area):
    """Verify specials table."""
    mismatches = []

    db_rows = conn.execute(
        "SELECT mob_vnum, spec_fun_name FROM specials ORDER BY mob_vnum"
    ).fetchall()
    db_by_vnum = {r[0]: r[1] for r in db_rows}

    expected_count = len(area.specials)
    actual_count = len(db_rows)
    if expected_count != actual_count:
        mismatches.append(f"  specials count: expected {expected_count}, got {actual_count}")

    for mob_vnum, spec_fun in area.specials.items():
        db_fun = db_by_vnum.get(mob_vnum)
        if db_fun is None:
            mismatches.append(f"  special mob {mob_vnum}: missing from db")
            continue
        m = compare_values(f"special {mob_vnum}.spec_fun_name", spec_fun, db_fun)
        if m:
            mismatches.append(m)

    for mob_vnum in db_by_vnum:
        if mob_vnum not in area.specials:
            mismatches.append(f"  special mob {mob_vnum}: in db but not in .are")

    return mismatches


def verify_area(area_path, db_path):
    """Verify a single area file against its db. Returns (passed, mismatches)."""
    area = parse_area_file(area_path)
    if area is None:
        return False, ["  Failed to parse .are file"]

    if not db_path.exists():
        return False, ["  .db file not found"]

    conn = sqlite3.connect(str(db_path))
    all_mismatches = []

    all_mismatches.extend(verify_area_table(conn, area))
    all_mismatches.extend(verify_mobiles(conn, area))
    all_mismatches.extend(verify_objects(conn, area))
    all_mismatches.extend(verify_object_affects(conn, area))
    all_mismatches.extend(verify_extra_descriptions(conn, area))
    all_mismatches.extend(verify_rooms(conn, area))
    all_mismatches.extend(verify_exits(conn, area))
    all_mismatches.extend(verify_room_texts(conn, area))
    all_mismatches.extend(verify_resets(conn, area))
    all_mismatches.extend(verify_shops(conn, area))
    all_mismatches.extend(verify_specials(conn, area))

    conn.close()
    passed = len(all_mismatches) == 0
    return passed, all_mismatches


def main():
    parser = argparse.ArgumentParser(description="Verify .are <-> .db migration")
    parser.add_argument('--area-dir', type=Path, default=None)
    parser.add_argument('--db-dir', type=Path, default=None)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent.parent
    area_dir = args.area_dir or repo_root / 'gamedata' / 'area'
    db_dir = args.db_dir or repo_root / 'gamedata' / 'db' / 'areas'

    if not area_dir.exists():
        print(f"Error: area directory not found: {area_dir}")
        sys.exit(1)
    if not db_dir.exists():
        print(f"Error: db directory not found: {db_dir}")
        sys.exit(1)

    area_list = area_dir / 'area.lst'
    if not area_list.exists():
        print(f"Error: area.lst not found in {area_dir}")
        sys.exit(1)

    with open(area_list, 'r') as f:
        area_files = [line.strip() for line in f if line.strip() and not line.startswith('$')]

    # Skip help.are
    area_files = [af for af in area_files if af.lower() != 'help.are']

    total = 0
    passed = 0
    failed = 0
    skipped = 0
    all_failures = {}

    print("=" * 70)
    print("Migration Verification: .are vs .db")
    print("=" * 70)
    print()

    for area_file in area_files:
        filepath = area_dir / area_file
        if not filepath.exists():
            print(f"  SKIP  {area_file} (file not found)")
            skipped += 1
            continue

        basename = filepath.stem
        db_path = db_dir / f"{basename}.db"
        total += 1

        ok, mismatches = verify_area(filepath, db_path)

        if ok:
            print(f"  PASS  {area_file}")
            passed += 1
        else:
            print(f"  FAIL  {area_file}  ({len(mismatches)} mismatch(es))")
            for m in mismatches:
                print(m)
            failed += 1
            all_failures[area_file] = mismatches

    print()
    print("=" * 70)
    print("Summary")
    print("=" * 70)
    print(f"  Total areas checked: {total}")
    print(f"  Passed:  {passed}")
    print(f"  Failed:  {failed}")
    print(f"  Skipped: {skipped}")
    print()

    if all_failures:
        print(f"Failed areas ({len(all_failures)}):")
        for af, mm in all_failures.items():
            print(f"  {af}: {len(mm)} mismatch(es)")
        sys.exit(1)
    else:
        print("All areas PASSED verification.")
        sys.exit(0)


if __name__ == '__main__':
    main()
