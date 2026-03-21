# Roadmap to 100% Coverage

Prioritized work items to close the verification gap from 5% to 100% quest coverage, 0% to 100% story coverage, and activate multiplayer scenarios.

## Current State

- **Quests**: 7/141 (5.0%) — T01-T05, M01, T_PRACTICE_01
- **Story nodes**: 0/16 (0%)
- **Multiplayer scenarios**: 0/3 (0%)
- **Best campaign run**: Single-bot, explevel=3, demon class (2026-03-13)
- **Campaign runner**: Has crashed at START state in recent runs

---

## Priority 1: Fix Campaign Runner

**Impact**: Unblocks all automated testing. **Effort**: Low.

The campaign runner (`campaign_runner.py`) generates the class × explevel profile matrix and runs QuestBot for each, but recent runs show bots ending in START state with 0 quests completed — likely a connection/login failure.

**Work items**:
- [ ] Diagnose why campaign bots fail at START (check: server running, port correct, character exists or creation works)
- [ ] Add connection retry logic and clearer error messages to campaign runner
- [ ] Reproduce the 7-quest baseline (explevel=3 single-bot) through the campaign runner
- [ ] Ensure campaign runner passes bot output to report even on partial completion

**Target**: Campaign runner reliably completes single-profile runs. 7/141 quests (5%).

---

## Priority 2: Phase 1 Completion

**Impact**: Unblocks Phase 2 progression. **Effort**: Medium.

T_STANCE_01 is the critical blocker — stance skill doesn't increase from instant kills (arena mobs die in 1 hit). This blocks M02 because T_STANCE_01 has quest priority.

**Work items**:
- [ ] Fix stance training: route bot to story areas with tougher mobs (multi-round combat trains stance skill)
- [ ] Or: add a `goto` to a mid-level area where fights last 3+ rounds at avatar power level
- [ ] Verify M02 (KILL_MOB 25) completes after T_STANCE_01 unblocked
- [ ] Verify M03 (REACH_STAT hp 5000) — bot already handles stat training
- [ ] Verify M04 (EARN_QP 400) — passive; verify accumulated QP from T01-M03 rewards exceeds 400

**Target**: T01-T08, M01-M04, T_PRACTICE_01, T_STANCE_01 all pass. ~15/141 quests (11%).

---

## Priority 3: Phase 2 System Tutorials

**Impact**: Covers core game systems (discipline, forge, quest items, exploration). **Effort**: Medium.

These quests teach the game's intermediate systems. Bot handlers exist for most objectives.

**Work items**:
- [ ] Verify T_DISC_01/02 (LEARN_DISCIPLINE) — handler exists, needs: research → kill mobs for XP → detect "finished researching" → train
- [ ] Fix T_FORGE_01/02 (FORGE_ITEM) — handler exists but needs materials. Options:
  - Add wizard command `grant <material>` for test characters
  - Or: have bot farm mobs that drop forge materials
  - Or: pre-seed test character inventory with materials via `qadmin` or `oload`
- [ ] Verify T_QITEM_01/02 (QUEST_CREATE, QUEST_MODIFY) — handler exists, costs QP
- [ ] Verify C01 (KILL_MOB 100) — bot kills mobs naturally during story; just needs enough cycles
- [ ] Verify E01/E02 (VISIT_AREA 5/15) — story navigation visits 16 areas if working

**Target**: M05, C01-C03, T_DISC_01/02, T_QITEM_01/02, E01/E02 pass. ~30/141 quests (21%).

---

## Priority 4: Class-Specific Phase 2 Quests

**Impact**: Covers 14 base class intro quests (28 quests). **Effort**: Medium-High.

Each class has CL_xxx_01 (intro commands) and CL_xxx_02 (CLASS_POWER or CLASS_TRAIN to level 3). The campaign runner should run one profile per class.

**Work items**:
- [ ] Run campaign with `--classes all` to generate 14 class profiles
- [ ] Verify each class's intro commands work (USE_COMMAND objectives vary by class)
- [ ] Verify CLASS_POWER objectives — discipline handler routes correctly for each class
- [ ] Verify CLASS_TRAIN objectives — `songtrain`, `psitrain`, etc. for train-cmd classes
- [ ] Fix any class-specific parsing issues (each class has unique output patterns)

**Target**: All 28 CL_xxx_01/02 quests pass. ~58/141 quests (41%).

---

## Priority 5: Story System Activation

**Impact**: 16/16 story nodes + enables VISIT_AREA quests. **Effort**: Medium.

StoryNavigator code is comprehensive but requires an immortal character (`goto` command). All 16 nodes are defined in `story_data.py`.

**Work items**:
- [ ] Ensure test character has immortal level (7+) for `goto` access
- [ ] Run single-bot with `--enable-story` and verify node 1 advancement
- [ ] Verify all 16 nodes complete in sequence (check `storyadmin` output after each)
- [ ] Verify VISIT_AREA objectives (E01, E02) fire from story area visits
- [ ] Alternatively: implement mortal pathfinding (walk commands instead of `goto`) for non-immortal testing

**Target**: 16/16 story nodes. E01/E02 pass as side effect.

---

## Priority 6: Phase 3 Progression

**Impact**: Unlocks mid-game content (generation, stances, arena). **Effort**: High.

Phase 3 introduces generation training, advanced stance mastery, and arena combat. Several objectives require extended grinding.

**Work items**:
- [ ] Verify M06 (REACH_STAT hp 35K, EARN_QP 5K) — needs Phase 2 prereqs (M05, T_DISC_02, T_FORGE_02, T_QITEM_02)
- [ ] Verify M07 (REACH_GEN 5) — `train generation` handler exists
- [ ] Fix T_STANCE_02/03 (LEARN_STANCE 3×100, 5×200) — needs sustained multi-round combat in tough areas
- [ ] Address CR01/CR02 (forge with specific material types: metal, gem)
- [ ] Verify CL_xxx_03 (CLASS_POWER level 7) for each class
- [ ] Verify D01/D02 (daily repeatables) work

**Target**: ~70/141 quests (50%).

---

## Priority 7: PvP and Arena Quests

**Impact**: Unblocks KILL_PLAYER, ARENA_WIN, REACH_PKSCORE objectives. **Effort**: High.

PvP quests (C04-C07, C08-C10, D03) require multiple interacting players. The multiplayer manager exists but hasn't been used for PvP verification.

**Work items**:
- [ ] Activate multiplayer `pk_duel_smoke` scenario — verify it completes
- [ ] Implement PvP bot pair: one bot kills the other, both track quest progress
- [ ] Verify ARENA_WIN objectives — bot enters `arena`, fights scheduled opponent
- [ ] Verify REACH_PKSCORE objectives — accumulate from PvP wins
- [ ] Consider: add NPC sparring partner for single-bot arena testing (server-side)

**Target**: C04-C10, D03 pass. +10 quests.

---

## Priority 8: Phase 4-5 Deep Progression

**Impact**: Covers superstances, upgrades, endgame. **Effort**: Very High.

Phase 4-5 have massive stat gates (hp 50K-110K, QP 40K-160K, PK score 500-5000) and require the upgrade system.

**Work items**:
- [ ] Verify T_SS_01-03 (LEARN_SUPERSTANCE) — handler cycles through 5 stances; needs all base stances at 200 first
- [ ] Verify M10/M11 (REACH_PKSCORE + REACH_UPGRADE) — PvP gate + upgrade command
- [ ] Verify M12-M14 (successive upgrade tiers with escalating stat gates)
- [ ] Verify CL_xxx_04 (CLASS_POWER level 10) for each class
- [ ] Verify CL_xxx_U01/U02 (upgraded classes) — 14 pairs after M11
- [ ] Verify CL_MASTERY (all weapons/spells/stances at 200)
- [ ] E05/E06 (VISIT_AREA 70/all) — need areas beyond the 16 story nodes

**Target**: ~120/141 quests (85%).

---

## Priority 9: 100% Coverage — Remaining Gaps

**Impact**: Full verification gate. **Effort**: Very High.

The remaining quests are blocked by systems the bot currently cannot automate:

**Work items**:
- [ ] A01-A04 (archaic quest cards) — implement quest card acceptance and completion bot logic
- [ ] COLLECT_ITEM objectives — implement item collection handler
- [ ] M15 (Apex Predator) — terminal quest, needs M14 complete
- [ ] Any remaining class-specific edge cases
- [ ] Run full campaign matrix: 14 base classes × 3 explevels × story × multiplayer

**Target**: 141/141 quests (100%), 16/16 story nodes, 3/3 multiplayer scenarios.

---

## Server-Side Changes That Would Accelerate Coverage

These are optional server enhancements that would make bot verification significantly easier:

| Change | Impact | Quests Unblocked |
|--------|--------|------------------|
| Wizard command: `grant <player> <material>` | Eliminates forge material RNG | T_FORGE_01/02, CR01-CR04 (6 quests) |
| Wizard command: `setpk <player> <score>` | Skip PvP grind for testing | M10, M11-M14, C06-C10 (10+ quests) |
| NPC arena opponent (sparring dummy) | Single-bot arena testing | C04, C05, C07 (3 quests) |
| Mortal pathfinding in bot | Remove immortal `goto` dependency | All 16 story nodes, E01-E06 |
| `qadmin <player> grantqp <amount>` | Skip QP accumulation grind | M04, M06, M08, M11-M14 (7 quests) |
| Archaic quest card bot automation | Complete quest card objectives | A01-A04 (4 quests) |

---

## Progress Tracking

Update this table after each campaign run:

| Date | Campaign Config | Quests | Story | Multi | Notes |
|------|-----------------|--------|-------|-------|-------|
| 2026-03-13 | explevel=3, demon, single-bot | 7/141 | 0/16 | 0/3 | Best run; campaign runner crashed in later runs |
| | | | | | |
