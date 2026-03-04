#!/usr/bin/env python3
"""
seed_story_quest.py - Insert story quest data into area databases.

"Echoes of the Sundering" — a breadcrumb-driven narrative quest with
SPEECH, KILL, EXAMINE, and FETCH activities interspersed.

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
# Helpers to build each type of entry
# ============================================================

def speech_node(area, vnum, pattern, req_node, next_node, npc_label, clue, lua_code):
    """Register a TRIG_SPEECH script on a mob."""
    SPEECH_NODES.append({
        "area": area, "vnum": vnum, "pattern": pattern,
        "lua_code": lua_code,
        "name": f"story_node_{req_node}_{npc_label}",
    })


def talk(area, vnum, pattern, req_node, next_node, npc_label, clue, lines):
    """Standard talk node — mob delivers dialogue and advances quest."""
    dialogue = "\\n\\r".join(lines)
    clue_escaped = clue.replace('"', '\\"')
    dialogue_escaped = dialogue.replace('"', '\\"')
    lua_code = f'''function on_speech(mob, ch, text)
  if ch:is_npc() then return end
  if ch:story_node() ~= {req_node} then return end
  ch:send("\\n\\r{dialogue_escaped}\\n\\r")
  ch:set_story_node({next_node})
  ch:set_story_clue("{clue_escaped}")
end'''
    speech_node(area, vnum, pattern, req_node, next_node, npc_label, clue, lua_code)


def fetch_talk(area, vnum, pattern, req_node, next_node, npc_label, clue,
               obj_vnum, missing_lines, has_lines):
    """Fetch-receiving NPC — checks has_object, takes item, advances."""
    missing_dia = "\\n\\r".join(missing_lines)
    has_dia = "\\n\\r".join(has_lines)
    clue_escaped = clue.replace('"', '\\"')
    missing_escaped = missing_dia.replace('"', '\\"')
    has_escaped = has_dia.replace('"', '\\"')
    lua_code = f'''function on_speech(mob, ch, text)
  if ch:is_npc() then return end
  if ch:story_node() ~= {req_node} then return end
  if not ch:has_object({obj_vnum}) then
    ch:send("\\n\\r{missing_escaped}\\n\\r")
    return
  end
  ch:take_object({obj_vnum})
  ch:send("\\n\\r{has_escaped}\\n\\r")
  ch:set_story_node({next_node})
  ch:set_story_clue("{clue_escaped}")
end'''
    speech_node(area, vnum, pattern, req_node, next_node, npc_label, clue, lua_code)


def kill_node(area, mob_vnum, req_node, next_node, npc_label, clue, lines):
    """TRIG_DEATH on a mob template — fires when player kills it."""
    dialogue = "\\n\\r".join(lines)
    clue_escaped = clue.replace('"', '\\"')
    dialogue_escaped = dialogue.replace('"', '\\"')
    lua_code = f'''function on_death(killer, mob_vnum, area_low, area_high)
  if killer:is_npc() then return end
  if killer:story_node() ~= {req_node} then return end
  killer:send("\\n\\r{dialogue_escaped}\\n\\r")
  killer:set_story_node({next_node})
  killer:set_story_clue("{clue_escaped}")
end'''
    DEATH_NODES.append({
        "area": area, "vnum": mob_vnum,
        "lua_code": lua_code,
        "name": f"story_kill_{req_node}_{npc_label}",
    })


def examine_node(area, room_vnum, pattern, req_node, next_node, label, clue, lines):
    """TRIG_EXAMINE on a room — fires when player examines keyword."""
    dialogue = "\\n\\r".join(lines)
    clue_escaped = clue.replace('"', '\\"')
    dialogue_escaped = dialogue.replace('"', '\\"')
    lua_code = f'''function on_examine(ch, room, keyword)
  if ch:is_npc() then return end
  if ch:story_node() ~= {req_node} then return end
  ch:send("\\n\\r{dialogue_escaped}\\n\\r")
  ch:set_story_node({next_node})
  ch:set_story_clue("{clue_escaped}")
end'''
    EXAMINE_NODES.append({
        "area": area, "room_vnum": room_vnum, "pattern": pattern,
        "lua_code": lua_code,
        "name": f"story_examine_{req_node}_{label}",
    })


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
# Act 1: "The World As It Is" (Nodes 1-4)
# Node 1: TALK (Executioner), Node 2: TALK (Henry),
# Node 3: KILL (Skeleton), Node 4: FETCH deliver (Priest)
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

talk("grave", 3600, "darkness", 2, 3, "henry",
    "#CHenry the Gardener in the Graveyard told you:#n "
    "\"Go into the tombs. Fight what stirs there. Bring back what you find on the bones.\"",
    [
        "#CHenry stops mid-dig and looks at you with tired, red-rimmed eyes.#n",
        "#C#n",
        "#C\"Darkness? Aye, that's one word for it. I've got a simpler one: wrong.#n",
        "#CEverything's gone wrong down here. They won't stay buried. Used to be#n",
        "#Ca quiet job, this. Now they claw up from the soil with marks on their#n",
        "#Cbones I've never seen -- symbols, like writing from somewhere else#n",
        "#Centirely. Go into the tombs. Fight what stirs there. And bring back#n",
        "#Cwhat you find on the bones -- there's a priest up at the chapel who#n",
        "#Ccollects strange writings. He'll want to see it.\"#n",
    ])

# Node 3: KILL — Dusty Skeleton (mob 3604, graveyard)
kill_node("grave", 3604, 3, 4, "skeleton",
    "#CYou killed the restless dead in the Graveyard.#n "
    "\"Look for a marked bone fragment in the deeper tombs. Bring it to the Priest at the Chapel.\"",
    [
        "#CThe skeleton crumbles to dust, and among the fragments you notice#n",
        "#Cstrange symbols etched into the bones -- the same marks Henry described.#n",
        "#CThe dead ARE restless here. Something is very wrong.#n",
        "#C#n",
        "#CYou should search the deeper tombs for a bone fragment covered in#n",
        "#Cthose symbols. The Priest at the Chapel would want to see it.#n",
    ])

# Fetch object: marked bone fragment
obj_template("grave", 3699, "bone fragment marked symbols",
    "a marked bone fragment",
    "A small bone fragment covered in strange symbols lies here.")
obj_reset("grave", 3699, 3607)

# Node 4: FETCH deliver — Priest takes bone, tells about Sundering
fetch_talk("chapel", 3405, "marks", 4, 5, "priest",
    "#CThe Priest in the Chapel told you:#n "
    "\"The druids in the Holy Grove keep the oldest memories. Ask about the Sundering.\"",
    3699,
    [
        "#CThe Priest looks up from his reading.#n",
        "#C#n",
        "#C\"You've seen the marks? Then bring me proof -- a fragment from the#n",
        "#Ctombs, something I can study. The bones down there carry symbols I've#n",
        "#Cnever been able to decipher.\"#n",
    ],
    [
        "#CThe Priest takes the bone fragment and holds it to the candlelight,#n",
        "#Chis hands trembling.#n",
        "#C#n",
        "#C\"These marks... I knew it. The same symbols I found on the oldest#n",
        "#Cstones in the catacombs. They speak of something called 'the Sundering'#n",
        "#C-- a time when the walls of the world grew thin and things bled through.#n",
        "#CI've studied them for years and I'm no closer to understanding. But the#n",
        "#Cdruids in the Holy Grove -- they keep the oldest memories. Living#n",
        "#Cmemories, held in the roots of ancient trees. They might remember what#n",
        "#Cthe rest of us have forgotten.\"#n",
    ])

# ============================================================
# Act 2: "What Came Before" (Nodes 5-12)
# ============================================================

talk("grove", 8900, "sundering", 5, 6, "hierophant",
    "#CThe Hierophant in the Holy Grove told you:#n "
    "\"Travel to the Elemental Canyon and see the discord for yourself.\"",
    [
        "#CThe Hierophant's eyes fly open and she pulls her hand from the oak as#n",
        "#Cif burned.#n",
        "#C#n",
        "#C\"Where did you learn that word? The Sundering... we do not speak it#n",
        "#Clightly here. The trees shudder when it's uttered -- even now, can you#n",
        "#Cfeel it?\" She steadies herself. \"The land remembers what people forget.#n",
        "#CBefore the breaking, each world was whole and separate. Now even the#n",
        "#Celements are in discord -- fire burns where it shouldn't, lightning#n",
        "#Cstrikes from clear skies. Don't take my word for it. Travel to the#n",
        "#CElemental Canyon and see for yourself. The rulers there are... agitated.#n",
        "#CThey remember their true homes.\"#n",
    ])

talk("canyon", 9208, "sundering", 6, 7, "earth_ruler",
    "#CThe Earth Ruler in the Elemental Canyon told you:#n "
    "\"Find the inscription in the deep tunnels of Moria. See them with your own eyes.\"",
    [
        "#CThe ground trembles. The Earth Ruler's voice grinds like stone on stone.#n",
        "#C#n",
        "#C\"You dare speak that word here? The Sundering...\" A crack splits the#n",
        "#Crock beneath your feet. \"...is the reason I exist in this wretched#n",
        "#Cplace. We are not of this world. None of the elemental lords are. We#n",
        "#Cwere pulled here when the barriers shattered, torn from realms where we#n",
        "#Cwere whole. The elements rage because they remember what was taken from#n",
        "#Cthem. If you would understand what broke the world, go deeper. Find the#n",
        "#Cinscription in the deep tunnels of Moria. See them with your own eyes#n",
        "#C-- carvings older than us, older than the breaking itself.\"#n",
    ])

# Node 7: EXAMINE — Moria carvings (room 4160)
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

examine_node("moria", 4160, "carvings", 7, 8, "moria_carvings",
    "#CYou examined the ancient carvings in Moria.#n "
    "\"Find someone who studies these -- the Mage deeper in the mines.\"",
    [
        "#CAs you trace the carvings, a chill runs through you. These aren't#n",
        "#Cdecorations -- they're a record. A warning. The spirals depict a city#n",
        "#Cthat reached between worlds... and what happened when the reaching#n",
        "#Cwent wrong. Someone who studies these tunnels might know more. You#n",
        "#Cremember hearing about a Mage deeper in the mines.#n",
    ])

talk("moria", 4100, "carvings", 8, 9, "the_mage",
    "#CThe Mage in Moria told you:#n "
    "\"His journal is in the study down the hall. Take it to the Librarian in Thalos.\"",
    [
        "#CThe Mage's eyes glow faintly in the dark of the mines. He steps closer,#n",
        "#Csuddenly intense.#n",
        "#C#n",
        "#C\"The carvings. You found them? Tell me -- the ones near the deep shaft,#n",
        "#Cwith the spiral pattern? I've spent lifetimes studying those. They depict#n",
        "#Ca city -- a great city that existed before all of this. Its people#n",
        "#Clearned to walk between worlds. And then... the carvings stop. Mid-stroke,#n",
        "#Cas if the hand that carved them simply ceased to exist. Something went#n",
        "#Cterribly wrong. A sorcerer in the High Tower tried to finish the story.#n",
        "#CHis journal is in the study down the hall. Take it to the Librarian in#n",
        "#CThalos -- she'll know what to make of it.\"#n",
    ])

# Fetch object: sorcerer's journal (pickup at node 9)
obj_template("hitower", 1499, "journal sorcerer notes leather",
    "a sorcerer's journal",
    "A leather-bound journal lies open on the desk.")
obj_reset("hitower", 1499, 1367)

# Node 9: FETCH deliver — Librarian takes journal
fetch_talk("mirror", 5315, "thalos", 9, 10, "librarian",
    "#CThe Librarian in Old Thalos told you:#n "
    "\"The Drow were pulled underground from another world. "
    "Ask their Priestess about displacement.\"",
    1499,
    [
        "#CThe Librarian carefully closes a crumbling tome and fixes you with a#n",
        "#Csharp look.#n",
        "#C#n",
        "#C\"You came about Thalos? Then you'll need more than words. Show me the#n",
        "#Csorcerer's journal from the tower -- I've heard rumors of it but never#n",
        "#Cseen the pages myself.\"#n",
    ],
    [
        "#CThe Librarian takes the journal and begins reading immediately, her#n",
        "#Ceyes widening with every page.#n",
        "#C#n",
        "#C\"This confirms everything. Thalos wasn't destroyed by war or plague.#n",
        "#CIt was replaced. One morning the city was there, whole and living. The#n",
        "#Cnext, this ruin stood in its place -- as if copied from a dream and set#n",
        "#Cdown wrong. The journal describes the same forces at work. I'll keep#n",
        "#Cthis safe. You should speak to the Drow -- an entire civilization#n",
        "#Cpulled underground from another world entirely. If anyone understands#n",
        "#Cwhat it means to be displaced, it's them.\"#n",
    ])

talk("drow", 5104, "displacement", 10, 11, "priestess",
    "#CThe Drow Priestess told you:#n "
    "\"Find the warning tablets in the ancient hall of the pyramid. "
    "Bring them to the Sphinx.\"",
    [
        "#CThe Drow Priestess regards you with cold, ancient eyes. A thin smile#n",
        "#Ccrosses her lips.#n",
        "#C#n",
        "#C\"Displacement. A surfacer's word for what happened to us. So clinical.#n",
        "#CSo tidy. Let me tell you what 'displacement' feels like: our entire#n",
        "#Ccity was torn from the Underdark of a world that no longer exists and#n",
        "#Cdeposited beneath yours. The screaming lasted for days. We adapted --#n",
        "#Cthe Drow always adapt. You want to understand the scope of this? Find#n",
        "#Cthe warning tablets in the ancient hall of the pyramid -- writings far#n",
        "#Colder than our arrival. Bring them to the Sphinx. It has guarded those#n",
        "#Cwarnings for eons.\"#n",
    ])

# Fetch object: warning tablet (pickup at node 11)
obj_template("pyramid", 2699, "tablet warning stone hieroglyphs",
    "a stone warning tablet",
    "A stone tablet covered in ancient hieroglyphs leans against the wall.")
obj_reset("pyramid", 2699, 2630)

# Node 11: FETCH deliver — Sphinx takes tablet
fetch_talk("pyramid", 2616, "cycle", 11, 12, "sphinx",
    "#CThe Great Sphinx told you:#n "
    "\"Seek the place between -- the void where the dreamer waits.\"",
    2699,
    [
        "#CThe Great Sphinx's stone eyes focus on you.#n",
        "#C#n",
        "#C\"You speak of the cycle. But where is the proof? The tablets from the#n",
        "#Cancient hall -- bring them to me. Only then can I share what I guard.\"#n",
    ],
    [
        "#CThe Great Sphinx takes the tablet in massive stone paws and reads the#n",
        "#Chieroglyphs with ancient recognition.#n",
        "#C#n",
        "#C\"Yes. These are the warnings. The cycle is thus: a world reaches far#n",
        "#Cenough to touch another. The touch becomes a tear. The tear becomes a#n",
        "#Cflood. All that was separate becomes one. I have guarded these warnings#n",
        "#Cthrough more turnings than you can imagine, and the message is always#n",
        "#Cthe same: this is not the first Sundering, child. It will not be the#n",
        "#Clast. If you wish to see where the walls are thinnest -- where the#n",
        "#Ccycle's edge cuts closest -- seek the place between. The void where the#n",
        "#Cdreamer waits.\"#n",
    ])

# ============================================================
# Act 3: "The Convergence" (Nodes 12-16)
# ============================================================

talk("dream", 8600, "between", 12, 13, "keeper",
    "#CThe Keeper in the Void told you:#n "
    "\"Seek the gods on Olympus, or the king beneath the waves. "
    "Ask about the convergence.\"",
    [
        "#CThe Keeper speaks without moving, its voice arriving from everywhere#n",
        "#Cand nowhere.#n",
        "#C#n",
        "#C\"Between. Yes. That is the only honest word for this place. You stand#n",
        "#Cbetween what was and what will be -- between worlds that were once#n",
        "#Cseparate and are now tangled together. Through one tear, an ocean where#n",
        "#Ca king rules waters that aren't his. Through another, a mountain where#n",
        "#Cgods sit in exile, pretending they still matter.\"#n",
        "#C#n",
        "#CA long silence.#n",
        "#C#n",
        "#C\"The Unraveling doesn't just pull across space. It pulls across time.#n",
        "#CWhat you call the future is already bleeding through. Seek the gods on#n",
        "#COlympus, or the king beneath the waves. Ask them about the convergence.#n",
        "#CThey know what it means to be torn from one world and dropped into#n",
        "#Canother.\"#n",
    ])

# Branch: Atlantis (13a) or Olympus (13b) -> 14
talk("atlantis", 8103, "convergence", 13, 14, "neptune",
    "#CKing Neptune in Atlantis told you:#n "
    "\"Ask the judges in the city of iron and glass about the future.\"",
    [
        "#CKing Neptune strokes his coral beard, his trident dimming at the word.#n",
        "#C#n",
        "#C\"The convergence. So you've learned its name. Most who swim these waters#n",
        "#Cthink they were always here. They weren't. My kingdom rests in waters#n",
        "#Cthat belong to another world. We arrived when the barriers fell, and we#n",
        "#Chave made our peace with it. But the newcomers -- the ones from the#n",
        "#Cstrange metal cities -- they arrived later. And stranger. As if pulled#n",
        "#Cfrom a time that hasn't fully happened yet. Ask the judges in the city#n",
        "#Cof iron and glass. They patrol streets that shouldn't exist yet -- and#n",
        "#Cthey don't even know it.\"#n",
    ])

talk("olympus", 901, "convergence", 13, 14, "zeus",
    "#CZeus on Olympus told you:#n "
    "\"Seek the metal city. Something there reeks of a future that was never meant to be.\"",
    [
        "#CZeus sits upon his throne, looking tired in a way that gods should not.#n",
        "#CHe regards you for a long moment.#n",
        "#C#n",
        "#C\"The convergence. I had hoped that word would die with the old city. You#n",
        "#Cmortals think us gods. We were merely... from elsewhere. A world where#n",
        "#Cwe were powerful, yes. But that world is gone -- scattered across the#n",
        "#CSundering like all the rest. We are refugees wearing the faces of#n",
        "#Clegends. Seek the metal city to the north. Something there reeks of a#n",
        "#Cfuture that was never meant to be.\"#n",
    ])

talk("mega1", 8010, "future", 14, 15, "judge",
    "#CJudge Eckersley in Mega-City One told you:#n "
    "\"Go up to the bridge of the dome ship, look at the holo-display. "
    "See where the threads lead.\"",
    [
        "#CJudge Eckersley adjusts his helmet and squints at you.#n",
        "#C#n",
        "#C\"Future? The future? This IS the future, citizen.\" He pauses.#n",
        "#C\"...isn't it?\"#n",
        "#C#n",
        "#CA flicker of confusion crosses his face.#n",
        "#C#n",
        "#C\"Sometimes I get these flashes -- like I remember a different sky.#n",
        "#CDifferent laws. Then it's gone and I'm back on patrol. Look, there's a#n",
        "#Cship in orbit. Aliens with instruments -- sensors, scanners. They say#n",
        "#Cthey can see where all the threads come from. Go up to the bridge, look#n",
        "#Cat the holo-display. See for yourself where the threads lead.\"#n",
    ])

# Node 15: EXAMINE — Dome Ship sensors (room 93045)
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

examine_node("domeship", 93045, "sensors", 15, 16, "domeship_sensors",
    "#CYou examined the sensor display on the Dome Ship.#n "
    "\"Every thread traces back to one origin -- an old city. Talk to Elfangor.\"",
    [
        "#CThe holographic threads pulse and converge before your eyes. Every#n",
        "#Cfragment of this patchwork world -- every displaced civilization, every#n",
        "#Ctorn reality -- traces back to a single point. An old city. The origin#n",
        "#Cof everything. The Andalite commander Elfangor might know more about#n",
        "#Cwhat these readings mean.#n",
    ])

talk("domeship", 93006, "origin", 16, 17, "elfangor",
    "#CElfangor on the Dome Ship told you:#n "
    "\"The old city is at the center. But it is guarded. Fight your way through.\"",
    [
        "#CElfangor-Sirinial-Shamtul turns all four eyes toward you, and you sense#n",
        "#Ca vast, alien sadness.#n",
        "#C#n",
        "#C\"The origin. You have come asking the same question we have spent cycles#n",
        "#Ctrying to answer. Our sensors have mapped the convergence -- every#n",
        "#Cfragment of reality in this patchwork world traces back to a single#n",
        "#Cpoint. An old city, at the center of everything. But our readings show#n",
        "#Csomething else: the temporal distortions aren't random. There is a#n",
        "#Cpattern. Something is threading through the timelines, pulling them#n",
        "#Ctogether. The old city may hold that answer. But be warned -- it is#n",
        "#Cguarded. The Royal Guard still patrols those ruins. You will need to#n",
        "#Cfight your way through.\"#n",
    ])

# ============================================================
# Act 4: "The Old City" (Nodes 17-21)
# ============================================================

# Node 17: KILL — Royal Guard (mob 30400, dystopia)
kill_node("dystopia", 30400, 17, 18, "royal_guard",
    "#CYou defeated a Royal Guard of Old Dystopia.#n "
    "\"The Queen holds court deeper in the ruins. Seek her out.\"",
    [
        "#CThe Royal Guard falls, his ancient armor clattering on the stone. With#n",
        "#Chis last breath he whispers:#n",
        "#C#n",
        "#C\"You... you've come to see the Queen? Then go. She's waited long enough#n",
        "#Cfor someone to ask the right questions. Deeper in the ruins. She holds#n",
        "#Ccourt still... after all this time...\"#n",
    ])

talk("dystopia", 30508, "old city", 18, 19, "queen",
    "#CThe Queen of Old Dystopia told you:#n "
    "\"Go to the library. Read the records. See what we did.\"",
    [
        "#CThe Queen of Dystopia looks at you with eyes that have seen too much.#n",
        "#CA sad smile crosses her face.#n",
        "#C#n",
        "#C\"The old city. You called it that -- everyone does now. We just called#n",
        "#Cit home. So you've come at last. They all do, eventually -- the ones#n",
        "#Cwho ask questions instead of swinging swords. This was the first city.#n",
        "#CThe great city. We learned to reach between worlds, and for a time, it#n",
        "#Cwas glorious. And then we reached too far, and everything came pouring#n",
        "#Cin. Before you speak to the King, go to the library. Read the records.#n",
        "#CSee what we did -- in our own words.\"#n",
    ])

# Node 19: EXAMINE — Library records (room 30460)
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

examine_node("dystopia", 30460, "records", 19, 20, "dystopia_records",
    "#CYou read the records in the library of Old Dystopia.#n "
    "\"They opened the doors between worlds and couldn't close them. "
    "The King carries the weight of it. Ask him about the Unraveling.\"",
    [
        "#CThe records tell the whole terrible story. They opened the doors between#n",
        "#Cworlds -- carefully at first, then recklessly. They saw wonders. And#n",
        "#Cthen the doors wouldn't close. Reality merged. The barriers fell. Every#n",
        "#Cworld they'd touched came flooding in. The graveyard's restless dead,#n",
        "#Cthe displaced Drow, the exiled gods -- all of it traces back to this#n",
        "#Ccity and its experiments. The King still rules here. He carries the#n",
        "#Cweight of it. Ask him about the Unraveling.#n",
    ])

talk("dystopia", 30509, "unraveling", 20, 21, "king",
    "#CThe King of Old Dystopia told you:#n "
    "\"The world didn't end. It changed. Seek Heaven or Hell for the last word.\"",
    [
        "#CThe King of Dystopia stares out over the ruins of his city. When he#n",
        "#Cspeaks, his voice is quiet.#n",
        "#C#n",
        "#C\"The Unraveling. That's what they call it now. We called it progress.#n",
        "#CWe were so certain we could control it -- open doors between worlds#n",
        "#Cand close them at will. We were wrong. The doors don't close. They#n",
        "#Cnever did. Every world we touched was pulled into ours, and ours into#n",
        "#Ctheirs, until none of it made sense anymore. Smiling blue creatures in#n",
        "#Cmushroom houses. Metal cities with armored judges. Gods from stories#n",
        "#Cthat were supposed to be fiction. All real. All here. All because of us.\"#n",
        "#C#n",
        "#CHe is silent for a long moment.#n",
        "#C#n",
        "#C\"But you know what I've learned, standing in these ruins? The world#n",
        "#Cdidn't end. It changed. The people -- the ones who were pulled here,#n",
        "#Cthe ones who were born here -- they built new lives in the wreckage.#n",
        "#CThat's not a tragedy. That's just... what people do.\"#n",
        "#C#n",
        "#C\"If you want the last word on all of this, seek Heaven or Hell. The#n",
        "#Cangels and demons have their own opinions on what comes next.\"#n",
    ])

# ============================================================
# Epilogue: "What Remains" (Nodes 21a/21b -> 99)
# ============================================================

talk("heaven", 99004, "what now", 21, 99, "overseer",
    "#CYou have completed 'Echoes of the Sundering.'#n",
    [
        "#CThe Overseer gazes down from a throne of light, and for a moment,#n",
        "#Csomething almost like pity crosses its luminous face.#n",
        "#C#n",
        "#C\"What now? That's what you ask, after everything you've learned? After#n",
        "#Cwalking through the wreckage of a dozen worlds?\" It leans forward.#n",
        "#C\"The angels would have you believe we can restore the barriers. Separate#n",
        "#Cthe worlds again. Put everything back the way it was. Perhaps they're#n",
        "#Cright. Perhaps the Sundering can be undone. But ask yourself -- would#n",
        "#Cthe people in those fragments want to go back? Would you erase#n",
        "#Ceverything they've built here, every story they've lived, just to make#n",
        "#Cthe world make sense again?\"#n",
        "#C#n",
        "#CYou stand in Heaven, at the edge of understanding. The Sundering broke#n",
        "#Cthe world -- or made it. The Unraveling continues -- or always has.#n",
        "#CThe truth is that no one knows, and perhaps no one ever will. But the#n",
        "#Cstories remain. The worlds remain. And as long as someone walks through#n",
        "#Cthem and listens, they are not forgotten.#n",
        "#C#n",
        "#G[Quest Complete: Echoes of the Sundering]#n",
    ])

talk("hell", 30101, "what now", 21, 99, "pitlord",
    "#CYou have completed 'Echoes of the Sundering.'#n",
    [
        "#CThe Pit Lord laughs, a sound like breaking stone.#n",
        "#C#n",
        "#C\"What now, it asks! Ha! You walk through a shattered world, piece#n",
        "#Ctogether its sorry history, and your question is 'what now?' I like#n",
        "#Cyou.\" The laughter dies. \"The demons don't want to fix anything. They#n",
        "#Cwant to pull more through. More worlds, more chaos, more power. And#n",
        "#Cmaybe they're right. But the force that threads through time -- the#n",
        "#Cdark current beneath all of this -- it doesn't care what angels or#n",
        "#Cdemons want. It just... is. Like a river. You can swim in it. You can#n",
        "#Cdrown in it. But you can't stop it.\"#n",
        "#C#n",
        "#CYou stand in Hell, at the edge of understanding. The Sundering broke#n",
        "#Cthe world -- or made it. The Unraveling continues -- or always has.#n",
        "#CThe truth is that no one knows, and perhaps no one ever will. But the#n",
        "#Cstories remain. The worlds remain. And as long as someone walks through#n",
        "#Cthem and listens, they are not forgotten.#n",
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
                "VALUES ('mob', ?, ?, ?, ?, ?, 0, 99)",
                (n["vnum"], TRIG_SPEECH, n["name"], n["lua_code"], n["pattern"]))
            print(f"  {area}.db: mob {n['vnum']} <- {n['name']}")
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
            print(f"  {area}.db: room {ed['room_vnum']} <- extra_desc '{ed['keyword']}'")
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

            # Get max sort_order for this area's resets
            cursor.execute("SELECT COALESCE(MAX(sort_order), 0) FROM resets")
            max_sort = cursor.fetchone()[0]

            cursor.execute(
                "INSERT INTO resets (command, arg1, arg2, arg3, sort_order) "
                "VALUES ('O', ?, 0, ?, ?)",
                (r["obj_vnum"], r["room_vnum"], max_sort + 1))
            print(f"  {area}.db: reset O {r['obj_vnum']} -> room {r['room_vnum']}")
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
