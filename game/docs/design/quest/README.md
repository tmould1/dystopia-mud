# Quest System

The quest system provides two distinct features for player progression and item customization.

## Subsystems

### 1. [Item Customization](item-customization.md)
Spend **quest points (QP)** to create and enhance equipment. Create custom weapons, armor, and special items with stat bonuses, spell effects, and unique properties.

### 2. [Quest Cards](quest-cards.md)
A collection minigame where players create quest cards and hunt for specific items to complete them. Cards are created using primal energy and require collecting 4 randomly-selected items.

## Command Quick Reference

### Item Customization
```
quest                              Show cost menu
quest create <type> [subtype]      Create new object (10-50 QP)
quest <item> <property> <value>    Modify existing item
```

### Quest Cards
```
cast 'quest'                       Create quest card (costs primal)
complete <card>                    Show required items
complete <card> <item>             Submit item to card
```

## Quest Point Costs

| Modification | Max | Cost |
|-------------|-----|------|
| Str/Dex/Int/Wis/Con | +3 | 20 QP per point |
| HP/Mana/Move | +25 | 5 QP per point |
| Hitroll/Damroll | +5 | 30 QP per point |
| AC | -25 | 10 QP per point |
| Spell weapon/affect | - | 50 QP |
| Wear location | - | 20 QP |
| Transporter | - | 50 QP |
| Weight (to 1) | - | 10 QP |
| Name/Short/Long | - | 1 QP total |

## Player Data Fields

Quest-related player data stored in `CHAR_DATA->pcdata`:

| Field | Purpose |
|-------|---------|
| `quest` | Current quest points available to spend |
| `questsrun` | Number of quests completed |
| `questtotal` | Lifetime quest points earned |

## Source Files

| File | Lines | Purpose |
|------|-------|---------|
| [kav_wiz.c](../../src/commands/kav_wiz.c) | 1351-3774 | `do_quest()`, `oset_affect()` |
| [act_obj.c](../../src/commands/act_obj.c) | 3677-3883 | `quest_object()`, `do_complete()` |
| [magic.c](../../src/combat/magic.c) | 4439-4468 | `spell_quest()` |
| [merc.h](../../src/core/merc.h) | 1500-1527 | QUEST_* flags |
