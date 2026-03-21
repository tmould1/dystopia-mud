# Coverage Matrix & Verification Contracts

Quest-by-quest verification status, phase-level contracts, story contracts, and multiplayer contracts for the bot-driven verification system.

## Coverage Summary

| Phase | Category | Total | Pass | Fail | Blocked | Untested |
|-------|----------|-------|------|------|---------|----------|
| 0 | T (Tutorial) | 8 | 5 | 0 | 0 | 3 |
| 1 | M (Main) | 4 | 1 | 0 | 1 | 2 |
| 1 | T (Tutorial) | 2 | 1 | 0 | 1 | 0 |
| 1 | A (Archaic) | 1 | 0 | 0 | 1 | 0 |
| 2 | M/C/T/E (System) | 13 | 0 | 0 | 3 | 10 |
| 2 | CL (Class) | 28 | 0 | 0 | 0 | 28 |
| 3 | All | 27 | 0 | 0 | 5 | 22 |
| 4 | All | 26 | 0 | 0 | 8 | 18 |
| 5 | All | 32 | 0 | 0 | 10 | 22 |
| **Total** | | **141** | **7** | **0** | **29** | **105** |

*Status as of 2026-03-13. Best single-bot run: explevel=3, demon class.*

---

## Full Quest Matrix

### Phase 0: Tutorial (8 quests)

| Quest ID | Name | Obj Types | Bot Status | Blocker | Notes |
|----------|------|-----------|------------|---------|-------|
| T01 | Finding Your Feet | USE_COMMAND ×3 | **PASS** | — | look, score, inventory |
| T02 | The World Around You | USE_COMMAND ×4 | **PASS** | — | north, south, exits, scan |
| T03 | Speaking Up | USE_COMMAND ×2 | **PASS** | — | say, newbie |
| T04 | Gear Up | USE_COMMAND ×3 | **PASS** | — | equipment, get, wield |
| T05 | First Fight | KILL_MOB ×1 | **PASS** | — | Any mob kill |
| T06 | Dystopia Orientation | USE_COMMAND ×3 | UNTESTED | SP-003 | explevel 1 only (score, scan, map) |
| T07 | Know Your Tools | USE_COMMAND ×2 | UNTESTED | SP-003 | explevel 1 only (help, commands) |
| T08 | Combat Primer | KILL_MOB 3 + USE_COMMAND | UNTESTED | SP-003 | explevel 1 only |

### Phase 1: Foundation (7 quests)

| Quest ID | Name | Obj Types | Bot Status | Blocker | Notes |
|----------|------|-----------|------------|---------|-------|
| M01 | Choose Your Path | REACH_STAT hp 2000, USE_COMMAND train, REACH_STAT class | **PASS** | — | Avatar + selfclass |
| M02 | Trial by Fire | KILL_MOB 25 | BLOCKED | SD-001 | T_STANCE_01 has priority; stance stalls block this |
| M03 | Building Strength | REACH_STAT hp 5000 | UNTESTED | SP-001 | Needs M02 |
| M04 | Earning Your Keep | EARN_QP 400 | UNTESTED | SP-001 | Needs M03; passive accumulation |
| T_PRACTICE_01 | Learning the Basics | USE_COMMAND practice ×3 | **PASS** | — | Practice 3 skills |
| T_STANCE_01 | Combat Stance | USE_COMMAND stance, LEARN_STANCE 10 | BLOCKED | BL-003 | Instant kills prevent stance training |
| A01 | Ancient Rites | COMPLETE_QUEST archaic 1 | BLOCKED | BL-004 | Quest card system not automated |

### Phase 2: Growth — System Tracks (13 quests)

| Quest ID | Name | Obj Types | Bot Status | Blocker | Notes |
|----------|------|-----------|------------|---------|-------|
| M05 | Rising Power | REACH_STAT hp 25K + mana 15K | UNTESTED | SP-001 | Needs M04; extended stat grind |
| C01 | Mob Hunter | KILL_MOB 100 | UNTESTED | SP-001 | Needs M05 |
| C02 | Slayer | KILL_MOB 500 | UNTESTED | SP-001 | Needs C01 |
| C03 | Exterminator | KILL_MOB 2000 | UNTESTED | SP-001 | Needs C02 |
| T_DISC_01 | Discipline Basics | USE_COMMAND research, LEARN_DISCIPLINE 1 | UNTESTED | SP-001 | Handler exists |
| T_DISC_02 | Dedicated Study | LEARN_DISCIPLINE 5 | UNTESTED | SP-001 | Handler exists |
| T_FORGE_01 | The Smith's Way | FORGE_ITEM 1 | BLOCKED | BL-002 | Material drops random |
| T_FORGE_02 | Superior Craft | FORGE_ITEM 3 | BLOCKED | BL-002 | Needs T_FORGE_01 |
| T_QITEM_01 | Custom Gear | QUEST_CREATE 1 | UNTESTED | SP-001 | Handler exists; costs QP |
| T_QITEM_02 | Personal Touch | QUEST_MODIFY 3 | UNTESTED | SP-001 | Handler exists |
| E01 | Explorer | VISIT_AREA 5 | UNTESTED | IF-001 | Story navigation fulfills this |
| E02 | Wanderer | VISIT_AREA 15 | UNTESTED | IF-001 | Story navigation fulfills this |
| A02 | Card Collector | COMPLETE_QUEST archaic 5 | BLOCKED | BL-004 | Quest cards not automated |

### Phase 2: Class Quests (28 quests — 14 classes × 2)

| Quest ID Pattern | Obj Types | Bot Status | Blocker | Notes |
|------------------|-----------|------------|---------|-------|
| CL_VAMP_01/02 | USE_COMMAND + CLASS_POWER protean 3 | UNTESTED | SP-002 | Vampire class required |
| CL_WW_01/02 | USE_COMMAND + CLASS_POWER gifts 3 | UNTESTED | SP-002 | Werewolf class required |
| CL_DEM_01/02 | USE_COMMAND + CLASS_POWER attack 3 | UNTESTED | SP-002 | Demon class — bot default |
| CL_MNK_01/02 | USE_COMMAND + CLASS_POWER martial 3 | UNTESTED | SP-002 | Monk class required |
| CL_MAG_01/02 | USE_COMMAND + SPELL_SKILL 100 | UNTESTED | SP-002 | Mage class required |
| CL_NIN_01/02 | USE_COMMAND + CLASS_POWER shadow 3 | UNTESTED | SP-002 | Ninja class required |
| CL_DRW_01/02 | USE_COMMAND + CLASS_POWER spider 3 | UNTESTED | SP-002 | Drow class required |
| CL_DGS_01/02 | USE_COMMAND + CLASS_TRAIN songtrain 3 | UNTESTED | SP-002 | Dirgesinger class required |
| CL_PSI_01/02 | USE_COMMAND + CLASS_TRAIN psitrain 3 | UNTESTED | SP-002 | Psion class required |
| CL_DKN_01/02 | USE_COMMAND + CLASS_TRAIN dragontrain 3 | UNTESTED | SP-002 | Dragonkin class required |
| CL_ART_01/02 | USE_COMMAND + CLASS_TRAIN techtrain 3 | UNTESTED | SP-002 | Artificer class required |
| CL_CLT_01/02 | USE_COMMAND + CLASS_TRAIN voidtrain 3 | UNTESTED | SP-002 | Cultist class required |
| CL_CHR_01/02 | USE_COMMAND + CLASS_TRAIN timetrain 3 | UNTESTED | SP-002 | Chronomancer class required |
| CL_SHM_01/02 | USE_COMMAND + CLASS_TRAIN spirittrain 3 | UNTESTED | SP-002 | Shaman class required |

### Phase 3: Power (27 quests)

| Quest ID | Name | Obj Types | Bot Status | Blocker | Notes |
|----------|------|-----------|------------|---------|-------|
| M06 | Proven Warrior | REACH_STAT hp 35K, EARN_QP 5K | UNTESTED | SP-001 | Needs M05 + T_DISC_02 + T_FORGE_02 + T_QITEM_02 |
| M07 | Generation Climb | REACH_GEN 5 | UNTESTED | SP-001 | Handler exists |
| C04 | Arena Initiate | ARENA_WIN 1 | BLOCKED | BL-001 | Needs another player |
| C05 | Arena Veteran | ARENA_WIN 10 | BLOCKED | BL-001 | Needs C04 |
| T_STANCE_02 | Stance Discipline | LEARN_STANCE 3×100 | BLOCKED | BL-003 | Needs multi-round combat |
| T_STANCE_03 | Stance Grandmaster | LEARN_STANCE 5×200 | BLOCKED | BL-003 | Needs T_STANCE_02 |
| T_TRAIN_01 | Training Regimen | USE_COMMAND train | UNTESTED | SP-001 | Simple once reached |
| CR01 | Forge Apprentice | FORGE_ITEM metal | UNTESTED | BL-002 | Needs metal slab |
| CR02 | Gemcrafter | FORGE_ITEM gem | UNTESTED | BL-002 | Needs gemstone |
| E03 | Cartographer | VISIT_AREA 30 | UNTESTED | IF-001 | Story covers 16 areas |
| E04 | World Walker | VISIT_AREA 50 | UNTESTED | IF-001 | Need extra areas beyond story |
| D01 | Daily: Hunting Grounds | KILL_MOB 50 (repeatable) | UNTESTED | SP-001 | Simple once reached |
| D02 | Daily: Wanderlust | VISIT_AREA 3 (repeatable) | UNTESTED | SP-001 | Simple once reached |
| A02 | Card Collector | COMPLETE_QUEST archaic 5 | BLOCKED | BL-004 | Quest cards not automated |
| CL_xxx_03 | Class Power (×14) | CLASS_POWER lv7 | UNTESTED | SP-001/002 | 14 class-specific quests |

### Phase 4: Ascension (26 quests)

| Quest ID | Name | Obj Types | Bot Status | Blocker | Notes |
|----------|------|-----------|------------|---------|-------|
| M08 | Path to Power | REACH_STAT hp 40K + mana 30K, EARN_QP 15K | UNTESTED | SP-001 | Needs M07 + T_STANCE_03 |
| M09 | Ancient Blood | REACH_GEN 3 | UNTESTED | SP-001 | Handler exists |
| M10 | True Power | REACH_GEN 1, REACH_PKSCORE 500 | BLOCKED | BL-001 | PK score requires PvP |
| M11 | Elite Transition | REACH_STAT ×3, EARN_QP 40K, REACH_PKSCORE 1K, REACH_UPGRADE 1 | BLOCKED | BL-001 | PK + upgrade gate |
| T_SS_01 | Beyond the Basics | LEARN_SUPERSTANCE 1 | UNTESTED | SP-001 | Handler exists; needs base stances at 200 |
| T_SS_02 | Expanding Mastery | LEARN_SUPERSTANCE 3 | UNTESTED | SP-001 | Needs T_SS_01 |
| T_SS_03 | Supreme Stances | LEARN_SUPERSTANCE 5 | UNTESTED | SP-001 | Needs T_SS_02 |
| C06 | PK Warrior | KILL_PLAYER 25 | BLOCKED | BL-001 | PvP required |
| C07 | Arena Champion | ARENA_WIN 50, REACH_PKSCORE 1K | BLOCKED | BL-001 | PvP + arena |
| CR03 | Master Smith | FORGE_ITEM 10 | BLOCKED | BL-002 | Material acquisition |
| CL_MASTERY | Weapons Master | MASTERY flag | BLOCKED | BL-006 | All skills to 200 |
| E05 | Pathfinder | VISIT_AREA 70 | UNTESTED | IF-001 | Exceeds story area count |
| A03 | Card Master | COMPLETE_QUEST archaic 15 | BLOCKED | BL-004 | Quest cards |
| CL_xxx_04 | Class Peak (×14) | CLASS_POWER lv10 | UNTESTED | SP-001/002 | 14 class-specific quests |

### Phase 5: Transcendence (32 quests)

| Quest ID | Name | Obj Types | Bot Status | Blocker | Notes |
|----------|------|-----------|------------|---------|-------|
| M12 | Upgrade II | REACH_STAT ×3 90K, EARN_QP 80K, REACH_PKSCORE 2K, REACH_UPGRADE 2 | BLOCKED | BL-001, SP-004 | Massive gates |
| M13 | Upgrade III | REACH_STAT ×3 100K, EARN_QP 120K, REACH_PKSCORE 2.5K, REACH_UPGRADE 3 | BLOCKED | BL-001, SP-004 | |
| M14 | Upgrade IV | REACH_STAT ×3 110K, EARN_QP 160K, REACH_PKSCORE 3K, REACH_UPGRADE 4 | BLOCKED | BL-001, SP-004 | |
| M15 | Apex Predator | COMPLETE_QUEST M14 | BLOCKED | SP-001 | Terminal quest |
| C08 | Bounty Hunter | KILL_PLAYER 100 | BLOCKED | BL-001 | PvP required |
| C09 | Warlord | REACH_PKSCORE 3K | BLOCKED | BL-001 | PvP + M12 |
| C10 | Legend | REACH_PKSCORE 5K | BLOCKED | BL-001 | PvP + M13 |
| CR04 | Legendary Craftsman | QUEST_MODIFY 20, FORGE_ITEM 25 | BLOCKED | BL-002 | Materials |
| E06 | Atlas | VISIT_AREA all | BLOCKED | IF-001 | Every area in game |
| D03 | Daily: Proving Grounds | KILL_PLAYER 3 (repeatable) | BLOCKED | BL-001 | PvP |
| A04 | Archaic Legend | COMPLETE_QUEST archaic 50 | BLOCKED | BL-004 | Quest cards |
| CL_xxx_U01/U02 | Upgraded Classes (×28) | USE_COMMAND + CLASS_POWER lv5 | UNTESTED | SP-001/002 | 14 upgraded classes × 2 quests |

---

## Per-Phase Verification Contracts

### Phase 0: Tutorial

**Entry conditions**: New character, no progression requirements.
**Bot configuration**: `--explevel 1` (for T01-T05) or `--explevel 2` (for T06-T08).
**Expected behavior**: Bot sends each command in objective list; auto-complete quests fire immediately.
**Success criteria**: All 5 (or 3) tutorial quest IDs appear in `quest history`.
**Known blockers**: None for explevel 1 path. Explevel 2 path (T06-T08) untested but should work identically.
**Cycle budget**: ~50 cycles.

### Phase 1: Foundation

**Entry conditions**: T05 or T08 completed, character at level 1.
**Bot configuration**: `--explevel 1 --mode all` or `--explevel 3 --mode all`.
**Expected behavior**:
1. M01: Kill arena mobs → train HP to 2000 → `train avatar` → `selfclass <class>`
2. M02: Kill 25 mobs (arena + story areas)
3. T_PRACTICE_01: Use `practice` 3 times
4. T_STANCE_01: `stance bull` + fight multi-round combat for skill 10
5. M03: Train HP to 5000
6. M04: Passive (QP from rewards accumulates to 400)
7. A01: Complete 1 archaic quest card (not automated)

**Success criteria**: M01, M02, M03, M04, T_PRACTICE_01, T_STANCE_01 in `quest history`.
**Known blockers**: BL-003 (T_STANCE_01 blocks due to instant kills), BL-004 (A01 quest cards).
**Cycle budget**: ~500 cycles.

### Phase 2: Growth

**Entry conditions**: M04 completed, HP ≥ 5000, QP ≥ 400.
**Bot configuration**: `--mode all --enable-story --selfclass <class>`.
**Expected behavior**:
1. M05: Extended stat training (HP 25K, mana 15K)
2. C01-C03: Kill mobs in story areas (100 → 500 → 2000 cumulative)
3. T_DISC_01/02: Research + train discipline to level 5
4. T_FORGE_01/02: Forge items (needs materials in inventory)
5. T_QITEM_01/02: Create + modify quest items (costs QP)
6. E01/E02: Visit areas (story navigation covers 16)
7. CL_xxx_01/02: Class-specific commands + power training

**Success criteria**: M05, C01-C03, T_DISC_01/02, T_QITEM_01/02, E01/E02, CL_xxx_01/02 in `quest history`.
**Known blockers**: BL-002 (forge materials), SP-004 (stat gates for M05).
**Cycle budget**: ~2000 cycles.

### Phase 3-5: Power through Transcendence

**Entry conditions**: Phase 2 complete, class selected, QP > 5000.
**Bot configuration**: `--mode all --enable-story --enable-pvp` (for PvP quests).
**Expected behavior**: Generation training, stance mastery, superstance unlocking, class power progression to level 7-10, upgrade system, PvP combat.
**Success criteria**: Progressive quest completion through M07 → M11 → M15.
**Known blockers**: BL-001 (PvP gate at M10), BL-003 (stance training speed), BL-006 (mastery grind), SP-004 (stat gates escalate rapidly).
**Cycle budget**: ~5000+ cycles per phase.

---

## Story Verification Contracts

| Node | Entry Condition | Bot Actions | Success Criterion | Blocker |
|------|-----------------|-------------|-------------------|---------|
| 1 | Avatar status | `goto 3011`, `say darkness` | `storyadmin` shows node=2 | IF-001 |
| 2 | Node 1 complete | `goto 3600`, `say darkness`, kill 3, fetch bone, examine tomb, `say done` | node=3 | IF-001 |
| 3 | Node 2 complete | `goto 3405`, `say marks`, kill 1, fetch scroll, examine etchings, `say done` | node=4 | IF-001 |
| 4 | Node 3 complete | `goto 8900`, `say sundering`, kill 2, examine roots, `say done` | node=5 | IF-001 |
| 5 | Node 4 complete | `goto 9208`, `say sundering`, kill 3, examine throne, `say done` | node=6 | IF-001 |
| 6 | Node 5 complete | `goto 4100`, `say carvings`, kill 5, fetch slime, examine carvings, `say done` | node=7 | IF-001 |
| 7 | Node 6 complete | `goto 1301`, `say sorcerer`, kill 3, fetch journal, examine mirror, `say done` | node=8 | IF-001 |
| 8 | Node 7 complete | `goto 5315`, `say thalos`, kill 2, examine statue + foundation, `say done` | node=9 | IF-001 |
| 9 | Node 8 complete | `goto 5104`, `say displacement`, examine altar, `say done` | node=10 | IF-001 |
| 10 | Node 9 complete | `goto 2616`, `say cycle`, fetch tablet, examine hieroglyphics, `say done` | node=11 | IF-001 |
| 11 | Node 10 complete | `goto 8600`, `say between`, examine church, `say done` | node=12 | IF-001 |
| 12 | Node 11 complete | `goto 8103`, `say convergence`, kill 1, examine murals, `say done` | node=13 | IF-001 |
| 13 | Node 12 complete | `goto 8010`, `say future`, kill 1, examine rubble, `say done` | node=14 | IF-001 |
| 14 | Node 13 complete | `goto 93006`, `say origin`, examine sensors, `say done` | node=15 | IF-001 |
| 15 | Node 14 complete | `goto 30508`, `say old city`, kill 1, examine records, talk King, `say done` | node=16 | IF-001 |
| 16 | Node 15 complete | `goto 99004`, `say what now` | node=17 (completed) | IF-001 |

**All nodes blocked by IF-001**: StoryNavigator requires immortal `goto` command. Test character must be level 7+.

---

## Multiplayer Verification Contracts

### Scenario 1: `follow_group_tutorial`

| Step | Bot | Command | Expected |
|------|-----|---------|----------|
| 1 | All | `quest list` | Quest list output |
| 2 | Followers | `follow <leader>` | "You now follow" message |
| 3 | Leader | `group <follower>` | "is now a member" message |
| 4 | All | `group` | Shows party members |
| 5 | All | `gtell testing` | Group channel message |
| 6 | All | `look`, `score` | Room + score output |
| 7 | All | `quest progress` | Quest progress output |

**Bot count**: 2+. **Status**: UNTESTED.

### Scenario 2: `pk_duel_smoke`

| Step | Bot | Command | Expected |
|------|-----|---------|----------|
| 1 | All | `recall` | Return to temple |
| 2 | Defender | breaks follow | Independent |
| 3 | Attacker | `kill <defender>` | Combat initiates |
| 4 | All | `score` | Score with combat stats |

**Bot count**: 2. **Status**: UNTESTED. **Note**: Requires PvP-enabled test world.

### Scenario 3: `story_party_smoke`

| Step | Bot | Command | Expected |
|------|-----|---------|----------|
| 1 | All | `story` | Story state output |
| 2 | Followers | `follow <leader>` | Follow message |
| 3 | Leader | `group <follower>` | Group message |
| 4 | All | `story` | Story state (verify party doesn't affect) |
| 5 | All | `quest progress` | Quest state output |

**Bot count**: 2+. **Status**: UNTESTED.
