# PK Protections and Restrictions

This document details all the systems that prevent or restrict PvP combat.

## is_safe() Function

**Location:** [fight.c:2205-2323](../../src/combat/fight.c#L2205-L2323)

This is the master function that checks if PK is allowed between two characters. Returns TRUE if combat should be prevented.

## Protection Types

### 1. Avatar Requirement

**Location:** [fight.c:2211-2215](../../src/combat/fight.c#L2211-L2215)

Both players must satisfy `CAN_PK()` macro (trust 3-12).

**Error message:** "Both players must be avatars to fight."

### 2. Safety Counter

**Location:** [fight.c:2219-2228](../../src/combat/fight.c#L2219-L2228)

Characters with `safe_counter > 0` cannot be attacked. This protects new avatars.

### 3. Ethereal State

**Location:** [fight.c:2238-2247](../../src/combat/fight.c#L2238-L2247)

Characters who are ethereal cannot engage in PK. Both attacker and defender checked.

### 4. Shadow Plane

**Location:** [fight.c:2248-2257](../../src/combat/fight.c#L2248-L2257)

Characters in the shadow plane cannot engage in PK. Both attacker and defender checked.

### 5. Decapitated State

**Location:** [fight.c:2258-2267](../../src/combat/fight.c#L2258-L2267)

Decapitated characters (head = `LOST_HEAD`) cannot engage in PK.

### 6. AFK Protection

**Location:** [fight.c:2268-2272](../../src/combat/fight.c#L2268-L2272)

Players marked as AFK cannot be attacked.

### 7. Linkdead Protection

**Location:** [fight.c:2273-2278](../../src/combat/fight.c#L2273-L2278)

Players without an active connection (linkdead) cannot be attacked.

### 8. Ragnarok Class Restrictions

**Location:** [fight.c:2279-2288](../../src/combat/fight.c#L2279-L2288)

During ragnarok:
- Upgrades cannot attack non-upgrades
- Non-upgrades cannot attack upgrades

Prevents tier mismatches during the global PvP event.

### 9. Fight Timer

**Location:** [fight.c:2289](../../src/combat/fight.c#L2289)

Characters with `fight_timer > 0` cannot be attacked by new opponents.

### 10. Safe Rooms

**Location:** [fight.c:2294-2298](../../src/combat/fight.c#L2294-L2298)

Rooms with `ROOM_SAFE` flag (1024) prevent all PvP combat.

**Exception:** Safe rooms do NOT protect during ragnarok events.

### 11. Peace Spell

**Location:** [fight.c:2302-2310](../../src/combat/fight.c#L2302-L2310)

Characters affected by `AFF_PEACE` cannot initiate or be targeted for PK.

### 12. Peace Item

**Location:** [fight.c:2312-2321](../../src/combat/fight.c#L2312-L2321)

Characters with `ITEMA_PEACE` item affect cannot initiate or be targeted for PK.

## Decapitation Restrictions

Additional checks in `behead()` prevent unfair decapitations:

### Newbie Protection

**Location:** [fight.c:5628-5629](../../src/combat/fight.c#L5628-L5629)

Newbies (age < 2) cannot decapitate or be decapitated.

**Check:** `(get_age(ch) - 17) < 2`

### Fair Fight System

**Location:** [fight.c:5620-5649](../../src/combat/fight.c#L5620-L5649)

Function: `fair_fight()`

Prevents decapitation with massive power imbalances:
- Both players must have minimum 150 might
- Power ratio checks based on attacker might:
  - If attacker < 1000 might: must be within 80% of defender
  - Higher scaling for more powerful characters

## Room Flag Reference

| Flag | Value | Line | Effect |
|------|-------|------|--------|
| `ROOM_SAFE` | 1024 | [merc.h:1611](../../src/core/merc.h#L1611) | No combat allowed |
| `ROOM_ARENA` | 131072 | [merc.h:1618](../../src/core/merc.h#L1618) | Arena zone marker |

## Affect Flag Reference

| Flag | Line | Effect |
|------|------|--------|
| `AFF_PEACE` | [merc.h:957](../../src/core/merc.h#L957) | Spell preventing PK |
| `ITEMA_PEACE` | [merc.h:985](../../src/core/merc.h#L985) | Item preventing PK |

## Summary Table

| Protection | Prevents | Can Override |
|------------|----------|--------------|
| Non-avatar | All PK | Become avatar |
| Safety counter | Being attacked | Wait for expiry |
| Ethereal | All PK | Leave ethereal |
| Shadow plane | All PK | Leave shadow |
| Decapitated | All PK | Resurrect |
| AFK | Being attacked | Return from AFK |
| Linkdead | Being attacked | Reconnect |
| Fight timer | Being attacked | Wait for expiry |
| Safe room | All PK | Leave room (or ragnarok) |
| Peace spell | All PK | Wait for expiry |
| Peace item | All PK | Remove item |
| Newbie | Decapitation | Reach age 2 |
| Might imbalance | Decapitation | Increase might |
