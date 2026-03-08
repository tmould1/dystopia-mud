#!/usr/bin/env python3
"""
seed_quest_db.py - Create and populate gamedata/db/game/quest.db

Creates the quest definition database with all quest definitions,
objectives, and prerequisites for the branching quest tree system.

Usage:
    python game/tools/seed_quest_db.py

Run from the repository root.  Idempotent -- safe to re-run.
"""

import os
import sqlite3
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Class constants (must match class.h)
# ---------------------------------------------------------------------------
CLASS_NONE          = 0
CLASS_DEMON         = 1
CLASS_MAGE          = 2
CLASS_WEREWOLF      = 4
CLASS_VAMPIRE       = 8
CLASS_SAMURAI       = 16
CLASS_DROW          = 32
CLASS_MONK          = 64
CLASS_NINJA         = 128
CLASS_LICH          = 256
CLASS_SHAPESHIFTER  = 512
CLASS_TANARRI       = 1024
CLASS_ANGEL         = 2048
CLASS_UNDEAD_KNIGHT = 4096
CLASS_DROID         = 8192
CLASS_DIRGESINGER   = 16384
CLASS_SIREN         = 32768
CLASS_PSION         = 65536
CLASS_MINDFLAYER    = 131072
CLASS_DRAGONKIN     = 262144
CLASS_WYRM          = 524288
CLASS_ARTIFICER     = 1048576
CLASS_MECHANIST     = 2097152
CLASS_CULTIST       = 4194304
CLASS_VOIDBORN      = 8388608
CLASS_CHRONOMANCER  = 16777216
CLASS_PARADOX       = 33554432
CLASS_SHAMAN        = 67108864
CLASS_SPIRITLORD    = 134217728

# ---------------------------------------------------------------------------
# Quest flags
# ---------------------------------------------------------------------------
QFLAG_AUTO_COMPLETE = (1 << 0)
QFLAG_REPEATABLE    = (1 << 1)
QFLAG_FTUE_SKIP     = (1 << 2)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def find_repo_root() -> Path:
    return Path(__file__).resolve().parent.parent.parent


def create_schema(conn):
    conn.executescript("""
        DROP TABLE IF EXISTS quest_prerequisites;
        DROP TABLE IF EXISTS quest_objectives;
        DROP TABLE IF EXISTS quest_defs;

        CREATE TABLE quest_defs (
            id              TEXT PRIMARY KEY,
            name            TEXT NOT NULL,
            description     TEXT NOT NULL,
            category        TEXT NOT NULL,
            tier            INTEGER NOT NULL DEFAULT 0,
            flags           INTEGER NOT NULL DEFAULT 0,
            min_explevel    INTEGER NOT NULL DEFAULT 0,
            max_explevel    INTEGER NOT NULL DEFAULT 3,
            qp_reward       INTEGER NOT NULL DEFAULT 0,
            exp_reward      INTEGER NOT NULL DEFAULT 0,
            primal_reward   INTEGER NOT NULL DEFAULT 0,
            item_reward_vnum INTEGER NOT NULL DEFAULT 0,
            required_class  INTEGER NOT NULL DEFAULT 0,
            sort_order      INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE quest_objectives (
            quest_id    TEXT NOT NULL REFERENCES quest_defs(id),
            obj_index   INTEGER NOT NULL,
            type        TEXT NOT NULL,
            target      TEXT NOT NULL,
            threshold   INTEGER NOT NULL DEFAULT 1,
            description TEXT NOT NULL,
            PRIMARY KEY (quest_id, obj_index)
        );

        CREATE TABLE quest_prerequisites (
            quest_id    TEXT NOT NULL REFERENCES quest_defs(id),
            requires_id TEXT NOT NULL REFERENCES quest_defs(id),
            PRIMARY KEY (quest_id, requires_id)
        );
    """)


# ---------------------------------------------------------------------------
# Quest data insertion helpers
# ---------------------------------------------------------------------------

_sort_counter = 0

def quest(conn, id, name, desc, cat, tier, flags=QFLAG_AUTO_COMPLETE,
          min_exp=0, max_exp=3, qp=0, exp=0, primal=0, item_vnum=0,
          req_class=CLASS_NONE, prereqs=None, objectives=None):
    """Insert a quest definition with its objectives and prerequisites."""
    global _sort_counter
    _sort_counter += 1

    conn.execute(
        "INSERT INTO quest_defs VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        (id, name, desc, cat, tier, flags, min_exp, max_exp,
         qp, exp, primal, item_vnum, req_class, _sort_counter))

    if objectives:
        for i, obj in enumerate(objectives):
            conn.execute(
                "INSERT INTO quest_objectives VALUES (?,?,?,?,?,?)",
                (id, i, obj[0], obj[1], obj[2], obj[3]))

    if prereqs:
        for req in prereqs:
            conn.execute(
                "INSERT INTO quest_prerequisites VALUES (?,?)",
                (id, req))


# ---------------------------------------------------------------------------
# Phase 0: Tutorial (FTUE-Gated)
# ---------------------------------------------------------------------------

def seed_phase0(conn):
    """Tutorial quests gated by experience level."""
    AC = QFLAG_AUTO_COMPLETE

    # FTUE Level 0: Never played a MUD
    quest(conn, "T01", "Finding Your Feet",
          "Learn the basic commands to perceive and understand the world around you.",
          "T", 0, AC, min_exp=0, max_exp=0, qp=10,
          objectives=[
              ("USE_COMMAND", "look", 1, "Use the 'look' command"),
              ("USE_COMMAND", "score", 1, "Check your score"),
              ("USE_COMMAND", "inventory", 1, "Check your inventory"),
          ])

    quest(conn, "T02", "The World Around You",
          "Learn to navigate the world and see what's around you.",
          "T", 0, AC, min_exp=0, max_exp=0, qp=10,
          prereqs=["T01"],
          objectives=[
              ("USE_COMMAND", "north", 1, "Walk north"),
              ("USE_COMMAND", "south", 1, "Walk south"),
              ("USE_COMMAND", "exits", 1, "Check available exits"),
              ("USE_COMMAND", "scan", 1, "Scan your surroundings"),
          ])

    quest(conn, "T03", "Speaking Up",
          "Learn to communicate with other players.",
          "T", 0, AC, min_exp=0, max_exp=0, qp=10,
          prereqs=["T02"],
          objectives=[
              ("USE_COMMAND", "say", 1, "Say something in the room"),
              ("USE_COMMAND", "newbie", 1, "Use the newbie chat channel (type: newbie <message>)"),
          ])

    quest(conn, "T04", "Gear Up",
          "Learn to pick up items and equip yourself for battle.",
          "T", 0, AC, min_exp=0, max_exp=0, qp=10,
          prereqs=["T03"],
          objectives=[
              ("USE_COMMAND", "equipment", 1, "Check your equipment"),
              ("USE_COMMAND", "get", 1, "Pick up an item"),
              ("USE_COMMAND", "wield", 1, "Equip an item"),
          ])

    quest(conn, "T05", "First Fight",
          "Defeat your first enemy in combat.",
          "T", 0, AC, min_exp=0, max_exp=0, qp=25,
          prereqs=["T04"],
          objectives=[
              ("KILL_MOB", "any", 1, "Kill any mob"),
          ])

    # FTUE Level 1: MUD experience, new to Dystopia
    quest(conn, "T06", "Dystopia Orientation",
          "Get your bearings in Dystopia - check your stats and surroundings.",
          "T", 0, AC, min_exp=1, max_exp=1, qp=15,
          objectives=[
              ("USE_COMMAND", "score", 1, "Check your score"),
              ("USE_COMMAND", "scan", 1, "Scan your surroundings"),
              ("USE_COMMAND", "map", 1, "View the area map"),
          ])

    quest(conn, "T07", "Know Your Tools",
          "Familiarize yourself with Dystopia's unique systems.",
          "T", 0, AC, min_exp=1, max_exp=1, qp=15,
          prereqs=["T06"],
          objectives=[
              ("USE_COMMAND", "help", 1, "Read a help topic"),
              ("USE_COMMAND", "commands", 1, "View available commands"),
          ])

    quest(conn, "T08", "Combat Primer",
          "Prove your combat skills against a few local threats.",
          "T", 0, AC, min_exp=1, max_exp=1, qp=25,
          prereqs=["T07"],
          objectives=[
              ("KILL_MOB", "any", 3, "Kill 3 mobs"),
              ("USE_COMMAND", "consider", 1, "Consider an enemy's difficulty"),
          ])


# ---------------------------------------------------------------------------
# Phase 1: Foundation
# ---------------------------------------------------------------------------

def seed_phase1(conn):
    AC = QFLAG_AUTO_COMPLETE

    quest(conn, "M01", "Choose Your Path",
          "Train to avatar status and select your class with selfclass.",
          "M", 1, AC, qp=50,
          prereqs=["T05", "T08"],  # Either tutorial path
          objectives=[
              ("REACH_STAT", "hp", 2000, "Reach 2000 HP to qualify for avatar"),
              ("USE_COMMAND", "train", 1, "Train avatar (type: train avatar)"),
              ("REACH_STAT", "class", 1, "Choose a class (type: selfclass <class>)"),
          ])

    quest(conn, "M02", "Trial by Fire",
          "Prove your worth by defeating enemies across the land.",
          "M", 1, AC, qp=100,
          prereqs=["M01"],
          objectives=[
              ("KILL_MOB", "any", 25, "Kill 25 mobs"),
          ])

    quest(conn, "T_PRACTICE_01", "Learning the Basics",
          "Use the practice command to learn skills available to your class.",
          "T", 1, AC, qp=50, exp=5000,
          prereqs=["M01"],
          objectives=[
              ("USE_COMMAND", "practice", 3, "Practice 3 skills"),
          ])

    quest(conn, "T_STANCE_01", "Combat Stance",
          "Learn a fighting stance and begin to master it through combat. Try 'help stance' to see the stances available.",
          "T", 1, AC, qp=50,
          prereqs=["M01"],
          objectives=[
              ("USE_COMMAND", "stance", 1, "Enter a combat stance"),
              ("LEARN_STANCE", "any", 10, "Raise any stance to skill 10 (hint: autostance)"),
          ])

    quest(conn, "M03", "Building Strength",
          "Grow your hit points through combat and training.",
          "M", 1, AC, qp=200,
          prereqs=["M02"],
          objectives=[
              ("REACH_STAT", "hp", 5000, "Reach 5,000 hit points"),
          ])

    quest(conn, "M04", "Earning Your Keep",
          "Accumulate quest points from your adventures.",
          "M", 1, AC, qp=200,
          prereqs=["M03"],
          objectives=[
              ("EARN_QP", "lifetime", 500, "Earn 500 lifetime quest points"),
          ])


# ---------------------------------------------------------------------------
# Phase 2: Growth — Core System Mastery
# ---------------------------------------------------------------------------

def seed_phase2(conn):
    AC = QFLAG_AUTO_COMPLETE

    quest(conn, "M05", "Rising Power",
          "Your growing strength marks you as a force to be reckoned with.",
          "M", 2, AC, qp=500,
          prereqs=["M04"],
          objectives=[
              ("REACH_STAT", "hp", 25000, "Reach 25,000 hit points"),
              ("REACH_STAT", "mana", 15000, "Reach 15,000 mana"),
          ])

    # Combat track
    quest(conn, "C01", "Mob Hunter",
          "Prove your combat prowess against the creatures of the world.",
          "C", 2, AC, qp=200,
          prereqs=["M05"],
          objectives=[("KILL_MOB", "any", 100, "Kill 100 mobs")])

    quest(conn, "C02", "Slayer",
          "Your kill count grows. The world knows your name.",
          "C", 2, AC, qp=500,
          prereqs=["C01"],
          objectives=[("KILL_MOB", "any", 500, "Kill 500 mobs")])

    quest(conn, "C03", "Exterminator",
          "You have become death incarnate. Few creatures survive your passing.",
          "C", 2, AC, qp=1000,
          prereqs=["C02"],
          objectives=[("KILL_MOB", "any", 2000, "Kill 2,000 mobs")])

    # Discipline tutorial
    quest(conn, "T_DISC_01", "Discipline Basics",
          "Begin researching a discipline to unlock new powers.",
          "T", 2, AC, qp=200,
          prereqs=["M05"],
          objectives=[
              ("USE_COMMAND", "research", 1, "Start researching a discipline"),
              ("LEARN_DISCIPLINE", "any", 1, "Raise any discipline to level 1"),
          ])

    quest(conn, "T_DISC_02", "Dedicated Study",
          "Deepen your discipline knowledge through sustained research.",
          "T", 2, AC, qp=500,
          prereqs=["T_DISC_01"],
          objectives=[
              ("LEARN_DISCIPLINE", "any", 5, "Raise any discipline to level 5"),
          ])

    # Forge tutorial
    quest(conn, "T_FORGE_01", "The Smith's Way",
          "Visit a forge and enhance an item with raw materials.",
          "T", 2, AC, qp=300,
          prereqs=["M05"],
          objectives=[
              ("FORGE_ITEM", "any", 1, "Forge any item with a material"),
          ])

    quest(conn, "T_FORGE_02", "Superior Craft",
          "Expand your forging skills by working on multiple items.",
          "T", 2, AC, qp=500,
          prereqs=["T_FORGE_01"],
          objectives=[
              ("FORGE_ITEM", "any", 3, "Forge 3 items"),
          ])

    # Quest item tutorial
    quest(conn, "T_QITEM_01", "Custom Gear",
          "Use quest points to create your own personalized equipment.",
          "T", 2, AC, qp=200,
          prereqs=["M05"],
          objectives=[
              ("QUEST_CREATE", "any", 1, "Create a quest item"),
          ])

    quest(conn, "T_QITEM_02", "Personal Touch",
          "Customize your equipment with stat bonuses and effects.",
          "T", 2, AC, qp=300,
          prereqs=["T_QITEM_01"],
          objectives=[
              ("QUEST_MODIFY", "any", 3, "Modify 3 properties on quest items"),
          ])

    # Exploration track
    quest(conn, "E01", "Explorer",
          "Venture beyond the familiar and discover new areas.",
          "E", 2, AC, qp=200,
          prereqs=["M05"],
          objectives=[("VISIT_AREA", "any", 5, "Visit 5 different areas")])

    quest(conn, "E02", "Wanderer",
          "Your travels take you across the breadth of the world.",
          "E", 2, AC, qp=500,
          prereqs=["E01"],
          objectives=[("VISIT_AREA", "any", 15, "Visit 15 different areas")])

    # Archaic quest card
    quest(conn, "A01", "Ancient Rites",
          "Complete an archaic quest card by collecting the required items.",
          "A", 2, AC, qp=300,
          prereqs=["M05"],
          objectives=[
              ("COMPLETE_QUEST", "archaic_card", 1, "Complete 1 quest card"),
          ])


# ---------------------------------------------------------------------------
# Phase 2: Per-Class Quests (CL_xxx_01 and CL_xxx_02)
# ---------------------------------------------------------------------------

def seed_class_phase2(conn):
    AC = QFLAG_AUTO_COMPLETE

    classes = [
        # (prefix, class_const, name, quest1_name, quest1_desc, quest1_objs,
        #  quest2_name, quest2_desc, quest2_objs)
        ("CL_VAMP", CLASS_VAMPIRE, "Vampire",
         "Embrace the Night",
         "Awaken your vampiric nature by extending your claws and fangs.",
         [("USE_COMMAND", "claws", 1, "Extend your claws"),
          ("USE_COMMAND", "fangs", 1, "Bare your fangs")],
         "Blood Disciplines",
         "Study the vampire clan disciplines to unlock deeper powers.",
         [("USE_COMMAND", "clandisc", 1, "View your clan disciplines"),
          ("CLASS_POWER", "protean", 3, "Raise Protean to level 3")]),

        ("CL_WW", CLASS_WEREWOLF, "Werewolf",
         "Call of the Wild",
         "Embrace the beast within. Rage and transform into your wolf form.",
         [("USE_COMMAND", "rage", 1, "Enter a rage"),
          ("USE_COMMAND", "werewolf", 1, "Transform into werewolf form")],
         "Gift of the Moon",
         "Study the totems and gifts granted by Luna.",
         [("USE_COMMAND", "totems", 1, "View your totems"),
          ("CLASS_POWER", "gifts", 3, "Raise Gifts to level 3")]),

        ("CL_DEM", CLASS_DEMON, "Demon",
         "Infernal Awakening",
         "Manifest your demonic features - horns and wings of destruction.",
         [("USE_COMMAND", "horns", 1, "Grow your horns"),
          ("USE_COMMAND", "wings", 1, "Spread your wings")],
         "Hellfire Mastery",
         "Embrace your demonic form and master the art of attack.",
         [("USE_COMMAND", "demonform", 1, "Enter demon form"),
          ("CLASS_POWER", "attack", 3, "Raise Attack to level 3")]),

        ("CL_MNK", CLASS_MONK, "Monk",
         "Inner Focus",
         "Center yourself through chi manipulation and meditation.",
         [("USE_COMMAND", "chi", 1, "Channel your chi"),
          ("USE_COMMAND", "meditate", 1, "Meditate")],
         "Way of the Fist",
         "Train your martial arts through disciplined combat.",
         [("USE_COMMAND", "kick", 3, "Perform 3 kicks"),
          ("CLASS_POWER", "martial", 3, "Raise Martial Arts to level 3")]),

        ("CL_MAG", CLASS_MAGE, "Mage",
         "Arcane Spark",
         "Begin casting spells and feel the flow of magical energies.",
         [("USE_COMMAND", "cast", 3, "Cast 3 spells"),
          ("SPELL_SKILL", "any", 50, "Raise any spell color to 50")],
         "Spell Research",
         "Deepen your arcane knowledge through focused research.",
         [("USE_COMMAND", "research", 1, "Research a discipline"),
          ("SPELL_SKILL", "any", 100, "Raise any spell color to 100")]),

        ("CL_NIN", CLASS_NINJA, "Ninja",
         "Shadow Arts",
         "Master the art of stealth and surprise attacks.",
         [("USE_COMMAND", "vanish", 1, "Vanish from sight"),
          ("USE_COMMAND", "backstab", 1, "Backstab an enemy")],
         "Path of Silence",
         "Develop your shadow abilities through focused training.",
         [("USE_COMMAND", "mitsukeru", 1, "Use mitsukeru"),
          ("CLASS_POWER", "shadow", 3, "Raise Shadow to level 3")]),

        ("CL_DRW", CLASS_DROW, "Drow",
         "Spider's Touch",
         "Harness the power of the spider and the shadow plane.",
         [("USE_COMMAND", "web", 1, "Cast a web"),
          ("USE_COMMAND", "shadowplane", 1, "Enter the shadow plane")],
         "Dark Elf Ways",
         "Deepen your connection to the spider's domain.",
         [("USE_COMMAND", "nightsight", 1, "Activate nightsight"),
          ("CLASS_POWER", "spider", 3, "Raise Spider to level 3")]),

        ("CL_DGS", CLASS_DIRGESINGER, "Dirgesinger",
         "First Melody",
         "Sing your first song and feel its power resonate.",
         [("USE_COMMAND", "sing", 1, "Sing a song")],
         "Song Mastery",
         "Train your voice and master the songs of power.",
         [("CLASS_TRAIN", "songtrain", 3, "Use songtrain 3 times"),
          ("CLASS_POWER", "song", 3, "Raise Song to level 3")]),

        ("CL_PSI", CLASS_PSION, "Psion",
         "Mind Awakens",
         "Open your mind to the psionic realm.",
         [("USE_COMMAND", "focus", 1, "Focus your mental energy")],
         "Psionic Training",
         "Develop your psionic abilities through structured training.",
         [("CLASS_TRAIN", "psitrain", 3, "Use psitrain 3 times"),
          ("CLASS_POWER", "psionic", 3, "Raise Psionic to level 3")]),

        ("CL_DKN", CLASS_DRAGONKIN, "Dragonkin",
         "Dragon Blood",
         "Awaken the dragon blood within and unleash your breath weapon.",
         [("USE_COMMAND", "breathe", 1, "Use your breath weapon")],
         "Dragon Training",
         "Train your draconic powers through dedicated practice.",
         [("CLASS_TRAIN", "dragontrain", 3, "Use dragontrain 3 times"),
          ("CLASS_POWER", "dragon", 3, "Raise Dragon to level 3")]),

        ("CL_ART", CLASS_ARTIFICER, "Artificer",
         "First Construct",
         "Deploy your first mechanical constructs on the battlefield.",
         [("USE_COMMAND", "turret", 1, "Deploy a turret"),
          ("USE_COMMAND", "blaster", 1, "Fire your blaster")],
         "Tech Mastery",
         "Advance your technological knowledge through research.",
         [("CLASS_TRAIN", "techtrain", 3, "Use techtrain 3 times"),
          ("CLASS_POWER", "tech", 3, "Raise Tech to level 3")]),

        ("CL_CLT", CLASS_CULTIST, "Cultist",
         "Dark Ritual",
         "Perform your first dark ritual and touch the void.",
         [("USE_COMMAND", "ritual", 1, "Perform a ritual")],
         "Void Training",
         "Deepen your connection to the void through training.",
         [("CLASS_TRAIN", "voidtrain", 3, "Use voidtrain 3 times"),
          ("CLASS_POWER", "void", 3, "Raise Void to level 3")]),

        ("CL_CHR", CLASS_CHRONOMANCER, "Chronomancer",
         "Time Flux",
         "Manipulate the flow of time with your chronomantic powers.",
         [("USE_COMMAND", "flux", 1, "Use time flux"),
          ("USE_COMMAND", "quicken", 1, "Quicken time")],
         "Temporal Training",
         "Master the intricacies of temporal manipulation.",
         [("CLASS_TRAIN", "timetrain", 3, "Use timetrain 3 times"),
          ("CLASS_POWER", "time", 3, "Raise Time to level 3")]),

        ("CL_SHM", CLASS_SHAMAN, "Shaman",
         "Spirit Call",
         "Commune with the spirits and hear their whispers.",
         [("USE_COMMAND", "commune", 1, "Commune with spirits")],
         "Spirit Training",
         "Deepen your bond with the spirit world.",
         [("CLASS_TRAIN", "spirittrain", 3, "Use spirittrain 3 times"),
          ("CLASS_POWER", "spirit", 3, "Raise Spirit to level 3")]),
    ]

    for (pfx, cls, cls_name, n1, d1, o1, n2, d2, o2) in classes:
        quest(conn, f"{pfx}_01", n1, d1, "CL", 2, AC, qp=300,
              req_class=cls, prereqs=["M05"], objectives=o1)
        quest(conn, f"{pfx}_02", n2, d2, "CL", 2, AC, qp=500,
              req_class=cls, prereqs=[f"{pfx}_01"], objectives=o2)


# ---------------------------------------------------------------------------
# Phase 3: Power
# ---------------------------------------------------------------------------

def seed_phase3(conn):
    AC = QFLAG_AUTO_COMPLETE

    # M06 requires all system tutorials (implemented via prereqs)
    quest(conn, "M06", "Proven Warrior",
          "You have mastered the core systems. A new chapter begins.",
          "M", 3, AC, qp=1000,
          prereqs=["M05", "T_DISC_02", "T_FORGE_02", "T_QITEM_02"],
          # Note: CL_xxx_02 prereqs are dynamic (checked by engine based on class)
          objectives=[
              ("REACH_STAT", "hp", 35000, "Reach 35,000 hit points"),
              ("EARN_QP", "lifetime", 5000, "Earn 5,000 lifetime quest points"),
          ])

    quest(conn, "M07", "Generation Climb",
          "Lower your generation through combat to unlock greater power.",
          "M", 3, AC, qp=1500,
          prereqs=["M06"],
          objectives=[("REACH_GEN", "gen", 5, "Reach generation 5 or lower")])

    # Arena
    quest(conn, "C04", "Arena Initiate",
          "Step into the arena and claim your first victory.",
          "C", 3, AC, qp=300,
          prereqs=["M07"],
          objectives=[("ARENA_WIN", "any", 1, "Win 1 arena fight")])

    quest(conn, "C05", "Arena Veteran",
          "You are a regular in the arena. Your name echoes off the walls.",
          "C", 3, AC, qp=1000,
          prereqs=["C04"],
          objectives=[("ARENA_WIN", "any", 10, "Win 10 arena fights")])

    # Stance progression
    quest(conn, "T_STANCE_02", "Stance Discipline",
          "Refine your stance mastery across multiple fighting styles.",
          "T", 3, AC, qp=500,
          prereqs=["M07"],
          objectives=[
              ("LEARN_STANCE", "count_100", 3, "Raise 3 stances to skill 100"),
          ])

    quest(conn, "T_STANCE_03", "Stance Grandmaster",
          "Achieve grandmaster rank in five basic stances.",
          "T", 3, AC, qp=1000,
          prereqs=["T_STANCE_02"],
          objectives=[
              ("LEARN_STANCE", "count_200", 5, "Raise 5 stances to skill 200"),
          ])

    # Training
    quest(conn, "T_TRAIN_01", "Training Regimen",
          "Use the train command to improve your generation or stats.",
          "T", 3, AC, qp=300,
          prereqs=["M07"],
          objectives=[
              ("USE_COMMAND", "train", 1, "Use the train command"),
          ])

    # Forge advancement
    quest(conn, "CR01", "Forge Apprentice",
          "Master the art of metal forging with slabs of raw metal.",
          "CR", 3, AC, qp=400,
          prereqs=["M06"],
          objectives=[("FORGE_ITEM", "metal", 1, "Forge with a metal slab")])

    quest(conn, "CR02", "Gemcrafter",
          "Set precious gemstones into your forged equipment.",
          "CR", 3, AC, qp=500,
          prereqs=["CR01"],
          objectives=[("FORGE_ITEM", "gem", 1, "Forge with a gemstone")])

    # Exploration
    quest(conn, "E03", "Cartographer",
          "Your travels extend across the breadth of the known world.",
          "E", 3, AC, qp=800,
          prereqs=["M06"],
          objectives=[("VISIT_AREA", "any", 30, "Visit 30 different areas")])

    quest(conn, "E04", "World Walker",
          "Few have walked as many paths as you.",
          "E", 3, AC, qp=1500,
          prereqs=["E03"],
          objectives=[("VISIT_AREA", "any", 50, "Visit 50 different areas")])

    # Dailies
    quest(conn, "D01", "Daily: Hunting Grounds",
          "Clear out dangerous creatures threatening the realm.",
          "D", 3, QFLAG_AUTO_COMPLETE | QFLAG_REPEATABLE, qp=100,
          prereqs=["M06"],
          objectives=[("KILL_MOB", "any", 50, "Kill 50 mobs")])

    quest(conn, "D02", "Daily: Wanderlust",
          "Patrol different areas to maintain your awareness of the world.",
          "D", 3, QFLAG_AUTO_COMPLETE | QFLAG_REPEATABLE, qp=75,
          prereqs=["M06"],
          objectives=[("VISIT_AREA", "any", 3, "Visit 3 different areas")])

    # Archaic
    quest(conn, "A02", "Card Collector",
          "Complete multiple quest cards to prove your dedication.",
          "A", 3, AC, qp=800,
          prereqs=["A01"],
          objectives=[
              ("COMPLETE_QUEST", "archaic_card", 5, "Complete 5 quest cards"),
          ])


# ---------------------------------------------------------------------------
# Phase 3: Per-Class Quests (CL_xxx_03)
# ---------------------------------------------------------------------------

def seed_class_phase3(conn):
    AC = QFLAG_AUTO_COMPLETE

    class_p3 = [
        ("CL_VAMP_03", CLASS_VAMPIRE, "Elder Blood",
         "Master the ancient vampire disciplines.",
         [("CLASS_POWER", "protean", 7, "Raise Protean to level 7"),
          ("CLASS_POWER", "obtenebration", 5, "Raise Obtenebration to level 5")]),
        ("CL_WW_03", CLASS_WEREWOLF, "Pack Alpha",
         "Lead the pack with your mastery of werewolf gifts.",
         [("CLASS_POWER", "gifts", 7, "Raise Gifts to level 7"),
          ("USE_COMMAND", "howl", 5, "Howl 5 times")]),
        ("CL_DEM_03", CLASS_DEMON, "Pit Lord",
         "Command hellfire and the art of demonic attack.",
         [("CLASS_POWER", "attack", 7, "Raise Attack to level 7"),
          ("CLASS_POWER", "hellfire", 5, "Raise Hellfire to level 5")]),
        ("CL_MNK_03", CLASS_MONK, "Iron Fist",
         "Your martial arts prowess and weapon skill grow fearsome.",
         [("CLASS_POWER", "martial", 7, "Raise Martial Arts to level 7"),
          ("WEAPON_SKILL", "any", 150, "Raise any weapon skill to 150")]),
        ("CL_MAG_03", CLASS_MAGE, "Archmage",
         "Your mastery of the arcane arts deepens across all spell colors.",
         [("SPELL_SKILL", "all", 150, "Raise all spell colors to 150"),
          ("CLASS_POWER", "arcane", 7, "Raise Arcane to level 7")]),
        ("CL_NIN_03", CLASS_NINJA, "Shadow Master",
         "Become one with the shadows through advanced ninja arts.",
         [("CLASS_POWER", "shadow", 7, "Raise Shadow to level 7"),
          ("USE_COMMAND", "koryou", 3, "Use koryou 3 times")]),
        ("CL_DRW_03", CLASS_DROW, "Matron's Favor",
         "The spider goddess smiles upon your mastery of drow arts.",
         [("CLASS_POWER", "spider", 7, "Raise Spider to level 7")]),
        ("CL_DGS_03", CLASS_DIRGESINGER, "Maestro",
         "Your songs resonate with power beyond mortal comprehension.",
         [("CLASS_POWER", "song", 7, "Raise Song to level 7")]),
        ("CL_PSI_03", CLASS_PSION, "Psychic",
         "Your mind reaches into realms most cannot perceive.",
         [("CLASS_POWER", "psionic", 7, "Raise Psionic to level 7")]),
        ("CL_DKN_03", CLASS_DRAGONKIN, "Drake",
         "The dragon within grows stronger with each passing day.",
         [("CLASS_POWER", "dragon", 7, "Raise Dragon to level 7")]),
        ("CL_ART_03", CLASS_ARTIFICER, "Engineer",
         "Your technological creations grow more sophisticated.",
         [("CLASS_POWER", "tech", 7, "Raise Tech to level 7")]),
        ("CL_CLT_03", CLASS_CULTIST, "Dark Acolyte",
         "The void's whispers grow clearer as your devotion deepens.",
         [("CLASS_POWER", "void", 7, "Raise Void to level 7")]),
        ("CL_CHR_03", CLASS_CHRONOMANCER, "Time Weaver",
         "You weave the threads of time with increasing confidence.",
         [("CLASS_POWER", "time", 7, "Raise Time to level 7")]),
        ("CL_SHM_03", CLASS_SHAMAN, "Spirit Walker",
         "You walk between worlds, communing with powerful spirits.",
         [("CLASS_POWER", "spirit", 7, "Raise Spirit to level 7")]),
    ]

    for (qid, cls, name, desc, objs) in class_p3:
        pfx = qid.rsplit("_", 1)[0]  # e.g. CL_VAMP
        quest(conn, qid, name, desc, "CL", 3, AC, qp=1000,
              req_class=cls, prereqs=["M06", f"{pfx}_02"], objectives=objs)


# ---------------------------------------------------------------------------
# Phase 4: Ascension
# ---------------------------------------------------------------------------

def seed_phase4(conn):
    AC = QFLAG_AUTO_COMPLETE

    quest(conn, "M08", "Path to Power",
          "Your strength and knowledge have prepared you for the road ahead.",
          "M", 4, AC, qp=2000,
          prereqs=["M07", "T_STANCE_03"],
          objectives=[
              ("REACH_STAT", "hp", 40000, "Reach 40,000 hit points"),
              ("REACH_STAT", "mana", 30000, "Reach 30,000 mana"),
              ("EARN_QP", "lifetime", 15000, "Earn 15,000 lifetime quest points"),
          ])

    quest(conn, "M09", "Ancient Blood",
          "Your generation grows ever lower. Ancient power flows in your veins.",
          "M", 4, AC, qp=2500,
          prereqs=["M08"],
          objectives=[("REACH_GEN", "gen", 3, "Reach generation 3 or lower")])

    quest(conn, "M10", "True Power",
          "Reach the pinnacle of generation and prove yourself in player combat.",
          "M", 4, AC, qp=3000,
          prereqs=["M09"],
          objectives=[
              ("REACH_GEN", "gen", 1, "Reach generation 1"),
              ("REACH_PKSCORE", "pk", 500, "Reach PK score 500"),
          ])

    # Superstance chain
    quest(conn, "T_SS_01", "Beyond the Basics",
          "Learn your first superstance by mastering two basic stances to 200.",
          "T", 4, AC, qp=1000,
          prereqs=["M08"],
          objectives=[
              ("LEARN_SUPERSTANCE", "any", 1, "Learn 1 superstance"),
          ])

    quest(conn, "T_SS_02", "Expanding Mastery",
          "Continue unlocking superstances by mastering more basic stances.",
          "T", 4, AC, qp=1500,
          prereqs=["T_SS_01"],
          objectives=[
              ("LEARN_SUPERSTANCE", "any", 3, "Learn 3 superstances"),
          ])

    quest(conn, "T_SS_03", "Supreme Stances",
          "Master all five superstances - a prerequisite for class upgrade.",
          "T", 4, AC, qp=2000,
          prereqs=["T_SS_02"],
          objectives=[
              ("LEARN_SUPERSTANCE", "all", 5, "Learn all 5 superstances"),
          ])

    # First upgrade
    quest(conn, "M11", "The Elite Transition",
          "You have met all requirements. Upgrade your class at the Altar of Midgaard.",
          "M", 4, AC, qp=5000,
          prereqs=["M10", "T_SS_03"],
          objectives=[
              ("REACH_STAT", "hp", 50000, "Reach 50,000 hit points"),
              ("REACH_STAT", "mana", 35000, "Reach 35,000 mana"),
              ("REACH_STAT", "move", 35000, "Reach 35,000 movement"),
              ("EARN_QP", "lifetime", 40000, "Earn 40,000 lifetime quest points"),
              ("REACH_PKSCORE", "pk", 1000, "Reach PK score 1,000"),
              ("REACH_UPGRADE", "upgrade", 1, "Complete your first upgrade"),
          ])

    # PvP
    quest(conn, "C06", "PK Warrior",
          "Make your mark in player combat.",
          "C", 4, AC, qp=1000,
          prereqs=["M08"],
          objectives=[("KILL_PLAYER", "any", 25, "Win 25 PvP fights")])

    quest(conn, "C07", "Arena Champion",
          "Dominate both the arena and the battlefield.",
          "C", 4, AC, qp=2000,
          prereqs=["C05", "C06"],
          objectives=[
              ("ARENA_WIN", "any", 50, "Win 50 arena fights"),
              ("REACH_PKSCORE", "pk", 1000, "Reach PK score 1,000"),
          ])

    # Forge mastery
    quest(conn, "CR03", "Master Smith",
          "You have forged enough to be considered a master craftsman.",
          "CR", 4, AC, qp=1000,
          prereqs=["CR02", "M08"],
          objectives=[("FORGE_ITEM", "any", 10, "Forge 10 items total")])

    # Mastery
    quest(conn, "CL_MASTERY", "Weapons Master",
          "Achieve mastery by raising all weapons, spells, and stances to 200.",
          "CL", 4, AC, qp=3000,
          prereqs=["M08"],
          objectives=[("MASTERY", "flag", 1, "Achieve mastery (all wpn/spl/stance at 200)")])

    # Exploration
    quest(conn, "E05", "Pathfinder",
          "You have explored the vast majority of the known world.",
          "E", 4, AC, qp=2000,
          prereqs=["E04"],
          objectives=[("VISIT_AREA", "any", 70, "Visit 70 different areas")])

    # Archaic
    quest(conn, "A03", "Card Master",
          "Your collection of completed quest cards is legendary.",
          "A", 4, AC, qp=1500,
          prereqs=["A02"],
          objectives=[
              ("COMPLETE_QUEST", "archaic_card", 15, "Complete 15 quest cards"),
          ])


# ---------------------------------------------------------------------------
# Phase 4: Per-Class Quests (CL_xxx_04)
# ---------------------------------------------------------------------------

def seed_class_phase4(conn):
    AC = QFLAG_AUTO_COMPLETE

    class_p4 = [
        ("CL_VAMP_04", CLASS_VAMPIRE, "Ancient Disciplines",
         "You have mastered the deepest vampire discipline secrets.",
         [("CLASS_POWER", "protean", 10, "Raise Protean to level 10"),
          ("CLASS_POWER", "presence", 7, "Raise Presence to level 7")]),
        ("CL_WW_04", CLASS_WEREWOLF, "Primal Beast",
         "The beast within has reached its full terrifying potential.",
         [("CLASS_POWER", "gifts", 10, "Raise Gifts to level 10"),
          ("CLASS_POWER", "luna", 5, "Raise Luna to level 5")]),
        ("CL_DEM_04", CLASS_DEMON, "Archfiend",
         "You command the infernal arts at their highest level.",
         [("CLASS_POWER", "attack", 10, "Raise Attack to level 10"),
          ("CLASS_POWER", "temptation", 7, "Raise Temptation to level 7")]),
        ("CL_MNK_04", CLASS_MONK, "Grandmaster",
         "All weapons bow to your mastery. Your martial arts are peerless.",
         [("CLASS_POWER", "martial", 10, "Raise Martial Arts to level 10"),
          ("WEAPON_SKILL", "all", 200, "Raise all weapon skills to 200")]),
        ("CL_MAG_04", CLASS_MAGE, "Grand Sorcerer",
         "All five spell colors flow through you at their peak.",
         [("SPELL_SKILL", "all", 200, "Raise all spell colors to 200"),
          ("CLASS_POWER", "arcane", 10, "Raise Arcane to level 10")]),
        ("CL_NIN_04", CLASS_NINJA, "Invisible Death",
         "You are the unseen blade. None escape your shadow.",
         [("CLASS_POWER", "shadow", 10, "Raise Shadow to level 10")]),
        ("CL_DRW_04", CLASS_DROW, "Underdark Lord",
         "You rule the spider's domain with absolute authority.",
         [("CLASS_POWER", "spider", 10, "Raise Spider to level 10")]),
        ("CL_DGS_04", CLASS_DIRGESINGER, "Virtuoso",
         "Your songs can shake the foundations of reality.",
         [("CLASS_POWER", "song", 10, "Raise Song to level 10")]),
        ("CL_PSI_04", CLASS_PSION, "Mind Lord",
         "Your psychic powers are unmatched across all realms.",
         [("CLASS_POWER", "psionic", 10, "Raise Psionic to level 10")]),
        ("CL_DKN_04", CLASS_DRAGONKIN, "Elder Dragon",
         "Ancient draconic power surges through every fiber of your being.",
         [("CLASS_POWER", "dragon", 10, "Raise Dragon to level 10")]),
        ("CL_ART_04", CLASS_ARTIFICER, "Grand Inventor",
         "Your technological genius has no equal.",
         [("CLASS_POWER", "tech", 10, "Raise Tech to level 10")]),
        ("CL_CLT_04", CLASS_CULTIST, "High Priest",
         "The void answers your every call. You are its chosen vessel.",
         [("CLASS_POWER", "void", 10, "Raise Void to level 10")]),
        ("CL_CHR_04", CLASS_CHRONOMANCER, "Temporal Lord",
         "Time itself bends to your will.",
         [("CLASS_POWER", "time", 10, "Raise Time to level 10")]),
        ("CL_SHM_04", CLASS_SHAMAN, "Spirit Elder",
         "The greatest spirits answer your summons.",
         [("CLASS_POWER", "spirit", 10, "Raise Spirit to level 10")]),
    ]

    for (qid, cls, name, desc, objs) in class_p4:
        pfx = qid.rsplit("_", 1)[0]  # e.g. CL_VAMP
        quest(conn, qid, name, desc, "CL", 4, AC, qp=2000,
              req_class=cls, prereqs=[f"{pfx}_03", "M08"], objectives=objs)


# ---------------------------------------------------------------------------
# Phase 5: Transcendence
# ---------------------------------------------------------------------------

def seed_phase5(conn):
    AC = QFLAG_AUTO_COMPLETE

    quest(conn, "M12", "Upgrade II: Refined",
          "Push beyond your first upgrade and refine your power.",
          "M", 5, AC, qp=5000,
          prereqs=["M11"],
          objectives=[
              ("REACH_STAT", "hp", 90000, "Reach 90,000 hit points"),
              ("REACH_STAT", "mana", 90000, "Reach 90,000 mana"),
              ("REACH_STAT", "move", 90000, "Reach 90,000 movement"),
              ("EARN_QP", "lifetime", 80000, "Earn 80,000 lifetime quest points"),
              ("REACH_PKSCORE", "pk", 2000, "Reach PK score 2,000"),
              ("REACH_UPGRADE", "upgrade", 2, "Complete upgrade level 2"),
          ])

    quest(conn, "M13", "Upgrade III: Ascended",
          "Ascend to the third tier of power.",
          "M", 5, AC, qp=7500,
          prereqs=["M12"],
          objectives=[
              ("REACH_STAT", "hp", 100000, "Reach 100,000 hit points"),
              ("REACH_STAT", "mana", 100000, "Reach 100,000 mana"),
              ("REACH_STAT", "move", 100000, "Reach 100,000 movement"),
              ("EARN_QP", "lifetime", 120000, "Earn 120,000 lifetime quest points"),
              ("REACH_PKSCORE", "pk", 2500, "Reach PK score 2,500"),
              ("REACH_UPGRADE", "upgrade", 3, "Complete upgrade level 3"),
          ])

    quest(conn, "M14", "Upgrade IV: Transcendent",
          "Reach the fourth and final tier of transcendent power.",
          "M", 5, AC, qp=10000,
          prereqs=["M13"],
          objectives=[
              ("REACH_STAT", "hp", 110000, "Reach 110,000 hit points"),
              ("REACH_STAT", "mana", 110000, "Reach 110,000 mana"),
              ("REACH_STAT", "move", 110000, "Reach 110,000 movement"),
              ("EARN_QP", "lifetime", 160000, "Earn 160,000 lifetime quest points"),
              ("REACH_PKSCORE", "pk", 3000, "Reach PK score 3,000"),
              ("REACH_UPGRADE", "upgrade", 4, "Complete upgrade level 4"),
          ])

    quest(conn, "M15", "Apex Predator",
          "You have reached the pinnacle. You are the Apex.",
          "M", 5, AC, qp=0,  # Reward is the title
          prereqs=["M14"],
          objectives=[
              ("COMPLETE_QUEST", "M14", 1, "Complete Upgrade IV: Transcendent"),
          ])

    # PvP endgame
    quest(conn, "C08", "Bounty Hunter",
          "You have become a fearsome player killer.",
          "C", 5, AC, qp=2000,
          prereqs=["M11"],
          objectives=[("KILL_PLAYER", "any", 100, "Win 100 PvP fights")])

    quest(conn, "C09", "Warlord",
          "Your PK score strikes fear into the hearts of all.",
          "C", 5, AC, qp=3000,
          prereqs=["C08", "M12"],
          objectives=[("REACH_PKSCORE", "pk", 3000, "Reach PK score 3,000")])

    quest(conn, "C10", "Legend",
          "You are a legend. Your name will echo through eternity.",
          "C", 5, AC, qp=5000,
          prereqs=["C09", "M13"],
          objectives=[("REACH_PKSCORE", "pk", 5000, "Reach PK score 5,000")])

    # Crafting
    quest(conn, "CR04", "Legendary Craftsman",
          "Your crafting skill is without peer in all the land.",
          "CR", 5, AC, qp=3000,
          prereqs=["CR03", "M11"],
          objectives=[
              ("QUEST_MODIFY", "any", 20, "Modify 20 quest item properties"),
              ("FORGE_ITEM", "any", 25, "Forge 25 items total"),
          ])

    # Exploration
    quest(conn, "E06", "Atlas",
          "You have set foot in every known area of the world.",
          "E", 5, AC, qp=5000,
          prereqs=["E05", "M11"],
          objectives=[("VISIT_AREA", "all", 1, "Visit every area")])

    # Daily
    quest(conn, "D03", "Daily: Proving Grounds",
          "Test your mettle against other players in daily combat.",
          "D", 5, QFLAG_AUTO_COMPLETE | QFLAG_REPEATABLE, qp=200,
          prereqs=["M11"],
          objectives=[("KILL_PLAYER", "any", 3, "Win 3 PvP fights")])

    # Archaic
    quest(conn, "A04", "Archaic Legend",
          "You have completed more quest cards than most will in a lifetime.",
          "A", 5, AC, qp=5000,
          prereqs=["A03", "M11"],
          objectives=[
              ("COMPLETE_QUEST", "archaic_card", 50, "Complete 50 quest cards"),
          ])


# ---------------------------------------------------------------------------
# Phase 5: Per-Class Upgraded Quests (CL_xxx_U01 and CL_xxx_U02)
# ---------------------------------------------------------------------------

def seed_class_upgraded(conn):
    AC = QFLAG_AUTO_COMPLETE

    upgraded = [
        ("CL_TAN", CLASS_TANARRI, "Tanarri",
         "Tanarri Awakening", "Embrace your new Tanarri form and powers.",
         [("USE_COMMAND", "demonform", 1, "Enter Tanarri-specific demon form")],
         "Tanarri Mastery", "Master the dark arts of the Tanarri.",
         [("CLASS_POWER", "tanarri", 5, "Raise Tanarri power to level 5")]),

        ("CL_DRD", CLASS_DROID, "Spider Droid",
         "Droid Online", "Activate your new droid systems.",
         [("USE_COMMAND", "power", 1, "Activate droid power systems")],
         "Droid Mastery", "Optimize all droid subsystems.",
         [("CLASS_POWER", "droid", 5, "Raise Droid power to level 5")]),

        ("CL_SAM", CLASS_SAMURAI, "Samurai",
         "Way of Bushido", "Walk the path of the samurai warrior.",
         [("USE_COMMAND", "koryou", 1, "Use samurai koryou technique")],
         "Sword Saint", "Achieve legendary weapon mastery.",
         [("WEAPON_SKILL", "any", 500, "Raise any weapon skill to 500"),
          ("CLASS_POWER", "bushido", 5, "Raise Bushido to level 5")]),

        ("CL_UDK", CLASS_UNDEAD_KNIGHT, "Undead Knight",
         "Death's Embrace", "Embrace the powers of undeath.",
         [("USE_COMMAND", "darkheart", 1, "Activate darkheart")],
         "Death Knight Mastery", "Command death's ultimate power.",
         [("CLASS_POWER", "death", 5, "Raise Death power to level 5")]),

        ("CL_ANG", CLASS_ANGEL, "Angel",
         "Divine Form", "Ascend to your angelic form.",
         [("USE_COMMAND", "spiritform", 1, "Enter spirit form"),
          ("USE_COMMAND", "halo", 1, "Manifest your halo")],
         "Seraphim", "Become a Seraphim of divine power.",
         [("CLASS_POWER", "angelic", 5, "Raise Angelic power to level 5")]),

        ("CL_SHP", CLASS_SHAPESHIFTER, "Shapeshifter",
         "Shifting Forms", "Master the art of morphological transformation.",
         [("USE_COMMAND", "morph", 1, "Use morph ability")],
         "Master Shifter", "Perfect the art of shapeshifting.",
         [("CLASS_POWER", "morphosis", 5, "Raise Morphosis to level 5")]),

        ("CL_LCH", CLASS_LICH, "Lich",
         "Undeath Rises", "Embrace your new lichdom.",
         [("USE_COMMAND", "lichform", 1, "Enter lich form")],
         "Arch-Lich", "Command necromantic power beyond mortal ken.",
         [("CLASS_POWER", "necromancy", 5, "Raise Necromancy to level 5")]),

        ("CL_SRN", CLASS_SIREN, "Siren",
         "Siren Song", "Learn the enchanting songs of the Siren.",
         [("CLASS_TRAIN", "voicetrain", 1, "Begin Siren voice training")],
         "Voice Mastery", "Your voice can shatter minds and bend wills.",
         [("CLASS_POWER", "voice", 5, "Raise Voice power to level 5")]),

        ("CL_MFL", CLASS_MINDFLAYER, "Mindflayer",
         "Cerebral Dominion", "Unleash the full power of your evolved mind.",
         [("CLASS_TRAIN", "mindtrain", 1, "Begin Mindflayer training")],
         "Elder Brain", "Your psychic dominion is absolute.",
         [("CLASS_POWER", "mind", 5, "Raise Mind power to level 5")]),

        ("CL_WYR", CLASS_WYRM, "Wyrm",
         "Wyrm Awakening", "Awaken the ancient wyrm within.",
         [("CLASS_TRAIN", "wyrmtrain", 1, "Begin Wyrm training")],
         "Ancient Wyrm", "You are an ancient wyrm of devastating power.",
         [("CLASS_POWER", "wyrm", 5, "Raise Wyrm power to level 5")]),

        ("CL_MCH", CLASS_MECHANIST, "Mechanist",
         "Cybernetic Evolution", "Upgrade to the Mechanist's cybernetic systems.",
         [("CLASS_TRAIN", "cybtrain", 1, "Begin cybernetic training")],
         "Grand Mechanist", "Your cybernetic mastery is unmatched.",
         [("CLASS_POWER", "cyber", 5, "Raise Cyber power to level 5")]),

        ("CL_VBN", CLASS_VOIDBORN, "Voidborn",
         "Void Embrace", "Become one with the void itself.",
         [("USE_COMMAND", "voidform", 1, "Enter Voidborn form")],
         "Void Master", "The void answers your every command.",
         [("CLASS_POWER", "voidborn", 5, "Raise Voidborn power to level 5")]),

        ("CL_PAR", CLASS_PARADOX, "Paradox",
         "Paradox Unleashed", "Wield the paradoxical forces of broken time.",
         [("CLASS_TRAIN", "paratrain", 1, "Begin Paradox training")],
         "Temporal Paradox", "You exist outside the bounds of causality.",
         [("CLASS_POWER", "paradox", 5, "Raise Paradox power to level 5")]),

        ("CL_SPL", CLASS_SPIRITLORD, "Spirit Lord",
         "Lord of Spirits", "Command spirits as their sovereign lord.",
         [("CLASS_TRAIN", "lordtrain", 1, "Begin Spirit Lord training")],
         "Spirit Sovereign", "All spirits bow to your absolute authority.",
         [("CLASS_POWER", "lordship", 5, "Raise Lordship to level 5")]),
    ]

    for (pfx, cls, cls_name, n1, d1, o1, n2, d2, o2) in upgraded:
        quest(conn, f"{pfx}_U01", n1, d1, "CL", 5, AC, qp=1000,
              req_class=cls, prereqs=["M11"], objectives=o1)
        quest(conn, f"{pfx}_U02", n2, d2, "CL", 5, AC, qp=2000,
              req_class=cls, prereqs=[f"{pfx}_U01"], objectives=o2)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    root = find_repo_root()
    db_path = root / "gamedata" / "db" / "game" / "quest.db"

    print(f"Creating quest database: {db_path}")

    # Remove existing
    if db_path.exists():
        db_path.unlink()
        print("  Removed existing quest.db")

    conn = sqlite3.connect(str(db_path))
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA foreign_keys=ON")

    create_schema(conn)

    seed_phase0(conn)
    seed_phase1(conn)
    seed_phase2(conn)
    seed_class_phase2(conn)
    seed_phase3(conn)
    seed_class_phase3(conn)
    seed_phase4(conn)
    seed_class_phase4(conn)
    seed_phase5(conn)
    seed_class_upgraded(conn)

    conn.commit()

    # Summary
    cur = conn.cursor()
    cur.execute("SELECT COUNT(*) FROM quest_defs")
    n_quests = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM quest_objectives")
    n_objs = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM quest_prerequisites")
    n_prereqs = cur.fetchone()[0]
    cur.execute("SELECT category, COUNT(*) FROM quest_defs GROUP BY category ORDER BY category")
    cats = cur.fetchall()

    print(f"\nQuest database created successfully:")
    print(f"  {n_quests} quest definitions")
    print(f"  {n_objs} objectives")
    print(f"  {n_prereqs} prerequisites")
    print(f"\n  By category:")
    for cat, count in cats:
        print(f"    {cat}: {count}")

    conn.close()
    print(f"\nDone: {db_path}")


if __name__ == "__main__":
    main()
