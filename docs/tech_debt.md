# Dystopia MUD - Technical Debt

This document tracks compiler warnings and code quality issues that should be addressed over time.

**Last Updated:** 2025-12-20
**Compiler:** GCC (MinGW-w64)
**Total Warnings:** 198 (down from 290)

---

## Summary by Category

| Category | Count | Severity | Status |
|----------|-------|----------|--------|
| Array Bounds | 4 | **Critical** | ✅ FIXED |
| Undefined Behavior in Loops | 3 | **Critical** | ✅ FIXED |
| Dangling Pointer | 1 | **Critical** | ✅ FIXED |
| Uninitialized Variables | 8 | **High** | ✅ FIXED |
| Pointer/Character Comparison | 54 | Medium | ✅ FIXED |
| String Literal Comparison | 7 | Medium | ✅ FIXED |
| Misleading Indentation | ~10 | Low | ✅ PARTIALLY FIXED |
| Unused Variables | ~20 | Low | ✅ PARTIALLY FIXED |
| Buffer Overflow Risk | 90+ | **High** | ⚠️ REMAINING |
| Format String | 4 | Medium | ⚠️ REMAINING |

---

## ✅ Fixed Issues

### Critical Issues (All Fixed)

1. **Array Out-of-Bounds Access** - Fixed in update.c (already fixed before this session)
2. **Undefined Behavior in Loops** - Removed duplicate cmbt loops in upgrade.c, save.c
3. **Dangling Pointer** - Fixed in act_move.c by moving variable to function scope

### High Priority (Fixed)

1. **Uninitialized Variables** - Fixed in special.c (v_next), olc_act.c (bob, output, value)
2. **Pointer vs Character Comparison** - Fixed 54 instances across act_obj.c, handler.c, act_comm.c, update.c, act_wiz.c, olc_act.c
3. **String Literal Comparison** - Fixed 7 instances in fight.c using `!strcmp()`, fixed arg3 comparison in act_wiz.c

### Medium Priority (Partially Fixed)

1. **Misleading Indentation** - Fixed in act_comm.c, act_obj.c, update.c
2. **Unused Variables** - Fixed in act_wiz.c (fOld, value, debug)

---

## ⚠️ Remaining Issues

### 1. Buffer Overflow Risks (`sprintf` without bounds)

The codebase extensively uses `sprintf()` with fixed-size buffers (typically `char buf[MAX_STRING_LENGTH]` = 8192 bytes). GCC detects many cases where the output could exceed the buffer size.

**Affected Files (~90 warnings):**
- act_info.c (do_who, do_score, do_stat, do_finger)
- act_move.c (move_char, do_disciplines, do_train)
- act_comm.c (do_command)
- fight.c (do_hurl, group_gain)
- Many others

**Fix:** Replace `sprintf` with `snprintf`:
```c
snprintf(buf, sizeof(buf), "format", args...);
```

### 2. Format String Issues

| File | Line | Issue |
|------|------|-------|
| board.c | 137-138 | `%ld` for `time_t` (should be `%lld` on Win64) |

**Fix:** Use portable format:
```c
printf("%lld", (long long)time_value);
```

### 3. Remaining Unused Variables

Some unused variables remain in:
- act_info.c (found)
- act_move.c (revdoor, found, needed, is_ok, primal)
- build.c (word2ln)
- fight.c (chance, ch_next, gm_stance, lch)
- handler.c (found)
- olc_act.c (ped)

### 4. Remaining Misleading Indentation

Some misleading indentation warnings remain in:
- act_info.c (lines 173, 229, 1642)
- act_move.c (line 4526)
- build.c (line 394)

### 5. Address of Array Always True

**Pattern:** Checking if a local array's address is NULL (it never will be).

| File | Line | Variable |
|------|------|----------|
| act_info.c | 4148 | `arg` |
| vamp.c | 634 | `arg1`, `arg2` |
| jobo_act.c | 2254 | `arg3` |
| jobo_fight.c | 1259 | `who` |
| build.c | 916 | `tmp` |

**Fix:**
```c
char arg[MAX_INPUT_LENGTH];
if (arg[0] == '\0')  // Correct: check first character
```

---

## Recommended Next Steps

1. **Buffer overflows** - Convert remaining `sprintf` to `snprintf` (largest category)
2. **Format strings** - Fix `%ld` to `%lld` for `time_t` on Win64
3. **Remaining unused variables** - Remove or mark with `(void)var;`
4. **Remaining misleading indentation** - Add braces for clarity

---

## Notes

- The WAIT_STATE macro in merc.h causes misleading indentation warnings wherever it's used
- Many buffer overflow warnings are theoretical (would require very long strings) but should still be fixed
- Some "unused variable" warnings may be intentional (future use) but should be marked with `(void)var;`
