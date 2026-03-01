# Client Protocol Stack

The server supports a full suite of MUD client protocols beyond basic telnet, enabling compression, rich text, structured data exchange, terminal capability detection, and accessibility features.

## Protocol Inventory

| Protocol | File | Lines | Purpose |
|----------|------|-------|---------|
| **GMCP** | [gmcp.c](../../../src/systems/gmcp.c) | 557 | Structured JSON data: character status, room info, inventory |
| **MCCP** | [mccp.c](../../../src/systems/mccp.c) | 295 | Zlib compression for network traffic |
| **MXP** | [mxp.c](../../../src/systems/mxp.c) | 1,388 | Rich text: clickable links, tooltips, interactive elements |
| **MSSP** | [mssp.c](../../../src/systems/mssp.c) | 187 | Server status broadcasting for MUD crawlers |
| **NAWS** | [naws.c](../../../src/systems/naws.c) | 111 | Negotiate terminal window size (columns/rows) |
| **TTYPE** | [ttype.c](../../../src/systems/ttype.c) / [ttype.h](../../../src/systems/ttype.h) | 194 | Terminal type and MTTS capability flags |
| **MCMP** | [mcmp.c](../../../src/systems/mcmp.c) | 946 | MUD Client Media Protocol (audio cues) |
| **Charset** | [charset.c](../../../src/systems/charset.c) | 192 | Character encoding negotiation (UTF-8/ASCII) |

## Connection Flow

When a client connects, the server enters `CON_DETECT_CAPS` state and sends telnet negotiation offers. The intro system waits for responses, classifies the client, and sends appropriate content:

```
New connection
  ├─ Server sends telnet WILL/DO for: TTYPE, NAWS, GMCP, MCCP, CHARSET
  ├─ Enter CON_DETECT_CAPS state
  │
  ├─ intro_check_ready() called each pulse (250ms)
  │   ├─ Client responds with capabilities over 1-3 TTYPE rounds
  │   ├─ NAWS reports terminal size
  │   ├─ GMCP negotiation completes
  │   │
  │   ├─ Short-circuit: TTYPE round 3 complete → full picture, send intro
  │   ├─ Short-circuit: TTYPE refused + 2 pulses → send intro
  │   └─ Timeout: 5 pulses (1.25s), extended to 15 if TTYPE in progress
  │
  ├─ intro_classify_tier(d) → determine client tier
  ├─ intro_send(d) → send tier-appropriate intro text
  └─ Transition to CON_GET_NAME
```

## Tiered Intro System

**Location:** [intro.h](../../../src/core/intro.h) / [intro.c](../../../src/core/intro.c) (181 lines)

After capability detection, the client is classified into one of four tiers:

| Tier | Enum | Detection Criteria | Content |
|------|------|--------------------|---------|
| **Rich** | `INTRO_TIER_RICH` | Truecolor or 256-color + terminal width >= 100 | Full ASCII art with rich color |
| **Standard** | `INTRO_TIER_STANDARD` | ANSI color, TTYPE detected, or GMCP-capable | 16-color ANSI art |
| **Basic** | `INTRO_TIER_BASIC` | No capabilities detected | Plain text, no formatting |
| **Screen Reader** | `INTRO_TIER_SCREENREADER` | MTTS screen reader flag set | Accessible prose, no art |

### Classification Logic

**Location:** [intro.c:38-62](../../../src/core/intro.c#L38-L62) — `intro_classify_tier()`

1. Screen reader flag (`MTTS_SCREEN_READER`) → **SCREENREADER** (takes priority)
2. Truecolor/256-color + wide terminal → **RICH**
3. Truecolor/256-color + narrow terminal → **STANDARD**
4. ANSI flag or TTYPE detection → **STANDARD**
5. GMCP-capable → **STANDARD**
6. No capabilities → **BASIC**

Intro text is loaded from the help database during `boot_db()` via `intro_load()`. Keywords: `intro_rich`, `intro_standard`, `intro_basic`, `intro_screenreader`. Falls back through tiers if a specific tier's help entry is missing.

## MTTS Capability Flags

**Location:** [ttype.h:19-30](../../../src/systems/ttype.h#L19-L30)

TTYPE negotiation performs up to 3 rounds. Round 2+ reports MTTS (MUD Terminal Type Standard) bitflags:

| Flag | Bit | Meaning |
|------|-----|---------|
| `MTTS_ANSI` | 1 | ANSI color support |
| `MTTS_VT100` | 2 | VT100 interface |
| `MTTS_UTF8` | 4 | UTF-8 character encoding |
| `MTTS_256_COLORS` | 8 | Xterm 256-color |
| `MTTS_MOUSE_TRACKING` | 16 | Mouse tracking |
| `MTTS_OSC_COLOR` | 32 | OSC color palette |
| `MTTS_SCREEN_READER` | 64 | Screen reader active |
| `MTTS_PROXY` | 128 | Proxy/relay connection |
| `MTTS_TRUECOLOR` | 256 | True color (24-bit RGB) |
| `MTTS_MNES` | 512 | MUD New-Environ Standard |
| `MTTS_MSLP` | 1024 | MUD Server Link Protocol |
| `MTTS_SSL` | 2048 | SSL/TLS secured connection |

These flags drive the intro tier classification and are stored on `DESCRIPTOR_DATA` for use throughout the session.

## Protocol Details

### GMCP (Generic MUD Communication Protocol)

**Location:** [gmcp.c](../../../src/systems/gmcp.c) (557 lines)

Structured JSON data exchange between server and client. Used by clients like Mudlet for graphical overlays, maps, and status bars.

Sends character vitals, room information, group data, and other structured game state to the client out-of-band, without polluting the text stream.

### MCCP (MUD Client Compression Protocol)

**Location:** [mccp.c](../../../src/systems/mccp.c) (295 lines)

Zlib-based compression for outgoing data. Negotiated via telnet option 85 (`TELOPT_COMPRESS`). Reduces bandwidth for verbose MUD output. Compression is per-descriptor with a 16KB buffer (`COMPRESS_BUF_SIZE`).

### MXP (MUD eXtension Protocol)

**Location:** [mxp.c](../../../src/systems/mxp.c) (1,388 lines)

Enables rich text features: clickable command links, tooltips, item interaction menus, formatted output. The forge system uses MXP for interactive crafting UI. Items can display MXP-formatted descriptions with embedded commands.

### MSSP (MUD Server Status Protocol)

**Location:** [mssp.c](../../../src/systems/mssp.c) (187 lines)

Broadcasts server metadata (name, codebase, players online, etc.) for MUD listing sites and crawlers.

### NAWS (Negotiate About Window Size)

**Location:** [naws.c](../../../src/systems/naws.c) (111 lines)

Reports client terminal dimensions (columns x rows). Used for:
- Center-aligned output (`cent_to_char()` in output.c)
- Intro tier classification (wide terminal = rich tier eligible)
- Formatting decisions throughout the game

### Charset Negotiation

**Location:** [charset.c](../../../src/systems/charset.c) (192 lines)

Negotiates character encoding between server and client. Supports UTF-8 and ASCII fallback. Works in conjunction with TTYPE/MTTS detection.

## UTF-8 Support

**Location:** [utf8.h](../../../src/core/utf8.h) / [utf8.c](../../../src/core/utf8.c)

Zero-dependency, cross-platform UTF-8 string handling:

| Function | Purpose |
|----------|---------|
| `utf8_decode()` / `utf8_encode()` | Codepoint decode/encode |
| `utf8_strlen()` | Count Unicode codepoints |
| `utf8_display_width()` | Raw display column width |
| `utf8_visible_width()` | Display width skipping MUD color codes |
| `utf8_wcwidth()` | Per-codepoint column width (0/1/2 for CJK) |
| `utf8_is_valid()` | Validate UTF-8 byte sequences |
| `utf8_truncate()` | Safe truncation at character boundaries |
| `utf8_pad_right()` | Pad to exact display width with spaces |
| `utf8_is_name_char()` | Validate codepoints for player names |
| `utf8_char_script()` | Detect script group (Latin/Cyrillic/CJK) for mixed-script detection |
| `utf8_skeletonize()` | Normalize to ASCII skeleton for profanity/confusable detection |

### Name Validation

Player names support Unicode (Latin Extended, Cyrillic, CJK, Kana, Hangul) with security protections:
- Mixed-script detection prevents Cyrillic/Latin confusable abuse
- Confusable table (loaded from `game.db`) maps lookalike Unicode chars to ASCII equivalents
- Skeleton normalization catches profanity bypass via homoglyphs

## Related Documents

- [Accessibility](../accessibility/overview.md) — Screen reader mode and MCMP audio
- [Heritage](heritage.md) — Protocol stack as modernization
- [Platform & Config](platform-config.md) — Output system integration
