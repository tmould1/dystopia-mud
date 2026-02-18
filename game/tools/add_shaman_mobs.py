#!/usr/bin/env python3
"""Add Shaman totem and Spirit Lord spirit warrior mob templates to classeq.db."""

import sqlite3
from pathlib import Path

script_dir = Path(__file__).parent
db_path = script_dir.parent.parent / "gamedata" / "db" / "areas" / "classeq.db"

conn = sqlite3.connect(str(db_path))
cursor = conn.cursor()

# ACT flags: ACT_IS_NPC(2) + ACT_SENTINEL(4) + ACT_NOEXP(128) + ACT_NOPARTS(512)
act_flags = 2 + 4 + 128 + 512

# Shaman Totem (vnum 33596)
cursor.execute("""INSERT OR REPLACE INTO mobiles
    (vnum, player_name, short_descr, long_descr, description,
     act, affected_by, alignment, level, hitroll, ac,
     hitnodice, hitsizedice, hitplus, damnodice, damsizedice, damplus,
     gold, sex)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)""",
    (33596, "spirit totem shaman", "a spirit totem",
     "A spirit totem stands here, pulsing with ethereal energy.\n\r",
     "This carved totem thrums with spiritual energy, spirits swirling around it.",
     act_flags, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0))
print("Added totem mob template (vnum 33596)")

# Spirit Lord Spirit Warrior (vnum 33616)
cursor.execute("""INSERT OR REPLACE INTO mobiles
    (vnum, player_name, short_descr, long_descr, description,
     act, affected_by, alignment, level, hitroll, ac,
     hitnodice, hitsizedice, hitplus, damnodice, damsizedice, damplus,
     gold, sex)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)""",
    (33616, "spirit warrior spectral", "a spirit warrior",
     "A spectral warrior hovers here, weapons ready.\n\r",
     "This translucent warrior shimmers between planes, ancient weapons at the ready.",
     act_flags, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0))
print("Added spirit warrior mob template (vnum 33616)")

conn.commit()
conn.close()
print("Done!")
