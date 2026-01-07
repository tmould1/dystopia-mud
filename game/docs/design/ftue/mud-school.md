# MUD School Redesign

## Overview

The MUD school is restructured into three distinct sections, each designed for a different experience level. Players enter at the section matching their selected experience, but can explore other sections if desired.

## Section 1: New to MUDs

**Purpose:** Teach fundamental MUD concepts to complete beginners.

### Learning Modules

#### 1.1 Movement and Navigation
- Cardinal directions (north, south, east, west, up, down)
- Opening and closing doors
- Understanding room descriptions
- Using `exits` to see available directions

#### 1.2 Awareness and Perception
- `look` - examining rooms and objects
- `scan` - seeing into adjacent rooms
- `area` - understanding your location in the world
- Reading room descriptions for environmental clues

#### 1.3 Communication
- `say` - talking to people in the same room
- `tell` - private messages to specific players
- `board` and `note` - asynchronous message boards
- `newbiechat` - dedicated channel for new players (before unlocking main chat)
- Explanation that main chat unlocks after a few hours of play

#### 1.4 World Interaction
- `take` / `get` - picking up items
- `drop` - putting items down
- `sacrifice` - disposing of unwanted items for rewards
- Interacting with NPCs

#### 1.5 Equipment Management
- `wear` - equipping items
- `remove` - unequipping items
- `compare` - evaluating item quality
- Understanding equipment slots
- `inventory` and `equipment` commands

#### 1.6 Skills and Abilities
- `cast` - using magical abilities
- `practice` - improving skills at trainers
- Understanding skill percentages
- Class-specific ability usage

#### 1.7 Character Development
- `train` - improving base attributes
- Understanding stats (str, int, wis, dex, con)
- Experience and leveling
- Where to train (finding trainers)

### Section 1 Flow

Each module should be a room or small area with:
- Clear instructions posted in the room description
- Practice objects or NPCs to interact with
- A gate/exit that encourages completion before moving on
- No hard locks - players can skip ahead if they want

## Section 2: New to Dystopia

**Purpose:** Introduce Dystopia-specific systems to players who already understand MUD basics.

### Learning Modules

#### 2.1 Class System
- Overview of available classes (Vampire, Werewolf, Demon, etc.)
- Using `clasself` to select a class
- Understanding that class choice is significant
- Brief description of each class's theme

#### 2.2 Upgrade Classes
- Explanation that base classes can upgrade
- Requirements for upgrading (stats, quest points, generation)
- Overview of upgrade paths (Vampire -> Undead Knight, etc.)
- Long-term progression goals

#### 2.3 Quest System
- How to find and complete quests
- Quest point rewards and their uses
- Quest difficulty and level appropriateness
- Quest command basics

#### 2.4 PK System
- Dystopia's approach to player killing
- Paradox penalties - what they are and how to avoid them
- Safe zones and PK-enabled areas
- Etiquette and unwritten rules

#### 2.5 Generations
- What generation means in Dystopia
- How generation affects power
- Ways to improve generation
- Generation requirements for upgrades

#### 2.6 Forge System
- Finding forge materials (metals, gems, hilts)
- Using the `forge` command
- Enhancement stacking rules
- Material rarity and drop locations

### Section 2 Flow

More compact than Section 1 - these players know how to navigate and read. Focus on information delivery through:
- Informative room descriptions
- NPCs that can be asked about specific topics
- Quick reference signs or books
- Clear exit to the main game when ready

## Section 3: Veteran Entry

**Purpose:** Minimal friction path for returning players.

### Content

- Single entry room with directions to key areas:
  - Arena (for immediate PvP)
  - Main city/hub
  - Popular hunting grounds
- New help entry for 'dystopiaplus/dystopia+" with changes from stock dystopia
- Quick access to trainers and shops
- No mandatory tutorial content

### Design Philosophy

Veterans chose this path because they want to play, not read. Get them into the game as fast as possible while still providing resources if they need a refresher.

## Cross-Section Navigation

- Each section should have clearly marked paths to other sections
- Section 2 and 3 should have shortcuts back to Section 1 for reference
- Completing Section 1 naturally flows into Section 2

## NPCs and Guidance

### The Spirit (Existing NPC)
Repurpose as a guide that:
- Greets players based on their experience level
- Offers directions to appropriate section
- Answers common questions
- Can be asked about specific topics

### Section-Specific Tutors
Each section could have a themed guide:
- Section 1: "The Mentor" - patient, encouraging, explains basics
- Section 2: "The Chronicler" - knowledgeable about Dystopia lore and systems
- Section 3: "The Gatekeeper" - brief, efficient, points the way
