---
issue: 3
---

# Issue #3 — Add viridis and turbo palettes from canonical tables

## Implementation
**Status**: complete (pending review)
**When**: 2026-06-07
**By**: Claude Code Agent (Claude Opus 4.8 (1M context))

**Branch**: feature/issue-3

- `src/perceptual_palettes.cpp` — GENERATED canonical 256-entry tables for
  `viridis` (van der Walt & Smith) and `turbo` (Mikhailov, Google), extracted
  verbatim from matplotlib 3.6.3; exposes `detail::viridis_colors()` /
  `turbo_colors()`. Not hand-rolled (per the ADR-0001 review).
- `palette.cpp` — appended viridis (index 3) and turbo (index 4) to the registry
  via the existing even() builder; forward-declares the data accessors.
- Tests: registry now validates names/indices 3=viridis, 4=turbo (exercises the
  append-only contract that issue #1's R2 fix set up); golden 8-bit endpoints
  vs matplotlib: viridis 68,1,84 -> 253,231,37; turbo 48,18,59 -> 122,4,3.
- README + palette.hpp registry comment updated.

colcon test: 80 tests, 0 failures. Validates the append-only design end-to-end
(indices 0-2 unchanged, 3-4 appended).
