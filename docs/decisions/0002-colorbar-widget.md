# ADR-0002: Interactive colorbar legend widget (`marine_colormap_widgets`)

## Status

Accepted

## Context

ADR-0001 added the Qt-free `RangeModel` (Auto/Manual range with a `lo <= hi`
invariant) to the `marine_colormap` core and explicitly deferred the interactive
colorbar widget — "the widget's own design ADR lands in Part 2" — because the
widget needs Qt and the core deliberately stays dependency-free (project ADR-0001
Tier 1).

This decision records the Part-2 design: an interactive colorbar legend a viewer
can drop in to **show** the current value→color mapping and let an operator
**drag** the range handles to pin a manual window over an outlier band (issue
#7's backscatter example: max 925, mean 0.16 — auto-range collapses all real
structure into the first sliver of the ramp).

The open questions from the plan are resolved here: Qt5 (Jazzy ships Qt5), and a
`reset()` slot is the testable reset path (a double-click is also wired as a UX
convenience, but the slot is the contract).

## Decision

### Separate package, not a Qt target in the core

The widget lands in a **new `marine_colormap_widgets` ament_cmake package** in the
same repo, depending on `marine_colormap` + Qt5 Widgets. The core gains no Qt
dependency. This is the package-boundary already committed in ADR-0001 (widgets
are a separate package, not a Qt target bolted onto the core).

**Repo restructured to a container layout.** Part 1 shipped the core package at
the *repo root* (its `package.xml` / `CMakeLists.txt` sat at the top level). A
second package cannot be **nested inside** the core package's directory: colcon's
recursive crawl **prunes a subtree once it identifies a package there**, so a
nested `marine_colormap_widgets/` would be invisible to `colcon build` in CI and
— critically — to this workspace's `ui_ws` layer (where camp / rqt consume it).
Verified directly: `colcon list --base-paths <repo>` reports only
`marine_colormap` until the core is moved out of the root.

The core package was therefore moved into a **`marine_colormap/` subdirectory**,
making the repo root a non-package **container** with two sibling package
directories — the same layout the workspace's other multi-package repo
(`rqt_operator_tools`) already uses (no root `package.xml`; one package per
subdir). `find_package(marine_colormap)` / `#include "marine_colormap/..."` are
install-space and unaffected by the source move; only the in-repo source layout
changes. (Considered and rejected: keeping the core at the root and adding a CI
symlink for the nested package — it leaves the `ui_ws` layer unable to discover
the widget, defeating the integration the package exists for.)

### `ColormapLegendWidget` owns its `RangeModel`

The widget **owns** a `RangeModel` by value; it exposes **no** `RangeModel&` /
`RangeModel*` accessor. Consumers read state through `lo()` / `hi()` / `mode()`
and react to the `rangeChanged(float, float)` signal. Rationale: a borrowed
reference would split ownership (who calls `set_manual` — the widget on drag, or
the consumer?) and invite the model to be mutated behind the widget's back,
desyncing the painted handles. A single owner with a value-out getter + a signal
is the simplest contract that keeps the view and the model consistent. (Considered
and rejected: taking a `RangeModel&` so several views share one model — deferred
until a concrete two-view consumer needs it; YAGNI.)

Public surface (`marine_colormap_widgets/colormap_legend_widget.hpp`):

- `setPalette(int index)` / `setLut(const std::vector<marine_colormap::Rgba8>&)`
  — appearance. A non-empty LUT (e.g. one a consumer baked with gain/contrast via
  `bake_lut`) takes precedence over the palette index for the drawn ramp.
- `setDomain(float min, float max)` — the value-axis extent the handles slide
  within. This is the **domain** (what values exist, e.g. the data extents),
  distinct from the model's **range** `[lo, hi]` (the active window inside the
  domain). A draggable window needs a domain wider than the window itself; the
  model carries only `lo/hi`, so the widget holds the domain. Defaults to
  `[0, 1]`. Inputs are ordered (`min <= max`) like the model's bounds.
- `updateAuto(float min, float max)` — feed data extents to the owned model
  (a no-op while Manual, per `RangeModel`); the only way to drive Auto mode from
  outside, since the model is not exposed.
- `lo()`, `hi()`, `mode()` — read-through getters.
- `reset()` **slot** — `RangeModel::reset()` (→ Auto) + `rangeChanged` + repaint.
- `rangeChanged(float lo, float hi)` **signal**.

The ramp is painted across the full domain by sampling
`clamp01(model_.normalize(value))` per pixel column, so values below `lo`
render as the floor color and above `hi` as the ceiling color — the gradient
is visibly compressed into the active `[lo, hi]` window, which is the whole
point of pinning.

### Handle-clamp policy (enforces ADR-0001 `lo <= hi`)

Dragging maps cursor-x → a candidate value in the domain, then clamps so the
handles **cannot cross**: the lo handle is capped at `hi - eps` and the hi
handle floored at `lo + eps`, with `eps = (domain span) * 1e-3`. The clamped
pair goes to `RangeModel::set_manual`, which switches the model to Manual.

This is the **widget-layer enforcement** ADR-0001 calls for: an inverted drag is
**prevented**, not reversed. The model would *also* order an inverted
`set_manual` (`std::minmax`), but clamping at the widget means the operator never
even sees the handles touch, and — critically — a crossed drag can't collapse the
range to a zero/degenerate span. Colormap **reversal** (high values → low end of
the ramp) remains a deliberate future feature behind an explicit toggle (ADR-0001
§ *Inverted range and reversal*), never inferred from a handle-cross.

### `rangeChanged(float lo, float hi)` signal contract

Emitted whenever the resolved `(lo, hi)` the widget would hand a renderer may have
changed: on every drag-move that calls `set_manual`, and on `reset()`. It carries
the model's **current** `lo()` / `hi()` (post-clamp). `reset()` emits even though
it changes only the *mode* (Auto vs Manual) and not the numeric bounds, because a
consumer copying `lo()/hi()` into `TransferParams` also needs the nudge to resume
auto-tracking. The signal is **not** emitted from `setPalette` / `setLut`
(appearance only) or `setDomain` (axis extent, not the range). Consumers wire it
to copy `lo()/hi()` into `params.min`/`max` (or the GPU `u_min`/`u_max` uniforms).

### Reset behavior

`reset()` calls `RangeModel::reset()` (returns to Auto; the extent is left as-is
until the next `updateAuto()` refreshes it from data — per ADR-0001), emits
`rangeChanged(lo(), hi())`, and repaints. A double-click on the widget is wired to
the same slot as a UX affordance, but the slot is the tested contract; the
double-click is a detail a consumer may rebind.

## Consequences

**Positive:**
- The core stays Qt-free; the Qt cost is isolated to the new package.
- One owner of the `RangeModel` keeps the painted handles and the emitted range
  in sync by construction.
- Handle-clamp upholds ADR-0001's `lo <= hi` at the point of interaction, so a
  crossed drag can never produce a degenerate range downstream.

**Negative / costs:**
- The repo is now multi-package, so **CI must build and test both packages**.
  `.github/workflows/ci.yml` is updated to `--packages-up-to
  marine_colormap_widgets` for the build and to add the package to the test
  `--packages-select`; `package.xml` declares Qt via the **rosdep key
  `qtbase5-dev`** (not a bare CMake name) so CI's `rosdep install` pulls the Qt
  dev headers + the offscreen platform plugin the test needs.
- `Q_OBJECT` + signals require moc; `CMakeLists.txt` sets `CMAKE_AUTOMOC ON`,
  else the build fails to link (undefined vtable / signal symbols).
- The `rangeChanged` signal contract is now a compatibility surface: consumer
  wiring (camp#142, `rqt_marine_sonar`, `rviz_sonar_image`) is **deferred** to
  those repos and not done here — this PR ships the widget + its tests only.

## References

- Issue: https://github.com/rolker/marine_colormap/issues/7
- ADR-0001 (this package — range model): `docs/decisions/0001-range-model.md`
  (§ *Inverted range and reversal*, § *Widget tier*).
- Project ADR-0001 (shared scalar colormap), Workspace ADR-0008 (ROS 2
  conventions) in `unh_marine_autonomy` / `project11`.
