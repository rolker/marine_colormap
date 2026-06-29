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

## Implementation
**Status**: complete
**When**: 2026-06-29 07:15 +00:00
**By**: Claude Opus

**Branch**: feature/issue-7
**Scope**: Part 1 only (range model + quality palette, Qt-free core). No Qt widget,
no `marine_colormap_widgets` package — that is Part 2 (separate run). PR is
**"Part of #7"** — does NOT close #7.

### What was done
- **`RangeModel`** (`include/marine_colormap/transfer.hpp`, `src/transfer.cpp`) —
  augments (does not wrap/replace) `TransferParams`. `RangeMode::{Auto, Manual}`;
  `update_auto(min,max)` tracks data extents in Auto (replace, not accumulate) and
  is a **no-op in Manual**; `set_manual(lo,hi)` pins + switches to Manual;
  `reset()` → Auto; getters `lo()`/`hi()`/`mode()`; `normalize(value)` delegates to
  the shared free `normalize(value, lo(), hi())`. `TransferParams` fields unchanged —
  callers copy `lo()`/`hi()` into `params.min`/`max` (or GPU `u_min`/`u_max`).
- **`quality` palette** (`src/palette.cpp`) — `even("quality", {green, yellow, red})`
  (t=0 good → t=0.5 caution → t=1 bad), appended at registry index 5 per the
  append-only, name-keyed contract.
- **ADR** `docs/decisions/0001-range-model.md` (creates `docs/decisions/`) — records
  the Auto/Manual API + its beside-`TransferParams` relationship; notes the widget
  tier is Part 2 in a new `marine_colormap_widgets` package.
- **Tests** — `test/test_transfer.cpp`: 7 `RangeModel` tests (default Auto/unit
  range, Auto tracking, set_manual switch+pin, Manual ignores exceeding data,
  normalize() == free normalize(), degenerate zero-width range, reset()→Auto +
  resumes tracking). `test/test_palette.cpp`: `quality` present at index 5 by name;
  t=0 greenish, t=0.5 yellowish, t=1 reddish.
- **README.md** — new "Range model" section + `quality` in the palette list.

### Five plan-review suggestions — all folded in
- **Palette traceability** — ADR + commit + README cite `quality` as issue #7's
  operator requirement (bathy-uncertainty warning ramp), not untracked scope.
- **Project ADR-0001 cross-reference** — added now in `0001-range-model.md` (not
  deferred), pointing at `unh_marine_autonomy/.../0001-shared-scalar-colormap.md`.
- **Widget Qt-package decision recorded** — ADR states the widget lands in a new
  `marine_colormap_widgets` package (package-boundary decision); the widget's own
  design ADR lands in Part 2.
- **"0001" numbering vs project ADR-0001** — explicit numbering note + cross-ref in
  the ADR disambiguates this package's 0001 from the cross-cutting project 0001.
- **`clamp()` open question resolved** — NO redundant `clamp()` helper added;
  existing `normalize()` (raw) + `apply_response()`/`sample()`/GPU sampler already
  clamp. Rationale recorded in the ADR.

### Build + test
- Built in-container: `./build.sh marine_colormap` → `Finished <<< marine_colormap`.
- Tested in-container: `./test.sh marine_colormap` →
  **105 tests, 0 errors, 0 failures, 14 skipped** (skipped = lint tools absent in
  container). New `RangeModel` suite (7) and `Palette.QualityWarningRamp` ran green.

### Next step
Open the Part 1 PR ("Part of #7"). Part 2 (the `marine_colormap_widgets` Qt
colorbar widget) is a separate run / separate PR and still leaves #7 open.

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-06-29 07:15 +00:00
**By**: Claude Code Agent (Claude Opus)
**Verdict**: approved

**Branch**: feature/issue-7 at `d9668d0`
**Mode**: pre-push
**Depth**: Deep (reason: new ADR `docs/decisions/0001-range-model.md` is a Deep promotion trigger; 200+ total lines)
**Must-fix**: 0 | **Suggestions**: 3
**Round**: 1 | **Ship**: recommended — no Must-fix; suggestions are hardening/hygiene only

### Findings
- [ ] (suggestion) Setters accept inverted range (`lo > hi`) silently; `normalize()` then maps all values to 0 (cross-pass confirmed Lens A+B) — `src/transfer.cpp:41,50`
- [ ] (suggestion) No test asserts `reset()` preserves the prior extent until the next `update_auto()` — `test/test_transfer.cpp` / `src/transfer.cpp:57`
- [ ] (suggestion) ADR status is `Proposed` though it merges with its implementation; consider `Accepted` — `docs/decisions/0001-range-model.md:5`

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-06-29 07:30 +00:00
**By**: Claude Code Agent (Claude Opus 4.8 (1M context))
**Verdict**: approved

**Branch**: feature/issue-7 at `f009945`
**Mode**: pre-push
**Depth**: Deep (reason: new ADR `docs/decisions/0001-range-model.md` is a Deep promotion trigger)
**Must-fix**: 0 | **Suggestions**: 1
**Round**: 2 | **Ship**: recommended — no Must-fix; all three Round-1 suggestions resolved (commits 55cb21a, f009945); lone remaining item is optional NaN-hardening

### Findings
- [ ] (suggestion) `update_auto()`/`set_manual()` don't guard NaN/inf; behavior is safe (degenerate guard returns 0, no UB) but untested/undocumented — add a doc note or lock-in test — `src/transfer.cpp:42,52`

<!-- Round-1 suggestions verified resolved this round:
     inverted-range guard (std::minmax + tests), reset-preserves-extent test,
     ADR status Proposed->Accepted. ament_cpplint clean; cppcheck only
     pre-existing style notes on untouched lines. Lens B clean. -->

