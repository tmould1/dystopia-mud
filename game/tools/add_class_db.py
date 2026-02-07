#!/usr/bin/env python3
"""
add_class_db.py - Add new class entries to class.db

Usage:
    python add_class_db.py                    # Run with default class data defined below
    python add_class_db.py --dry-run          # Show what would be inserted without committing
    python add_class_db.py --delete CLASS_ID  # Delete a class and all related entries

This script inserts class configuration data into gamedata/db/game/class.db.
Modify the CLASS_DATA section at the bottom to define your class entries.

Database Tables:
    class_registry     - Core class metadata (names, keywords, upgrade paths)
    class_brackets     - WHO list display brackets/colors
    class_generations  - Generation titles (up to 14 per class)
    class_auras        - Room presence text
    class_starting     - Starting values (beast, level, disciplines)
    class_score_stats  - Custom score display stats
    class_armor_config - Armor creation command config
    class_armor_pieces - Individual armor piece vnums
"""

import sqlite3
import argparse
import os
import sys
from pathlib import Path


def find_class_db():
    """Find the class.db file relative to script location."""
    script_dir = Path(__file__).parent
    # Try relative paths from game/tools
    candidates = [
        script_dir.parent.parent / "gamedata" / "db" / "game" / "class.db",
        script_dir / ".." / ".." / "gamedata" / "db" / "game" / "class.db",
        Path("gamedata/db/game/class.db"),
    ]
    for path in candidates:
        if path.exists():
            return str(path.resolve())
    raise FileNotFoundError("Could not find class.db")


def insert_class_data(conn, class_data, dry_run=False):
    """Insert all class data for a single class."""
    cursor = conn.cursor()
    class_id = class_data["class_id"]
    class_name = class_data["class_name"]

    print(f"\n{'[DRY RUN] ' if dry_run else ''}Adding class: {class_name} (ID: {class_id})")

    # 1. class_registry
    if "registry" in class_data:
        reg = class_data["registry"]
        sql = """INSERT OR REPLACE INTO class_registry
                 (class_id, class_name, keyword, keyword_alt, mudstat_label,
                  selfclass_message, display_order, upgrade_class, requirements)
                 VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"""
        params = (
            class_id,
            class_name,
            reg.get("keyword", class_name.lower()),
            reg.get("keyword_alt"),
            reg.get("mudstat_label", f"{class_name}s"),
            reg.get("selfclass_message", f"You are now a {class_name}."),
            reg.get("display_order", 10),
            reg.get("upgrade_class"),
            reg.get("requirements")
        )
        print(f"  - class_registry: {params[2]}")
        if not dry_run:
            cursor.execute(sql, params)

    # 2. class_brackets
    if "brackets" in class_data:
        brk = class_data["brackets"]
        sql = """INSERT OR REPLACE INTO class_brackets
                 (class_id, class_name, open_bracket, close_bracket, accent_color, primary_color)
                 VALUES (?, ?, ?, ?, ?, ?)"""
        params = (
            class_id,
            class_name,
            brk.get("open", "#n[#n"),
            brk.get("close", "#n]#n"),
            brk.get("accent"),
            brk.get("primary")
        )
        print(f"  - class_brackets: {params[2]}...{params[3]}")
        if not dry_run:
            cursor.execute(sql, params)

    # 3. class_generations (list of titles)
    if "generations" in class_data:
        # Delete existing generations for this class first
        if not dry_run:
            cursor.execute("DELETE FROM class_generations WHERE class_id = ?", (class_id,))

        for gen_num, title in enumerate(class_data["generations"]):
            sql = """INSERT INTO class_generations (class_id, generation, title)
                     VALUES (?, ?, ?)"""
            # gen 0 = default/highest, gen 1-13 = specific generations
            params = (class_id, gen_num, title)
            if not dry_run:
                cursor.execute(sql, params)
        print(f"  - class_generations: {len(class_data['generations'])} titles")

    # 4. class_auras
    if "aura" in class_data:
        aura = class_data["aura"]
        sql = """INSERT OR REPLACE INTO class_auras
                 (class_id, aura_text, mxp_tooltip, display_order)
                 VALUES (?, ?, ?, ?)"""
        params = (
            class_id,
            aura.get("text", f"({class_name}) "),
            aura.get("tooltip", class_name),
            aura.get("display_order", 10)
        )
        print(f"  - class_auras: {params[1][:30]}...")
        if not dry_run:
            cursor.execute(sql, params)

    # 5. class_starting
    if "starting" in class_data:
        start = class_data["starting"]
        sql = """INSERT OR REPLACE INTO class_starting
                 (class_id, starting_beast, starting_level, has_disciplines)
                 VALUES (?, ?, ?, ?)"""
        params = (
            class_id,
            start.get("beast", 15),
            start.get("level", 1),
            1 if start.get("has_disciplines", False) else 0
        )
        print(f"  - class_starting: beast={params[1]}, level={params[2]}")
        if not dry_run:
            cursor.execute(sql, params)

    # 6. class_score_stats (list of stats to display)
    if "score_stats" in class_data:
        # Delete existing score stats for this class first
        if not dry_run:
            cursor.execute("DELETE FROM class_score_stats WHERE class_id = ?", (class_id,))

        for order, stat in enumerate(class_data["score_stats"]):
            sql = """INSERT INTO class_score_stats
                     (class_id, stat_source, stat_source_max, stat_label, format_string, display_order)
                     VALUES (?, ?, ?, ?, ?, ?)"""
            params = (
                class_id,
                stat["source"],
                stat.get("source_max", 0),
                stat["label"],
                stat.get("format", "#R[#n%s: #C%d#R]\\n\\r"),
                stat.get("order", order)
            )
            if not dry_run:
                cursor.execute(sql, params)
        print(f"  - class_score_stats: {len(class_data['score_stats'])} stats")

    # 7. class_armor_config
    if "armor_config" in class_data:
        armor = class_data["armor_config"]
        sql = """INSERT OR REPLACE INTO class_armor_config
                 (class_id, acfg_cost_key, usage_message, act_to_char, act_to_room, mastery_vnum)
                 VALUES (?, ?, ?, ?, ?, ?)"""
        params = (
            class_id,
            armor["acfg_cost_key"],
            armor["usage_message"],
            armor.get("act_to_char", "$p appears in your hands."),
            armor.get("act_to_room", "$p appears in $n's hands."),
            armor.get("mastery_vnum", 0)
        )
        print(f"  - class_armor_config: {params[1]}")
        if not dry_run:
            cursor.execute(sql, params)

    # 8. class_armor_pieces (list of armor pieces)
    if "armor_pieces" in class_data:
        # Delete existing armor pieces for this class first
        if not dry_run:
            cursor.execute("DELETE FROM class_armor_pieces WHERE class_id = ?", (class_id,))

        for piece in class_data["armor_pieces"]:
            sql = """INSERT INTO class_armor_pieces (class_id, keyword, vnum)
                     VALUES (?, ?, ?)"""
            params = (class_id, piece["keyword"], piece["vnum"])
            if not dry_run:
                cursor.execute(sql, params)
        print(f"  - class_armor_pieces: {len(class_data['armor_pieces'])} pieces")

    if not dry_run:
        conn.commit()
    print(f"{'[DRY RUN] ' if dry_run else ''}Done: {class_name}")


def delete_class(conn, class_id, dry_run=False):
    """Delete a class and all related entries."""
    cursor = conn.cursor()

    # Check if class exists
    cursor.execute("SELECT class_name FROM class_registry WHERE class_id = ?", (class_id,))
    row = cursor.fetchone()
    if not row:
        print(f"Class ID {class_id} not found in database.")
        return

    class_name = row[0]
    print(f"\n{'[DRY RUN] ' if dry_run else ''}Deleting class: {class_name} (ID: {class_id})")

    tables = [
        "class_armor_pieces",
        "class_armor_config",
        "class_score_stats",
        "class_starting",
        "class_auras",
        "class_generations",
        "class_brackets",
        "class_registry"
    ]

    for table in tables:
        if not dry_run:
            cursor.execute(f"DELETE FROM {table} WHERE class_id = ?", (class_id,))
        print(f"  - Deleted from {table}")

    if not dry_run:
        conn.commit()
    print(f"{'[DRY RUN] ' if dry_run else ''}Done: {class_name} deleted")


# ============================================================================
# STAT_SOURCE enum values (must match db_class.h)
# ============================================================================
STAT_NONE = 0
STAT_BEAST = 1
STAT_RAGE = 2
STAT_CHI_CURRENT = 3
STAT_CHI_MAXIMUM = 4
STAT_GNOSIS_CURRENT = 5
STAT_GNOSIS_MAXIMUM = 6
STAT_MONKBLOCK = 7
STAT_SILTOL = 8
STAT_SOULS = 9
STAT_DEMON_POWER = 10
STAT_DEMON_TOTAL = 11
STAT_DROID_POWER = 12
STAT_DROW_POWER = 13
STAT_DROW_MAGIC = 14
STAT_TPOINTS = 15
STAT_ANGEL_JUSTICE = 16
STAT_ANGEL_LOVE = 17
STAT_ANGEL_HARMONY = 18
STAT_ANGEL_PEACE = 19
STAT_SHAPE_COUNTER = 20
STAT_PHASE_COUNTER = 21
STAT_HARA_KIRI = 22
STAT_DRAGON_ATTUNEMENT = 23
STAT_DRAGON_ESSENCE_PEAK = 24


# ============================================================================
# CLASS DATA - Modify this section to add your classes
# ============================================================================

# Class IDs (must match class.h defines)
CLASS_DRAGONKIN = 262144
CLASS_WYRM = 524288

CLASSES_TO_ADD = [
    # -------------------------------------------------------------------------
    # DRAGONKIN (Base Class)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_DRAGONKIN,
        "class_name": "Dragonkin",

        "registry": {
            "keyword": "dragonkin",
            "keyword_alt": None,
            "mudstat_label": "Dragonkin",
            "selfclass_message": "Draconic blood awakens within you. Embrace the #x220power of dragons#n.",
            "display_order": 7,
            "upgrade_class": None,  # Base class
            "requirements": None
        },

        "brackets": {
            "open": "#x202<*#n",
            "close": "#n*#x202>#n",
            "accent": "#x202",
            "primary": "#x220"
        },

        # Generation 0 = default (lowest), then 1 = highest gen, counting up
        "generations": [
            "#x220Hatchling#n",       # 0 - default
            "#x220Dragon Primarch#n", # 1 - highest
            "#x220Dragon Emperor#n",  # 2
            "#x220Dragon Sovereign#n",# 3
            "#x220Dragon Lord#n",     # 4
            "#x220Dragon Ascendant#n",# 5
            "#x220Ancient Dragon#n",  # 6
            "#x220Elder Dragon#n",    # 7
            "#x220Dragon#n",          # 8
            "#x220Young Dragon#n",    # 9
            "#x220Drake#n",           # 10
            "#x220Fledgling#n",       # 11
            "#x220Whelp#n",           # 12
            "#x220Hatchling#n",       # 13 - lowest specific gen
        ],

        "aura": {
            "text": "#x202(#x220Dragonkin#x202)#n ",
            "tooltip": "Dragonkin",
            "display_order": 20
        },

        "starting": {
            "beast": 15,
            "level": 1,
            "has_disciplines": False
        },

        "score_stats": [
            {"source": STAT_RAGE, "label": "Draconic Essence", "order": 1,
             "format": "#x202[#nEssence: #x220%d#x202]#n\\n\\r"},
            {"source": STAT_DRAGON_ATTUNEMENT, "label": "Attunement", "order": 2,
             "format": "#x202[#nAttunement: #x220%d#x202]#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "dragonkin.dragonarmor.practice_cost",
            "usage_message": "Syntax: dragonarmor <piece>\\n\\rPieces: fang, ring, tooth, scales, helm, leggings, boots, gauntlets, bracers, cloak, belt, bracer, visage",
            "act_to_char": "Dragon scales form into $p in your claws.",
            "act_to_room": "Dragon scales form into $p in $n's claws.",
            "mastery_vnum": 33415
        },

        "armor_pieces": [
            {"keyword": "fang", "vnum": 33400},
            {"keyword": "ring", "vnum": 33401},
            {"keyword": "tooth", "vnum": 33402},
            {"keyword": "scales", "vnum": 33403},
            {"keyword": "helm", "vnum": 33404},
            {"keyword": "leggings", "vnum": 33405},
            {"keyword": "boots", "vnum": 33406},
            {"keyword": "gauntlets", "vnum": 33407},
            {"keyword": "bracers", "vnum": 33408},
            {"keyword": "cloak", "vnum": 33409},
            {"keyword": "belt", "vnum": 33410},
            {"keyword": "bracer", "vnum": 33411},
            {"keyword": "visage", "vnum": 33412},
        ],
    },

    # -------------------------------------------------------------------------
    # WYRM (Upgrade Class from Dragonkin)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_WYRM,
        "class_name": "Wyrm",

        "registry": {
            "keyword": "wyrm",
            "keyword_alt": None,
            "mudstat_label": "Wyrms",
            "selfclass_message": "Your draconic heritage reaches its apex. You are now a #x220Wyrm#n.",
            "display_order": 7,
            "upgrade_class": CLASS_DRAGONKIN,  # Upgrades from Dragonkin
            "requirements": None
        },

        "brackets": {
            "open": "#x202<#x220*#n",
            "close": "#n#x220*#x202>#n",
            "accent": "#x202",
            "primary": "#x220"
        },

        "generations": [
            "#x220Young Wyrm#n",        # 0 - default
            "#x220Primordial Wyrm#n",   # 1 - highest
            "#x220World Wyrm#n",        # 2
            "#x220Cataclysm Wyrm#n",    # 3
            "#x220Apocalypse Wyrm#n",   # 4
            "#x220Doom Wyrm#n",         # 5
            "#x220Ancient Wyrm#n",      # 6
            "#x220Great Wyrm#n",        # 7
            "#x220Elder Wyrm#n",        # 8
            "#x220Wyrm#n",              # 9
            "#x220Young Wyrm#n",        # 10
        ],

        "aura": {
            "text": "#x202(#x220Wyrm#x202)#n ",
            "tooltip": "Wyrm",
            "display_order": 21
        },

        # Wyrm doesn't need starting - inherits from upgrade

        "score_stats": [
            {"source": STAT_RAGE, "label": "Draconic Essence", "order": 1,
             "format": "#x202[#nEssence: #x220%d#x202]#n\\n\\r"},
            {"source": STAT_DRAGON_ATTUNEMENT, "label": "Attunement", "order": 2,
             "format": "#x202[#nAttunement: #x220%d#x202]#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "wyrm.wyrmarmor.practice_cost",
            "usage_message": "Syntax: wyrmarmor <piece>\\n\\rPieces: talon, ring, fang, scales, helm, leggings, boots, gauntlets, bracers, cloak, belt, bracer, visage",
            "act_to_char": "Ancient scales form into $p in your talons.",
            "act_to_room": "Ancient scales form into $p in $n's talons.",
            "mastery_vnum": 33435
        },

        "armor_pieces": [
            {"keyword": "talon", "vnum": 33420},
            {"keyword": "ring", "vnum": 33421},
            {"keyword": "fang", "vnum": 33422},
            {"keyword": "scales", "vnum": 33423},
            {"keyword": "helm", "vnum": 33424},
            {"keyword": "leggings", "vnum": 33425},
            {"keyword": "boots", "vnum": 33426},
            {"keyword": "gauntlets", "vnum": 33427},
            {"keyword": "bracers", "vnum": 33428},
            {"keyword": "cloak", "vnum": 33429},
            {"keyword": "belt", "vnum": 33430},
            {"keyword": "bracer", "vnum": 33431},
            {"keyword": "visage", "vnum": 33432},
        ],
    },
]


# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description="Add class entries to class.db")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be done without making changes")
    parser.add_argument("--delete", type=int, metavar="CLASS_ID",
                        help="Delete a class by ID")
    parser.add_argument("--db", type=str, help="Path to class.db (auto-detected if not specified)")
    args = parser.parse_args()

    try:
        db_path = args.db or find_class_db()
    except FileNotFoundError as e:
        print(f"Error: {e}")
        sys.exit(1)

    print(f"Using database: {db_path}")
    conn = sqlite3.connect(db_path)

    try:
        if args.delete:
            delete_class(conn, args.delete, dry_run=args.dry_run)
        else:
            for class_data in CLASSES_TO_ADD:
                insert_class_data(conn, class_data, dry_run=args.dry_run)
    finally:
        conn.close()

    print("\nComplete!")


if __name__ == "__main__":
    main()
