# Message Board System Overview

The board system provides 8 named boards with per-board read/write permissions, note expiration, recipient targeting, and unread tracking. Boards are virtual — they can be read from anywhere, not tied to physical objects or rooms. Notes are written through an interactive state machine that takes over the player's connection.

Based on Erwin S. Andreasen's Note Board system v2 (1995-96).

**Location:** [board.c](../../src/systems/board.c), [board.h](../../src/systems/board.h)

## Board Configuration

**Location:** [board.c:58-68](../../src/systems/board.c#L58-L68)

8 boards defined in `boards[MAX_BOARD]`:

| # | Name | Description | Read | Write | Default Recipient | Force Type | Expiry |
|---|------|-------------|------|-------|-------------------|------------|--------|
| 0 | General | General discussion | 0 | 2 | "all" | `DEF_INCLUDE` | 14 days |
| 1 | Ideas | Suggestion for improvement | 0 | 2 | "all" | `DEF_NORMAL` | 14 days |
| 2 | Announce | Announcements from Immortals | 0 | 8 | "all" | `DEF_NORMAL` | 60 days |
| 3 | Bugs | Typos, bugs, errors | 2 | 2 | "imm" | `DEF_INCLUDE` | 60 days |
| 4 | Personal | Personal messages | 0 | 2 | "all" | `DEF_EXCLUDE` | 14 days |
| 5 | Immortal | Immortal Board | 8 | 8 | "imm" | `DEF_INCLUDE` | 60 days |
| 6 | Builder | Builder Board | 7 | 7 | "imm" | `DEF_INCLUDE` | 20 days |
| 7 | Kingdom | Kingdom messages | 0 | 2 | "all" | `DEF_EXCLUDE` | 20 days |

Board 0 (General) is `DEFAULT_BOARD` and must be readable by everyone.

## Data Structures

### BOARD_DATA

**Location:** [board.h:29-45](../../src/systems/board.h#L29-L45)

```c
struct board_data {
    char *short_name;       // max 8 chars, used as filename
    char *long_name;        // description shown in board list
    int read_level;         // minimum trust to see board
    int write_level;        // minimum trust to post (must be >= read_level)
    char *names;            // default recipient string
    int force_type;         // DEF_NORMAL, DEF_INCLUDE, or DEF_EXCLUDE
    int purge_days;         // days until notes expire
    NOTE_DATA *note_first;  // linked list of notes
    bool changed;           // dirty flag for save
};
```

### NOTE_DATA

Each note stores:

| Field | Purpose |
|-------|---------|
| `sender` | Author's name |
| `to_list` | Space-separated recipient list |
| `subject` | Subject line (max 60 chars) |
| `date` | Human-readable date string |
| `date_stamp` | Unix timestamp (unique, monotonic) |
| `expire` | Unix timestamp when note should be archived |
| `text` | Body text (max `MAX_NOTE_TEXT`) |

### Player Fields

| Field | Location | Purpose |
|-------|----------|---------|
| `ch->pcdata->board` | `PC_DATA` | Pointer to currently selected board |
| `ch->pcdata->last_note[MAX_BOARD]` | `PC_DATA` | Timestamp of last read note per board |
| `ch->pcdata->in_progress` | `PC_DATA` | Note currently being written |

## Recipient Targeting

### Force Types

**Location:** [board.h:12-14](../../src/systems/board.h#L12-L14)

Each board has a `force_type` that controls how the default `names` field interacts with the player's chosen recipients:

| Type | Value | Behavior |
|------|-------|----------|
| `DEF_NORMAL` | 0 | If player leaves recipient blank, use `names` as default |
| `DEF_INCLUDE` | 1 | `names` is always added to recipient list (auto-appended if missing) |
| `DEF_EXCLUDE` | 2 | `names` must NOT appear in recipient list (rejected if included) |

Example: The Bugs board has `DEF_INCLUDE` with `names = "imm"`, so every bug report automatically includes "imm" as a recipient, ensuring immortals always see it.

### Recipient Matching

**Location:** [board.c:370-396](../../src/systems/board.c#L370-L396) — `is_note_to()`

A note is visible to a character if any of these match:

1. Character is the sender
2. `to_list` contains "all"
3. Character is immortal and `to_list` contains "imm", "imms", "immortal", "god", "gods", or "immortals"
4. Character is implementor and `to_list` contains "imp", "imps", "implementor", or "implementors"
5. Character's name appears in `to_list`
6. `to_list` is a number and character's trust level >= that number

## Commands

### do_board

**Location:** [board.c:693](../../src/systems/board.c#L693)

No arguments: lists all accessible boards with unread counts and current board info.

With argument (name or number): switches to that board. Displays read/write access status.

### do_note

**Location:** [board.c:658](../../src/systems/board.c#L658)

Dispatch command with subcommands:

| Subcommand | Function | Description |
|------------|----------|-------------|
| `note` / `note read` | [do_nread](../../src/systems/board.c#L526) | Read next unread note, or `note read <#>` for specific note, or `note read again` |
| `note list` | [do_nlist](../../src/systems/board.c#L608) | List notes with `*` marking unread; optional `note list <N>` shows last N |
| `note write` | [do_nwrite](../../src/systems/board.c#L422) | Enter the note writing state machine |
| `note remove <#>` | [do_nremove](../../src/systems/board.c#L580) | Remove a note (sender or implementor only) |
| `note catchup` | [do_ncatchup](../../src/systems/board.c#L643) | Mark all notes on current board as read |

### Unread Tracking

**Location:** [board.c:400-415](../../src/systems/board.c#L400-L415) — `unread_notes()`

Each character stores a `last_note[board_index]` timestamp per board. Notes with `date_stamp > last_note` that pass `is_note_to()` are counted as unread.

Reading a note advances `last_note` to `UMAX(last_note, note->date_stamp)`.

When no unread notes remain on the current board, `note read` auto-advances to the next accessible board via `next_board()`.

## Note Writing State Machine

**Location:** [board.c:422-523](../../src/systems/board.c#L422-L523) (entry), [board.c:837-1089](../../src/systems/board.c#L837-L1089) (handlers)

Writing a note takes over the player's connection via `CON_NOTE_*` states. The nanny handles input at each stage:

```
do_nwrite()
  │
  ├─ CON_NOTE_TO        → handle_con_note_to()
  │   Enter recipients. Force type applied (include/exclude/default).
  │
  ├─ CON_NOTE_SUBJECT   → handle_con_note_subject()
  │   Enter subject (max 60 chars, cannot be empty).
  │
  ├─ CON_NOTE_EXPIRE    → handle_con_note_expire()
  │   Immortals only: choose expiry days. Mortals get board default.
  │
  ├─ CON_NOTE_TEXT       → handle_con_note_text()
  │   Enter body text line by line (max 80 chars/line).
  │   End with ~ or END on empty line.
  │
  └─ CON_NOTE_FINISH    → handle_con_note_finish()
      (C)ontinue writing, (V)iew note, (P)ost, or (F)orget
```

### Writing Restrictions

- NPCs cannot write
- Newbies (age < 2) cannot write
- `PLR_SILENCE` flag blocks writing
- Must have trust >= board's `write_level`
- Lines over 80 characters are rejected
- Total text capped at `MAX_NOTE_TEXT` (about 15KB)
- If player loses link mid-write, note text is preserved for continuation on reconnect (but notes without text are discarded)

### Posting

On post (`P`):
1. `finish_note()` assigns a unique `date_stamp` and appends to the board's linked list and file
2. Posts to boards 0-3 broadcast via `do_info()` ("A new note has been posted by X on board Y")
3. Connection returns to `CON_PLAYING`

## Persistence

**Location:** [board.c:120-167](../../src/systems/board.c#L120-L167) (save), [board.c:268-359](../../src/systems/board.c#L268-L359) (load)

Each board saves to a file named after `short_name` in `NOTE_DIR` (the notes data directory).

### File Format

```
Sender  <name>~
Date    <date string>~
Stamp   <unix timestamp>
Expire  <unix timestamp>
To      <recipient list>~
Subject <subject>~
Text
<body text>~
```

### Auto-Archive

On load, notes with `expire < current_time` are moved to `<boardname>.old` in the same directory and removed from the active board. The `.old` file can be rotated externally (e.g. by a startup script).

### Save Triggers

- `finish_note()` appends to file immediately on post
- `do_nremove()` rewrites the full board file after removal
- `save_notes()` saves all boards marked `changed` (called periodically)

## Programmatic Note Creation

**Location:** [board.c:787-820](../../src/systems/board.c#L787-L820)

Two functions allow code to post notes without a player:

```c
// Post to any board
make_note("Bugs", "System", "imm", "Auto-detected issue", 60, "Details here...");

// Shortcut for Personal board
personal_message("System", "PlayerName", "Welcome", 14, "Hello!");
```

Both create a `NOTE_DATA`, set the expiry from `expire_days`, and call `finish_note()`.

## Source Files

| File | Contents |
|------|----------|
| [board.c](../../src/systems/board.c) | Board configuration, all commands, note writing state machine, persistence, recipient matching |
| [board.h](../../src/systems/board.h) | `BOARD_DATA` struct, `CON_NOTE_*` states, `DEF_*` force types, constants, prototypes |
