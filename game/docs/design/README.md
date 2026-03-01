# Design Documentation

Reference documentation for Dystopia MUD's game systems, architecture, and engineering infrastructure.

## Architecture & Infrastructure

Technical documentation covering the codebase's engineering foundations, modernization state, and development workflow.

| Document | Description |
|----------|-------------|
| [Heritage & Modernization](architecture/heritage.md) | Codebase lineage (Diku → Merc → GodWars → Dystopia → Dystopia+), source organization, modernization inventory |
| [Database Layer](architecture/database.md) | SQLite persistence: area databases, game state, class registry, boot sequence, two-phase loading |
| [Build System & CI/CD](architecture/build-system.md) | Project file generation, build scripts, GitHub Actions, Docker, AWS deployment |
| [Test Infrastructure](architecture/testing.md) | Test framework, assertion macros, headless boot, output capture, adding tests |
| [Client Protocols](architecture/client-protocols.md) | Protocol stack (GMCP, MCCP, MXP, MSSP, NAWS, TTYPE), tiered intro system, UTF-8 |
| [Platform & Config](architecture/platform-config.md) | Cross-platform compatibility layer, unified configuration system, output system |

## Game Systems

Core gameplay mechanics and subsystem documentation.

| Document | Description |
|----------|-------------|
| [Combat](combat/overview.md) | THAC0-based combat, damage calculation, defense, stances, attack system |
| [Update / Tick System](update/overview.md) | Game loop, pulse timing, class updates, weather, regeneration |
| [Command Interpreter](interpreter/overview.md) | 724-command table, trust levels, discipline gating, prefix matching |
| [Communication](communication/overview.md) | 23+ channels, socials, deafness, GMCP integration |
| [Movement](movement/overview.md) | Room transitions, portals, doors, flight, water, tracking, map |
| [Objects](objects/overview.md) | 75+ item types, 27 equipment slots, containers, transfer functions |
| [Message Boards](boards/overview.md) | 8 virtual boards, note writing, persistence, recipient targeting |
| [Kingdoms](kingdoms/overview.md) | Faction system, kills/deaths, treasury, rankings |

## Player Systems

Character progression, classes, and competitive systems.

| Document | Description |
|----------|-------------|
| [Player Classes](playerclass/README.md) | All 26 classes (13 base + 13 upgrade), disciplines, implementation guide |
| [PvP Systems](pvp/overview.md) | Arena, bounties, Ragnarok, PK flags, protections, Paradox anti-griefing |
| [Mastery](mastery.md) | End-game achievement system, 28 skill categories, unique items |
| [Quest System](quest/README.md) | Item customization (costs/modifiers) and quest cards (collection) |

## World Building & Content

Area creation, crafting, and player onboarding.

| Document | Description |
|----------|-------------|
| [OLC](olc/README.md) | Online creation: room/object/mobile editors, special functions, RoomText |
| [Forge System](forge/README.md) | Metals, gems, hilts crafting with drop rates and balance |
| [First-Time Experience](ftue/README.md) | Experience-based onboarding, MUD school, spam reduction |
| [Accessibility](accessibility/overview.md) | Screen reader mode, MCMP audio protocol, sounds |
