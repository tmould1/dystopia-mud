# Screen Reader Mode

Screen reader mode reformats game output for compatibility with assistive technology (JAWS, NVDA, VoiceOver, etc.). It strips decorative characters, replaces ASCII art with plain text, and restructures tabular data into labeled lines.

## Enabling

### During Character Creation

Screen reader mode is offered as part of the display mode prompt during character creation:

```
Choose your display mode:
  [N]one  - Plain text, no colors
  [A]NSI  - Standard 16 colors (works with most clients)
  [X]term - Full 256 colors (modern clients only)
  [S]creen reader - Optimized for JAWS, NVDA, VoiceOver
Your choice (N/A/X/S)?
```

Choosing S enables screen reader mode with all associated settings (no colors, brief, blank lines, auto-exits). No separate color prompt is needed.

**Location:** [nanny.c — CON_GET_NEW_ANSI](../../src/core/nanny.c)

### In-Game Toggle

```
screenreader       -- toggle on/off
config screenrd    -- alias toggle
```

When enabled, the following settings are automatically applied:

| Setting | Value | Reason |
|---------|-------|--------|
| `PLR_ANSI` | OFF | ANSI escape codes confuse screen readers |
| `PLR_BLANK` | ON | Blank lines between paragraphs aid navigation |
| `PLR_AUTOEXIT` | ON | Always announce available exits |

These can be individually overridden via `config` after enabling screen reader mode.

**Location:** [act_info.c — do_screenreader()](../../src/commands/act_info.c)

## Output Changes

### Score

**Location:** [act_info.c — do_score()](../../src/commands/act_info.c)

Normal output uses bracket-wrapped colored stats:
```
#R[#C500#n/#C500 hp#R]#n #R[#C300#n/#C300 mana#R]#n
```

Screen reader output uses labeled lines:
```
Hit Points: 500/500
Mana: 300/300
Move: 200/200
Level: 50
Class: Vampire
Generation: 3
...
```

### Exits

**Location:** [act_info.c — do_exits()](../../src/commands/act_info.c)

Normal: `#R[#GExits#7:#C North South East#R]#n`

Screen reader: `Exits: north, south, east`

### Who List

**Location:** [act_info.c — do_who()](../../src/commands/act_info.c)

Normal output uses decorative banners (`make_who_banner()`) and class-specific ASCII bracket pairs (`#R<<`, `#y((`, etc.) around class names.

Screen reader output:
- Replaces banners with plain text section headers: `Gods:`, `Avatars:`, `Mortals:`
- Removes decorative bracket pairs around class names
- Uses plain text player count: `3 of 5 visible players and 1 visible immortals connected.`

### Equipment

**Location:** [act_info.c — do_equipment()](../../src/commands/act_info.c)

Normal: `#R[#CLight#R]#n          a glowing orb`

Screen reader: `Light: a glowing orb`

Uses `where_name_sr[]` array — plain text slot names without color codes or bracket decorations.

### Weapon/Skill List

**Location:** [act_info.c — do_weaplist()](../../src/commands/act_info.c)

Normal output uses column borders and ASCII decorations. Screen reader output uses simple labeled lines:
```
Unarmed: 500
Slice: 300
Stab: 200
...
```

### Combat Damage Messages

**Location:** [fight.c — dam_message()](../../src/combat/fight.c)

Replaces ASCII art damage descriptors with plain text equivalents:

| Normal | Screen Reader |
|--------|---------------|
| `#G<#y*#L{#R*#L}#y*#G> extracting organs` | `, extracting organs!` |
| `#R()#G()#R() Humiliatingly Hard` | `, humiliatingly hard!` |

### General Output Filtering

**Location:** [comm.c — write_to_buffer()](../../src/core/comm.c)

When screen reader mode is active, `write_to_buffer()` collapses multiple consecutive spaces to a single space. This cleans up padded table layouts and alignment spaces that are meaningless to screen readers.

This runs after ANSI color stripping (already handled by the existing color code path when `PLR_ANSI` is off).

## Config Display

The `config` command shows screen reader and MCMP status alongside other protocol toggles:

```
SCREENRD: ON     MCMP: ON
```
