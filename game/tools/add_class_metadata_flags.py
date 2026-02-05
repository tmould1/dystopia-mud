#!/usr/bin/env python3
"""
Add metadata flags to class_registry table.

Adds columns:
- has_class_armor: 1 if class should have armor_config, 0 if armor handled in code/not applicable
- has_score_stats: 1 if class should have score_stats, 0 if score handled in code/not applicable

Default is 1 (expected to have database entries). Classes set to 0 are explicit exceptions.
"""

import sqlite3
from pathlib import Path


def get_db_path() -> Path:
    """Get path to game.db"""
    return Path(__file__).parent.parent.parent / 'gamedata' / 'db' / 'game' / 'game.db'


def add_columns(conn: sqlite3.Connection) -> None:
    """Add the new columns if they don't exist."""
    cursor = conn.cursor()

    # Check existing columns
    cursor.execute("PRAGMA table_info(class_registry)")
    existing_cols = {row[1] for row in cursor.fetchall()}

    if 'has_class_armor' not in existing_cols:
        cursor.execute("""
            ALTER TABLE class_registry
            ADD COLUMN has_class_armor INTEGER NOT NULL DEFAULT 1
        """)
        print("  Added has_class_armor column (default 1)")
    else:
        print("  has_class_armor column already exists")

    if 'has_score_stats' not in existing_cols:
        cursor.execute("""
            ALTER TABLE class_registry
            ADD COLUMN has_score_stats INTEGER NOT NULL DEFAULT 1
        """)
        print("  Added has_score_stats column (default 1)")
    else:
        print("  has_score_stats column already exists")


def set_exception_flags(conn: sqlite3.Connection) -> None:
    """Set flags for classes that handle things in code or don't have the feature."""
    cursor = conn.cursor()

    # Classes without class armor (armor handled differently or not at all)
    # Samurai (16): only has do_katana for weapons, no armor command
    no_armor_classes = [
        (16, 'Samurai'),  # do_katana creates weapon, not armor
    ]

    # Classes without database-driven score stats
    # These either have no custom stats or handle them in code
    no_score_classes = [
        (1, 'Demon'),         # demonic power is hard-coded in score display
        (2, 'Mage'),          # no custom score stats
        (16, 'Samurai'),      # no custom score stats
        (128, 'Ninja'),       # no custom score stats
        (256, 'Lich'),        # no custom score stats
        (4096, 'Undead Knight'),  # no custom score stats
    ]

    print("\n  Setting armor exceptions:")
    for class_id, name in no_armor_classes:
        cursor.execute("""
            UPDATE class_registry SET has_class_armor = 0 WHERE class_id = ?
        """, (class_id,))
        print(f"    {name} ({class_id}): has_class_armor = 0")

    print("\n  Setting score_stats exceptions:")
    for class_id, name in no_score_classes:
        cursor.execute("""
            UPDATE class_registry SET has_score_stats = 0 WHERE class_id = ?
        """, (class_id,))
        print(f"    {name} ({class_id}): has_score_stats = 0")


def show_current_state(conn: sqlite3.Connection) -> None:
    """Show current state of the flags."""
    cursor = conn.cursor()

    print("\n  Current class_registry flags:")
    cursor.execute("""
        SELECT class_id, class_name, has_class_armor, has_score_stats
        FROM class_registry
        ORDER BY class_id
    """)

    print(f"    {'Class':<20} {'ID':>6}  Armor  Score")
    print(f"    {'-'*20} {'-'*6}  {'-'*5}  {'-'*5}")
    for row in cursor.fetchall():
        armor = 'yes' if row[2] else 'NO'
        score = 'yes' if row[3] else 'NO'
        print(f"    {row[1]:<20} {row[0]:>6}  {armor:<5}  {score:<5}")


def main():
    db_path = get_db_path()

    if not db_path.exists():
        print(f"ERROR: Database not found at {db_path}")
        return 1

    print(f"Adding metadata flags to class_registry in {db_path}")
    print()

    conn = sqlite3.connect(str(db_path))

    try:
        print("Step 1: Adding columns...")
        add_columns(conn)

        print("\nStep 2: Setting exception flags...")
        set_exception_flags(conn)

        show_current_state(conn)

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
