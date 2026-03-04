#!/usr/bin/env python3
"""
seed_story_quest.py - Insert story quest dialogue scripts into area databases.

"Echoes of the Sundering" — a breadcrumb-driven narrative quest.
Each node is a TRIG_SPEECH script on a questgiver mob that checks the
player's story_node and delivers dialogue + advances the node.

Usage:
    python seed_story_quest.py [--db-dir PATH]

Default db-dir: gamedata/db/areas/
"""

import argparse
import os
import sqlite3
import sys

# TRIG_SPEECH = (1 << 1) = 2
TRIG_SPEECH = 2

# Each story node: (area_db, mob_vnum, pattern, node_required, next_node, clue_text, dialogue_lua)
# pattern: substring match on player's speech (case-insensitive)
# node_required: player must have this story_node to trigger
# next_node: what story_node gets set to after dialogue

STORY_NODES = []

def node(area, vnum, pattern, req_node, next_node, npc_label, clue, lines):
    """Helper to build a story node entry."""
    # Build the Lua on_speech function
    # mob = the NPC, ch = the player who spoke, text = what they said
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

    STORY_NODES.append({
        "area": area,
        "vnum": vnum,
        "pattern": pattern,
        "lua_code": lua_code,
        "name": f"story_node_{req_node}_{npc_label}",
    })


# ============================================================
# Act 1: "The World As It Is" (Nodes 1-5)
# ============================================================

node("midgaard", 3011, "darkness", 1, 2, "executioner",
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

node("grave", 3600, "darkness", 2, 3, "henry",
    "#CHenry the Gardener in the Graveyard told you:#n "
    "\"The priest up at the chapel collects strange writings. Show him the marks.\"",
    [
        "#CHenry stops mid-dig and looks at you with tired, red-rimmed eyes.#n",
        "#C#n",
        "#C\"Darkness? Aye, that's one word for it. I've got a simpler one: wrong.#n",
        "#CEverything's gone wrong down here. They won't stay buried. Used to be#n",
        "#Ca quiet job, this. Now they claw up from the soil with marks on their#n",
        "#Cbones I've never seen -- symbols, like writing from somewhere else#n",
        "#Centirely. I'm just a gardener, friend. This is beyond me. The priest#n",
        "#Cup at the chapel, though -- he collects strange writings. Show him the#n",
        "#Cmarks. He might know what they mean.\"#n",
    ])

node("chapel", 3405, "marks", 3, 4, "priest",
    "#CThe Priest in the Chapel told you:#n "
    "\"The druids in the Holy Grove keep the oldest memories. They might remember.\"",
    [
        "#CThe Priest's hand freezes over the page he was reading.#n",
        "#C#n",
        "#C\"The marks? You've seen them too? Show me your hands -- no, it doesn't#n",
        "#Cmatter. I already know what you'll describe. I've found the same symbols#n",
        "#Con the oldest stones deep in these catacombs. They speak of something#n",
        "#Ccalled 'the Sundering' -- a time when the walls of the world grew thin#n",
        "#Cand things... bled through. I've studied them for years and I'm no#n",
        "#Ccloser to understanding. But the druids in the Holy Grove -- they keep#n",
        "#Cthe oldest memories. Living memories, held in the roots of ancient#n",
        "#Ctrees. They might remember what the rest of us have forgotten.\"#n",
    ])

node("grove", 8900, "sundering", 4, 5, "hierophant",
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

node("canyon", 9208, "sundering", 5, 6, "earth_ruler",
    "#CThe Earth Ruler in the Elemental Canyon told you:#n "
    "\"The mines of Moria hold carvings older than anything on the surface.\"",
    [
        "#CThe ground trembles. The Earth Ruler's voice grinds like stone on stone.#n",
        "#C#n",
        "#C\"You dare speak that word here? The Sundering...\" A crack splits the#n",
        "#Crock beneath your feet. \"...is the reason I exist in this wretched#n",
        "#Cplace. We are not of this world. None of the elemental lords are. We#n",
        "#Cwere pulled here when the barriers shattered, torn from realms where we#n",
        "#Cwere whole. The elements rage because they remember what was taken from#n",
        "#Cthem. If you would understand what broke the world, go deeper. The mines#n",
        "#Cof Moria hold carvings older than anything on the surface -- older than#n",
        "#Cus. Older than the breaking itself.\"#n",
    ])

# ============================================================
# Act 2: "What Came Before" (Nodes 6-10)
# ============================================================

node("moria", 4100, "carvings", 6, 7, "the_mage",
    "#CThe Mage in Moria told you:#n "
    "\"There's a tower in the Shadow Grove where sorcerers named the force. "
    "They called it 'the Unraveling.'\"",
    [
        "#CThe Mage's eyes glow faintly in the dark of the mines. He steps closer,#n",
        "#Csuddenly intense.#n",
        "#C#n",
        "#C\"The carvings. You found them? Tell me -- the ones near the deep shaft,#n",
        "#Cwith the spiral pattern? I've spent lifetimes studying those. They depict#n",
        "#Ca city -- a great city that existed before all of this. Its people#n",
        "#Clearned to walk between worlds. And then... the carvings stop. Mid-stroke,#n",
        "#Cas if the hand that carved them simply ceased to exist. Something went#n",
        "#Cterribly wrong. There's a tower in the Shadow Grove where sorcerers#n",
        "#Ctried to finish the story. They had a name for the force that did this.#n",
        "#CThey called it 'the Unraveling.'\"#n",
    ])

node("hitower", 1301, "unraveling", 7, 8, "adventurer",
    "#CThe Lost Adventurer in the High Tower told you:#n "
    "\"There's a ruined city to the east, Thalos. Some say it fell because of what it saw.\"",
    [
        "#CThe Lost Adventurer flinches at the word, clutching a tattered journal#n",
        "#Cto his chest.#n",
        "#C#n",
        "#C\"The Unraveling? Don't -- don't say it so loud. Not here. I came to#n",
        "#Cthis tower looking for answers too. Found this journal instead -- a#n",
        "#Csorcerer's notes. He wrote about experiments, trying to pierce the veil#n",
        "#Cbetween worlds. Said he could see through to 'the other side.' The#n",
        "#Centries get more frantic as they go. The last one just says: 'It sees#n",
        "#Cus back.'\"#n",
        "#C#n",
        "#CThe adventurer shudders.#n",
        "#C#n",
        "#C\"There's a ruined city to the east -- Thalos. It fell before any of us#n",
        "#Cwere born. Some say it fell because of what it saw through the veil.\"#n",
    ])

node("mirror", 5315, "thalos", 8, 9, "librarian",
    "#CThe Librarian in Old Thalos told you:#n "
    "\"The Drow were pulled underground from another world. "
    "If anyone understands displacement, it's them.\"",
    [
        "#CThe Librarian carefully closes a crumbling tome and fixes you with a#n",
        "#Csharp look.#n",
        "#C#n",
        "#C\"You came here asking about Thalos? Then you already know more than#n",
        "#Cmost. Everyone assumes the ruins are ancient -- some old civilization#n",
        "#Cthat crumbled naturally. They're wrong. Thalos wasn't destroyed by war#n",
        "#Cor plague. It was... replaced. One morning the city was there, whole and#n",
        "#Cliving. The next, this ruin stood in its place -- as if copied from a#n",
        "#Cdream and set down wrong. I've preserved what records survived. They#n",
        "#Cmention the Drow -- an entire civilization pulled underground from#n",
        "#Canother world entirely. If anyone understands what it means to be#n",
        "#Cdisplaced, it's them.\"#n",
    ])

node("drow", 5104, "displacement", 9, 10, "priestess",
    "#CThe Drow Priestess told you:#n "
    "\"The pyramid in the eastern desert holds warnings from those who saw it coming.\"",
    [
        "#CThe Drow Priestess regards you with cold, ancient eyes. A thin smile#n",
        "#Ccrosses her lips.#n",
        "#C#n",
        "#C\"Displacement. A surfacer's word for what happened to us. So clinical.#n",
        "#CSo tidy. Let me tell you what 'displacement' feels like: our entire#n",
        "#Ccity was torn from the Underdark of a world that no longer exists and#n",
        "#Cdeposited beneath yours. Or what you call yours. The screaming lasted#n",
        "#Cfor days. We adapted -- the Drow always adapt. Others were not so#n",
        "#Cfortunate. You want to understand the scope of this? The pyramid in#n",
        "#Cthe eastern desert holds writings far older than our arrival -- warnings#n",
        "#Cfrom a civilization that saw the convergence coming and could do nothing#n",
        "#Cto stop it.\"#n",
    ])

node("pyramid", 2616, "cycle", 10, 11, "sphinx",
    "#CThe Great Sphinx told you:#n "
    "\"Seek the place between -- the void where the dreamer waits.\"",
    [
        "#CThe Great Sphinx's stone eyes focus on you, and something ancient stirs#n",
        "#Cbehind them.#n",
        "#C#n",
        "#C\"You speak of the cycle. Good. Most who come here ask about treasure or#n",
        "#Cpower. You ask the right question. The cycle is thus: a world reaches#n",
        "#Cfar enough to touch another. The touch becomes a tear. The tear becomes#n",
        "#Ca flood. All that was separate becomes one. I have guarded these warnings#n",
        "#Cthrough more turnings than you can imagine, and the message is always#n",
        "#Cthe same: this is not the first Sundering, child. It will not be the#n",
        "#Clast. If you wish to see where the walls are thinnest -- where the#n",
        "#Ccycle's edge cuts closest -- seek the place between. The void where the#n",
        "#Cdreamer waits.\"#n",
    ])

# ============================================================
# Act 3: "The Convergence" (Nodes 11-14)
# ============================================================

node("dream", 8600, "between", 11, 12, "keeper",
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
        "#Cgods sit in exile, pretending they still matter. Through another...#n",
        "#Csomething that hasn't happened yet.\"#n",
        "#C#n",
        "#CA long silence.#n",
        "#C#n",
        "#C\"The Unraveling doesn't just pull across space. It pulls across time.#n",
        "#CWhat you call the future is already bleeding through. Seek the gods on#n",
        "#COlympus, or the king beneath the waves. Ask them about the convergence.#n",
        "#CThey know what it means to be torn from one world and dropped into#n",
        "#Canother.\"#n",
    ])

# Branch: Atlantis path (node 12 from either 12a or 12b -> 13)
node("atlantis", 8103, "convergence", 12, 13, "neptune",
    "#CKing Neptune in Atlantis told you:#n "
    "\"Ask the judges in the city of iron and glass. They patrol streets "
    "that shouldn't exist yet.\"",
    [
        "#CKing Neptune strokes his coral beard, his trident dimming at the word.#n",
        "#C#n",
        "#C\"The convergence. So you've learned its name. Most who swim these waters#n",
        "#Cthink they were always here -- that this ocean has always sat beneath#n",
        "#Cthis sky. It hasn't. My kingdom rests in waters that belong to another#n",
        "#Cworld. We arrived when the barriers fell, and we have made our peace#n",
        "#Cwith it. But the newcomers -- the ones from the strange metal cities --#n",
        "#Cthey arrived later. And stranger. As if pulled from a time that hasn't#n",
        "#Cfully happened yet. If you want to understand how deep this convergence#n",
        "#Cgoes, ask the judges in the city of iron and glass. They patrol streets#n",
        "#Cthat shouldn't exist yet -- and they don't even know it.\"#n",
    ])

# Branch: Olympus path (node 12 -> 13)
node("olympus", 901, "convergence", 12, 13, "zeus",
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
        "#Clegends. The convergence that brought us here brought everything here,#n",
        "#Cand the real power -- the force that did this -- threads through time#n",
        "#Citself. Seek the metal city to the north. Something there reeks of a#n",
        "#Cfuture that was never meant to be.\"#n",
    ])

node("mega1", 8010, "future", 13, 14, "judge",
    "#CJudge Eckersley in Mega-City One told you:#n "
    "\"There's a ship in orbit. Aliens with instruments. "
    "Docking bay's through the portal.\"",
    [
        "#CJudge Eckersley adjusts his helmet and squints at you.#n",
        "#C#n",
        "#C\"Future? The future? This IS the future, citizen. Mega-City One. The#n",
        "#Claw. Always has been.\" He pauses. \"...hasn't it?\"#n",
        "#C#n",
        "#CA flicker of confusion crosses his face.#n",
        "#C#n",
        "#C\"Sometimes I get these flashes -- like I remember a different sky.#n",
        "#CDifferent laws. A world where none of this was real. Then it's gone and#n",
        "#CI'm back on patrol. Look, I don't know what you're digging into, but#n",
        "#Cthere's a ship in orbit. Aliens. They've got instruments up there --#n",
        "#Csensors, scanners, things I don't understand. They say they can see#n",
        "#Cwhere all the threads come from. Some kind of origin point. Docking#n",
        "#Cbay's through the portal.\"#n",
    ])

node("domeship", 93006, "origin", 14, 15, "elfangor",
    "#CElfangor on the Dome Ship told you:#n "
    "\"Every fragment traces back to a single point -- an old city "
    "at the center of everything.\"",
    [
        "#CElfangor-Sirinial-Shamtul turns all four eyes toward you, and you sense#n",
        "#Ca vast, alien sadness.#n",
        "#C#n",
        "#C\"The origin. You have come asking the same question we have spent cycles#n",
        "#Ctrying to answer. Our sensors have mapped the convergence -- every#n",
        "#Cfragment of reality in this patchwork world traces back to a single#n",
        "#Cpoint. An old city, at the center of everything. But our readings show#n",
        "#Csomething else, something that troubles us. The temporal distortions#n",
        "#Caren't random. There is a pattern. Something -- or someone -- is#n",
        "#Cthreading through the timelines, pulling them together. Whether to#n",
        "#Cpreserve them or consume them, we cannot say. The old city may hold that#n",
        "#Canswer. It held all the answers, once.\"#n",
    ])

# ============================================================
# Act 4: "The Old City" (Nodes 15-16)
# ============================================================

node("dystopia", 30508, "old city", 15, 16, "queen",
    "#CThe Queen of Old Dystopia told you:#n "
    "\"The King remembers more than I do. Ask him about the Unraveling.\"",
    [
        "#CThe Queen of Dystopia looks at you with eyes that have seen too much.#n",
        "#CA sad smile crosses her face.#n",
        "#C#n",
        "#C\"The old city. You called it that -- everyone does now. We just called#n",
        "#Cit home. So you've come at last. They all do, eventually -- the ones#n",
        "#Cwho ask questions instead of swinging swords. This was the first city.#n",
        "#CThe great city. We learned to reach between worlds, and for a time, it#n",
        "#Cwas glorious -- we saw wonders you cannot imagine. And then we reached#n",
        "#Ctoo far, and everything came pouring in. The barriers shattered. The#n",
        "#Cworlds collapsed together. And here we remain -- ruins among the#n",
        "#Cfragments of everything we touched. The King remembers more than I do.#n",
        "#CHe sits with it every day. Ask him about the Unraveling.\"#n",
    ])

node("dystopia", 30509, "unraveling", 16, 17, "king",
    "#CThe King of Old Dystopia told you:#n "
    "\"The world didn't end. It changed. That's just what people do. "
    "Seek Heaven or Hell for the last word.\"",
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
        "#Cthe ones who were born here, the ones who chose to stay -- they built#n",
        "#Cnew lives in the wreckage. That's not a tragedy. That's just... what#n",
        "#Cpeople do.\"#n",
        "#C#n",
        "#C\"If you want the last word on all of this, seek Heaven or Hell. The#n",
        "#Cangels and demons have their own opinions on what comes next.\"#n",
    ])

# ============================================================
# Epilogue: "What Remains" (Nodes 17a/17b -> 18+)
# ============================================================

node("heaven", 99004, "what now", 17, 99, "overseer",
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

node("hell", 30101, "what now", 17, 99, "pitlord",
    "#CYou have completed 'Echoes of the Sundering.'#n",
    [
        "#CThe Pit Lord laughs, a sound like breaking stone.#n",
        "#C#n",
        "#C\"What now, it asks! Ha! You walk through a shattered world, piece#n",
        "#Ctogether its sorry history, and your question is 'what now?' I like#n",
        "#Cyou.\" The laughter dies. \"The demons don't want to fix anything. They#n",
        "#Cwant to pull more through. More worlds, more chaos, more power. They#n",
        "#Csay the Unraveling is opportunity. And maybe they're right. But the#n",
        "#Cforce that threads through time -- the dark current beneath all of this#n",
        "#C-- it doesn't care what angels or demons want. It just... is. Like a#n",
        "#Criver. You can swim in it. You can drown in it. But you can't stop it.\"#n",
        "#C#n",
        "#CYou stand in Hell, at the edge of understanding. The Sundering broke#n",
        "#Cthe world -- or made it. The Unraveling continues -- or always has.#n",
        "#CThe truth is that no one knows, and perhaps no one ever will. But the#n",
        "#Cstories remain. The worlds remain. And as long as someone walks through#n",
        "#Cthem and listens, they are not forgotten.#n",
        "#C#n",
        "#G[Quest Complete: Echoes of the Sundering]#n",
    ])


def seed_scripts(db_dir):
    """Insert story quest scripts into area databases."""
    # Group nodes by area
    by_area = {}
    for n in STORY_NODES:
        by_area.setdefault(n["area"], []).append(n)

    for area, nodes in by_area.items():
        db_path = os.path.join(db_dir, f"{area}.db")
        if not os.path.exists(db_path):
            print(f"  WARNING: {db_path} not found, skipping {area}")
            continue

        db = sqlite3.connect(db_path)
        cursor = db.cursor()

        for n in nodes:
            # Remove any existing story script for this mob
            cursor.execute(
                "DELETE FROM scripts WHERE owner_type='mob' AND owner_vnum=? "
                "AND name LIKE 'story_node_%'",
                (n["vnum"],)
            )

            # Insert the new script
            cursor.execute(
                "INSERT INTO scripts (owner_type, owner_vnum, trigger, name, "
                "code, pattern, chance, sort_order) "
                "VALUES ('mob', ?, ?, ?, ?, ?, 0, 99)",
                (n["vnum"], TRIG_SPEECH, n["name"], n["lua_code"], n["pattern"])
            )
            print(f"  {area}.db: mob {n['vnum']} <- {n['name']}")

        db.commit()
        db.close()


def main():
    parser = argparse.ArgumentParser(
        description="Seed story quest dialogue scripts into area databases.")
    parser.add_argument("--db-dir", default=None,
        help="Path to area database directory (default: gamedata/db/areas/)")
    args = parser.parse_args()

    if args.db_dir:
        db_dir = args.db_dir
    else:
        # Auto-detect: script is in game/tools/, db is in gamedata/db/areas/
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(os.path.dirname(script_dir))
        db_dir = os.path.join(project_root, "gamedata", "db", "areas")

    if not os.path.isdir(db_dir):
        print(f"ERROR: Database directory not found: {db_dir}")
        sys.exit(1)

    print(f"Seeding story quest scripts into: {db_dir}")
    print(f"Total nodes: {len(STORY_NODES)}")
    print()
    seed_scripts(db_dir)
    print()
    print("Done. Story quest scripts seeded successfully.")
    print("Restart the MUD server to load the new scripts.")


if __name__ == "__main__":
    main()
