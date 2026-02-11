"""
cfg_keys.py - Valid config keys (auto-generated)

This module provides validation and autocomplete for cfg keys in MudEdit.
DO NOT EDIT MANUALLY - regenerate using:
    python game/tools/generate_cfg_keys.py
"""

from typing import Set, List, Dict, Optional


# All valid cfg keys
ALL_KEYS: Set[str] = {
}


# Keys organized by category
KEYS_BY_CATEGORY: Dict[str, List[str]] = {
}


# Default values for each key
KEY_DEFAULTS: Dict[str, int] = {
}


def is_valid_key(key: str) -> bool:
    """Check if a key is valid."""
    return key in ALL_KEYS


def get_default(key: str) -> Optional[int]:
    """Get the default value for a key, or None if invalid."""
    return KEY_DEFAULTS.get(key)


def get_categories() -> List[str]:
    """Get list of all category names."""
    return list(KEYS_BY_CATEGORY.keys())


def get_keys_for_category(category: str) -> List[str]:
    """Get all keys for a specific category."""
    return KEYS_BY_CATEGORY.get(category, [])


def search_keys(prefix: str) -> List[str]:
    """Search for keys matching a prefix (case-insensitive)."""
    prefix_lower = prefix.lower()
    return [k for k in ALL_KEYS if k.lower().startswith(prefix_lower)]
