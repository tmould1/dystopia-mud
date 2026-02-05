# Quest System Design Document

## Overview

A unified graph-based quest system that guides players from brand-new to max upgrade level (4), integrating the existing quest card system as "Archaic Quests".

## Goals

1. **Player Guidance**: Clear progression path from newbie to max upgrade
2. **System Unification**: Migrate old quest cards into new framework
3. **Extensibility**: Support multiple quest categories (progression, exploration, combat, daily)
4. **Modern Features**: GMCP integration for MUD clients

## Quest Categories

| Category | Description |
|----------|-------------|
| **Progression** | Main path from newbie to max upgrade (milestone quests, auto-complete) |
| **Archaic** | Migrated quest card collection system (4 random items to collect) |
| **Exploration** | Visit rooms/areas, discover hidden locations |
| **Combat** | Kill mobs, PvP, arena battles |
| **Daily** | Repeatable quests that reset daily |
| **Class** | Class-specific progression quests |

## Main Progression Chain

```
[Q1: First Steps] - Learn look, score, visit temple
      |
[Q2: Choose Your Path] - Select a class
      |
[Q3: Building Strength] - Reach 10K HP
      |
[Q4: First Blood] - Kill 10 mobs
      |
[Q5: Earning Your Keep] - Earn 1,000 QP
      |
[Q6: Generation Climb] - Reach Gen 3
      |
[Q7: Combat Ready] - Win 5 arena fights
      |
[Q8: Quest Point Hoarder] - Earn 10,000 QP
      |
[Q9: True Power] - Reach Gen 1
      |
[Q10: The Elite Transition] - Complete first upgrade
      |
[Q11: Superstance Seeker] - Learn first superstance
      |
[Q12: PK Warrior] - Reach 1,000 PK score
      |
[Q13: Stance Mastery] - Learn all 5 superstances
      |
[Q14-16: Upgrades II-IV] - Complete remaining upgrades
      |
[Q17: Apex Predator] - Reach max upgrade (level 4)
```

## Technical Design

### Command Interface

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

### Data Structures

**Quest Definition** (in game.db):
- id, name, description
- category, tier, flags
- min_generation, min_upgrade, required_class
- qp_reward, exp_reward
- prerequisites (graph edges to other quest IDs)
- objectives (type, target_value, params, description)

**Player Progress** (in player.db):
- quest_id, status (locked/available/active/complete/finished)
- objective_progress array
- timestamps (started, completed)
- daily completion tracking

### Objective Types

| Type | Description | Params |
|------|-------------|--------|
| KILL_MOB | Kill X mobs | vnum or level range |
| KILL_PLAYER | Win X PvP fights | - |
| VISIT_ROOM | Visit specific room | room vnum |
| VISIT_AREA | Explore area | area vnum |
| EARN_QP | Earn X quest points | cumulative |
| REACH_STAT | Reach HP/Mana/Move | stat type, threshold |
| REACH_GEN | Reach generation | gen number |
| GET_SUPERSTANCE | Acquire superstance | stance index |
| REACH_PKSCORE | Reach PK score | threshold |
| USE_COMMAND | Use command X times | command name |
| REACH_UPGRADE | Complete upgrade | level |
| COLLECT_ITEM | Collect item by vnum | vnum (for archaic) |

### Event Hooks

Insert `quest_check_progress()` calls in:
- `fight.c:raw_kill()` - mob/player kills
- `act_move.c:move_char()` - room entry
- `upgrade.c:do_upgrade()` - upgrade completion
- `handler.c` - stat changes
- Superstance acquisition functions

### GMCP Protocol

```json
Quest.List { "quests": [...] }
Quest.Update { "id": N, "objectives": [...] }
Quest.Complete { "id": N, "name": "...", "rewards": {...} }
```

## Files to Create

```
game/src/systems/quest_new.h    - Structures, constants, prototypes
game/src/systems/quest_new.c    - Main quest logic and commands
game/src/systems/quest_archaic.c - Archaic quest migration layer
game/src/db/db_quest.h          - Database prototypes
game/src/db/db_quest.c          - Quest load/save operations
```

## Files to Modify

- `merc.h` - Add QUEST_TRACKER to PC_DATA
- `interp.c` - Register quest command
- `db_game.c` - Add quest tables to schema
- `db_player.c` - Add progress tables to schema
- `gmcp.c/h` - Add Quest.* support
- `fight.c`, `act_move.c`, `upgrade.c`, `handler.c` - Event hooks
- `kav_wiz.c`, `act_obj.c`, `magic.c` - Remove old quest code (after migration)

## Implementation Phases

1. **Core Infrastructure** - Structures, schemas, basic command
2. **Quest Processing** - Accept/track/complete, event hooks, auto-complete
3. **Archaic Migration** - Port quest cards to new framework
4. **Quest Content** - Main chain + side quests
5. **Daily/GMCP** - Repeatable quests, client integration
6. **Command Migration** - Replace old quest command, cleanup

## Archaic Quest Migration

The old quest card system (spell_quest, do_complete, quest_object[]) becomes:
1. Player uses primal energy to "accept" an Archaic Quest
2. System generates 4 random COLLECT_ITEM objectives
3. Player hunts items, uses `quest complete` to submit
4. Preserves existing gameplay, unified with new system

## Database Schema

See full plan at: `~/.claude/plans/breezy-noodling-tarjan.md`

Contains complete SQL schemas for:
- quest_defs, quest_objectives, quest_prerequisites (game.db)
- quest_progress, quest_obj_progress, quest_daily (player.db)
- quest_archaic_items (migrated item pool)