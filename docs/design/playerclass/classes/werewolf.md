# Werewolf Class Design

## Overview

Werewolves (also called Garou) are shapechangers who draw power from primal totems and the moon. They use Gnosis as their primary resource and can learn Gifts from various spirit totems.

**Source Files**: `src/ww.c`, `src/garou.h`
**Class Constant**: `CLASS_WEREWOLF` (4)

## Core Mechanics

### Gnosis

Gnosis is the spiritual energy that powers werewolf abilities:

```c
ch->gnosis[GCURRENT]   // Current gnosis points
ch->gnosis[GMAXIMUM]   // Maximum gnosis pool
```

- Regenerates over time
- Spent to activate gifts and abilities
- Maximum increases with rank/experience

### Forms

Werewolves can shift between multiple forms:

```c
ch->form      // Current form ID
ch->cur_form  // Alternative form tracking
```

| Form | Description |
|------|-------------|
| Homid | Human form |
| Glabro | Near-human (larger, hairier) |
| Crinos | War form (full werewolf) |
| Hispo | Dire wolf |
| Lupus | Wolf form |

### Gifts

Special abilities granted by spirit totems:

```c
ch->gifts[21]  // Array of gift levels
```

Gifts are separate from disciplines and represent tribal/totem powers.

## Disciplines (Totems)

Werewolves have 12 totem disciplines:

| Totem | Index | Theme |
|-------|-------|-------|
| Bear | DISC_WERE_BEAR | Strength, endurance |
| Lynx | DISC_WERE_LYNX | Stealth, hunting |
| Boar | DISC_WERE_BOAR | Ferocity, charging |
| Owl | DISC_WERE_OWL | Wisdom, night vision |
| Spider | DISC_WERE_SPID | Weaving, patience |
| Wolf | DISC_WERE_WOLF | Pack tactics, loyalty |
| Hawk | DISC_WERE_HAWK | Vision, speed |
| Mantis | DISC_WERE_MANT | Precision, martial arts |
| Raptor | DISC_WERE_RAPT | Predation, ferocity |
| Luna | DISC_WERE_LUNA | Moon powers |
| Pain | DISC_WERE_PAIN | Pain tolerance |
| Congregation | DISC_WERE_CONG | Pack bonding |

## Key Commands

### Form Shifting
- `shift <form>` - Change to specified form
- `crinos` - Shift to war form
- `lupus` - Shift to wolf form
- `homid` - Return to human form

### Combat
- `frenzy` - Enter rage-fueled combat state
- `bite` - Powerful bite attack
- `claw` - Claw attack
- `howl` - Various howl effects

### Gifts
- `gifts` - View available gifts
- `learn <gift>` - Learn a new gift
- `<giftname>` - Activate a gift

### Pack
- `tribe` - View tribe members (Tribe of Gaia)
- `pack` - Pack management commands

## Form Statistics

Each form provides different bonuses:

| Form | Strength | Dexterity | Appearance | Special |
|------|----------|-----------|------------|---------|
| Homid | Normal | Normal | Normal | Can use tools |
| Glabro | +2 | +1 | -1 | Partial claws |
| Crinos | +4 | +2 | - | Full combat form |
| Hispo | +3 | +3 | - | Fast movement |
| Lupus | +1 | +4 | - | Best tracking |

## Rage System

```c
ch->rage  // Current rage level
```

- Rage builds during combat
- High rage can trigger frenzy
- Frenzy = uncontrolled aggression
- Luna discipline affects rage

## Tribal Organization

Werewolves belong to the "Tribe of Gaia":

```c
// Check tribe membership
void do_church(CHAR_DATA *ch, char *argument) {
    // Lists all online werewolves
    // Shows tribal hierarchy
}
```

## Combat Bonuses

### Form-Based
```c
// Crinos form bonuses
if (ch->form == FORM_CRINOS) {
    damage *= 1.5;  // Increased damage
    // Natural armor bonus
}
```

### Totem-Based
```c
// Bear totem adds strength
str_bonus = ch->power[DISC_WERE_BEAR] * 2;

// Lynx adds to stealth
stealth_bonus = ch->power[DISC_WERE_LYNX] * 10;
```

## Regeneration

Werewolves have enhanced healing:

```c
// Regeneration rate modified by form
if (IS_CLASS(ch, CLASS_WEREWOLF)) {
    heal_rate *= 2;  // Double normal healing
    if (ch->form == FORM_CRINOS)
        heal_rate *= 1.5;  // Even faster in war form
}
```

## Weaknesses

- **Silver**: Vulnerable to silver weapons
- **Wolfsbane**: Poisonous plant
- **Full Moon**: May trigger involuntary shift
- **Frenzy**: Risk of losing control

## Gift System Details

Gifts are learned from spirit totems:

```c
// Learning a gift
ch->gifts[gift_index] = level;

// Using a gift
if (ch->gifts[GIFT_RAZOR_CLAWS] >= required_level) {
    // Execute gift ability
    ch->gnosis[GCURRENT] -= gnosis_cost;
}
```

### Gift Categories

| Category | Source | Theme |
|----------|--------|-------|
| Ragabash | Moon | Stealth, trickery |
| Theurge | Spirits | Spirit interaction |
| Philodox | Law | Judgment, truth |
| Galliard | Moon | Inspiration, tales |
| Ahroun | War | Combat, rage |

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_WEREWOLF bit |
| disciplines | ch->power[] | Totem levels |
| gifts | ch->gifts[] | Gift levels |
| gnosis | ch->gnosis[2] | Current/max gnosis |
| form | ch->form | Current form |
| rage | ch->rage | Rage meter |
| wolfform | ch->pcdata->wolfform[2] | Form tracking |
