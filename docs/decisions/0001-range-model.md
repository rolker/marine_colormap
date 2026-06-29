# ADR-0001: Auto/Manual range model beside `TransferParams`

## Status

Accepted

> **Numbering note.** This is **marine_colormap's own** ADR-0001 — the first
> decision recorded *inside this package's* `docs/decisions/`. It is distinct
> from the cross-cutting **project ADR-0001** "shared scalar colormap"
> (`unh_marine_autonomy/docs/decisions/0001-shared-scalar-colormap.md`), which
> defines the tiered colormap contract this package implements. They share the
> number "0001" only because each ADR set numbers from 1; cite the repo to
> disambiguate. This decision is consistent with — and refines — that project
> ADR-0001's Tier 1 (Qt-free core) contract.

## Context

`marine_colormap` exposes `TransferParams::min`/`max` as plain `float`s. They
are the range over which a scalar value is normalized to `[0, 1]` before the
palette is indexed, but the struct carries **no notion of where those numbers
came from** — whether they were computed from the data or fixed by an operator.

That distinction matters in the field. Auto-ranging over a band with an outlier
(issue #7's example: backscatter band 2 with max 925, mean 0.16, stddev 6.2)
maps the whole `[0, 925]` onto the palette and collapses all real structure into
the first fraction of the ramp. The fix an operator needs is to **pin** the
range to a sane window and have incoming data stop re-widening it — i.e. a
mode flag plus the policy that goes with each mode. `TransferParams` is the
wrong home for that: it is a passive value object copied into GPU uniforms, and
adding mode state to it would change its shape for every consumer (re-bake,
serialization) for a concern most of them don't have.

This is the first of two PRs for #7. This PR (Part 1) adds the Qt-free range
model and a warning palette to the core. Part 2 adds the interactive colorbar
widget in a **new `marine_colormap_widgets` package** (see *Widget tier*).

## Decision

**Add a `RangeModel` that sits *beside* `TransferParams`, not wrapping or
replacing it.** It owns the auto-vs-manual distinction and resolves to a
`(lo, hi)` extent the caller copies into `params.min`/`max` (or the GPU
`u_min`/`u_max` uniforms). `TransferParams` is unchanged.

API (`marine_colormap/transfer.hpp`):

- `enum class RangeMode { Auto, Manual }`.
- `RangeModel` with:
  - `update_auto(float min, float max)` — in `Auto`, set the tracked extent to
    the data's `[min, max]`. **A no-op in `Manual`**, so a pinned range is never
    re-widened by an outlier frame.
  - `set_manual(float lo, float hi)` — pin the range and switch to `Manual`.
  - `reset()` — return to `Auto`; tracking resumes on the next `update_auto()`.
  - `lo()`, `hi()`, `mode()` getters.
  - `normalize(float)` — delegates to the shared free
    `normalize(value, lo(), hi())`.

**Auto tracks the latest frame (replace, not accumulate).** `update_auto`
replaces the extent rather than growing a running min/max. A persistent
accumulated range would make a single outlier permanent — the opposite of what
#7 needs; per-frame replacement lets the range follow the data.

**Normalization stays single-sourced.** `RangeModel::normalize` calls the same
free `normalize()` the CPU `lookup()` and the GLSL shader use, so CPU, GPU and
model agree by construction. It returns the **raw** position (may be `< 0` or
`> 1`); callers clamp, exactly as today.

**No `clamp()` helper is added** (resolves the plan's open question). The
existing `normalize()` returns raw out-of-range positions by contract and
`apply_response()` / `Palette::sample()` already clamp to `[0, 1]`; the GPU path
clamps at the LUT sampler. A `RangeModel::clamp()` would duplicate that with no
caller needing it. If a concrete use appears (e.g. an uncertainty sentinel that
must distinguish "beyond range" from "at the end"), add it then, with the case
that motivates it.

### Inverted range and reversal

`set_manual(lo, hi)` and `update_auto(min, max)` enforce the invariant
**`lo() <= hi()`** by ordering their inputs (`std::minmax`), rather than
rejecting them or emitting the all-zeros a negative span would otherwise
produce. An inverted input — e.g. an operator dragging the Part-2 colorbar's
min/max handles past each other — is therefore normalized to a usable range; it
is **not** interpreted as a request to *reverse* the colormap.

**Reversal** (mapping high data values to the low end of the ramp) is a
**deliberate future feature**, to be added as an **explicit toggle** (e.g. a
`reversed` flag), not inferred from `lo > hi`. Overloading an inverted range to
mean "reverse" is ambiguous — it cannot distinguish an accidental handle-cross
from an intentional flip — so the model keeps the `lo <= hi` invariant, the
Part-2 widget will clamp handles so they cannot cross, and reversal, when
wanted, will be requested explicitly. (Operator decision, 2026-06-29.)

### `quality` warning palette

A `quality` palette (`green → yellow → red`, `t=0` good → `t=0.5` caution →
`t=1` bad) is appended to the registry at index 5. It is the
**bathy-uncertainty warning ramp** called for by issue #7's operator
requirement — tracked scope, not an unrelated addition. It follows the
existing **append-only, name-keyed** registry contract, so existing indices are
untouched; consumers should persist a palette by name.

### Widget tier (Part 2)

The interactive colorbar widget is **out of scope for this package**: it pulls
in Qt, which the core deliberately avoids (project ADR-0001 Tier 1). It will
land in a **new `marine_colormap_widgets` package** in Part 2, keeping the core
dependency-free. That package's own design (widget API, Qt target structure)
will be recorded in **its own ADR in Part 2**; this ADR records only the
package-boundary decision (widgets are a separate package, not a Qt target
added to `marine_colormap`).

## Consequences

**Positive:**
- The auto/manual concern lives in one small, testable type without disturbing
  `TransferParams` or its GPU consumers.
- Operators can pin an outlier band and have it stay pinned (`update_auto`
  no-op in `Manual`) — the concrete #7 failure is addressed.
- CPU/GPU normalization parity is preserved (single `normalize()` source).
- The core stays Qt-free; the widget cost is isolated to Part 2's package.

**Negative:**
- Callers must remember to copy `lo()`/`hi()` into `params.min`/`max` — the two
  are not auto-synced (deliberate: keeps `TransferParams` a passive value type).
- "Replace, not accumulate" means a transient narrow frame narrows the range; a
  consumer wanting a sticky range must track extents itself before calling
  `update_auto`.
- Two ADR-0001s now exist in different repos (mitigated by the numbering note
  above and the cross-reference).

## References

- Issue: https://github.com/rolker/marine_colormap/issues/7
- Project ADR-0001 (shared scalar colormap):
  `unh_marine_autonomy/docs/decisions/0001-shared-scalar-colormap.md`
- Workspace ADR-0001 (adopt ADRs), ADR-0008 (ROS 2 conventions),
  ADR-0012 (cross-reference addendums) in `project11/docs/decisions/`.
