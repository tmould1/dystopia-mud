# Shapeshifter Class Design

## Overview

Shapeshifters are powerful creatures able to take any form they wish. They are an **upgrade class** obtained by upgrading from Werewolf.

**Source File**: `src/shapeshifter.c`
**Class Constant**: `CLASS_SHAPESHIFTER` (512)
**Upgrades From**: Werewolf

## Lore (from help.are)

> Shapeshifters are powerful creatures able to take any form they wish. No one knows what a shapechanger really looks like, so how can you know that your best friend isn't one... better kill him to be on the safe side.

## Commands (from help.are)

| Command | Description |
|---------|-------------|
| shift | Change between beast forms |
| truesight | Gain superior vision |
| formlearn | Learn more about different beast forms |
| mask | Mask yourself as another player |
| shapeshift | Change into ANYTHING |
| hatform | Why pull rabbits out of a hat, when you can be a hat instead |
| camouflage | Camouflage your items |
| mistwalk | Class portal |
| shapearmor | Make class equipment |

## Beast Forms

Shapeshifters can shift into various beast forms, each with unique abilities:

### Tiger Form
- roar
- phase

### Faerie Form
- fblink
- faeriecurse

### Hydra Form
- breath

### Bull Form
- charge
- stomp

## Key Abilities

### Shift
Change between available beast forms, each granting different abilities and combat bonuses.

### Shapeshift
Unlike basic shift, this allows changing into ANYTHING - far more versatile transformation.

### Mask
Disguise yourself as another player - perfect for infiltration.

### Hatform
A humorous ability that turns you into a hat.

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_SHAPESHIFTER bit |
| form | ch->form | Current beast form |
| polyaff | ch->polyaff | Polymorphic affinity bits |

## Notes

- Shapeshifters are a unique class made for Dystopia
- The Malaugrym connection (from upgrade message: "reverts to the true form of the Malaugrym")
- Malaugrym are the ancient shapeshifting beings from the plane of shadows
- Upgrade classes can further upgrade (levels 2-5) for additional combat bonuses
