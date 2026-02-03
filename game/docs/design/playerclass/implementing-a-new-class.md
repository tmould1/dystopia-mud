# Implementing a New Class

Lessons learned and a reference checklist from implementing the Dirgesinger (base) and Siren (upgrade) classes. Use this as a guide when adding future classes.

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
| `src/core/class.h` | `#define CLASS_<NAME> <bit>` | Must be a unique power-of-2. Next available after 32768 is 65536. |
| `src/core/merc.h` | `DECLARE_DO_FUN` for every ability + training + armor command | One line per function |
| `src/core/interp.c` | Entry in `cmd_table[]` for every player command | Set `race` field to `CLASS_<NAME>` for class-restricted commands |
| `src/commands/act_info.c` | Who list brackets (`openb`/`closeb`), who titles by generation, room aura tag | Three separate locations in this file |
| `src/systems/upgrade.c` | `is_upgrade()` check (upgrade classes only), upgrade path mapping | Two locations |
| `src/combat/fight.c` | Damcap bonuses, extra attack hooks, defensive mechanics (barrier, reflect) | Multiple locations, search for existing class patterns |
| `src/systems/update.c` | Call to `<class>_update(ch)` in the tick loop | One line in the character update section |
| `src/core/handler.c` | Equipment vnum restrictions (if class armor exists) | Prevents other classes from using your gear |
| `src/commands/wizutil.c` | `selfclass` command entry for base classes | Allows players to select the class at avatar |
| `src/classes/clan.c` | `do_class` entry for immortal class assignment | Allows immortals to set player class |
| `src/db/db_sql.h` | Default `acfg` values for all balance-tunable parameters | One INSERT per config key |
| `src/systems/save.c` | Verify `pcdata->powers[]` and `pcdata->stats[]` are saved/loaded | Usually already handled generically, but verify |
| `game/build/Makefile` | Add new `.c` file to the build | Run `GenerateProjectFiles.bat` (Windows) or `.sh` (Linux) instead of editing manually |

### Data Files to Modify

| File | What to Add |
|------|-------------|
| `gamedata/db/game/base_help.db` | Help entry for the class (and update the CLASS/CLASSES entry) |
| `gamedata/db/game/game.db` | Default `acfg` balance values (inserted via db_sql.h on fresh DB, or manually for existing DBs) |
| `gamedata/db/game/classeq.db` | Equipment stats for class armor pieces (if applicable) |

## Step-by-Step Implementation Order

This order minimizes back-and-forth and catches integration issues early.

### Phase 1: Foundation

1. **Pick a class constant bit value** - Must be unique power-of-2 in `class.h`
2. **Create the header file** - Define all `pcdata->powers[]` and `pcdata->stats[]` indices up front. Plan these carefully; changing indices later means dealing with existing player save files.
3. **Create the source file** - Start with stub functions that just send "Not yet implemented." This lets you add everything to `interp.c` and `merc.h` immediately and verify it compiles.
4. **Add all `DECLARE_DO_FUN` entries to `merc.h`** - Do this early. Forgetting one causes a linker error that can be confusing.
5. **Add all `cmd_table` entries to `interp.c`** - Set the `race` field to your class constant. This is easy to forget for the training and armor commands.
6. **Regenerate build files** - Run `GenerateProjectFiles.bat`/`.sh` to pick up the new `.c` file.
7. **Build and verify** - Should compile with no errors at this point, even though abilities do nothing yet.

### Phase 2: Core Abilities

8. **Implement abilities one at a time** - Start with the simplest (usually a basic damage attack). Test each before moving to the next.
9. **Add `acfg` keys for every tunable value** - Mana costs, cooldowns, damage ranges, durations. Add defaults to `db_sql.h`. This makes balance adjustable at runtime without recompiling.
10. **Implement the tick update function** - Resonance/resource build/decay, buff duration countdowns, timeout resets. Hook it into `update.c`.

### Phase 3: Combat Integration

11. **Add damcap bonuses to `fight.c`** - Search for existing class patterns (e.g., `IS_CLASS(ch, CLASS_ANGEL)`) and add yours nearby.
12. **Add defensive mechanics to `fight.c`** - Damage absorption barriers, reflection, etc. These go in the damage-receiving section.
13. **Add extra attack hooks** - If the class has haste/extra-attack buffs, add the check in the multi-attack section of `fight.c`.

### Phase 4: Class Equipment

14. **Define equipment vnums** - Pick a contiguous range that doesn't overlap existing ranges. Add the objects to `classeq.db`.
15. **Implement the armor creation command** - Standard pattern: check class, check primal cost, look up vnum by keyword, create object, give to character.
16. **Add equipment restrictions to `handler.c`** - Prevent other classes from equipping your class gear by checking vnum ranges.

### Phase 5: Presentation and Polish

17. **Add who list brackets and titles** - In `act_info.c`, add `openb`/`closeb` with your color scheme, generation-based titles, and room aura tags.
18. **Add self class announcements** - Base classes need entries in two places:
    - `wizutil.c:do_classself()` - Add a case for your class so players can select it via `selfclass`. Include a themed welcome message.
    - `clan.c:do_class()` - Add a case so immortals can assign the class via `class <player> <classname>`. Include a "You are now a ..." message.
19. **Create help file entries** - Update `base_help.db` with the class help entry, and add the class to the CLASSES help entry.
20. **Write the design doc** - Document everything in `game/docs/design/playerclass/classes/<class>.md`.
21. **Update overview docs** - Add to `overview.md` and `README.md`.

### Phase 6: Upgrade Class (if applicable)

22. **Add upgrade path in `upgrade.c`** - Map base class to upgrade class in the upgrade switch, and add to `is_upgrade()`.
23. **Verify `selfclass` entry** - Base classes need a selection entry in `wizutil.c:do_classself()`; upgrade classes do not (they are reached via the upgrade system, not selfclass).

## Common Pitfalls

### `pcdata->powers[]` Index Conflicts

Both the base class and its upgrade class share the same `pcdata->powers[]` array. Since a character can only be one class at a time, it is safe to reuse indices between a base/upgrade pair. However, **ability state indices** (0-9) and **training indices** (10+) should be kept separate from each other.

Example from Dirgesinger/Siren:
- Indices 0-8: Ability state (buff durations, DOT stacks, echoshield, crescendo)
- Indices 10-13: Training categories (reused between both classes)

### The `race` Field in `cmd_table`

The `race` field in `cmd_table[]` entries is a class restriction bitmask, not a "race." If set to `CLASS_DIRGESINGER`, only Dirgesingers can use the command. If set to 0, anyone can use it. A common mistake is forgetting to set this, which makes class abilities available to everyone.

### `visible_strlen()` and Color Codes

The who list formatting in `act_info.c` uses `visible_strlen()` to calculate display width for alignment. This function correctly handles standard `#X` codes (2 chars) and `#xNNN` codes (5 chars). If you use other non-standard color sequences, verify alignment still works.

### `acfg` Default Values

Every `acfg("key")` call must have a corresponding default value in `db_sql.h`. If a key has no default and no entry in `game.db`, `acfg()` returns 0 silently. This can cause abilities to cost 0 mana, deal 0 damage, or have 0-tick cooldowns, which can be difficult to diagnose.

Pattern for adding defaults:
```c
// In db_sql.h, within the CREATE/INSERT block:
"INSERT OR IGNORE INTO config VALUES('classname.ability.mana_cost', 50);\n"
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

### Training Category Design

Group abilities into 2-4 training categories of 2-4 abilities each. This gives players meaningful progression choices without being overwhelming. Each category should have a thematic identity (offensive, defensive, utility, etc.) so the choice feels distinct.

Training cost formula pattern: `(current_level + 1) * base_cost` where `base_cost` is higher for upgrade classes (50 primal for Siren vs 40 for Dirgesinger).

### Help File Color Codes

Help entries in `base_help.db` support the same color codes as in-game text. Use your class colors in the help entry to give it visual identity. Keep the overall structure consistent with other class help entries: overview paragraph, ability list with short descriptions, key commands section.

### Self Class Announcements

Base classes need entries in both `wizutil.c:do_classself()` and `clan.c:do_class()` so players can select the class themselves and immortals can assign it. Missing these entries means:
- Players can't select the class via `selfclass` (it won't appear in the list or won't work)
- Immortals can't assign the class via `class <player> <classname>`

Each entry should include a themed announcement message that fits the class identity. See existing classes for examples of the messaging style.

## Template: Minimal Class Skeleton

A minimal new base class needs at minimum:

```
class.h         +1 line   (CLASS_FOO define)
merc.h          +N lines  (DECLARE_DO_FUN per ability)
interp.c        +N lines  (cmd_table entries)
act_info.c      +3 blocks (openb/closeb, who titles, room tag)
update.c        +1 line   (tick update call)
fight.c         +1-3 blocks (damcap, defense, extra attacks)
db_sql.h        +N lines  (acfg defaults)
handler.c       +1 block  (equipment restrictions)
wizutil.c       +1 block  (selfclass entry with themed message)
clan.c          +1 block  (do_class entry for immortal assignment)
base_help.db    +1 entry  (class help)
```

An upgrade class additionally needs:
```
upgrade.c       +2 blocks (is_upgrade check, upgrade path mapping)
wizutil.c       no change (upgrade classes aren't self-selectable)
clan.c          +1 block  (do_class entry for immortal assignment)
```
