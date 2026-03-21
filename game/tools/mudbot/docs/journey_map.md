# Player Progression Journey Map

Complete quest dependency graph, story node integration, and class branching for the Dystopia MUD verification system.

## Phase Overview

| Phase | Name | Quests | Gate Quest | Key Objective Types | Story Nodes |
|-------|------|--------|------------|---------------------|-------------|
| 0 | Tutorial | T01-T08 (8) | explevel gating | USE_COMMAND, KILL_MOB | — |
| 1 | Foundation | M01-M04, T_PRACTICE_01, T_STANCE_01, A01 (7) | T05 or T08 → M01 | REACH_STAT, EARN_QP, LEARN_STANCE | 1-2 |
| 2 | Growth | M05, C01-C03, T_DISC/FORGE/QITEM, E01-E02, CL_xxx_01/02 (41) | M04 → M05 | LEARN_DISCIPLINE, FORGE_ITEM, QUEST_CREATE, CLASS_POWER, VISIT_AREA | 3-8 |
| 3 | Power | M06-M07, C04-C05, T_STANCE_02/03, T_TRAIN_01, CR01-CR02, E03-E04, D01-D02, A02, CL_xxx_03 (27) | M05 + system tutorials → M06 | ARENA_WIN, REACH_GEN, LEARN_STANCE (mastery) | 9-12 |
| 4 | Ascension | M08-M11, T_SS_01-03, C06-C07, CR03, CL_MASTERY, E05, A03, CL_xxx_04 (26) | M07 + T_STANCE_03 → M08 | LEARN_SUPERSTANCE, REACH_UPGRADE, KILL_PLAYER, REACH_PKSCORE | 13-15 |
| 5 | Transcendence | M12-M15, C08-C10, CR04, E06, D03, A04, CL_xxx_U01/U02 (32) | M11 → M12 | REACH_UPGRADE (2-4), massive stat/QP gates | 16 |

**Total: 141 quest definitions**

---

## Quest Dependency Graph: Phases 0-2

```mermaid
flowchart TD
    classDef tutorial fill:#4a9eff,color:white
    classDef main fill:#2ecc71,color:white
    classDef combat fill:#e74c3c,color:white
    classDef classq fill:#f39c12,color:white
    classDef explore fill:#9b59b6,color:white
    classDef craft fill:#795548,color:white
    classDef archaic fill:#607d8b,color:white

    %% Phase 0: Tutorial - FTUE Level 0
    T01[T01: Finding Your Feet]:::tutorial
    T02[T02: The World Around You]:::tutorial
    T03[T03: Speaking Up]:::tutorial
    T04[T04: Gear Up]:::tutorial
    T05[T05: First Fight]:::tutorial
    T01 --> T02 --> T03 --> T04 --> T05

    %% Phase 0: Tutorial - FTUE Level 1
    T06[T06: Dystopia Orientation]:::tutorial
    T07[T07: Know Your Tools]:::tutorial
    T08[T08: Combat Primer]:::tutorial
    T06 --> T07 --> T08

    %% Phase 1: Foundation
    M01[M01: Choose Your Path<br/>REACH_STAT hp 2000<br/>train avatar<br/>selfclass]:::main
    T05 --> M01
    T08 --> M01

    M02[M02: Trial by Fire<br/>KILL_MOB 25]:::main
    TP01[T_PRACTICE_01: Learning the Basics<br/>USE_COMMAND practice 3]:::tutorial
    TS01[T_STANCE_01: Combat Stance<br/>LEARN_STANCE 10]:::tutorial
    M01 --> M02
    M01 --> TP01
    M01 --> TS01

    M03[M03: Building Strength<br/>REACH_STAT hp 5000]:::main
    M02 --> M03

    A01[A01: Ancient Rites<br/>COMPLETE_QUEST archaic 1]:::archaic
    M03 --> A01

    M04[M04: Earning Your Keep<br/>EARN_QP 400]:::main
    M03 --> M04

    %% Phase 2: Growth - Main
    M05[M05: Rising Power<br/>REACH_STAT hp 25000<br/>REACH_STAT mana 15000]:::main
    M04 --> M05

    %% Phase 2: Combat Track
    C01[C01: Mob Hunter<br/>KILL_MOB 100]:::combat
    C02[C02: Slayer<br/>KILL_MOB 500]:::combat
    C03[C03: Exterminator<br/>KILL_MOB 2000]:::combat
    M05 --> C01 --> C02 --> C03

    %% Phase 2: Discipline Track
    TD01[T_DISC_01: Discipline Basics<br/>LEARN_DISCIPLINE 1]:::tutorial
    TD02[T_DISC_02: Dedicated Study<br/>LEARN_DISCIPLINE 5]:::tutorial
    M05 --> TD01 --> TD02

    %% Phase 2: Forge Track
    TF01[T_FORGE_01: Smith's Way<br/>FORGE_ITEM 1]:::tutorial
    TF02[T_FORGE_02: Superior Craft<br/>FORGE_ITEM 3]:::tutorial
    M05 --> TF01 --> TF02

    %% Phase 2: Quest Item Track
    TQ01[T_QITEM_01: Custom Gear<br/>QUEST_CREATE 1]:::tutorial
    TQ02[T_QITEM_02: Personal Touch<br/>QUEST_MODIFY 3]:::tutorial
    M05 --> TQ01 --> TQ02

    %% Phase 2: Exploration Track
    E01[E01: Explorer<br/>VISIT_AREA 5]:::explore
    E02[E02: Wanderer<br/>VISIT_AREA 15]:::explore
    M05 --> E01 --> E02

    %% Phase 2: Class Quests (14 classes, shown as group)
    CL01[CL_xxx_01: Class Intro<br/>14 classes × USE_COMMAND + CLASS_POWER]:::classq
    CL02[CL_xxx_02: Class Mastery<br/>14 classes × CLASS_POWER lv3]:::classq
    M05 --> CL01 --> CL02
```

## Quest Dependency Graph: Phases 3-5

```mermaid
flowchart TD
    classDef main fill:#2ecc71,color:white
    classDef combat fill:#e74c3c,color:white
    classDef classq fill:#f39c12,color:white
    classDef explore fill:#9b59b6,color:white
    classDef craft fill:#795548,color:white
    classDef tutorial fill:#4a9eff,color:white
    classDef daily fill:#00bcd4,color:white
    classDef archaic fill:#607d8b,color:white

    %% Phase 3 entry
    M05[M05]:::main
    TD02[T_DISC_02]:::tutorial
    TF02[T_FORGE_02]:::tutorial
    TQ02[T_QITEM_02]:::tutorial

    M06[M06: Proven Warrior<br/>REACH_STAT hp 35000<br/>EARN_QP 5000]:::main
    M05 --> M06
    TD02 --> M06
    TF02 --> M06
    TQ02 --> M06

    M07[M07: Generation Climb<br/>REACH_GEN 5]:::main
    M06 --> M07

    %% Phase 3: Arena
    C04[C04: Arena Initiate<br/>ARENA_WIN 1]:::combat
    C05[C05: Arena Veteran<br/>ARENA_WIN 10]:::combat
    M07 --> C04 --> C05

    %% Phase 3: Stance
    TS02[T_STANCE_02: Stance Discipline<br/>LEARN_STANCE 3×100]:::tutorial
    TS03[T_STANCE_03: Stance Grandmaster<br/>LEARN_STANCE 5×200]:::tutorial
    M07 --> TS02 --> TS03

    %% Phase 3: Other tracks
    TT01[T_TRAIN_01: Training Regimen]:::tutorial
    M07 --> TT01

    CR01[CR01: Forge Apprentice<br/>FORGE_ITEM metal]:::craft
    CR02[CR02: Gemcrafter<br/>FORGE_ITEM gem]:::craft
    M06 --> CR01 --> CR02

    E03[E03: Cartographer<br/>VISIT_AREA 30]:::explore
    E04[E04: World Walker<br/>VISIT_AREA 50]:::explore
    M06 --> E03 --> E04

    D01[D01: Daily Hunt<br/>KILL_MOB 50 ↻]:::daily
    D02[D02: Daily Wander<br/>VISIT_AREA 3 ↻]:::daily
    M06 --> D01
    M06 --> D02

    A01[A01]:::archaic
    A02[A02: Card Collector<br/>COMPLETE_QUEST 5]:::archaic
    A01 --> A02

    CL02[CL_xxx_02]:::classq
    CL03[CL_xxx_03: Class Power<br/>14 classes × CLASS_POWER lv7]:::classq
    M06 --> CL03
    CL02 --> CL03

    %% Phase 4: Ascension
    M08[M08: Path to Power<br/>REACH_STAT hp 40000<br/>EARN_QP 15000]:::main
    M07 --> M08
    TS03 --> M08

    M09[M09: Ancient Blood<br/>REACH_GEN 3]:::main
    M08 --> M09

    M10[M10: True Power<br/>REACH_GEN 1<br/>REACH_PKSCORE 500]:::main
    M09 --> M10

    TSS01[T_SS_01: Beyond the Basics<br/>LEARN_SUPERSTANCE 1]:::tutorial
    TSS02[T_SS_02: Expanding Mastery<br/>LEARN_SUPERSTANCE 3]:::tutorial
    TSS03[T_SS_03: Supreme Stances<br/>LEARN_SUPERSTANCE 5]:::tutorial
    M08 --> TSS01 --> TSS02 --> TSS03

    M11[M11: Elite Transition<br/>REACH_STAT hp/mana/move 50K/35K/35K<br/>EARN_QP 40000<br/>REACH_PKSCORE 1000<br/>REACH_UPGRADE 1]:::main
    M10 --> M11
    TSS03 --> M11

    C06[C06: PK Warrior<br/>KILL_PLAYER 25]:::combat
    C07[C07: Arena Champion<br/>ARENA_WIN 50<br/>REACH_PKSCORE 1000]:::combat
    M08 --> C06
    C05 --> C07
    C06 --> C07

    CR03[CR03: Master Smith<br/>FORGE_ITEM 10]:::craft
    CR02 --> CR03
    M08 --> CR03

    CLM[CL_MASTERY: Weapons Master<br/>MASTERY flag]:::classq
    M08 --> CLM

    E05[E05: Pathfinder<br/>VISIT_AREA 70]:::explore
    E04 --> E05

    A03[A03: Card Master<br/>COMPLETE_QUEST 15]:::archaic
    A02 --> A03

    CL04[CL_xxx_04: Class Peak<br/>14 classes × CLASS_POWER lv10]:::classq
    CL03 --> CL04
    M08 --> CL04

    %% Phase 5: Transcendence
    M12[M12: Upgrade II<br/>hp/mana/move 90K<br/>QP 80K, PK 2000]:::main
    M11 --> M12

    M13[M13: Upgrade III<br/>hp/mana/move 100K<br/>QP 120K, PK 2500]:::main
    M12 --> M13

    M14[M14: Upgrade IV<br/>hp/mana/move 110K<br/>QP 160K, PK 3000]:::main
    M13 --> M14

    M15[M15: Apex Predator<br/>COMPLETE_QUEST M14]:::main
    M14 --> M15

    C08[C08: Bounty Hunter<br/>KILL_PLAYER 100]:::combat
    C09[C09: Warlord<br/>REACH_PKSCORE 3000]:::combat
    C10[C10: Legend<br/>REACH_PKSCORE 5000]:::combat
    M11 --> C08
    C08 --> C09
    M12 --> C09
    C09 --> C10
    M13 --> C10

    CR04[CR04: Legendary Craftsman<br/>QUEST_MODIFY 20, FORGE 25]:::craft
    CR03 --> CR04
    M11 --> CR04

    E06[E06: Atlas<br/>VISIT_AREA all]:::explore
    E05 --> E06
    M11 --> E06

    D03[D03: Daily PvP<br/>KILL_PLAYER 3 ↻]:::daily
    M11 --> D03

    A04[A04: Archaic Legend<br/>COMPLETE_QUEST 50]:::archaic
    A03 --> A04
    M11 --> A04

    CLU[CL_xxx_U01/U02: Upgraded Classes<br/>14 classes × USE_COMMAND + CLASS_POWER lv5]:::classq
    M11 --> CLU
```

---

## FTUE Branching

Three experience levels converge at M01 via OR-logic prerequisites:

```
explevel 0 (Never played a MUD):  T01 → T02 → T03 → T04 → T05 ─┐
                                                                   ├──→ M01
explevel 1 (MUD veteran, new to Dystopia):  T06 → T07 → T08 ─────┘
                                                                   │
explevel 2+ (Dystopia veteran):  ──────────────────────────────────┘
                                 (M01 auto-available, T quests FTUE-skipped)
```

- `min_explevel` / `max_explevel` on each tutorial quest controls visibility
- FTUE-skipped quests (QFLAG_FTUE_SKIP) auto-satisfy as prerequisites for downstream quests
- Bot `--explevel` flag maps: CLI 1 → server explevel 0, CLI 2 → server 1, CLI 3 → server 2

---

## Class Branching

### 14 Base Classes (Phase 2-4)

Each base class has a 4-quest chain gated by `required_class`:

| Prefix | Class | Phase 2 (_01, _02) | Phase 3 (_03) | Phase 4 (_04) | Progression Type |
|--------|-------|-------------------|---------------|---------------|------------------|
| CL_VAMP | Vampire | Claws/fangs → Protean 3 | Protean 7 + Obtenebration 5 | Protean 10 + Presence 7 | Discipline (disc: protean) |
| CL_WW | Werewolf | Rage/transform → Gifts 3 | Gifts 7 | Gifts 10 + Luna 5 | Discipline (disc: bear) |
| CL_DEM | Demon | Horns/wings → Attack 3 | Attack 7 + Hellfire 5 | Attack 10 + Temptation 7 | Discipline (disc: attack) |
| CL_MNK | Monk | Chi/meditate → Martial 3 | Martial 7 + Weapon 150 | Martial 10 + all wpn 200 | Discipline |
| CL_MAG | Mage | Cast/spell 50 → Spell 100 | All spells 150 + Arcane 7 | All spells 200 + Arcane 10 | Discipline (disc: arcane) |
| CL_NIN | Ninja | Vanish/backstab → Shadow 3 | Shadow 7 | Shadow 10 | Discipline |
| CL_DRW | Drow | Web/shadowplane → Spider 3 | Spider 7 | Spider 10 | Discipline |
| CL_DGS | Dirgesinger | Sing → Song 3 | Song 7 | Song 10 | Train cmd: songtrain |
| CL_PSI | Psion | Focus → Psionic 3 | Psionic 7 | Psionic 10 | Train cmd: psitrain |
| CL_DKN | Dragonkin | Breathe → Dragon 3 | Dragon 7 | Dragon 10 | Train cmd: dragontrain |
| CL_ART | Artificer | Turret/blaster → Tech 3 | Tech 7 | Tech 10 | Train cmd: techtrain |
| CL_CLT | Cultist | Ritual → Void 3 | Void 7 | Void 10 | Train cmd: voidtrain |
| CL_CHR | Chronomancer | Flux/quicken → Time 3 | Time 7 | Time 10 | Train cmd: timetrain |
| CL_SHM | Shaman | Commune → Spirit 3 | Spirit 7 | Spirit 10 | Train cmd: spirittrain |

### 14 Upgraded Classes (Phase 5)

Each upgraded class has a 2-quest chain, unlocked after M11:

| Prefix | Class | Upgrades From | _U01 | _U02 | Train cmd |
|--------|-------|---------------|------|------|-----------|
| CL_TAN | Tanarri | Demon | demonform | Tanarri power 5 | — |
| CL_DRD | Spider Droid | Ninja? | power | Droid power 5 | — |
| CL_SAM | Samurai | Monk? | koryou | Weapon 500 + Bushido 5 | — |
| CL_UDK | Undead Knight | — | darkheart | Death power 5 | — |
| CL_ANG | Angel | — | spiritform/halo | Angelic power 5 | — |
| CL_SHP | Shapeshifter | — | morph | Morphosis 5 | — |
| CL_LCH | Lich | Mage? | lichform | Necromancy 5 | — |
| CL_SRN | Siren | Dirgesinger | voicetrain | Voice 5 | voicetrain |
| CL_MFL | Mindflayer | Psion | mindtrain | Mind 5 | mindtrain |
| CL_WYR | Wyrm | Dragonkin | wyrmtrain | Wyrm 5 | wyrmtrain |
| CL_MCH | Mechanist | Artificer | cybtrain | Cyber 5 | cybtrain |
| CL_VBN | Voidborn | Cultist | voidform | Voidborn 5 | — |
| CL_PAR | Paradox | Chronomancer | paratrain | Paradox 5 | paratrain |
| CL_SPL | Spirit Lord | Shaman | lordtrain | Lordship 5 | lordtrain |

---

## Story Node Map: "Echoes of the Sundering"

16-node narrative progression tracked via `story_node`, `story_kills`, and `story_progress` bitfield.

| Node | Area | Hub NPC (vnum) | Hub Type | Kill | Fetch | Examine | Talk NPC | Intro Keyword |
|------|------|---------------|----------|------|-------|---------|----------|---------------|
| 1 | Midgaard | Executioner (3011) | talk | — | — | — | — | "darkness" |
| 2 | Graveyard | Henry (3600) | full | 3 undead | bone (3699 @ 3607) | tomb (3650) | — | "darkness" |
| 3 | Chapel | Priest (3405) | full | 1 wraith | scroll (3498 @ 3457) | etchings (3465) | — | "marks" |
| 4 | Grove | Hierophant (8900) | light | 2 corrupted | — | roots (8905) | — | "sundering" |
| 5 | Canyon | Earth Ruler (9208) | light | 3 elementals | — | throne (9227) | — | "sundering" |
| 6 | Moria | The Mage (4100) | full | 5 tunnel mobs | slime mold (4103 @ 4118) | carvings (4160) | — | "carvings" |
| 7 | Hitower | Adventurer (1301) | full | 3 golems | journal (1499 @ 1367) | mirror (1336) | — | "sorcerer" |
| 8 | Thalos | Librarian (5315) | full | 2 beasts | — | statue (5386), foundation (5371) | — | "thalos" |
| 9 | Drow City | Priestess (5104) | light | — | — | altar (5145) | — | "displacement" |
| 10 | Pyramid | Sphinx (2616) | full | — | tablet (2699 @ 2630) | hieroglyphics (2629) | — | "cycle" |
| 11 | Dreamscape | Keeper (8600) | light | — | — | church (8637) | — | "between" |
| 12 | Atlantis | Neptune (8103) | light | 1 guard | — | murals (8131) | — | "convergence" |
| 13 | Mega-City | Judge (8010) | light | 1 punk | — | rubble (8014) | — | "future" |
| 14 | Domeship | Elfangor (93006) | light | — | — | sensors (93045) | — | "origin" |
| 15 | Dystopia | Queen (30508) | full | 1 guard | — | records (30460) | King (30509): "unraveling" | "old city" |
| 16 | Heaven | Overseer (99004) | talk | — | — | — | — | "what now" |

**Node 12 alternate**: Olympus path — Zeus (901), kill chimera (920), examine scorch (910). Bot defaults to Atlantis.

### Task Bits

Each node's progress is tracked by a bitfield in `story_progress`:
- `0x01` — Kill task (or first examine-only task)
- `0x02` — Fetch task (or second examine task)
- `0x04` — Examine/talk NPC task (third task on full hubs)

Return keyword "done" advances to next node after all required bits are set.

---

## Story ↔ Quest Synergy

Story progression naturally fulfills or advances these quest objective types:

| Quest Objective | How Story Helps | Example Quests |
|-----------------|-----------------|----------------|
| VISIT_AREA | 16 unique areas traversed | E01 (5 areas), E02 (15), E03 (30) |
| KILL_MOB | Kill tasks in nodes 2-8, 12-13, 15 | M02 (25 kills), C01 (100), C02 (500) |
| WEAPON_SKILL | Multi-round combat in tough story areas trains skills | CL_MNK_03, CL_MAG_03 |
| SPELL_SKILL | Same: multi-round combat trains spell proficiency | CL_MAG_02 (spell 100), CL_MAG_03 (spell 150) |
| LEARN_STANCE | Multi-round fights train stance skill | T_STANCE_01 (skill 10), T_STANCE_02 (3×100) |
| LEARN_DISCIPLINE | Kills during story grant discipline XP (research active) | T_DISC_01, T_DISC_02, CL_DEM_02 |

**Key insight**: Running story progression while questing creates a positive feedback loop — story kills feed quest objectives, and quest rewards (QP, XP) enable stat training for later story nodes.
