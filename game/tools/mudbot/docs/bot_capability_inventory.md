# Bot Capability Inventory

Technical reference for what the mudbot can and cannot verify, organized by objective type, story capabilities, failure classification, and class command support.

## Objective Type Support Matrix

All 24 objective types defined in `db_quest.h`, with bot handler status:

| Objective Type | Handler Location | Can Drive? | Notes |
|----------------|------------------|------------|-------|
| `USE_COMMAND` | `quest_bot.py` | **Yes** | Sends command directly; core tutorial handler |
| `KILL_MOB` | `quest_bot.py` `_kill_mobs` | **Yes** | Arena mobs + story area kills; word-boundary matching |
| `KILL_PLAYER` | — | **No** | Requires PvP opponent; stalls without `--enable-pvp` + target |
| `VISIT_ROOM` | — | **Passive** | Server tracks on room entry; not directly driven |
| `VISIT_AREA` | `objective_handlers.py` | **Yes** | Delegates to StoryNavigator (16 areas); needs `goto` |
| `REACH_STAT` (hp/mana/move) | `quest_bot.py` `_train_stats` | **Yes** | Train loop at Temple trainer |
| `REACH_STAT` (class) | `quest_bot.py` `_select_class` | **Yes** | `selfclass <class>` command |
| `REACH_GEN` | `objective_handlers.py` | **Yes** | `train generation`; farms XP in story areas if needed |
| `REACH_PKSCORE` | — | **No** | Requires sustained PvP wins |
| `REACH_UPGRADE` | `objective_handlers.py` | **Yes** | `upgrade` command at altar |
| `EARN_QP` | — | **Passive** | Server accumulates from quest rewards; no active handler |
| `LEARN_STANCE` | `quest_bot.py` (combat) | **Partial** | Needs multi-round combat; instant kills don't train |
| `LEARN_SUPERSTANCE` | `objective_handlers.py` | **Partial** | Cycles through 5 stances; requires base stances at 200 first |
| `LEARN_DISCIPLINE` | `objective_handlers.py` | **Yes** | `research` → kill mobs → detect "finished" → `train` |
| `WEAPON_SKILL` | — | **Passive** | Server tracks via multi-round combat; bot fights in story |
| `SPELL_SKILL` | — | **Passive** | Server tracks via combat casting; bot fights in story |
| `CLASS_POWER` | `objective_handlers.py` | **Yes** | Routes to discipline handler |
| `CLASS_TRAIN` | `objective_handlers.py` | **Yes** | Executes class-specific train cmd (songtrain, psitrain, etc.) |
| `FORGE_ITEM` | `objective_handlers.py` | **Partial** | Checks inventory for materials; `forge` cmd; farms if no materials |
| `QUEST_CREATE` | `objective_handlers.py` | **Yes** | `questcreate create armor` (costs QP) |
| `QUEST_MODIFY` | `objective_handlers.py` | **Yes** | `questcreate all str 1` (costs QP) |
| `ARENA_WIN` | `objective_handlers.py` | **No** | Requires another player in arena; stall-skips if no PvP |
| `COLLECT_ITEM` | — | **No** | Not implemented in bot |
| `COMPLETE_QUEST` | — | **Passive** | Server tracks quest completion chains; bot just continues |
| `MASTERY` | `objective_handlers.py` | **Partial** | Sets autostance "bull"; kills 30+ mobs; all skills to 200 |

### Summary

| Status | Count | Types |
|--------|-------|-------|
| **Yes** (actively driven) | 12 | USE_COMMAND, KILL_MOB, VISIT_AREA, REACH_STAT, REACH_GEN, REACH_UPGRADE, LEARN_DISCIPLINE, CLASS_POWER, CLASS_TRAIN, QUEST_CREATE, QUEST_MODIFY, FORGE_ITEM |
| **Partial** (limited) | 4 | LEARN_STANCE, LEARN_SUPERSTANCE, MASTERY, FORGE_ITEM |
| **Passive** (server-tracked) | 5 | VISIT_ROOM, EARN_QP, WEAPON_SKILL, SPELL_SKILL, COMPLETE_QUEST |
| **No** (cannot drive) | 3 | KILL_PLAYER, REACH_PKSCORE, ARENA_WIN, COLLECT_ITEM |

---

## StoryNavigator Capabilities

The StoryNavigator (`story_navigator.py`) handles all 16 story nodes through a 9-phase state machine:

| Phase | Action | Implementation |
|-------|--------|----------------|
| CHECKING | Query `storyadmin <name>` for current node/progress | Parse StoryState from output |
| NAVIGATING | `goto <npc_vnum>` to reach hub NPC room | Requires immortal `goto` command |
| TALKING_INTRO | `say <intro_keyword>` to hub NPC | Triggers Lua SPEECH script |
| KILLING | Kill mobs from `kill_mobs` list until `kill_threshold` met | Word-boundary mob matching; move to adjacent room after each kill |
| FETCHING | `goto <fetch_room>`, `get <obj>` | Pick up fetch object by vnum |
| EXAMINING | `goto <examine_room>`, `examine <keyword>` | Triggers Lua EXAMINE script |
| TALKING_NPC | `goto <npc_room>`, `say <keyword>` to task NPC | Only node 15 (King) |
| RETURNING | `goto <npc_vnum>`, `say done` to advance | Checks all task_bits set before return |
| IDLE | Node complete, waiting for next dispatch | Returns control to QuestBot |

### Limitations

- **Requires immortal character**: `goto` command needs level 7+ (LEVEL_IMMORTAL). No mortal pathfinding implemented.
- **Mob keyword heuristic**: Kill target matching uses server `look` output and word boundaries; can fail on mobs with common name fragments.
- **Kill tracking desync**: If mob is killed by another player or respawns, kill count may not match story_kills.
- **Node 12 branching**: Always takes Atlantis path; Olympus path untested.
- **No error recovery**: If a story task fails silently (Lua trigger doesn't fire), bot may loop indefinitely until stall detection kicks in.

---

## Failure Taxonomy

Classification codes for why quest verification fails. Used in the [coverage matrix](coverage_matrix.md).

### BL — Bot Limitation

| Code | Description | Affected Quests |
|------|-------------|-----------------|
| BL-001 | Bot cannot drive PvP combat | C06, C07, C08, C09, C10, D03 (KILL_PLAYER, ARENA_WIN, REACH_PKSCORE) |
| BL-002 | Bot cannot reliably acquire forge materials | T_FORGE_01, T_FORGE_02, CR01, CR02, CR03, CR04 (material drops are random) |
| BL-003 | Instant kills prevent stance skill training | T_STANCE_01, T_STANCE_02, T_STANCE_03 (arena mobs die in 1 hit) |
| BL-004 | Bot cannot complete archaic quest cards | A01, A02, A03, A04 (COMPLETE_QUEST archaic_card — quest card system not automated) |
| BL-005 | Bot cannot collect specific items | Any quest with COLLECT_ITEM objective |
| BL-006 | Mastery requires all skills at 200 — very long grind | CL_MASTERY (bot can attempt but cycle budget may be exceeded) |

### SP — Server Prerequisite

| Code | Description | Affected Quests |
|------|-------------|-----------------|
| SP-001 | Quest prerequisites not met (cascading) | Phase 3+ quests depend on Phase 2 completion |
| SP-002 | Class gating — wrong class selected | All CL_xxx quests when bot class doesn't match |
| SP-003 | FTUE gating — wrong explevel for tutorial | T01-T05 invisible at explevel 2; T06-T08 invisible at explevel 0 |
| SP-004 | Stat gates unreachable in cycle budget | M05 (hp 25K), M08 (hp 40K), M11-M14 (hp 50K-110K) |

### IF — Infrastructure

| Code | Description | Affected Quests |
|------|-------------|-----------------|
| IF-001 | StoryNavigator requires immortal `goto` | All story nodes; all VISIT_AREA quests |
| IF-002 | Campaign runner fails at START state | All quests when campaign bot can't connect/login |
| IF-003 | Server not running or port mismatch | All quests |
| IF-004 | Test character missing or wrong level | All quests requiring specific character state |

### SD — Stall Detection

| Code | Description | Affected Quests |
|------|-------------|-----------------|
| SD-001 | Quest stall-skipped (no progress after 15 cycles) | T_STANCE_01 blocks M02 (known); any quest with slow objectives |
| SD-002 | Bot stuck in combat loop (mob too strong / can't flee) | Story node kills in later areas (high-level mobs) |

### SB — Server Bug

| Code | Description | Affected Quests |
|------|-------------|-----------------|
| SB-xxx | *To be filled as bugs are discovered during verification* | — |

---

## Class Command Mapping

From `story_data.py` `CLASS_COMMANDS` — determines how the bot handles `CLASS_TRAIN` and `LEARN_DISCIPLINE` objectives:

### Discipline-based classes (use `research` → kill → `train` cycle)

| Class | disc_default | Train Command |
|-------|-------------|---------------|
| Demon | attack | — (research/train) |
| Vampire | protean | — (research/train) |
| Werewolf | bear | — (research/train) |
| Mage | arcane | — (research/train) |

### Classes with dedicated train commands

| Class | Train Command |
|-------|---------------|
| Dirgesinger | `songtrain` |
| Psion | `psitrain` |
| Dragonkin | `dragontrain` |
| Artificer | `techtrain` |
| Mechanist | `cybtrain` |
| Cultist | `voidtrain` |
| Chronomancer | `timetrain` |
| Paradox | `paratrain` |
| Shaman | `spirittrain` |
| Spirit Lord | `lordtrain` |
| Siren | `voicetrain` |
| Mindflayer | `mindtrain` |
| Wyrm | `wyrmtrain` |

### Classes with no train command and no disc_default

These classes use the discipline research/train system but have no default discipline configured — the bot must determine the correct discipline from class-specific logic:

Ninja, Monk, Drow, Angel, Samurai, Lich, Shapeshifter, Tanarri, Undead Knight, Spider Droid

---

## Parser Inventory

What the bot can extract from server output:

| Parser | Location | Parses | Key Fields |
|--------|----------|--------|------------|
| QuestParser | `utils/quest_parser.py` | `quest list`, `quest progress`, `quest accept`, `quest complete`, `quest history` | QuestEntry (id, name, status, qp_reward), QuestProgress (objectives with current/threshold), completed IDs |
| StoryParser | `utils/quest_parser.py` | `story`, `storyadmin <name>` | StoryState (node, kills, progress_bits, tasks_bits, not_started, completed) |
| ScoreParser | `utils/parsers.py` | `score` output | HP/mana/move (current/max), avatar status, exp, level |
| CombatParser | `utils/parsers.py` | Combat output | Combat start/end, victory/defeat, instant kill vs multi-round |
| RoomParser | `utils/parsers.py` | `look` output | Room name, exits, mobs present, objects present |
| TrainParser | `utils/parsers.py` | `train` output | Train success/failure, stat changes |
| DisciplineParser | `utils/class_parsers/discipline_parser.py` | Research/train patterns | Research started/complete, may train, discipline point, train success |
| DemonParser | `utils/class_parsers/demon_parser.py` | Demon-specific patterns | Obtain success, graft success, claws/wings unlock |

### Color Code Stripping

All parsers strip Dystopia's custom color codes before matching:
- Escape sequences: `#t`, `#x`, `#R`, `#G`, `#B`, `#Y`, `#C`, `#W`, `#n`, etc.
- UTF-8 symbols: `✓`, `✗`, `▸`, `◆`, `○`, `✦`, `•`
