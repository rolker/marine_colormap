# Plan: Colormap range model + colorbar legend widget — Part 1 (range model, Qt-free core)

## Issue

https://github.com/rolker/marine_colormap/issues/7

## Context

`marine_colormap` has `TransferParams::min`/`max` as plain floats — no concept of
auto (data-driven) vs. manual (operator-fixed) range. Outliers (e.g. backscatter
band 2: max 925, mean 0.16, stddev 6.2) collapse all structure when auto-range
maps to [0, 925]. This Part-1 PR adds a Qt-free `RangeModel` to the core and a
"quality/warning" palette; Part 2 (separate run, separate PR) adds the interactive
`marine_colormap_widgets` package with the Qt colorbar widget.

Existing palettes: grayscale, bronze, thermal, viridis, turbo. No quality/warning
ramp today.

## Approach

1. **Add `RangeModel` to `transfer.hpp`/`transfer.cpp`** — Augments (does not replace
   or wrap) `TransferParams`. `RangeModel` tracks `RangeMode::{Auto, Manual}` and
   resolves to `(lo, hi)` that callers place into `TransferParams::min/max` (or GPU
   uniforms `u_min`/`u_max`). In `Auto` mode, `update_auto(float, float)` tracks
   data-driven range. In `Manual` mode, `set_manual(float, float)` fixes it;
   `reset()` returns to Auto. Exposes `lo()`, `hi()`, `mode()`, and a convenience
   `normalize(float)` that delegates to `::normalize(value, lo(), hi())`. No change
   to `TransferParams` fields; consumers update `params.min/max` from the model.

2. **Add `quality` palette to `src/palette.cpp`** — `even("quality", {green, yellow,
   red})` for the uncertainty/warning use case (t=0=good/green, t=1=bad/red). Appended
   at index 5 per the append-only, name-keyed registry contract.

3. **Write `docs/decisions/0001-range-model.md`** — New ADR for the `RangeModel` API
   decision (Auto/Manual model, relationship to `TransferParams`, widget tier deferred
   to Part 2). This is marine_colormap's first own ADR; creates `docs/decisions/`.

4. **Update `test/test_transfer.cpp`** — Add tests: auto↔manual mode switch, clamp
   behavior when `Manual` range is exceeded, `normalize()` consistency, degenerate range,
   `reset()` returns to `Auto`.

5. **Update `test/test_palette.cpp`** — Verify `quality` is in the registry at the
   expected name; sample at t=0 is greenish, t=1 is reddish.

6. **Update `README.md`** — Add `RangeModel` to the API section and note the `quality`
   palette; update palette list.

## Files to Change

| File | Change |
|------|--------|
| `include/marine_colormap/transfer.hpp` | Add `RangeMode` enum and `RangeModel` class |
| `src/transfer.cpp` | Implement `RangeModel` |
| `src/palette.cpp` | Append `quality` palette at index 5 |
| `test/test_transfer.cpp` | Range model unit tests |
| `test/test_palette.cpp` | Quality palette registry test |
| `README.md` | Document `RangeModel` and `quality` palette |
| `docs/decisions/0001-range-model.md` | New ADR (creates `docs/decisions/`) |

## Principles Self-Check

| Principle | Consideration |
|---|---|
| Only what's needed | `RangeModel` is minimal — augments TransferParams, no new deps. Quality palette is one entry. No widget in Part 1. |
| A change includes its consequences | Tests, README, and ADR updated in the same PR; Part-2 widget consumer noted as follow-on |
| Capture decisions, not just implementations | ADR-0001 in `docs/decisions/` records the Auto/Manual API design rationale |
| Test what breaks | Range model tests cover the auto↔manual boundary and clamp behavior — the cases field use depends on |
| Improve incrementally | Staged into two PRs: range model first (this), then widget (Part 2) |

## ADR Compliance

| ADR | Triggered | How addressed |
|---|---|---|
| Workspace ADR-0001 (adopt ADRs) | Yes — new API decision | New `docs/decisions/0001-range-model.md` captures the range model design |
| Project ADR-0001 (shared colormap) | Yes — new capability in Tier 1 | `RangeModel` is Qt-free, consistent with the Tier 1 contract; ADR-0001 addendum cross-ref will be added from `unh_marine_autonomy` in a follow-on |
| ADR-0008 (ROS 2 conventions) | Yes — modifying a ROS 2 package | No new deps introduced; ament export stays clean (no Qt/Ogre leak) |

## Consequences

| If we change... | Also update... | Included in plan? |
|---|---|---|
| `TransferParams` shape | GPU consumers re-bake (no shape change here) | N/A — TransferParams unchanged |
| Palette registry index | Consumers persisting by index silently remap | README note; palette.hpp append-only contract holds (new palette appends at 5) |
| `normalize()` signature | GPU shader mirrors it | Not changed; RangeModel delegates to existing `normalize()` |

## Open Questions

- Should `RangeModel` expose a `clamp(float value) const` helper for the uncertainty
  overlay sentinel case (values beyond manual range → clamp to LUT end), or is that
  handled implicitly by the existing `normalize()` raw-value contract + GPU clamp?
  (Existing `normalize()` already returns raw <0 or >1; callers handle clamping. If no
  explicit `clamp()` is needed, remove this question after implementation.)

## Estimated Scope

Two PRs total for #7: this PR (Part 1 — range model, Qt-free core) + Part 2 (marine_colormap_widgets + interactive colorbar widget, separate run). This PR is "Part of #7" — does NOT close #7.
