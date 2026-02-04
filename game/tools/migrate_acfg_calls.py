#!/usr/bin/env python3
"""
Migrate acfg("string.key") calls to acfg(ACFG_ENUM_KEY) format.

This script reads the key-to-enum mapping and updates all C source files
to use the type-safe enum-based acfg() calls.

Usage:
    python migrate_acfg_calls.py [--dry-run]

Options:
    --dry-run   Show what would be changed without modifying files
"""

import re
import json
import sys
from pathlib import Path

# Paths
SCRIPT_DIR = Path(__file__).parent
GAME_DIR = SCRIPT_DIR.parent
SRC_DIR = GAME_DIR / "src"
KEY_TO_ENUM_JSON = SCRIPT_DIR / "acfg_key_to_enum.json"


def load_key_mapping() -> dict[str, str]:
    """Load the key-to-enum mapping from JSON."""
    with open(KEY_TO_ENUM_JSON, encoding="utf-8") as f:
        return json.load(f)


def migrate_file(filepath: Path, key_to_enum: dict[str, str], dry_run: bool = False) -> tuple[int, list[str]]:
    """
    Migrate acfg("key") calls in a single file.

    Returns (count of replacements, list of unmatched keys).
    """
    content = filepath.read_text(encoding="utf-8")
    original = content

    # Pattern: acfg( "some.key.here" )  with optional whitespace
    pattern = r'acfg\(\s*"([^"]+)"\s*\)'

    count = 0
    unmatched = []

    def replace_match(m):
        nonlocal count, unmatched
        key = m.group(1)
        if key in key_to_enum:
            count += 1
            return f'acfg( {key_to_enum[key]} )'
        else:
            unmatched.append(key)
            return m.group(0)  # Keep original if not found

    content = re.sub(pattern, replace_match, content)

    if content != original and not dry_run:
        filepath.write_text(content, encoding="utf-8")

    return count, unmatched


def main():
    dry_run = "--dry-run" in sys.argv

    if dry_run:
        print("DRY RUN - no files will be modified\n")

    print(f"Loading key mapping from {KEY_TO_ENUM_JSON}...")
    key_to_enum = load_key_mapping()
    print(f"  Loaded {len(key_to_enum)} key mappings\n")

    total_replacements = 0
    all_unmatched = set()
    modified_files = []

    # Find all .c files in src directory
    c_files = list(SRC_DIR.rglob("*.c"))
    print(f"Scanning {len(c_files)} source files...\n")

    for filepath in sorted(c_files):
        rel_path = filepath.relative_to(GAME_DIR)
        count, unmatched = migrate_file(filepath, key_to_enum, dry_run)

        if count > 0:
            modified_files.append((rel_path, count))
            total_replacements += count
            status = "[would modify]" if dry_run else "[modified]"
            print(f"  {status} {rel_path}: {count} replacements")

        all_unmatched.update(unmatched)

    print(f"\n{'=' * 60}")
    print(f"Summary:")
    print(f"  Files modified: {len(modified_files)}")
    print(f"  Total replacements: {total_replacements}")

    if all_unmatched:
        print(f"\n  WARNING: {len(all_unmatched)} unmatched keys:")
        for key in sorted(all_unmatched):
            print(f"    - {key}")

    if dry_run:
        print(f"\nRun without --dry-run to apply changes.")


if __name__ == "__main__":
    main()
