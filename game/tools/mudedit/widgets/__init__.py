"""Reusable Tkinter widgets for MUD editor."""

from .colors import TK_COLORS, DEFAULT_FG, PREVIEW_BG, EDITOR_BG, EDITOR_FG
from .colors import xterm256_to_hex, parse_colored_segments, strip_colors
from .color_text import ColorTextEditor
from .list_panel import EntityListPanel
from .flags import FlagEditor, CompactFlagDisplay
from .dice import DiceEditor, DiceDisplay

__all__ = [
    # Colors
    'TK_COLORS',
    'DEFAULT_FG',
    'PREVIEW_BG',
    'EDITOR_BG',
    'EDITOR_FG',
    'xterm256_to_hex',
    'parse_colored_segments',
    'strip_colors',
    # Widgets
    'ColorTextEditor',
    'EntityListPanel',
    'FlagEditor',
    'CompactFlagDisplay',
    'DiceEditor',
    'DiceDisplay',
]
