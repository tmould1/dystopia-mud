# Game Update / Tick System Overview

The game loop runs at 4 pulses per second (250ms per pulse). A central dispatcher fires different update functions on staggered schedules — combat every 3 seconds, NPC AI every 4 seconds, and the main character/weather tick on a randomized 15-45 second cycle to prevent tick-timing exploits.

## Pulse Constants

**Location:** [merc.h:250-265](../../src/core/merc.h#L250-L265)

| Constant | Pulses | Real Time | Purpose |
|----------|--------|-----------|---------|
| `PULSE_PER_SECOND` | 4 | 250ms each | Base timing unit |
| `PULSE_VIOLENCE` | 12 | 3 seconds | Combat rounds |
| `PULSE_MOBILE` | 16 | 4 seconds | NPC AI + class updates |
| `PULSE_PLAYERS` | 16 | 4 seconds | Player-specific updates |
| `PULSE_EMBRACE` | 16 | 4 seconds | Vampire feeding |
| `PULSE_WW` | 16 | 4 seconds | Werewolf blade barrier |
| `PULSE_TICK` | 120 | 30 seconds | Main tick (base, randomized in practice) |
| `PULSE_AREA` | 240 | 60 seconds | Area updates (base, randomized) |
| `PULSE_MINUTE` | 240 | 60 seconds | Event timers |
| `PULSE_DB_DUMP` | 7200 | 30 minutes | Database persistence |

## Game Loop

**Location:** [comm.c:510-728](../../src/core/comm.c#L510-L728) — `game_loop()`

Each iteration (250ms):

```
game_loop()
  ├─ Poll descriptors (select)
  ├─ Accept new connections
  ├─ Process input → command interpreter
  ├─ update_handler()              ← all game updates
  ├─ Process output (prompts, messages)
  └─ Sleep to maintain 4 pulses/sec
```

The loop targets 250ms per iteration and sleeps dynamically if it runs faster.

## update_handler()

**Location:** [update.c:1094-1154](../../src/systems/update.c#L1094-L1154)

The master dispatcher, called once per pulse. Uses static countdown counters that decrement each pulse and fire their function when reaching zero.

### Dispatch Order

| Priority | Counter | Cycle | Function(s) | What It Does |
|----------|---------|-------|-------------|--------------|
| Every pulse | — | 250ms | `recycle_descriptors()` | Cleanup disconnected descriptors |
| 1 | `pulse_violence` | 3s | `violence_update()` | Combat rounds |
| 2 | `pulse_mobile` | 4s | `mobile_update()` | NPC AI, class-specific updates |
| 3 | `pulse_embrace` | 4s | `embrace_update()` | Vampire feeding drain |
| 4 | `pulse_ww` | 4s | `ww_update()` | Werewolf blade barrier damage |
| 5 | `pulse_point` | 15-45s | `weather_update()`, `char_update()`, `obj_update()`, `room_update()` | Main tick |
| 6 | `pulse_area` | 30-90s | `area_update()` | Area time progression |
| 7 | `pulse_minute` | 60s | `update_ragnarok()`, `update_arena()`, `update_doubleexp()`, `update_doubleqps()` | Event timers |

### Anti-Tick-Timing

Two counters use randomized reset values to prevent players from exploiting tick timing:

- `pulse_point` resets to `number_range(PULSE_TICK/2, 3*PULSE_TICK/2)` → **15-45 seconds**
- `pulse_area` resets to `number_range(PULSE_AREA/2, 3*PULSE_AREA/2)` → **30-90 seconds**

All other counters reset to their fixed `PULSE_*` value.

## violence_update()

**Location:** [fight.c:56-219](../../src/combat/fight.c#L56-L219)

Fires every **3 seconds**. Processes one combat round for every character in `char_list` who is fighting.

### Per-Character Steps

1. **Blinky attack** — execute delayed 2-round strike if pending
2. **Decrement timers** — `fight_timer`, demon `rage`
3. **Validate embrace** — check vampire feeding state consistency
4. **Monk effects** — process `MONK_DEATH` damage and `MONK_HEAL` healing
5. **Main attack** — if fighting, awake, and in same room: call `multi_hit()`; otherwise `stop_fighting()`
6. **PvP timer escalation** — PvP combat sets `fight_timer` to minimum 10, increments by 3 up to 25
7. **Group auto-assist** — mounts assist riders; grouped PCs assist each other; NPCs assist same type or 12.5% random chance

See [Combat System](../combat/overview.md) for full combat mechanics.

## mobile_update()

**Location:** [update.c:433-561](../../src/systems/update.c#L433-L561)

Fires every **4 seconds**. Handles NPC behavior and all class-specific player updates.

### NPC Behavior

- Scavenging (pick up items)
- Wandering (random movement)
- Special procedures (`spec_fun` callbacks)

### PC Updates

For each living, non-AFK hero:

1. **Morted timer** — death recovery countdown
2. **Sit-safe counter** — safe room penalty tracking
3. **Drunk counter** — alcohol wearing off
4. **Safe powers** — power upkeep costs for young players
5. **Class-specific update** — dispatched by class (see below)

### Class Update Functions

Each class has a dedicated update function called every 4 seconds from `mobile_update()`:

| Class | Function | Key Updates |
|-------|----------|-------------|
| Vampire | `update_vampire()` | Home room regen, rage management, blood drain |
| Werewolf | `update_werewolf()` | Rage gain in combat (+5-10), auto-transform at 100 rage, 2x regen |
| Demon | `update_demon()` | Domain room 2x regen, warp regen option |
| Angel | `update_angel()` | Love power regen, peace counter, domain regen |
| Monk | `update_monk()` | Chi regen out of combat (25% chance per tick), cloak healing |
| Ninja | `update_ninja()` | Rage management, meditation 3x regen |
| Undead Knight | `update_knight()` | Power tick countdown, domain regen, 2x regen |
| Lich | `update_lich()` | Golem cooldowns, meditation mana gain |
| Shapeshifter | `update_shapeshifter()` | Phase/shape counters, domain regen |
| Tanarri | `update_tanarri()` | Domain regen |
| Drow | `update_drow()` | Domain regen |
| Samurai | `update_highlander()` | Focus countdown, weapon-based regen |
| Mage | `update_mage()` | Meditation mana gain |
| Spider Droid | `update_cyborg()` / `update_drider()` | Cyborg body checks, domain regen |

### Regen Multipliers

**Location:** [update.c:1566-1607](../../src/systems/update.c#L1566-L1607) — `werewolf_regen()`

The shared regen function applies position-based multipliers:

| Position | HP Regen Range |
|----------|---------------|
| Sleeping (bear form) | 1205-1705 |
| Sleeping (normal) | 305-505 |
| Fighting / Standing | 105-205 |

Base multiplier is passed by the class update (1x base, 2x combat, 3x meditation).

## char_update()

**Location:** [update.c:151-426](../../src/systems/update.c#L151-L426)

Fires every **15-45 seconds** (randomized main tick). Handles regeneration, status effects, environmental damage, and timers.

### Processing Order

1. **Ability timers** — decrement `tick_timer[]` array for all cooldowns
2. **Idle check** — 12+ ticks idle → save room; 20+ ticks → force quit
3. **Autosave** — periodic character save
4. **Affect expiration** — decrement spell/effect durations; remove expired affects
5. **Class non-combat updates**:
   - Werewolf gnosis regen (sleeping > resting > standing)
   - Vampire beast trigger when blood <= 15
   - Ninja HaraKiri countdown
   - Monk/Knight cloak restoration
6. **Position recovery** — mortal/stunned/incapacitated characters recover toward standing
7. **Base regeneration** — all characters gain 5-10 HP/mana/move per tick (if above stunned)
8. **Bleeding damage** — per body part: head (20-50), throat/arms/legs (10-20 each)
9. **Status effect damage**:

| Effect | Damage/Tick | Notes |
|--------|-------------|-------|
| `EXTRA_ROT` | 250-500 | Rot curse |
| `AFF_FLAMING` | 250-300 | On fire |
| `AFF_POISON` | 100-200 | 25% chance to clear each tick |

10. **Vampire sunlight** — 5-10 damage outdoors in daylight (2-4 in serpent form); blocked by `AFF_SHADOWPLANE` or `IMM_SUNLIGHT`

## weather_update()

**Location:** [update.c:566-742](../../src/systems/update.c#L566-L742)

Fires as part of the main tick (15-45 seconds). Advances the in-game clock and weather.

### Time Cycle

| Hour | Event | Constant |
|------|-------|----------|
| 5 | Dawn | `SUN_LIGHT` |
| 6 | Sunrise | `SUN_RISE` |
| 19 | Sunset | `SUN_SET` |
| 20 | Night | `SUN_DARK` |
| 24 | Midnight (resets to 0) | — |

- **35 days** per month, **17 months** per year
- At midnight: vampire full heal, Baal spirit expires, room flags (silence, flaming) clear

### Weather System

Atmospheric pressure-based. Sky transitions: `SKY_CLOUDLESS` → `SKY_CLOUDY` → `SKY_RAINING` → `SKY_LIGHTNING`.

## room_update()

**Location:** [update.c:766-848](../../src/systems/update.c#L766-L848)

Fires as part of the main tick. Decrements room-based effect timers:

- Wall timers (ice, fire, sword, prismatic, iron, mushroom, caltrop, ash)
- Cloud timers (stinking cloud, ghost light, swarm bees/bats/rats)
- Silence and protection timers
- Glyph of protection expiration

Uses `RTIMER(room, timer_id)` macro for access ([merc.h:435-445](../../src/core/merc.h#L435-L445)).

## obj_update()

**Location:** [update.c:854-953](../../src/systems/update.c#L854-L953)

Fires as part of the main tick. Handles object decay:

- Corpse decomposition (timer-based)
- Food spoilage
- Daemon seed explosions
- Generic item expiration with type-specific messages

## embrace_update()

**Location:** [update.c:955-1058](../../src/systems/update.c#L955-L1058)

Fires every **4 seconds**. Manages vampire feeding in progress:

- Blood transfer from victim to vampire
- Thirst satiation tracking
- Validates embrace state (same room, both alive)

## Timer Architecture

### Character Timers

**Location:** [merc.h:2228](../../src/core/merc.h#L2228)

```c
sh_int tick_timer[MAX_TIMER];  // ability cooldown array
```

Macros at [merc.h:435-445](../../src/core/merc.h#L435-L445):

| Macro | Usage |
|-------|-------|
| `TIMER(ch, tmr)` | Read timer value |
| `SET_TIMER(ch, tmr, val)` | Set timer |
| `ADD_TIMER(ch, tmr, val)` | Add to timer |
| `SUB_TIMER(ch, tmr, val)` | Subtract from timer |
| `TIME_UP(ch, tmr)` | Check if timer reached 0 |

Timers decrement by 1 each main tick (15-45s) in `char_update()`.

### Room Timers

Same macro pattern with `RTIMER(room, rtmr)`. Decremented in `room_update()` each main tick.

### Fight Timer

`ch->fight_timer` decrements in `violence_update()` every 3 seconds. Used for PvP engagement tracking — prevents players from fleeing combat too quickly.

## Minute Updates

**Location:** [update.c:1094-1110](../../src/systems/update.c#L1094-L1110)

Fires every **60 seconds**. Manages server-wide events:

| Function | Purpose |
|----------|---------|
| `update_ragnarok()` | Ragnarok PvP event state |
| `update_arena()` | Arena match management |
| `update_doubleexp()` | Double XP event countdown |
| `update_doubleqps()` | Double QPS event countdown |

## Source Files

| File | Contents |
|------|----------|
| [update.c](../../src/systems/update.c) | `update_handler()`, `char_update()`, `mobile_update()`, `weather_update()`, `room_update()`, `obj_update()`, `embrace_update()`, all class update functions |
| [comm.c:510-728](../../src/core/comm.c#L510-L728) | `game_loop()` — main loop, timing synchronization |
| [fight.c:56-219](../../src/combat/fight.c#L56-L219) | `violence_update()` — combat round processing |
| [merc.h:250-265](../../src/core/merc.h#L250-L265) | `PULSE_*` constants |
| [merc.h:435-445](../../src/core/merc.h#L435-L445) | Timer macros |
