#!/usr/bin/env python3
"""
Generate cfg_keys.h X-macro header from balance.c and acfg_keys.h.

This script unifies the balance and acfg systems into a single cfg system.

Sources:
1. balance.c balance_map[] - converted to combat.*/progression.*/world.* keys
2. acfg_keys.h ACFG_ENTRIES - converted to ability.* keys

Output files:
- cfg_keys.h - X-macro definitions and cfg_key_t enum
- cfg_key_to_enum.json - Mapping for migration script
- cfg_keys.py - Python module for MudEdit validation/autocomplete

Usage:
    python generate_cfg_keys.py
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Tuple, Optional

# Paths
SCRIPT_DIR = Path(__file__).parent
GAME_DIR = SCRIPT_DIR.parent
SRC_CORE_DIR = GAME_DIR / "src" / "core"
MUDEDIT_DIR = SCRIPT_DIR / "mudedit"

# Input files
BALANCE_C = SRC_CORE_DIR / "balance.c"
ACFG_KEYS_H = SRC_CORE_DIR / "acfg_keys.h"

# Output files
CFG_KEYS_H = SRC_CORE_DIR / "cfg_keys.h"
CFG_KEY_TO_ENUM_JSON = SCRIPT_DIR / "cfg_key_to_enum.json"
CFG_KEYS_PY = MUDEDIT_DIR / "cfg_keys.py"


@dataclass
class CfgEntry:
    """A single configuration entry."""
    enum_name: str      # COMBAT_BASE_DAMCAP
    key: str            # "combat.base_damcap"
    default: int        # 1000
    category: str       # "combat", "ability", etc.
    source: str         # "balance" or "acfg"


def key_to_enum_name(key: str) -> str:
    """
    Convert a dotted key like "combat.damcap.mage"
    to an enum name like "COMBAT_DAMCAP_MAGE"
    """
    name = key.replace(".", "_").replace("-", "_")
    return name.upper()


def categorize_balance_key(old_key: str) -> Tuple[str, str]:
    """
    Convert a balance key to a new cfg key with proper category.
    Returns (new_key, category).
    """
    # Progression-related keys
    if old_key.startswith("upgrade_") or old_key.startswith("gen_"):
        return f"progression.{old_key}", "progression"

    # World settings
    if old_key == "time_scale":
        return f"world.{old_key}", "world"

    # Damage caps get their own subcategory, grouped by class
    if old_key.startswith("damcap_"):
        suffix = old_key[7:]  # Remove "damcap_"
        # Group by class prefix
        class_prefixes = [
            "vamp", "ww", "demon", "drow", "mage", "monk", "ninja", "samurai",
            "tanarri", "shape", "droid", "lich", "angel", "uk", "dragonkin",
            "wyrm", "dirgesinger", "siren", "stance", "superstance", "rev_superstance",
            "artifact"
        ]
        for prefix in sorted(class_prefixes, key=len, reverse=True):  # Longest first
            if suffix.startswith(prefix + "_"):
                rest = suffix[len(prefix)+1:]  # Remove prefix and underscore
                return f"combat.damcap.{prefix}.{rest}", f"combat.damcap.{prefix}"
            elif suffix == prefix:  # Exact match (e.g., "mage", "artifact")
                return f"combat.damcap.{prefix}", "combat.damcap"
        # No class prefix found, keep as-is
        return f"combat.damcap.{suffix}", "combat.damcap"

    # Weapon skill caps
    if old_key.startswith("wpn_cap_"):
        suffix = old_key[8:]  # Remove "wpn_cap_"
        return f"combat.wpn_cap.{suffix}", "combat.wpn_cap"

    # Weapon damage formula
    if old_key.startswith("wpn_dam_") or old_key.startswith("wpn_gen"):
        return f"combat.{old_key}", "combat"

    # Damage multipliers
    if old_key.startswith("dmg_mult_"):
        suffix = old_key[9:]  # Remove "dmg_mult_"
        return f"combat.dmg_mult.{suffix}", "combat.dmg_mult"

    # Everything else is combat
    return f"combat.{old_key}", "combat"


def parse_balance_c(filepath: Path) -> List[CfgEntry]:
    """
    Parse balance.c to extract balance_map[] entries and defaults.
    Returns list of CfgEntry objects.
    """
    content = filepath.read_text(encoding="utf-8")
    entries = []

    # Parse balance_map entries: { "key", &balance.field }
    map_pattern = r'\{\s*"([^"]+)"\s*,\s*&balance\.(\w+(?:\[\d+\])?)\s*\}'

    # Parse defaults from load_balance(): balance.field = value;
    # The array index part is optional: balance.field or balance.field[0]
    default_pattern = r'balance\.(\w+(?:\[\d+\])?)\s*=\s*(-?\d+)\s*;'

    # Extract defaults
    defaults: Dict[str, int] = {}
    for match in re.finditer(default_pattern, content):
        field = match.group(1)
        value = int(match.group(2))
        defaults[field] = value

    # Extract map entries and match with defaults
    for match in re.finditer(map_pattern, content):
        old_key = match.group(1)
        field = match.group(2)

        # Get default value from load_balance()
        default = defaults.get(field, 0)

        # Convert to new key format
        new_key, category = categorize_balance_key(old_key)
        enum_name = key_to_enum_name(new_key)

        entries.append(CfgEntry(
            enum_name=enum_name,
            key=new_key,
            default=default,
            category=category,
            source="balance"
        ))

    return entries


def parse_acfg_keys_h(filepath: Path) -> List[CfgEntry]:
    """
    Parse acfg_keys.h ACFG_ENTRIES to extract ability config entries.
    Returns list of CfgEntry objects with "ability." prefix added.
    """
    content = filepath.read_text(encoding="utf-8")
    entries = []

    # Match X-macro: ACFG_X(ENUM_NAME, "key", default)
    xmacro_pattern = r'ACFG_X\(\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*(-?\d+)\s*\)'

    for match in re.finditer(xmacro_pattern, content):
        old_enum = match.group(1)
        old_key = match.group(2)
        default = int(match.group(3))

        # Add ability. prefix to key
        new_key = f"ability.{old_key}"

        # Determine subcategory from the class name
        class_name = old_key.split(".")[0]
        category = f"ability.{class_name}"

        # Convert enum name
        enum_name = f"ABILITY_{old_enum}"

        entries.append(CfgEntry(
            enum_name=enum_name,
            key=new_key,
            default=default,
            category=category,
            source="acfg"
        ))

    return entries


def generate_cfg_keys_h(entries: List[CfgEntry]) -> str:
    """Generate the cfg_keys.h X-macro header content."""
    lines = []

    # Header
    lines.append("""/***************************************************************************
 *  cfg_keys.h - Unified configuration keys (auto-generated)
 *
 *  This file is the SINGLE SOURCE OF TRUTH for all game config entries.
 *  The enum and table are both generated from the CFG_ENTRIES macro.
 *
 *  Categories:
 *    CFG_COMBAT_*      - Global combat parameters
 *    CFG_PROGRESSION_* - Upgrade/generation bonuses
 *    CFG_WORLD_*       - Time, weather, world settings
 *    CFG_ABILITY_*     - Per-class ability parameters
 *
 *  DO NOT EDIT MANUALLY - regenerate using:
 *      python game/tools/generate_cfg_keys.py
 ***************************************************************************/

#ifndef CFG_KEYS_H
#define CFG_KEYS_H

/*
 * X-Macro definitions for config keys.
 * Format: CFG_X(ENUM_NAME, "string.key", default_value)
 */
#define CFG_ENTRIES \\""")

    # Group entries by category
    current_category = None

    # Sort entries by category to group them nicely
    sorted_entries = sorted(entries, key=lambda e: (
        0 if e.category.startswith("combat") else
        1 if e.category.startswith("progression") else
        2 if e.category.startswith("world") else
        3,  # ability entries
        e.category,
        e.key
    ))

    for entry in sorted_entries:
        # Add section comment when category changes
        if entry.category != current_category:
            current_category = entry.category
            # Create readable section header
            section = current_category.upper().replace(".", " - ")
            lines.append(f"    \\")
            lines.append(f"    /* =========== {section} =========== */ \\")

        # Output the X-macro entry
        lines.append(f'    CFG_X({entry.enum_name:55s}, "{entry.key}", {entry.default:>10}) \\')

    # Remove trailing backslash from last entry
    if lines:
        lines[-1] = lines[-1].rstrip(" \\")

    lines.append("")
    lines.append("")

    # Generate enum
    lines.append("""/* Generate enum from X-macro */
typedef enum {
#define CFG_X(name, key, def) CFG_##name,
    CFG_ENTRIES
#undef CFG_X
    CFG_COUNT  /* Total count, also serves as invalid sentinel */
} cfg_key_t;

#endif /* CFG_KEYS_H */
""")

    return "\n".join(lines)


def generate_migration_mappings(entries: List[CfgEntry]) -> Dict:
    """Generate mappings for migration scripts."""
    acfg_to_cfg = {}
    balance_to_cfg = {}

    # Build a map from balance_map keys to actual field references
    # by parsing balance.c again
    balance_c = Path(__file__).parent.parent / "src" / "core" / "balance.c"
    key_to_field = {}
    if balance_c.exists():
        content = balance_c.read_text(encoding="utf-8")
        # Parse: { "upgrade_dmg_1", &balance.upgrade_dmg[0] }
        pattern = r'\{\s*"([^"]+)"\s*,\s*&balance\.(\w+(?:\[\d+\])?)\s*\}'
        for match in re.finditer(pattern, content):
            map_key = match.group(1)
            field_ref = match.group(2)
            key_to_field[map_key] = field_ref

    for entry in entries:
        if entry.source == "acfg":
            # Map old ACFG enum to new CFG enum
            old_enum = entry.enum_name.replace("ABILITY_", "ACFG_")
            new_enum = f"CFG_{entry.enum_name}"
            acfg_to_cfg[old_enum] = new_enum
        elif entry.source == "balance":
            # Map old balance.field pattern to new cfg() call
            # Extract the original key from the new key
            old_key = entry.key.split(".", 1)[1] if "." in entry.key else entry.key
            # Handle special cases to get the balance_map key
            if entry.key.startswith("combat.damcap."):
                old_key = "damcap_" + entry.key.split(".")[-1]
            elif entry.key.startswith("combat.wpn_cap."):
                old_key = "wpn_cap_" + entry.key.split(".")[-1]
            elif entry.key.startswith("combat.dmg_mult."):
                old_key = "dmg_mult_" + entry.key.split(".")[-1]
            elif entry.key.startswith("progression.") or entry.key.startswith("world."):
                old_key = entry.key.split(".", 1)[1]
            elif entry.key.startswith("combat."):
                old_key = entry.key.split(".", 1)[1]

            cfg_call = f"cfg( CFG_{entry.enum_name} )"

            # Add mapping for the balance_map key style (e.g., upgrade_dmg_1)
            balance_to_cfg[f"balance.{old_key}"] = cfg_call

            # Also add mapping for actual field reference if different
            # e.g., balance.upgrade_dmg[0] for upgrade_dmg_1
            if old_key in key_to_field:
                field_ref = key_to_field[old_key]
                if field_ref != old_key:
                    balance_to_cfg[f"balance.{field_ref}"] = cfg_call

    return {
        "acfg_to_cfg": acfg_to_cfg,
        "balance_to_cfg": balance_to_cfg,
    }


def generate_python_module(entries: List[CfgEntry]) -> str:
    """Generate Python module for MudEdit validation."""
    lines = []

    lines.append('"""')
    lines.append("cfg_keys.py - Valid config keys (auto-generated)")
    lines.append("")
    lines.append("This module provides validation and autocomplete for cfg keys in MudEdit.")
    lines.append("DO NOT EDIT MANUALLY - regenerate using:")
    lines.append("    python game/tools/generate_cfg_keys.py")
    lines.append('"""')
    lines.append("")
    lines.append("from typing import Set, List, Dict, Optional")
    lines.append("")
    lines.append("")

    # Generate ALL_KEYS set
    lines.append("# All valid cfg keys")
    lines.append("ALL_KEYS: Set[str] = {")
    for entry in sorted(entries, key=lambda e: e.key):
        lines.append(f'    "{entry.key}",')
    lines.append("}")
    lines.append("")
    lines.append("")

    # Generate KEYS_BY_CATEGORY dict
    lines.append("# Keys organized by category")
    lines.append("KEYS_BY_CATEGORY: Dict[str, List[str]] = {")

    categories: Dict[str, List[str]] = {}
    for entry in entries:
        # Use top-level category (combat, progression, world, ability)
        top_cat = entry.category.split(".")[0]
        if top_cat not in categories:
            categories[top_cat] = []
        categories[top_cat].append(entry.key)

    for cat in sorted(categories.keys()):
        lines.append(f'    "{cat}": [')
        for key in sorted(categories[cat]):
            lines.append(f'        "{key}",')
        lines.append("    ],")

    lines.append("}")
    lines.append("")
    lines.append("")

    # Generate KEY_DEFAULTS dict
    lines.append("# Default values for each key")
    lines.append("KEY_DEFAULTS: Dict[str, int] = {")
    for entry in sorted(entries, key=lambda e: e.key):
        lines.append(f'    "{entry.key}": {entry.default},')
    lines.append("}")
    lines.append("")
    lines.append("")

    # Helper functions
    lines.append("def is_valid_key(key: str) -> bool:")
    lines.append('    """Check if a key is valid."""')
    lines.append("    return key in ALL_KEYS")
    lines.append("")
    lines.append("")
    lines.append("def get_default(key: str) -> Optional[int]:")
    lines.append('    """Get the default value for a key, or None if invalid."""')
    lines.append("    return KEY_DEFAULTS.get(key)")
    lines.append("")
    lines.append("")
    lines.append("def get_categories() -> List[str]:")
    lines.append('    """Get list of all category names."""')
    lines.append("    return list(KEYS_BY_CATEGORY.keys())")
    lines.append("")
    lines.append("")
    lines.append("def get_keys_for_category(category: str) -> List[str]:")
    lines.append('    """Get all keys for a specific category."""')
    lines.append("    return KEYS_BY_CATEGORY.get(category, [])")
    lines.append("")
    lines.append("")
    lines.append("def search_keys(prefix: str) -> List[str]:")
    lines.append('    """Search for keys matching a prefix (case-insensitive)."""')
    lines.append("    prefix_lower = prefix.lower()")
    lines.append("    return [k for k in ALL_KEYS if k.lower().startswith(prefix_lower)]")
    lines.append("")

    return "\n".join(lines)


def main():
    """Main entry point for cfg keys generation."""
    print("=" * 60)
    print("Generating unified cfg_keys.h")
    print("=" * 60)

    all_entries: List[CfgEntry] = []

    # Parse balance.c
    if BALANCE_C.exists():
        print(f"\nParsing {BALANCE_C}...")
        balance_entries = parse_balance_c(BALANCE_C)
        print(f"  Found {len(balance_entries)} balance entries")
        all_entries.extend(balance_entries)
    else:
        print(f"WARNING: {BALANCE_C} not found!")

    # Parse acfg_keys.h
    if ACFG_KEYS_H.exists():
        print(f"\nParsing {ACFG_KEYS_H}...")
        acfg_entries = parse_acfg_keys_h(ACFG_KEYS_H)
        print(f"  Found {len(acfg_entries)} ability config entries")
        all_entries.extend(acfg_entries)
    else:
        print(f"WARNING: {ACFG_KEYS_H} not found!")

    print(f"\nTotal entries: {len(all_entries)}")

    # Count by category
    categories: Dict[str, int] = {}
    for entry in all_entries:
        top_cat = entry.category.split(".")[0]
        categories[top_cat] = categories.get(top_cat, 0) + 1

    print("\nEntries by category:")
    for cat in sorted(categories.keys()):
        print(f"  {cat}: {categories[cat]}")

    # Generate cfg_keys.h
    print(f"\nGenerating {CFG_KEYS_H}...")
    header_content = generate_cfg_keys_h(all_entries)
    CFG_KEYS_H.write_text(header_content, encoding="utf-8")
    print(f"  Written {len(header_content)} bytes")

    # Generate migration mappings
    print(f"\nGenerating {CFG_KEY_TO_ENUM_JSON}...")
    mappings = generate_migration_mappings(all_entries)
    CFG_KEY_TO_ENUM_JSON.write_text(json.dumps(mappings, indent=2), encoding="utf-8")
    print(f"  Written {len(mappings['acfg_to_cfg'])} acfg mappings")
    print(f"  Written {len(mappings['balance_to_cfg'])} balance mappings")

    # Generate Python module
    if MUDEDIT_DIR.exists():
        print(f"\nGenerating {CFG_KEYS_PY}...")
        python_content = generate_python_module(all_entries)
        CFG_KEYS_PY.write_text(python_content, encoding="utf-8")
        print(f"  Written {len(python_content)} bytes")
    else:
        print(f"\nSkipping {CFG_KEYS_PY} (mudedit directory not found)")

    print("\n" + "=" * 60)
    print("Done! Generated files:")
    print(f"  - {CFG_KEYS_H}")
    print(f"  - {CFG_KEY_TO_ENUM_JSON}")
    if MUDEDIT_DIR.exists():
        print(f"  - {CFG_KEYS_PY}")
    print("=" * 60)


if __name__ == "__main__":
    main()
