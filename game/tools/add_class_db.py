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
                 (class_id, open_bracket, close_bracket, accent_color, primary_color)
                 VALUES (?, ?, ?, ?, ?)"""
        params = (
            class_id,
            brk.get("open", "#n[#n"),
            brk.get("close", "#n]#n"),
            brk.get("accent"),
            brk.get("primary")
        )
        print(f"  - class_brackets: {params[1]}...{params[2]}")
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

    # 4. class_auras (mxp_tooltip derived from class_registry.class_name via JOIN in C code,
    #    but we still write class_name to satisfy NOT NULL constraint)
    if "aura" in class_data:
        aura = class_data["aura"]
        sql = """INSERT OR REPLACE INTO class_auras
                 (class_id, aura_text, mxp_tooltip, display_order)
                 VALUES (?, ?, ?, ?)"""
        params = (
            class_id,
            aura.get("text", f"({class_name}) "),
            class_name,  # Not used by C code (uses JOIN), but satisfies NOT NULL constraint
            aura.get("display_order", 10)
        )
        print(f"  - class_auras: {params[1][:30]}...")
        if not dry_run:
            cursor.execute(sql, params)

    # 5. class_starting
    if "starting" in class_data:
        start = class_data["starting"]
        # Ensure starting_rage column exists (migration for older databases)
        if not dry_run:
            try:
                cursor.execute("ALTER TABLE class_starting ADD COLUMN starting_rage INTEGER DEFAULT 0")
            except sqlite3.OperationalError:
                pass  # Column already exists
        sql = """INSERT OR REPLACE INTO class_starting
                 (class_id, starting_beast, starting_level, has_disciplines, starting_rage)
                 VALUES (?, ?, ?, ?, ?)"""
        params = (
            class_id,
            start.get("beast", 15),
            start.get("level", 1),
            1 if start.get("has_disciplines", False) else 0,
            start.get("rage", 0)
        )
        print(f"  - class_starting: beast={params[1]}, level={params[2]}, rage={params[4]}")
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

        # Auto-generate usage_message from armor_pieces if not explicitly set
        if "usage_message" not in armor and "armor_pieces" in class_data:
            pieces = class_data["armor_pieces"]
            keywords = ", ".join(p["keyword"] for p in pieces)
            command = armor.get("command_name", f"{class_name.lower()}armor")
            armor["usage_message"] = f"Syntax: {command} <piece>\\n\\rPieces: {keywords}"

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
CLASS_ARTIFICER = 1048576
CLASS_MECHANIST = 2097152
CLASS_CULTIST = 4194304
CLASS_VOIDBORN = 8388608
CLASS_CHRONOMANCER = 16777216
CLASS_PARADOX = 33554432
CLASS_SHAMAN = 67108864
CLASS_SPIRITLORD = 134217728

CLASSES_TO_ADD = [
    # -------------------------------------------------------------------------
    # ARTIFICER (Base Class)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_ARTIFICER,
        "class_name": "Artificer",

        "registry": {
            "keyword": "artificer",
            "keyword_alt": None,
            "mudstat_label": "Artificers",
            "selfclass_message": "Technology bends to your will. You are now an #x117Artificer#n.",
            "display_order": 8,
            "upgrade_class": None,  # Base class
            "requirements": None
        },

        "brackets": {
            "open": "#x037[=#n",
            "close": "#x037=]#n",
            "accent": "#x037",
            "primary": "#x117"
        },

        # Generation 0 = default (lowest), then 1 = highest gen, counting up
        # NOTE: Generation titles should NOT include color codes - primary_color
        # from brackets is applied automatically when displaying in WHO/selfclass.
        "generations": [
            "Mechanic",          # 0 - default
            "Master Engineer",   # 1 - highest
            "Inventor",          # 2
            "Artificer",         # 3
            "Tinkerer",          # 4
            "Apprentice",        # 5
        ],

        "aura": {
            "text": "#x037(#x117Artificer#x037)#n ",
            "display_order": 22
        },

        "starting": {
            "beast": 15,
            "level": 1,
            "has_disciplines": False
        },

        "score_stats": [
            {"source": STAT_RAGE, "label": "Power Cells", "order": 1,
             "format": "#x037[#n%s: #x117%d#x037]#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "artificer.artificerarmor.practice_cost",
            "command_name": "artificerarmor",
            "act_to_char": "Nano-assemblers construct $p in your hands.",
            "act_to_room": "Nano-assemblers construct $p in $n's hands.",
            "mastery_vnum": 33455
        },

        "armor_pieces": [
            {"keyword": "wrench",   "vnum": 33440},
            {"keyword": "ring",     "vnum": 33441},
            {"keyword": "collar",   "vnum": 33442},
            {"keyword": "vest",     "vnum": 33443},
            {"keyword": "goggles",  "vnum": 33444},
            {"keyword": "pants",    "vnum": 33445},
            {"keyword": "boots",    "vnum": 33446},
            {"keyword": "gloves",   "vnum": 33447},
            {"keyword": "bracers",  "vnum": 33448},
            {"keyword": "harness",  "vnum": 33449},
            {"keyword": "belt",     "vnum": 33450},
            {"keyword": "wrist",    "vnum": 33451},
            {"keyword": "mask",     "vnum": 33452},
        ],
    },

    # -------------------------------------------------------------------------
    # MECHANIST (Upgrade Class from Artificer)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_MECHANIST,
        "class_name": "Mechanist",

        "registry": {
            "keyword": "mechanist",
            "keyword_alt": None,
            "mudstat_label": "Mechanists",
            "selfclass_message": "Machine and flesh become one. You are now a #x177Mechanist#n.",
            "display_order": 8,
            "upgrade_class": CLASS_ARTIFICER,  # Upgrades from Artificer
            "requirements": None
        },

        "brackets": {
            "open": "#x127>/#n",
            "close": "#x127\\<#n",
            "accent": "#x127",
            "primary": "#x177"
        },

        # NOTE: Generation titles should NOT include color codes - primary_color
        # from brackets is applied automatically when displaying in WHO/selfclass.
        "generations": [
            "Cyborg",          # 0 - default
            "War Machine",     # 1 - highest
            "Cyborg Lord",     # 2
            "Mechanist",       # 3
            "Augmented",       # 4
        ],

        "aura": {
            "text": "#x127(#x177Mechanist#x127)#n ",
            "display_order": 23
        },

        # Mechanist doesn't need starting - inherits from upgrade

        "score_stats": [
            {"source": STAT_RAGE, "label": "Power Cells", "order": 1,
             "format": "#x127[#n%s: #x177%d#x127]#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "mechanist.mechanistarmor.practice_cost",
            "command_name": "mechanistarmor",
            "act_to_char": "Cybernetic fabricators assemble $p before you.",
            "act_to_room": "Cybernetic fabricators assemble $p before $n.",
            "mastery_vnum": 33475
        },

        "armor_pieces": [
            {"keyword": "cutter",    "vnum": 33460},
            {"keyword": "ring",      "vnum": 33461},
            {"keyword": "collar",    "vnum": 33462},
            {"keyword": "chassis",   "vnum": 33463},
            {"keyword": "helm",      "vnum": 33464},
            {"keyword": "leggings",  "vnum": 33465},
            {"keyword": "boots",     "vnum": 33466},
            {"keyword": "gauntlets", "vnum": 33467},
            {"keyword": "mounts",    "vnum": 33468},
            {"keyword": "harness",   "vnum": 33469},
            {"keyword": "belt",      "vnum": 33470},
            {"keyword": "holo",      "vnum": 33471},
            {"keyword": "visor",     "vnum": 33472},
        ],
    },

    # -------------------------------------------------------------------------
    # CULTIST (Base Class)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_CULTIST,
        "class_name": "Cultist",

        "registry": {
            "keyword": "cultist",
            "keyword_alt": None,
            "mudstat_label": "Cultists",
            "selfclass_message": "The void whispers your name. You are now a #x120Cultist#n.",
            "display_order": 9,
            "upgrade_class": None,  # Base class
            "requirements": None
        },

        "brackets": {
            "open": "#x064{~#n",
            "close": "#x064~}#n",
            "accent": "#x064",
            "primary": "#x120"
        },

        # Generation 0 = default (lowest), then 1 = highest gen, counting up
        "generations": [
            "Acolyte",         # 0 - default
            "High Priest",     # 1 - highest
            "Void Touched",    # 2
            "Cultist",         # 3
            "Initiate",        # 4
            "Seeker",          # 5
        ],

        "aura": {
            "text": "#x120(#nCultist#x120)#n ",
            "display_order": 24
        },

        "starting": {
            "beast": 15,
            "level": 1,
            "has_disciplines": False
        },

        "score_stats": [
            {"source": STAT_RAGE, "label": "Corruption", "order": 1,
             "format": "#x064{#n%s: #x120%d#x064}#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "cultist.cultistarmor.practice_cost",
            "command_name": "cultistarmor",
            "act_to_char": "Void energy coalesces into $p in your hands.",
            "act_to_room": "Void energy coalesces into $p in $n's hands.",
            "mastery_vnum": 33515
        },

        "armor_pieces": [
            {"keyword": "staff",     "vnum": 33500},
            {"keyword": "ring",      "vnum": 33501},
            {"keyword": "amulet",    "vnum": 33502},
            {"keyword": "robes",     "vnum": 33503},
            {"keyword": "hood",      "vnum": 33504},
            {"keyword": "leggings",  "vnum": 33505},
            {"keyword": "boots",     "vnum": 33506},
            {"keyword": "gloves",    "vnum": 33507},
            {"keyword": "bracers",   "vnum": 33508},
            {"keyword": "shroud",    "vnum": 33509},
            {"keyword": "sash",      "vnum": 33510},
            {"keyword": "bangle",    "vnum": 33511},
            {"keyword": "mask",      "vnum": 33512},
        ],
    },

    # -------------------------------------------------------------------------
    # VOIDBORN (Upgrade Class from Cultist)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_VOIDBORN,
        "class_name": "Voidborn",

        "registry": {
            "keyword": "voidborn",
            "keyword_alt": None,
            "mudstat_label": "Voidborn",
            "selfclass_message": "Reality unravels around you. You are now #x097Voidborn#n.",
            "display_order": 9,
            "upgrade_class": CLASS_CULTIST,  # Upgrades from Cultist
            "requirements": None
        },

        "brackets": {
            "open": "#x055*(#n",
            "close": "#x055)*#n",
            "accent": "#x055",
            "primary": "#x097"
        },

        "generations": [
            "Voidborn",        # 0 - default
            "Void Incarnate",  # 1 - highest
            "Aberration",      # 2
            "Voidspawn",       # 3
            "Void Touched",    # 4
        ],

        "aura": {
            "text": "#x097(#nVoidborn#x097)#n ",
            "display_order": 25
        },

        # Voidborn doesn't need starting - inherits from upgrade

        "score_stats": [
            {"source": STAT_RAGE, "label": "Corruption", "order": 1,
             "format": "#x055{#n%s: #x097%d#x055}#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "voidborn.voidbornarmor.practice_cost",
            "command_name": "voidbornarmor",
            "act_to_char": "Reality tears open and $p materializes in your grasp.",
            "act_to_room": "Reality tears open and $p materializes in $n's grasp.",
            "mastery_vnum": 33535
        },

        "armor_pieces": [
            {"keyword": "scepter",    "vnum": 33520},
            {"keyword": "ring",       "vnum": 33521},
            {"keyword": "collar",     "vnum": 33522},
            {"keyword": "robes",      "vnum": 33523},
            {"keyword": "crown",      "vnum": 33524},
            {"keyword": "leggings",   "vnum": 33525},
            {"keyword": "boots",      "vnum": 33526},
            {"keyword": "gauntlets",  "vnum": 33527},
            {"keyword": "bracers",    "vnum": 33528},
            {"keyword": "shroud",     "vnum": 33529},
            {"keyword": "sash",       "vnum": 33530},
            {"keyword": "bangle",     "vnum": 33531},
            {"keyword": "mask",       "vnum": 33532},
        ],
    },

    # -------------------------------------------------------------------------
    # CHRONOMANCER (Base Class)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_CHRONOMANCER,
        "class_name": "Chronomancer",

        "registry": {
            "keyword": "chronomancer",
            "keyword_alt": "chrono",
            "mudstat_label": "Chronomancers",
            "selfclass_message": "Time bends to your will. You are now a #x215Chronomancer#n.",
            "display_order": 10,
            "upgrade_class": None,  # Base class
            "requirements": None
        },

        "brackets": {
            "open": "#x130[>#n",
            "close": "#x130<]#n",
            "accent": "#x130",
            "primary": "#x215"
        },

        # Generation 0 = default (lowest), then 1 = highest gen, counting up
        "generations": [
            "Novice",            # 0 - default
            "Time Lord",         # 1 - highest
            "Temporal Master",   # 2
            "Chronomancer",      # 3
            "Time Weaver",       # 4
            "Initiate",          # 5
        ],

        "aura": {
            "text": "#x215(#nChronomancer#x215)#n ",
            "display_order": 26
        },

        "starting": {
            "beast": 15,
            "level": 1,
            "has_disciplines": False,
            "rage": 50  # Flux starts at 50 (center)

        },

        "score_stats": [
            {"source": STAT_RAGE, "label": "Flux", "order": 1,
             "format": "#x130[#n%s: #x215%d#x130]#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "chrono.chronoarmor.practice_cost",
            "command_name": "chronoarmor",
            "act_to_char": "Temporal energy coalesces into $p in your hands.",
            "act_to_room": "Temporal energy coalesces into $p in $n's hands.",
            "mastery_vnum": 33555
        },

        "armor_pieces": [
            {"keyword": "staff",    "vnum": 33540},
            {"keyword": "ring",     "vnum": 33541},
            {"keyword": "amulet",   "vnum": 33542},
            {"keyword": "robes",    "vnum": 33543},
            {"keyword": "circlet",  "vnum": 33544},
            {"keyword": "pants",    "vnum": 33545},
            {"keyword": "boots",    "vnum": 33546},
            {"keyword": "gloves",   "vnum": 33547},
            {"keyword": "bracers",  "vnum": 33548},
            {"keyword": "cloak",    "vnum": 33549},
            {"keyword": "sash",     "vnum": 33550},
            {"keyword": "sundial",  "vnum": 33551},
            {"keyword": "mask",     "vnum": 33552},
        ],
    },

    # -------------------------------------------------------------------------
    # PARADOX (Upgrade Class - upgrades from Chronomancer)
    # -------------------------------------------------------------------------
    {
        "class_id": 33554432,  # CLASS_PARADOX
        "class_name": "Paradox",

        "registry": {
            "keyword": "paradox",
            "keyword_alt": "para",
            "mudstat_label": "Paradoxes",
            "selfclass_message": "#x160>(#x210You shatter the timeline, becoming a Paradox!#x160)<#n",
            "display_order": 11,
            "upgrade_class": CLASS_CHRONOMANCER,
            "requirements": "Chronomancer upgrade"
        },

        "brackets": {
            "open":    "#x160>(#n",
            "close":   "#x160)<#n",
            "accent":  "#x160",
            "primary": "#x210"
        },

        "generations": [
            "Glitch",            # 0 - default
            "Temporal God",      # 1 - highest
            "Timeline Breaker",  # 2
            "Paradox",           # 3
            "Anomaly",           # 4
            "Distortion",        # 5
        ],

        "aura": {
            "text": "#x210(#nParadox#x210)#n ",
            "display_order": 27
        },

        "starting": {
            "beast": 15,
            "level": 1,
            "has_disciplines": False,
            "rage": 75  # Flux starts at 75 (Paradox center)
        },

        "score_stats": [
            {"source": STAT_RAGE, "label": "Flux", "order": 1,
             "format": "#x160[#n%s: #x210%d#x160]#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "para.paradoxarmor.practice_cost",
            "command_name": "paradoxarmor",
            "act_to_char": "Timeline fragments coalesce into $p in your hands.",
            "act_to_room": "Timeline fragments coalesce into $p in $n's hands.",
            "mastery_vnum": 33575
        },

        "armor_pieces": [
            {"keyword": "staff",    "vnum": 33560},
            {"keyword": "ring",     "vnum": 33561},
            {"keyword": "amulet",   "vnum": 33562},
            {"keyword": "robe",     "vnum": 33563},
            {"keyword": "crown",    "vnum": 33564},
            {"keyword": "pants",    "vnum": 33565},
            {"keyword": "boots",    "vnum": 33566},
            {"keyword": "gloves",   "vnum": 33567},
            {"keyword": "bracers",  "vnum": 33568},
            {"keyword": "cloak",    "vnum": 33569},
            {"keyword": "belt",     "vnum": 33570},
            {"keyword": "bracer",   "vnum": 33571},
            {"keyword": "mask",     "vnum": 33572},
        ],
    },

    # -------------------------------------------------------------------------
    # SHAMAN (Base Class)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_SHAMAN,
        "class_name": "Shaman",

        "registry": {
            "keyword": "shaman",
            "keyword_alt": None,
            "mudstat_label": "Shamans",
            "selfclass_message": "The spirit world opens its veil. You are now a #x116Shaman#n.",
            "display_order": 11,
            "upgrade_class": None,
            "requirements": None
        },

        "brackets": {
            "open": "#x026~(#n",
            "close": "#x026)~#n",
            "accent": "#x026",
            "primary": "#x116"
        },

        "generations": [
            "Initiate",          # 0 - default
            "Spirit Walker",     # 1 - highest
            "Elder Shaman",      # 2
            "Shaman",            # 3
            "Seer",              # 4
            "Apprentice",        # 5
        ],

        "aura": {
            "text": "#x116(#nShaman#x116)#n ",
            "display_order": 28
        },

        "starting": {
            "beast": 15,
            "level": 1,
            "has_disciplines": False,
            "rage": 50  # Tether starts at 50 (center)
        },

        "score_stats": [
            {"source": STAT_RAGE, "label": "Spirit Tether", "order": 1,
             "format": "#x026~(#n%s: #x116%d#x026)~#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "ability.shaman.shamanarmor.practice_cost",
            "command_name": "shamanarmor",
            "act_to_char": "Spirits weave $p into existence in your hands.",
            "act_to_room": "Spirits weave $p into existence in $n's hands.",
            "mastery_vnum": 33595
        },

        "armor_pieces": [
            {"keyword": "staff",      "vnum": 33580},
            {"keyword": "ring",       "vnum": 33581},
            {"keyword": "talisman",   "vnum": 33582},
            {"keyword": "vestments",  "vnum": 33583},
            {"keyword": "headdress",  "vnum": 33584},
            {"keyword": "leggings",   "vnum": 33585},
            {"keyword": "moccasins",  "vnum": 33586},
            {"keyword": "gloves",     "vnum": 33587},
            {"keyword": "bracers",    "vnum": 33588},
            {"keyword": "cloak",      "vnum": 33589},
            {"keyword": "sash",       "vnum": 33590},
            {"keyword": "bangle",     "vnum": 33591},
            {"keyword": "mask",       "vnum": 33592},
        ],
    },

    # -------------------------------------------------------------------------
    # SPIRIT LORD (Upgrade Class from Shaman)
    # -------------------------------------------------------------------------
    {
        "class_id": CLASS_SPIRITLORD,
        "class_name": "Spirit Lord",

        "registry": {
            "keyword": "spiritlord",
            "keyword_alt": "spirit",
            "mudstat_label": "Spirit Lords",
            "selfclass_message": "#x135+(#WYou transcend the veil, becoming a Spirit Lord!#x135)+#n",
            "display_order": 11,
            "upgrade_class": CLASS_SHAMAN,
            "requirements": "Shaman upgrade"
        },

        "brackets": {
            "open": "#x135+(#n",
            "close": "#x135)+#n",
            "accent": "#x135",
            "primary": "#W"
        },

        "generations": [
            "Spirit Touched",    # 0 - default
            "Spirit God",        # 1 - highest
            "Ascended One",      # 2
            "Spirit Lord",       # 3
            "Soul Binder",       # 4
            "Medium",            # 5
        ],

        "aura": {
            "text": "#W(#nSpirit Lord#W)#n ",
            "display_order": 29
        },

        "starting": {
            "beast": 15,
            "level": 1,
            "has_disciplines": False,
            "rage": 75  # Tether starts at 75 (Spirit Lord center)
        },

        "score_stats": [
            {"source": STAT_RAGE, "label": "Spirit Tether", "order": 1,
             "format": "#x135+(#n%s: #W%d#x135)+#n\\n\\r"},
        ],

        "armor_config": {
            "acfg_cost_key": "ability.sl.lordarmor.practice_cost",
            "command_name": "lordarmor",
            "act_to_char": "Ancestral spirits forge $p from pure ether.",
            "act_to_room": "Ancestral spirits forge $p from pure ether before $n.",
            "mastery_vnum": 33615
        },

        "armor_pieces": [
            {"keyword": "scepter",    "vnum": 33600},
            {"keyword": "ring",       "vnum": 33601},
            {"keyword": "pendant",    "vnum": 33602},
            {"keyword": "robes",      "vnum": 33603},
            {"keyword": "crown",      "vnum": 33604},
            {"keyword": "leggings",   "vnum": 33605},
            {"keyword": "boots",      "vnum": 33606},
            {"keyword": "gauntlets",  "vnum": 33607},
            {"keyword": "bracers",    "vnum": 33608},
            {"keyword": "mantle",     "vnum": 33609},
            {"keyword": "belt",       "vnum": 33610},
            {"keyword": "bangle",     "vnum": 33611},
            {"keyword": "mask",       "vnum": 33612},
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
