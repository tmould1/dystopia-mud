#!/usr/bin/env python3
"""Generate the Candyland area SQLite database."""

import sqlite3
import os

DB_PATH = os.path.join(os.path.dirname(__file__), '..', '..', 'gamedata', 'db', 'areas', 'candyland.db')

# Remove existing database if present
if os.path.exists(DB_PATH):
    os.remove(DB_PATH)

db = sqlite3.connect(DB_PATH)
c = db.cursor()

# Schema (matches db_sql.c SCHEMA_SQL)
c.executescript("""
CREATE TABLE IF NOT EXISTS area (
  name TEXT NOT NULL, builders TEXT DEFAULT '', lvnum INTEGER NOT NULL,
  uvnum INTEGER NOT NULL, security INTEGER DEFAULT 3, recall INTEGER DEFAULT 0,
  area_flags INTEGER DEFAULT 0, is_hidden INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS mobiles (
  vnum INTEGER PRIMARY KEY, player_name TEXT, short_descr TEXT,
  long_descr TEXT, description TEXT, act INTEGER, affected_by INTEGER,
  alignment INTEGER, level INTEGER, hitroll INTEGER, ac INTEGER,
  hitnodice INTEGER, hitsizedice INTEGER, hitplus INTEGER,
  damnodice INTEGER, damsizedice INTEGER, damplus INTEGER,
  gold INTEGER, sex INTEGER
);

CREATE TABLE IF NOT EXISTS objects (
  vnum INTEGER PRIMARY KEY, name TEXT, short_descr TEXT, description TEXT,
  item_type INTEGER, extra_flags INTEGER, wear_flags INTEGER,
  value0 INTEGER, value1 INTEGER, value2 INTEGER, value3 INTEGER,
  weight INTEGER, cost INTEGER,
  chpoweron TEXT, chpoweroff TEXT, chpoweruse TEXT,
  victpoweron TEXT, victpoweroff TEXT, victpoweruse TEXT,
  spectype INTEGER DEFAULT 0, specpower INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS object_affects (
  id INTEGER PRIMARY KEY AUTOINCREMENT, obj_vnum INTEGER NOT NULL REFERENCES objects(vnum),
  location INTEGER NOT NULL, modifier INTEGER NOT NULL, sort_order INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS extra_descriptions (
  id INTEGER PRIMARY KEY AUTOINCREMENT, owner_type TEXT NOT NULL,
  owner_vnum INTEGER NOT NULL, keyword TEXT NOT NULL,
  description TEXT NOT NULL, sort_order INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS rooms (
  vnum INTEGER PRIMARY KEY, name TEXT, description TEXT,
  room_flags INTEGER, sector_type INTEGER
);

CREATE TABLE IF NOT EXISTS exits (
  room_vnum INTEGER, direction INTEGER, description TEXT,
  keyword TEXT, exit_info INTEGER, key_vnum INTEGER, to_vnum INTEGER
);

CREATE TABLE IF NOT EXISTS room_texts (
  room_vnum INTEGER, input TEXT, output TEXT, choutput TEXT,
  name TEXT, type INTEGER, power INTEGER, mob INTEGER
);

CREATE TABLE IF NOT EXISTS resets (
  command TEXT, arg1 INTEGER, arg2 INTEGER, arg3 INTEGER, sort_order INTEGER
);

CREATE TABLE IF NOT EXISTS shops (
  keeper_vnum INTEGER PRIMARY KEY, buy_type0 INTEGER, buy_type1 INTEGER,
  buy_type2 INTEGER, buy_type3 INTEGER, buy_type4 INTEGER,
  profit_buy INTEGER, profit_sell INTEGER, open_hour INTEGER, close_hour INTEGER
);

CREATE TABLE IF NOT EXISTS specials (
  mob_vnum INTEGER PRIMARY KEY, spec_fun_name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS scripts (
  id INTEGER PRIMARY KEY AUTOINCREMENT, owner_type TEXT, owner_vnum INTEGER,
  trigger INTEGER, name TEXT, code TEXT, pattern TEXT,
  chance INTEGER, sort_order INTEGER, library_name TEXT DEFAULT NULL
);
""")

# --- Area metadata ---
c.execute("""INSERT INTO area (name, builders, lvnum, uvnum, security, recall, area_flags, is_hidden)
  VALUES ('Candyland', 'Builder', 40000, 40099, 9, 40000, 0, 0)""")

# --- Rooms ---
# Colors in names only, descriptions are plain text for time-of-day tinting
# sector_type: 0=INSIDE, 2=FIELD (outdoor), 9=AIR
rooms = [
    (40000, '#x223The #x214C#x220a#x221n#x226d#x227y #x223Gate#n',
     'You stand before an enormous gate made entirely of twisted candy canes, their red and white stripes gleaming in an otherworldly light. The air is thick with the scent of sugar and spun confections. Beyond the gate, you can glimpse a land of impossible sweetness - rolling hills of frosting, trees dripping with caramel, and a distant castle that appears to be made of solid chocolate. The only exit leads back to the real world.',
     0, 2),  # outdoor - entrance gate
    (40001, '#x034G#x071u#x077m#x071d#x034r#x071o#x077p #x071Gardens#n',
     'Massive gumdrops the size of boulders dot this surreal landscape, their translucent surfaces glowing with inner light in shades of ruby, emerald, and amber. The ground beneath your feet is soft and slightly sticky, made of compressed sugar crystals. Gumdrop bushes grow in neat rows, their fruits glistening with a sugary sheen.',
     0, 2),  # outdoor - gardens
    (40002, '#x231Peppermint #x196Pathway#n',
     'The ground here is paved with enormous peppermint discs, their surfaces swirling with hypnotic red and white spirals. Tall peppermint stick columns line the path like an endless colonnade, their tips sharpened to dangerous points. A cool, minty breeze carries the faint sound of crunching candy.',
     0, 2),  # outdoor - open pathway
    (40003, '#x053Licorice #x090Labyrinth#n',
     'Walls of braided black licorice rise high around you, their surfaces glistening with a dark, oily sheen. The air carries a pungent anise scent that is almost overwhelming. Twisted ropes of red licorice hang from above like living vines, occasionally twitching as if alive.',
     0, 0),  # inside - enclosed labyrinth
    (40004, '#x136Gingerbread #x172Grotto#n',
     'You find yourself in a vast cavern whose walls, floor, and ceiling are made entirely of gingerbread. Ornate frosting decorations cover every surface - white royal icing drips from stalactites, and gumdrop gems are embedded in the walls. The warm scent of ginger and cinnamon fills the air.',
     0, 0),  # inside - cavern
    (40005, '#x196Candy #x231Cane #x160Corridor#n',
     'A long hallway stretches before you, its walls made of enormous candy canes arranged in alternating red, green, and white patterns. The ceiling arches overhead in a peppermint vault, and the floor is smooth butterscotch, polished to a mirror finish. Crystal sugar chandeliers cast prismatic light across every surface.',
     0, 0),  # inside - hallway
    (40006, '#x058Chocolate #x094Cavern#n',
     'Rich, dark chocolate forms the walls of this enormous cavern, its surface smooth and slightly warm to the touch. Rivers of molten milk chocolate flow through channels carved in the floor, filling the air with an intoxicating aroma. Stalactites of white chocolate hang from the ceiling, occasionally dripping creamy droplets.',
     0, 0),  # inside - cavern
    (40007, '#x213T#x218a#x219f#x218f#x213y #x218Tunnel#n',
     'The walls of this tunnel are made of stretched saltwater taffy in every color imaginable - pink, blue, yellow, green - all swirled together in psychedelic patterns. The surfaces pulse and stretch as if breathing, and the floor has a disconcerting bounce to it. The sweet, buttery scent of taffy permeates everything.',
     0, 0),  # inside - tunnel
    (40008, '#x196L#x208o#x220l#x046l#x033i#x135p#x196o#x208p #x220Landing#n',
     'Giant lollipops rise from the ground like alien trees, their flat circular tops forming a colorful canopy overhead. Spiral-patterned trunks of hardened sugar support these candy platforms. The ground is covered in crushed rock candy that crunches underfoot, sparkling like gemstones in the filtered candy-colored light.',
     0, 2),  # outdoor - open area with lollipop trees
    (40009, '#x231Marshmallow #x253Meadow#n',
     'Soft, pillowy marshmallow forms gentle rolling hills in every direction. The ground gives way with each step, and the white landscape is broken only by the occasional chocolate chip boulder or graham cracker outcropping. A sweet vanilla scent hangs in the warm air.',
     0, 2),  # outdoor - open meadow
    (40010, '#x218C#x183o#x147t#x111t#x147o#x183n #x218C#x183a#x147n#x111d#x147y #x189Clouds#n',
     'You stand on what appears to be a solid cloud of pink and blue cotton candy. The fluffy substance supports your weight but dissolves slightly with each step, reforming behind you. Other cotton candy clouds float nearby, and far below you can see the sugary landscape of Candyland spread out like a dream.',
     0, 9),  # air - floating on clouds
    (40011, '#x223The #x214C#x220a#x221n#x226d#x227y #x223Castle #x226Throne Room#n',
     'This magnificent hall is the heart of the Candy Castle itself. Walls of layered hard candy in stained-glass patterns filter light into rainbow shafts. The floor is polished butterscotch, and enormous peppermint pillars support a ceiling of sculpted marzipan depicting scenes of candy warfare. At the far end sits an enormous throne made of crystallized sugar.',
     0, 0),  # inside - castle throne room
]

# Ensure all descriptions end with \n\r for proper display spacing
rooms = [(v, n, d + '\n\r' if not d.endswith('\n\r') else d, rf, st) for v, n, d, rf, st in rooms]

c.executemany("INSERT INTO rooms (vnum, name, description, room_flags, sector_type) VALUES (?,?,?,?,?)", rooms)

# --- Exit from entry room back to Midgaard Park Entrance (west = 3) ---
c.execute("INSERT INTO exits (room_vnum, direction, description, keyword, exit_info, key_vnum, to_vnum) VALUES (?, ?, ?, ?, ?, ?, ?)",
          (40000, 3, 'The path leads back to the park.', '', 0, -1, 3105))

# --- Mobiles ---
# ACT_IS_NPC=1, ACT_SENTINEL=2, ACT_AGGRESSIVE=8, ACT_STAY_AREA=16
# Entry guard: 1|2|16 = 19 (not aggressive)
# Others:      1|2|8|16 = 27 (aggressive)
# Death teleport behavior is handled by TRIG_DEATH Lua scripts (see below).

mobiles = [
    # vnum, player_name, short_descr, long_descr, description,
    # act, affected_by, alignment, level, hitroll, ac,
    # hitnodice, hitsizedice, hitplus, damnodice, damsizedice, damplus,
    # gold, sex
    (40000, 'gumdrop guard',
     '#x223the #x034G#x071u#x077m#x071d#x034r#x071o#x077p #x034Guard#n',
     '#x223A guard made of hardened #x034g#x071u#x077m#x071d#x034r#x071o#x077p#x034s #x223stands watch here.#n\n\r',
     'This sentinel is composed entirely of compressed gumdrops in various colors. Its body is translucent, revealing a sticky, sugary interior. Despite its sweet appearance, it looks ready to fight.',
     19, 0, 0, 40, 10, 50, 8, 8, 400, 4, 4, 40, 0, 0),

    (40001, 'gumdrop golem',
     '#x223the #x034G#x071u#x077m#x071d#x034r#x071o#x077p #x071Golem#n',
     '#x223A massive #x071golem #x223of fused #x034g#x071u#x077m#x071d#x034r#x071o#x077p#x034s #x223lumbers about.#n\n\r',
     'Towering above you, this golem is made of thousands of gumdrops fused together into a hulking form. Its fists are dense clusters of hardened candy.',
     27, 0, 0, 41, 11, 48, 8, 8, 420, 4, 4, 42, 0, 0),

    (40002, 'peppermint soldier',
     '#x223the #x231Peppermint #x196Soldier#n',
     '#x223A soldier in #x231peppermint #x223armor patrols aggressively.#n\n\r',
     'Clad in armor forged from hardened peppermint, this soldier carries a sharpened candy cane spear. A cool minty aura surrounds it.',
     27, 0, 0, 41, 11, 46, 8, 9, 430, 4, 5, 44, 0, 0),

    (40003, 'licorice lord',
     '#x223the #x053Licorice #x090Lord#n',
     '#x223A dark figure wrapped in #x053licorice #x090tentacles #x223lurks here.#n\n\r',
     'This creature appears to be made entirely of black licorice, its body shifting and reforming constantly. Whip-like tendrils of licorice lash out from its form.',
     27, 0, 0, 42, 12, 44, 9, 9, 450, 5, 5, 46, 0, 0),

    (40004, 'gingerbread warrior',
     '#x223the #x136Gingerbread #x172Warrior#n',
     '#x223A massive #x136gingerbread #x172warrior #x223stands ready for battle.#n\n\r',
     'This warrior is carved from a single enormous slab of gingerbread, decorated with fierce frosting war paint. It wields a candy cane greataxe.',
     27, 0, 0, 42, 12, 42, 9, 9, 460, 5, 5, 48, 0, 0),

    (40005, 'candy cane sentinel',
     '#x223the #x196Candy #x231Cane #x160Sentinel#n',
     '#x223A #x196sentinel #x223forged from twisted #x231candy #x196canes #x223blocks your path.#n\n\r',
     'This towering sentinel is made of intertwined candy canes, red and white stripes spiraling along its crystalline form. It crackles with sugary energy.',
     27, 0, 0, 43, 13, 40, 9, 10, 480, 5, 5, 50, 0, 0),

    (40006, 'chocolate knight',
     '#x223the #x058Chocolate #x094Knight#n',
     '#x223A knight in dark #x058chocolate #x094armor #x223challenges all comers.#n\n\r',
     'Encased in armor of tempered dark chocolate, this knight is an imposing figure. Its shield bears a cocoa bean crest, and its sword drips with molten chocolate.',
     27, 0, 0, 43, 13, 38, 10, 10, 490, 5, 6, 52, 0, 0),

    (40007, 'taffy titan',
     '#x223the #x213T#x218a#x219f#x218f#x213y #x218Titan#n',
     '#x223An enormous #x213t#x218i#x219t#x218a#x213n #x223of stretched #x218taffy #x223fills the space.#n\n\r',
     'This massive creature is made of saltwater taffy, its body constantly stretching and reforming. Blows seem to sink into its elastic form before being hurled back.',
     27, 0, 0, 43, 14, 36, 10, 10, 500, 5, 6, 54, 0, 0),

    (40008, 'lollipop lancer',
     '#x223the #x196L#x208o#x220l#x046l#x033i#x135p#x196o#x208p #x220Lancer#n',
     '#x223A #x220lancer #x223mounted on a #x196g#x208u#x046m#x033m#x135y #x033bear #x223charges forward.#n\n\r',
     'This warrior rides a giant gummy bear mount and carries an enormous lollipop lance. The lance has been sharpened to a razor edge.',
     27, 0, 0, 44, 14, 34, 10, 10, 520, 6, 6, 56, 0, 0),

    (40009, 'marshmallow mauler',
     '#x223the #x231Marshmallow #x253Mauler#n',
     '#x223A hulking #x231marshmallow #x253creature #x223raises enormous fists.#n\n\r',
     'This creature resembles an enormous, slightly charred marshmallow with muscular arms. Despite its soft appearance, its blows land with devastating force.',
     27, 0, 0, 44, 15, 32, 10, 11, 540, 6, 6, 58, 0, 0),

    (40010, 'cotton candy cyclone',
     '#x223the #x218C#x183o#x147t#x111t#x147o#x183n #x218C#x183a#x147n#x111d#x147y #x218Cyclone#n',
     '#x223A whirling vortex of #x218c#x183o#x147t#x111t#x147o#x183n #x218c#x183a#x147n#x111d#x147y #x223tears through the area.#n\n\r',
     'This living tornado of cotton candy spins with incredible force. Shards of crystallized sugar fly from its form like deadly shrapnel.',
     27, 0, 0, 44, 15, 30, 11, 11, 560, 6, 7, 60, 0, 0),

    (40011, 'king kandy',
     '#x214K#x220i#x221n#x226g #x214K#x220a#x221n#x226d#x227y#n',
     '#x214K#x220i#x221n#x226g #x214K#x220a#x221n#x226d#x227y #x223sits upon his crystallized #x220sugar #x226throne#x223, radiating power.#n\n\r',
     'The legendary ruler of Candyland sits before you in all his sugary glory. His crown is made of the finest chocolate, his robes spun from cotton candy, and his scepter is a massive candy cane encrusted with rock candy gems. His eyes burn with an ancient confectionery power.',
     27, 0, 0, 45, 16, 25, 12, 12, 600, 7, 7, 70, 0, 0),
]

c.executemany("""INSERT INTO mobiles (vnum, player_name, short_descr, long_descr, description,
  act, affected_by, alignment, level, hitroll, ac,
  hitnodice, hitsizedice, hitplus, damnodice, damsizedice, damplus,
  gold, sex)
  VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""", mobiles)

# --- Objects ---
# item_type: 5=WEAPON, 9=ARMOR
# extra_flags: 64=ITEM_LOYAL
# wear_flags: ITEM_TAKE=1, ITEM_WEAR_BODY=8, ITEM_WEAR_HEAD=16, ITEM_WEAR_LEGS=32,
#   ITEM_WEAR_FEET=64, ITEM_WEAR_HANDS=128, ITEM_WEAR_SHIELD=512, ITEM_WEAR_ABOUT=1024,
#   ITEM_WIELD=8192

objects = [
    # vnum, name, short_descr, description, item_type, extra_flags, wear_flags,
    # v0, v1, v2, v3, weight, cost
    (40050, 'candy cane sword',
     '#x223a #x196Candy #x231Cane #x160Sword ~#n',
     'A wickedly sharp sword made from a twisted candy cane lies here.',
     5, 64, 1|8192, 0, 5, 5, 0, 5, 0),

    (40051, 'gumdrop shield',
     '#x223a #x034G#x071u#x077m#x071d#x034r#x071o#x077p #x034Shield#n',
     'A shield made of fused, hardened gumdrops lies here.',
     9, 64, 1|512, 50, 0, 0, 0, 10, 0),

    (40052, 'peppermint armor',
     '#x223a suit of #x231Peppermint #x196Armor#n',
     'A suit of armor forged from hardened peppermint lies here.',
     9, 64, 1|8, 75, 0, 0, 0, 15, 0),

    (40053, 'chocolate crown',
     '#x223a #x058Chocolate #x220Crown *#n',
     'A crown of tempered dark chocolate rests here.',
     9, 64, 1|16, 40, 0, 0, 0, 3, 0),

    (40054, 'licorice gauntlets',
     '#x223a pair of #x053Licorice #x090Gauntlets#n',
     'A pair of gauntlets woven from hardened black licorice lie here.',
     9, 64, 1|128, 30, 0, 0, 0, 4, 0),

    (40055, 'gingerbread greaves',
     '#x223a pair of #x136Gingerbread #x172Greaves#n',
     'A pair of greaves carved from reinforced gingerbread lie here.',
     9, 64, 1|32, 35, 0, 0, 0, 8, 0),

    (40056, 'taffy boots',
     '#x223a pair of #x213T#x218a#x219f#x218f#x213y #x218Boots#n',
     'A pair of boots made from stretched and hardened taffy lie here.',
     9, 64, 1|64, 25, 0, 0, 0, 5, 0),

    (40057, 'cotton candy cloak',
     '#x223a #x218C#x183o#x147t#x111t#x147o#x183n #x218C#x183a#x147n#x111d#x147y #x117Cloak#n',
     'A shimmering cloak of woven cotton candy lies here.',
     9, 64, 1|1024, 45, 0, 0, 0, 2, 0),
]

c.executemany("""INSERT INTO objects (vnum, name, short_descr, description,
  item_type, extra_flags, wear_flags, value0, value1, value2, value3, weight, cost)
  VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)""", objects)

# --- Object affects ---
# APPLY_STR=1, APPLY_DEX=2, APPLY_CON=5, APPLY_MANA=12, APPLY_HIT=13,
# APPLY_MOVE=14, APPLY_AC=17, APPLY_HITROLL=18, APPLY_DAMROLL=19

object_affects = [
    # Candy Cane Sword: +5 hitroll, +5 damroll
    (40050, 18, 5, 0), (40050, 19, 5, 1),
    # Gumdrop Shield: +50 hp, -10 AC
    (40051, 13, 50, 0), (40051, 17, -10, 1),
    # Peppermint Armor: +75 hp, -15 AC
    (40052, 13, 75, 0), (40052, 17, -15, 1),
    # Chocolate Crown: +3 hitroll, +3 damroll
    (40053, 18, 3, 0), (40053, 19, 3, 1),
    # Licorice Gauntlets: +2 str, +2 dex
    (40054, 1, 2, 0), (40054, 2, 2, 1),
    # Gingerbread Greaves: +2 con, +50 move
    (40055, 5, 2, 0), (40055, 14, 50, 1),
    # Taffy Boots: +2 dex, +30 move
    (40056, 2, 2, 0), (40056, 14, 30, 1),
    # Cotton Candy Cloak: +60 hp, +40 mana
    (40057, 13, 60, 0), (40057, 12, 40, 1),
]

c.executemany("INSERT INTO object_affects (obj_vnum, location, modifier, sort_order) VALUES (?,?,?,?)",
              object_affects)

# --- Resets ---
# 'M' = mob: arg1=mob_vnum, arg2=max_count, arg3=room_vnum
# 'G' = give obj to last mob: arg1=obj_vnum, arg2=0, arg3=0
resets = [
    ('M', 40000, 1, 40000, 1),   # Gumdrop Guard in entry room
    ('M', 40001, 1, 40001, 2),   # Gumdrop Golem
    ('M', 40002, 1, 40002, 3),   # Peppermint Soldier
    ('G', 40052, 0, 0, 4),       #   Give: Peppermint Armor
    ('M', 40003, 1, 40003, 5),   # Licorice Lord
    ('G', 40054, 0, 0, 6),       #   Give: Licorice Gauntlets
    ('M', 40004, 1, 40004, 7),   # Gingerbread Warrior
    ('G', 40055, 0, 0, 8),       #   Give: Gingerbread Greaves
    ('M', 40005, 1, 40005, 9),   # Candy Cane Sentinel
    ('G', 40050, 0, 0, 10),      #   Give: Candy Cane Sword
    ('M', 40006, 1, 40006, 11),  # Chocolate Knight (no loot)
    ('M', 40007, 1, 40007, 12),  # Taffy Titan
    ('G', 40056, 0, 0, 13),      #   Give: Taffy Boots
    ('M', 40008, 1, 40008, 14),  # Lollipop Lancer (no loot)
    ('M', 40009, 1, 40009, 15),  # Marshmallow Mauler (no loot)
    ('M', 40010, 1, 40010, 16),  # Cotton Candy Cyclone
    ('G', 40057, 0, 0, 17),      #   Give: Cotton Candy Cloak
    ('M', 40011, 1, 40011, 18),  # King Kandy
    ('G', 40053, 0, 0, 19),      #   Give: Chocolate Crown
    ('G', 40051, 0, 0, 20),      #   Give: Gumdrop Shield
]

c.executemany("INSERT INTO resets (command, arg1, arg2, arg3, sort_order) VALUES (?,?,?,?,?)", resets)

# --- Scripts ---
# TRIG_DEATH Lua scripts: gauntlet for regular mobs, direct entrance for boss.
# Regular mobs (40000-40010): death_gauntlet library script
# King Kandy (40011): death_direct_entrance library script
scripts = []
for vnum in range(40000, 40011):
    scripts.append(('mob', vnum, 0, '', '', None, 0, len(scripts), 'death_gauntlet'))
scripts.append(('mob', 40011, 0, '', '', None, 0, len(scripts), 'death_direct_entrance'))

c.executemany("""INSERT INTO scripts (owner_type, owner_vnum, trigger, name, code, pattern,
  chance, sort_order, library_name)
  VALUES (?,?,?,?,?,?,?,?,?)""", scripts)

db.commit()
db.close()

print(f"Created {DB_PATH}")
print("Candyland area database generated successfully!")
