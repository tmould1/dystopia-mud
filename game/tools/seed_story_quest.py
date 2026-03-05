#!/usr/bin/env python3
"""
seed_story_quest.py - Insert story quest data into area databases.

"Echoes of the Sundering" — a hub-and-spoke narrative quest.
Players visit 16 areas, completing local tasks (kill, fetch, examine) at
each hub before being sent to the next.

Usage:
    python seed_story_quest.py [--db-dir PATH]

Default db-dir: gamedata/db/areas/
"""

import argparse
import os
import sqlite3
import sys

# Trigger bitmasks
TRIG_SPEECH  = 2   # (1 << 1)
TRIG_DEATH   = 16  # (1 << 4)
TRIG_EXAMINE = 32  # (1 << 5)

# Collectors for each type of data to seed
SPEECH_NODES = []
DEATH_NODES = []
EXAMINE_NODES = []
EXTRA_DESCS = []
OBJECTS = []
RESETS = []


# ============================================================
# Helpers
# ============================================================

def _esc(s):
    """Escape double quotes for embedding in Lua string literals."""
    return s.replace('"', '\\"')


def _dia(lines):
    """Join dialogue lines with MUD newlines for Lua."""
    return "\\n\\r".join(lines)


def speech_node(area, vnum, pattern, name, lua_code, sort_order=99):
    """Register a TRIG_SPEECH script on a mob."""
    SPEECH_NODES.append({
        "area": area, "vnum": vnum, "pattern": pattern,
        "lua_code": lua_code, "name": name, "sort_order": sort_order,
    })


def talk(area, vnum, pattern, req_node, next_node, label, clue, lines):
    """Simple talk-through — deliver dialogue and advance quest."""
    lua_code = f'''function on_speech(mob, ch, text)
  if ch:is_npc() then return end
  if ch:story_node() ~= {req_node} then return end
  ch:send("\\n\\r{_esc(_dia(lines))}\\n\\r")
  ch:set_story_node({next_node})
  ch:set_story_clue("{_esc(clue)}")
  ch:set_story_progress(0)
end'''
    speech_node(area, vnum, pattern,
                f"story_node_{req_node}_{label}", lua_code)


def hub_intro(area, vnum, pattern, hub_node, label, clue, intro_lines,
              partial_lines=None):
    """Hub intro script — delivers task list on first visit, reminder on partial."""
    partial = partial_lines or ["#C\\\"You haven't finished what I asked. Get to it.\\\"#n"]
    lua_code = f'''function on_speech(mob, ch, text)
  if ch:is_npc() then return end
  if ch:story_node() ~= {hub_node} then return end
  local p = ch:story_progress()
  if p == 0 then
    ch:send("\\n\\r{_esc(_dia(intro_lines))}\\n\\r")
    ch:set_story_clue("{_esc(clue)}")
    ch:set_story_progress(0)
  else
    ch:send("\\n\\r{_esc(_dia(partial))}\\n\\r")
  end
end'''
    speech_node(area, vnum, pattern,
                f"story_hub_{hub_node}_{label}_intro", lua_code,
                sort_order=98)


def hub_return(area, vnum, pattern, hub_node, next_node, label, clue,
               required_bits, obj_vnum=None, missing_lines=None,
               done_lines=None):
    """Hub return script — checks all bits, takes fetch obj, resets, advances."""
    if done_lines is None:
        done_lines = ["#C\\\"Well done. Move on.\\\"#n"]

    # Build bit-check conditions
    checks = " and ".join(
        f"ch:story_has_task({bit})" for bit in required_bits)

    # Build fetch-object handling
    if obj_vnum and missing_lines:
        fetch_check = f'''
  if not ch:has_object({obj_vnum}) then
    ch:send("\\n\\r{_esc(_dia(missing_lines))}\\n\\r")
    return
  end
  ch:take_object({obj_vnum})'''
    else:
        fetch_check = ""

    lua_code = f'''function on_speech(mob, ch, text)
  if ch:is_npc() then return end
  if ch:story_node() ~= {hub_node} then return end
  if not ({checks}) then return end{fetch_check}
  ch:send("\\n\\r{_esc(_dia(done_lines))}\\n\\r")
  ch:set_story_node({next_node})
  ch:set_story_clue("{_esc(clue)}")
  ch:set_story_progress(0)
end'''
    speech_node(area, vnum, pattern,
                f"story_hub_{hub_node}_{label}_return", lua_code,
                sort_order=99)


def hub_kill(area, mob_vnum, hub_node, task_bit, threshold, label,
             progress_msg, complete_msg):
    """TRIG_DEATH — increments kill counter, sets task bit at threshold."""
    lua_code = f'''function on_death(killer, mob_vnum, area_low, area_high)
  if killer:is_npc() then return end
  if killer:story_node() ~= {hub_node} then return end
  if killer:story_has_task({task_bit}) then return end
  local kills = killer:add_story_kill()
  if kills >= {threshold} then
    killer:set_story_task({task_bit})
    killer:send("\\n\\r{_esc(complete_msg)}\\n\\r")
  else
    killer:send("\\n\\r{_esc(progress_msg)}" .. kills .. "/{threshold}]#n\\n\\r")
  end
end'''
    DEATH_NODES.append({
        "area": area, "vnum": mob_vnum,
        "lua_code": lua_code,
        "name": f"story_kill_{hub_node}_{label}",
    })


def hub_examine(area, room_vnum, pattern, hub_node, task_bit, label, lines):
    """TRIG_EXAMINE on a room — sets task bit."""
    lua_code = f'''function on_examine(ch, room, keyword)
  if ch:is_npc() then return end
  if ch:story_node() ~= {hub_node} then return end
  if ch:story_has_task({task_bit}) then return end
  ch:set_story_task({task_bit})
  ch:send("\\n\\r{_esc(_dia(lines))}\\n\\r")
end'''
    EXAMINE_NODES.append({
        "area": area, "room_vnum": room_vnum, "pattern": pattern,
        "lua_code": lua_code,
        "name": f"story_examine_{hub_node}_{label}",
    })


def hub_talk(area, vnum, pattern, hub_node, task_bit, label, lines):
    """TRIG_SPEECH on a mob — sets task bit (doesn't advance node)."""
    lua_code = f'''function on_speech(mob, ch, text)
  if ch:is_npc() then return end
  if ch:story_node() ~= {hub_node} then return end
  if ch:story_has_task({task_bit}) then return end
  ch:set_story_task({task_bit})
  ch:send("\\n\\r{_esc(_dia(lines))}\\n\\r")
end'''
    speech_node(area, vnum, pattern,
                f"story_talk_{hub_node}_{label}", lua_code, sort_order=97)


def extra_desc(area, room_vnum, keyword, description):
    """Register an extra description on a room."""
    EXTRA_DESCS.append({
        "area": area, "room_vnum": room_vnum,
        "keyword": keyword, "description": description,
    })


def obj_template(area, vnum, name, short_descr, description):
    """Register a new ITEM_TREASURE object template."""
    OBJECTS.append({
        "area": area, "vnum": vnum, "name": name,
        "short_descr": short_descr, "description": description,
    })


def obj_reset(area, obj_vnum, room_vnum):
    """Register an 'O' reset to place an object in a room."""
    RESETS.append({
        "area": area, "obj_vnum": obj_vnum, "room_vnum": room_vnum,
    })


# ============================================================
# Node 1: Midgaard — Prologue (talk-through)
# Say "darkness" to Executioner (mob 3011) → node 2
# ============================================================

talk("midgaard", 3011, "darkness", 1, 2, "executioner",
    "#CThe Executioner in the Temple of Midgaard told you:#n "
    "\"Go south to the graveyard. Find Henry. The dead have started to stir.\"",
    [
        "#CThe Executioner's eyes snap to you, his knuckles whitening on the haft#n",
        "#Cof his axe.#n",
        "#C#n",
        "#C\"The darkness? Keep your voice down. Where did you hear that -- no,#n",
        "#Cnever mind. If you know enough to ask, then someone thinks you're ready.#n",
        "#CBut you're a new face, and before we talk about what's spreading through#n",
        "#Cthese lands, you need to understand what you're dealing with. Go south#n",
        "#Cto the graveyard. Find Henry -- he tends the grounds. He says at night,#n",
        "#Cthe dead have started to stir. That's where it begins.\"#n",
    ])


# ============================================================
# Node 2: Graveyard — Full Hub (Henry, mob 3600)
# 0x01: Kill 3 undead (zombie 3601, zombie 3605, ghoul 3602, skeleton 3604)
# 0x02: Fetch bone fragment (obj 3699, room 3607)
# 0x04: Examine tombstone (room 3650)
# ============================================================

hub_intro("grave", 3600, "darkness", 2, "henry",
    "#CHenry the Gardener told you:#n "
    "\"Kill the undead, find the marked bone, examine the old tombstones.\"",
    [
        "#CHenry stops mid-dig and looks at you with tired, red-rimmed eyes.#n",
        "#C#n",
        "#C\"Darkness? Aye, that's one word for it. I've got a simpler one: wrong.#n",
        "#CEverything's gone wrong down here. They won't stay buried.\"#n",
        "#C#n",
        "#CHe leans on his shovel.#n",
        "#C#n",
        "#C\"I need your help with three things. First, clear out some of these#n",
        "#Cundead -- kill three of the wretched things so I can work in peace.#n",
        "#CSecond, search the deeper tombs for a bone fragment covered in strange#n",
        "#Cmarks. And third, examine the old tombstones south of here -- there are#n",
        "#Csymbols on them I've never seen before. Come back when you've done all#n",
        "#Cthree.\"#n",
    ],
    [
        "#CHenry glances at you.#n",
        "#C#n",
        "#C\"Still working on it? Good. Remember -- kill three undead, find the#n",
        "#Cmarked bone in the deeper tombs, and examine those strange tombstones#n",
        "#Cto the south. Come back when you're done.\"#n",
    ])

# Kill target: undead mobs
for mob_vnum in [3601, 3605, 3602, 3604]:
    hub_kill("grave", mob_vnum, 2, 0x01, 3, "undead",
        "#w[Story: undead slain ",
        "#G[Story: You've slain enough undead. Return to Henry.]#n")

# Fetch object: marked bone fragment
obj_template("grave", 3699, "bone fragment marked symbols",
    "a marked bone fragment",
    "A small bone fragment covered in strange symbols lies here.")
obj_reset("grave", 3699, 3607)

# Examine: tombstone
extra_desc("grave", 3650, "tomb stone tombstone marked symbols",
    "You kneel before the weathered tombstone and brush away the dirt.\n\r"
    "Strange symbols are carved deep into the stone -- not any language\n\r"
    "you recognize. They spiral inward, forming a pattern that seems to\n\r"
    "shift as you stare at it. The marks look identical to the ones Henry\n\r"
    "described finding on the restless dead.\n\r")

hub_examine("grave", 3650, "tomb", 2, 0x04, "tombstone",
    [
        "#CThe symbols on the tombstone seem to writhe under your gaze. These are#n",
        "#Cthe same marks Henry described -- ancient, alien, wrong. Something is#n",
        "#Cusing the dead as a canvas.#n",
        "#C#n",
        "#G[Story: Tombstone examined. Return to Henry when all tasks are done.]#n",
    ])

# Hub return: Henry takes bone, sends to Chapel
hub_return("grave", 3600, "done", 2, 3, "henry",
    "#CHenry sent you to the Chapel:#n "
    "\"Take what you've learned to the Priest. He collects strange writings.\"",
    [0x01, 0x02, 0x04], obj_vnum=3699,
    missing_lines=[
        "#CHenry looks at your empty hands.#n",
        "#C#n",
        "#C\"You've done good work, but I need that bone fragment too. Search the#n",
        "#Cdeeper tombs -- it's covered in those strange marks.\"#n",
    ],
    done_lines=[
        "#CHenry takes the bone fragment and holds it up to the fading light.#n",
        "#C#n",
        "#C\"You've done well. The undead are thinned, I've got the marked bone,#n",
        "#Cand you've seen those tombstones with your own eyes. Now take what#n",
        "#Cyou've learned to the Priest up at the Chapel -- he collects strange#n",
        "#Cwritings. He'll want to see this. Tell him about the marks.\"#n",
    ])


# ============================================================
# Node 3: Chapel — Full Hub (Priest, mob 3405)
# 0x01: Kill 1 wraith (mob 3403)
# 0x02: Fetch scroll (obj 3498, room 3457)
# 0x04: Examine etchings (room 3465)
# ============================================================

hub_intro("chapel", 3405, "marks", 3, "priest",
    "#CThe Priest told you:#n "
    "\"Destroy the wraith, find the scroll in the catacombs, examine the etchings.\"",
    [
        "#CThe Priest looks up from his reading, candlelight flickering across#n",
        "#Chis worried face.#n",
        "#C#n",
        "#C\"You've come about the marks? Henry sent you. Good. The same symbols#n",
        "#Cappear throughout these catacombs -- on walls, on bones, on things that#n",
        "#Cshould not bear writing. Three things I need from you.\"#n",
        "#C#n",
        "#C\"First, there is a wraith haunting the inner sanctum -- a creature of#n",
        "#Cpure malice. Destroy it. Second, somewhere in the catacombs below is a#n",
        "#Cscroll I've been unable to reach -- it's in the deepest chamber. And#n",
        "#Cthird, examine the etchings on the walls of the lower passages. Come#n",
        "#Cback when you've done all three.\"#n",
    ],
    [
        "#CThe Priest glances at you over his spectacles.#n",
        "#C#n",
        "#C\"Still at it? Kill the wraith, find the scroll below, and study the#n",
        "#Cetchings. I'll be here.\"#n",
    ])

hub_kill("chapel", 3403, 3, 0x01, 1, "wraith",
    "#w[Story: ",
    "#G[Story: The wraith is destroyed! Return to the Priest.]#n")

obj_template("chapel", 3498, "scroll ancient symbols catacombs",
    "an ancient scroll",
    "A tightly rolled scroll covered in faded symbols lies here.")
obj_reset("chapel", 3498, 3457)

hub_examine("chapel", 3465, "etchings", 3, 0x04, "etchings",
    [
        "#CThe etchings on the catacomb walls are old -- impossibly old. They#n",
        "#Cdepict the same spiraling symbols you found on the tombstones, but here#n",
        "#Cthey form a larger pattern: a map of connections between worlds, drawn#n",
        "#Cby hands that understood what was happening to reality.#n",
        "#C#n",
        "#G[Story: Etchings examined. Return to the Priest when all tasks are done.]#n",
    ])

hub_return("chapel", 3405, "done", 3, 4, "priest",
    "#CThe Priest sent you to the Holy Grove:#n "
    "\"The druids keep the oldest memories. Ask about the Sundering.\"",
    [0x01, 0x02, 0x04], obj_vnum=3498,
    missing_lines=[
        "#CThe Priest shakes his head.#n",
        "#C#n",
        "#C\"The scroll -- I need the scroll from the catacombs. Without it, the#n",
        "#Cetchings are just fragments.\"#n",
    ],
    done_lines=[
        "#CThe Priest takes the scroll and unrolls it carefully, his eyes#n",
        "#Cwidening.#n",
        "#C#n",
        "#C\"These symbols... I knew it. The same patterns from the oldest stones.#n",
        "#CThey speak of something called 'the Sundering' -- a time when the#n",
        "#Cwalls of the world grew thin and things bled through. I've studied#n",
        "#Cthem for years. But the druids in the Holy Grove -- they keep the#n",
        "#Coldest memories. Living memories, held in the roots of ancient trees.#n",
        "#CThey might remember what the rest of us have forgotten.\"#n",
    ])


# ============================================================
# Node 4: Grove — Light Hub (Hierophant, mob 8900)
# 0x01: Kill 2 corrupted creatures (snake 8908, stag 8906)
# 0x02: Examine sacred roots (room 8905)
# ============================================================

hub_intro("grove", 8900, "sundering", 4, "hierophant",
    "#CThe Hierophant told you:#n "
    "\"Kill the corrupted creatures and examine the sacred roots.\"",
    [
        "#CThe Hierophant's eyes fly open and she pulls her hand from the oak#n",
        "#Cas if burned.#n",
        "#C#n",
        "#C\"Where did you learn that word? The Sundering... we do not speak it#n",
        "#Clightly here. The trees shudder when it's uttered.\" She steadies#n",
        "#Cherself. \"The corruption has spread even to our sacred grove. Some of#n",
        "#Cthe creatures here have gone wrong -- twisted by whatever force bleeds#n",
        "#Cthrough the cracks.\"#n",
        "#C#n",
        "#C\"Two things. Kill two of the corrupted creatures that wander the grove#n",
        "#C-- you'll know them by their madness. And examine the roots of the#n",
        "#Csacred tree in the glade. The roots remember what the land forgets.#n",
        "#CReturn to me when you've done both.\"#n",
    ],
    [
        "#CThe Hierophant watches you with ancient eyes.#n",
        "#C#n",
        "#C\"The creatures still wander, or the roots still wait. Finish what#n",
        "#Cyou've started.\"#n",
    ])

for mob_vnum in [8908, 8906]:
    hub_kill("grove", mob_vnum, 4, 0x01, 2, "corrupted",
        "#w[Story: corrupted creatures slain ",
        "#G[Story: The corrupted creatures are cleansed. Return to the Hierophant.]#n")

extra_desc("grove", 8905, "roots tree ancient sacred",
    "You kneel among the massive roots of the sacred tree. They pulse\n\r"
    "faintly with a deep, earthy warmth. As you press your palm to the\n\r"
    "bark, images flood your mind: worlds overlapping, forests from a\n\r"
    "dozen realities tangled together, their roots intertwining beneath\n\r"
    "the soil. The tree remembers a time before the breaking -- when each\n\r"
    "grove grew in its own world, under its own sky.\n\r")

hub_examine("grove", 8905, "roots", 4, 0x02, "sacred_roots",
    [
        "#CThe sacred roots pulse beneath your touch, and for a moment you see#n",
        "#Cwhat the tree remembers: worlds that were once separate, their forests#n",
        "#Cnow tangled together beneath the soil. The land itself knows what#n",
        "#Cwas lost.#n",
        "#C#n",
        "#G[Story: Sacred roots examined. Return to the Hierophant.]#n",
    ])

hub_return("grove", 8900, "done", 4, 5, "hierophant",
    "#CThe Hierophant sent you to the Elemental Canyon:#n "
    "\"The elements are in discord. See for yourself.\"",
    [0x01, 0x02],
    done_lines=[
        "#CThe Hierophant nods slowly.#n",
        "#C#n",
        "#C\"You've done well. The creatures are cleansed, and the roots have#n",
        "#Cshown you what they remember. The land remembers what people forget.#n",
        "#CBefore the breaking, each world was whole and separate. Now even the#n",
        "#Celements are in discord -- fire burns where it shouldn't, lightning#n",
        "#Cstrikes from clear skies. Travel to the Elemental Canyon and see for#n",
        "#Cyourself. The rulers there are... agitated. They remember their true#n",
        "#Chomes.\"#n",
    ])


# ============================================================
# Node 5: Canyon — Light Hub (Earth Ruler, mob 9208)
# 0x01: Kill 3 elementals (dust cloud 9224, small rock 9217, tiny elemental 9203)
# 0x02: Examine marble throne (room 9227)
# ============================================================

hub_intro("canyon", 9208, "sundering", 5, "earth_ruler",
    "#CThe Earth Ruler told you:#n "
    "\"Slay three of the minor elementals, then examine the marble throne.\"",
    [
        "#CThe ground trembles. The Earth Ruler's voice grinds like stone on#n",
        "#Cstone.#n",
        "#C#n",
        "#C\"You dare speak that word here? The Sundering...\" A crack splits the#n",
        "#Crock beneath your feet. \"...is the reason I exist in this wretched#n",
        "#Cplace. We are not of this world. We were pulled here when the barriers#n",
        "#Cshattered.\"#n",
        "#C#n",
        "#C\"Prove your resolve. Slay three of the minor elementals that infest#n",
        "#Cthis canyon -- they are fragments, echoes of broken realms. Then seek#n",
        "#Cthe marble throne in the deeper caves and examine what is written there.#n",
        "#CReturn when you have done both.\"#n",
    ],
    [
        "#CThe Earth Ruler rumbles impatiently.#n",
        "#C#n",
        "#C\"The elementals still roam, or the throne still holds its secret.#n",
        "#CFinish what you began.\"#n",
    ])

for mob_vnum in [9224, 9217, 9203]:
    hub_kill("canyon", mob_vnum, 5, 0x01, 3, "elemental",
        "#w[Story: elementals slain ",
        "#G[Story: The elementals are dispersed. Return to the Earth Ruler.]#n")

hub_examine("canyon", 9227, "throne", 5, 0x02, "marble_throne",
    [
        "#CYou examine the marble throne and find ancient text carved into its#n",
        "#Cbase. The words describe beings of pure element, torn from their home#n",
        "#Crealms when the barriers between worlds collapsed. They rage because#n",
        "#Cthey remember what was taken from them.#n",
        "#C#n",
        "#G[Story: Marble throne examined. Return to the Earth Ruler.]#n",
    ])

hub_return("canyon", 9208, "done", 5, 6, "earth_ruler",
    "#CThe Earth Ruler sent you to the mines of Moria:#n "
    "\"Find the inscriptions in the deep tunnels. See them with your own eyes.\"",
    [0x01, 0x02],
    done_lines=[
        "#CThe Earth Ruler's voice is almost gentle.#n",
        "#C#n",
        "#C\"You have proven yourself. The elements rage because they remember#n",
        "#Cwhat was taken from them. If you would understand what broke the world,#n",
        "#Cgo deeper. Find the inscriptions in the deep tunnels of Moria -- carvings#n",
        "#Colder than us, older than the breaking itself.\"#n",
    ])


# ============================================================
# Node 6: Moria — Full Hub (The Mage, mob 4100)
# 0x01: Kill 5 tunnel creatures (orc, kobold, hobgoblin, troll, snake)
# 0x02: Fetch slime mold (obj 4103, room 4118)
# 0x04: Examine carvings (room 4160)
# ============================================================

hub_intro("moria", 4100, "carvings", 6, "the_mage",
    "#CThe Mage told you:#n "
    "\"Clear the tunnels, find the slime mold, and examine the deep carvings.\"",
    [
        "#CThe Mage's eyes glow faintly in the dark of the mines.#n",
        "#C#n",
        "#C\"The carvings? Ah, you've come seeking knowledge. Good. But the deep#n",
        "#Ctunnels are infested -- five of those tunnel creatures need clearing#n",
        "#Cbefore you can work in peace down there. And while you're at it, bring#n",
        "#Cme a slime mold from the lower chambers -- I need it for my research.#n",
        "#CFinally, examine the ancient carvings near the deep shaft. They hold#n",
        "#Cthe key to understanding what happened. Return when you've done all#n",
        "#Cthree.\"#n",
    ],
    [
        "#CThe Mage strokes his beard.#n",
        "#C#n",
        "#C\"Not finished yet? Five creatures cleared, the slime mold retrieved,#n",
        "#Cand the carvings examined. That's what I need.\"#n",
    ])

for mob_vnum in [4004, 4003, 4052, 4156, 4053]:
    hub_kill("moria", mob_vnum, 6, 0x01, 5, "tunnel_creature",
        "#w[Story: tunnel creatures slain ",
        "#G[Story: The tunnels are clear. Return to the Mage.]#n")

# Slime mold already exists as obj 4103; just add reset
obj_reset("moria", 4103, 4118)

# Carvings extra desc already exists on room 4160 from previous seed
extra_desc("moria", 4160, "carvings spirals ancient inscription",
    "You trace the deep-cut spirals with your fingers. The carvings depict\n\r"
    "a great city whose spires reached between worlds -- lines radiating\n\r"
    "outward like cracks in glass. In the center, a single figure stands\n\r"
    "with arms raised, and from its hands pour rivers of light connecting\n\r"
    "dozens of spheres. Then, abruptly, the spirals shatter. The lines\n\r"
    "become jagged. The spheres collapse inward. The final panel shows\n\r"
    "everything tangled together -- worlds overlapping, boundaries gone.\n\r"
    "The carving ends mid-stroke, as if the hand that made it simply\n\r"
    "ceased to exist.\n\r")

hub_examine("moria", 4160, "carvings", 6, 0x04, "moria_carvings",
    [
        "#CAs you trace the carvings, a chill runs through you. These aren't#n",
        "#Cdecorations -- they're a record. A warning. The spirals depict a city#n",
        "#Cthat reached between worlds... and what happened when the reaching#n",
        "#Cwent wrong.#n",
        "#C#n",
        "#G[Story: Carvings examined. Return to the Mage when all tasks are done.]#n",
    ])

hub_return("moria", 4100, "done", 6, 7, "the_mage",
    "#CThe Mage sent you to the High Tower:#n "
    "\"A sorcerer there tried to finish the story. Find his journal.\"",
    [0x01, 0x02, 0x04], obj_vnum=4103,
    missing_lines=[
        "#CThe Mage peers at you.#n",
        "#C#n",
        "#C\"Where's my slime mold? I need it from the lower chambers. Bring it#n",
        "#Cto me along with everything else.\"#n",
    ],
    done_lines=[
        "#CThe Mage takes the slime mold and tucks it into his robes with a#n",
        "#Csatisfied nod.#n",
        "#C#n",
        "#C\"Excellent. The tunnels are clear, the carvings speak for themselves,#n",
        "#Cand this mold -- you wouldn't believe what it reveals under the right#n",
        "#Cconditions. The carvings depict a city that reached between worlds.#n",
        "#CA sorcerer in the High Tower tried to finish the story. His journal#n",
        "#Cis in the study. Take it to the Librarian in Thalos -- she'll know#n",
        "#Cwhat to make of it.\"#n",
    ])


# ============================================================
# Node 7: Hitower — Full Hub (Adventurer, mob 1301)
# 0x01: Kill 3 golems (wooden 1339, bronze 1343, flesh 1344, adamantite 1346, clay 1347)
# 0x02: Fetch journal (obj 1499, room 1367)
# 0x04: Examine mirror (room 1336)
# ============================================================

hub_intro("hitower", 1301, "sorcerer", 7, "adventurer",
    "#CThe Adventurer told you:#n "
    "\"Smash the golems, find the journal, and look into the mirror.\"",
    [
        "#CThe lost adventurer looks up with haunted eyes.#n",
        "#C#n",
        "#C\"The sorcerer? You're looking for his work? Then you're braver than#n",
        "#Cme. This tower is crawling with his creations -- golems, animated by#n",
        "#Cwhatever power he was tapping into. Three things you'll need to do.\"#n",
        "#C#n",
        "#C\"First, destroy three of the golems -- they guard the upper levels.#n",
        "#CSecond, the sorcerer's journal is in the study further up. And third#n",
        "#C-- there's a mirror on one of the upper floors. Don't just glance at#n",
        "#Cit. Examine it. Really look. Then come back and tell me what you saw.\"#n",
    ],
    [
        "#CThe adventurer shifts nervously.#n",
        "#C#n",
        "#C\"Three golems, the journal, and the mirror. I'll wait here.\"#n",
    ])

for mob_vnum in [1339, 1343, 1344, 1346, 1347]:
    hub_kill("hitower", mob_vnum, 7, 0x01, 3, "golem",
        "#w[Story: golems destroyed ",
        "#G[Story: Enough golems destroyed. Return to the Adventurer.]#n")

obj_template("hitower", 1499, "journal sorcerer notes leather",
    "a sorcerer's journal",
    "A leather-bound journal lies open on the desk.")
obj_reset("hitower", 1499, 1367)

hub_examine("hitower", 1336, "mirror", 7, 0x04, "hitower_mirror",
    [
        "#CYou stare into the mirror and for a moment see not your reflection but#n",
        "#Csomething else entirely -- a city of impossible spires reaching between#n",
        "#Cstars, its towers bridging gaps in reality itself. Then the image#n",
        "#Cshifts: the spires crack, the bridges shatter, and everything falls#n",
        "#Cinward. The mirror goes dark, then shows only your face again.#n",
        "#C#n",
        "#G[Story: Mirror examined. Return to the Adventurer when all tasks are done.]#n",
    ])

hub_return("hitower", 1301, "done", 7, 8, "adventurer",
    "#CThe Adventurer sent you to Old Thalos:#n "
    "\"Take the journal to the Librarian. She'll know what to make of it.\"",
    [0x01, 0x02, 0x04], obj_vnum=1499,
    missing_lines=[
        "#CThe adventurer looks at your hands.#n",
        "#C#n",
        "#C\"The journal -- it's in the study upstairs. I can't make sense of#n",
        "#Canything without it.\"#n",
    ],
    done_lines=[
        "#CThe adventurer takes a deep breath.#n",
        "#C#n",
        "#C\"The golems are down, you've seen what the mirror shows, and that#n",
        "#Cjournal... I couldn't bring myself to read past the first page. Take#n",
        "#Cit to the Librarian in Old Thalos. She studies the old writings. She'll#n",
        "#Cknow what to make of it.\"#n",
    ])


# ============================================================
# Node 8: Thalos — Full Hub (Librarian, mob 5315)
# 0x01: Kill 2 hostile creatures (stag 5324, deer 5323, fido 5333)
# 0x02: Examine statue of Odin (room 5386)
# 0x04: Examine foundation ruins (room 5371)
# ============================================================

hub_intro("mirror", 5315, "thalos", 8, "librarian",
    "#CThe Librarian told you:#n "
    "\"Clear the wild beasts, examine the statue and the old foundations.\"",
    [
        "#CThe Librarian carefully closes a crumbling tome and fixes you with a#n",
        "#Csharp look.#n",
        "#C#n",
        "#C\"You came about Thalos? Then you'll need more than words. The old#n",
        "#Ccity has secrets, but the ruins are dangerous -- wild beasts roam#n",
        "#Cthe outer districts. Clear two of them so we can work safely.\"#n",
        "#C#n",
        "#C\"Then I need you to examine two things: the statue of Odin in the#n",
        "#Cnorthern ruins -- it's not from this world, and the carvings on it#n",
        "#Cprove it. And the foundation stones south of here -- they're all#n",
        "#Cthat remain of the original Thalos, the one that existed before the#n",
        "#Creplacement. Come back when you've seen both.\"#n",
    ],
    [
        "#CThe Librarian adjusts her spectacles.#n",
        "#C#n",
        "#C\"Two beasts cleared, the Odin statue, and the foundations. I need#n",
        "#Call three.\"#n",
    ])

for mob_vnum in [5324, 5323, 5333]:
    hub_kill("mirror", mob_vnum, 8, 0x01, 2, "beast",
        "#w[Story: wild beasts cleared ",
        "#G[Story: The beasts are cleared. Return to the Librarian.]#n")

hub_examine("mirror", 5386, "statue", 8, 0x02, "odin_statue",
    [
        "#CThe statue of Odin stands impossibly tall, carved from stone that#n",
        "#Cdoesn't match anything in this region. Its single eye seems to follow#n",
        "#Cyou. At the base, runes tell of a god displaced from his own world,#n",
        "#Cset down in a land that was never meant to hold him.#n",
        "#C#n",
        "#G[Story: Statue examined. Return to the Librarian when all tasks are done.]#n",
    ])

hub_examine("mirror", 5371, "foundation", 8, 0x04, "thalos_foundation",
    [
        "#CThe foundation stones are older than anything else in the city. You#n",
        "#Ccan see where the original walls stood -- and where they simply stop,#n",
        "#Cas if the building was cut away and replaced with something else. The#n",
        "#Cmortar between the old and new stones doesn't match. Thalos wasn't#n",
        "#Cdestroyed. It was replaced, overnight, by a copy from another world.#n",
        "#C#n",
        "#G[Story: Foundations examined. Return to the Librarian when all tasks are done.]#n",
    ])

hub_return("mirror", 5315, "done", 8, 9, "librarian",
    "#CThe Librarian sent you to the Drow city:#n "
    "\"The Drow were pulled from another world entirely. Ask about displacement.\"",
    [0x01, 0x02, 0x04], obj_vnum=1499,
    missing_lines=[
        "#CThe Librarian shakes her head.#n",
        "#C#n",
        "#C\"The journal -- I need the sorcerer's journal from the tower. Show#n",
        "#Cme the pages.\"#n",
    ],
    done_lines=[
        "#CThe Librarian takes the journal and begins reading immediately, her#n",
        "#Ceyes widening with every page.#n",
        "#C#n",
        "#C\"This confirms everything. Thalos wasn't destroyed by war or plague.#n",
        "#CIt was replaced. One morning the city was there, whole and living.#n",
        "#CThe next, this ruin stood in its place -- as if copied from a dream#n",
        "#Cand set down wrong. You should speak to the Drow -- an entire#n",
        "#Ccivilization pulled underground from another world entirely. If#n",
        "#Canyone understands what it means to be displaced, it's them.\"#n",
    ])


# ============================================================
# Node 9: Drow — Light Hub (Priestess, mob 5104)
# 0x01: Examine the altar of Lloth (room 5145)
# ============================================================

hub_intro("drow", 5104, "displacement", 9, "priestess",
    "#CThe Drow Priestess told you:#n "
    "\"Examine the Altar of Lloth. See what displacement looks like.\"",
    [
        "#CThe Drow Priestess regards you with cold, ancient eyes. A thin smile#n",
        "#Ccrosses her lips.#n",
        "#C#n",
        "#C\"Displacement. A surfacer's word for what happened to us. So clinical.#n",
        "#CSo tidy. Let me tell you what 'displacement' feels like: our entire#n",
        "#Ccity was torn from the Underdark of a world that no longer exists.#n",
        "#CThe screaming lasted for days.\"#n",
        "#C#n",
        "#C\"Go to the Altar of Lloth. Examine it. See with your own eyes what#n",
        "#Cdisplacement looks like -- the marks of a world ripped away from#n",
        "#Citself. Then return to me.\"#n",
    ],
    [
        "#CThe Priestess narrows her eyes.#n",
        "#C#n",
        "#C\"The Altar awaits. Go. Look. Then return.\"#n",
    ])

extra_desc("drow", 5145, "altar lloth spider web",
    "The Altar of Lloth is a massive spider-shaped construction of black\n\r"
    "obsidian. But something is wrong with it. Cracks run through the stone\n\r"
    "in patterns you recognize -- the same spiraling fractures you've seen\n\r"
    "on tombstones, in catacombs, carved into ancient walls. The altar was\n\r"
    "not built here. It was torn from another world and deposited in this\n\r"
    "one, and the stone itself bears the scars of the crossing.\n\r")

hub_examine("drow", 5145, "altar", 9, 0x01, "drow_altar",
    [
        "#CThe cracks in the altar tell the story: this sacred object was ripped#n",
        "#Cfrom one world and dropped into another. The same spiraling fractures#n",
        "#Cyou've seen everywhere -- the signature of the Sundering, written in#n",
        "#Cbroken stone.#n",
        "#C#n",
        "#G[Story: Altar examined. Return to the Priestess.]#n",
    ])

hub_return("drow", 5104, "done", 9, 10, "priestess",
    "#CThe Priestess sent you to the Pyramid:#n "
    "\"Find the warning tablets. Bring them to the Sphinx.\"",
    [0x01],
    done_lines=[
        "#CThe Priestess nods, a rare sign of approval.#n",
        "#C#n",
        "#C\"Now you see. We adapted -- the Drow always adapt. But the scars#n",
        "#Cremain. You want to understand the scope of this? Find the warning#n",
        "#Ctablets in the ancient hall of the pyramid -- writings far older#n",
        "#Cthan our arrival. Bring them to the Sphinx. It has guarded those#n",
        "#Cwarnings for eons.\"#n",
    ])


# ============================================================
# Node 10: Pyramid — Full Hub (Sphinx, mob 2616)
# 0x01: Fetch warning tablet (obj 2699, room 2630)
# 0x02: Examine hieroglyphics (room 2629)
# ============================================================

hub_intro("pyramid", 2616, "cycle", 10, "sphinx",
    "#CThe Sphinx told you:#n "
    "\"Retrieve the warning tablet and study the hieroglyphics.\"",
    [
        "#CThe Great Sphinx's stone eyes focus on you.#n",
        "#C#n",
        "#C\"You speak of the cycle. Interesting. Few make it this far. Two things#n",
        "#CI require of you before I share what I guard.\"#n",
        "#C#n",
        "#C\"First, retrieve the warning tablet from the ancient hall deeper in#n",
        "#Cthe pyramid. It contains the oldest known record of the cycle. Second,#n",
        "#Cstudy the hieroglyphics on the walls near the burial chamber -- they#n",
        "#Ctell the same story in a different tongue. Return with both the tablet#n",
        "#Cand your understanding.\"#n",
    ],
    [
        "#CThe Sphinx's gaze is patient but unyielding.#n",
        "#C#n",
        "#C\"The tablet and the hieroglyphics. I will wait.\"#n",
    ])

obj_template("pyramid", 2699, "tablet warning stone hieroglyphs",
    "a stone warning tablet",
    "A stone tablet covered in ancient hieroglyphs leans against the wall.")
obj_reset("pyramid", 2699, 2630)

hub_examine("pyramid", 2629, "hieroglyphics", 10, 0x02, "hieroglyphics",
    [
        "#CThe hieroglyphics on the wall depict a recurring cycle: worlds reach#n",
        "#Cout, touch each other, tear open, and flood together. The images show#n",
        "#Cit happening again and again -- this is not the first Sundering, and#n",
        "#Cthe ancient artists knew it would not be the last.#n",
        "#C#n",
        "#G[Story: Hieroglyphics studied. Return to the Sphinx when all tasks are done.]#n",
    ])

hub_return("pyramid", 2616, "done", 10, 11, "sphinx",
    "#CThe Sphinx sent you to the Void:#n "
    "\"Seek the place between -- where the dreamer waits.\"",
    [0x01, 0x02], obj_vnum=2699,
    missing_lines=[
        "#CThe Sphinx rumbles.#n",
        "#C#n",
        "#C\"The tablet -- bring it to me from the ancient hall. Only then.\"#n",
    ],
    done_lines=[
        "#CThe Great Sphinx takes the tablet in massive stone paws and reads the#n",
        "#Chieroglyphs with ancient recognition.#n",
        "#C#n",
        "#C\"Yes. These are the warnings. The cycle is thus: a world reaches far#n",
        "#Cenough to touch another. The touch becomes a tear. The tear becomes a#n",
        "#Cflood. I have guarded these warnings through more turnings than you#n",
        "#Ccan imagine. This is not the first Sundering, child. It will not be#n",
        "#Cthe last. If you wish to see where the walls are thinnest -- where#n",
        "#Cthe cycle's edge cuts closest -- seek the place between. The void#n",
        "#Cwhere the dreamer waits.\"#n",
    ])


# ============================================================
# Node 11: Dream — Light Hub (Keeper, mob 8600)
# 0x01: Examine the old church in Newfane (room 8637)
# ============================================================

hub_intro("dream", 8600, "between", 11, "keeper",
    "#CThe Keeper told you:#n "
    "\"Examine the old church in Newfane. See what bleeds through.\"",
    [
        "#CThe Keeper speaks without moving, its voice arriving from everywhere#n",
        "#Cand nowhere.#n",
        "#C#n",
        "#C\"Between. Yes. That is the only honest word for this place. You stand#n",
        "#Cbetween what was and what will be -- between worlds that were once#n",
        "#Cseparate and are now tangled together.\"#n",
        "#C#n",
        "#C\"But even the void has its memories. There is an old church in the#n",
        "#Ctownship of Newfane -- a fragment from a world of quiet faith, dropped#n",
        "#Chere when the barriers fell. Examine it. See what bleeds through the#n",
        "#Cwalls of a building that was never meant to exist in this place. Then#n",
        "#Creturn to me.\"#n",
    ],
    [
        "#CThe Keeper's silence is patient.#n",
        "#C#n",
        "#C\"The church in Newfane. Go.\"#n",
    ])

extra_desc("dream", 8637, "church pews windows stained",
    "The old wooden church creaks in a wind that doesn't exist here. The\n\r"
    "pews are arranged for a congregation that never arrives, and the\n\r"
    "stained glass windows depict scenes from a world you don't recognize\n\r"
    "-- green hills under a yellow sun, a river that flows upward, children\n\r"
    "playing in fields of silver grass. This building was pulled whole from\n\r"
    "another reality, and it sits here in the void like a memory that\n\r"
    "refuses to fade.\n\r")

hub_examine("dream", 8637, "church", 11, 0x01, "newfane_church",
    [
        "#CThe church is a fragment of another world, preserved perfectly in the#n",
        "#Cvoid. Its stained glass windows show scenes from a reality that no#n",
        "#Clonger exists -- or perhaps one that hasn't happened yet. The Sundering#n",
        "#Cdoesn't just pull across space. It pulls across time.#n",
        "#C#n",
        "#G[Story: Church examined. Return to the Keeper.]#n",
    ])

hub_return("dream", 8600, "done", 11, 12, "keeper",
    "#CThe Keeper sent you onward:#n "
    "\"Seek the gods on Olympus, or the king beneath the waves. "
    "Ask about the convergence.\"",
    [0x01],
    done_lines=[
        "#CThe Keeper's voice arrives from the emptiness.#n",
        "#C#n",
        "#C\"You saw. The Unraveling doesn't just pull across space. It pulls#n",
        "#Cacross time. What you call the future is already bleeding through.#n",
        "#CThrough one tear, an ocean where a king rules waters that aren't#n",
        "#Chis. Through another, a mountain where gods sit in exile, pretending#n",
        "#Cthey still matter. Seek the gods on Olympus, or the king beneath#n",
        "#Cthe waves. Ask them about the convergence. They know what it means#n",
        "#Cto be torn from one world and dropped into another.\"#n",
    ])


# ============================================================
# Node 12: Atlantis / Olympus — Light Hub (branch)
# Both paths: 0x01 Kill 1, 0x02 Examine
# Both advance to node 13
# ============================================================

# --- Atlantis path (Neptune, mob 8103) ---

hub_intro("atlantis", 8103, "convergence", 12, "neptune",
    "#CKing Neptune told you:#n "
    "\"Defeat one of my guards and examine the temple murals.\"",
    [
        "#CKing Neptune strokes his coral beard, his trident dimming at the word.#n",
        "#C#n",
        "#C\"The convergence. So you've learned its name. Most who swim these#n",
        "#Cwaters think they were always here. They weren't. My kingdom rests in#n",
        "#Cwaters that belong to another world.\"#n",
        "#C#n",
        "#C\"To understand, you must see and do. Defeat one of my guards -- they#n",
        "#Chave been... changed by the crossing, made aggressive. And examine the#n",
        "#Cmurals in the Temple of Atlantis. They tell the story of our arrival.#n",
        "#CReturn when you have done both.\"#n",
    ],
    [
        "#CKing Neptune waves a webbed hand.#n",
        "#C#n",
        "#C\"A guard defeated and the temple murals examined. That is what I#n",
        "#Crequire.\"#n",
    ])

hub_kill("atlantis", 8110, 12, 0x01, 1, "atlantis_guard",
    "#w[Story: ",
    "#G[Story: The guard is defeated. Return to King Neptune.]#n")

extra_desc("atlantis", 8131, "murals coral temple paintings",
    "The temple murals are painted in luminous coral pigments that still\n\r"
    "glow after untold centuries. They depict a great kingdom beneath a\n\r"
    "different ocean -- warmer, brighter, with twin suns reflected in the\n\r"
    "waves. Then a crack appears in the water itself, and the kingdom is\n\r"
    "pulled through, deposited in cold, dark waters beneath an alien sky.\n\r"
    "The final panel shows the Atlanteans adapting, building anew, but\n\r"
    "the twin suns in the background have been replaced by a single,\n\r"
    "dimmer light.\n\r")

hub_examine("atlantis", 8131, "murals", 12, 0x02, "atlantis_murals",
    [
        "#CThe murals tell the story of Atlantis's displacement -- a kingdom torn#n",
        "#Cfrom warmer waters and deposited here. The Atlanteans adapted, but the#n",
        "#Ctwin suns of their home world are gone, replaced by this single dim#n",
        "#Clight.#n",
        "#C#n",
        "#G[Story: Murals examined. Return to King Neptune.]#n",
    ])

hub_return("atlantis", 8103, "done", 12, 13, "neptune",
    "#CKing Neptune sent you to Mega-City One:#n "
    "\"Ask the judges in the city of iron and glass about the future.\"",
    [0x01, 0x02],
    done_lines=[
        "#CKing Neptune nods, a current of ancient sadness in his eyes.#n",
        "#C#n",
        "#C\"Now you see what we lost. But the newcomers -- the ones from the#n",
        "#Cstrange metal cities -- they arrived later. And stranger. As if pulled#n",
        "#Cfrom a time that hasn't fully happened yet. Ask the judges in the#n",
        "#Ccity of iron and glass. They patrol streets that shouldn't exist yet#n",
        "#C-- and they don't even know it.\"#n",
    ])

# --- Olympus path (Zeus, mob 901) ---

hub_intro("olympus", 901, "convergence", 12, "zeus",
    "#CZeus told you:#n "
    "\"Defeat a beast on the mountain and examine the scorch marks on the throne.\"",
    [
        "#CZeus sits upon his throne, looking tired in a way that gods should#n",
        "#Cnot. He regards you for a long moment.#n",
        "#C#n",
        "#C\"The convergence. I had hoped that word would die with the old city.#n",
        "#CYou mortals think us gods. We were merely... from elsewhere. A world#n",
        "#Cwhere we were powerful, yes. But that world is gone.\"#n",
        "#C#n",
        "#C\"Prove yourself. A chimera prowls the mountain paths -- defeat it.#n",
        "#CAnd examine the scorch marks around this throne. They are not from#n",
        "#Cmy lightning. They are from the crossing. Return when you've done#n",
        "#Cboth.\"#n",
    ],
    [
        "#CZeus stirs impatiently.#n",
        "#C#n",
        "#C\"The chimera and the scorch marks. Go.\"#n",
    ])

hub_kill("olympus", 920, 12, 0x01, 1, "chimera",
    "#w[Story: ",
    "#G[Story: The chimera is slain. Return to Zeus.]#n")

extra_desc("olympus", 910, "scorch marks lightning burns crossing",
    "Around the base of the throne, deep burn marks scar the marble floor.\n\r"
    "They don't radiate outward like lightning strikes -- they spiral inward,\n\r"
    "as if reality itself was dragged through this point. The same spiraling\n\r"
    "pattern you've seen on tombstones, in catacombs, on altars. This is\n\r"
    "where Olympus arrived -- torn from its own sky and dropped into this\n\r"
    "one.\n\r")

hub_examine("olympus", 910, "scorch", 12, 0x02, "olympus_scorch",
    [
        "#CThe scorch marks spiral inward -- not lightning, but the signature of#n",
        "#Ca world being torn through the barriers. Olympus was ripped from its#n",
        "#Cown sky and deposited here, and the stone still bears the burns.#n",
        "#C#n",
        "#G[Story: Scorch marks examined. Return to Zeus.]#n",
    ])

hub_return("olympus", 901, "done", 12, 13, "zeus",
    "#CZeus sent you to Mega-City One:#n "
    "\"Seek the metal city. Something there reeks of a future that was never meant to be.\"",
    [0x01, 0x02],
    done_lines=[
        "#CZeus nods heavily.#n",
        "#C#n",
        "#C\"Now you've seen it. We are refugees wearing the faces of legends.#n",
        "#CSeek the metal city to the north. Something there reeks of a future#n",
        "#Cthat was never meant to be.\"#n",
    ])


# ============================================================
# Node 13: Mega-City — Light Hub (Judge, mob 8010)
# 0x01: Kill 1 punk (mob 8001)
# 0x02: Examine construction site (room 8014)
# ============================================================

hub_intro("mega1", 8010, "future", 13, "judge",
    "#CJudge Eckersley told you:#n "
    "\"Take down a punk and examine the construction site anomaly.\"",
    [
        "#CJudge Eckersley adjusts his helmet and squints at you.#n",
        "#C#n",
        "#C\"Future? The future? This IS the future, citizen.\" He pauses.#n",
        "#C\"...isn't it?\"#n",
        "#C#n",
        "#CA flicker of confusion crosses his face.#n",
        "#C#n",
        "#C\"Sometimes I get these flashes -- like I remember a different sky.#n",
        "#CDifferent laws. Look, you want to understand? Two things. First, deal#n",
        "#Cwith one of those punks causing trouble on the streets. Second, go to#n",
        "#Cthe construction site -- there's something wrong with the foundation#n",
        "#Cthere. Some kind of anomaly in the rubble. Examine it. Come back and#n",
        "#Ctell me what you find.\"#n",
    ],
    [
        "#CThe Judge taps his badge impatiently.#n",
        "#C#n",
        "#C\"A punk and the construction site. Move it, citizen.\"#n",
    ])

hub_kill("mega1", 8001, 13, 0x01, 1, "punk",
    "#w[Story: ",
    "#G[Story: The punk is dealt with. Return to Judge Eckersley.]#n")

extra_desc("mega1", 8014, "construction rubble anomaly foundation",
    "Beneath the rubble of the construction site, you find something that\n\r"
    "shouldn't be here: a layer of cobblestone from another era entirely,\n\r"
    "perfectly preserved beneath the ferrocrete. The stones are warm to\n\r"
    "the touch and covered in the same spiraling fracture patterns you've\n\r"
    "seen across the world. This city wasn't built here -- it was pulled\n\r"
    "from a future that was never meant to arrive, and dropped on top of\n\r"
    "whatever was here before.\n\r")

hub_examine("mega1", 8014, "rubble", 13, 0x02, "construction_anomaly",
    [
        "#CBeneath the ferrocrete, cobblestones from another era. This city was#n",
        "#Cpulled from a future that hasn't fully happened yet and dropped on#n",
        "#Ctop of whatever was here before. The spiraling fractures confirm it.#n",
        "#C#n",
        "#G[Story: Construction site examined. Return to Judge Eckersley.]#n",
    ])

hub_return("mega1", 8010, "done", 13, 14, "judge",
    "#CJudge Eckersley sent you to the Dome Ship:#n "
    "\"There's a ship in orbit with sensors that can see where the threads come from.\"",
    [0x01, 0x02],
    done_lines=[
        "#CThe Judge takes this in, looking shaken.#n",
        "#C#n",
        "#C\"Cobblestones under the ferrocrete... that explains the flashes. Look,#n",
        "#Cthere's a ship in orbit. Aliens with instruments -- sensors, scanners.#n",
        "#CThey say they can see where all the threads come from. Go up to the#n",
        "#Cbridge, look at the holo-display. See for yourself where the threads#n",
        "#Clead.\"#n",
    ])


# ============================================================
# Node 14: Domeship — Light Hub (Elfangor, mob 93006)
# 0x01: Examine sensors (room 93045)
# ============================================================

hub_intro("domeship", 93006, "origin", 14, "elfangor",
    "#CElfangor told you:#n "
    "\"Examine the sensor display on the bridge. See where the threads lead.\"",
    [
        "#CElfangor-Sirinial-Shamtul turns all four eyes toward you, and you#n",
        "#Csense a vast, alien sadness.#n",
        "#C#n",
        "#C\"The origin. You have come asking the same question we have spent#n",
        "#Ccycles trying to answer. Our sensors have mapped the convergence --#n",
        "#Cevery fragment of reality in this patchwork world traces back to a#n",
        "#Csingle point.\"#n",
        "#C#n",
        "#C\"Go to the bridge. Examine the sensor display. See for yourself where#n",
        "#Cthe threads lead. Then return to me.\"#n",
    ],
    [
        "#CElfangor's eyestalks swivel patiently.#n",
        "#C#n",
        "#C\"The sensor display on the bridge. Go.\"#n",
    ])

extra_desc("domeship", 93045, "sensors display holo readings",
    "The holographic display flickers to life as you approach. Thousands of\n\r"
    "luminous threads radiate outward from a central point, each one a\n\r"
    "connection between worlds. You trace them: one leads to an ocean\n\r"
    "kingdom, another to a mountain of gods, another to tunnels of dark\n\r"
    "elves. The threads pulse with different colors -- some ancient gold,\n\r"
    "some fresh silver, some a sickly green that seems to eat the light\n\r"
    "around it. But every single thread, no matter how far it reaches,\n\r"
    "traces back to the same origin point: a city. An old city, at the\n\r"
    "center of everything.\n\r")

hub_examine("domeship", 93045, "sensors", 14, 0x01, "domeship_sensors",
    [
        "#CThe holographic threads pulse and converge before your eyes. Every#n",
        "#Cfragment of this patchwork world -- every displaced civilization, every#n",
        "#Ctorn reality -- traces back to a single point. An old city. The origin#n",
        "#Cof everything.#n",
        "#C#n",
        "#G[Story: Sensors examined. Return to Elfangor.]#n",
    ])

hub_return("domeship", 93006, "done", 14, 15, "elfangor",
    "#CElfangor sent you to Old Dystopia:#n "
    "\"The old city is at the center. But it is guarded.\"",
    [0x01],
    done_lines=[
        "#CElfangor's thought-speak fills your mind.#n",
        "#C#n",
        "#C\"Now you see what we see. Every thread traces back to one point -- an#n",
        "#Cold city, at the center of everything. But our readings show something#n",
        "#Celse: the temporal distortions aren't random. Something is threading#n",
        "#Cthrough the timelines, pulling them together. The old city may hold#n",
        "#Cthat answer. But be warned -- it is guarded. The Royal Guard still#n",
        "#Cpatrols those ruins. You will need to fight your way through.\"#n",
    ])


# ============================================================
# Node 15: Dystopia — Full Hub (Queen, mob 30508)
# 0x01: Kill 1 royal guard (mob 30400)
# 0x02: Examine records (room 30460)
# 0x04: Talk to King (mob 30509, SPEECH sets bit only)
# ============================================================

hub_intro("dystopia", 30508, "old city", 15, "queen",
    "#CThe Queen told you:#n "
    "\"Fight a guard, read the records, and speak to the King about what we did.\"",
    [
        "#CThe Queen of Dystopia looks at you with eyes that have seen too much.#n",
        "#CA sad smile crosses her face.#n",
        "#C#n",
        "#C\"The old city. You called it that -- everyone does now. We just called#n",
        "#Cit home. So you've come at last. The ones who ask questions instead of#n",
        "#Cswinging swords always do, eventually.\"#n",
        "#C#n",
        "#C\"Three things before we speak further. Fight one of the Royal Guard --#n",
        "#Cthey need thinning, and you need to prove you can survive what's coming.#n",
        "#CThen go to the library and read the records -- see what we did, in our#n",
        "#Cown words. And finally, speak to the King about the Unraveling. He#n",
        "#Ccarries the weight of it. Return to me when you've done all three.\"#n",
    ],
    [
        "#CThe Queen regards you patiently.#n",
        "#C#n",
        "#C\"A guard defeated, the records read, the King consulted. I need all#n",
        "#Cthree before we proceed.\"#n",
    ])

hub_kill("dystopia", 30400, 15, 0x01, 1, "royal_guard",
    "#w[Story: ",
    "#G[Story: The Royal Guard falls. Return to the Queen when all tasks are done.]#n")

extra_desc("dystopia", 30460, "records scrolls writings library",
    "You pull a crumbling scroll from the shelf and unroll it carefully.\n\r"
    "The handwriting is precise, scientific -- lab notes from a civilization\n\r"
    "that treated reality itself as an experiment. 'Day 1,247: Successfully\n\r"
    "opened a stable conduit to Realm-7 (oceanic). Sustained contact for\n\r"
    "14 hours. Inhabitants observed but unaware of our presence.' The entries\n\r"
    "grow bolder. 'Day 2,891: Twelve simultaneous conduits. We can see them\n\r"
    "all. A mountain realm of tremendous beings. A fungal forest of small\n\r"
    "blue creatures. An underground civilization of exquisite cruelty.' Then\n\r"
    "the tone changes. 'Day 3,002: The conduits will not close. Realm-7\n\r"
    "inhabitants are appearing in our streets. Day 3,003: Reality is\n\r"
    "merging. Day 3,004: The barriers are gone. All of them. What have\n\r"
    "we done.'\n\r")

hub_examine("dystopia", 30460, "records", 15, 0x02, "dystopia_records",
    [
        "#CThe records tell the whole terrible story. They opened the doors#n",
        "#Cbetween worlds -- carefully at first, then recklessly. They saw#n",
        "#Cwonders. And then the doors wouldn't close. Reality merged. The#n",
        "#Cbarriers fell. Every world they'd touched came flooding in.#n",
        "#C#n",
        "#G[Story: Records read. Return to the Queen when all tasks are done.]#n",
    ])

# King talk — sets 0x04 bit but doesn't advance node
hub_talk("dystopia", 30509, "unraveling", 15, 0x04, "king",
    [
        "#CThe King of Dystopia stares out over the ruins of his city. When he#n",
        "#Cspeaks, his voice is quiet.#n",
        "#C#n",
        "#C\"The Unraveling. That's what they call it now. We called it progress.#n",
        "#CWe were so certain we could control it -- open doors between worlds#n",
        "#Cand close them at will. We were wrong. The doors don't close. They#n",
        "#Cnever did. Every world we touched was pulled into ours, and ours into#n",
        "#Ctheirs, until none of it made sense anymore.\"#n",
        "#C#n",
        "#CHe is silent for a long moment.#n",
        "#C#n",
        "#C\"But you know what I've learned? The world didn't end. It changed.#n",
        "#CThe people built new lives in the wreckage. That's not a tragedy.#n",
        "#CThat's just... what people do.\"#n",
        "#C#n",
        "#G[Story: The King has spoken. Return to the Queen when all tasks are done.]#n",
    ])

hub_return("dystopia", 30508, "done", 15, 16, "queen",
    "#CThe Queen sent you to seek the final word:#n "
    "\"Heaven or Hell -- the angels and demons have opinions on what comes next.\"",
    [0x01, 0x02, 0x04],
    done_lines=[
        "#CThe Queen nods slowly, something like peace crossing her face.#n",
        "#C#n",
        "#C\"You've done what I asked. You fought through the guard, you read the#n",
        "#Crecords, and you heard the King's confession. Now you know the full#n",
        "#Cstory -- what we did, and what it cost. If you want the last word on#n",
        "#Call of this, seek Heaven or Hell. The angels and demons have their own#n",
        "#Copinions on what comes next.\"#n",
    ])


# ============================================================
# Node 16: Epilogue — Talk-through (Heaven or Hell → 99)
# ============================================================

talk("heaven", 99004, "what now", 16, 99, "overseer",
    "#CYou have completed 'Echoes of the Sundering.'#n",
    [
        "#CThe Overseer gazes down from a throne of light, and for a moment,#n",
        "#Csomething almost like pity crosses its luminous face.#n",
        "#C#n",
        "#C\"What now? That's what you ask, after everything you've learned?\"#n",
        "#CIt leans forward. \"The angels would have you believe we can restore#n",
        "#Cthe barriers. Separate the worlds again. Perhaps they're right.#n",
        "#CPerhaps the Sundering can be undone. But ask yourself -- would the#n",
        "#Cpeople in those fragments want to go back?\"#n",
        "#C#n",
        "#CYou stand in Heaven, at the edge of understanding. The Sundering#n",
        "#Cbroke the world -- or made it. The stories remain. The worlds remain.#n",
        "#CAnd as long as someone walks through them and listens, they are not#n",
        "#Cforgotten.#n",
        "#C#n",
        "#G[Quest Complete: Echoes of the Sundering]#n",
    ])

talk("hell", 30101, "what now", 16, 99, "pitlord",
    "#CYou have completed 'Echoes of the Sundering.'#n",
    [
        "#CThe Pit Lord laughs, a sound like breaking stone.#n",
        "#C#n",
        "#C\"What now, it asks! Ha! You walk through a shattered world, piece#n",
        "#Ctogether its sorry history, and your question is 'what now?' The#n",
        "#Cdemons don't want to fix anything. They want to pull more through.#n",
        "#CMore worlds, more chaos, more power. And maybe they're right.\"#n",
        "#C#n",
        "#CYou stand in Hell, at the edge of understanding. The Sundering#n",
        "#Cbroke the world -- or made it. The stories remain. The worlds remain.#n",
        "#CAnd as long as someone walks through them and listens, they are not#n",
        "#Cforgotten.#n",
        "#C#n",
        "#G[Quest Complete: Echoes of the Sundering]#n",
    ])


# ============================================================
# Seeding functions
# ============================================================

def seed_speech_scripts(db_dir):
    """Insert TRIG_SPEECH story scripts on mobs."""
    by_area = {}
    for n in SPEECH_NODES:
        by_area.setdefault(n["area"], []).append(n)

    count = 0
    for area, nodes in by_area.items():
        db_path = os.path.join(db_dir, f"{area}.db")
        if not os.path.exists(db_path):
            print(f"  WARNING: {db_path} not found, skipping {area}")
            continue

        db = sqlite3.connect(db_path)
        cursor = db.cursor()

        for n in nodes:
            cursor.execute(
                "DELETE FROM scripts WHERE owner_type='mob' AND owner_vnum=? "
                "AND name LIKE 'story_%'",
                (n["vnum"],))

            cursor.execute(
                "INSERT INTO scripts (owner_type, owner_vnum, trigger, name, "
                "code, pattern, chance, sort_order) "
                "VALUES ('mob', ?, ?, ?, ?, ?, 0, ?)",
                (n["vnum"], TRIG_SPEECH, n["name"], n["lua_code"],
                 n["pattern"], n["sort_order"]))
            print(f"  {area}.db: mob {n['vnum']} <- {n['name']} "
                  f"(sort={n['sort_order']})")
            count += 1

        db.commit()
        db.close()
    return count


def seed_death_scripts(db_dir):
    """Insert TRIG_DEATH story scripts on mob templates."""
    by_area = {}
    for n in DEATH_NODES:
        by_area.setdefault(n["area"], []).append(n)

    count = 0
    for area, nodes in by_area.items():
        db_path = os.path.join(db_dir, f"{area}.db")
        if not os.path.exists(db_path):
            print(f"  WARNING: {db_path} not found, skipping {area}")
            continue

        db = sqlite3.connect(db_path)
        cursor = db.cursor()

        for n in nodes:
            cursor.execute(
                "DELETE FROM scripts WHERE owner_type='mob' AND owner_vnum=? "
                "AND name LIKE 'story_kill_%'",
                (n["vnum"],))

            cursor.execute(
                "INSERT INTO scripts (owner_type, owner_vnum, trigger, name, "
                "code, pattern, chance, sort_order) "
                "VALUES ('mob', ?, ?, ?, ?, NULL, 0, 99)",
                (n["vnum"], TRIG_DEATH, n["name"], n["lua_code"]))
            print(f"  {area}.db: mob {n['vnum']} <- {n['name']} (DEATH)")
            count += 1

        db.commit()
        db.close()
    return count


def seed_examine_scripts(db_dir):
    """Insert TRIG_EXAMINE story scripts on rooms."""
    by_area = {}
    for n in EXAMINE_NODES:
        by_area.setdefault(n["area"], []).append(n)

    count = 0
    for area, nodes in by_area.items():
        db_path = os.path.join(db_dir, f"{area}.db")
        if not os.path.exists(db_path):
            print(f"  WARNING: {db_path} not found, skipping {area}")
            continue

        db = sqlite3.connect(db_path)
        cursor = db.cursor()

        for n in nodes:
            cursor.execute(
                "DELETE FROM scripts WHERE owner_type='room' AND owner_vnum=? "
                "AND name LIKE 'story_examine_%'",
                (n["room_vnum"],))

            cursor.execute(
                "INSERT INTO scripts (owner_type, owner_vnum, trigger, name, "
                "code, pattern, chance, sort_order) "
                "VALUES ('room', ?, ?, ?, ?, ?, 0, 99)",
                (n["room_vnum"], TRIG_EXAMINE, n["name"], n["lua_code"],
                 n["pattern"]))
            print(f"  {area}.db: room {n['room_vnum']} <- {n['name']} (EXAMINE)")
            count += 1

        db.commit()
        db.close()
    return count


def seed_extra_descs(db_dir):
    """Insert extra descriptions on rooms for examine nodes."""
    by_area = {}
    for ed in EXTRA_DESCS:
        by_area.setdefault(ed["area"], []).append(ed)

    count = 0
    for area, descs in by_area.items():
        db_path = os.path.join(db_dir, f"{area}.db")
        if not os.path.exists(db_path):
            print(f"  WARNING: {db_path} not found, skipping {area}")
            continue

        db = sqlite3.connect(db_path)
        cursor = db.cursor()

        for ed in descs:
            cursor.execute(
                "DELETE FROM extra_descriptions WHERE owner_type='room' "
                "AND owner_vnum=? AND keyword=?",
                (ed["room_vnum"], ed["keyword"]))

            cursor.execute(
                "INSERT INTO extra_descriptions "
                "(owner_type, owner_vnum, keyword, description, sort_order) "
                "VALUES ('room', ?, ?, ?, 99)",
                (ed["room_vnum"], ed["keyword"], ed["description"]))
            print(f"  {area}.db: room {ed['room_vnum']} <- "
                  f"extra_desc '{ed['keyword'][:30]}...'")
            count += 1

        db.commit()
        db.close()
    return count


def seed_objects(db_dir):
    """Insert ITEM_TREASURE object templates for fetch quests."""
    by_area = {}
    for o in OBJECTS:
        by_area.setdefault(o["area"], []).append(o)

    count = 0
    for area, objs in by_area.items():
        db_path = os.path.join(db_dir, f"{area}.db")
        if not os.path.exists(db_path):
            print(f"  WARNING: {db_path} not found, skipping {area}")
            continue

        db = sqlite3.connect(db_path)
        cursor = db.cursor()

        for o in objs:
            cursor.execute("DELETE FROM objects WHERE vnum=?", (o["vnum"],))

            cursor.execute(
                "INSERT INTO objects (vnum, name, short_descr, description, "
                "item_type, extra_flags, wear_flags, "
                "value0, value1, value2, value3, weight, cost, "
                "chpoweron, chpoweroff, chpoweruse, "
                "victpoweron, victpoweroff, victpoweruse, "
                "spectype, specpower) "
                "VALUES (?, ?, ?, ?, 8, 0, 1, "
                "0, 0, 0, 0, 1, 0, "
                "'(null)', '(null)', '(null)', "
                "'(null)', '(null)', '(null)', "
                "0, 0)",
                (o["vnum"], o["name"], o["short_descr"], o["description"]))
            print(f"  {area}.db: obj {o['vnum']} <- '{o['short_descr']}'")
            count += 1

        db.commit()
        db.close()
    return count


def seed_resets(db_dir):
    """Insert 'O' resets to place fetch quest objects in rooms."""
    by_area = {}
    for r in RESETS:
        by_area.setdefault(r["area"], []).append(r)

    count = 0
    for area, resets in by_area.items():
        db_path = os.path.join(db_dir, f"{area}.db")
        if not os.path.exists(db_path):
            print(f"  WARNING: {db_path} not found, skipping {area}")
            continue

        db = sqlite3.connect(db_path)
        cursor = db.cursor()

        for r in resets:
            cursor.execute(
                "DELETE FROM resets WHERE command='O' AND arg1=?",
                (r["obj_vnum"],))

            cursor.execute(
                "SELECT COALESCE(MAX(sort_order), 0) FROM resets")
            max_sort = cursor.fetchone()[0]

            cursor.execute(
                "INSERT INTO resets (command, arg1, arg2, arg3, sort_order) "
                "VALUES ('O', ?, 0, ?, ?)",
                (r["obj_vnum"], r["room_vnum"], max_sort + 1))
            print(f"  {area}.db: reset O {r['obj_vnum']} -> "
                  f"room {r['room_vnum']}")
            count += 1

        db.commit()
        db.close()
    return count


def main():
    parser = argparse.ArgumentParser(
        description="Seed story quest data into area databases.")
    parser.add_argument("--db-dir", default=None,
        help="Path to area database directory (default: gamedata/db/areas/)")
    args = parser.parse_args()

    if args.db_dir:
        db_dir = args.db_dir
    else:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(os.path.dirname(script_dir))
        db_dir = os.path.join(project_root, "gamedata", "db", "areas")

    if not os.path.isdir(db_dir):
        print(f"ERROR: Database directory not found: {db_dir}")
        sys.exit(1)

    print(f"Seeding story quest into: {db_dir}")
    print()

    print("--- Speech scripts (TRIG_SPEECH on mobs) ---")
    n_speech = seed_speech_scripts(db_dir)
    print()

    print("--- Death scripts (TRIG_DEATH on mobs) ---")
    n_death = seed_death_scripts(db_dir)
    print()

    print("--- Examine scripts (TRIG_EXAMINE on rooms) ---")
    n_examine = seed_examine_scripts(db_dir)
    print()

    print("--- Extra descriptions (room examine targets) ---")
    n_ed = seed_extra_descs(db_dir)
    print()

    print("--- Object templates (fetch quest items) ---")
    n_obj = seed_objects(db_dir)
    print()

    print("--- Object resets (place items in rooms) ---")
    n_reset = seed_resets(db_dir)
    print()

    total = n_speech + n_death + n_examine + n_ed + n_obj + n_reset
    print(f"Done. {total} entries seeded:")
    print(f"  {n_speech} speech scripts, {n_death} death scripts, "
          f"{n_examine} examine scripts")
    print(f"  {n_ed} extra descriptions, {n_obj} objects, {n_reset} resets")
    print("Restart the MUD server to load the new data.")


if __name__ == "__main__":
    main()
