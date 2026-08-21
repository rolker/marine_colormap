---
issue: 15
---

# Issue #15 — Topo-bathy palette: vendor oleron (MIT) with a hard shoreline discontinuity and intrinsic hinge

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-08-21 14:57 -04:00
**By**: Claude Code Agent (Claude Fable 5)
**Verdict**: approved

**Branch**: feature/issue-15 at `808eb18`
**Mode**: pre-push
**Depth**: Light (reason: additive palette data + one optional metadata struct; stacked on feature/issue-13)
**Must-fix**: 0 | **Suggestions**: 0
**Round**: 1 | **Ship**: recommended — 194 tests pass, no compiler warnings, lint clean

Static analysis: cpplint + uncrustify clean via colcon test.
Scope changed from the issue as filed: a second palette (`hypsometric`) was added after
Roland pointed at GeoZui4D for a licence-clean alternative to GMT's `globe`. Its CLUT is
CCOM/UNH Apache-2.0 — the same licence as this package — and it matches the classic
hypsometric look the issue said we would otherwise have to hand-author.

### Findings
- [ ] No issues found. LGTM.

### Notes
- `PaletteDomain` natural bounds are optional and set together: `hypsometric` has a real
  natural range (its stops are literal elevations), `oleron` does not (normalised ±1
  stretchable master). Claiming −1..+1 metres for oleron would have been a fiction.
- A GCC `-Wdangling-reference` warning in the tests was fixed at source (helper takes
  `const char *` rather than `const std::string &`, so a literal call creates no
  temporary) rather than suppressed.
