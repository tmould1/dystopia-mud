#!/usr/bin/env python3
"""
Unified MUD Database Editor for Dystopia MUD.

A Tkinter-based GUI editor for all MUD SQLite database files:
- Help files (gamedata/db/game/*.db)
- Area files (gamedata/db/areas/*.db)
- Game configuration (gamedata/db/game/game.db)
- Class configuration (gamedata/db/game/class.db)
- Player files (gamedata/db/players/*.db)

Usage:
    python mudedit_gui.py
"""

import sys
import tkinter as tk
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).resolve().parent))

from mudedit.app import MudEditorApp


def main():
    """Main entry point."""
    root = tk.Tk()

    # Try to set a better window icon (optional)
    try:
        # On Windows, you could set an icon here
        pass
    except Exception:
        pass

    app = MudEditorApp(root)
    app.run()


if __name__ == '__main__':
    main()
