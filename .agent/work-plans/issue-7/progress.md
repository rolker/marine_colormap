---
issue: 7
---

# Issue #7 — Colormap range model + colorbar legend widget

## Issue Review
**Status**: complete
**When**: 2026-06-29 00:00 +00:00
**By**: Claude Code Agent (Claude Sonnet)

**Issue**: #7
**Comment**: (best-effort post follows this entry; not recorded inline)
**Scope verdict**: well-scoped

### Actions
- [ ] Decide Qt-target-vs-package (new CMake target inside `marine_colormap` vs new `marine_colormap_widgets` package) and record the decision as an ADR addendum or new project ADR — not only a plan section.
- [ ] Plan must enforce staged PRs: range model (Part 1) merged before the colorbar widget (Part 2); the issue requests this but the plan must make it explicit with separate PR scopes.
- [ ] Widget interaction test should be automated (not "or recorded manual verification") per the project's established test pattern.
- [ ] Plan must clarify how the range model relates to the existing `TransferParams::min`/`max` fields — whether it wraps, replaces, or augments them — and update consumers accordingly in the same PRs.
- [ ] Project ADR-0001 may need an addendum covering the new range model API and the widget tier (currently only Tier 1 core and Tier 2 GPU math are defined there).
