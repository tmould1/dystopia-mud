#!/usr/bin/env python3
"""
add_truecolor_help.py - Add help entries for true color and color codes

Usage:
    python add_truecolor_help.py           # Add all help entries
    python add_truecolor_help.py --dry-run # Preview without changes
"""

import sqlite3
import argparse
from pathlib import Path

COLOR_HELP = """              #1-=#6[#n C O L O R   C O D E S #6]#1=-#n

Dystopia supports three levels of color output. Use #Rconfig#n to see
your current mode, or choose during character creation.

#7Standard 16-color (ANSI):#n
  Use ##  followed by a color code letter or digit:

  #7Code  Color          Code  Color          Code  Color#n
  ##0   #0Bright Black#n    ##1   #1Bright Red#n      ##2   #2Bright Green#n
  ##3   #3Bright Yellow#n   ##4   #4Bright Blue#n     ##5   #5Bright Purple#n
  ##6   #6Bright Cyan#n     ##7   #7White#n            ##8   #8Black#n
  ##9   #9Bright White#n    ##r   #rDark Red#n        ##g   #gDark Green#n
  ##o   #oDark Yellow#n     ##l   #lDark Blue#n       ##p   #pDark Purple#n
  ##c   #cDark Cyan#n       ##y   #yBright Yellow#n

  #7Uppercase Aliases:#n
  ##R   #RBright Red#n      ##G   #GBright Green#n    ##L   #LBright Blue#n
  ##P   #PBright Purple#n   ##C   #CBright Cyan#n

  #7Special Codes:#n
  ##n   #nReset all#n       ##i   #iInverse#n         ##u   #uUnderline#n
  ##s   Random color

#7Xterm 256-color:#n  (requires #Rconfig +xterm#n)
  ##x followed by a three-digit color index (000-255):
  ##x208 = #x208Orange#n    ##x033 = #x033Teal#n    ##x171 = #x171Purple#n

#7True color 24-bit RGB:#n  (requires #Rconfig +truecolor#n)
  ##t followed by 6 hex digits (RRGGBB) for foreground:
  ##tFF6600 = #tFF6600Sunset Orange#n   ##t00CC99 = #t00CC99Seafoam#n
  ##T followed by 6 hex digits (RRGGBB) for background:
  ##T003366 = #T003366#tFFFFFF White on Blue #n

#7Background colors:#n
  ##T + RRGGBB  true color background   ##X + NNN  256-color background
  ##b           reset background only    ##n        reset all (fg + bg)

#7Escaping:#n  #### prints a literal ##, ##- prints ~, ##+ prints %%

See also: TRUECOLOR, COLORTEST, CONFIG"""

TRUECOLOR_HELP = """            #1-=#6[#n T R U E   C O L O R #6]#1=-#n

True color mode enables 24-bit RGB color output, providing over 16 million
colors for rich, smooth gradients and precise color control.

#7Enabling:#n
  config +truecolor     Toggle true color on (also enables ANSI and xterm)
  config -truecolor     Toggle true color off
  truecolor             Quick toggle

#7Token Format:#n
  ##tRRGGBB             Foreground color (6 hex digits for RGB)
  ##TRRGGBB             Background color (6 hex digits for RGB)

  RR, GG, BB are two-digit hexadecimal values (00-FF) for red, green, blue.

#7Examples:#n
  ##tFF0000 = #tFF0000Red#n  ##t00FF00 = #t00FF00Green#n  ##t0000FF = #t0000FFBlue#n
  ##tFF6600 = #tFF6600Sunset Orange#n  ##t00CC99 = #t00CC99Seafoam#n
  ##tFF00FF = #tFF00FFMagenta#n  ##t8844FF = #t8844FFViolet#n

#7Background Example:#n
  ##T003366##tFFFFFF = #T003366#tFFFFFF White text on blue background #n

#7Graceful Degradation:#n
  If your client doesn't support true color, the server automatically maps
  true color codes to the closest available color:
  - #7True color#n -> 256-color -> 16-color -> stripped (plain text)
  This means builders can use true color codes and they'll look reasonable
  on all clients.

#7Supported Clients:#n
  Mudlet 4.11+, TinTin++ 2.02+, Blightmud, and most modern terminal
  emulators. The server auto-detects capability via MTTS if supported.

#7Testing:#n
  Use the #Rcolortest#n command to see all color modes in action.

See also: COLOR, COLORTEST, CONFIG"""

COLORTEST_HELP = """Syntax: colortest

Displays test swatches for all three color modes (16-color, 256-color,
and true color) so you can verify your client's support level. Also
shows your current color mode settings and MTTS detection results.

See also: COLOR, TRUECOLOR, CONFIG"""


def add_help_entry(cursor, keyword, text, dry_run=False):
    """Add or update a help entry."""
    print(f"  Adding help: {keyword}")
    if not dry_run:
        cursor.execute("DELETE FROM helps WHERE keyword = ?", (keyword,))
        cursor.execute("INSERT INTO helps (keyword, text) VALUES (?, ?)", (keyword, text))


def main():
    parser = argparse.ArgumentParser(description="Add true color help entries")
    parser.add_argument("--dry-run", action="store_true", help="Preview without making changes")
    args = parser.parse_args()

    script_dir = Path(__file__).parent
    db_path = script_dir.parent.parent / "gamedata" / "db" / "game" / "base_help.db"

    print(f"Using database: {db_path}")
    if args.dry_run:
        print("[DRY RUN MODE]")

    conn = sqlite3.connect(str(db_path))
    cursor = conn.cursor()

    print("\nAdding help entries:")
    add_help_entry(cursor, "COLOR COLORS COLOUR COLOURS", COLOR_HELP, args.dry_run)
    add_help_entry(cursor, "TRUECOLOR TRUECOLOUR TRUE COLOR", TRUECOLOR_HELP, args.dry_run)
    add_help_entry(cursor, "COLORTEST", COLORTEST_HELP, args.dry_run)

    if not args.dry_run:
        conn.commit()
        print("\nDone! All help entries added.")
    else:
        print("\n[DRY RUN] No changes made.")

    conn.close()


if __name__ == "__main__":
    main()
