# Quest System Design Document

## Overview

A unified graph-based quest system that guides players from brand-new to max upgrade level (4), with branching paths that teach every major game system. Integrates the existing quest card system as "Archaic Quests".

## Goals

1. **Teach by Doing**: Each quest introduces a system by requiring the player to use it
2. **Strict Gating**: All system tutorials (forge, disciplines, quest items, class training) required before advancing
3. **Per-Class Chains**: Every class has a unique quest chain teaching their specific abilities
4. **FTUE-Aware**: Tutorial depth adapts to player's declared experience level
5. **Modern DB**: Dedicated quest.db for definitions, player.db for progress

## Quest Categories

| Code | Category | Description |
|------|----------|-------------|
| **M** | Main | Core progression milestones (required gates) |
| **T** | Tutorial | System-teaching quests (contextual to FTUE level) |
| **C** | Combat | Kill achievements, PvP milestones, arena |
| **E** | Exploration | Area discovery, room visits |
| **CL** | Class | Class-specific progression (14 base + 14 upgraded) |
| **CR** | Crafting | Forge, quest items, equipment |
| **D** | Daily | Repeatable content |
| **A** | Archaic | Migrated quest card system |

## Quest Count

| Category | In DB | Per Player | Notes |
|----------|-------|-----------|-------|
| Tutorial (T01-T08) | 8 | 3-5 | FTUE-gated |
| Main (M01-M15) | 15 | 15 | Required chain |
| Tutorial-System (T_*) | 13 | 13 | Forge, stances, disciplines, etc. |
| Combat (C01-C10) | 10 | 10 | Kill/PvP/arena |
| Exploration (E01-E06) | 6 | 6 | Area discovery |
| Class Base (CL_xxx_01-04) | 56 | 4 | Only your class visible |
| Class Upgraded (CL_xxx_U01-02) | 28 | 2 | Post-upgrade |
| Class Mastery | 1 | 1 | All wpn/spl/stance at 200 |
| Crafting (CR01-CR04) | 4 | 4 | Forge + quest items |
| Daily (D01-D03) | 3 | 3 | Repeatable |
| Archaic (A01-A04) | 4 | 4 | Quest cards |
| **Total** | **148** | **~65** | |

## Progression Phases

### Phase 0: Tutorial (FTUE-Gated)

```
FTUE 0 (never MUD'd):  T01 → T02 → T03 → T04 → T05 → M01
FTUE 1 (MUD veteran):  T06 → T07 → T08 → M01
FTUE 2 (Dystopia vet): (skip all) → M01
```

| ID | Name | Objectives |
|----|------|-----------|
| T01 | Finding Your Feet | look, score, inventory |
| T02 | The World Around You | move, exits, scan |
| T03 | Speaking Up | say, chat |
| T04 | Gear Up | equipment, get, wear |
| T05 | First Fight | Kill 1 mob |
| T06 | Dystopia Orientation | score, scan, map |
| T07 | Know Your Tools | help, commands |
| T08 | Combat Primer | Kill 3 mobs, consider |

### Phase 1: Foundation — Class Selection & First Systems

```
M01 (clasself) → M02 (kill 25) → M03 (10K HP) → M04 (500 QP)
  └→ T_PRACTICE_01 (practice 3 skills)
  └→ T_STANCE_01 (use stance, raise to 10)
```

### Phase 2: Growth — Core System Mastery (ALL required for M06)

```
M05 (25K HP) → ┬→ T_DISC_01 → T_DISC_02 (research, disc lv5) ──┐
               ├→ T_FORGE_01 → T_FORGE_02 (forge 3 items) ──────┤
               ├→ T_QITEM_01 → T_QITEM_02 (create+modify items) ┤
               ├→ CL_xxx_01 → CL_xxx_02 (class abilities) ──────┤
               ├→ C01 → C02 → C03 (mob kills)                    │
               ├→ E01 → E02 (area exploration)                    │
               └→ A01 (quest card)                                │
                                                                  ▼
                    M06 requires ALL of: T_DISC_02 + T_FORGE_02 + T_QITEM_02 + CL_xxx_02
```

**Per-class Phase 2 chains** (14 classes, only yours visible):

| Class | CL_xxx_01 | CL_xxx_02 |
|-------|-----------|-----------|
| Vampire | claws + fangs | clandisc, Protean 3 |
| Werewolf | rage + wolf form | totems, Gifts 3 |
| Demon | horns + wings | demonform, Attack 3 |
| Monk | chi + meditate | kick, Martial 3 |
| Mage | cast 3x, spell 50 | research, spell 100 |
| Ninja | vanish + backstab | mitsukeru, Shadow 3 |
| Drow | web + shadowplane | nightsight, Spider 3 |
| Dirgesinger | sing | songtrain, Song 3 |
| Psion | focus | psitrain, Psionic 3 |
| Dragonkin | breathe | dragontrain, Dragon 3 |
| Artificer | turret + blaster | techtrain, Tech 3 |
| Cultist | ritual | voidtrain, Void 3 |
| Chronomancer | flux + quicken | timetrain, Time 3 |
| Shaman | commune | spirittrain, Spirit 3 |

### Phase 3: Power — PvP, Stances, Generation

```
M06 → M07 (Gen 5) → M08 (40K HP, 30K mana, 15K QP)
  ├→ T_STANCE_02 → T_STANCE_03 (5 stances at 200)
  ├→ T_TRAIN_01 (use train command)
  ├→ CR01 → CR02 (forge metal + gem)
  ├→ CL_xxx_03 (class power 7)
  ├→ C04 → C05 (arena wins)
  ├→ E03 → E04 (30-50 areas)
  └→ D01, D02 (dailies unlock)
```

### Phase 4: Ascension — Road to First Upgrade

```
M08 → M09 (Gen 3) → M10 (Gen 1, PK 500) → M11 (FIRST UPGRADE)
  ├→ T_SS_01 → T_SS_02 → T_SS_03 (all 5 superstances, REQUIRED for M11)
  ├→ CL_xxx_04 (class power 10) → CL_MASTERY (all wpn/spl/stance 200)
  ├→ C06 → C07 (PK + arena champion)
  └→ CR03, E05, A03
```

### Phase 5: Transcendence — Upgrades 2-4

```
M11 → M12 (Upgrade II) → M13 (Upgrade III) → M14 (Upgrade IV) → M15 (Apex)
  ├→ CL_xxx_U01 → CL_xxx_U02 (upgraded class abilities)
  ├→ C08 → C09 → C10 (PK legend)
  └→ CR04, E06, D03, A04
```

**Per-class upgraded chains** (14 upgraded classes):

| Upgraded | From | CL_xxx_U01 | CL_xxx_U02 |
|----------|------|-----------|-----------|
| Tanarri | Demon | Tanarri form | power 5 |
| Spider Droid | Drow | power command | droid 5 |
| Samurai | Ninja | bushido stance | wpn 500 |
| Undead Knight | Vampire | darkheart | death 5 |
| Angel | Monk | spiritform + halo | angelic 5 |
| Shapeshifter | Werewolf | morph | morphosis 5 |
| Lich | Mage | lichform | necromancy 5 |
| Siren | Dirgesinger | voicetrain | voice 5 |
| Mindflayer | Psion | mindtrain | mind 5 |
| Wyrm | Dragonkin | wyrmtrain | wyrm 5 |
| Mechanist | Artificer | cybtrain | cyber 5 |
| Voidborn | Cultist | voidform | voidborn 5 |
| Paradox | Chronomancer | paratrain | paradox 5 |
| Spirit Lord | Shaman | lordtrain | lordship 5 |

## Database Schema

### quest.db (gamedata/db/game/quest.db) — Shipped with game

```sql
CREATE TABLE quest_defs (
    id              TEXT PRIMARY KEY,
    name            TEXT NOT NULL,
    description     TEXT NOT NULL,
    category        TEXT NOT NULL,       -- M/T/C/E/CL/CR/D/A
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
```

### player.db — Per-Character Progress

```sql
CREATE TABLE quest_progress (
    quest_id     TEXT PRIMARY KEY,
    status       INTEGER NOT NULL DEFAULT 0,
    started_at   INTEGER NOT NULL DEFAULT 0,
    completed_at INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE quest_obj_progress (
    quest_id    TEXT NOT NULL,
    obj_index   INTEGER NOT NULL,
    current     INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (quest_id, obj_index)
);
```

## Objective Types

| Type | Description | Hook |
|------|-------------|------|
| USE_COMMAND | Execute command N times | interp.c |
| KILL_MOB | Kill N mobs | fight.c |
| KILL_PLAYER | Win N PvP fights | fight.c |
| VISIT_ROOM | Enter specific room | act_move.c |
| VISIT_AREA | Enter any room in area | act_move.c |
| REACH_STAT | HP/Mana/Move threshold | handler.c / tick |
| REACH_GEN | Generation reaches N | train / PvP |
| REACH_PKSCORE | PKScore threshold | fight.c |
| REACH_UPGRADE | Upgrade level N | upgrade.c |
| EARN_QP | Lifetime QP total | QP grant sites |
| LEARN_STANCE | Stance skill threshold | kav_fight.c |
| LEARN_SUPERSTANCE | Superstance learned | stance unlock |
| LEARN_DISCIPLINE | Discipline level N | research tick |
| WEAPON_SKILL | Weapon skills reach N | combat |
| SPELL_SKILL | Spell colors reach N | spell improvement |
| FORGE_ITEM | Forge with material | kav_wiz.c |
| QUEST_CREATE | Create quest item | kav_wiz.c |
| QUEST_MODIFY | Modify quest item | kav_wiz.c |
| ARENA_WIN | Win arena fights | arena.c |
| COLLECT_ITEM | Possess item by vnum | act_obj.c / tick |
| COMPLETE_QUEST | Another quest done | quest system |
| MASTERY | Achieve mastery flag | jobo_act.c |
| CLASS_TRAIN | Use class train cmd | class do_*train() |
| CLASS_POWER | Class power level N | class power arrays |

## Command Interface

```
quest                 - Show active quests
quest list [category] - Show available quests
quest accept <id>     - Accept a quest
quest progress        - Show detailed progress
quest complete        - Turn in completed quest
quest abandon <id>    - Abandon a quest
quest path            - Show main progression path
quest history         - Show completed quests
```

## Files to Create

```
game/src/systems/quest_new.h    - Structures, constants, prototypes
game/src/systems/quest_new.c    - Quest engine (accept/track/complete/display)
game/src/db/db_quest.h          - Database prototypes
game/src/db/db_quest.c          - SQLite ops for quest.db + player progress
gamedata/db/game/quest.db       - Quest definition database
```

## Files to Modify

- `char.h` — Add quest_tracker pointer to PC_DATA
- `interp.c` — Register quest command, add USE_COMMAND hook
- `db_game.c` — Add quest_db_init() call during boot
- `db_player.c` — Add quest_progress tables to player schema
- `fight.c`, `act_move.c`, `upgrade.c`, `kav_fight.c` — Event hooks
- `kav_wiz.c`, `act_obj.c`, `arena.c`, `jobo_act.c` — Event hooks
- `game/src/classes/*.c` — quest_check() in each do_*train() (14 files)

## Implementation Phases

1. **Schema + Core Engine** — quest.db, structures, load/save, quest command
2. **Event Hooks** — quest_check_progress() at all hook points
3. **Tutorial + Foundation** — T01-T08, M01-M04
4. **Growth + Power** — M05-M08, all side tracks
5. **Ascension + Endgame** — M09-M15, upgrade chain
6. **Daily + Archaic** — Repeatable quests, quest card migration
7. **GMCP + Polish** — Client protocol support

## Archaic Quest Migration

The old quest card system (spell_quest, do_complete, quest_object[]) becomes:
1. Player uses primal energy to "accept" an Archaic Quest
2. System generates 4 random COLLECT_ITEM objectives
3. Player hunts items, uses `quest complete` to submit
4. Preserves existing gameplay, unified with new system

## Full Plan Reference

Complete quest tree with every node, per-class details, and DB access patterns:
`~/.claude/plans/async-strolling-meteor.md`
