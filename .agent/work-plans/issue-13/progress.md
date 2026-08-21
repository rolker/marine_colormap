---
issue: 13
---

# Issue #13 — Core: anchored breakpoints and the S-100-shaped lookup table (value ranges with explicit closures)

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-08-21 14:24 -04:00
**By**: Claude Code Agent (Claude Fable 5)
**Verdict**: approved

**Branch**: feature/issue-13 at `d9a5f92`
**Mode**: pre-push
**Depth**: Light (reason: single new module, additive, 4 files)
**Must-fix**: 2 | **Suggestions**: 7
**Round**: 1 | **Ship**: recommended — both must-fixes and all 7 suggestions applied in d9a5f92; 173 tests pass

Static analysis: cpplint + uncrustify clean via colcon test.
Claude Adversarial: 1 pass, correctness-focused; findings reproduced against a compiled probe harness rather than inferred.

### Findings
- [x] (must-fix) +inf break value clamped to lo instead of hi, inverting the palette split in the "no land in view" case — `src/lookup.cpp`
- [x] (must-fix) Value exactly on an exclusive domain edge returned unmapped, not over/under — renders a legal sounding as a transparent hole — `src/lookup.cpp`
- [x] (suggestion) rampable() true for ranges whose width overflows to inf, silently flattening the ramp — `src/lookup.cpp`
- [x] (suggestion) Non-finite lo/hi undocumented and leaked NaN through lo()/hi() — `src/lookup.cpp`
- [x] (suggestion) under/over fallbacks resolved by table position rather than by value — `src/lookup.cpp`
- [x] (suggestion) over fallback could return an end_color the entry never renders — `src/lookup.cpp`
- [x] (suggestion) domain_min/max did not skip empty (inverted) ranges, making over unreachable — `src/lookup.cpp`
- [x] (suggestion) lookup() recomputed both domain bounds per call on a per-pixel path — now indexed once at construction — `src/lookup.cpp`
- [x] (suggestion) Header overstated CPU/GPU parity as enforced; nothing enforces it yet — `include/marine_colormap/lookup.hpp`
- [x] (test gap) Exclusive domain edge, infinite break endpoints, domain value assertions incl. semi-interval, non-rampable over fallback, monotonicity fuzz — `test/test_lookup.cpp`
