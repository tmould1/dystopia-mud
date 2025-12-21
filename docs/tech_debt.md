# Dystopia MUD - Technical Debt

This document tracks compiler warnings and code quality issues that should be addressed over time.

**Last Updated:** 2025-12-20
**Compiler:** GCC (MinGW-w64)
**Total Warnings:** 0 (down from 290) ✅

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
| Misleading Indentation | ~10 | Low | ✅ FIXED |
| Unused Variables | ~20 | Low | ✅ FIXED |
| Address of Array Comparison | ~5 | Low | ✅ FIXED |
| Buffer Overflow Risk | 90+ | **High** | ✅ FIXED (snprintf) |
| Format String | 4 | Medium | ✅ FIXED |

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

### Medium Priority (All Fixed)

1. **Misleading Indentation** - Fixed in act_comm.c, act_obj.c, update.c, act_info.c, act_move.c, build.c
2. **Unused Variables** - Fixed in act_wiz.c, vamp.c, act_move.c, and others
3. **Address of Array Comparison** - Fixed in act_info.c, vamp.c, const.c, build.c

### Low Priority (All Fixed)

1. **Buffer Overflow Risks** - Converted all `sprintf` to `snprintf` across the codebase
2. **Format String Issues** - Fixed `%ld` to use proper casting for `time_t`

---

## 🎉 All Warnings Eliminated!

The codebase now compiles with **0 warnings** using GCC with `-Wall` flag.

### Key Changes Made:

1. **sprintf → snprintf** - Converted all unsafe sprintf calls to use bounds-checked snprintf
2. **Array comparisons** - Fixed `arg == '\0'` to `arg[0] == '\0'` patterns
3. **Misleading indentation** - Added braces where code appeared guarded but wasn't
4. **Unused variables** - Removed genuinely unused variables (revdoor, spelltype, pcost, bloodpool, etc.)
5. **Uninitialized variables** - Added proper initialization
6. **String literal comparisons** - Changed `==` to `!strcmp()` for string comparisons

---

## Notes

- The WAIT_STATE macro in merc.h was a source of misleading indentation warnings
- Buffer overflow fixes are preventive - the old code was unlikely to overflow in practice with 8KB buffers
- Some variables that appeared unused were actually used in different code paths and were left intact
