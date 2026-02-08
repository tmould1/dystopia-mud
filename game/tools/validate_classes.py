#!/usr/bin/env python3
"""
validate_classes.py - Validate class registry consistency

Checks:
1. CLASS_* constants in class.h match class_registry.class_id values
2. No duplicate color combinations in class_brackets
3. No overlapping vnum ranges in class_vnum_ranges
4. No duplicate bracket combinations in class_brackets
5. Classes with vnum_ranges have class_armor_config entries
6. Classes with armor_config have armor_pieces defined
7. Armor piece vnums fall within class's vnum range
8. All registered classes have entries in required tables (brackets, generations, auras, starting, armor_config)

Usage:
    python validate_classes.py [--db-path PATH]

Default database path: ../../gamedata/db/game/class.db
"""

import argparse
import re
import sqlite3
import sys
from pathlib import Path


def parse_class_h(class_h_path: str) -> dict[str, int]:
    """Parse class.h for CLASS_* defines and return dict of name->value."""
    constants = {}

    with open(class_h_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Match #define CLASS_NAME value patterns
    pattern = r'#define\s+(CLASS_\w+)\s+(\d+)'
    matches = re.findall(pattern, content)

    for name, value in matches:
        constants[name] = int(value)

    return constants


def get_class_registry(db_path: str) -> dict[int, str]:
    """Get class_registry entries as dict of class_id->class_name."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute("SELECT class_id, class_name FROM class_registry")
    entries = {row[0]: row[1] for row in cursor.fetchall()}

    conn.close()
    return entries


def get_class_metadata(db_path: str) -> dict[int, dict]:
    """Get class metadata flags from class_registry.

    Returns dict of class_id -> {has_class_armor, has_score_stats}
    """
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Check if columns exist (for backwards compatibility)
    cursor.execute("PRAGMA table_info(class_registry)")
    columns = {row[1] for row in cursor.fetchall()}

    has_armor_col = 'has_class_armor' in columns
    has_score_col = 'has_score_stats' in columns

    if has_armor_col and has_score_col:
        cursor.execute("""
            SELECT class_id, has_class_armor, has_score_stats
            FROM class_registry
        """)
        entries = {
            row[0]: {'has_class_armor': bool(row[1]), 'has_score_stats': bool(row[2])}
            for row in cursor.fetchall()
        }
    else:
        # Columns don't exist yet - assume all classes need entries
        cursor.execute("SELECT class_id FROM class_registry")
        entries = {
            row[0]: {'has_class_armor': True, 'has_score_stats': True}
            for row in cursor.fetchall()
        }

    conn.close()
    return entries


def get_class_brackets(db_path: str) -> list[dict]:
    """Get class_brackets entries with colors."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Check if color columns exist
    cursor.execute("PRAGMA table_info(class_brackets)")
    columns = {row[1] for row in cursor.fetchall()}
    has_colors = 'accent_color' in columns

    if has_colors:
        cursor.execute("""
            SELECT b.class_id, r.class_name, b.open_bracket, b.close_bracket,
                   b.accent_color, b.primary_color
            FROM class_brackets b
            JOIN class_registry r ON b.class_id = r.class_id
        """)
    else:
        print("  NOTE: accent_color/primary_color columns not yet in database")
        print("        Run the server once to auto-migrate existing database")
        cursor.execute("""
            SELECT b.class_id, r.class_name, b.open_bracket, b.close_bracket
            FROM class_brackets b
            JOIN class_registry r ON b.class_id = r.class_id
        """)

    entries = []
    for row in cursor.fetchall():
        entry = {
            'class_id': row[0],
            'class_name': row[1],
            'open_bracket': row[2],
            'close_bracket': row[3],
            'accent_color': row[4] if has_colors else None,
            'primary_color': row[5] if has_colors else None
        }
        entries.append(entry)

    conn.close()
    return entries


def get_vnum_ranges(db_path: str) -> list[dict]:
    """Get class_vnum_ranges entries."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        cursor.execute("""
            SELECT class_id, armor_vnum_start, armor_vnum_end,
                   mastery_vnum, description
            FROM class_vnum_ranges
        """)

        entries = []
        for row in cursor.fetchall():
            entries.append({
                'class_id': row[0],
                'armor_vnum_start': row[1],
                'armor_vnum_end': row[2],
                'mastery_vnum': row[3],
                'description': row[4]
            })
    except sqlite3.OperationalError as e:
        if "no such table" in str(e):
            print("  WARNING: class_vnum_ranges table does not exist yet")
            entries = []
        else:
            raise

    conn.close()
    return entries


def get_armor_config(db_path: str) -> dict[int, dict]:
    """Get class_armor_config entries as dict of class_id->config."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        cursor.execute("""
            SELECT class_id, acfg_cost_key, usage_message
            FROM class_armor_config
        """)

        entries = {}
        for row in cursor.fetchall():
            entries[row[0]] = {
                'class_id': row[0],
                'acfg_cost_key': row[1],
                'usage_message': row[2]
            }
    except sqlite3.OperationalError as e:
        if "no such table" in str(e):
            print("  WARNING: class_armor_config table does not exist yet")
            entries = {}
        else:
            raise

    conn.close()
    return entries


def get_armor_pieces(db_path: str) -> dict[int, list[dict]]:
    """Get class_armor_pieces grouped by class_id."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        cursor.execute("""
            SELECT class_id, keyword, vnum
            FROM class_armor_pieces
            ORDER BY class_id, keyword
        """)

        entries = {}
        for row in cursor.fetchall():
            class_id = row[0]
            if class_id not in entries:
                entries[class_id] = []
            entries[class_id].append({
                'keyword': row[1],
                'vnum': row[2]
            })
    except sqlite3.OperationalError as e:
        if "no such table" in str(e):
            print("  WARNING: class_armor_pieces table does not exist yet")
            entries = {}
        else:
            raise

    conn.close()
    return entries


def get_class_auras(db_path: str) -> set[int]:
    """Get class_ids that have aura entries."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        cursor.execute("SELECT DISTINCT class_id FROM class_auras")
        class_ids = {row[0] for row in cursor.fetchall()}
    except sqlite3.OperationalError:
        class_ids = set()

    conn.close()
    return class_ids


def get_class_starting(db_path: str) -> set[int]:
    """Get class_ids that have starting entries."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        cursor.execute("SELECT DISTINCT class_id FROM class_starting")
        class_ids = {row[0] for row in cursor.fetchall()}
    except sqlite3.OperationalError:
        class_ids = set()

    conn.close()
    return class_ids


def get_class_generations(db_path: str) -> set[int]:
    """Get class_ids that have generation entries."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        cursor.execute("SELECT DISTINCT class_id FROM class_generations")
        class_ids = {row[0] for row in cursor.fetchall()}
    except sqlite3.OperationalError:
        class_ids = set()

    conn.close()
    return class_ids


def get_class_score_stats(db_path: str) -> set[int]:
    """Get class_ids that have score stats entries."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        cursor.execute("SELECT DISTINCT class_id FROM class_score_stats")
        class_ids = {row[0] for row in cursor.fetchall()}
    except sqlite3.OperationalError:
        class_ids = set()

    conn.close()
    return class_ids


def check_constants(class_h_constants: dict, db_registry: dict) -> tuple[int, int]:
    """Check if CLASS_* constants match database entries."""
    errors = 0
    warnings = 0

    print("\nChecking class constants...")

    # Check each constant against database
    for name, value in sorted(class_h_constants.items()):
        if value in db_registry:
            print(f"  OK: {name} ({value}) matches class_registry ({db_registry[value]})")
        else:
            print(f"  WARNING: {name} ({value}) not found in class_registry")
            warnings += 1

    # Check for database entries without constants
    defined_values = set(class_h_constants.values())
    for class_id, class_name in db_registry.items():
        if class_id not in defined_values:
            print(f"  WARNING: {class_name} ({class_id}) in database but no CLASS_* constant")
            warnings += 1

    return errors, warnings


def check_color_conflicts(brackets: list[dict]) -> int:
    """Check for duplicate accent/primary color combinations."""
    errors = 0

    print("\nChecking color conflicts...")

    # Check if any colors are defined
    has_any_colors = any(e['accent_color'] or e['primary_color'] for e in brackets)
    if not has_any_colors:
        print("  SKIP: No color data available (database needs update)")
        return 0

    # Group by color combination (only include entries with colors)
    color_combos = {}
    for entry in brackets:
        accent = entry['accent_color'] or ''
        primary = entry['primary_color'] or ''

        # Skip entries without any color data
        if not accent and not primary:
            continue

        combo = (accent, primary)

        if combo not in color_combos:
            color_combos[combo] = []
        color_combos[combo].append(entry['class_name'])

    # Report duplicates
    for combo, classes in color_combos.items():
        if len(classes) > 1:
            print(f"  WARNING: Color combo ({combo[0]}, {combo[1]}) used by: {', '.join(classes)}")
            errors += 1

    if errors == 0:
        print("  OK: No color conflicts found")

    return errors


def check_bracket_duplicates(brackets: list[dict]) -> int:
    """Check for duplicate bracket combinations."""
    errors = 0

    print("\nChecking bracket uniqueness...")

    # Group by bracket combination
    bracket_combos = {}
    for entry in brackets:
        combo = (entry['open_bracket'], entry['close_bracket'])

        if combo not in bracket_combos:
            bracket_combos[combo] = []
        bracket_combos[combo].append(entry['class_name'])

    # Report duplicates
    for combo, classes in bracket_combos.items():
        if len(classes) > 1:
            print(f"  WARNING: Bracket combo used by multiple classes: {', '.join(classes)}")
            errors += 1

    if errors == 0:
        print("  OK: All bracket combinations are unique")

    return errors


def check_armor_consistency(
    db_registry: dict[int, str],
    vnum_ranges: list[dict],
    armor_config: dict[int, dict],
    armor_pieces: dict[int, list[dict]]
) -> tuple[int, int]:
    """Check cross-table consistency for armor-related tables."""
    errors = 0
    warnings = 0

    print("\nChecking armor table consistency...")

    # Get class_ids from each table
    vnum_class_ids = {r['class_id'] for r in vnum_ranges}
    config_class_ids = set(armor_config.keys())
    pieces_class_ids = set(armor_pieces.keys())

    # Check: classes with vnum_ranges should have armor_config
    missing_config = vnum_class_ids - config_class_ids
    if missing_config:
        for class_id in sorted(missing_config):
            class_name = db_registry.get(class_id, f"Unknown({class_id})")
            print(f"  WARNING: {class_name} ({class_id}) has vnum_range but no armor_config")
            warnings += 1

    # Check: classes with armor_config should have armor_pieces (if they have vnum_ranges)
    config_with_vnums = config_class_ids & vnum_class_ids
    missing_pieces = config_with_vnums - pieces_class_ids
    if missing_pieces:
        for class_id in sorted(missing_pieces):
            class_name = db_registry.get(class_id, f"Unknown({class_id})")
            print(f"  WARNING: {class_name} ({class_id}) has armor_config but no armor_pieces")
            warnings += 1

    # Check: armor piece vnums should fall within the class's vnum range
    vnum_range_map = {r['class_id']: r for r in vnum_ranges}
    for class_id, pieces in armor_pieces.items():
        if class_id not in vnum_range_map:
            continue  # Skip classes without tracked vnum ranges

        r = vnum_range_map[class_id]
        start, end = r['armor_vnum_start'], r['armor_vnum_end']
        class_name = db_registry.get(class_id, f"Unknown({class_id})")

        for piece in pieces:
            vnum = piece['vnum']
            if vnum < start or vnum > end:
                print(f"  ERROR: {class_name} piece '{piece['keyword']}' vnum {vnum} "
                      f"outside range {start}-{end}")
                errors += 1

    if errors == 0 and warnings == 0:
        print("  OK: Armor tables are consistent")

    return errors, warnings


def check_registry_completeness(
    db_path: str,
    db_registry: dict[int, str],
    brackets: list[dict],
    armor_config: dict[int, dict]
) -> tuple[int, int]:
    """Check that all registered classes have entries in required tables.

    Required tables (every class must have an entry):
    - class_brackets: WHO list display
    - class_generations: Generation titles
    - class_auras: Room aura text
    - class_starting: Starting stats

    Conditional tables (checked against class_registry metadata flags):
    - class_armor_config: Required if has_class_armor=1
    - class_score_stats: Required if has_score_stats=1
    """
    errors = 0
    warnings = 0

    print("\nChecking registry completeness...")

    # Get class_ids from each table
    bracket_ids = {b['class_id'] for b in brackets}
    generation_ids = get_class_generations(db_path)
    aura_ids = get_class_auras(db_path)
    starting_ids = get_class_starting(db_path)
    armor_config_ids = set(armor_config.keys())
    score_stat_ids = get_class_score_stats(db_path)
    class_metadata = get_class_metadata(db_path)

    all_class_ids = set(db_registry.keys())

    # Tables where every class MUST have an entry
    required_tables = [
        ('class_brackets', bracket_ids, 'WHO list brackets'),
        ('class_generations', generation_ids, 'generation titles'),
        ('class_auras', aura_ids, 'room aura'),
        ('class_starting', starting_ids, 'starting values'),
    ]

    missing_by_class = {}  # class_id -> list of missing tables

    for table_name, table_ids, description in required_tables:
        missing = all_class_ids - table_ids
        for class_id in missing:
            if class_id not in missing_by_class:
                missing_by_class[class_id] = []
            missing_by_class[class_id].append(description)

    # Check conditional tables based on metadata flags
    for class_id in all_class_ids:
        meta = class_metadata.get(class_id, {'has_class_armor': True, 'has_score_stats': True})

        # Check armor_config if class should have it
        if meta['has_class_armor'] and class_id not in armor_config_ids:
            if class_id not in missing_by_class:
                missing_by_class[class_id] = []
            missing_by_class[class_id].append('armor config')

        # Check score_stats if class should have it
        if meta['has_score_stats'] and class_id not in score_stat_ids:
            if class_id not in missing_by_class:
                missing_by_class[class_id] = []
            missing_by_class[class_id].append('score stats')

    # Report missing entries grouped by class
    if missing_by_class:
        for class_id in sorted(missing_by_class.keys()):
            class_name = db_registry.get(class_id, f"Unknown({class_id})")
            missing_tables = missing_by_class[class_id]
            print(f"  WARNING: {class_name} ({class_id}) missing: {', '.join(missing_tables)}")
            warnings += 1
    else:
        print("  OK: All classes have entries in required tables")

    # Show classes with exceptions (informational)
    armor_exceptions = [cid for cid, m in class_metadata.items() if not m['has_class_armor']]
    score_exceptions = [cid for cid, m in class_metadata.items() if not m['has_score_stats']]

    if armor_exceptions or score_exceptions:
        print("\n  Exceptions (handled in code):")
        if armor_exceptions:
            names = [db_registry.get(cid, f"({cid})") for cid in sorted(armor_exceptions)]
            print(f"    has_class_armor=0: {', '.join(names)}")
        if score_exceptions:
            names = [db_registry.get(cid, f"({cid})") for cid in sorted(score_exceptions)]
            print(f"    has_score_stats=0: {', '.join(names)}")

    return errors, warnings


def check_vnum_overlaps(ranges: list[dict], db_registry: dict[int, str]) -> int:
    """Check for overlapping vnum ranges."""
    errors = 0

    print("\nChecking vnum ranges...")

    if not ranges:
        print("  No vnum ranges defined")
        return 0

    # Sort by start vnum
    sorted_ranges = sorted(ranges, key=lambda x: x['armor_vnum_start'])

    # Check for overlaps
    for i, r1 in enumerate(sorted_ranges):
        for j, r2 in enumerate(sorted_ranges):
            if i >= j:
                continue

            # Check if ranges overlap
            if (r1['armor_vnum_start'] <= r2['armor_vnum_end'] and
                r1['armor_vnum_end'] >= r2['armor_vnum_start']):
                name1 = db_registry.get(r1['class_id'], f"Unknown({r1['class_id']})")
                name2 = db_registry.get(r2['class_id'], f"Unknown({r2['class_id']})")
                print(f"  ERROR: Vnum overlap between {name1} "
                      f"({r1['armor_vnum_start']}-{r1['armor_vnum_end']}) and "
                      f"{name2} ({r2['armor_vnum_start']}-{r2['armor_vnum_end']})")
                errors += 1

    if errors == 0:
        print("  OK: No overlapping vnum ranges")

    # Show current ranges
    print("\n  Current vnum ranges:")
    for r in sorted_ranges:
        class_name = db_registry.get(r['class_id'], f"Unknown({r['class_id']})")
        mastery = f", mastery={r['mastery_vnum']}" if r['mastery_vnum'] else ""
        print(f"    {class_name:15} ({r['class_id']:6}): {r['armor_vnum_start']}-{r['armor_vnum_end']}{mastery}")

    # Show next available
    if sorted_ranges:
        max_end = max(r['armor_vnum_end'] for r in sorted_ranges)
        for r in sorted_ranges:
            if r['mastery_vnum'] and r['mastery_vnum'] > max_end:
                max_end = r['mastery_vnum']
        print(f"\n  Next available vnum range starts at: {max_end + 20}")

    return errors


def main():
    """Entry point for class registry validation."""
    parser = argparse.ArgumentParser(description='Validate class registry consistency')
    parser.add_argument('--db-path', type=str,
                        help='Path to class.db (default: ../../gamedata/db/game/class.db)')
    args = parser.parse_args()

    # Determine paths relative to script location
    script_dir = Path(__file__).parent

    if args.db_path:
        db_path = Path(args.db_path)
    else:
        db_path = script_dir / '../../gamedata/db/game/class.db'

    class_h_path = script_dir / '../src/classes/class.h'

    # Verify paths exist
    if not db_path.exists():
        print(f"ERROR: Database not found: {db_path}")
        sys.exit(1)

    if not class_h_path.exists():
        print(f"ERROR: class.h not found: {class_h_path}")
        sys.exit(1)

    print("Validating class registry...")
    print(f"  Database: {db_path}")
    print(f"  class.h: {class_h_path}")

    # Load data
    class_h_constants = parse_class_h(str(class_h_path))
    db_registry = get_class_registry(str(db_path))
    brackets = get_class_brackets(str(db_path))
    vnum_ranges = get_vnum_ranges(str(db_path))
    armor_config = get_armor_config(str(db_path))
    armor_pieces = get_armor_pieces(str(db_path))

    # Run checks
    total_errors = 0
    total_warnings = 0

    errors, warnings = check_constants(class_h_constants, db_registry)
    total_errors += errors
    total_warnings += warnings

    total_errors += check_color_conflicts(brackets)
    total_errors += check_bracket_duplicates(brackets)
    total_errors += check_vnum_overlaps(vnum_ranges, db_registry)

    errors, warnings = check_armor_consistency(
        db_registry, vnum_ranges, armor_config, armor_pieces
    )
    total_errors += errors
    total_warnings += warnings

    errors, warnings = check_registry_completeness(
        str(db_path), db_registry, brackets, armor_config
    )
    total_errors += errors
    total_warnings += warnings

    # Summary
    print("\n" + "=" * 50)
    if total_errors == 0 and total_warnings == 0:
        print("PASS: All checks passed!")
        sys.exit(0)
    else:
        print(f"RESULT: {total_errors} errors, {total_warnings} warnings")
        sys.exit(1 if total_errors > 0 else 0)


if __name__ == '__main__':
    main()
