# Implementing a New Class

Lessons learned and a reference checklist from implementing the Dirgesinger (base) and Siren (upgrade) classes. Use this as a guide when adding future classes.

**Major Update (2024):** Many class display features are now **database-driven** via MudEdit:
- WHO list brackets and titles → `class_brackets`, `class_generations` tables
- Room aura text → `class_auras` table
- Score display stats → `class_score_stats` table
- Class armor configuration → `class_armor_config`, `class_armor_pieces` tables
- Selfclass/mudstat metadata → `class_registry` table
- Starting values → `class_starting` table

This significantly reduces the code changes needed for a new class. See "Database Tables (via MudEdit)" section below.

## Files That Need Changes

Every new class touches a minimum set of files. Missing any of these will cause build failures or runtime issues.

### New Files to Create

| File | Purpose |
|------|---------|
| `src/classes/<class>.c` | All ability functions, training command, armor command, tick update |
| `src/classes/<class>.h` | `pcdata->powers[]` index defines, `pcdata->stats[]` index defines, training constants |

If the new class is an upgrade that shares a header with its base class (like Siren shares `dirgesinger.h`), you can add the new defines to the existing header instead of creating a separate one.

### Core Files to Modify

| File | What to Add | Notes |
|------|-------------|-------|
| `src/core/class.h` | `#define CLASS_<NAME> <bit>` | Must be a unique power-of-2. Next available after 131072 is 262144. |
| `src/core/merc.h` | `DECLARE_DO_FUN` for every ability + training command | One line per function (armor command is now generic) |
| `src/core/interp.c` | Entry in `cmd_table[]` for every player command | Set `race` field to `CLASS_<NAME>` for class-restricted commands |
| `src/commands/act_info.c` | `class_name()` function, value calc (if using ch->rage) | Only 2-3 locations now - most display is DB-driven |
| `src/classes/clan.c` | `do_class` immortal class-setting command | Add to help text and class-setting switch |
| `src/systems/upgrade.c` | `is_upgrade()` check (upgrade classes only), upgrade path mapping | Two locations |
| `src/combat/fight.c` | Damcap bonuses, extra attack hooks, defensive mechanics (barrier, reflect) | Multiple locations, add `#include "<class>.h"` for constants |
| `src/systems/update.c` | Call to `<class>_update(ch)` in the tick loop | One line in the character update section |
| `src/core/handler.c` | Equipment vnum restrictions (if class armor exists) | Prevents other classes from using your gear |
| `src/core/ability_config.c` | Default `acfg` values for all balance-tunable parameters | Add entries to `acfg_table[]` array |
| `src/core/prompt.c` | `%R` prompt variable (if class uses `ch->rage` for resource) | Add class to the `case 'R':` check |
| `src/systems/save.c` | Verify `pcdata->powers[]` and `pcdata->stats[]` are saved/loaded | Usually already handled generically, but verify |
| `game/build/Makefile` | Add new `.c` file to the build | Run `GenerateProjectFiles.bat` (Windows) or `.sh` (Linux) instead of editing manually |

**Note:** Many items that previously required code changes are now database-driven. See "Database Tables" section below.

### Data Files to Modify

| File | What to Add |
|------|-------------|
| `gamedata/db/game/base_help.db` | Help entry for the class, system-specific help entries, update CLASSES help |
| `gamedata/db/game/game.db` | Runtime overrides for `acfg` values (defaults are in `ability_config.c`, overrides saved here) |
| `gamedata/db/areas/classeq.db` | Equipment stats for class armor pieces and mastery item |

### Database Tables (via MudEdit)

Most class display and configuration is now database-driven. Use MudEdit (`python -m mudedit` from `game/tools/`) to add entries. The server loads these at boot time.

| Table | MudEdit Panel | What to Add | Notes |
|-------|---------------|-------------|-------|
| `class_registry` | Class Registry | Class metadata: name, keyword, mudstat label, selfclass message | **Add first** - other tables reference this |
| `class_brackets` | Class Display | WHO list brackets with colors | e.g., `#R<<#n` and `#R>>#n` |
| `class_generations` | Class Display | Generation-based titles (1-13 + default) | One row per generation level |
| `class_auras` | Class Aura | Room presence text shown when looking | e.g., `#y(#LVampire#y)#n ` |
| `class_starting` | Class Starting | Starting beast/level values | Vampire: 30 beast, Monk/Mage: level 3 |
| `class_score_stats` | Class Score | Custom stats shown in `score` command | Uses STAT_SOURCE enum for value lookup |
| `class_armor_config` | Class Armor | Armor creation messages and primal cost key | Generic `do_classarmor` uses this |
| `class_armor_pieces` | Class Armor | Keyword-to-vnum mapping for armor pieces | e.g., "ring" → vnum 12345 |

**Important:** Add `class_registry` entry FIRST, before other tables. Foreign key constraints reference it.

**MudEdit also requires updating `repository.py`:** Add the new class to the `CLASS_NAMES` dictionary at the top of the file (single location - no longer duplicated across classes).

## Phase Planning Approach

Implementing a new class is complex work spanning many files. To increase accuracy and reduce errors, approach each phase as a distinct unit with its own planning step.

**Why plan each phase separately?**
- Each phase has different concerns and touches different files
- Planning before coding catches missing dependencies early
- Verification after each phase prevents error accumulation
- Smaller units of work reduce cognitive load

**Before starting any phase:**
1. Read through the phase requirements completely
2. List the specific files you'll modify and what changes each needs
3. Check if any previous phase work is incomplete
4. Identify potential conflicts or edge cases

**After completing any phase:**
1. Build the project and fix any compile errors
2. Run through the verification checklist
3. Note any discoveries that affect later phases

## Step-by-Step Implementation Order

This order minimizes back-and-forth and catches integration issues early.

### Phase 1: Class Definition

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Design document exists with ability list and resource mechanics
- [ ] `src/core/class.h` - identify the next available power-of-2 bit value
- [ ] Plan all `pcdata->powers[]` indices (0-9 for state, 10+ for training)
- [ ] Plan all `pcdata->stats[]` indices if needed

**Why this phase matters:**
The header file defines the permanent data structure for this class. Changing indices later requires dealing with existing player save files, so careful planning upfront saves significant pain.

**Implementation Steps:**
1. **Pick a class constant bit value** - Must be unique power-of-2 in `class.h`. Next available after 131072 is 262144.
2. **Create the header file** (`src/classes/<class>.h`) - Define all `pcdata->powers[]` and `pcdata->stats[]` indices. Separate ability state indices (0-9) from training indices (10+).

**Verification:**
- [ ] Class constant added to `class.h`
- [ ] Header file exists with all planned indices
- [ ] No index conflicts with other classes (base and upgrade share indices, but different classes must not overlap)

---

### Phase 2: Source Scaffolding

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 1 complete - header file exists with all indices
- [ ] Design document lists all abilities with their command names
- [ ] Search codebase for potential function name conflicts (see Common Pitfalls)

**Why this phase matters:**
Creating stub functions early lets you add all command table entries and function declarations in one pass. This catches naming conflicts and missing declarations before you invest time implementing abilities.

**Implementation Steps:**
1. **Create the source file** (`src/classes/<class>.c`) - Start with stub functions that send "Not yet implemented." Include the training command and armor command stubs.
2. **List all function names** - One `do_<ability>` function per ability, plus `do_<class>train`, `do_<class>armor`, and `<class>_update`.

**Verification:**
- [ ] Source file exists with all stub functions
- [ ] Each function has the correct signature: `void do_name(CHAR_DATA *ch, char *argument)`
- [ ] No function name conflicts with existing codebase

---

### Phase 3: Build Integration

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 2 complete - source file exists with all stub functions
- [ ] Count all functions that need `DECLARE_DO_FUN` entries
- [ ] Count all commands that need `cmd_table` entries

**Why this phase matters:**
This phase integrates your new files into the build system. A clean compile at this stage proves all declarations match their definitions and all commands are properly registered.

**Implementation Steps:**
1. **Add all `DECLARE_DO_FUN` entries to `merc.h`** - One line per function. Forgetting one causes a confusing linker error.
2. **Add all `cmd_table` entries to `interp.c`** - Set the `race` field to your class constant. Easy to forget for training and armor commands.
3. **Regenerate build files** - Run `GenerateProjectFiles.bat` (Windows) or `.sh` (Linux) to pick up the new `.c` file.
4. **Build and verify** - Should compile with no errors, even though abilities do nothing yet.

**Verification:**
- [ ] Build succeeds with no errors or warnings about the new class
- [ ] Each command works in-game (shows "Not yet implemented" message)
- [ ] Class-restricted commands are only available to the correct class

### Phase 4: Core Abilities

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 3 complete - all stubs compile and commands are registered
- [ ] Design document specifies ability mechanics, costs, and cooldowns
- [ ] Identify ability dependencies (which abilities must work before others)
- [ ] List all `acfg` keys needed for tunable values

**Why this phase matters:**
This is the core implementation work. Starting with the simplest ability and testing each before moving on catches bugs early and builds confidence in your understanding of the codebase patterns.

**Implementation Steps:**
1. **Prioritize abilities by dependency** - Some abilities may depend on others (e.g., a finisher that requires building stacks). Implement foundational abilities first.
2. **Implement abilities one at a time** - Start with the simplest (usually a basic damage attack). Test each in-game before moving to the next.
3. **Add `acfg` keys for every tunable value** - Mana costs, cooldowns, damage ranges, durations. Add defaults to `ability_config.c`. This makes balance adjustable at runtime without recompiling.

**Verification:**
- [ ] Each ability functions correctly in-game
- [ ] All `acfg` keys have defaults in `ability_config.c`
- [ ] No abilities crash or produce unexpected behavior
- [ ] Mana/resource costs are being deducted correctly

---

### Phase 5: Tick System

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 4 complete - core abilities work
- [ ] Design document specifies resource mechanics (build rate, decay, caps)
- [ ] Identify all buffs that need duration countdowns
- [ ] Identify all cooldowns that need timeout resets

**Why this phase matters:**
The tick update function handles all time-based mechanics: resource regeneration, buff expiration, cooldown recovery. Without this, abilities that apply buffs or use cooldowns won't work correctly.

**Implementation Steps:**
1. **Implement the tick update function** (`<class>_update`) - Handle:
   - Resource build/decay (e.g., Resonance, Focus)
   - Buff duration countdowns
   - Timeout resets for cooldown-based abilities
2. **Hook into `update.c`** - Add a call to `<class>_update(ch)` in the character update section.

**Verification:**
- [ ] Resource values change over time as expected
- [ ] Buffs expire after the correct number of ticks
- [ ] Cooldowns reset properly
- [ ] Build succeeds with no warnings

### Phase 6: Combat Integration

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 5 complete - tick system works correctly
- [ ] `src/combat/fight.c` - search for existing class patterns (e.g., `IS_CLASS(ch, CLASS_ANGEL)`)
- [ ] Design document specifies damcap bonuses, defensive mechanics, extra attack conditions
- [ ] Add `#include "<class>.h"` to fight.c for any constants needed

**Why this phase matters:**
Combat integration determines how your class performs in the core gameplay loop. Following existing patterns ensures consistency and makes the code easier to maintain.

**Implementation Steps:**
1. **Add damcap bonuses to `fight.c`** - Search for existing class patterns and add yours nearby. Damcap affects maximum damage per hit.
2. **Add defensive mechanics to `fight.c`** - Damage absorption barriers, reflection, etc. These go in the damage-receiving section.
3. **Add extra attack hooks** - If the class has haste/extra-attack buffs, add the check in the multi-attack section of `fight.c`.

**Verification:**
- [ ] Damcap values are correct (test with `score` or debug output)
- [ ] Defensive abilities reduce/reflect damage as designed
- [ ] Extra attacks trigger under the correct conditions
- [ ] Build succeeds with no warnings

### Phase 7: Class Equipment

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 6 complete - combat integration works
- [ ] `gamedata/db/areas/classeq.db` - identify existing vnum ranges to avoid overlap
- [ ] Design document specifies equipment slots and stat themes
- [ ] Decide on armor piece names and keywords

**Why this phase matters:**
Class equipment provides progression and identity. The vnum range you choose is permanent, so check existing ranges carefully. Equipment restrictions prevent other classes from using your gear.

**Implementation Steps:**
1. **Define equipment vnums** - Pick a contiguous range that doesn't overlap existing ranges. Add the objects to `classeq.db`.
2. **Configure armor via MudEdit** - Open the Class Armor panel:
   - Add a `class_armor_config` entry with:
     - `acfg_cost_key`: Key for primal cost lookup (e.g., `"classname.armor.practice_cost"`)
     - `usage_message`: Text showing available pieces
     - `act_to_char` / `act_to_room`: Messages when creating armor
   - Add `class_armor_pieces` entries mapping keywords to vnums (e.g., "ring" → 12345)
3. **Add equipment restrictions to `handler.c`** - Prevent other classes from equipping your class gear by checking vnum ranges.
4. **Add `acfg` default in `ability_config.c`** - Set the primal cost for armor creation.

**Note:** The armor command itself (`do_classarmor_generic` in `class_armor.c`) is shared by all classes. You do NOT need to write a custom armor command - just configure the database entries.

**Verification:**
- [ ] All equipment pieces can be created via the armor command
- [ ] Equipment has correct stats when examined
- [ ] Other classes cannot equip your class gear (test with a different class character)
- [ ] Primal costs are deducted correctly

### Phase 8: Display Integration

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 7 complete - equipment works correctly
- [ ] Choose your class color scheme (accent and primary colors)
- [ ] Plan generation-based title progression (13 levels + default)

**Why this phase matters:**
Display integration controls how your class appears everywhere in the game: who list, room descriptions, score command. Most of this is now database-driven via MudEdit, making it much simpler to add.

**Implementation Steps (MudEdit - no code changes):**
1. **Add who list brackets** - Class Display panel → class_brackets table. Add `open_bracket`/`close_bracket` with your color scheme.
2. **Add who list titles** - Class Display panel → class_generations table. Add 13 generation entries + a generation 0 default.
3. **Add room aura prefix** - Class Aura panel → class_auras table. Add the class tag shown when looking at players.
4. **Add score display** - Class Score panel → class_score_stats table. Add entries for each custom stat (use STAT_SOURCE enum).

**Implementation Steps (Code - minimal):**
5. **Add value calculation** (if needed) - If your class uses `ch->rage` for a resource, add it to value calculation in `act_info.c` (search for "Dirgesinger/Siren Resonance").
6. **Add to `class_name()` function** - Return the class name string in the switch statement.

**Verification:**
- [ ] Who list shows correct brackets and titles for your class
- [ ] Looking at a player shows the correct room aura tag
- [ ] Score command displays custom stats correctly
- [ ] `class_name()` returns correct string

---

### Phase 9: Help System

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 8 complete - display works correctly
- [ ] Draft help text before inserting into database
- [ ] Review existing class help entries for format consistency
- [ ] List all system-specific topics that need separate help entries

**Why this phase matters:**
Help entries are how players learn to use your class. Well-written help with proper cross-references makes the class accessible. The CLASSES help entry is the first thing new players see when choosing a class.

**Implementation Steps:**
1. **Create main class help entry** - Overview, abilities list, key commands. Use class colors. Keyword should be the class name.
2. **Create system-specific help entries** - If the class has unique systems (e.g., "RESONANCE", "FOCUS"), create separate help entries.
3. **Add "See also:" footers** - Cross-reference related topics at the bottom of each help entry.
4. **Update CLASSES help entry** - Add the new class name to the list of available classes.

**Verification:**
- [ ] `help <classname>` shows the main help entry
- [ ] All system-specific help entries work
- [ ] Cross-references link to valid help topics
- [ ] `help classes` includes the new class

---

### Phase 10: Class Selection

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 9 complete - help entries exist
- [ ] Prepare a thematic welcome/confirmation message for `selfclass`
- [ ] Decide if this is a base class (player-selectable) or upgrade class

**Why this phase matters:**
These commands are how players and immortals assign classes. Base classes are available via `selfclass`; upgrade classes only via immortal `class` command or the upgrade system.

**Implementation Steps (MudEdit):**
1. **Add to class_registry** - Class Registry panel:
   - `class_name`: Display name (e.g., "Vampire")
   - `keyword`: Selfclass keyword (e.g., "vampire")
   - `keyword_alt`: Optional alternate keyword (e.g., "battlemage" for Mage)
   - `mudstat_label`: Plural form for mudstat (e.g., "Vampires")
   - `selfclass_message`: Welcome message with color codes
   - `upgrade_class`: NULL for base class, or class_id of the base class this upgrades FROM
   - `display_order`: Controls mudstat display order

**Implementation Steps (Code):**
2. **Add to `do_class`** (all classes) in `clan.c`:
   - Add class name to help text listing
   - Add class-setting case

**Note:** The `do_classself` command now uses database lookup via `db_game_get_registry_by_keyword()`. You do NOT need to modify `wizutil.c` for new base classes - just add the class_registry entry with `upgrade_class = NULL`.

**Verification:**
- [ ] `selfclass <classname>` works for base classes
- [ ] `class <player> <classname>` works for immortals
- [ ] Welcome message displays correctly (from class_registry.selfclass_message)
- [ ] `mudstat` shows the new class with correct label

### Phase 11: Mastery Item

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 10 complete - class selection works
- [ ] `gamedata/db/areas/classeq.db` - pick a vnum outside armor range but documented nearby
- [ ] Design mastery item stats that match class theme

**Why this phase matters:**
The mastery item is a prestigious reward for dedicated players. It uses `questowner` binding rather than class restrictions, so it doesn't need handler.c updates.

**Implementation Steps:**
1. **Create mastery item** - Add to `classeq.db`:
   - Pick a vnum outside your class armor range (e.g., Dirgesinger armor is 33320-33332, mastery is 33253)
   - Design stats matching your class theme (mana-focused for casters, HP/combat for melee)
   - Use your class color scheme in the short description
2. **Add mastery vnum to `jobo_act.c`** - In `do_mastery()`, add an `else if (IS_CLASS(ch, CLASS_<NAME>))` case with your item vnum.

**Verification:**
- [ ] Mastery command creates the correct item for your class
- [ ] Item stats match design document
- [ ] Item has correct name and description with class colors

---

### Phase 12: Documentation

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phase 11 complete - mastery item works
- [ ] All abilities implemented and tested
- [ ] Class is fully playable from selection to mastery

**Why this phase matters:**
Documentation preserves knowledge for future development. The design doc captures decisions and tradeoffs; the overview docs help players and developers find information.

**Implementation Steps:**
1. **Write the design doc** - Document everything in `game/docs/design/playerclass/classes/<class>.md`.
2. **Update overview docs** - Add to `overview.md` and `README.md`.

**Verification:**
- [ ] Design doc exists and covers all abilities
- [ ] Overview docs updated with new class
- [ ] Any new patterns or pitfalls documented in this guide

---

### Phase 13: Upgrade Class

**Planning Checklist:**
Before starting this phase, verify:
- [ ] Phases 1-12 complete for the base class
- [ ] Design document exists for the upgrade class
- [ ] Identify which elements are shared vs unique between base and upgrade

**Why this phase matters:**
Every base class has a corresponding upgrade class. This is a required part of class design. The upgrade class shares some infrastructure (header indices, upgrade.c mapping) but needs its own abilities, display entries, help, and mastery item.

**Implementation Steps:**
1. **Add upgrade path in `upgrade.c`** - Map base class to upgrade class in the upgrade switch, and add to `is_upgrade()`.
2. **Add to immortal `do_class`** - Upgrade classes need entries in `clan.c` for immortals to assign them, but NOT in `do_classself` (players can only select base classes).
3. **Repeat Phases 1-12 for the upgrade class** - The upgrade class needs its own abilities, act_info.c display entries, help entries, and mastery item.

**Verification:**
- [ ] `upgrade` command transitions base class to upgrade class correctly
- [ ] Upgrade class has all display/help/equipment entries
- [ ] Upgrade abilities work independently of base abilities
- [ ] Mastery item exists for upgrade class

## Common Pitfalls

### `pcdata->powers[]` Index Conflicts

Both the base class and its upgrade class share the same `pcdata->powers[]` array. Since a character can only be one class at a time, it is safe to reuse indices between a base/upgrade pair. However, **ability state indices** (0-9) and **training indices** (10+) should be kept separate from each other.

Example from Dirgesinger/Siren:
- Indices 0-8: Ability state (buff durations, DOT stacks, echoshield, crescendo)
- Indices 10-13: Training categories (reused between both classes)

### The `race` Field in `cmd_table`

The `race` field in `cmd_table[]` entries is a class restriction bitmask, not a "race." If set to `CLASS_DIRGESINGER`, only Dirgesingers can use the command. If set to 0, anyone can use it. A common mistake is forgetting to set this, which makes class abilities available to everyone.

### Function Name Conflicts

Before naming abilities, search the codebase for existing `do_<name>` functions to avoid linker errors (LNK2005 multiply defined symbol). Common conflicts encountered:

| Desired Name | Conflict Location | Resolution |
|--------------|-------------------|------------|
| `do_focus` | `samurai.c` | Use `do_psifocus` |
| `do_meditate` | `act_move.c` | Use `do_psimeditate` |
| `do_mindblast` | `vamp.c` | Use `do_psiblast` |

**Best practice**: Prefix class-specific commands with a short class identifier (e.g., `psi`, `dirge`, `siren`) to avoid conflicts and make grep searches easier.

### act_info.c Checklist (Simplified)

Most display features are now database-driven. The `act_info.c` file only needs updates in **2-3** locations:

1. **class_name() function** - Add class name string. Search for `CLASS_SIREN` in `class_name()` and add before `return "Hero"`.

2. **Value calculation** (if class uses `ch->rage`) - Add `ch->rage` to value if class uses a resource stored there (Focus, Resonance, etc.). Search for "Dirgesinger/Siren Resonance" to find both locations.

3. **"mad frenzy" exclusion** (if class uses `ch->rage`) - Update the exclusion list if your class uses `ch->rage` for something other than frenzy (search for "mad frenzy").

**The following are now DATABASE-DRIVEN (via MudEdit):**
- ~~Room aura prefix~~ → `class_auras` table (Class Aura panel)
- ~~Score display~~ → `class_score_stats` table (Class Score panel)
- ~~Who list brackets~~ → `class_brackets` table (Class Display panel)
- ~~Who list titles~~ → `class_generations` table (Class Display panel)

### do_mudstat Player Statistics (Now Database-Driven)

The `do_mudstat` command in `jobo_act.c` now uses the `class_registry` database table. **No code changes are needed** - just add an entry to the class_registry table via MudEdit.

The command iterates through all entries in `class_registry` using `db_game_get_registry_by_index()` and displays the `mudstat_label` field for each class.

**To add a new class to mudstat:**
1. Open MudEdit → Class Registry panel
2. Add your class with appropriate `mudstat_label` (e.g., "Vampires", "Demons")
3. Set `display_order` to control row grouping (lower = displayed earlier)

**Note**: The old manual counter/switch/sprintf pattern in `jobo_act.c` has been replaced with a loop over the registry cache.

### `visible_strlen()` and Color Codes

The who list formatting in `act_info.c` uses `visible_strlen()` to calculate display width for alignment. This function correctly handles standard `#X` codes (2 chars) and `#xNNN` codes (5 chars). If you use other non-standard color sequences, verify alignment still works.

### Prompt Variables for Combat Resources

If your class uses `ch->rage` for a combat resource (Focus, Resonance, Beast, etc.), update `prompt.c` to include the class in the `%R` prompt variable:

```c
// In prompt.c, case 'R':
if ( !IS_NPC( ch ) && ( IS_CLASS( ch, CLASS_WEREWOLF ) || ... || IS_CLASS( ch, CLASS_YOURCLASS ) ) ) {
    snprintf( buf2, sizeof( buf2 ), "#r%d#n", ch->rage );
}
```

This allows players to display their resource in their prompt using `%R`. Without this change, `%R` will show "0" for your class. Players can then configure prompts like:
- `prompt <%hhp %mm %vmv %RFocus>`

### `acfg` Default Values

Every `acfg("key")` call must have a corresponding default value in `ability_config.c`. If a key has no default and no entry in `game.db`, `acfg()` logs a warning and returns 0. This can cause abilities to cost 0 mana, deal 0 damage, or have 0-tick cooldowns, which can be difficult to diagnose.

Pattern for adding defaults:
```c
// In ability_config.c, within the acfg_table[] array:
{ "classname.ability.mana_cost", 50, 50 },  // key, value, default_value
```

### `raw_kill()` vs Normal Death

If an ability can reduce HP below the death threshold directly (bypassing `damage()`), you must call `raw_kill()` manually. This skips normal death processing (corpse creation, death cry, etc.), so only use it intentionally. Siren's `soulrend` does this.

### Color Scheme Selection

When choosing x256 colors (`#xNNN`), test them on both dark and light terminal backgrounds. Some clients render 256-color differently. Stick to colors in the 16-231 range for consistent results. The 232-255 range is grayscale and may not stand out.

Use two colors per class: an **accent** color (for decorative elements like tildes and brackets) and a **primary** color (for class name, titles, ability highlights). This gives the class visual identity on the who list and in ability messages.

### Build File Regeneration

After adding or removing any `.c` or `.h` file, regenerate the build files:
- **Windows**: `cmd.exe /c game\build\GenerateProjectFiles.bat`
- **Linux**: `cd game && ./build/GenerateProjectFiles.sh`

Forgetting this step means the new source file won't be compiled, leading to linker errors about undefined functions.

### Class Selection Commands

**`do_classself` (wizutil.c)** - Player self-selection at avatar:
- **Now database-driven** - Uses `db_game_get_registry_by_keyword()` to look up class
- **No code changes needed for new base classes** - just add a class_registry entry with `upgrade_class = NULL`
- The ASCII art display in `do_classself` is still hardcoded (rarely changes)
- Mage still has a special requirements check in code (5K mana + spell colors)

**`do_class` (clan.c)** - Immortal command to set any player's class:
- **Still requires code changes** - Both base and upgrade classes need entries here
- Add the class name to the help text that lists available classes
- Add an `else if (!str_cmp(arg2, "classname"))` case to set `victim->class`
- Call `set_learnable_disciplines()` if the class uses the discipline system

### Ability Learning Systems

The codebase has two paradigms for how players unlock and progress abilities:

**Discipline System** (Vampire, Werewolf, Demon and their upgrades):
- Uses `pcdata->disc[]` array for discipline levels
- Abilities are learned through in-game actions (drinking blood, shifting forms, etc.)
- `set_learnable_disciplines()` in `handler.c` initializes available disciplines
- See [disciplines.md](disciplines.md) for full documentation
- Call `set_learnable_disciplines(ch)` when setting the class in `do_classself` and `do_class`

**Training System** (Dirgesinger, Siren, Psion, Mindflayer, Ninja, Monk):
- Uses `pcdata->powers[10+]` indices for training category levels
- Players spend primal currency via a training command (e.g., `psitrain`, `songtrain`, `voicetrain`)
- Categories unlock abilities progressively (level 1-4 in each category)
- No special initialization needed - training levels start at 0

Most new classes should use the **Training System** as it's simpler to implement and doesn't require modifying `handler.c`. The discipline system is legacy code maintained for the original classes.

When implementing a training system:
1. Define `*_TRAIN_*` constants in your header (indices 10+)
2. Create a `do_<class>train` function that handles primal cost and level incrementing
3. Check training levels in ability functions to gate access

### Training Category Design

Group abilities into 2-4 training categories of 2-4 abilities each. This gives players meaningful progression choices without being overwhelming. Each category should have a thematic identity (offensive, defensive, utility, etc.) so the choice feels distinct.

Training cost formula pattern: `(current_level + 1) * base_cost` where `base_cost` is higher for upgrade classes (50 primal for Siren vs 40 for Dirgesinger).

### Help File Structure

Help entries in `base_help.db` support the same color codes as in-game text. Each class needs multiple help entries:

**Main Class Help Entry:**
- Keyword: Class name (e.g., `DIRGESINGER`)
- Contents: Overview paragraph, ability list with short descriptions, key commands section
- Use class colors for visual identity
- End with "See also:" footer linking to related topics

**System-Specific Help Entries:**
- Create separate entries for unique class mechanics (e.g., `RESONANCE`, `FOCUS`, `ECHOES`)
- These give players detailed information on resource systems, training categories, etc.
- Cross-reference back to main class help

**CLASSES Help Entry:**
- Update the existing `CLASSES` entry to include the new class in the list
- This is what players see when typing `help classes`

**Example "See also:" footer:**
```
See also: RESONANCE, DIRGESINGERARMOR, DIRGESINGER TRAINING
```

### Help File Color Codes

Use your class colors in the help entry. Keep the overall structure consistent with other class help entries.

## Template: Minimal Class Skeleton

### Code Changes (Compile-Time)

A minimal new base class needs these **code** changes:

```
class.h          +1 line   (CLASS_FOO define)
merc.h           +N lines  (DECLARE_DO_FUN per ability)
interp.c         +N lines  (cmd_table entries)
act_info.c       +1-3 blocks (class_name, value calc if using ch->rage)
clan.c           +2 blocks (do_class help text + class-setting case)
update.c         +1 line   (tick update call)
fight.c          +1-3 blocks (damcap, defense, extra attacks) + #include
ability_config.c +N lines  (acfg defaults in acfg_table[])
handler.c        +1 block  (equipment restrictions)
jobo_act.c       +1 block  (do_mastery vnum mapping only - mudstat is now DB)
```

### Database Entries (via MudEdit)

These are configured in `game.db` via MudEdit - no code changes needed:

```
class_registry      +1 entry  (name, keyword, mudstat label, selfclass message)  ← ADD FIRST
class_brackets      +1 entry  (WHO list open/close brackets with colors)
class_generations   +14 entries (generation titles 1-13 + default)
class_auras         +1 entry  (room presence text)
class_starting      +1 entry  (starting beast/level values)
class_score_stats   +N entries (custom score display stats)
class_armor_config  +1 entry  (armor creation messages)
class_armor_pieces  +N entries (keyword-to-vnum mapping)
```

### Data Files

```
classeq.db       +N entries (class armor + mastery item object definitions)
base_help.db     +2-4 entries (class help, system helps, update CLASSES)
repository.py    +1 line   (add class to CLASS_NAMES dictionary at module top)
```

### Upgrade Class Additions

The corresponding upgrade class (required for every base class) additionally needs:

**Code:**
```
upgrade.c        +2 blocks (is_upgrade check, upgrade path mapping)
clan.c           +1 block  (do_class class-setting case for immortals)
act_info.c       +1 block  (class_name only - display is DB-driven)
+ Standard code changes (merc.h, interp.c, fight.c, etc.) for upgrade abilities
```

**Database (MudEdit):**
```
class_registry   +1 entry  (set upgrade_class to base class_id)
class_brackets   +1 entry
class_generations +14 entries
class_auras      +1 entry
class_starting   +1 entry
class_score_stats +N entries
class_armor_*    (if upgrade has different armor)
```

**Note:** Upgrade classes set `upgrade_class` in class_registry to the base class's class_id (not NULL). This marks them as upgrade classes, making them unavailable via `selfclass`.
