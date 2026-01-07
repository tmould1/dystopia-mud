# Experience Level Selection

## Overview

During character creation, new players are asked about their prior experience. This selection determines their starting location, tutorial path, and the type of welcome messages they receive.

## The Three Experience Levels

### Level 1: Never Played a MUD

**Target audience:** Complete beginners who have never played any text-based multiplayer game.

**What they need to learn:**
- How to navigate (cardinal directions, open/close doors)
- How to perceive the world (look, scan, area commands)
- How to communicate (say, tell, channels)
- How to interact with objects (take, drop, sacrifice)
- How to manage equipment (wear, remove, compare)
- How to use abilities (cast, practice)
- How to improve their character (train)

**Starting point:** Beginning of the full tutorial in MUD school Section 1.

**Welcome messaging:** Comprehensive introduction explaining the text-based interface and basic interaction model.

### Level 2: MUD Experience, New to Dystopia

**Target audience:** Players familiar with MUDs but who haven't played Dystopia before.

**What they need to learn:**
- Class system and `clasself` command
- Upgrade class progression
- Quest system mechanics
- PK rules and paradox penalties
- Generation system
- Forge system for equipment enhancement

**Starting point:** MUD school Section 2, skipping basic MUD concepts.

**Welcome messaging:** Brief overview highlighting what makes Dystopia unique compared to other MUDs.

### Level 3: Dystopia Veteran

**Target audience:** Returning players or those creating alt characters who already know the game.

**What they need:** Quick path to gameplay with minimal interruption.

**Starting point:** Current school entrance (near arena directions), allowing immediate access to familiar content.

**Welcome messaging:** Minimal - just essential info like recent changes or server rules.

## User Flow

```
Character Creation
        |
        v
  Name Selection
        |
        v
  Password Setup
        |
        v
  Gender Selection
        |
        v
+-------------------+
| Experience Level? |
|                   |
| 1. Never MUD'd    |
| 2. MUD, not Dyst. |
| 3. Dystopia vet   |
+-------------------+
        |
        v
  Protocol Options
  (ANSI/GMCP/MXP)
        |
        v
  MOTD Display
        |
        v
  Enter World
  (location based on
   experience level)
```

## Design Considerations

### Placement in Character Creation

The experience question should come after basic identity setup (name, password, gender) but before technical options (color, protocols). This groups "about you as a player" questions together.

### Non-Judgmental Framing

The question should be welcoming regardless of answer. Avoid implying that veterans are "better" or that beginners are a burden. All paths lead to the same game - just with different amounts of guidance.

### Flexibility

Players should be able to access tutorial content later if needed, even if they chose to skip it initially. Consider a command like `tutorial` or `school` to revisit lessons.

### Persistence

The experience level could optionally be stored on the character for:
- Adjusting help system verbosity
- Unlocking newbie chat access timing (4 hours before regular chat for complete beginners)
- Tailoring hint messages during early gameplay
