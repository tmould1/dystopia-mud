# Dystopia MUD - Technical Debt

This document tracks compiler warnings and code quality issues that should be addressed over time.

**Last Updated:** 2025-12-19
**Compiler:** GCC (MinGW-w64)
**Total Warnings:** 290

---

## Summary by Category

| Category | Count | Severity | Description |
|----------|-------|----------|-------------|
| Pointer/Character Comparison | 54 | Medium | Comparing pointer to `'\0'` instead of dereferencing |
| Misleading Indentation | 32 | Low | `if`/`else`/`while` clauses don't guard intended code |
| Buffer Overflow Risk | 90+ | **High** | `sprintf` can overflow fixed buffers |
| String Literal Comparison | 13 | Medium | Using `==` instead of `strcmp()` |
| Unused Variables | 35+ | Low | Variables set but never used |
| Uninitialized Variables | 8 | **High** | May be used before initialization |
| Array Bounds | 4 | **Critical** | Array access beyond bounds |
| Format String | 4 | Medium | Wrong format specifier for type |

---

## Critical Issues (Fix First)

### 1. Array Out-of-Bounds Access

These cause undefined behavior and potential crashes/security issues.

| File | Line | Issue |
|------|------|-------|
| update.c | 980 | `array subscript 30 is above array bounds of 'sh_int[30]'` |
| update.c | 1806 | `array subscript 23 is above array bounds of 'int[20]'` |

**Root Cause:** Off-by-one errors or incorrect loop bounds.

### 2. Undefined Behavior in Loops

| File | Line | Issue |
|------|------|-------|
| upgrade.c | 262 | `iteration 8 invokes undefined behavior` |
| save.c | 801 | `iteration 8 invokes undefined behavior` |
| save.c | 1040 | `iteration 8 invokes undefined behavior` |

**Root Cause:** Loop iterates beyond array bounds.

### 3. Dangling Pointer Usage

| File | Line | Issue |
|------|------|-------|
| act_move.c | 4086 | `dangling pointer 'pAbility' to 'quiet_pointer' may be used` |

**Root Cause:** Pointer to local variable used after scope ends.

---

## High Priority (Security/Stability)

### 1. Buffer Overflow Risks (`sprintf` without bounds)

The codebase extensively uses `sprintf()` with fixed-size buffers (typically `char buf[MAX_STRING_LENGTH]` = 8192 bytes). GCC detects many cases where the output could exceed the buffer size.

**Affected Files (90+ warnings):**
- act_info.c (do_who, do_score, do_stat, do_finger)
- act_move.c (move_char, do_disciplines, do_train)
- act_comm.c (do_command)
- act_wiz.c (multiple functions)
- jobo_act.c, jobo_fight.c, jobo_util.c
- Many others

**Example:**
```c
// act_info.c:2795 - do_who
sprintf(buf17 + strlen(buf17), " %-16s %-6s %-24s %-12s %s\n\r", ...);
// Output: 66+ bytes, possibly up to 24609 into 8192-byte buffer
```

**Fix:** Replace `sprintf` with `snprintf`:
```c
snprintf(buf, sizeof(buf), "format", args...);
```

### 2. Uninitialized Variable Usage

| File | Line | Variable | Function |
|------|------|----------|----------|
| special.c | 81 | `v_next` | (multiple) |
| special.c | 132 | `v_next` | |
| special.c | 1626 | `v_next` | |
| olc_act.c | 1246 | `value` | |
| olc_act.c | 3698 | `bob` | |
| db.c | (various) | `pArea`, `pMob`, `pObj` | |

**Fix:** Initialize variables at declaration or ensure all code paths assign before use.

---

## Medium Priority (Bugs/Logic Errors)

### 1. Pointer vs Character Comparison (54 instances)

**Pattern:** Code compares a pointer to `'\0'` instead of checking if the string is empty.

**Example:**
```c
// act_comm.c:940
if (rt->name != NULL && rt->name != '\0'  // WRONG: comparing pointer to char
```

**Should be:**
```c
if (rt->name != NULL && rt->name[0] != '\0'  // Correct: dereference first
// Or simply:
if (rt->name != NULL && *rt->name != '\0'
```

**Affected Files:**
- act_comm.c (1)
- act_obj.c (12)
- act_wiz.c (14)
- handler.c (8)
- olc_act.c (2)
- update.c (1)
- jobo_act.c (10)
- jobo_fight.c (4)
- Many others

### 2. String Literal Comparison (13 instances)

**Pattern:** Using `==` to compare strings instead of `strcmp()`.

**Example:**
```c
if (argument == "some string")  // WRONG: compares pointers
```

**Should be:**
```c
if (strcmp(argument, "some string") == 0)  // Correct
// Or for prefix:
if (str_cmp(argument, "some string") == 0)  // MUD's case-insensitive version
```

**Affected Files:**
- act_wiz.c (9)
- jobo_act.c (1)
- jobo_fight.c (2)
- jobo_util.c (1)

### 3. Address of Array Always True

**Pattern:** Checking if a local array's address is NULL (it never will be).

| File | Line | Variable |
|------|------|----------|
| act_info.c | 4148 | `arg` |
| vamp.c | 634 | `arg1`, `arg2` |
| jobo_act.c | 2254 | `arg3` |
| jobo_fight.c | 1259 | `who` |

**Example:**
```c
char arg[MAX_INPUT_LENGTH];
if (arg == '\0')  // WRONG: should be arg[0] == '\0'
```

### 4. Format String Mismatches

| File | Line | Issue |
|------|------|-------|
| save.c | 468 | `%ld` for `time_t` (should be `%lld` on Win64) |
| db.c | various | `%ld` for `time_t` |

**Fix:** Use portable format or cast:
```c
printf("%lld", (long long)time_value);
// Or use PRId64 from <inttypes.h>
```

---

## Low Priority (Code Quality)

### 1. Misleading Indentation (32 instances)

Code is indented as if it's inside an `if`/`else`/`while` block, but it's actually not guarded.

**Example:**
```c
// act_comm.c:305
else
    do_something();
    position = ch->position;  // NOT part of else!
```

**Affected Files:**
- act_comm.c (2)
- act_info.c (3)
- act_move.c (2)
- act_wiz.c (5)
- vamp.c (2)
- ww.c (2)
- update.c (2)
- merc.h (WAIT_STATE macro)
- Many others

**Fix:** Add braces or fix indentation:
```c
else {
    do_something();
}
position = ch->position;
```

### 2. Unused Variables (35+ instances)

Variables are assigned but never read.

**Common culprits:**
- `found` - boolean that gets set but never checked
- `spelltype` - set but unused (5 instances)
- Direction variables: `north_ok`, `south_ok`, etc.

**Files with multiple unused vars:**
- act_move.c (6)
- special.c (12)
- vamp.c (4)
- save.c (2)

**Fix:** Remove the variable or use `(void)varname;` if intentionally unused.

---

## File-by-File Summary

| File | Warnings | Primary Issues |
|------|----------|----------------|
| act_info.c | 30+ | Buffer overflow, unused vars |
| act_wiz.c | 25+ | Pointer compare, string compare, buffer overflow |
| act_move.c | 18 | Buffer overflow, unused vars, dangling pointer |
| jobo_act.c | 15+ | Pointer compare, buffer overflow |
| jobo_fight.c | 10+ | Pointer compare, string compare |
| handler.c | 10 | Pointer compare |
| save.c | 8 | Format string, loop bounds, unused vars |
| special.c | 15 | Unused vars, uninitialized vars |
| update.c | 6 | Array bounds, pointer compare |
| vamp.c | 8 | Unused vars, misleading indent |

---

## Recommended Fix Order

1. **Array bounds violations** (update.c) - crashes/security
2. **Loop undefined behavior** (upgrade.c, save.c) - crashes
3. **Dangling pointer** (act_move.c) - crashes
4. **Uninitialized variables** (special.c, olc_act.c, db.c) - unpredictable behavior
5. **Buffer overflows** (all files) - security vulnerabilities
6. **Pointer/char comparisons** - logic bugs
7. **String comparisons** - logic bugs
8. **Format strings** - portability
9. **Misleading indentation** - maintainability
10. **Unused variables** - code cleanup

---

## Notes

- The WAIT_STATE macro in merc.h:3117 causes misleading indentation warnings wherever it's used
- Many buffer overflow warnings are theoretical (would require very long strings) but should still be fixed
- Some "unused variable" warnings may be intentional (future use) but should be marked with `(void)var;`
- The `_filbuf` redeclaration warnings are Windows-specific and benign
