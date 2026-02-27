#!/usr/bin/env python3
"""
Seed the intro help entries for the tiered connection experience.

Adds 4 help entries to base_help.db:
  intro_rich          - Truecolor/256-color, wide terminal (100+ cols)
  intro_standard      - ANSI 16-color, 80-column
  intro_basic         - Plain text, no color codes
  intro_screenreader  - Accessible prose for screen readers

Each entry includes the required Diku/Merc/Godwars/Dystopia credits.
"""

import sqlite3
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
DB_PATH = REPO_ROOT / 'gamedata' / 'db' / 'game' / 'base_help.db'

# ---------------------------------------------------------------------------
# Intro content for each tier
# ---------------------------------------------------------------------------

# Rich tier: truecolor gradients, wide terminal (100+ cols)
# Uses #tRRGGBB for truecolor foreground
INTRO_RICH = """.
#n

#t606060       The darkness lifts, if only for a moment.#n
#t505050                                                                                                 #n
#t404040             A wind carries the scent of ash and iron across a land of ruin.#n
#t484848        Beyond the treeline, crows circle the broken towers of a fallen kingdom.#n
#t404040                        The earth remembers what men have forgotten.#n

#n
#t6B0000                           ____            _              _       #n
#t8B0000                          |  _ \\ _   _ ___| |_ ___  _ __ (_) __ _ #n
#tA00000                          | | | | | | / __| __/ _ \\| '_ \\| |/ _` |#n
#tC00000                          | |_| | |_| \\__ \\ || (_) | |_) | | (_| |#n
#tA00000                          |____/ \\__, |___/\\__\\___/| .__/|_|\\__,_|#n
#t8B0000                                 |___/             |_|            #n
#n
#t606060                An old name, carved deep in weathered stone.#n
#t505050                                                                                                 #n
#t404040         The realm is shattered. The strong endure. The rest are forgotten.#n
#n
#t303030 ============================================================================================#n
#t383838  Diku: Sebastian Hammer, Michael Seifert, Hans Henrik Staerfeldt, Tom Madsen, Katja Nyboe#n
#t303030  Merc: Furey, Hatchet, Kahn#n
#t383838  Godwars: KaVir#n
#t303030  Dystopia: Vladd, Tarasque, Dracknuur, Jobo#n
#t303030 ============================================================================================#n
#n
#t808080 What is your name, stranger? #n"""

# Standard tier: ANSI 16-color, 80-column layout
INTRO_STANDARD = """.
#n

#0  The darkness lifts, if only for a moment.#n

#0  A wind carries the scent of ash and iron across a land of ruin.#n
#0  Crows circle the broken towers of a fallen kingdom.#n
#0  The earth remembers what men have forgotten.#n

#r   ____            _              _       #n
#r  |  _ \\ _   _ ___| |_ ___  _ __ (_) __ _ #n
#R  | | | | | | / __| __/ _ \\| '_ \\| |/ _` |#n
#R  | |_| | |_| \\__ \\ || (_) | |_) | | (_| |#n
#R  |____/ \\__, |___/\\__\\___/| .__/|_|\\__,_|#n
#r         |___/             |_|            #n

#0  An old name, carved deep in weathered stone.#n

#0  The realm is shattered. The strong endure.#n
#0  The rest are forgotten.#n

#0 ==========================================================================#n
#0  Diku: Sebastian Hammer, Michael Seifert, Hans Henrik Staerfeldt,#n
#0        Tom Madsen, Katja Nyboe#n
#0  Merc: Furey, Hatchet, Kahn#n
#0  Godwars: KaVir#n
#0  Dystopia: Vladd, Tarasque, Dracknuur, Jobo#n
#0 ==========================================================================#n

 What is your name, stranger? """

# Basic tier: no color codes at all, plain text
INTRO_BASIC = """.

  The darkness lifts, if only for a moment.

  A wind carries the scent of ash and iron across a land of ruin.
  Crows circle the broken towers of a fallen kingdom.
  The earth remembers what men have forgotten.

                    D Y S T O P I A

  An old name, carved deep in weathered stone.

  The realm is shattered. The strong endure.
  The rest are forgotten.

 ==========================================================================
  Diku: Sebastian Hammer, Michael Seifert, Hans Henrik Staerfeldt,
        Tom Madsen, Katja Nyboe
  Merc: Furey, Hatchet, Kahn
  Godwars: KaVir
  Dystopia: Vladd, Tarasque, Dracknuur, Jobo
 ==========================================================================

 What is your name, stranger? """

# Screen reader tier: no art, no formatting, accessible prose
INTRO_SCREENREADER = """.

Welcome to Dystopia.

A wind carries the scent of ash and iron.
Crows circle the broken towers of a fallen kingdom.
The realm is shattered. The strong endure.

Based on DikuMUD by Sebastian Hammer, Michael Seifert,
Hans Henrik Staerfeldt, Tom Madsen, and Katja Nyboe.
Merc by Furey, Hatchet, and Kahn.
God Wars by KaVir.
Dystopia by Vladd, Tarasque, Dracknuur, and Jobo.

What is your name? """

# ---------------------------------------------------------------------------
# Insert into database
# ---------------------------------------------------------------------------

ENTRIES = [
    ('INTRO_RICH',         -1, INTRO_RICH),
    ('INTRO_STANDARD',     -1, INTRO_STANDARD),
    ('INTRO_BASIC',        -1, INTRO_BASIC),
    ('INTRO_SCREENREADER', -1, INTRO_SCREENREADER),
]

def main():
    if not DB_PATH.exists():
        print(f"Error: database not found at {DB_PATH}", file=sys.stderr)
        sys.exit(1)

    conn = sqlite3.connect(str(DB_PATH))
    cursor = conn.cursor()

    for keyword, level, text in ENTRIES:
        # Check if entry already exists
        row = cursor.execute(
            "SELECT id FROM helps WHERE keyword = ?", (keyword,)
        ).fetchone()

        if row:
            cursor.execute(
                "UPDATE helps SET text = ?, level = ? WHERE id = ?",
                (text, level, row[0])
            )
            print(f"  Updated: [{row[0]}] {keyword}")
        else:
            cursor.execute(
                "INSERT INTO helps (level, keyword, text) VALUES (?, ?, ?)",
                (level, keyword, text)
            )
            new_id = cursor.lastrowid
            print(f"  Created: [{new_id}] {keyword}")

    conn.commit()
    conn.close()
    print("Done. Intro help entries seeded.")

if __name__ == '__main__':
    main()
