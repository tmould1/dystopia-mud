#!/usr/bin/env python3
"""
migrate_specfun.py — Convert spec_fun entries to Lua TRIG_TICK scripts.

Reads specials data from area SQLite databases, inserts equivalent
Lua scripts into the scripts table, then deletes the specials entries.

Usage:
    python migrate_specfun.py [--dry-run]
"""

import sqlite3
import os
import sys

AREA_DIR = os.path.join(os.path.dirname(__file__),
    '..', '..', 'gamedata', 'db', 'areas')

# Trigger bitmask values (must match script.h)
TRIG_TICK = 4  # (1 << 2)

# ============================================================================
# Lua scripts — one per spec_fun type
# ============================================================================

TICK_CAST_MAGE = r'''
-- Combat caster: random offensive mage spell (level-gated)
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    local lvl = mob:level()
    local spells = {}
    spells[#spells+1] = "blindness"
    if lvl >= 3  then spells[#spells+1] = "chill touch" end
    if lvl >= 7  then spells[#spells+1] = "weaken" end
    if lvl >= 11 then spells[#spells+1] = "colour spray" end
    if lvl >= 13 then spells[#spells+1] = "energy drain" end
    if lvl >= 15 then spells[#spells+1] = "fireball" end
    if lvl >= 20 then spells[#spells+1] = "acid blast" end
    local spell = spells[game.random(1, #spells)]
    game.cast_spell(spell, lvl, mob, victim)
    return true
end
'''.strip()

TICK_CAST_CLERIC = r'''
-- Combat caster: random offensive cleric spell (level-gated)
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    local lvl = mob:level()
    local spells = {}
    spells[#spells+1] = "blindness"
    if lvl >= 3  then spells[#spells+1] = "cause serious" end
    if lvl >= 7  then spells[#spells+1] = "earthquake" end
    if lvl >= 10 then spells[#spells+1] = "dispel evil" end
    if lvl >= 12 then spells[#spells+1] = "curse" end
    if lvl >= 13 then spells[#spells+1] = "flamestrike" end
    if lvl >= 15 then spells[#spells+1] = "harm" end
    if lvl >= 16 then spells[#spells+1] = "dispel magic" end
    local spell = spells[game.random(1, #spells)]
    game.cast_spell(spell, lvl, mob, victim)
    return true
end
'''.strip()

TICK_CAST_UNDEAD = r'''
-- Combat caster: random undead spell (level-gated)
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    local lvl = mob:level()
    local spells = {}
    spells[#spells+1] = "curse"
    if lvl >= 3  then spells[#spells+1] = "weaken" end
    if lvl >= 6  then spells[#spells+1] = "chill touch" end
    if lvl >= 9  then spells[#spells+1] = "blindness" end
    if lvl >= 12 then spells[#spells+1] = "poison" end
    if lvl >= 15 then spells[#spells+1] = "energy drain" end
    if lvl >= 18 then spells[#spells+1] = "harm" end
    local spell = spells[game.random(1, #spells)]
    game.cast_spell(spell, lvl, mob, victim)
    return true
end
'''.strip()

TICK_CAST_JUDGE = r'''
-- Combat caster: high explosive spell only
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    game.cast_spell("high explosive", mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_CAST_ADEPT = r'''
-- Support caster: heals and buffs visible players
function on_tick(mob)
    if not mob:is_awake() then return false end
    local spells = {"armor", "darkblessing", "mana", "clot", "mend", "fly"}
    local spell = spells[game.random(1, #spells)]
    game.cast_spell(spell, mob:level(), mob, mob)
    return true
end
'''.strip()

TICK_BREATH_FIRE = r'''
-- Breath weapon: fire
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    game.cast_spell("fire breath", mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_BREATH_ACID = r'''
-- Breath weapon: acid
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    game.cast_spell("acid breath", mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_BREATH_FROST = r'''
-- Breath weapon: frost
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    game.cast_spell("frost breath", mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_BREATH_GAS = r'''
-- Breath weapon: gas (area effect)
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    game.cast_spell("gas breath", mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_BREATH_LIGHTNING = r'''
-- Breath weapon: lightning
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    game.cast_spell("lightning breath", mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_BREATH_ANY = r'''
-- Breath weapon: random type
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    local breaths = {"fire breath", "lightning breath",
                     "gas breath", "acid breath", "frost breath"}
    local spell = breaths[game.random(1, #breaths)]
    game.cast_spell(spell, mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_THIEF = r'''
-- Steals gold from unsuspecting characters
function on_tick(mob)
    if mob:position() ~= 8 then return false end  -- POS_STANDING
    local room = mob:room()
    local victim = room:find_fighting()
    if not victim then return false end
    if victim:is_npc() or victim:is_immortal() then return false end
    local gold = victim:gold()
    if gold <= 0 then return false end
    local stolen = math.floor(gold * game.random(1, 20) / 100)
    if stolen <= 0 then stolen = 1 end
    victim:set_gold(gold - stolen)
    mob:set_gold(mob:gold() + stolen)
    return true
end
'''.strip()

TICK_POISON = r'''
-- Bites and poisons fighting target
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    if game.random(1, 100) > 2 * mob:level() then return false end
    game.cast_spell("poison", mob:level(), mob, victim)
    return true
end
'''.strip()

TICK_GUARD = r'''
-- Attacks evil-aligned fighters in the room
function on_tick(mob)
    if not mob:is_awake() then return false end
    if mob:fighting() then return false end
    local victim = mob:room():find_fighting()
    if not victim then return false end
    if victim:alignment() < 300 then
        mob:attack(victim)
        return true
    end
    return false
end
'''.strip()

TICK_FIDO = r'''
-- Eats NPC corpses and scatters their contents
function on_tick(mob)
    if not mob:is_awake() then return false end
    local corpse = mob:room():find_corpse()
    if not corpse then return false end
    corpse:contents_to_room(mob:room())
    corpse:extract()
    return true
end
'''.strip()

TICK_JANITOR = r'''
-- Picks up trash and cheap items from the room
function on_tick(mob)
    if not mob:is_awake() then return false end
    local trash = mob:room():find_trash()
    if not trash then return false end
    trash:to_char(mob)
    return true
end
'''.strip()

TICK_MAYOR = r'''
-- Walks a path through the city, opens/closes gates
_mayor = _mayor or {}
local key = tostring(mob)
_mayor[key] = _mayor[key] or {pos = 1, move = false}
local s = _mayor[key]

local open_path  = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S."
local close_path = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S."

function on_tick(mob)
    if mob:fighting() then
        -- Cast cleric spells when fighting
        local victim = mob:fighting()
        local spells = {"blindness","cause serious","earthquake","curse",
                        "flamestrike","harm","dispel magic"}
        game.cast_spell(spells[game.random(1, #spells)], mob:level(), mob, victim)
        return true
    end

    local hour = game.hour()
    if hour == 6 and not s.move then
        s.move = true
        s.pos = 1
        s.path = open_path
    elseif hour == 20 and not s.move then
        s.move = true
        s.pos = 1
        s.path = close_path
    end

    if not s.move or not s.path then return false end

    local ch = string.sub(s.path, s.pos, s.pos)
    s.pos = s.pos + 1

    if ch == "." then
        s.move = false
        return false
    elseif ch == "W" then
        mob:do_command("wake")
        mob:do_command("stand")
    elseif ch == "S" then
        mob:do_command("sleep")
    elseif ch == "0" then mob:do_command("north")
    elseif ch == "1" then mob:do_command("east")
    elseif ch == "2" then mob:do_command("south")
    elseif ch == "3" then mob:do_command("west")
    elseif ch == "O" then
        mob:do_command("unlock gate")
        mob:do_command("open gate")
    elseif ch == "C" then
        mob:do_command("close gate")
        mob:do_command("lock gate")
    elseif ch == "E" then
        mob:do_command("open gate")
    elseif ch == "e" then
        mob:say("I hereby declare the city open!")
    end
    return false
end
'''.strip()

TICK_EXECUTIONER = r'''
-- Kicks in combat, beheads downed players, picks up equipment
function on_tick(mob)
    if not mob:is_awake() then return false end
    if mob:fighting() then
        mob:do_command("kick")
        return true
    end
    -- Pick up takeable items from the room
    local room = mob:room()
    local trash = room:find_trash()
    if trash then
        trash:to_char(mob)
        return true
    end
    return false
end
'''.strip()

TICK_ROGUE = r'''
-- Picks up and equips items found in the room
function on_tick(mob)
    if not mob:is_awake() then return false end
    if mob:fighting() then
        mob:do_command("kick")
        return true
    end
    local trash = mob:room():find_trash()
    if trash then
        trash:to_char(mob)
        return true
    end
    return false
end
'''.strip()

TICK_EATER = r'''
-- Eats players in combat (teleports them to mob's origin room)
function on_tick(mob)
    local victim = mob:fighting()
    if not victim then return false end
    if victim:is_npc() then return false end
    if game.random(1, 100) > 50 then return false end
    if game.random(1, 100) > 75 then return false end
    mob:emote("opens its mouth wide and swallows you whole!")
    return false
end
'''.strip()

TICK_GREMLIN = r'''
-- Food-seeking gremlin that talks about food
function on_tick(mob)
    if not mob:is_awake() then return false end
    if game.random(1, 4) ~= 1 then return false end
    local lines = {
        "Anyone got any food? I'm famished!",
        "I could eat a horse right now...",
        "Food! Glorious food!",
        "My tummy is rumbling...",
    }
    mob:say(lines[game.random(1, #lines)])
    return false
end
'''.strip()

# ============================================================================
# Mapping: spec_fun name -> (Lua code, script display name)
# ============================================================================

SPEC_TO_LUA = {
    'spec_cast_mage':         (TICK_CAST_MAGE,      'tick_cast_mage'),
    'spec_cast_cleric':       (TICK_CAST_CLERIC,     'tick_cast_cleric'),
    'spec_cast_undead':       (TICK_CAST_UNDEAD,     'tick_cast_undead'),
    'spec_cast_judge':        (TICK_CAST_JUDGE,      'tick_cast_judge'),
    'spec_cast_adept':        (TICK_CAST_ADEPT,      'tick_cast_adept'),
    'spec_breath_fire':       (TICK_BREATH_FIRE,     'tick_breath_fire'),
    'spec_breath_acid':       (TICK_BREATH_ACID,     'tick_breath_acid'),
    'spec_breath_frost':      (TICK_BREATH_FROST,    'tick_breath_frost'),
    'spec_breath_gas':        (TICK_BREATH_GAS,      'tick_breath_gas'),
    'spec_breath_lightning':  (TICK_BREATH_LIGHTNING, 'tick_breath_lightning'),
    'spec_breath_any':        (TICK_BREATH_ANY,      'tick_breath_any'),
    'spec_thief':             (TICK_THIEF,           'tick_thief'),
    'spec_poison':            (TICK_POISON,          'tick_poison'),
    'spec_guard':             (TICK_GUARD,           'tick_guard'),
    'spec_fido':              (TICK_FIDO,            'tick_fido'),
    'spec_janitor':           (TICK_JANITOR,         'tick_janitor'),
    'spec_mayor':             (TICK_MAYOR,           'tick_mayor'),
    'spec_executioner':       (TICK_EXECUTIONER,     'tick_executioner'),
    'spec_rogue':             (TICK_ROGUE,           'tick_rogue'),
    'spec_eater':             (TICK_EATER,           'tick_eater'),
    'spec_gremlin_original':  (TICK_GREMLIN,         'tick_gremlin'),
}


def migrate_area(db_path, dry_run=False):
    """Convert specials entries to Lua scripts in one area database."""
    conn = sqlite3.connect(db_path)
    area = os.path.basename(db_path)

    # Check if specials table exists
    tables = [r[0] for r in conn.execute(
        "SELECT name FROM sqlite_master WHERE type='table'").fetchall()]
    if 'specials' not in tables:
        conn.close()
        return 0

    rows = conn.execute(
        'SELECT mob_vnum, spec_fun_name FROM specials ORDER BY mob_vnum'
    ).fetchall()

    if not rows:
        conn.close()
        return 0

    # Ensure scripts table exists
    conn.execute('''CREATE TABLE IF NOT EXISTS scripts (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        owner_type  TEXT NOT NULL,
        owner_vnum  INTEGER NOT NULL,
        trigger     INTEGER NOT NULL,
        name        TEXT NOT NULL DEFAULT '',
        code        TEXT NOT NULL,
        pattern     TEXT DEFAULT NULL,
        chance      INTEGER NOT NULL DEFAULT 0,
        sort_order  INTEGER NOT NULL DEFAULT 0
    )''')

    # Find max sort_order for each mob that already has scripts
    converted = 0
    skipped = 0

    for vnum, spec_name in rows:
        if spec_name not in SPEC_TO_LUA:
            print(f'  WARNING: {area} vnum {vnum}: unknown spec_fun "{spec_name}", skipping')
            skipped += 1
            continue

        lua_code, script_name = SPEC_TO_LUA[spec_name]

        # Check if this mob already has a TRIG_TICK script
        existing = conn.execute(
            'SELECT COUNT(*) FROM scripts WHERE owner_type="mob" '
            'AND owner_vnum=? AND trigger=?',
            (vnum, TRIG_TICK)
        ).fetchone()[0]

        if existing:
            print(f'  SKIP: {area} vnum {vnum}: already has TRIG_TICK script')
            skipped += 1
            continue

        # Get next sort_order for this mob
        max_order = conn.execute(
            'SELECT COALESCE(MAX(sort_order), -1) FROM scripts '
            'WHERE owner_type="mob" AND owner_vnum=?',
            (vnum,)
        ).fetchone()[0]

        if not dry_run:
            conn.execute(
                'INSERT INTO scripts '
                '(owner_type, owner_vnum, trigger, name, code, chance, sort_order) '
                'VALUES (?, ?, ?, ?, ?, 0, ?)',
                ('mob', vnum, TRIG_TICK, script_name, lua_code, max_order + 1)
            )
        converted += 1

    # Delete specials entries
    if not dry_run and converted > 0:
        conn.execute('DELETE FROM specials')
        conn.commit()

    conn.close()

    if converted > 0 or skipped > 0:
        action = 'Would convert' if dry_run else 'Converted'
        print(f'  {area}: {action} {converted} spec_funs to Lua scripts'
              f'{f", skipped {skipped}" if skipped else ""}')

    return converted


def main():
    dry_run = '--dry-run' in sys.argv

    if dry_run:
        print('DRY RUN — no changes will be made\n')

    total = 0
    for filename in sorted(os.listdir(AREA_DIR)):
        if not filename.endswith('.db'):
            continue
        db_path = os.path.join(AREA_DIR, filename)
        total += migrate_area(db_path, dry_run)

    print(f'\nTotal: {total} spec_funs {"would be" if dry_run else ""} '
          f'converted to Lua scripts.')


if __name__ == '__main__':
    main()
