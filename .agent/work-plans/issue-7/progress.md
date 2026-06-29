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

## Plan Authored
**Status**: complete
**When**: 2026-06-29 10:00 +00:00
**By**: Claude Code Agent (Claude Sonnet)

**Plan**: `.agent/work-plans/issue-7/plan.md` at `1c02312`
**Branch**: feature/issue-7 at `1c02312`
**Phases**: 2 (this PR = Part 1 range model; Part 2 = marine_colormap_widgets widget)

### Open questions
- [ ] Should `RangeModel` expose an explicit `clamp()` helper for the uncertainty sentinel case, or rely on the existing raw-value `normalize()` + GPU clamp contract?

## Plan Review
**Status**: complete
**When**: 2026-06-29 07:00 +00:00
**By**: Claude Code Agent (Claude Opus 4.6)
<!-- Independent: fresh-context Opus review of a Sonnet-authored plan. The
     name-only self-review heuristic matches (workspace uses one agent name),
     but this is genuinely out-of-context — no "in-context" annotation. -->

**Plan**: `.agent/work-plans/issue-7/plan.md` at `1c02312`
**PR**: PR-less (`--issue` mode; gh not authenticated, issue body taken from the prior `## Issue Review` entry)
**Verdict**: approve-with-suggestions

### Findings
- [ ] (suggestion) `quality` palette not traceable to the issue — neither the issue title nor the recorded review-issue actions mention it; confirm it's in scope or split it out (Only what's needed) — `plan.md:32`
- [ ] (suggestion) Project ADR-0001 addendum (review-issue action e) deferred to "a follow-on"; add at least the cross-reference in this PR per ADR-0012 — `plan.md:75`
- [ ] (suggestion) Widget Qt-target-vs-package decision (review-issue action a) asserted as a new `marine_colormap_widgets` package but not yet recorded as an ADR; state where that ADR lands — `plan.md:14`
- [ ] (suggestion) New `docs/decisions/0001-range-model.md` shares the number "0001" with the cross-cutting project ADR-0001; cross-reference it to avoid confusion — `plan.md:35`
- [ ] (suggestion) Resolve/close the `clamp()` open question during implementation — existing `normalize()` + `apply_response()` already clamp, so an explicit helper is likely unnecessary — `plan.md:88`
