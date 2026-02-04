"""
Color code utilities for MUD text rendering.

Provides parsing and rendering of MUD color codes for Tkinter Text widgets.
Supports standard #X codes and xterm 256-color #xNNN codes.
"""

import re
from typing import List, Tuple

# MUD color code -> hex color mapping for tkinter
TK_COLORS = {
    '0': '#808080',   # Bright Black (gray)
    '1': '#ff5555',   # Bright Red
    '2': '#55ff55',   # Bright Green
    '3': '#ffff55',   # Bright Yellow
    '4': '#5555ff',   # Bright Blue
    '5': '#ff55ff',   # Bright Purple
    '6': '#55ffff',   # Bright Cyan
    '7': '#c0c0c0',   # White
    '8': '#404040',   # Black
    '9': '#ffffff',   # Bright White
    'r': '#aa0000',   # Dark Red
    'g': '#00aa00',   # Dark Green
    'o': '#aa5500',   # Brown
    'l': '#0000aa',   # Dark Blue
    'p': '#aa00aa',   # Dark Purple
    'c': '#00aaaa',   # Dark Cyan
    'y': '#ffff55',   # Bright Yellow
    'R': '#ff5555',   # Bright Red
    'G': '#55ff55',   # Bright Green
    'L': '#5555ff',   # Bright Blue
    'P': '#ff55ff',   # Bright Purple
    'C': '#55ffff',   # Bright Cyan
    'n': '#c0c0c0',   # Reset (default text)
    'i': None,        # Inverse (handled specially)
    'u': None,        # Underline (handled specially)
}

# Theme colors
DEFAULT_FG = '#c0c0c0'
PREVIEW_BG = '#1a1a1a'
EDITOR_BG = '#2d2d2d'
EDITOR_FG = '#d4d4d4'


def xterm256_to_hex(n: int) -> str:
    """Convert xterm 256-color index to hex color."""
    if n < 16:
        # Standard colors
        basic = [
            '#000000', '#aa0000', '#00aa00', '#aa5500',
            '#0000aa', '#aa00aa', '#00aaaa', '#c0c0c0',
            '#808080', '#ff5555', '#55ff55', '#ffff55',
            '#5555ff', '#ff55ff', '#55ffff', '#ffffff',
        ]
        return basic[n]
    elif n < 232:
        # 6x6x6 color cube
        n -= 16
        b = n % 6
        g = (n // 6) % 6
        r = n // 36
        return f'#{r*51:02x}{g*51:02x}{b*51:02x}'
    else:
        # Grayscale
        v = 8 + (n - 232) * 10
        return f'#{v:02x}{v:02x}{v:02x}'


def parse_colored_segments(text: str) -> List[Tuple[str, str]]:
    """
    Parse MUD color-coded text into list of (plain_text, color_tag) tuples.

    Supports:
    - Standard codes: #0-#9, #r-#c, #R-#C, #n (reset), #i (inverse), #u (underline)
    - 256-color codes: #xNNN (3-digit color index)
    - Escape sequences: ## -> #, #- -> ~, #+ -> %
    """
    segments = []
    current_tag = 'color_n'
    buf = []
    i = 0

    while i < len(text):
        if text[i] == '#' and i + 1 < len(text):
            nxt = text[i + 1]
            # Flush buffer
            if buf:
                segments.append((''.join(buf), current_tag))
                buf = []

            if nxt == '#':
                buf.append('#')
                i += 2
            elif nxt == '-':
                buf.append('~')
                i += 2
            elif nxt == '+':
                buf.append('%')
                i += 2
            elif nxt == 'x' and i + 4 < len(text) and text[i+2:i+5].isdigit():
                code = int(text[i+2:i+5])
                current_tag = f'color_x{code}'
                i += 5
            elif nxt in TK_COLORS:
                current_tag = f'color_{nxt}'
                i += 2
            else:
                buf.append(text[i])
                i += 1
        else:
            buf.append(text[i])
            i += 1

    if buf:
        segments.append((''.join(buf), current_tag))

    return segments


def strip_colors(text: str) -> str:
    """Remove all MUD color codes from text, returning plain text."""
    # Handle escape sequences first
    result = text.replace('##', '\x00HASH\x00')
    result = result.replace('#-', '~')
    result = result.replace('#+', '%')

    # Remove #xNNN codes
    result = re.sub(r'#x\d{3}', '', result)

    # Remove standard #X codes
    for code in TK_COLORS:
        result = result.replace(f'#{code}', '')

    # Restore escaped hashes
    result = result.replace('\x00HASH\x00', '#')

    return result
