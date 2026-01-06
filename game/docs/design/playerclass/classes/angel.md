# Angel Class Design

## Overview

Angels are God's avengers, sent to punish the wicked. They are an **upgrade class** obtained by upgrading from Monk.

**Source Files**: `src/classes/angel.c`, `src/classes/angel.h`
**Class Constant**: `CLASS_ANGEL` (2048)
**Upgrades From**: Monk

## Core Mechanics

### Power Paths

Angels have four power paths stored in `ch->pcdata->powers[]`:

| Index | Path | Max Level | Purpose |
|-------|------|-----------|---------|
| ANGEL_PEACE (1) | Peace | 5 | Protection, spirit form |
| ANGEL_LOVE (2) | Love | 5 | Healing, forgiveness |
| ANGEL_JUSTICE (3) | Justice | 5 | Combat, punishment |
| ANGEL_HARMONY (4) | Harmony | 5 | Auras, balance attacks |

### Toggle Flags (ANGEL_POWERS)

Stored in `ch->pcdata->powers[5]`:

| Flag | Value | Effect |
|------|-------|--------|
| ANGEL_WINGS | 1 | Wings manifested, enables flying |
| ANGEL_HALO | 2 | Halo visible |
| ANGEL_AURA | 4 | Extra heavenlyaura attacks |
| ANGEL_EYE | 8 | Reflect damage back on attacker |

### Combat Bonuses

**Attack Count** (`fight.c:1009`, `fight.c:1087`):
- +ANGEL_JUSTICE attacks per round
- +2 base attacks

**Extra Attacks** (`fight.c:844-847`):
- ANGEL_AURA active: Extra heavenlyaura attack per round
- Fiery: Double fiery attack vs Angels

**Damage Multiplier** (`fight.c:1362`):
- 1 + (ANGEL_JUSTICE / 10) multiplier (minimal effect per comment)

**Damage Reduction** (`fight.c:1621-1625`):
- (100 - ANGEL_HARMONY * 12)% damage taken
- At HARMONY 5: 40% damage reduction

**Damage Cap Bonus** (`fight.c:1788-1791`):
- +125 * ANGEL_JUSTICE max damage

**Dodge/Parry Bonus** (`fight.c:2420`, `fight.c:2453`, `fight.c:2497-2498`, `fight.c:2646`, `fight.c:2681`, `fight.c:2725-2726`):
- -9% enemy hit chance per ANGEL_JUSTICE level (dodge)
- +9% dodge bonus from ANGEL_PEACE (victim)
- -3% per ANGEL_JUSTICE level (parry)

**Eye for an Eye** (`fight.c:2088-2089`):
- When ANGEL_EYE active, reflects damage back on attacker

**Flight** (`fight.c:3944-3945`):
- ANGEL_WINGS: Immune to fall damage

## Peace Path (ANGEL_PEACE)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 1 | `gpeace` | Toggle AFF_PEACE - cannot be attacked | `angel.c:60-85` |
| 2 | `spiritform` | Toggle AFF_ETHEREAL - become spirit | `angel.c:33-58` |
| 3 | `innerpeace` | Heal (PEACE * 500) HP for 1500 mana | `angel.c:87-112` |
| 5 | `houseofgod` | Stop all fighting in room (50-tick cooldown) | `angel.c:114-141` |

## Love Path (ANGEL_LOVE)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 1 | `gsenses` | Toggle PLR_HOLYLIGHT truesight | `angel.c:274-298` |
| 2 | `gfavor` | Toggle +400 hit/damroll, angel form (2000 mana/move) | `angel.c:300-349` |
| 3 | `forgiveness <target>` | Heal good-aligned PC 1000-1500 HP | `angel.c:351-402` |
| 4 | (passive) | 2x werewolf_regen + limb regen | `update.c:1638-1643` |
| 5 | `martyr` | Restore all PCs in room, set self to 1 HP/mana/move | `angel.c:404-436` |

## Justice Path (ANGEL_JUSTICE)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 1 | `awings` | Toggle wings (enables flying/swoop) | `angel.c:550-574` |
| 1 | `swoop <target>` | Fly to target (500 move, requires wings) | `angel.c:438-496` |
| 2 | `halo` | Toggle halo visibility | `angel.c:576-600` |
| 3 | `sinsofthepast <target>` | Fire + poison + wrathofgod attack | `angel.c:602-649` |
| 4 | `touchofgod <target>` | 100-200 damage, 33% stun | `angel.c:498-548` |
| 5 | `eyeforaneye` | Toggle damage reflection mode | `angel.c:651-675` |

## Harmony Path (ANGEL_HARMONY)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 2 | `angelicaura` | Toggle extra heavenlyaura attacks | `angel.c:143-167` |
| 3 | `gbanish <target>` | Alignment damage, 30% teleport to Hell | `angel.c:169-241` |
| 5 | `harmony <target>` | Cast spirit kiss spell | `angel.c:243-272` |

### Gbanish Details (`angel.c:169-241`)

Damage based on target alignment:
- Neutral (>0): 500 damage
- Slightly evil (>-500): 1000 damage
- Evil (<=-500): 1500 damage
- Cannot target good (>500)
- 30% chance to teleport to Hell

## Support Commands

| Command | Effect | Code |
|---------|--------|------|
| `angelicarmor <piece>` | Create angel equipment (150 primal) | `angel.c:677-731` |

## Angel Armor

Create equipment via `angelicarmor <piece>` (`angel.c:677-731`):

**Cost**: 150 primal per piece

**Available pieces** (vnums 33180-33192): sword, bracer, necklace, ring, plate, helmet, leggings, boots, gauntlets, sleeves, cloak, belt, visor

## Regeneration

**Love Path Regen** (`update.c:1638-1643`):
- ANGEL_LOVE 4+: 2x werewolf_regen + limb regen

**Home rooms** (vnum 93340-93349): Additional 1x regen bonus (`update.c:1645-1649`)

**No inherent regen** without LOVE 4+. Relies on ITEMA_REGENERATE artifacts.

## Class Interactions

**Takes extra damage from** (`fight.c:800-801`):
- Tanarri FIERY: Double fiery attack

**Deals reduced damage to** (`fight.c:1629`):
- vs Tanarri: 75% damage (trained to kill Angels)

**Charging** (`fight.c:4016-4021`):
- Angels have special swoop charge attack

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_ANGEL bit |
| peace | ch->pcdata->powers[1] | Peace path level (0-5) |
| love | ch->pcdata->powers[2] | Love path level (0-5) |
| justice | ch->pcdata->powers[3] | Justice path level (0-5) |
| harmony | ch->pcdata->powers[4] | Harmony path level (0-5) |
| toggles | ch->pcdata->powers[5] | WINGS/HALO/AURA/EYE bits |
| cooldown | ch->pcdata->powers[6] | Houseofgod cooldown |
| cubeform | ch->newbits | NEW_CUBEFORM for gfavor form |
