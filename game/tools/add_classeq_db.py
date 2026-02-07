#!/usr/bin/env python3
"""
add_classeq_db.py - Add class equipment to classeq.db (areas database)

Usage:
    python add_classeq_db.py                    # Run with default equipment data
    python add_classeq_db.py --dry-run          # Show what would be inserted
    python add_classeq_db.py --delete CLASS_ID  # Delete equipment for a class
    python add_classeq_db.py --list-range START END  # List vnums in range

This script inserts equipment objects and their affects into classeq.db.
Modify the EQUIPMENT_DATA section at the bottom to define your equipment.

Database Tables:
    objects         - Object definitions (vnum, name, stats, wear_flags)
    object_affects  - Stat modifiers (hitroll, damroll, AC)

REQUIRED: Each class equipment set MUST include:
    - vnum_start    - Starting vnum for regular equipment pieces
    - pieces        - List of armor/weapon pieces (13 standard slots)
    - mastery_vnum  - Vnum for the mastery item (separate from piece range)
    - mastery       - Mastery item definition (held item with enhanced stats)

The mastery_vnum should also be set in class.db via add_class_db.py in the
armor_config section to enable the mastery command.
"""

import sqlite3
import argparse
import os
import sys
from pathlib import Path


# =============================================================================
# CONSTANTS
# =============================================================================

# Item types (from merc.h)
ITEM_WEAPON = 5
ITEM_ARMOR = 9

# Wear flags (bit flags, combine with |)
WEAR_TAKE   = 1
WEAR_FINGER = 2       # + TAKE = 3
WEAR_NECK   = 4       # + TAKE = 5
WEAR_BODY   = 8       # + TAKE = 9
WEAR_HEAD   = 16      # + TAKE = 17
WEAR_LEGS   = 32      # + TAKE = 33
WEAR_FEET   = 64      # + TAKE = 65
WEAR_HANDS  = 128     # + TAKE = 129
WEAR_ARMS   = 256     # + TAKE = 257
WEAR_ABOUT  = 1024    # + TAKE = 1025
WEAR_WAIST  = 2048    # + TAKE = 2049
WEAR_WRIST  = 4096    # + TAKE = 4097
WEAR_HOLD   = 512     # + TAKE = 513
WEAR_WIELD  = 8192    # + TAKE = 8193
WEAR_FACE   = 32768   # + TAKE = 32769

# Affect locations (from merc.h)
APPLY_AC      = 17
APPLY_HITROLL = 18
APPLY_DAMROLL = 19

# Weapon values (value0-3 for item_type=5)
# value0 = min damage, value1 = max damage, value2 = attack type, value3 = special


def find_classeq_db():
    """Find the classeq.db file relative to script location."""
    script_dir = Path(__file__).parent
    candidates = [
        script_dir.parent.parent / "gamedata" / "db" / "areas" / "classeq.db",
        script_dir / ".." / ".." / "gamedata" / "db" / "areas" / "classeq.db",
        Path("gamedata/db/areas/classeq.db"),
    ]
    for path in candidates:
        if path.exists():
            return str(path.resolve())
    raise FileNotFoundError("Could not find classeq.db")


def insert_equipment(conn, equipment_data, dry_run=False):
    """Insert equipment objects and affects for a class."""
    cursor = conn.cursor()
    class_name = equipment_data["class_name"]
    vnum_start = equipment_data["vnum_start"]

    print(f"\n{'[DRY RUN] ' if dry_run else ''}Adding equipment for: {class_name}")

    for i, piece in enumerate(equipment_data["pieces"]):
        vnum = vnum_start + i

        # Build object record
        obj = {
            "vnum": vnum,
            "name": piece["name"],
            "short_descr": piece["short_descr"],
            "description": piece.get("description", f"A piece of {class_name.lower()} equipment lies here."),
            "item_type": piece.get("item_type", ITEM_ARMOR),
            "extra_flags": piece.get("extra_flags", 0),
            "wear_flags": piece["wear_flags"],
            "value0": piece.get("value0", 25),
            "value1": piece.get("value1", 0),
            "value2": piece.get("value2", 0),
            "value3": piece.get("value3", 0),
            "weight": piece.get("weight", 1),
            "cost": piece.get("cost", 0),
        }

        # Weapon values
        if piece.get("item_type") == ITEM_WEAPON:
            obj["value0"] = piece.get("dam_min", 18000)
            obj["value1"] = piece.get("dam_max", 50)
            obj["value2"] = piece.get("dam_type", 75)
            obj["value3"] = piece.get("dam_special", 1)

        print(f"  [{vnum}] {piece['keyword']}: {piece['short_descr']}")

        if not dry_run:
            # Insert or replace object
            sql = """INSERT OR REPLACE INTO objects
                     (vnum, name, short_descr, description, item_type, extra_flags,
                      wear_flags, value0, value1, value2, value3, weight, cost)
                     VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"""
            cursor.execute(sql, (
                obj["vnum"], obj["name"], obj["short_descr"], obj["description"],
                obj["item_type"], obj["extra_flags"], obj["wear_flags"],
                obj["value0"], obj["value1"], obj["value2"], obj["value3"],
                obj["weight"], obj["cost"]
            ))

            # Delete existing affects for this vnum
            cursor.execute("DELETE FROM object_affects WHERE obj_vnum = ?", (vnum,))

            # Insert affects
            affects = piece.get("affects", [])
            for sort_order, affect in enumerate(affects):
                sql = """INSERT INTO object_affects (obj_vnum, location, modifier, sort_order)
                         VALUES (?, ?, ?, ?)"""
                cursor.execute(sql, (vnum, affect["location"], affect["modifier"], sort_order))

    # Handle mastery item if present
    mastery = equipment_data.get("mastery")
    mastery_vnum = equipment_data.get("mastery_vnum")
    if mastery and mastery_vnum:
        obj = {
            "vnum": mastery_vnum,
            "name": mastery["name"],
            "short_descr": mastery["short_descr"],
            "description": f"A powerful {class_name.lower()} mastery item radiates energy.",
            "item_type": mastery.get("item_type", ITEM_ARMOR),
            "extra_flags": mastery.get("extra_flags", 0),
            "wear_flags": mastery["wear_flags"],
            "value0": mastery.get("value0", 25),
            "value1": 0, "value2": 0, "value3": 0,
            "weight": 1, "cost": 0,
        }
        print(f"  [{mastery_vnum}] MASTERY: {mastery['short_descr']}")

        if not dry_run:
            sql = """INSERT OR REPLACE INTO objects
                     (vnum, name, short_descr, description, item_type, extra_flags,
                      wear_flags, value0, value1, value2, value3, weight, cost)
                     VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"""
            cursor.execute(sql, (
                obj["vnum"], obj["name"], obj["short_descr"], obj["description"],
                obj["item_type"], obj["extra_flags"], obj["wear_flags"],
                obj["value0"], obj["value1"], obj["value2"], obj["value3"],
                obj["weight"], obj["cost"]
            ))
            cursor.execute("DELETE FROM object_affects WHERE obj_vnum = ?",
                           (mastery_vnum,))
            for sort_order, affect in enumerate(mastery.get("affects", [])):
                sql = """INSERT INTO object_affects (obj_vnum, location, modifier, sort_order)
                         VALUES (?, ?, ?, ?)"""
                cursor.execute(sql, (mastery_vnum, affect["location"],
                               affect["modifier"], sort_order))

    if not dry_run:
        conn.commit()

    piece_count = len(equipment_data['pieces'])
    mastery_str = " + mastery" if mastery else ""
    print(f"{'[DRY RUN] ' if dry_run else ''}Done: {piece_count} pieces{mastery_str}")


def delete_equipment(conn, vnum_start, vnum_end, dry_run=False):
    """Delete equipment in a vnum range."""
    cursor = conn.cursor()

    print(f"\n{'[DRY RUN] ' if dry_run else ''}Deleting vnums {vnum_start}-{vnum_end}")

    if not dry_run:
        cursor.execute("DELETE FROM object_affects WHERE obj_vnum BETWEEN ? AND ?",
                      (vnum_start, vnum_end))
        cursor.execute("DELETE FROM objects WHERE vnum BETWEEN ? AND ?",
                      (vnum_start, vnum_end))
        conn.commit()

    print(f"{'[DRY RUN] ' if dry_run else ''}Done")


def list_vnums(conn, vnum_start, vnum_end):
    """List objects in a vnum range."""
    cursor = conn.cursor()
    cursor.execute("SELECT vnum, name, short_descr FROM objects WHERE vnum BETWEEN ? AND ? ORDER BY vnum",
                  (vnum_start, vnum_end))
    rows = cursor.fetchall()

    if not rows:
        print(f"No objects found in range {vnum_start}-{vnum_end}")
        return

    print(f"\nObjects in range {vnum_start}-{vnum_end}:")
    for row in rows:
        print(f"  [{row[0]}] {row[1]}: {row[2]}")


# =============================================================================
# EQUIPMENT DATA - Modify this section to add your equipment
# =============================================================================

# Helper to create standard armor piece
def armor_piece(keyword, name, short_descr, wear_flags, hitroll, damroll, ac,
                item_type=ITEM_ARMOR, **kwargs):
    """Create a standard armor piece with hitroll/damroll/AC."""
    piece = {
        "keyword": keyword,
        "name": name,
        "short_descr": short_descr,
        "wear_flags": wear_flags,
        "item_type": item_type,
        "affects": [
            {"location": APPLY_HITROLL, "modifier": hitroll},
            {"location": APPLY_DAMROLL, "modifier": damroll},
            {"location": APPLY_AC, "modifier": ac},
        ],
        **kwargs
    }
    # Remove AC affect for weapons (only hitroll/damroll)
    if item_type == ITEM_WEAPON:
        piece["affects"] = [a for a in piece["affects"] if a["location"] != APPLY_AC]
    return piece


def weapon_piece(keyword, name, short_descr, hitroll, damroll,
                 dam_min=18000, dam_max=50, dam_type=75, dam_special=1):
    """Create a weapon piece."""
    return armor_piece(
        keyword, name, short_descr,
        wear_flags=WEAR_TAKE | WEAR_WIELD,
        hitroll=hitroll, damroll=damroll, ac=0,
        item_type=ITEM_WEAPON,
        dam_min=dam_min, dam_max=dam_max, dam_type=dam_type, dam_special=dam_special
    )


def mastery_piece(keyword, name, short_descr, hitroll, damroll):
    """Create a mastery item (held item with enhanced stats, no AC)."""
    return armor_piece(
        keyword, name, short_descr,
        wear_flags=WEAR_TAKE | WEAR_HOLD,
        hitroll=hitroll, damroll=damroll, ac=0,
        item_type=ITEM_ARMOR
    )


# Dragonkin Equipment (vnums 33400-33412, mastery at 33415)
# Stats: Weapon +35/+35, Armor +30/+30/-35 AC, Mastery +50/+50
DRAGONKIN_EQUIPMENT = {
    "class_name": "Dragonkin",
    "vnum_start": 33400,
    "mastery_vnum": 33415,
    "pieces": [
        weapon_piece("dragonkin fang", "dragonkin fang draconic",
                    "#x202<*#x220a draconic fang#x202*>#n", 35, 35),
        armor_piece("dragonkin ring scale", "dragonkin ring scale",
                   "#x202<*#x220a scale ring#x202*>#n", WEAR_TAKE | WEAR_FINGER, 30, 30, -35),
        armor_piece("dragonkin tooth pendant", "dragonkin tooth pendant",
                   "#x202<*#x220a dragon tooth pendant#x202*>#n", WEAR_TAKE | WEAR_NECK, 30, 30, -35),
        armor_piece("dragonkin scales breastplate", "dragonkin scales breastplate",
                   "#x202<*#x220a dragonscale breastplate#x202*>#n", WEAR_TAKE | WEAR_BODY, 30, 30, -35),
        armor_piece("dragonkin helm horned", "dragonkin helm horned",
                   "#x202<*#x220a horned dragon helm#x202*>#n", WEAR_TAKE | WEAR_HEAD, 30, 30, -35),
        armor_piece("dragonkin leggings scaled", "dragonkin leggings scaled",
                   "#x202<*#x220scaled dragonkin leggings#x202*>#n", WEAR_TAKE | WEAR_LEGS, 30, 30, -35),
        armor_piece("dragonkin boots clawed", "dragonkin boots clawed",
                   "#x202<*#x220clawed dragon boots#x202*>#n", WEAR_TAKE | WEAR_FEET, 30, 30, -35),
        armor_piece("dragonkin gauntlets taloned", "dragonkin gauntlets taloned",
                   "#x202<*#x220taloned dragon gauntlets#x202*>#n", WEAR_TAKE | WEAR_HANDS, 30, 30, -35),
        armor_piece("dragonkin bracers scaled", "dragonkin bracers scaled",
                   "#x202<*#x220scaled dragon bracers#x202*>#n", WEAR_TAKE | WEAR_ARMS, 30, 30, -35),
        armor_piece("dragonkin cloak wings", "dragonkin cloak wings",
                   "#x202<*#x220a cloak of dragon wings#x202*>#n", WEAR_TAKE | WEAR_ABOUT, 30, 30, -35),
        armor_piece("dragonkin belt scale", "dragonkin belt scale",
                   "#x202<*#x220a dragonscale belt#x202*>#n", WEAR_TAKE | WEAR_WAIST, 30, 30, -35),
        armor_piece("dragonkin bracer wrist", "dragonkin bracer wrist",
                   "#x202<*#x220a dragon wristguard#x202*>#n", WEAR_TAKE | WEAR_WRIST, 30, 30, -35),
        armor_piece("dragonkin visage mask", "dragonkin visage mask",
                   "#x202<*#x220a draconic visage#x202*>#n", WEAR_TAKE | WEAR_FACE, 30, 30, -35),
    ],
    "mastery": mastery_piece("dragonkin heart shard", "dragonkin heart shard dragon",
                             "#x202<*#x220a pulsing dragon heart shard#x202*>#n", 50, 50),
}

# Wyrm Equipment (vnums 33420-33432)
# Stats: Weapon +45/+45, Armor +40/+40/-45 AC, Mastery +75/+75
WYRM_EQUIPMENT = {
    "class_name": "Wyrm",
    "vnum_start": 33420,
    "mastery_vnum": 33435,
    "pieces": [
        weapon_piece("wyrm talon ancient", "wyrm talon ancient",
                    "#x202<#x220*an ancient wyrm talon*#x202>#n", 45, 45),
        armor_piece("wyrm ring primordial", "wyrm ring primordial",
                   "#x202<#x220*a primordial scale ring*#x202>#n", WEAR_TAKE | WEAR_FINGER, 40, 40, -45),
        armor_piece("wyrm fang pendant", "wyrm fang pendant",
                   "#x202<#x220*an ancient wyrm fang pendant*#x202>#n", WEAR_TAKE | WEAR_NECK, 40, 40, -45),
        armor_piece("wyrm scales carapace", "wyrm scales carapace",
                   "#x202<#x220*an ancient wyrm carapace*#x202>#n", WEAR_TAKE | WEAR_BODY, 40, 40, -45),
        armor_piece("wyrm helm crowned", "wyrm helm crowned",
                   "#x202<#x220*a crowned wyrm helm*#x202>#n", WEAR_TAKE | WEAR_HEAD, 40, 40, -45),
        armor_piece("wyrm leggings ancient", "wyrm leggings ancient",
                   "#x202<#x220*ancient wyrm leggings*#x202>#n", WEAR_TAKE | WEAR_LEGS, 40, 40, -45),
        armor_piece("wyrm boots clawed", "wyrm boots clawed",
                   "#x202<#x220*clawed wyrm boots*#x202>#n", WEAR_TAKE | WEAR_FEET, 40, 40, -45),
        armor_piece("wyrm gauntlets taloned", "wyrm gauntlets taloned",
                   "#x202<#x220*taloned wyrm gauntlets*#x202>#n", WEAR_TAKE | WEAR_HANDS, 40, 40, -45),
        armor_piece("wyrm bracers scaled", "wyrm bracers scaled",
                   "#x202<#x220*ancient wyrm bracers*#x202>#n", WEAR_TAKE | WEAR_ARMS, 40, 40, -45),
        armor_piece("wyrm cloak wings", "wyrm cloak wings",
                   "#x202<#x220*a cloak of wyrm wings*#x202>#n", WEAR_TAKE | WEAR_ABOUT, 40, 40, -45),
        armor_piece("wyrm belt ancient", "wyrm belt ancient",
                   "#x202<#x220*an ancient wyrm belt*#x202>#n", WEAR_TAKE | WEAR_WAIST, 40, 40, -45),
        armor_piece("wyrm bracer wrist", "wyrm bracer wrist",
                   "#x202<#x220*an ancient wyrm wristguard*#x202>#n", WEAR_TAKE | WEAR_WRIST, 40, 40, -45),
        armor_piece("wyrm visage mask", "wyrm visage mask",
                   "#x202<#x220*an ancient wyrm visage*#x202>#n", WEAR_TAKE | WEAR_FACE, 40, 40, -45),
    ],
    "mastery": mastery_piece("wyrm primordial core", "wyrm primordial dragon core",
                             "#x202<#x220*a primordial dragon core*#x202>#n", 75, 75),
}

EQUIPMENT_TO_ADD = [
    DRAGONKIN_EQUIPMENT,
    WYRM_EQUIPMENT,
]


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description="Add class equipment to classeq.db")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be done without making changes")
    parser.add_argument("--delete", nargs=2, type=int, metavar=("START", "END"),
                        help="Delete equipment in vnum range")
    parser.add_argument("--list-range", nargs=2, type=int, metavar=("START", "END"),
                        help="List objects in vnum range")
    parser.add_argument("--db", type=str, help="Path to classeq.db")
    args = parser.parse_args()

    try:
        db_path = args.db or find_classeq_db()
    except FileNotFoundError as e:
        print(f"Error: {e}")
        sys.exit(1)

    print(f"Using database: {db_path}")
    conn = sqlite3.connect(db_path)

    try:
        if args.list_range:
            list_vnums(conn, args.list_range[0], args.list_range[1])
        elif args.delete:
            delete_equipment(conn, args.delete[0], args.delete[1], dry_run=args.dry_run)
        else:
            for equipment_data in EQUIPMENT_TO_ADD:
                insert_equipment(conn, equipment_data, dry_run=args.dry_run)
    finally:
        conn.close()

    print("\nComplete!")


if __name__ == "__main__":
    main()
