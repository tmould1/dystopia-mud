#!/usr/bin/env python3
"""
Fix validation warnings by adding missing database entries.

Adds:
- Shapeshifter aura (class_id 512)
- Demon armor_config and armor_pieces (class_id 1)
- Drow armor_config and armor_pieces (class_id 32)
- Score stats for Dirgesinger, Siren, Psion, Mindflayer
"""

import sqlite3
from pathlib import Path


def get_db_path() -> Path:
    """Get path to game.db"""
    return Path(__file__).parent.parent.parent / 'gamedata' / 'db' / 'game' / 'game.db'


def add_shapeshifter_aura(conn: sqlite3.Connection) -> None:
    """Add missing Shapeshifter aura."""
    cursor = conn.cursor()

    # Check if already exists
    cursor.execute("SELECT 1 FROM class_auras WHERE class_id = 512")
    if cursor.fetchone():
        print("  Shapeshifter aura already exists, skipping")
        return

    # Get next display_order
    cursor.execute("SELECT MAX(display_order) FROM class_auras")
    max_order = cursor.fetchone()[0] or 0

    cursor.execute("""
        INSERT INTO class_auras (class_id, aura_text, mxp_tooltip, display_order)
        VALUES (512, '#P(#0Shapeshifter#P)#n ', 'Shapeshifter', ?)
    """, (max_order + 1,))
    print("  Added Shapeshifter aura")


def add_demon_armor(conn: sqlite3.Connection) -> None:
    """Add Demon armor_config and armor_pieces."""
    cursor = conn.cursor()

    # Check if config already exists
    cursor.execute("SELECT 1 FROM class_armor_config WHERE class_id = 1")
    if cursor.fetchone():
        print("  Demon armor_config already exists, skipping")
        return

    # Add armor_config
    cursor.execute("""
        INSERT INTO class_armor_config (class_id, acfg_cost_key, usage_message, act_to_char, act_to_room)
        VALUES (
            1,
            'demon.demonarmour.primal_cost',
            'Please specify which piece of demon armor you wish to make: Ring Collar Plate Helmet Leggings Boots Gauntlets Sleeves Cape Belt Bracer Visor longsword shortsword.',
            '$p appears in your hands in a blast of flames.',
            '$p appears in $n''s hands in a blast of flames.'
        )
    """)
    print("  Added Demon armor_config")

    # Add armor_pieces
    pieces = [
        ('ring', 33122),
        ('collar', 33123),
        ('bracer', 33124),
        ('plate', 33125),
        ('helmet', 33126),
        ('leggings', 33127),
        ('boots', 33128),
        ('gauntlets', 33129),
        ('sleeves', 33130),
        ('cape', 33131),
        ('belt', 33132),
        ('visor', 33133),
        ('longsword', 33120),
        ('shortsword', 33121),
    ]

    for keyword, vnum in pieces:
        cursor.execute("""
            INSERT INTO class_armor_pieces (class_id, keyword, vnum)
            VALUES (?, ?, ?)
        """, (1, keyword, vnum))

    print(f"  Added {len(pieces)} Demon armor_pieces")


def add_drow_armor(conn: sqlite3.Connection) -> None:
    """Add Drow armor_config and armor_pieces."""
    cursor = conn.cursor()

    # Check if config already exists
    cursor.execute("SELECT 1 FROM class_armor_config WHERE class_id = 32")
    if cursor.fetchone():
        print("  Drow armor_config already exists, skipping")
        return

    # Add armor_config - multi-line usage message
    usage_msg = (
        'Please specify what kind of equipment you want to create.\\n'
        'Whip, Dagger, Ring, Amulet, Armor, Helmet,\\n'
        'Leggings, Boots, Gauntlets, Sleeves,\\n'
        'Belt, Bracer, Mask, Cloak.'
    )

    cursor.execute("""
        INSERT INTO class_armor_config (class_id, acfg_cost_key, usage_message, act_to_char, act_to_room)
        VALUES (
            32,
            'drow.drowcreate.primal_cost',
            ?,
            '$p appears in your hands.',
            '$p appears in $n''s hands.'
        )
    """, (usage_msg,))
    print("  Added Drow armor_config")

    # Add armor_pieces
    pieces = [
        ('dagger', 33060),
        ('whip', 33061),
        ('belt', 33062),
        ('ring', 33063),
        ('amulet', 33064),
        ('helmet', 33065),
        ('leggings', 33066),
        ('boots', 33067),
        ('gauntlets', 33068),
        ('sleeves', 33069),
        ('cloak', 33070),
        ('bracer', 33071),
        ('mask', 33072),
        ('armor', 33073),
    ]

    for keyword, vnum in pieces:
        cursor.execute("""
            INSERT INTO class_armor_pieces (class_id, keyword, vnum)
            VALUES (?, ?, ?)
        """, (32, keyword, vnum))

    print(f"  Added {len(pieces)} Drow armor_pieces")


def add_score_stats(conn: sqlite3.Connection) -> None:
    """Add missing score_stats entries."""
    cursor = conn.cursor()

    # STAT_RAGE = 2 (from stat_source convention)
    STAT_RAGE = 2

    stats_to_add = [
        # (class_id, stat_label, stat_source, display_order)
        (16384, 'Your resonance pulses at', STAT_RAGE, 10),   # Dirgesinger
        (32768, 'Your resonance pulses at', STAT_RAGE, 10),   # Siren
        (65536, 'Your psionic focus burns at', STAT_RAGE, 10),       # Psion
        (131072, 'Your alien intellect seethes at', STAT_RAGE, 10),  # Mindflayer
    ]

    class_names = {
        16384: 'Dirgesinger',
        32768: 'Siren',
        65536: 'Psion',
        131072: 'Mindflayer',
    }

    for class_id, stat_label, stat_source, display_order in stats_to_add:
        # Check if already exists
        cursor.execute("SELECT 1 FROM class_score_stats WHERE class_id = ?", (class_id,))
        if cursor.fetchone():
            print(f"  {class_names[class_id]} score_stats already exists, skipping")
            continue

        cursor.execute("""
            INSERT INTO class_score_stats (class_id, stat_source, stat_source_max, stat_label, format_string, display_order)
            VALUES (?, ?, 0, ?, '#R[#n%s: #C%d#R]\\n\\r', ?)
        """, (class_id, stat_source, stat_label, display_order))
        print(f"  Added {class_names[class_id]} score_stats")


def main():
    db_path = get_db_path()

    if not db_path.exists():
        print(f"ERROR: Database not found at {db_path}")
        return 1

    print(f"Fixing validation warnings in {db_path}")
    print()

    conn = sqlite3.connect(str(db_path))

    try:
        print("Step 1: Adding Shapeshifter aura...")
        add_shapeshifter_aura(conn)

        print("\nStep 2: Adding Demon armor configuration...")
        add_demon_armor(conn)

        print("\nStep 3: Adding Drow armor configuration...")
        add_drow_armor(conn)

        print("\nStep 4: Adding score stats...")
        add_score_stats(conn)

        conn.commit()
        print("\nAll changes committed successfully!")

    except Exception as e:
        conn.rollback()
        print(f"\nERROR: {e}")
        return 1
    finally:
        conn.close()

    return 0


if __name__ == '__main__':
    exit(main())
