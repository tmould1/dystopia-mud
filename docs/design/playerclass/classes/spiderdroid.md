# Spider Droid Class Design

## Overview

Spider Droids are the ultimate champions of Lloth - cybernetic driders with mechanical body parts. They are an **upgrade class** obtained by upgrading from Drow.

**Source File**: `src/spiderdroid.c`
**Class Constant**: `CLASS_DROID` (8192)
**Upgrades From**: Drow

## Lore (from help.are)

> The drows are know for spawning elite warriors, and from the very best of this fighter elite, Lloth chooses her champions, and bless them with powers way beyond that of normal drows. These creatures are called driders, and among them exists a few who are so close to their goddess that she has blessed them with superior bodies. These are the Spider Droids, the machine beings who spread chaos and havoc in the name of the Spider Queen, their goddess Lloth.

## Commands (from help.are)

| Command | Description |
|---------|-------------|
| venomspit | Spit venom on opponent |
| avatarOfLloth | Accept Lloth into your body |
| darkness | Summon a globe of darkness |
| llothsight | Superior vision |
| preach | Preach the will of Lloth to fellow Driders |
| dridereq | Create unholy Drider equipment |
| implant | Graft mechanical bodypart onto body |
| lloth | List members of Church of Lloth |
| web | Entangle opponent in sticky web |
| scry | Spy on mobs/players |
| readaura | Know true power of enemy |

## Key Mechanics

### Implant System
Spider Droids can graft mechanical bodyparts onto themselves, enhancing their abilities. This combines the organic drider form with cybernetic enhancements.

### Avatar of Lloth
Accept Lloth into your body for enhanced divine power from the Spider Queen.

### Church of Lloth Integration
Spider Droids maintain connection to the Church of Lloth:
- Use `lloth` to see other Church members
- Use `preach` to communicate with fellow Driders

## Combat Abilities

### Venomspit
Ranged poison attack unique to Spider Droids.

### Web
Entangle enemies in sticky webs - inherited from drow/spider nature.

### Darkness
Create globe of darkness for tactical advantage.

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DROID bit |
| implants | varies | Mechanical enhancements |

## Notes

- Spider Droids are a modified cyborg class
- Combines drow heritage with mechanical enhancement
- Serve Lloth, the Spider Queen
- Maintain connection to Church of Lloth community
- Upgrade classes can further upgrade (levels 2-5) for additional combat bonuses
- The "droid" in name refers to cybernetic enhancements, not robots
