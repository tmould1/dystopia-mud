#!/usr/bin/env python3
"""
Generate acfg_keys.h X-macro header from existing ability_config.c table.

This script parses the acfg_table[] in ability_config.c and generates:
1. acfg_keys.h - X-macro definitions and acfg_key_t enum
2. key_to_enum.json - Mapping for migration script
3. acfg_keys.py - Python module for MudEdit validation/autocomplete

Usage:
    python generate_acfg_keys.py

Output files are written to game/src/core/ and game/tools/mudedit/
"""

import re
import json
from pathlib import Path

# Paths
SCRIPT_DIR = Path(__file__).parent
GAME_DIR = SCRIPT_DIR.parent
SRC_CORE_DIR = GAME_DIR / "src" / "core"
MUDEDIT_DIR = SCRIPT_DIR / "mudedit"
ABILITY_CONFIG_C = SRC_CORE_DIR / "ability_config.c"
ACFG_KEYS_H = SRC_CORE_DIR / "acfg_keys.h"
KEY_TO_ENUM_JSON = SCRIPT_DIR / "acfg_key_to_enum.json"
ACFG_KEYS_PY = MUDEDIT_DIR / "acfg_keys.py"


def key_to_enum_name(key: str) -> str:
    """
    Convert a dotted key like "vampire.spiritgate.blood_cost"
    to an enum name like "VAMPIRE_SPIRITGATE_BLOOD_COST"
    """
    # Replace dots and hyphens with underscores
    name = key.replace(".", "_").replace("-", "_")
    # Convert to uppercase
    name = name.upper()
    return name


def parse_acfg_table(source_file: Path) -> list[tuple[str, str, str]]:
    """
    Parse acfg entries from acfg_keys.h X-macro format.

    Returns list of (key, value, default_value) tuples.
    """
    content = source_file.read_text(encoding="utf-8")

    # First try the new X-macro format in acfg_keys.h:
    # ACFG_X(ENUM_NAME, "key", default)
    xmacro_pattern = r'X\(\s*\w+\s*,\s*"([^"]+)"\s*,\s*(-?\d+)\s*\)'
    entries = []
    for match in re.finditer(xmacro_pattern, content):
        key = match.group(1)
        default = match.group(2)
        entries.append((key, default, default))

    if entries:
        return entries

    # Fallback: try old format { "key", value, default }
    old_pattern = r'\{\s*"([^"]+)"\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}'
    for match in re.finditer(old_pattern, content):
        key = match.group(1)
        value = match.group(2)
        default = match.group(3)
        entries.append((key, value, default))

    return entries


def extract_comments(source_file: Path) -> dict[str, str]:
    """
    Extract section comments from ability_config.c to preserve organization.
    Returns a dict mapping the first key in each section to the comment.
    """
    content = source_file.read_text(encoding="utf-8")

    comments = {}
    current_comment = None

    # Pattern for section headers like /* VAMPIRE (vamp.c) */
    section_pattern = r'/\*\s*={10,}.*?\n\s*\*\s*(\w+.*?)\n\s*\*\s*={10,}\s*\*/'

    # Find all section comments and the first key after each
    for match in re.finditer(section_pattern, content, re.DOTALL):
        section_name = match.group(1).strip()
        # Find the position of this comment
        comment_end = match.end()

        # Find the first key after this comment
        key_match = re.search(r'\{\s*"([^"]+)"', content[comment_end:])
        if key_match:
            first_key = key_match.group(1)
            comments[first_key] = section_name

    return comments


def generate_xmacro_header(entries: list[tuple[str, str, str]],
                           section_comments: dict[str, str]) -> str:
    """
    Generate the acfg_keys.h X-macro header content.
    """
    lines = []

    # Header
    lines.append("""/***************************************************************************
 *  acfg_keys.h - Type-safe ability config keys (auto-generated)
 *
 *  This file is the SINGLE SOURCE OF TRUTH for all ability config entries.
 *  The enum and table are both generated from the ACFG_ENTRIES macro.
 *
 *  DO NOT EDIT MANUALLY - regenerate using:
 *      python game/tools/generate_acfg_keys.py
 ***************************************************************************/

#ifndef ACFG_KEYS_H
#define ACFG_KEYS_H

/*
 * X-Macro definitions for ability config keys.
 * Format: ACFG_X(ENUM_NAME, "string.key", default_value)
 */
#define ACFG_ENTRIES \\""")

    current_section = None

    for key, value, default in entries:
        # Check if this key starts a new section
        if key in section_comments:
            section = section_comments[key]
            if current_section != section:
                lines.append(f"    /* {section} */ \\")
                current_section = section

        enum_name = key_to_enum_name(key)
        # Use default value (third field) as the value in the macro
        lines.append(f'    ACFG_X({enum_name:50s}, "{key}", {default:>10}) \\')

    # Remove trailing backslash from last entry
    if lines:
        lines[-1] = lines[-1].rstrip(" \\")

    lines.append("")
    lines.append("")

    # Generate enum
    lines.append("""/* Generate enum from X-macro */
typedef enum {
#define ACFG_X(name, key, def) ACFG_##name,
    ACFG_ENTRIES
#undef ACFG_X
    ACFG_COUNT  /* Total count, also serves as invalid sentinel */
} acfg_key_t;

#endif /* ACFG_KEYS_H */
""")

    return "\n".join(lines)


def generate_key_to_enum_mapping(entries: list[tuple[str, str, str]]) -> dict[str, str]:
    """
    Generate a mapping from string key to enum name for the migration script.
    """
    mapping = {}
    for key, _, _ in entries:
        enum_name = f"ACFG_{key_to_enum_name(key)}"
        mapping[key] = enum_name
    return mapping


def generate_python_module(entries: list[tuple[str, str, str]],
                           section_comments: dict[str, str]) -> str:
    """
    Generate a Python module with all valid acfg keys for MudEdit validation.
    """
    lines = []

    lines.append('"""')
    lines.append("acfg_keys.py - Valid ability config keys (auto-generated)")
    lines.append("")
    lines.append("This module provides validation and autocomplete for acfg keys in MudEdit.")
    lines.append("DO NOT EDIT MANUALLY - regenerate using:")
    lines.append("    python game/tools/generate_acfg_keys.py")
    lines.append('"""')
    lines.append("")
    lines.append("from typing import Set, List, Dict, Optional")
    lines.append("")
    lines.append("")

    # Generate ALL_KEYS set
    lines.append("# All valid acfg keys")
    lines.append("ALL_KEYS: Set[str] = {")
    for key, _, default in entries:
        lines.append(f'    "{key}",')
    lines.append("}")
    lines.append("")
    lines.append("")

    # Generate KEYS_BY_CLASS dict
    lines.append("# Keys organized by class prefix")
    lines.append("KEYS_BY_CLASS: Dict[str, List[str]] = {")

    # Group entries by class
    current_class = None
    class_keys: list[str] = []

    for key, _, _ in entries:
        cls = key.split(".")[0]
        if cls != current_class:
            if current_class is not None:
                lines.append(f'    "{current_class}": [')
                for k in class_keys:
                    lines.append(f'        "{k}",')
                lines.append("    ],")
            current_class = cls
            class_keys = []
        class_keys.append(key)

    # Don't forget the last class
    if current_class is not None:
        lines.append(f'    "{current_class}": [')
        for k in class_keys:
            lines.append(f'        "{k}",')
        lines.append("    ],")

    lines.append("}")
    lines.append("")
    lines.append("")

    # Generate KEY_DEFAULTS dict
    lines.append("# Default values for each key")
    lines.append("KEY_DEFAULTS: Dict[str, int] = {")
    for key, _, default in entries:
        lines.append(f'    "{key}": {default},')
    lines.append("}")
    lines.append("")
    lines.append("")

    # Generate helper functions
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
    lines.append("def get_classes() -> List[str]:")
    lines.append('    """Get list of all class prefixes."""')
    lines.append("    return list(KEYS_BY_CLASS.keys())")
    lines.append("")
    lines.append("")
    lines.append("def get_keys_for_class(class_name: str) -> List[str]:")
    lines.append('    """Get all keys for a specific class."""')
    lines.append("    return KEYS_BY_CLASS.get(class_name, [])")
    lines.append("")
    lines.append("")
    lines.append("def search_keys(prefix: str) -> List[str]:")
    lines.append('    """Search for keys matching a prefix (case-insensitive)."""')
    lines.append("    prefix_lower = prefix.lower()")
    lines.append("    return [k for k in ALL_KEYS if k.lower().startswith(prefix_lower)]")
    lines.append("")

    return "\n".join(lines)


def main():
    """Main entry point for acfg keys generation."""
    # Parse from acfg_keys.h if it has entries, else ability_config.c
    entries = []
    source_file = ABILITY_CONFIG_C

    if ACFG_KEYS_H.exists():
        entries = parse_acfg_table(ACFG_KEYS_H)
        if entries:
            source_file = ACFG_KEYS_H

    if not entries:
        entries = parse_acfg_table(ABILITY_CONFIG_C)
        source_file = ABILITY_CONFIG_C

    print(f"Parsing {source_file}...")
    print(f"  Found {len(entries)} entries")

    print("Extracting section comments...")
    section_comments = extract_comments(ABILITY_CONFIG_C)
    print(f"  Found {len(section_comments)} sections")

    print(f"Generating {ACFG_KEYS_H}...")
    header_content = generate_xmacro_header(entries, section_comments)
    ACFG_KEYS_H.write_text(header_content, encoding="utf-8")
    print(f"  Written {len(header_content)} bytes")

    print(f"Generating {KEY_TO_ENUM_JSON}...")
    mapping = generate_key_to_enum_mapping(entries)
    KEY_TO_ENUM_JSON.write_text(json.dumps(mapping, indent=2), encoding="utf-8")
    print(f"  Written {len(mapping)} mappings")

    print(f"Generating {ACFG_KEYS_PY}...")
    python_content = generate_python_module(entries, section_comments)
    ACFG_KEYS_PY.write_text(python_content, encoding="utf-8")
    print(f"  Written {len(python_content)} bytes")

    print("\nDone! Generated files:")
    print(f"  - {ACFG_KEYS_H} (C header with X-macros)")
    print(f"  - {KEY_TO_ENUM_JSON} (migration mapping)")
    print(f"  - {ACFG_KEYS_PY} (Python validation module for MudEdit)")


if __name__ == "__main__":
    main()
