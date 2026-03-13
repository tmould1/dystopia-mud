"""
story_data.py - Static story node data and class command mappings.

Extracted from game/tools/seeders/seed_story_quest.py.  Used by the quest bot
to navigate the 16-node "Echoes of the Sundering" story quest and to drive
class-specific training objectives.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional


# ── Story Node definitions ─────────────────────────────────────────────

@dataclass(frozen=True)
class StoryNode:
    """One hub in the story progression (nodes 1-16)."""

    node: int                           # story_node value
    area: str                           # area file key
    npc_vnum: int                       # hub NPC vnum (or 0 for talk-through)
    intro_keyword: str                  # keyword to say to NPC for intro
    return_keyword: str = "done"        # keyword to say when all tasks done
    kill_mobs: list[int] = field(default_factory=list)   # mob vnums for kill task
    kill_threshold: int = 0             # kills needed
    fetch_obj: int = 0                  # obj vnum to pick up (0 = none)
    fetch_room: int = 0                 # room where fetch obj spawns
    examine_rooms: list[tuple[int, str]] = field(default_factory=list)  # (room_vnum, keyword)
    talk_npcs: list[tuple[int, str]] = field(default_factory=list)      # (mob_vnum, keyword)
    task_bits: list[int] = field(default_factory=list)    # required bits for return
    hub_type: str = "full"              # "talk" (1, 16), "full" (3-task), "light" (2-task)


# Node 1: Midgaard — talk-through (Executioner, mob 3011)
# Say "darkness" → advances to node 2
NODE_1 = StoryNode(
    node=1, area="midgaard", npc_vnum=3011,
    intro_keyword="darkness", return_keyword="",
    hub_type="talk",
)

# Node 2: Graveyard — Full Hub (Henry, mob 3600)
# 0x01: Kill 3 undead | 0x02: Fetch bone (obj 3699, room 3607) | 0x04: Examine tomb (room 3650)
NODE_2 = StoryNode(
    node=2, area="grave", npc_vnum=3600,
    intro_keyword="darkness", return_keyword="done",
    kill_mobs=[3601, 3605, 3602, 3604], kill_threshold=3,
    fetch_obj=3699, fetch_room=3607,
    examine_rooms=[(3650, "tomb")],
    task_bits=[0x01, 0x02, 0x04],
    hub_type="full",
)

# Node 3: Chapel — Full Hub (Priest, mob 3405)
# 0x01: Kill 1 wraith (3403) | 0x02: Fetch scroll (obj 3498, room 3457) | 0x04: Examine etchings (room 3465)
NODE_3 = StoryNode(
    node=3, area="chapel", npc_vnum=3405,
    intro_keyword="marks", return_keyword="done",
    kill_mobs=[3403], kill_threshold=1,
    fetch_obj=3498, fetch_room=3457,
    examine_rooms=[(3465, "etchings")],
    task_bits=[0x01, 0x02, 0x04],
    hub_type="full",
)

# Node 4: Grove — Light Hub (Hierophant, mob 8900)
# 0x01: Kill 2 corrupted (snake 8908, stag 8906) | 0x02: Examine roots (room 8905)
NODE_4 = StoryNode(
    node=4, area="grove", npc_vnum=8900,
    intro_keyword="sundering", return_keyword="done",
    kill_mobs=[8908, 8906], kill_threshold=2,
    examine_rooms=[(8905, "roots")],
    task_bits=[0x01, 0x02],
    hub_type="light",
)

# Node 5: Canyon — Light Hub (Earth Ruler, mob 9208)
# 0x01: Kill 3 elementals | 0x02: Examine marble throne (room 9227)
NODE_5 = StoryNode(
    node=5, area="canyon", npc_vnum=9208,
    intro_keyword="sundering", return_keyword="done",
    kill_mobs=[9224, 9217, 9203], kill_threshold=3,
    examine_rooms=[(9227, "throne")],
    task_bits=[0x01, 0x02],
    hub_type="light",
)

# Node 6: Moria — Full Hub (The Mage, mob 4100)
# 0x01: Kill 5 tunnel creatures | 0x02: Fetch slime mold (obj 4103, room 4118) | 0x04: Examine carvings (room 4160)
NODE_6 = StoryNode(
    node=6, area="moria", npc_vnum=4100,
    intro_keyword="carvings", return_keyword="done",
    kill_mobs=[4004, 4003, 4052, 4156, 4053], kill_threshold=5,
    fetch_obj=4103, fetch_room=4118,
    examine_rooms=[(4160, "carvings")],
    task_bits=[0x01, 0x02, 0x04],
    hub_type="full",
)

# Node 7: Hitower — Full Hub (Adventurer, mob 1301)
# 0x01: Kill 3 golems | 0x02: Fetch journal (obj 1499, room 1367) | 0x04: Examine mirror (room 1336)
NODE_7 = StoryNode(
    node=7, area="hitower", npc_vnum=1301,
    intro_keyword="sorcerer", return_keyword="done",
    kill_mobs=[1339, 1343, 1344, 1346, 1347], kill_threshold=3,
    fetch_obj=1499, fetch_room=1367,
    examine_rooms=[(1336, "mirror")],
    task_bits=[0x01, 0x02, 0x04],
    hub_type="full",
)

# Node 8: Thalos — Full Hub (Librarian, mob 5315)
# 0x01: Kill 2 beasts | 0x02: Examine Odin statue (room 5386) | 0x04: Examine foundation (room 5371)
NODE_8 = StoryNode(
    node=8, area="mirror", npc_vnum=5315,
    intro_keyword="thalos", return_keyword="done",
    kill_mobs=[5324, 5323, 5333], kill_threshold=2,
    examine_rooms=[(5386, "statue"), (5371, "foundation")],
    task_bits=[0x01, 0x02, 0x04],
    hub_type="full",
)

# Node 9: Drow City — Light Hub (Priestess, mob 5104)
# 0x01: Examine altar (room 5145)  [no kill task — examine only]
NODE_9 = StoryNode(
    node=9, area="drow", npc_vnum=5104,
    intro_keyword="displacement", return_keyword="done",
    examine_rooms=[(5145, "altar")],
    task_bits=[0x01],
    hub_type="light",
)

# Node 10: Pyramid — Full Hub (Sphinx, mob 2616)
# 0x01: Fetch tablet (obj 2699, room 2630) | 0x02: Examine hieroglyphics (room 2629)
NODE_10 = StoryNode(
    node=10, area="pyramid", npc_vnum=2616,
    intro_keyword="cycle", return_keyword="done",
    fetch_obj=2699, fetch_room=2630,
    examine_rooms=[(2629, "hieroglyphics")],
    task_bits=[0x01, 0x02],
    hub_type="full",
)

# Node 11: Dreamscape — Light Hub (Keeper, mob 8600)
# 0x01: Examine church (room 8637)
NODE_11 = StoryNode(
    node=11, area="dream", npc_vnum=8600,
    intro_keyword="between", return_keyword="done",
    examine_rooms=[(8637, "church")],
    task_bits=[0x01],
    hub_type="light",
)

# Node 12: Atlantis/Olympus — branching Light Hub
# Atlantis path: Neptune (mob 8103) | 0x01: Kill guard (8110) | 0x02: Examine murals (room 8131)
# Olympus path: Zeus (mob 901) | 0x01: Kill chimera (920) | 0x02: Examine scorch (room 910)
# Bot uses Atlantis path by default
NODE_12_ATLANTIS = StoryNode(
    node=12, area="atlantis", npc_vnum=8103,
    intro_keyword="convergence", return_keyword="done",
    kill_mobs=[8110], kill_threshold=1,
    examine_rooms=[(8131, "murals")],
    task_bits=[0x01, 0x02],
    hub_type="light",
)

NODE_12_OLYMPUS = StoryNode(
    node=12, area="olympus", npc_vnum=901,
    intro_keyword="convergence", return_keyword="done",
    kill_mobs=[920], kill_threshold=1,
    examine_rooms=[(910, "scorch")],
    task_bits=[0x01, 0x02],
    hub_type="light",
)

# Node 13: Mega-City — Light Hub (Judge, mob 8010)
# 0x01: Kill punk (8001) | 0x02: Examine rubble (room 8014)
NODE_13 = StoryNode(
    node=13, area="mega1", npc_vnum=8010,
    intro_keyword="future", return_keyword="done",
    kill_mobs=[8001], kill_threshold=1,
    examine_rooms=[(8014, "rubble")],
    task_bits=[0x01, 0x02],
    hub_type="light",
)

# Node 14: Domeship — Light Hub (Elfangor, mob 93006)
# 0x01: Examine sensors (room 93045)
NODE_14 = StoryNode(
    node=14, area="domeship", npc_vnum=93006,
    intro_keyword="origin", return_keyword="done",
    examine_rooms=[(93045, "sensors")],
    task_bits=[0x01],
    hub_type="light",
)

# Node 15: Dystopia — Full Hub (Queen, mob 30508)
# 0x01: Kill royal guard (30400) | 0x02: Examine records (room 30460) | 0x04: Talk to King (mob 30509)
NODE_15 = StoryNode(
    node=15, area="dystopia", npc_vnum=30508,
    intro_keyword="old city", return_keyword="done",
    kill_mobs=[30400], kill_threshold=1,
    examine_rooms=[(30460, "records")],
    talk_npcs=[(30509, "unraveling")],
    task_bits=[0x01, 0x02, 0x04],
    hub_type="full",
)

# Node 16: Epilogue — talk-through (Heaven: Overseer 99004, Hell: Pitlord 30101)
NODE_16 = StoryNode(
    node=16, area="heaven", npc_vnum=99004,
    intro_keyword="what now", return_keyword="",
    hub_type="talk",
)

# Ordered list for easy iteration (bot uses Atlantis path for node 12)
STORY_NODES: list[StoryNode] = [
    NODE_1, NODE_2, NODE_3, NODE_4, NODE_5, NODE_6,
    NODE_7, NODE_8, NODE_9, NODE_10, NODE_11,
    NODE_12_ATLANTIS,
    NODE_13, NODE_14, NODE_15, NODE_16,
]

# Lookup by node number (defaults to Atlantis for node 12)
STORY_NODE_MAP: dict[int, StoryNode] = {n.node: n for n in STORY_NODES}


# ── Class → training command mapping ───────────────────────────────────

@dataclass(frozen=True)
class ClassInfo:
    """Training info for a character class."""
    train_cmd: Optional[str]     # e.g. "songtrain", None if uses discipline system
    disc_default: Optional[str]  # default discipline to research (for disc-based classes)


CLASS_COMMANDS: dict[str, ClassInfo] = {
    # Disc-based classes (vampire, werewolf, demon)
    "demon":        ClassInfo(train_cmd=None,           disc_default="attack"),
    "vampire":      ClassInfo(train_cmd=None,           disc_default="protean"),
    "werewolf":     ClassInfo(train_cmd=None,           disc_default="bear"),

    # Classes with dedicated train commands
    "dirgesinger":  ClassInfo(train_cmd="songtrain",    disc_default=None),
    "psion":        ClassInfo(train_cmd="psitrain",     disc_default=None),
    "dragonkin":    ClassInfo(train_cmd="dragontrain",  disc_default=None),
    "artificer":    ClassInfo(train_cmd="techtrain",    disc_default=None),
    "mechanist":    ClassInfo(train_cmd="cybtrain",     disc_default=None),
    "cultist":      ClassInfo(train_cmd="voidtrain",    disc_default=None),
    "chronomancer": ClassInfo(train_cmd="timetrain",    disc_default=None),
    "paradox":      ClassInfo(train_cmd="paratrain",    disc_default=None),
    "shaman":       ClassInfo(train_cmd="spirittrain",  disc_default=None),
    "spiritlord":   ClassInfo(train_cmd="lordtrain",    disc_default=None),
    "siren":        ClassInfo(train_cmd="voicetrain",   disc_default=None),
    "mindflayer":   ClassInfo(train_cmd="mindtrain",    disc_default=None),
    "wyrm":         ClassInfo(train_cmd="wyrmtrain",    disc_default=None),

    # Classes using research/train discipline system
    "mage":         ClassInfo(train_cmd=None,           disc_default="arcane"),
    "ninja":        ClassInfo(train_cmd=None,           disc_default=None),
    "monk":         ClassInfo(train_cmd=None,           disc_default=None),
    "drow":         ClassInfo(train_cmd=None,           disc_default=None),
    "angel":        ClassInfo(train_cmd=None,           disc_default=None),
    "samurai":      ClassInfo(train_cmd=None,           disc_default=None),
    "lich":         ClassInfo(train_cmd=None,           disc_default=None),
    "shapeshifter": ClassInfo(train_cmd=None,           disc_default=None),
    "tanarri":      ClassInfo(train_cmd=None,           disc_default=None),
    "undead_knight": ClassInfo(train_cmd=None,          disc_default=None),
    "spiderdroid":  ClassInfo(train_cmd=None,           disc_default=None),
}
