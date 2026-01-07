# First-Time User Experience (FTUE) Design

This directory contains design documentation for improving the new player onboarding experience in Dystopia MUD.

## Overview

The goal of these improvements is to create a smoother, more tailored experience for new players based on their prior experience with MUDs and Dystopia specifically. Rather than a one-size-fits-all approach, the system adapts to the player's familiarity level.

## Design Goals

1. **Reduce information overload** - New players shouldn't be bombarded with irrelevant messages
2. **Respect player time** - Veterans shouldn't be forced through tutorials they don't need
3. **Teach contextually** - Introduce concepts when they're relevant, not all at once
4. **Support different learning needs** - Complete beginners need fundamentals; returning players need Dystopia-specific info

## Current State

Currently, all new players:
- Receive the same welcome messages regardless of experience
- See verbose equipment creation and wear messages
- Start in the same room (MUD school entrance)
- Must navigate the same tutorial path

## Proposed Changes

### Experience-Based Onboarding

During character creation, players select their experience level:
- **Never played a MUD** - Full tutorial with basic concepts
- **MUD experience, new to Dystopia** - Dystopia-specific features only
- **Dystopia veteran** - Minimal tutorial, quick start

### Redesigned MUD School

Three distinct sections corresponding to experience levels:
- Basic MUD concepts (movement, communication, items)
- Dystopia-specific systems (classes, quests, PK, forge)
- Quick veteran path to main game

### Cleaner Login Experience

- Reduced spam during login process
- Silent newbie gear equipping
- Experience-appropriate welcome messages

## Documentation Index

- [Experience Levels](experience-levels.md) - The three-tier experience selection system
- [MUD School Redesign](mud-school.md) - Tutorial sections and learning paths
- [Spam Reduction](spam-reduction.md) - Login message cleanup and streamlining
