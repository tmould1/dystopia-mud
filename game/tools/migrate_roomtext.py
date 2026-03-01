#!/usr/bin/env python3
"""
migrate_roomtext.py — Convert RoomText entries to Lua scripts.

Reads roomtext data from area SQLite databases, inserts equivalent
Lua scripts into the scripts table, then deletes the old room_texts.

Usage:
    python migrate_roomtext.py [--dry-run]
"""

import sqlite3
import os
import sys

AREA_DIR = os.path.join(os.path.dirname(__file__),
    '..', '..', 'gamedata', 'db', 'areas')

# Trigger bitmask values (must match script.h)
TRIG_GREET  = 1  # (1 << 0)
TRIG_SPEECH = 2  # (1 << 1)

# ============================================================================
# Lua scripts — one per room, hand-crafted from roomtext data
# ============================================================================

# --- school.db room 3700: Spirit guide ---
SCHOOL_3700 = r'''
-- Spirit guide in the MUD School entrance (room 3700, mob 3721)
-- Converted from RoomText: greets players, gives directions and equipment.

local ITEMS = {
    breastplate = 30333,
    helmet      = 30334,
    sleeves     = 30335,
    leggings    = 30336,
    gauntlets   = 30337,
    boots       = 30338,
    bracer      = 30339,
    longsword   = 30340,
    buckler     = 30341,
    ring        = 30342,
    collar      = 30343,
    backpack    = 1345,
    axe         = 3005,
    banner      = 3716,
}

function on_enter(ch, room)
    local spirit = room:find_mob(3721)
    if not spirit then return end
    spirit:say("Welcome to Dystopia, " .. ch:name() .. "!")
    spirit:say("I can give you EQUIPMENT for your journey or tell you about AREAS to explore.")
    spirit:say("Go NORTH to learn basic MUD commands, or EAST if you already know the basics.")
end

function on_say(ch, room, text)
    local spirit = room:find_mob(3721)
    if not spirit then return end

    -- Greetings
    if text:find("hello") or text:find("hi ") then
        spirit:say("Hello there " .. ch:name() .. ", can I be of service?")
        return
    end

    if text:find("thank") and text:find("spirit") then
        spirit:say("You are welcome, " .. ch:name() .. ".")
        return
    end

    if text:find("yes") and not text:find("vest") then
        spirit:say("Sorry " .. ch:name() .. " but you'll need to be more specific.")
        return
    end

    if text:find("help") then
        spirit:say("I can help you with EQUIPMENT for your journey, or give DIRECTIONS to beginner areas!")
        spirit:say("Go NORTH for Section 1 (basic MUD commands) or EAST for Section 2 (Dystopia systems).")
        spirit:say('Try asking: "equipment" or "areas" or "where should I go?"')
        return
    end

    if text:find("where") and text:find("go") then
        spirit:say("If you are new to MUDs, go NORTH for Section 1 to learn the basics.")
        spirit:say("If you already know MUD commands, go EAST for Section 2 to learn Dystopia systems.")
        spirit:say("Or go SOUTH to the Combat Arena to practice fighting!")
        return
    end

    if text:find("what") and text:find("do") then
        spirit:say("Go NORTH for basic commands, EAST for Dystopia systems, or SOUTH to fight!")
        return
    end

    -- Equipment list
    if text:find("equipment") or text:find("gear") then
        spirit:say("I can conjure these items for you, " .. ch:name() .. ":")
        spirit:say("Armor: BREASTPLATE, HELMET, SLEEVES, LEGGINGS, GAUNTLETS, BOOTS")
        spirit:say("Accessories: BRACER, BUCKLER, RING, COLLAR, BACKPACK")
        spirit:say("Weapons: LONGSWORD, AXE, WAR BANNER")
        spirit:say("Just say the item name and I will give it to you!")
        return
    end

    -- Item dispensing
    for keyword, vnum in pairs(ITEMS) do
        if text:find(keyword) then
            local obj = game.create_object(vnum)
            obj:to_char(ch)
            spirit:say("Here you go " .. ch:name() .. ", maybe this " .. keyword .. " will help!")
            spirit:say("Ask for more EQUIPMENT if you need it!")
            return
        end
    end

    -- Area directions
    if text:find("areas") or text:find("destinations") or text:find("where should") then
        spirit:say("I know directions to these beginner areas, " .. ch:name() .. ":")
        spirit:say("The ARENA - just 1 south from here, practice combat safely!")
        spirit:say("The SMURF village - easy creatures for new adventurers")
        spirit:say("The GNOME village - good for learning combat")
        spirit:say("The SHIRE - peaceful hobbits to practice on")
        spirit:say("The DWARVEN drop-off centre - useful supplies")
        spirit:say("Just ask about any area and I will give you directions!")
        spirit:say("For harder areas: ELEMENTAL CANYON or BLACK DRAGON lair.")
        spirit:say("The EXECUTIONER in the Temple knows even more dangerous places!")
        return
    end

    if text:find("arena") then
        spirit:say("The newbie arena is just 1 south from here, " .. ch:name() .. "!")
        spirit:say("You can practice combat safely there.")
        spirit:say("I also know directions to: SMURF village, GNOME village, SHIRE, DWARVEN drop-off.")
        return
    end

    if text:find("smurf") then
        spirit:say("To get to the smurf village from recall,")
        spirit:say("Go 2 south, 3 west, and keep going north!")
        spirit:say("I also know directions to: ARENA, GNOME village, SHIRE, DWARVEN drop-off.")
        return
    end

    if text:find("gnome") then
        spirit:say("To get to the gnome village from recall,")
        spirit:say("Go 2 south, 5 east, and then go south.")
        spirit:say("I also know directions to: ARENA, SMURF village, SHIRE, DWARVEN drop-off.")
        return
    end

    if text:find("shire") then
        spirit:say("To get to the shire from recall,")
        spirit:say("Go 2 south, 5 west, and then keep going north.")
        spirit:say("I also know directions to: ARENA, SMURF village, GNOME village, DWARVEN drop-off.")
        return
    end

    if text:find("dwarven") or text:find("drop") then
        spirit:say("To get to the dwarven drop-off centre from recall,")
        spirit:say("Go 2 south, 6 east, 1 down, then go south.")
        spirit:say("I also know directions to: ARENA, SMURF village, GNOME village, SHIRE.")
        return
    end

    if text:find("canyon") or text:find("elemental") then
        spirit:say("To get to Elemental Canyon from recall,")
        spirit:say("Go 2 south, 6 east, 4 south, 2 east, south, 2 east, down, south.")
        spirit:say("For harder areas, ask the EXECUTIONER in the Temple below.")
        return
    end

    if text:find("dragon") or text:find("lair") or text:find("black") then
        spirit:say("To get to the Black Dragon Lair from recall,")
        spirit:say("Go 2 south, 6 east, 2 south, and then down.")
        spirit:say("For other challenging areas, ask the EXECUTIONER in the Temple.")
        return
    end
end
'''

# --- school.db room 3718: Adept gives lantern ---
SCHOOL_3718 = r'''
-- Adept in the shop (room 3718, mob 3717)
-- Gives a lantern to anyone who enters.

function on_enter(ch, room)
    local adept = room:find_mob(3717)
    if not adept then return end
    local obj = game.create_object(3720)
    obj:to_char(ch)
    adept:say("Ah, welcome " .. ch:name() .. "! Sorry, the shop is closed for renovations.")
    adept:say("But if you need a lantern for the dark room ahead, I have one to spare!")
end
'''

# --- midgaard.db room 3001: Executioner ---
MIDGAARD_3001 = r'''
-- Executioner in the Temple of Midgaard (room 3001, mob 3011)
-- Greets players, gives area directions and PK rules.

function on_enter(ch, room)
    local exec = room:find_mob(3011)
    if not exec then return end
    exec:emote("glances at you with cold eyes.")
    exec:say("Welcome to the Temple, mortal. Behave yourself here.")
    exec:say("I enforce the law. Those who grief newbies or break PK rules face my axe.")
    exec:say("If you seek adventure, ask me about AREAS beyond the mud school.")
end

function on_say(ch, room, text)
    local exec = room:find_mob(3011)
    if not exec then return end

    if text:find("areas") or text:find("adventure") then
        exec:say("Looking for a challenge? I know of several dangerous places:")
        exec:say("The CANYON of elementals to the east, the black DRAGON lair underground,")
        exec:say("The mines of MORIA, the towers of DRACONIA, or the ruins of THALOS.")
        exec:say("Ask me about any of these if you dare.")
        return
    end

    if text:find("canyon") or text:find("elemental") then
        exec:say("To reach Elemental Canyon from here,")
        exec:say("Go 2 south, 6 east, 4 south, 2 east, south, 2 east, down, south.")
        exec:say("The elementals await those brave enough to face them.")
        return
    end

    if text:find("dragon") or text:find("lair") or text:find("black") then
        exec:say("The Black Dragon Lair is a dangerous place for the bold.")
        exec:say("From here, go 2 south, 6 east, 2 south, and then down.")
        exec:say("You will find the entrance to the dragon domain.")
        return
    end

    if text:find("moria") or text:find("mines") then
        exec:say("The Mines of Moria are east of the city.")
        exec:say("Exit through the East Gate and follow the trail north around the walls.")
        exec:say("You will reach the hills where the mine entrance awaits.")
        return
    end

    if text:find("draconia") or text:find("tower") then
        exec:say("Draconia lies to the west, through the forest.")
        exec:say("Exit through the West Gate and travel through the woods.")
        exec:say("The dragon towers will become visible as you approach.")
        return
    end

    if text:find("thalos") or text:find("ruins") then
        exec:say("Thalos is a ruined city far to the east.")
        exec:say("Travel east across the plains, then through the Dwarf Forest.")
        exec:say("The ancient gates of Thalos await at the forest end.")
        return
    end

    if text:find("pk") or text:find("rules") or text:find("kill") then
        exec:say("The rules are simple: newbies are protected for their first 4 hours.")
        exec:say("Attack them during that time, and you answer to ME.")
        exec:say("Read HELP RULES and HELP POLICY if you value your head.")
        return
    end
end
'''

# --- quest.db rooms 29500-29502: Spell rooms ---
QUEST_29500 = r'''
-- Quest room: restore health (room 29500)
-- Say "restore health" to receive a healing spell.

function on_say(ch, room, text)
    if text:find("restore") and text:find("health") then
        game.cast_spell(43, 25, ch)
        room:echo("The room pulses with green energy.\n")
    end
end
'''

QUEST_29501 = r'''
-- Quest room: restore mana (room 29501)
-- Say "restore mana" to receive a mana restoration spell.

function on_say(ch, room, text)
    if text:find("restore") and text:find("mana") then
        game.cast_spell(90, 25, ch)
        room:echo("The room pulses with blue energy.\n")
    end
end
'''

QUEST_29502 = r'''
-- Quest room: restore move (room 29502)
-- Say "restore move" to receive a movement restoration spell.

function on_say(ch, room, text)
    if text:find("restore") and text:find("move") then
        game.cast_spell(55, 25, ch)
        room:echo("The room pulses with red energy.\n")
    end
end
'''

# --- Vallanda.db room 30328: Guardian riddle ---
VALLANDA_30328 = r'''
-- Guardian of Vallanda (room 30328, mob 30330)
-- Recites a riddle on entry. Answer "swarm bees" to open a portal.

function on_enter(ch, room)
    local guardian = room:find_mob(30330)
    if not guardian then return end
    guardian:say("I watched an army gathering supplies,")
    guardian:say("Over miles of countryside,")
    guardian:say("No homestead did they pillage,")
    guardian:say("No blade of grass was broken.")
    guardian:emote("grins evilly.")
    guardian:say("What was this army?")
end

function on_say(ch, room, text)
    local guardian = room:find_mob(30330)
    if not guardian then return end

    if text:find("swarm") and text:find("bees") then
        guardian:say("Well done, you have solved my puzzle!")
        guardian:emote("makes a few gestures and the gateway shimmers with energy.")
        game.create_portal(12, room)
        return
    end

    if text:find("bees") then
        guardian:say("Nearly...but a 'what' of bees?")
        return
    end

    if text:find("swarm") then
        guardian:say("Close, but I need to know what the swarm was of...")
        return
    end

    if text:find("insects") or text:find("ants") then
        guardian:say("Not quite, but you're on the right track...")
        return
    end
end
'''

# ============================================================================
# Migration mapping: (db_file, room_vnum, trigger_bits, script_name, lua_code)
# ============================================================================

MIGRATIONS = [
    ('school.db',   3700,  TRIG_GREET | TRIG_SPEECH, 'school_spirit',      SCHOOL_3700),
    ('school.db',   3718,  TRIG_GREET,                'school_adept',       SCHOOL_3718),
    ('midgaard.db', 3001,  TRIG_GREET | TRIG_SPEECH, 'temple_executioner', MIDGAARD_3001),
    ('quest.db',    29500, TRIG_SPEECH,               'quest_health',       QUEST_29500),
    ('quest.db',    29501, TRIG_SPEECH,               'quest_mana',         QUEST_29501),
    ('quest.db',    29502, TRIG_SPEECH,               'quest_move',         QUEST_29502),
    ('Vallanda.db', 30328, TRIG_GREET | TRIG_SPEECH, 'vallanda_guardian',  VALLANDA_30328),
]


def migrate(dry_run=False):
    """Run the migration."""
    area_dir = os.path.abspath(AREA_DIR)
    if not os.path.isdir(area_dir):
        print(f"ERROR: Area directory not found: {area_dir}")
        sys.exit(1)

    # Group migrations by database file
    by_db = {}
    for db_file, vnum, trigger, name, code in MIGRATIONS:
        by_db.setdefault(db_file, []).append((vnum, trigger, name, code))

    for db_file, entries in by_db.items():
        db_path = os.path.join(area_dir, db_file)
        if not os.path.isfile(db_path):
            print(f"WARNING: Database not found: {db_path}")
            continue

        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        # Count existing roomtext entries
        rt_count = cursor.execute("SELECT COUNT(*) FROM room_texts").fetchone()[0]

        print(f"\n=== {db_file} ({rt_count} roomtext entries) ===")

        for vnum, trigger, name, code in entries:
            # Check if script already exists
            existing = cursor.execute(
                "SELECT COUNT(*) FROM scripts WHERE owner_type='room' AND owner_vnum=?",
                (vnum,)
            ).fetchone()[0]

            if existing > 0:
                print(f"  Room {vnum}: SKIP (script already exists)")
                continue

            print(f"  Room {vnum}: INSERT script '{name}' "
                  f"(trigger={trigger}, {len(code)} bytes)")

            if not dry_run:
                cursor.execute(
                    "INSERT INTO scripts "
                    "(owner_type, owner_vnum, trigger, name, code, pattern, chance, sort_order) "
                    "VALUES (?, ?, ?, ?, ?, NULL, 0, 0)",
                    ('room', vnum, trigger, name, code)
                )

        # Delete roomtext entries for migrated rooms
        migrated_vnums = [vnum for vnum, _, _, _ in entries]
        for vnum in migrated_vnums:
            count = cursor.execute(
                "SELECT COUNT(*) FROM room_texts WHERE room_vnum=?", (vnum,)
            ).fetchone()[0]
            if count > 0:
                print(f"  Room {vnum}: DELETE {count} roomtext entries")
                if not dry_run:
                    cursor.execute(
                        "DELETE FROM room_texts WHERE room_vnum=?", (vnum,)
                    )

        if not dry_run:
            conn.commit()
            remaining = cursor.execute(
                "SELECT COUNT(*) FROM room_texts"
            ).fetchone()[0]
            print(f"  Remaining roomtext entries: {remaining}")

        conn.close()

    if dry_run:
        print("\n[DRY RUN] No changes were made.")
    else:
        print("\nMigration complete.")


if __name__ == '__main__':
    dry_run = '--dry-run' in sys.argv
    migrate(dry_run=dry_run)
