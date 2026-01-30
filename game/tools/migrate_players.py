#!/usr/bin/env python3
"""
Migrate Diku/Merc MUD player text save files to per-player SQLite databases.

Reads all player files from gamedata/player/ and creates corresponding .db
files in gamedata/db/players/ using the same schema as db_player.c.

Usage:
    python migrate_players.py [--dry-run] [--player-dir PATH] [--db-dir PATH]

Run from the repository root (dystopia-mud/).
"""

import argparse
import logging
import os
import sqlite3
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Logging setup
# ---------------------------------------------------------------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(levelname)-5s  %(message)s",
)
log = logging.getLogger("migrate_players")

# ---------------------------------------------------------------------------
# SQLite schema  (must match db_player.c PLAYER_SCHEMA_SQL)
# ---------------------------------------------------------------------------
PLAYER_SCHEMA_SQL = """
CREATE TABLE IF NOT EXISTS meta (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS player (
    /* Identity strings */
    name           TEXT NOT NULL,
    switchname     TEXT NOT NULL DEFAULT '',
    short_descr    TEXT NOT NULL DEFAULT '',
    long_descr     TEXT NOT NULL DEFAULT '',
    objdesc        TEXT NOT NULL DEFAULT '',
    description    TEXT NOT NULL DEFAULT '',
    lord           TEXT NOT NULL DEFAULT '',
    clan           TEXT NOT NULL DEFAULT '',
    morph          TEXT NOT NULL DEFAULT '',
    createtime     TEXT NOT NULL DEFAULT '',
    lasttime       TEXT NOT NULL DEFAULT '',
    lasthost       TEXT NOT NULL DEFAULT '',
    poweraction    TEXT NOT NULL DEFAULT '',
    powertype      TEXT NOT NULL DEFAULT '',
    prompt         TEXT NOT NULL DEFAULT '',
    cprompt        TEXT NOT NULL DEFAULT '',
    /* PC-only strings */
    password       TEXT NOT NULL DEFAULT '',
    bamfin         TEXT NOT NULL DEFAULT '',
    bamfout        TEXT NOT NULL DEFAULT '',
    title          TEXT NOT NULL DEFAULT '',
    conception     TEXT NOT NULL DEFAULT '',
    parents        TEXT NOT NULL DEFAULT '',
    cparents       TEXT NOT NULL DEFAULT '',
    marriage       TEXT NOT NULL DEFAULT '',
    decapmessage   TEXT NOT NULL DEFAULT '',
    loginmessage   TEXT NOT NULL DEFAULT '',
    logoutmessage  TEXT NOT NULL DEFAULT '',
    avatarmessage  TEXT NOT NULL DEFAULT '',
    tiemessage     TEXT NOT NULL DEFAULT '',
    last_decap_0   TEXT NOT NULL DEFAULT '',
    last_decap_1   TEXT NOT NULL DEFAULT '',
    /* Core ints */
    sex            INTEGER NOT NULL DEFAULT 0,
    class          INTEGER NOT NULL DEFAULT 0,
    level          INTEGER NOT NULL DEFAULT 0,
    trust          INTEGER NOT NULL DEFAULT 0,
    played         INTEGER NOT NULL DEFAULT 0,
    room_vnum      INTEGER NOT NULL DEFAULT 3001,
    gold           INTEGER NOT NULL DEFAULT 0,
    exp            INTEGER NOT NULL DEFAULT 0,
    expgained      INTEGER NOT NULL DEFAULT 0,
    act            INTEGER NOT NULL DEFAULT 0,
    extra          INTEGER NOT NULL DEFAULT 0,
    newbits        INTEGER NOT NULL DEFAULT 0,
    special        INTEGER NOT NULL DEFAULT 0,
    affected_by    INTEGER NOT NULL DEFAULT 0,
    immune         INTEGER NOT NULL DEFAULT 0,
    polyaff        INTEGER NOT NULL DEFAULT 0,
    itemaffect     INTEGER NOT NULL DEFAULT 0,
    form           INTEGER NOT NULL DEFAULT 1048575,
    position       INTEGER NOT NULL DEFAULT 0,
    practice       INTEGER NOT NULL DEFAULT 0,
    saving_throw   INTEGER NOT NULL DEFAULT 0,
    alignment      INTEGER NOT NULL DEFAULT 0,
    xhitroll       INTEGER NOT NULL DEFAULT 0,
    xdamroll       INTEGER NOT NULL DEFAULT 0,
    hitroll        INTEGER NOT NULL DEFAULT 0,
    damroll        INTEGER NOT NULL DEFAULT 0,
    armor          INTEGER NOT NULL DEFAULT 0,
    wimpy          INTEGER NOT NULL DEFAULT 0,
    deaf           INTEGER NOT NULL DEFAULT 0,
    beast          INTEGER NOT NULL DEFAULT 15,
    home           INTEGER NOT NULL DEFAULT 3001,
    spectype       INTEGER NOT NULL DEFAULT 0,
    specpower      INTEGER NOT NULL DEFAULT 0,
    /* HP/Mana/Move */
    hit            INTEGER NOT NULL DEFAULT 0,
    max_hit        INTEGER NOT NULL DEFAULT 0,
    mana           INTEGER NOT NULL DEFAULT 0,
    max_mana       INTEGER NOT NULL DEFAULT 0,
    move           INTEGER NOT NULL DEFAULT 0,
    max_move       INTEGER NOT NULL DEFAULT 0,
    /* PK/PD/MK/MD */
    pkill          INTEGER NOT NULL DEFAULT 0,
    pdeath         INTEGER NOT NULL DEFAULT 0,
    mkill          INTEGER NOT NULL DEFAULT 0,
    mdeath         INTEGER NOT NULL DEFAULT 0,
    awins          INTEGER NOT NULL DEFAULT 0,
    alosses        INTEGER NOT NULL DEFAULT 0,
    /* Class-specific */
    warp           INTEGER NOT NULL DEFAULT 0,
    warpcount      INTEGER NOT NULL DEFAULT 0,
    monkstuff      INTEGER NOT NULL DEFAULT 0,
    monkcrap       INTEGER NOT NULL DEFAULT 0,
    garou1         INTEGER NOT NULL DEFAULT 0,
    garou2         INTEGER NOT NULL DEFAULT 0,
    rage           INTEGER NOT NULL DEFAULT 0,
    generation     INTEGER NOT NULL DEFAULT 0,
    cur_form       INTEGER NOT NULL DEFAULT 0,
    flag2          INTEGER NOT NULL DEFAULT 0,
    flag3          INTEGER NOT NULL DEFAULT 0,
    flag4          INTEGER NOT NULL DEFAULT 0,
    siltol         INTEGER NOT NULL DEFAULT 0,
    gnosis_max     INTEGER NOT NULL DEFAULT 0,
    /* PC_DATA ints */
    kingdom        INTEGER NOT NULL DEFAULT 0,
    quest          INTEGER NOT NULL DEFAULT 0,
    rank           INTEGER NOT NULL DEFAULT 0,
    bounty         INTEGER NOT NULL DEFAULT 0,
    security       INTEGER NOT NULL DEFAULT 0,
    jflags         INTEGER NOT NULL DEFAULT 0,
    souls          INTEGER NOT NULL DEFAULT 0,
    upgrade_level  INTEGER NOT NULL DEFAULT 0,
    mean_paradox   INTEGER NOT NULL DEFAULT 0,
    relrank        INTEGER NOT NULL DEFAULT 0,
    rune_count     INTEGER NOT NULL DEFAULT 0,
    revision       INTEGER NOT NULL DEFAULT 0,
    disc_research  INTEGER NOT NULL DEFAULT -1,
    disc_points    INTEGER NOT NULL DEFAULT 0,
    obj_vnum       INTEGER NOT NULL DEFAULT 0,
    exhaustion     INTEGER NOT NULL DEFAULT 0,
    questsrun      INTEGER NOT NULL DEFAULT 0,
    questtotal     INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS player_arrays (
    name TEXT PRIMARY KEY,
    data TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS skills (
    skill_name TEXT PRIMARY KEY,
    value      INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS aliases (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    short_n TEXT NOT NULL,
    long_n  TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS affects (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    skill_name TEXT NOT NULL,
    duration   INTEGER NOT NULL,
    modifier   INTEGER NOT NULL,
    location   INTEGER NOT NULL,
    bitvector  INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS boards (
    board_name TEXT PRIMARY KEY,
    last_note  INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS objects (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    nest         INTEGER NOT NULL DEFAULT 0,
    vnum         INTEGER NOT NULL,
    name         TEXT NOT NULL DEFAULT '',
    short_descr  TEXT NOT NULL DEFAULT '',
    description  TEXT NOT NULL DEFAULT '',
    chpoweron    TEXT,
    chpoweroff   TEXT,
    chpoweruse   TEXT,
    victpoweron  TEXT,
    victpoweroff TEXT,
    victpoweruse TEXT,
    questmaker   TEXT,
    questowner   TEXT,
    extra_flags  INTEGER NOT NULL DEFAULT 0,
    extra_flags2 INTEGER NOT NULL DEFAULT 0,
    weapflags    INTEGER NOT NULL DEFAULT 0,
    wear_flags   INTEGER NOT NULL DEFAULT 0,
    wear_loc     INTEGER NOT NULL DEFAULT 0,
    item_type    INTEGER NOT NULL DEFAULT 0,
    weight       INTEGER NOT NULL DEFAULT 0,
    spectype     INTEGER NOT NULL DEFAULT 0,
    specpower    INTEGER NOT NULL DEFAULT 0,
    condition    INTEGER NOT NULL DEFAULT 100,
    toughness    INTEGER NOT NULL DEFAULT 0,
    resistance   INTEGER NOT NULL DEFAULT 100,
    quest        INTEGER NOT NULL DEFAULT 0,
    points       INTEGER NOT NULL DEFAULT 0,
    level        INTEGER NOT NULL DEFAULT 0,
    timer        INTEGER NOT NULL DEFAULT 0,
    cost         INTEGER NOT NULL DEFAULT 0,
    value_0      INTEGER NOT NULL DEFAULT 0,
    value_1      INTEGER NOT NULL DEFAULT 0,
    value_2      INTEGER NOT NULL DEFAULT 0,
    value_3      INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS obj_affects (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    obj_id   INTEGER NOT NULL,
    duration INTEGER NOT NULL,
    modifier INTEGER NOT NULL,
    location INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS obj_extra_descr (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    obj_id      INTEGER NOT NULL,
    keyword     TEXT NOT NULL,
    description TEXT NOT NULL
);
"""


# =========================================================================
# Text file reader -- mimics C fread_* functions
# =========================================================================
class MudFileReader:
    """
    Reads a Diku/Merc-style player text file.

    The file is treated as a stream of characters; whitespace (spaces,
    tabs, newlines) is generally interchangeable.  Strings are terminated
    by '~'.  Numbers are plain integers (possibly negative).
    """

    def __init__(self, filepath: Path):
        with open(filepath, "r", encoding="latin-1") as f:
            self.data = f.read()
        self.pos = 0
        self.filepath = filepath

    # -- low-level helpers ------------------------------------------------

    def _at_end(self) -> bool:
        return self.pos >= len(self.data)

    def _peek(self) -> str:
        if self._at_end():
            return ""
        return self.data[self.pos]

    def _advance(self) -> str:
        ch = self.data[self.pos]
        self.pos += 1
        return ch

    def _skip_whitespace(self):
        """Skip spaces, tabs, newlines, carriage returns."""
        while self.pos < len(self.data) and self.data[self.pos] in " \t\n\r":
            self.pos += 1

    # -- public API matching C fread_* functions --------------------------

    def fread_letter(self) -> str:
        """Skip whitespace, return next character (like C fread_letter)."""
        self._skip_whitespace()
        if self._at_end():
            return ""
        return self._advance()

    def fread_word(self) -> str:
        """
        Skip whitespace, read one word (non-whitespace token).
        If the word starts with a single quote, read until closing quote.
        Mirrors C fread_word().
        """
        self._skip_whitespace()
        if self._at_end():
            return ""

        # Quoted word (skill names use 'some skill')
        if self._peek() == "'":
            self._advance()  # skip opening quote
            word = []
            while not self._at_end() and self._peek() != "'":
                word.append(self._advance())
            if not self._at_end():
                self._advance()  # skip closing quote
            return "".join(word)

        word = []
        while not self._at_end() and self.data[self.pos] not in " \t\n\r":
            word.append(self._advance())
        return "".join(word)

    def fread_string(self) -> str:
        """
        Skip leading whitespace, read characters until '~'.
        Trim trailing whitespace from the result.
        Mirrors C fread_string().
        """
        self._skip_whitespace()
        result = []
        while not self._at_end():
            ch = self._advance()
            if ch == "~":
                break
            result.append(ch)
        return "".join(result).rstrip()

    def fread_number(self) -> int:
        """
        Skip whitespace, read an integer (optional sign + digits).
        Mirrors C fread_number().
        """
        self._skip_whitespace()
        sign = 1
        if not self._at_end() and self._peek() in "+-":
            if self._advance() == "-":
                sign = -1

        num = 0
        while not self._at_end() and self.data[self.pos].isdigit():
            num = num * 10 + int(self._advance())

        return sign * num

    def fread_numbers(self, count: int) -> list:
        """Read *count* integers separated by whitespace."""
        return [self.fread_number() for _ in range(count)]


# =========================================================================
# Player data container
# =========================================================================
class PlayerData:
    """Holds all parsed data for one player."""

    def __init__(self):
        # String fields  (text key -> DB column)
        self.strings = {}
        # Single-int fields  (DB column -> value)
        self.ints = {}
        # Array fields  (array name -> list of ints)
        self.arrays = {}
        # Skills  (skill_name -> value)
        self.skills = {}
        # Aliases  [(short, long), ...]
        self.aliases = []
        # Character affects  [(skill_name, duration, modifier, location, bitvector), ...]
        self.affects = []
        # Board timestamps  [(board_name, last_note), ...]
        self.boards = []
        # Objects  [dict, ...]
        self.objects = []


# =========================================================================
# Parser
# =========================================================================

# Mapping: text file key -> (DB column, type)
# type: 's' = string, 'i' = single int, 'a:NAME:COUNT' = array, 'x' = ignored
# For strings, the DB column is the player table column name.
# For ints, the DB column is the player table column name.

# String keys: text key -> DB column name
STRING_KEYS = {
    "Name":           "name",
    "Switchname":     "switchname",
    "ShortDescr":     "short_descr",
    "LongDescr":      "long_descr",
    "ObjDesc":        "objdesc",
    "Description":    "description",
    "Lord":           "lord",
    "Clan":           "clan",
    "Morph":          "morph",
    "Createtime":     "createtime",
    "Lasttime":       "lasttime",
    "Lasthost":       "lasthost",
    "Poweraction":    "poweraction",
    "Powertype":      "powertype",
    "Prompt":         "prompt",
    "Cprompt":        "cprompt",
    "Password":       "password",
    "Bamfin":         "bamfin",
    "Bamfout":        "bamfout",
    "Title":          "title",
    "Decapmessage":   "decapmessage",
    "Loginmessage":   "loginmessage",
    "Logoutmessage":  "logoutmessage",
    "Avatarmessage":  "avatarmessage",
    "Tiemessage":     "tiemessage",
    "Conception":     "conception",
    "Parents":        "parents",
    "Cparents":       "cparents",
    "Marriage":       "marriage",
    "Lastdecap1":     "last_decap_0",
    "Lastdecap2":     "last_decap_1",
}

# Single-int keys: text key -> DB column name
INT_KEYS = {
    "Sex":           "sex",
    "Class":         "class",
    "Level":         "level",
    "Trust":         "trust",
    "Played":        "played",
    "Gold":          "gold",
    "Exp":           "exp",
    "Expgained":     "expgained",
    "Act":           "act",
    "Extra":         "extra",
    "Newbits":       "newbits",
    "Special":       "special",
    "AffectedBy":    "affected_by",
    "Immune":        "immune",
    "Polyaff":       "polyaff",
    "Itemaffect":    "itemaffect",
    "Form":          "form",
    "Beast":         "beast",
    "Home":          "home",
    "Spectype":      "spectype",
    "Specpower":     "specpower",
    "Position":      "position",
    "Practice":      "practice",
    "SavingThrow":   "saving_throw",
    "Alignment":     "alignment",
    "XHitroll":      "xhitroll",
    "XDamroll":      "xdamroll",
    "Hitroll":       "hitroll",
    "Damroll":       "damroll",
    "Armor":         "armor",
    "Wimpy":         "wimpy",
    "Deaf":          "deaf",
    "Kingdom":       "kingdom",
    "Quest":         "quest",
    "Rank":          "rank",
    "Bounty":        "bounty",
    "Security":      "security",
    "Jflags":        "jflags",
    "Souls":         "souls",
    "Rage":          "rage",
    "Generation":    "generation",
    "Flag2":         "flag2",
    "Flag3":         "flag3",
    "Flag4":         "flag4",
    "Monkstuff":     "monkstuff",
    "Monkcrap":      "monkcrap",
    "Garou1":        "garou1",
    "Garou2":        "garou2",
    "Room":          "room_vnum",
    "Awin":          "awins",
    "Alos":          "alosses",
    "Exhaustion":    "exhaustion",
    # Mapped keys (text name differs from DB column)
    "Upgradelevel":  "upgrade_level",
    "Meanparadox":   "mean_paradox",
    "Relrank":       "relrank",
    "Runecount":     "rune_count",
    "Revision":      "revision",
    "Gnosis":        "gnosis_max",
    "DiscRese":      "disc_research",
    "DiscPoin":      "disc_points",
    "CurrentForm":   "cur_form",
    "SilTol":        "siltol",
    "Questsrun":     "questsrun",
    "Queststotal":   "questtotal",
    "Objvnum":       "obj_vnum",
    "Warps":         "warp",
    "WarpCount":     "warpcount",
}

# Multi-int array keys: text key -> (DB array name, count)
ARRAY_KEYS = {
    "CPower":       ("power", 44),
    "Weapons":      ("wpn", 13),
    "Spells":       ("spl", 5),
    "Combat":       ("cmbt", 8),
    "Locationhp":   ("loc_hp", 7),
    "HpManaMove":   (None, 6),      # special: split into 6 individual DB columns
    "PkPdMkMd":     (None, 4),      # special: split into 4 individual DB columns
    "Chi":          ("chi", 2),
    "Focus":        ("focus", 2),
    "Monkab":       ("monkab", 4),
    "Gifts":        ("gifts", 21),
    "Paradox":      ("paradox", 3),
    "AttrPerm":     ("attr_perm", 5),
    "AttrMod":      ("attr_mod", 5),
    "Language":     ("language", 2),
    "Stage":        ("stage", 3),
    "Score":        ("score", 6),
    "Genes":        ("genes", 10),
    "Power":        ("powers", 20),
    "Stats":        ("stats", 12),
    "FakeCon":      ("fake_con", 8),
    "Condition":    ("condition", 3),
    "StatAbility":  ("stat_ability", 4),
    "StatAmount":   ("stat_amount", 4),
    "StatDuration": ("stat_duration", 4),
    # Stance is split across two keys (0..11 and 12..23)
    "Stance":       (None, 12),     # special handling
    "Stance2":      (None, 12),     # special handling
}

# Legacy/ignored string keys (read the tilde-terminated string and discard)
IGNORED_STRING_KEYS = {"Email", "Smite", "Breath1", "Breath2", "Breath3",
                       "Breath4", "Breath5"}

# Legacy/ignored int keys (read one integer and discard)
IGNORED_INT_KEYS = {"Vnum", "Race", "Demonic", "Dragonaff", "Dragonage",
                    "Drowaff", "Drowpwr", "Drowmag", "Hatch", "MageFlags",
                    "Levelexp", "Vampaff", "Vampgen", "Power_Point",
                    "Wolf", "Explevel"}

# Legacy/ignored multi-int keys: key -> count
IGNORED_ARRAY_KEYS = {
    "Disc":     11,
    "Wolfform": 2,
    "Runes":    4,
}

# Object string keys: text key -> DB column
OBJ_STRING_KEYS = {
    "Name":          "name",
    "ShortDescr":    "short_descr",
    "Description":   "description",
    "Poweronch":     "chpoweron",
    "Poweroffch":    "chpoweroff",
    "Powerusech":    "chpoweruse",
    "Poweronvict":   "victpoweron",
    "Poweroffvict":  "victpoweroff",
    "Powerusevict":  "victpoweruse",
    "Questmaker":    "questmaker",
    "Questowner":    "questowner",
}

# Object single-int keys: text key -> DB column
OBJ_INT_KEYS = {
    "Nest":          "nest",
    "Vnum":          "vnum",
    "ExtraFlags":    "extra_flags",
    "ExtraFlags2":   "extra_flags2",
    "WeapFlags":     "weapflags",
    "WearFlags":     "wear_flags",
    "WearLoc":       "wear_loc",
    "ItemType":      "item_type",
    "Weight":        "weight",
    "Spectype":      "spectype",
    "Specpower":     "specpower",
    "Condition":     "condition",
    "Toughness":     "toughness",
    "Resistance":    "resistance",
    "Quest":         "quest",
    "Points":        "points",
    "Level":         "level",
    "Timer":         "timer",
    "Cost":          "cost",
}


def clean_string(s: str) -> str:
    """
    Clean a parsed string value.
    The C code treats '(null)' as an empty string for most fields.
    """
    if s == "(null)":
        return ""
    return s


def parse_player_section(reader: MudFileReader, data: PlayerData):
    """
    Parse the #PLAYERS section until 'End' is found.
    Populates data.strings, data.ints, data.arrays, data.skills,
    data.aliases, data.affects, and data.boards.
    """
    stance_low = None   # Stance (indices 0..11)
    stance_high = None  # Stance2 (indices 12..23)

    while True:
        key = reader.fread_word()
        if not key:
            break

        # End of player section
        if key == "End":
            break

        # -- String keys --
        if key in STRING_KEYS:
            val = reader.fread_string()
            data.strings[STRING_KEYS[key]] = clean_string(val)
            continue

        # -- Single int keys --
        if key in INT_KEYS:
            val = reader.fread_number()
            data.ints[INT_KEYS[key]] = val
            continue

        # -- Multi-int array keys --
        if key in ARRAY_KEYS:
            arr_name, count = ARRAY_KEYS[key]
            values = reader.fread_numbers(count)

            if key == "HpManaMove":
                # Split into individual columns: hit, max_hit, mana, max_mana, move, max_move
                data.ints["hit"] = values[0]
                data.ints["max_hit"] = values[1]
                data.ints["mana"] = values[2]
                data.ints["max_mana"] = values[3]
                data.ints["move"] = values[4]
                data.ints["max_move"] = values[5]
            elif key == "PkPdMkMd":
                # Split into individual columns: pkill, pdeath, mkill, mdeath
                data.ints["pkill"] = values[0]
                data.ints["pdeath"] = values[1]
                data.ints["mkill"] = values[2]
                data.ints["mdeath"] = values[3]
            elif key == "Stance":
                stance_low = values
            elif key == "Stance2":
                stance_high = values
            else:
                data.arrays[arr_name] = values
            continue

        # -- Skill --
        if key == "Skill":
            value = reader.fread_number()
            skill_name = reader.fread_word()  # reads 'skill name' (quoted)
            if skill_name and value > 0:
                data.skills[skill_name] = value
            continue

        # -- Alias --
        if key == "Alias":
            short_n = reader.fread_string()
            long_n = reader.fread_string()
            data.aliases.append((short_n, long_n))
            continue

        # -- AffectData (player affects have skill name) --
        if key == "AffectData":
            skill_name = reader.fread_word()  # reads 'skill name' (quoted)
            duration = reader.fread_number()
            modifier = reader.fread_number()
            location = reader.fread_number()
            bitvector = reader.fread_number()
            data.affects.append((skill_name, duration, modifier, location, bitvector))
            continue

        # -- Boards --
        if key == "Boards":
            count = reader.fread_number()
            for _ in range(count):
                board_name = reader.fread_word()
                last_note = reader.fread_number()
                data.boards.append((board_name, last_note))
            continue

        # -- Legacy/ignored string keys --
        if key in IGNORED_STRING_KEYS:
            reader.fread_string()
            continue

        # -- Legacy/ignored int keys --
        if key in IGNORED_INT_KEYS:
            reader.fread_number()
            continue

        # -- Legacy/ignored array keys --
        if key in IGNORED_ARRAY_KEYS:
            reader.fread_numbers(IGNORED_ARRAY_KEYS[key])
            continue

        # Unknown key -- log warning, try to skip safely
        log.warning("Unknown key '%s' in player section of %s (pos %d)",
                    key, reader.filepath.name, reader.pos)

    # Combine Stance and Stance2 into a single 24-element array
    if stance_low is not None or stance_high is not None:
        low = stance_low if stance_low is not None else [0] * 12
        high = stance_high if stance_high is not None else [0] * 12
        data.arrays["stance"] = low + high


def parse_object_section(reader: MudFileReader) -> dict:
    """
    Parse one #OBJECT section until 'End'.
    Returns a dict with string fields, int fields, values array,
    affects list, extra descriptions list, and a spell list.
    """
    obj = {
        "strings": {},
        "ints": {},
        "values": [0, 0, 0, 0],
        "affects": [],        # [(duration, modifier, location), ...]
        "extra_descr": [],    # [(keyword, description), ...]
        "spells": [],         # [(iValue, skill_name), ...] -- currently unused in DB
    }

    while True:
        key = reader.fread_word()
        if not key:
            break
        if key == "End":
            break

        # Object string keys
        if key in OBJ_STRING_KEYS:
            val = reader.fread_string()
            obj["strings"][OBJ_STRING_KEYS[key]] = val
            continue

        # Object single-int keys
        if key in OBJ_INT_KEYS:
            val = reader.fread_number()
            obj["ints"][OBJ_INT_KEYS[key]] = val
            continue

        # Values (4 ints)
        if key == "Values":
            obj["values"] = reader.fread_numbers(4)
            continue

        # Object AffectData (no skill name, just 3 ints: duration, modifier, location)
        if key == "AffectData":
            duration = reader.fread_number()
            modifier = reader.fread_number()
            location = reader.fread_number()
            obj["affects"].append((duration, modifier, location))
            continue

        # Extra descriptions
        if key == "ExtraDescr":
            keyword = reader.fread_string()
            description = reader.fread_string()
            obj["extra_descr"].append((keyword, description))
            continue

        # Spell (object spells -- iValue + 'skill name')
        if key == "Spell":
            ivalue = reader.fread_number()
            skill_name = reader.fread_word()  # quoted
            obj["spells"].append((ivalue, skill_name))
            continue

        log.warning("Unknown key '%s' in object section of %s (pos %d)",
                    key, reader.filepath.name, reader.pos)

    return obj


def parse_player_file(filepath: Path) -> PlayerData:
    """
    Parse a complete player text file.
    Returns a PlayerData instance with all sections populated.
    """
    reader = MudFileReader(filepath)
    data = PlayerData()

    while True:
        # Read the next section marker (starts with '#')
        ch = reader.fread_letter()
        if not ch:
            break

        if ch != "#":
            # Skip comment lines (starting with '*')
            if ch == "*":
                # Skip to end of line
                while not reader._at_end() and reader._peek() != "\n":
                    reader._advance()
                continue
            # Unexpected character, skip
            continue

        section = reader.fread_word()

        if section == "PLAYERS":
            parse_player_section(reader, data)
        elif section == "OBJECT":
            obj = parse_object_section(reader)
            data.objects.append(obj)
        elif section == "END":
            break
        else:
            log.warning("Unknown section '#%s' in %s", section, filepath.name)

    return data


# =========================================================================
# SQLite writer
# =========================================================================

def write_player_db(data: PlayerData, db_path: Path) -> dict:
    """
    Write parsed player data to a SQLite database.
    Returns a counts dict for logging.
    """
    if db_path.exists():
        db_path.unlink()

    conn = sqlite3.connect(str(db_path))
    conn.executescript(PLAYER_SCHEMA_SQL)

    counts = {
        "skills": 0,
        "aliases": 0,
        "affects": 0,
        "boards": 0,
        "objects": 0,
        "obj_affects": 0,
        "obj_extra_descr": 0,
    }

    # ------------------------------------------------------------------
    # Player table -- single row with all scalar fields
    # ------------------------------------------------------------------
    # Build column list and values dynamically from what was parsed
    columns = []
    values = []

    # String columns
    for db_col in [
        "name", "switchname", "short_descr", "long_descr", "objdesc",
        "description", "lord", "clan", "morph", "createtime", "lasttime",
        "lasthost", "poweraction", "powertype", "prompt", "cprompt",
        "password", "bamfin", "bamfout", "title", "conception", "parents",
        "cparents", "marriage", "decapmessage", "loginmessage",
        "logoutmessage", "avatarmessage", "tiemessage",
        "last_decap_0", "last_decap_1",
    ]:
        if db_col in data.strings:
            columns.append(db_col)
            values.append(data.strings[db_col])

    # Integer columns
    for db_col in [
        "sex", "class", "level", "trust", "played", "room_vnum", "gold",
        "exp", "expgained", "act", "extra", "newbits", "special",
        "affected_by", "immune", "polyaff", "itemaffect", "form",
        "position", "practice", "saving_throw", "alignment",
        "xhitroll", "xdamroll", "hitroll", "damroll", "armor", "wimpy",
        "deaf", "beast", "home", "spectype", "specpower",
        "hit", "max_hit", "mana", "max_mana", "move", "max_move",
        "pkill", "pdeath", "mkill", "mdeath", "awins", "alosses",
        "warp", "warpcount", "monkstuff", "monkcrap", "garou1", "garou2",
        "rage", "generation", "cur_form", "flag2", "flag3", "flag4",
        "siltol", "gnosis_max",
        "kingdom", "quest", "rank", "bounty", "security", "jflags", "souls",
        "upgrade_level", "mean_paradox", "relrank", "rune_count", "revision",
        "disc_research", "disc_points", "obj_vnum", "exhaustion",
        "questsrun", "questtotal",
    ]:
        if db_col in data.ints:
            columns.append(db_col)
            values.append(data.ints[db_col])

    if columns:
        placeholders = ", ".join(["?"] * len(columns))
        col_names = ", ".join(columns)
        conn.execute(
            f"INSERT INTO player ({col_names}) VALUES ({placeholders})",
            values,
        )

    # ------------------------------------------------------------------
    # Player arrays
    # ------------------------------------------------------------------
    for arr_name, arr_values in data.arrays.items():
        arr_str = " ".join(str(v) for v in arr_values)
        conn.execute(
            "INSERT INTO player_arrays (name, data) VALUES (?, ?)",
            (arr_name, arr_str),
        )

    # ------------------------------------------------------------------
    # Skills
    # ------------------------------------------------------------------
    for skill_name, value in data.skills.items():
        conn.execute(
            "INSERT INTO skills (skill_name, value) VALUES (?, ?)",
            (skill_name, value),
        )
        counts["skills"] += 1

    # ------------------------------------------------------------------
    # Aliases
    # ------------------------------------------------------------------
    for short_n, long_n in data.aliases:
        conn.execute(
            "INSERT INTO aliases (short_n, long_n) VALUES (?, ?)",
            (short_n, long_n),
        )
        counts["aliases"] += 1

    # ------------------------------------------------------------------
    # Character affects
    # ------------------------------------------------------------------
    for skill_name, duration, modifier, location, bitvector in data.affects:
        conn.execute(
            "INSERT INTO affects (skill_name, duration, modifier, location, bitvector)"
            " VALUES (?, ?, ?, ?, ?)",
            (skill_name, duration, modifier, location, bitvector),
        )
        counts["affects"] += 1

    # ------------------------------------------------------------------
    # Board timestamps
    # ------------------------------------------------------------------
    for board_name, last_note in data.boards:
        conn.execute(
            "INSERT INTO boards (board_name, last_note) VALUES (?, ?)",
            (board_name, last_note),
        )
        counts["boards"] += 1

    # ------------------------------------------------------------------
    # Objects
    # ------------------------------------------------------------------
    for obj_data in data.objects:
        obj_strs = obj_data["strings"]
        obj_ints = obj_data["ints"]
        obj_vals = obj_data["values"]

        # Build object insert
        obj_cols = []
        obj_values = []

        # Integer fields
        for db_col in [
            "nest", "vnum", "extra_flags", "extra_flags2", "weapflags",
            "wear_flags", "wear_loc", "item_type", "weight", "spectype",
            "specpower", "condition", "toughness", "resistance", "quest",
            "points", "level", "timer", "cost",
        ]:
            if db_col in obj_ints:
                obj_cols.append(db_col)
                obj_values.append(obj_ints[db_col])

        # String fields
        for db_col in [
            "name", "short_descr", "description",
            "chpoweron", "chpoweroff", "chpoweruse",
            "victpoweron", "victpoweroff", "victpoweruse",
            "questmaker", "questowner",
        ]:
            if db_col in obj_strs:
                val = obj_strs[db_col]
                # Store NULL for power strings that are "(null)" or empty
                if db_col in ("chpoweron", "chpoweroff", "chpoweruse",
                              "victpoweron", "victpoweroff", "victpoweruse"):
                    if not val or val == "(null)" or len(val) <= 1:
                        continue  # skip, will be NULL by default
                obj_cols.append(db_col)
                obj_values.append(val)

        # Values
        obj_cols.extend(["value_0", "value_1", "value_2", "value_3"])
        obj_values.extend(obj_vals)

        placeholders = ", ".join(["?"] * len(obj_cols))
        col_str = ", ".join(obj_cols)
        cursor = conn.execute(
            f"INSERT INTO objects ({col_str}) VALUES ({placeholders})",
            obj_values,
        )
        obj_id = cursor.lastrowid
        counts["objects"] += 1

        # Object affects
        for duration, modifier, location in obj_data["affects"]:
            conn.execute(
                "INSERT INTO obj_affects (obj_id, duration, modifier, location)"
                " VALUES (?, ?, ?, ?)",
                (obj_id, duration, modifier, location),
            )
            counts["obj_affects"] += 1

        # Object extra descriptions
        for keyword, description in obj_data["extra_descr"]:
            conn.execute(
                "INSERT INTO obj_extra_descr (obj_id, keyword, description)"
                " VALUES (?, ?, ?)",
                (obj_id, keyword, description),
            )
            counts["obj_extra_descr"] += 1

    # Schema version
    conn.execute(
        "INSERT OR REPLACE INTO meta (key, value) VALUES ('schema_version', '1')"
    )

    conn.commit()
    conn.close()
    return counts


# =========================================================================
# Main
# =========================================================================

def is_player_file(filepath: Path) -> bool:
    """
    Determine if a path is a valid player text file.
    Skip directories, .migrated files, and other non-player files.
    """
    if not filepath.is_file():
        return False
    name = filepath.name
    # Skip files with extensions (like .migrated, .bak, etc.)
    if "." in name:
        return False
    # Skip 'store' directory marker or other known non-player entries
    if name.lower() in ("store", "backup"):
        return False
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Migrate player text save files to SQLite databases."
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Show what would be migrated without writing any files."
    )
    parser.add_argument(
        "--player-dir", type=Path, default=None,
        help="Path to player/ directory (default: gamedata/player/)."
    )
    parser.add_argument(
        "--db-dir", type=Path, default=None,
        help="Path to db/players/ output directory (default: gamedata/db/players/)."
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true",
        help="Enable debug-level logging."
    )
    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    # Derive paths relative to repo root
    repo_root = Path(__file__).resolve().parent.parent.parent
    player_dir = args.player_dir or repo_root / "gamedata" / "player"
    db_dir = args.db_dir or repo_root / "gamedata" / "db" / "players"

    if not player_dir.exists():
        log.error("Player directory not found: %s", player_dir)
        sys.exit(1)

    if not args.dry_run:
        db_dir.mkdir(parents=True, exist_ok=True)

    # Collect player files
    player_files = sorted(
        f for f in player_dir.iterdir() if is_player_file(f)
    )

    if not player_files:
        log.info("No player files found in %s", player_dir)
        return

    log.info("Migrating %d player file(s) from %s", len(player_files), player_dir)
    log.info("Output directory: %s", db_dir)
    if args.dry_run:
        log.info("DRY RUN -- no files will be written or renamed.")
    print()

    total = 0
    errors = []

    for filepath in player_files:
        name = filepath.name
        db_path = db_dir / f"{name}.db"

        try:
            data = parse_player_file(filepath)

            # Determine the player name for logging
            pname = data.strings.get("name", name)

            if args.dry_run:
                parts = []
                if data.skills:
                    parts.append(f"{len(data.skills)}sk")
                if data.objects:
                    parts.append(f"{len(data.objects)}obj")
                if data.aliases:
                    parts.append(f"{len(data.aliases)}ali")
                if data.affects:
                    parts.append(f"{len(data.affects)}aff")
                if data.boards:
                    parts.append(f"{len(data.boards)}bd")
                summary = " ".join(parts) if parts else "basic"
                print(f"  WOULD  {name:20s} -> {name}.db  [{summary}]")
            else:
                counts = write_player_db(data, db_path)

                # Build summary
                parts = []
                if counts["skills"]:
                    parts.append(f"{counts['skills']}sk")
                if counts["objects"]:
                    parts.append(f"{counts['objects']}obj")
                if counts["aliases"]:
                    parts.append(f"{counts['aliases']}ali")
                if counts["affects"]:
                    parts.append(f"{counts['affects']}aff")
                if counts["boards"]:
                    parts.append(f"{counts['boards']}bd")
                if counts["obj_affects"]:
                    parts.append(f"{counts['obj_affects']}oa")
                summary = " ".join(parts) if parts else "basic"
                print(f"  OK     {name:20s} -> {name}.db  [{summary}]")

                # Rename original to .migrated
                migrated_path = filepath.with_suffix(".migrated")
                filepath.rename(migrated_path)
                log.debug("Renamed %s -> %s", filepath.name, migrated_path.name)

            total += 1

        except Exception as e:
            errors.append(f"{name}: {e}")
            print(f"  FAIL   {name}: {e}")
            log.debug("Exception details:", exc_info=True)

    print()
    print(f"Migration complete: {total} player(s) processed.")
    if errors:
        print(f"\n{len(errors)} error(s):")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
