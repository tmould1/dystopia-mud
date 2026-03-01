"""
Game constants parsed directly from C header files at import time.

Parses merc.h, class.h, and db_class.h to extract #define constants and enums,
ensuring Python always has the same values as the compiled game. No manual
synchronization or regeneration step needed.
"""

import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# =============================================================================
# C Header Parsing Utilities
# =============================================================================

def _find_repo_root() -> Path:
    """Find the repository root relative to this file."""
    # This file is at game/tools/mudlib/merc_constants.py
    return Path(__file__).resolve().parent.parent.parent.parent


def _read_header(path: Path) -> str:
    """Read a C header file, returning empty string if not found."""
    try:
        return path.read_text(encoding='utf-8', errors='replace')
    except (FileNotFoundError, OSError):
        return ''



def _parse_numeric_defines(text: str, prefix: str,
                           skip_substrings: Optional[List[str]] = None,
                           include_substrings: Optional[List[str]] = None
                           ) -> Dict[int, str]:
    """
    Parse #define PREFIX_NAME <number> patterns into {value: 'name'} dict.

    Args:
        text: Header file contents
        prefix: Define prefix to match (e.g., 'ROOM_', 'EX_')
        skip_substrings: Skip defines whose name (after prefix) contains these
                         (e.g., ['VNUM'] to skip ROOM_VNUM_*)
        include_substrings: If set, only include defines whose name contains
                            at least one of these substrings

    Returns:
        Dict mapping numeric value to lowercase flag name.
    """
    skip = skip_substrings or []
    result = {}
    pattern = rf'^#define\s+{re.escape(prefix)}(\w+)\s+(-?\d+)'
    for m in re.finditer(pattern, text, re.MULTILINE):
        name = m.group(1)
        value = int(m.group(2))
        if any(s in name for s in skip):
            continue
        if include_substrings and not any(s in name for s in include_substrings):
            continue
        # First occurrence wins — handles overlapping define sets
        # (e.g., ITEM_LIGHT=1 vs ITEM_GLOW=1, AFF_BLIND=1 vs AFF_CONTRACEPTION=1)
        if value not in result:
            result[value] = name.lower()
    return result


def _parse_bitshift_defines(text: str, prefix: str) -> Dict[int, str]:
    """
    Parse #define PREFIX_NAME (1 << N) patterns into {value: 'name'} dict.

    Args:
        text: Header file contents
        prefix: Define prefix to match (e.g., 'ACT_', 'WEAPON_')

    Returns:
        Dict mapping numeric value to lowercase flag name.
    """
    result = {}
    pattern = rf'^#define\s+{re.escape(prefix)}(\w+)\s+\(\s*1\s*<<\s*(\d+)\s*\)'
    for m in re.finditer(pattern, text, re.MULTILINE):
        name = m.group(1)
        value = 1 << int(m.group(2))
        if value not in result:
            result[value] = name.lower()
    return result


def _parse_enum_values(text: str, enum_name: str) -> Dict[int, str]:
    """
    Parse a C typedef enum into {value: 'name'} dict.

    Handles sequential enums with optional = value assignments.
    Matches: typedef enum { ... } ENUM_NAME;
    """
    pattern = rf'typedef\s+enum\s*\{{([^}}]+)\}}\s*{re.escape(enum_name)}\s*;'
    match = re.search(pattern, text, re.DOTALL)
    if not match:
        return {}

    result = {}
    counter = 0
    for line in match.group(1).split('\n'):
        line = re.sub(r'/\*.*?\*/', '', line).strip().rstrip(',')
        if not line or line.startswith('//'):
            continue
        eq_match = re.match(r'(\w+)\s*=\s*(-?\d+)', line)
        if eq_match:
            name = eq_match.group(1)
            counter = int(eq_match.group(2))
            result[counter] = name.lower()
            counter += 1
        else:
            name_match = re.match(r'(\w+)', line)
            if name_match:
                name = name_match.group(1)
                result[counter] = name.lower()
                counter += 1

    return result


def _to_display_name(raw_name: str, prefix: str = '',
                     overrides: Optional[Dict[str, str]] = None) -> str:
    """
    Convert a C define name to a human-readable display name.

    Examples:
        'DRAGONKIN' -> 'Dragonkin'
        'UNDEAD_KNIGHT' -> 'Undead Knight'
        'DROID' -> 'Spider Droid' (via override)
    """
    if overrides and raw_name in overrides:
        return overrides[raw_name]
    # Strip prefix, replace underscores, title case
    name = raw_name
    if prefix and name.startswith(prefix):
        name = name[len(prefix):]
    return name.replace('_', ' ').title()


# =============================================================================
# Parse Headers
# =============================================================================

_root = _find_repo_root()
_core_dir = _root / 'game' / 'src' / 'core'
_class_path = _root / 'game' / 'src' / 'classes' / 'class.h'
_db_class_path = _root / 'game' / 'src' / 'db' / 'db_class.h'

# Read all core headers — merc.h was decomposed into sub-headers (Phase 3),
# so we concatenate them all for regex parsing of #define constants.
_merc_text = '\n'.join(
    _read_header(_core_dir / h)
    for h in ['merc.h', 'types.h', 'network.h', 'char.h',
              'object.h', 'room.h', 'world.h', 'combat.h']
)
_class_text = _read_header(_class_path)
_db_class_text = _read_header(_db_class_path)


# --- Direction constants ---
DIR_NORTH = 0
DIR_EAST = 1
DIR_SOUTH = 2
DIR_WEST = 3
DIR_UP = 4
DIR_DOWN = 5
DIR_NAMES = ['north', 'east', 'south', 'west', 'up', 'down']
REVERSE_DIR = [2, 3, 0, 1, 5, 4]
DIR_OFFSETS = {
    DIR_NORTH: (0, 1, 0),
    DIR_EAST: (1, 0, 0),
    DIR_SOUTH: (0, -1, 0),
    DIR_WEST: (-1, 0, 0),
    DIR_UP: (0, 0, 1),
    DIR_DOWN: (0, 0, -1),
}

# --- Room flags (bitfield) ---
ROOM_FLAGS = _parse_numeric_defines(_merc_text, 'ROOM_', skip_substrings=['VNUM'])

# --- Exit flags (bitfield) ---
EXIT_FLAGS = _parse_numeric_defines(_merc_text, 'EX_')
# EX_ISDOOR is conventionally displayed as just 'door'
if 1 in EXIT_FLAGS and EXIT_FLAGS[1] == 'isdoor':
    EXIT_FLAGS[1] = 'door'

# --- Sex types ---
SEX_TYPES_RAW = _parse_numeric_defines(_merc_text, 'SEX_')
SEX_TYPES = {v: _to_display_name(n) for v, n in SEX_TYPES_RAW.items()}

# --- Sector types ---
SECTOR_NAMES = _parse_numeric_defines(_merc_text, 'SECT_',
                                       skip_substrings=['MAX', 'UNUSED'])

# --- Item types (ITEM_LIGHT..ITEM_INSTRUMENT, not extra/wear flags) ---
_ITEM_NON_TYPES = [
    # Extra flags (ITEM_GLOW, ITEM_HUM, etc.)
    'GLOW', 'HUM', 'THROWN', 'KEEP', 'VANISH', 'INVIS', 'MAGIC',
    'NODROP', 'BLESS', 'ANTI_', 'NOREMOVE', 'INVENTORY', 'LOYAL',
    'SHADOWPLANE', 'MENCHANT',
    # Wear flags (ITEM_WEAR_*, ITEM_WIELD, ITEM_HOLD)
    # Note: ITEM_TAKE=1 is handled by first-occurrence (ITEM_LIGHT=1 wins)
    'WEAR_', 'WIELD', 'HOLD',
    # Misc non-type flags
    'EQUEST',
]
ITEM_TYPES = _parse_numeric_defines(_merc_text, 'ITEM_', skip_substrings=_ITEM_NON_TYPES)

# --- Item extra flags (ITEM_GLOW, ITEM_HUM, etc. — stored in obj->extra_flags) ---
ITEM_EXTRA_FLAGS = _parse_numeric_defines(
    _merc_text, 'ITEM_',
    include_substrings=[
        'GLOW', 'HUM', 'THROWN', 'KEEP', 'VANISH', 'INVIS', 'MAGIC',
        'NODROP', 'BLESS', 'ANTI_', 'NOREMOVE', 'INVENTORY', 'LOYAL',
        'SHADOWPLANE', 'MENCHANT',
    ]
)

# --- Item wear flags (ITEM_TAKE, ITEM_WEAR_*, etc. — stored in obj->wear_flags) ---
ITEM_WEAR_FLAGS = _parse_numeric_defines(
    _merc_text, 'ITEM_',
    skip_substrings=['STAKE'],  # Prevent ITEM_STAKE matching 'TAKE' include
    include_substrings=['TAKE', 'WEAR_', 'WIELD', 'HOLD'],
)

# --- ACT flags (bit-shift based) ---
ACT_FLAGS = _parse_bitshift_defines(_merc_text, 'ACT_')

# --- AFF flags (numeric) ---
# merc.h has two sets of AFF_ defines - we want the numeric ones (AFF_BLIND=1, etc.)
# not the letter-macro extras (AFF_TOTALBLIND, AFF_CLAW, etc.)
AFF_FLAGS = _parse_numeric_defines(_merc_text, 'AFF_')

# --- Player flags (numeric) ---
# Skip PLR_IMPLAG (stray define near "Affected_by 2" section, not a real player flag)
PLR_FLAGS_RAW = _parse_numeric_defines(_merc_text, 'PLR_', skip_substrings=['IMPLAG'])

# --- Extra flags (numeric) ---
# Skip EXTRA_BLINKY (stray define near "Affected_by 2" section, not a real extra flag)
EXTRA_FLAGS_RAW = _parse_numeric_defines(_merc_text, 'EXTRA_', skip_substrings=['BLINKY'])

# --- Newbits flags (numeric) ---
NEWBITS_FLAGS_RAW = _parse_numeric_defines(_merc_text, 'NEW_')

# --- Immunity flags (numeric) ---
IMM_FLAGS_RAW = _parse_numeric_defines(_merc_text, 'IMM_')

# --- Apply types ---
APPLY_TYPES_RAW = _parse_numeric_defines(_merc_text, 'APPLY_')

# --- Wear locations ---
WEAR_LOCATIONS_RAW = _parse_numeric_defines(_merc_text, 'WEAR_')

# --- Weapon flags (bit-shift based) ---
WEAPON_TYPES_RAW = _parse_bitshift_defines(_merc_text, 'WEAPON_')

# --- Discipline indices ---
DISC_DEFINES = _parse_numeric_defines(_merc_text, 'DISC_')

# --- Stance indices ---
STANCE_DEFINES = _parse_numeric_defines(_merc_text, 'STANCE_',
                                         skip_substrings=['NONE', 'NORMAL'])

# --- Class defines from class.h ---
_CLASS_NAME_OVERRIDES = {
    'DROID': 'Spider Droid',
    'UNDEAD': 'Undead Knight',
    'WW': 'Werewolf',
    'SPIRITLORD': 'Spirit Lord',
}
CLASS_DEFINES = _parse_numeric_defines(_class_text, 'CLASS_')

# Build CLASS_TABLE with human-readable names
CLASS_TABLE = {}
for class_id, raw_name in CLASS_DEFINES.items():
    CLASS_TABLE[class_id] = _to_display_name(
        raw_name.upper(), overrides=_CLASS_NAME_OVERRIDES
    )

# --- STAT_SOURCE enum from db_class.h ---
STAT_SOURCE_ENUM = _parse_enum_values(_db_class_text, 'STAT_SOURCE')
# Remove STAT_MAX sentinel
STAT_SOURCE_ENUM.pop(max(STAT_SOURCE_ENUM.keys(), default=-1), None)
