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

# Artificer Equipment (vnums 33440-33452, mastery at 33455)
# Stats: Weapon +35/+35, Armor +30/+30/-35 AC, Mastery +50/+50 (base class = Dragonkin tier)
# Colors: accent #x037 (dark teal), primary #x117 (sky blue)
ARTIFICER_EQUIPMENT = {
    "class_name": "Artificer",
    "vnum_start": 33440,
    "mastery_vnum": 33455,
    "pieces": [
        weapon_piece("artificer wrench power", "artificer wrench power",
                    "#x037[=#x117a crackling power wrench#x037=]#n", 35, 35),
        armor_piece("artificer ring circuit", "artificer ring circuit",
                   "#x037[=#x117a glowing circuit ring#x037=]#n", WEAR_TAKE | WEAR_FINGER, 30, 30, -35),
        armor_piece("artificer collar tech", "artificer collar tech",
                   "#x037[=#x117a humming tech collar#x037=]#n", WEAR_TAKE | WEAR_NECK, 30, 30, -35),
        armor_piece("artificer vest engineer", "artificer vest engineer",
                   "#x037[=#x117a reinforced engineer vest#x037=]#n", WEAR_TAKE | WEAR_BODY, 30, 30, -35),
        armor_piece("artificer goggles magnifying", "artificer goggles magnifying",
                   "#x037[=#x117a pair of magnifying goggles#x037=]#n", WEAR_TAKE | WEAR_HEAD, 30, 30, -35),
        armor_piece("artificer pants utility", "artificer pants utility",
                   "#x037[=#x117a pair of utility pants#x037=]#n", WEAR_TAKE | WEAR_LEGS, 30, 30, -35),
        armor_piece("artificer boots mag", "artificer boots mag",
                   "#x037[=#x117a pair of mag boots#x037=]#n", WEAR_TAKE | WEAR_FEET, 30, 30, -35),
        armor_piece("artificer gloves work", "artificer gloves work",
                   "#x037[=#x117a pair of insulated work gloves#x037=]#n", WEAR_TAKE | WEAR_HANDS, 30, 30, -35),
        armor_piece("artificer bracers tool", "artificer bracers tool",
                   "#x037[=#x117a set of tool bracers#x037=]#n", WEAR_TAKE | WEAR_ARMS, 30, 30, -35),
        armor_piece("artificer harness equipment", "artificer harness equipment",
                   "#x037[=#x117a heavy equipment harness#x037=]#n", WEAR_TAKE | WEAR_ABOUT, 30, 30, -35),
        armor_piece("artificer belt tool", "artificer belt tool",
                   "#x037[=#x117a loaded tool belt#x037=]#n", WEAR_TAKE | WEAR_WAIST, 30, 30, -35),
        armor_piece("artificer wrist computer", "artificer wrist computer",
                   "#x037[=#x117a blinking wrist computer#x037=]#n", WEAR_TAKE | WEAR_WRIST, 30, 30, -35),
        armor_piece("artificer mask welding", "artificer mask welding",
                   "#x037[=#x117a darkened welding mask#x037=]#n", WEAR_TAKE | WEAR_FACE, 30, 30, -35),
    ],
    "mastery": mastery_piece("artificer power core shard", "artificer power core shard",
                             "#x037[=#x117a pulsing power core shard#x037=]#n", 50, 50),
}

# Mechanist Equipment (vnums 33460-33472, mastery at 33475)
# Stats: Weapon +45/+45, Armor +40/+40/-45 AC, Mastery +75/+75 (upgrade class = Wyrm tier)
# Colors: accent #x127 (dark magenta), primary #x177 (violet)
MECHANIST_EQUIPMENT = {
    "class_name": "Mechanist",
    "vnum_start": 33460,
    "mastery_vnum": 33475,
    "pieces": [
        weapon_piece("mechanist cutter plasma", "mechanist cutter plasma",
                    "#x127>/#x177a searing plasma cutter#x127\\<#n", 45, 45),
        armor_piece("mechanist ring data", "mechanist ring data",
                   "#x127>/#x177a pulsing data ring#x127\\<#n", WEAR_TAKE | WEAR_FINGER, 40, 40, -45),
        armor_piece("mechanist collar neural", "mechanist collar neural",
                   "#x127>/#x177a thrumming neural collar#x127\\<#n", WEAR_TAKE | WEAR_NECK, 40, 40, -45),
        armor_piece("mechanist chassis combat", "mechanist chassis combat",
                   "#x127>/#x177a hulking combat chassis#x127\\<#n", WEAR_TAKE | WEAR_BODY, 40, 40, -45),
        armor_piece("mechanist helm targeting", "mechanist helm targeting",
                   "#x127>/#x177a scanning targeting helm#x127\\<#n", WEAR_TAKE | WEAR_HEAD, 40, 40, -45),
        armor_piece("mechanist leggings servo", "mechanist leggings servo",
                   "#x127>/#x177a pair of servo leggings#x127\\<#n", WEAR_TAKE | WEAR_LEGS, 40, 40, -45),
        armor_piece("mechanist boots thruster", "mechanist boots thruster",
                   "#x127>/#x177a pair of thruster boots#x127\\<#n", WEAR_TAKE | WEAR_FEET, 40, 40, -45),
        armor_piece("mechanist gauntlets power", "mechanist gauntlets power",
                   "#x127>/#x177a pair of power gauntlets#x127\\<#n", WEAR_TAKE | WEAR_HANDS, 40, 40, -45),
        armor_piece("mechanist mounts weapon", "mechanist mounts weapon",
                   "#x127>/#x177a set of weapon mounts#x127\\<#n", WEAR_TAKE | WEAR_ARMS, 40, 40, -45),
        armor_piece("mechanist harness drone", "mechanist harness drone",
                   "#x127>/#x177a drone control harness#x127\\<#n", WEAR_TAKE | WEAR_ABOUT, 40, 40, -45),
        armor_piece("mechanist belt ammo", "mechanist belt ammo",
                   "#x127>/#x177a heavy ammo belt#x127\\<#n", WEAR_TAKE | WEAR_WAIST, 40, 40, -45),
        armor_piece("mechanist holo display", "mechanist holo display",
                   "#x127>/#x177a flickering holo-display#x127\\<#n", WEAR_TAKE | WEAR_WRIST, 40, 40, -45),
        armor_piece("mechanist visor tactical", "mechanist visor tactical",
                   "#x127>/#x177a glowing tactical visor#x127\\<#n", WEAR_TAKE | WEAR_FACE, 40, 40, -45),
    ],
    "mastery": mastery_piece("mechanist core implant", "mechanist core implant",
                             "#x127>/#x177a thrumming mechanist core#x127\\<#n", 75, 75),
}

# Cultist Equipment (vnums 33500-33512, mastery at 33515)
# Stats: Weapon +35/+35, Armor +30/+30/-35 AC, Mastery +50/+50 (base class = Dragonkin tier)
# Colors: accent #x064 (dark olive), primary #x120 (lime green)
CULTIST_EQUIPMENT = {
    "class_name": "Cultist",
    "vnum_start": 33500,
    "mastery_vnum": 33515,
    "pieces": [
        weapon_piece("cultist void staff", "cultist void staff eldritch",
                    "#x064{~#x120a crackling void staff#x064~}#n", 35, 35),
        armor_piece("cultist sigil ring", "cultist sigil ring",
                   "#x064{~#x120a void sigil ring#x064~}#n", WEAR_TAKE | WEAR_FINGER, 30, 30, -35),
        armor_piece("cultist eldritch amulet", "cultist eldritch amulet",
                   "#x064{~#x120an eldritch amulet#x064~}#n", WEAR_TAKE | WEAR_NECK, 30, 30, -35),
        armor_piece("cultist robes void", "cultist robes void",
                   "#x064{~#x120void-touched cultist robes#x064~}#n", WEAR_TAKE | WEAR_BODY, 30, 30, -35),
        armor_piece("cultist hood whispers", "cultist hood whispers",
                   "#x064{~#x120a hood of whispers#x064~}#n", WEAR_TAKE | WEAR_HEAD, 30, 30, -35),
        armor_piece("cultist ritual leggings", "cultist ritual leggings",
                   "#x064{~#x120ritual leggings#x064~}#n", WEAR_TAKE | WEAR_LEGS, 30, 30, -35),
        armor_piece("cultist boots void-touched", "cultist boots void-touched",
                   "#x064{~#x120void-touched boots#x064~}#n", WEAR_TAKE | WEAR_FEET, 30, 30, -35),
        armor_piece("cultist sacrificial gloves", "cultist sacrificial gloves",
                   "#x064{~#x120sacrificial gloves#x064~}#n", WEAR_TAKE | WEAR_HANDS, 30, 30, -35),
        armor_piece("cultist bracers binding", "cultist bracers binding",
                   "#x064{~#x120bracers of binding#x064~}#n", WEAR_TAKE | WEAR_ARMS, 30, 30, -35),
        armor_piece("cultist shroud madness", "cultist shroud madness",
                   "#x064{~#x120a shroud of madness#x064~}#n", WEAR_TAKE | WEAR_ABOUT, 30, 30, -35),
        armor_piece("cultist ritual sash", "cultist ritual sash",
                   "#x064{~#x120a ritual sash#x064~}#n", WEAR_TAKE | WEAR_WAIST, 30, 30, -35),
        armor_piece("cultist tentacle bangle", "cultist tentacle bangle",
                   "#x064{~#x120a tentacle bangle#x064~}#n", WEAR_TAKE | WEAR_WRIST, 30, 30, -35),
        armor_piece("cultist mask deep", "cultist mask deep",
                   "#x064{~#x120a mask of the deep#x064~}#n", WEAR_TAKE | WEAR_FACE, 30, 30, -35),
    ],
    "mastery": mastery_piece("cultist void heart", "cultist void heart eldritch",
                             "#x064{~#x120a pulsing void heart#x064~}#n", 50, 50),
}

# Voidborn Equipment (vnums 33520-33532, mastery at 33535)
# Stats: Weapon +45/+45, Armor +40/+40/-45 AC, Mastery +75/+75 (upgrade class = Wyrm tier)
# Colors: accent #x055 (dark violet), primary #x097 (eldritch purple)
VOIDBORN_EQUIPMENT = {
    "class_name": "Voidborn",
    "vnum_start": 33520,
    "mastery_vnum": 33535,
    "pieces": [
        weapon_piece("voidborn scepter void", "voidborn scepter void",
                    "#x055*(#x097a void scepter of unmaking#x055)*#n", 45, 45),
        armor_piece("voidborn ring unmaking", "voidborn ring unmaking",
                   "#x055*(#x097a ring of unmaking#x055)*#n", WEAR_TAKE | WEAR_FINGER, 40, 40, -45),
        armor_piece("voidborn collar stars", "voidborn collar stars",
                   "#x055*(#x097a collar of distant stars#x055)*#n", WEAR_TAKE | WEAR_NECK, 40, 40, -45),
        armor_piece("voidborn aberrant robes", "voidborn aberrant robes",
                   "#x055*(#x097aberrant void robes#x055)*#n", WEAR_TAKE | WEAR_BODY, 40, 40, -45),
        armor_piece("voidborn crown madness", "voidborn crown madness",
                   "#x055*(#x097a crown of madness#x055)*#n", WEAR_TAKE | WEAR_HEAD, 40, 40, -45),
        armor_piece("voidborn leggings void-woven", "voidborn leggings void-woven",
                   "#x055*(#x097void-woven leggings#x055)*#n", WEAR_TAKE | WEAR_LEGS, 40, 40, -45),
        armor_piece("voidborn phasing boots", "voidborn phasing boots",
                   "#x055*(#x097phasing boots#x055)*#n", WEAR_TAKE | WEAR_FEET, 40, 40, -45),
        armor_piece("voidborn tentacle gauntlets", "voidborn tentacle gauntlets",
                   "#x055*(#x097tentacle gauntlets#x055)*#n", WEAR_TAKE | WEAR_HANDS, 40, 40, -45),
        armor_piece("voidborn dimensional bracers", "voidborn dimensional bracers",
                   "#x055*(#x097dimensional bracers#x055)*#n", WEAR_TAKE | WEAR_ARMS, 40, 40, -45),
        armor_piece("voidborn shroud entropy", "voidborn shroud entropy",
                   "#x055*(#x097a shroud of entropy#x055)*#n", WEAR_TAKE | WEAR_ABOUT, 40, 40, -45),
        armor_piece("voidborn sash binding", "voidborn sash binding",
                   "#x055*(#x097a sash of binding#x055)*#n", WEAR_TAKE | WEAR_WAIST, 40, 40, -45),
        armor_piece("voidborn rift bangle", "voidborn rift bangle",
                   "#x055*(#x097a rift bangle#x055)*#n", WEAR_TAKE | WEAR_WRIST, 40, 40, -45),
        armor_piece("voidborn mask void", "voidborn mask void",
                   "#x055*(#x097a mask of the void#x055)*#n", WEAR_TAKE | WEAR_FACE, 40, 40, -45),
    ],
    "mastery": mastery_piece("voidborn heart void", "voidborn heart void eldritch",
                             "#x055*(#x097a pulsing heart of the void#x055)*#n", 75, 75),
}

# Chronomancer Equipment (vnums 33540-33552, mastery at 33555)
# Stats: Weapon +35/+35, Armor +30/+30/-35 AC, Mastery +50/+50 (base class = Dragonkin tier)
# Colors: accent #x130 (deep copper), primary #x215 (warm amber)
CHRONOMANCER_EQUIPMENT = {
    "class_name": "Chronomancer",
    "vnum_start": 33540,
    "mastery_vnum": 33555,
    "pieces": [
        weapon_piece("chronomancer staff temporal", "chronomancer staff temporal",
                    "#x130[>#x215a temporal staff#x130<]#n", 35, 35),
        armor_piece("chronomancer ring hourglass", "chronomancer ring hourglass",
                   "#x130[>#x215an hourglass ring#x130<]#n", WEAR_TAKE | WEAR_FINGER, 30, 30, -35),
        armor_piece("chronomancer amulet timepiece", "chronomancer amulet timepiece",
                   "#x130[>#x215a timepiece amulet#x130<]#n", WEAR_TAKE | WEAR_NECK, 30, 30, -35),
        armor_piece("chronomancer robes flux", "chronomancer robes flux",
                   "#x130[>#x215flux robes#x130<]#n", WEAR_TAKE | WEAR_BODY, 30, 30, -35),
        armor_piece("chronomancer circlet chrono", "chronomancer circlet chrono",
                   "#x130[>#x215a chrono circlet#x130<]#n", WEAR_TAKE | WEAR_HEAD, 30, 30, -35),
        armor_piece("chronomancer pants time-touched", "chronomancer pants time-touched",
                   "#x130[>#x215time-touched pants#x130<]#n", WEAR_TAKE | WEAR_LEGS, 30, 30, -35),
        armor_piece("chronomancer boots phasing", "chronomancer boots phasing",
                   "#x130[>#x215phasing boots#x130<]#n", WEAR_TAKE | WEAR_FEET, 30, 30, -35),
        armor_piece("chronomancer gloves temporal", "chronomancer gloves temporal",
                   "#x130[>#x215temporal gloves#x130<]#n", WEAR_TAKE | WEAR_HANDS, 30, 30, -35),
        armor_piece("chronomancer bracers clockwork", "chronomancer bracers clockwork",
                   "#x130[>#x215clockwork bracers#x130<]#n", WEAR_TAKE | WEAR_ARMS, 30, 30, -35),
        armor_piece("chronomancer cloak moments", "chronomancer cloak moments",
                   "#x130[>#x215a cloak of moments#x130<]#n", WEAR_TAKE | WEAR_ABOUT, 30, 30, -35),
        armor_piece("chronomancer sash temporal", "chronomancer sash temporal",
                   "#x130[>#x215a temporal sash#x130<]#n", WEAR_TAKE | WEAR_WAIST, 30, 30, -35),
        armor_piece("chronomancer sundial wrist", "chronomancer sundial wrist",
                   "#x130[>#x215a wrist sundial#x130<]#n", WEAR_TAKE | WEAR_WRIST, 30, 30, -35),
        armor_piece("chronomancer mask ages", "chronomancer mask ages",
                   "#x130[>#x215a mask of ages#x130<]#n", WEAR_TAKE | WEAR_FACE, 30, 30, -35),
    ],
    "mastery": mastery_piece("chronomancer core temporal shard", "chronomancer core temporal shard",
                             "#x130[>#x215a pulsing temporal core shard#x130<]#n", 50, 50),
}

# =========================================================================
# PARADOX (Upgrade Class) - Vnums 33560-33572, mastery 33575
# Stats: Weapon +45/+45, Armor +40/+40/-45 AC, Mastery +75/+75 (upgrade class = Wyrm tier)
# Colors: accent #x160 (deep crimson), primary #x210 (warm rose)
PARADOX_EQUIPMENT = {
    "class_name": "Paradox",
    "vnum_start": 33560,
    "mastery_vnum": 33575,
    "pieces": [
        weapon_piece("paradox staff infinity", "paradox staff infinity",
                    "#x160>(#x210an infinity staff#x160)<#n", 45, 45),
        armor_piece("paradox ring mobius", "paradox ring mobius",
                   "#x160>(#x210a mobius ring#x160)<#n", WEAR_TAKE | WEAR_FINGER, 40, 40, -45),
        armor_piece("paradox amulet hourglass", "paradox amulet hourglass",
                   "#x160>(#x210an hourglass pendant#x160)<#n", WEAR_TAKE | WEAR_NECK, 40, 40, -45),
        armor_piece("paradox robes timeweave", "paradox robes timeweave",
                   "#x160>(#x210timeweave robes#x160)<#n", WEAR_TAKE | WEAR_BODY, 40, 40, -45),
        armor_piece("paradox crown moments", "paradox crown moments",
                   "#x160>(#x210a crown of moments#x160)<#n", WEAR_TAKE | WEAR_HEAD, 40, 40, -45),
        armor_piece("paradox leggings paradox", "paradox leggings paradox",
                   "#x160>(#x210paradox leggings#x160)<#n", WEAR_TAKE | WEAR_LEGS, 40, 40, -45),
        armor_piece("paradox boots ages", "paradox boots ages",
                   "#x160>(#x210boots of ages#x160)<#n", WEAR_TAKE | WEAR_FEET, 40, 40, -45),
        armor_piece("paradox gauntlets temporal", "paradox gauntlets temporal",
                   "#x160>(#x210temporal gauntlets#x160)<#n", WEAR_TAKE | WEAR_HANDS, 40, 40, -45),
        armor_piece("paradox bracers chrono", "paradox bracers chrono",
                   "#x160>(#x210chrono-bracers#x160)<#n", WEAR_TAKE | WEAR_ARMS, 40, 40, -45),
        armor_piece("paradox cloak eternities", "paradox cloak eternities",
                   "#x160>(#x210a cloak of eternities#x160)<#n", WEAR_TAKE | WEAR_ABOUT, 40, 40, -45),
        armor_piece("paradox belt loop", "paradox belt loop",
                   "#x160>(#x210a loop belt#x160)<#n", WEAR_TAKE | WEAR_WAIST, 40, 40, -45),
        armor_piece("paradox bracer timeline", "paradox bracer timeline",
                   "#x160>(#x210a timeline bracer#x160)<#n", WEAR_TAKE | WEAR_WRIST, 40, 40, -45),
        armor_piece("paradox mask void", "paradox mask void",
                   "#x160>(#x210a mask of the void#x160)<#n", WEAR_TAKE | WEAR_FACE, 40, 40, -45),
    ],
    "mastery": mastery_piece("paradox core temporal shard", "paradox core temporal shard",
                             "#x160>(#x210a pulsing paradox core shard#x160)<#n", 75, 75),
}

EQUIPMENT_TO_ADD = [
    DRAGONKIN_EQUIPMENT,
    WYRM_EQUIPMENT,
    ARTIFICER_EQUIPMENT,
    MECHANIST_EQUIPMENT,
    CULTIST_EQUIPMENT,
    VOIDBORN_EQUIPMENT,
    CHRONOMANCER_EQUIPMENT,
    PARADOX_EQUIPMENT,
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
