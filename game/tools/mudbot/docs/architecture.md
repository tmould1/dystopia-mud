# Mudbot Architecture

## Overview

The mudbot framework provides automated character progression for Dystopia MUD. It uses a modular plugin-based architecture that makes adding support for new character classes straightforward.

## Design Goals

- Adding a new class requires only a config file + class-specific module
- Common functionality (avatar progression, combat, movement) is shared
- Class-specific logic is isolated in plugins
- Configuration-driven where possible (YAML)

## Directory Structure

```
game/tools/mudbot/
├── bot/
│   ├── base.py                    # MudBot base (connection, login)
│   ├── actions.py                 # Common actions (move, kill, score, equip)
│   ├── avatar_bot.py              # Legacy avatar-only bot
│   ├── state_machine.py           # Login state machine
│   ├── progression/
│   │   ├── __init__.py
│   │   ├── base.py                # ClassProgressionBot base class
│   │   ├── avatar.py              # AvatarProgression mixin (pre-class)
│   │   ├── registry.py            # Class plugin registry
│   │   └── classes/
│   │       ├── __init__.py
│   │       ├── demon.py           # DemonProgression
│   │       ├── vampire.py         # VampireProgression (future)
│   │       ├── werewolf.py        # WerewolfProgression (future)
│   │       └── mage.py            # MageProgression (future)
│   └── class_actions/
│       ├── __init__.py
│       ├── base.py                # ClassActions base
│       ├── discipline_actions.py  # Shared: research, train (vampire/demon/werewolf)
│       └── demon_actions.py       # Demon-specific: obtain, inpart, graft
├── config/
│   ├── classes/
│   │   ├── demon.yaml             # Demon progression config
│   │   └── ...                    # Future class configs
│   └── shared.yaml                # Shared settings (arena, HP targets)
├── utils/
│   ├── parsers.py                 # Common parsers
│   └── class_parsers/
│       ├── __init__.py
│       ├── base.py                # BaseClassParser
│       ├── discipline_parser.py   # Shared research/train patterns
│       └── demon_parser.py        # Demon-specific patterns
├── docs/
│   └── architecture.md            # This file
├── config.py                      # Dataclasses for all configs
├── gui.py                         # Tkinter GUI
└── __main__.py                    # CLI entry point
```

## Core Components

### ClassProgressionBot Base

Abstract base class for all class progression bots. Provides:
- Common state machine states (START, CHECKING_STATS, FARMING, etc.)
- Shared methods (select_class, create_armor)
- Abstract methods subclasses must implement

```python
class ClassProgressionBot(MudBot, ABC):
    @abstractmethod
    def get_class_name(self) -> str:
        """Return class name for selfclass command."""
        pass

    @abstractmethod
    async def run_class_progression(self) -> bool:
        """Execute class-specific progression logic."""
        pass
```

### AvatarProgressionMixin

Mixin providing shared avatar progression (level 1 → 3). All classes must reach avatar status before selecting their class, so this logic is shared.

### Class Plugin Registry

Decorator-based registry for class progression bots:

```python
@ClassRegistry.register("demon")
class DemonProgressionBot(ClassProgressionBot, AvatarProgressionMixin):
    ...

# Usage:
bot = ClassRegistry.create("demon", config)
```

### Discipline Actions (Shared)

Discipline-based classes (Vampire, Demon, Werewolf) share the research/train system:

```python
class DisciplineActions:
    async def start_research(self, discipline: str) -> bool
    async def cancel_research(self) -> bool
    async def train_discipline(self, discipline: str) -> dict
    async def get_disciplines(self) -> Dict[str, int]
```

## Progression Types

### Pattern A: Discipline-Based (Vampire, Demon, Werewolf)

```
research <discipline> → kill mobs → train <discipline> → repeat
```

- Uses shared `DisciplineActions` class
- Points needed per level: `(current_level + 1) * 10`
- Config specifies: disciplines, levels, unlocks

### Pattern B: Invoke-Based (Mage)

```
invoke learn <power> → use spells → progress
```

- Uses `MageActions` class for chanting/invoking
- Config specifies: invokes, mana costs

## Configuration Files

### Class Config (YAML)

Each class has a YAML configuration file defining:
- Selfclass command and success patterns
- Progression type (discipline/invoke)
- Disciplines/abilities with priority order
- Ability requirements and unlock levels
- Armor pieces and costs
- Message patterns for parsing

Example structure:

```yaml
class_name: demon
display_name: Demon
progression_type: discipline

disciplines:
  - name: attack
    target_level: 7
    priority: 1
    unlocks:
      2: [claws]
      5: [wings, graft]

abilities:
  obtain:
    command: "obtain"
    cost: 15000
    success_pattern: "You have obtained a new warp!"

armor:
  command: "demonarmour {piece}"
  cost: 60
  pieces: [plate, helmet, ...]

patterns:
  research_complete: "You have finished researching"
```

### Shared Config (YAML)

Common settings for all classes:

```yaml
avatar:
  hp_target: 2000
  kills_needed: 5
  arena_monsters: [fox, rabbit, ...]

combat:
  min_hp_to_fight: 500
  flee_threshold: 100
```

## Adding a New Class

### Step 1: Create Config File

Create `config/classes/<classname>.yaml` with:
- Class metadata (name, display_name)
- Progression configuration
- Abilities and requirements
- Message patterns

### Step 2: Create Class Module

Create `bot/progression/classes/<classname>.py`:

```python
@ClassRegistry.register("classname")
class ClassnameProgressionBot(ClassProgressionBot, AvatarProgressionMixin):
    def get_class_name(self) -> str:
        return "classname"

    async def run_class_progression(self) -> bool:
        # Class-specific logic
        # Uses shared DisciplineActions if discipline-based
        pass
```

### Step 3: Add Class-Specific Actions (if needed)

If the class has unique abilities beyond the shared systems, create `bot/class_actions/<classname>_actions.py`.

### Step 4: Add Parser Patterns (if needed)

If the class has unique message patterns, create `utils/class_parsers/<classname>_parser.py`.

## State Machine

### Avatar Progression States

```
START → CHECKING_STATS → FINDING_ARENA → KILLING_MONSTERS
    → WAITING_COMBAT → REGENERATING → SAVING
    → TRAINING_HP → TRAINING_AVATAR → [class selection]
```

### Demon Progression States

```
SELECTING_CLASS → CHECKING_DISCIPLINES → STARTING_RESEARCH
    → FARMING_DISCIPLINE_POINTS → TRAINING_DISCIPLINE
    → ENABLING_COMBAT_ABILITIES → OBTAINING_WARPS
    → CHECKING_GRAFT → GRAFTING_ARMS → CREATING_DEMONARMOUR
    → COMPLETE
```

## Message Patterns

### Discipline System (Shared)

| Event | Pattern |
|-------|---------|
| Research start | `You begin your research into (\w+).` |
| Research complete | `You have finished researching your discipline` |
| May train | `You may now use the 'train' command` |
| Discipline point | `You gained a discipline point` |
| Train success | `Your mastery of (\w+) increases` |

### Demon-Specific

| Event | Pattern |
|-------|---------|
| Obtain success | `You have obtained a new warp!` |
| Graft success | `You graft an arm onto your body` |
| Claws unlock | `You grow a pair of wicked claws` |
| Wings unlock | `A pair of leathery wings grow out of your back` |

## Future Enhancements

- Load class configs from YAML at runtime
- Auto-discover class modules via entry points
- Web-based monitoring dashboard
- Multi-character party coordination
- Equipment optimization
- PvP combat support
