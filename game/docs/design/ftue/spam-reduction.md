# Spam Reduction Design

## Overview

New players are currently greeted with a wall of text during login and character creation. This document outlines changes to create a cleaner, more focused first impression.

## Current Issues

### 1. Placeholder Welcome Message
The current welcome message contains "XXXXXXXXX" as a placeholder for the game name. This looks unprofessional and should be replaced or removed entirely.

### 2. Verbose Newbie Gear Messages
When new characters receive their starting equipment, each item creation and equip action generates visible output. This creates a rapid scroll of:
- Item creation messages
- "You wear X" messages for each piece
- Inventory updates

This is confusing for new players who haven't even learned what equipment is yet.

### 3. Generic Welcome Text
All players receive the same welcome regardless of their experience level. A veteran creating an alt doesn't need the same introduction as a first-time MUD player.

### 4. System Noise
Various system messages during login (multiplay warnings, connection info, etc.) appear without context, adding to the information overload.

## Proposed Changes

### Welcome Message Replacement

**Remove:** The "Welcome to XXXXXXXXX" placeholder message.

**Replace with:** Experience-level-appropriate greetings:

**For Never MUD'd:**
Brief, welcoming introduction that sets expectations for a text-based game without overwhelming with information. Focus on encouragement and pointing toward the tutorial.

**For MUD Experience:**
Acknowledge their familiarity with MUDs while highlighting what makes Dystopia different. Point toward Dystopia-specific tutorials.

**For Veterans:**
Minimal greeting. Perhaps just a "Welcome back" or nothing beyond the MOTD. Get them into the game quickly.

### Silent Newbie Gear

**Goal:** The newbie pack creation and equipping should happen silently.

**Current behavior:**
- Each item spawn shows a message
- Each `wear` command shows "You wear the X"
- Results in 10+ lines of equipment spam

**New behavior:**
- Items created without output
- Items equipped without individual messages
- Single summary message after completion: "You have been outfitted with basic equipment."
- Or no message at all - let them discover their inventory naturally

### Contextual System Messages

**Multiplay warnings:** Remove as this is an out-dated way of doing things.  Leave tracking and alerts to staff.

**Connection info:** Move technical details to a `whoami` or `connection` command rather than displaying on login.

**MOTD:** Keep, but ensure it's concise and relevant; mention help tutorial as a springboard into the help system.  create a help tutorial to springboard users.

## Message Timing

### What to show BEFORE entering the world:
- Character creation prompts (essential)
- MOTD (important announcements)
- Brief experience-appropriate welcome

### What to show AFTER entering the world:
- Room description (their first look at the game)
- Minimal orientation hint based on experience level

### What to NOT show:
- Equipment creation/wear spam
- Technical connection details
- Generic welcome banners
- Redundant confirmations (consider removing gmcp and mxp confirmations; allow players to toggle through do_config)

## Experience-Specific Text

### Level 1 (Never MUD'd) - Entry Text
Should convey:
- This is a text-based game - you type commands
- The tutorial ahead will teach you how to play
- Take your time, there's no rush
- Help is available (newbiechat channel, help command)

### Level 2 (MUD Experience) - Entry Text
Should convey:
- Welcome to Dystopia specifically
- Key differences from other MUDs
- Where to find Dystopia-specific documentation
- How to skip to playing if they want

### Level 3 (Veteran) - Entry Text
Should convey:
- Welcome back (or nothing)
- Any major recent changes (optional)
- Directions to key locations
- Get out of their way

## Implementation Principles

1. **Less is more** - When in doubt, show less text
2. **Front-load important info** - Critical messages first, details later
3. **Respect player agency** - Let them explore rather than dumping info
4. **Context matters** - Same message isn't right for everyone
5. **Clean presentation** - Proper spacing, no broken formatting, no placeholders
