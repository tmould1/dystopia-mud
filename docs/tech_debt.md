# Dystopia MUD - Technical Debt

This document tracks compiler warnings and code quality issues that should be addressed over time.

**Last Updated:** 2025-12-21
**Compiler:** GCC (MinGW-w64)
**Total Warnings:** 183

---

## Current Warnings Summary

| Category | Count | Severity | Status |
|----------|-------|----------|--------|
| Format Overflow (`-Wformat-overflow`) | ~130 | Medium | Pending |
| Unused Variables (`-Wunused-but-set-variable`) | 26 | Low | Pending |
| Misleading Indentation (`-Wmisleading-indentation`) | 20 | Low | Pending |
| Format Truncation (`-Wformat-truncation`) | 6 | Medium | Pending |
| Maybe Uninitialized (`-Wmaybe-uninitialized`) | 4 | **High** | Pending |
| Format Type Mismatch (`-Wformat=`) | 3 | Medium | Pending |
| String Truncation (`-Wstringop-truncation`) | 2 | Medium | Pending |
| Attribute Warning | 2 | Low | Pending |
| String Literal Comparison (`-Waddress`) | 1 | Medium | Pending |
| Unused Statement (`-Wunused-value`) | 1 | Low | Pending |
| Zero-Length Format (`-Wformat-zero-length`) | 1 | Low | Pending |

---

## Warnings by File

| File | Warnings | Primary Issues |
|------|----------|----------------|
| kav_fight.c | 31 | Format overflow |
| act_info.c | 23 | Format overflow, unused vars |
| fight.c | 20 | Format overflow, unused vars |
| special.c | 12 | Format overflow |
| daemon.c | 12 | Format overflow, misleading indentation |
| act_move.c | 12 | Format overflow, unused vars |
| db.c | 9 | Format truncation, misleading indentation |
| kav_info.c | 7 | Format overflow |
| comm.c | 7 | Misleading indentation, strncpy truncation |
| clan.c | 7 | Format overflow, misleading indentation |
| save.c | 4 | Format overflow |
| olc.c | 4 | Maybe uninitialized |
| magic.c | 4 | Format overflow |
| Others | 31 | Various |

---

## High Priority Issues

### Maybe Uninitialized Variables (4 warnings)

These can cause undefined behavior and should be fixed first:

1. **olc.c** - `pArea` may be used uninitialized (2 warnings)
2. **olc_act.c** - `pObj` may be used uninitialized
3. **olc_act.c** - `pMob` may be used uninitialized

**Fix:** Initialize pointers to NULL at declaration.

---

## Medium Priority Issues

### Format Overflow (~130 warnings)

sprintf/snprintf calls where the compiler detects potential buffer overflow:

**Common patterns:**
- `sprintf(buf, "...", name)` where `name` could be up to 399 bytes into a 400-byte region
- Buffer concatenation where accumulated size exceeds buffer

**Fix approaches:**
1. Increase buffer sizes where safe
2. Use snprintf with proper size limits
3. Truncate input strings before formatting

### Format Truncation (6 warnings)

snprintf output may be truncated (db.c path construction):

**Affected:** `mud_area_dir`, `mud_player_dir`, `mud_backup_dir`, `mud_txt_dir`, `mud_log_dir`

**Fix:** Increase path buffer sizes or validate base path length.

### Format Type Mismatch (3 warnings)

`%ld` format with `time_t` type (which is `long long int` on this platform):

**Affected:** board.c lines 137, 138, and one other location

**Fix:** Cast `time_t` to `long` or use `%lld` format specifier.

### String Truncation (2 warnings)

strncpy truncation issues in comm.c:
- Line 2226: strncpy output truncated before terminating nul

**Fix:** Ensure null termination after strncpy or use safer alternatives.

### String Literal Comparison (1 warning)

comm.c:2332 - Comparison with string literal results in unspecified behavior

**Fix:** Use `strcmp()` instead of `==` for string comparison.

---

## Low Priority Issues

### Misleading Indentation (20 warnings)

Code where if/else/while clauses don't guard the statements that appear indented:

**Affected files:** clan.c, comm.c, daemon.c, db.c, demon.c, drow.c

**Fix:** Add braces `{}` to clarify control flow.

### Unused Variables (26 warnings)

Variables set but never used:

| Variable | File |
|----------|------|
| found | act_info.c (2), act_move.c (2) |
| north_ok, south_ok, east_ok, west_ok, up_ok, down_ok | Various |
| spelltype | 2 files |
| chance, ch_next, gm_stance, lch | fight.c |
| fMatch | db.c |
| iFork | comm.c |
| word2ln | build.c |
| Others | Various |

**Fix:** Remove unused variables or suppress with `(void)var;`.

### Other Low Priority (4 warnings)

- `_filbuf` redeclared without dllimport (db.c, merc.h) - Windows-specific, benign
- Statement with no effect (compat.h:93 via comm.c) - Likely intentional no-op
- Zero-length format string (fight.c:3296) - Review intent

---

## Remediation Plan

### Phase 1: High Priority
Fix maybe-uninitialized warnings in olc.c and olc_act.c to prevent undefined behavior.

### Phase 2: Medium Priority
1. Fix format type mismatches (time_t formatting)
2. Fix string literal comparison
3. Address format overflow warnings file by file, starting with highest counts

### Phase 3: Low Priority
1. Fix misleading indentation with braces
2. Remove or suppress unused variables
3. Address minor warnings

---

## Previously Fixed Issues (2025-12-20)

The following issues were fixed in a previous cleanup session (290 warnings eliminated):

### Critical Issues Fixed
- **Array Out-of-Bounds Access** - Fixed in update.c
- **Undefined Behavior in Loops** - Removed duplicate cmbt loops in upgrade.c, save.c
- **Dangling Pointer** - Fixed in act_move.c by moving variable to function scope

### High Priority Fixed
- **Uninitialized Variables** - Fixed in special.c (v_next), olc_act.c (bob, output, value)
- **Pointer vs Character Comparison** - Fixed 54 instances across multiple files
- **String Literal Comparison** - Fixed 7 instances in fight.c using `!strcmp()`

### Medium Priority Fixed
- **Misleading Indentation** - Fixed in act_comm.c, act_obj.c, update.c, act_info.c, act_move.c, build.c
- **Unused Variables** - Fixed in act_wiz.c, vamp.c, act_move.c, and others
- **Address of Array Comparison** - Fixed in act_info.c, vamp.c, const.c, build.c

### Low Priority Fixed
- **Buffer Overflow Risks** - Converted many `sprintf` to `snprintf`
- **Format String Issues** - Fixed some `%ld` to use proper casting for `time_t`

---

## Notes

- The WAIT_STATE macro in merc.h was a source of misleading indentation warnings
- Many format-overflow warnings are theoretical - buffer sizes are typically adequate but compiler cannot prove safety
- Some unused variables may be intentionally set for debugging or future use
