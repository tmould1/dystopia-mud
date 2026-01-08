# Mob Special Functions (spec_fun)

Special functions are hardcoded C procedures that give mobs custom AI behaviors. Unlike scripted MobProgs, these are compiled into the server.

## Overview

Source: [special.c](../../src/combat/special.c)

Special functions are called during the mob AI update loop in [update.c:594-596](../../src/systems/update.c#L594-L596):

```c
if ( ch->spec_fun != 0 )
{
    if ( (*ch->spec_fun) ( ch ) ) continue;
}
```

## Data Structures

### Type Definition

From [merc.h:151](../../src/core/merc.h#L151):
```c
typedef bool SPEC_FUN args( ( CHAR_DATA *ch ) );
```

### Storage

- `MOB_INDEX_DATA->spec_fun` - Prototype (template) special function
- `CHAR_DATA->spec_fun` - Instance special function (copied from prototype)

### Lookup Table

The `spec_table[]` array maps names to function pointers.
Defined in [special.c:2760-2804](../../src/combat/special.c#L2760-L2804):

```c
const struct spec_type spec_table[] =
{
    { "spec_breath_any",        spec_breath_any         },
    { "spec_breath_acid",       spec_breath_acid        },
    // ... etc
    { "",                       0                       }
};
```

---

## Available Special Functions

### Breath Attacks

| Function | Line | Description |
|----------|------|-------------|
| spec_breath_any | [722](../../src/combat/special.c#L722) | Random breath type |
| spec_breath_acid | [744](../../src/combat/special.c#L744) | Acid breath attack |
| spec_breath_fire | [751](../../src/combat/special.c#L751) | Fire breath attack |
| spec_breath_frost | [758](../../src/combat/special.c#L758) | Frost breath attack |
| spec_breath_gas | [765](../../src/combat/special.c#L765) | Gas breath attack |
| spec_breath_lightning | [780](../../src/combat/special.c#L780) | Lightning breath attack |

### Spellcasters

| Function | Line | Description |
|----------|------|-------------|
| spec_cast_adept | [787](../../src/combat/special.c#L787) | Healer/buffer NPC (bless, armor, cure) |
| spec_cast_cleric | [847](../../src/combat/special.c#L847) | Cleric combat spells |
| spec_cast_judge | [899](../../src/combat/special.c#L899) | Judge combat spells |
| spec_cast_mage | [928](../../src/combat/special.c#L928) | Mage combat spells |
| spec_cast_undead | [979](../../src/combat/special.c#L979) | Undead combat spells |

### Behaviors

| Function | Line | Description |
|----------|------|-------------|
| spec_fido | [1028](../../src/combat/special.c#L1028) | Eats corpses |
| spec_guard | [1060](../../src/combat/special.c#L1060) | Attacks criminals/wanted players |
| spec_janitor | [1143](../../src/combat/special.c#L1143) | Picks up trash/objects |
| spec_mayor | [1172](../../src/combat/special.c#L1172) | Walks patrol route, opens/closes gates |
| spec_poison | [1274](../../src/combat/special.c#L1274) | Poisons in combat |
| spec_thief | [1292](../../src/combat/special.c#L1292) | Steals gold from players |

### Monsters

| Function | Line | Description |
|----------|------|-------------|
| spec_eater | [1332](../../src/combat/special.c#L1332) | Eats players on death |
| spec_gremlin_original | [1363](../../src/combat/special.c#L1363) | Gremlin behavior (multiplies) |
| spec_gremlin_born | [1475](../../src/combat/special.c#L1475) | Spawned gremlin behavior |
| spec_rogue | [1781](../../src/combat/special.c#L1781) | Rogue combat AI |
| spec_zombie_lord | [2047](../../src/combat/special.c#L2047) | Summons zombies |
| spec_dog | [2520](../../src/combat/special.c#L2520) | Dog pet behavior |

### Class Guards

Guards that protect specific class headquarters:

| Function | Line | Description |
|----------|------|-------------|
| spec_guard_werewolf | [167](../../src/combat/special.c#L167) | Werewolf HQ guard |
| spec_guard_dragon | [225](../../src/combat/special.c#L225) | Dragon HQ guard |
| spec_guard_vampire | [399](../../src/combat/special.c#L399) | Vampire HQ guard |
| spec_guard_mage | [283](../../src/combat/special.c#L283) | Mage HQ guard |
| spec_guard_demon | [457](../../src/combat/special.c#L457) | Demon HQ guard |
| spec_guard_drow | [515](../../src/combat/special.c#L515) | Drow HQ guard |
| spec_guard_ninja | [631](../../src/combat/special.c#L631) | Ninja HQ guard |
| spec_guard_monk | [573](../../src/combat/special.c#L573) | Monk HQ guard |
| spec_guard_highlander | [341](../../src/combat/special.c#L341) | Highlander HQ guard |
| spec_guard_cyborg | [1988](../../src/combat/special.c#L1988) | Cyborg HQ guard |
| spec_clan_guardian | [1912](../../src/combat/special.c#L1912) | Clan hall guard |

### Combat

| Function | Line | Description |
|----------|------|-------------|
| spec_assassin | [123](../../src/combat/special.c#L123) | Assassin combat AI |
| spec_vladd_decap | [71](../../src/combat/special.c#L71) | Decapitates dying players |
| spec_executioner | [1612](../../src/combat/special.c#L1612) | Executes criminals |

---

## OLC Editing

### Setting via medit

In mobile editor, use the `spec` command:

```
medit spec spec_cast_mage    (set special function)
medit spec none              (remove special function)
```

Implementation: [olc_act.c medit_spec()](../../src/world/olc_act.c)

### Lookup Functions

From [special.c:2812-2837](../../src/combat/special.c#L2812-L2837):

```c
SPEC_FUN *spec_lookup( const char *name )   // Name -> function pointer
char *spec_string( SPEC_FUN *fun )          // Function pointer -> name
```

---

## Area File Format

Specials are stored in the `#SPECIALS` section of .are files:

```
#SPECIALS
M 3000 spec_cast_mage
M 3005 spec_thief
M 3011 spec_executioner
S
```

Format: `M <mob_vnum> <spec_name>`

### Loading

Loaded by `load_specials()` in [db.c:1743-1775](../../src/core/db.c#L1743-L1775):
```c
pMobIndex->spec_fun = spec_lookup(fread_word(fp));
```

### Saving

Saved by `save_specials()` in [olc_save.c:330-351](../../src/world/olc_save.c#L330-L351).

---

## Runtime Assignment

The `mset` wizard command can set spec_fun at runtime:

```
mset <mob> spec <spec_name>
```

From [act_wiz.c:4246](../../src/commands/act_wiz.c#L4246):
```c
victim->spec_fun = spec_lookup(arg3);
```

---

## Writing New Special Functions

To add a new special function:

1. Declare in [special.c](../../src/combat/special.c) header area:
   ```c
   DECLARE_SPEC_FUN( spec_my_function );
   ```

2. Implement the function:
   ```c
   bool spec_my_function( CHAR_DATA *ch )
   {
       // Return TRUE to skip normal AI this tick
       // Return FALSE to continue normal AI
       return FALSE;
   }
   ```

3. Add to `spec_table[]`:
   ```c
   { "spec_my_function", spec_my_function },
   ```

4. Recompile the server.
