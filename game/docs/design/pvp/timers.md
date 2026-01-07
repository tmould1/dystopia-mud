# PK Timers and Cooldowns

This document details the timer systems that regulate PvP engagement frequency.

## Fight Timer

The fight timer prevents immediate re-engagement after PvP combat.

### Implementation

**Location:** [fight.c](../../src/combat/fight.c)

| Action | Line | Behavior |
|--------|------|----------|
| Decrement | [113](../../src/combat/fight.c#L113) | Timer decremented each combat round |
| Set on PvP | [194-195](../../src/combat/fight.c#L194-L195) | Set to minimum 10, increases by 3 per round (max 25) |
| Check | [2289](../../src/combat/fight.c#L2289) | Cannot attack player with `fight_timer > 0` |

### Behavior

- When two players engage in combat, both receive a fight timer
- Timer starts at 10 ticks minimum
- Each combat round adds 3 ticks (capped at 25)
- A player with an active fight timer cannot be attacked by others
- This prevents "dog-piling" on players already in combat

## Safety Counter (Post-Avatar Protection)

New avatars receive temporary protection from PK.

### Setting the Counter

**Location:** [act_move.c:3293-3294](../../src/commands/act_move.c#L3293-L3294)

| Condition | Duration |
|-----------|----------|
| Normal circumstances | 10 ticks |
| During ragnarok | 3 ticks |

### Counter Decrement

**Location:** [update.c:1349](../../src/systems/update.c#L1349)

The `update_sit_safe_counter()` function decrements the counter each tick.

### Enforcement

**Location:** [fight.c:2219-2228](../../src/combat/fight.c#L2219-L2228)

Players with `safe_counter > 0` cannot be attacked. The attacker receives an error message.

## Timer Command

Players can check active timers with the `timer` command.

**Location:** [jobo_act.c:1253-1286](../../src/systems/jobo_act.c#L1253-L1286)

Displays countdowns for:
- Arena opening
- Ragnarok activation
- Happy hour
- Questor's delight

## Summary Table

| Timer | Purpose | Duration | Location |
|-------|---------|----------|----------|
| Fight Timer | Prevents re-engagement | 10-25 ticks | [fight.c:194](../../src/combat/fight.c#L194) |
| Safety Counter | New avatar protection | 3-10 ticks | [act_move.c:3293](../../src/commands/act_move.c#L3293) |
| Ragnarok On | Event duration | PULSE_RAGNAROK (15 min) | [jobo_act.c:1239](../../src/systems/jobo_act.c#L1239) |
| Ragnarok Safe | Cooldown after event | 120 minutes | [jobo_act.c:1215](../../src/systems/jobo_act.c#L1215) |
