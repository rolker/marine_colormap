# Plan: Colormap range model + colorbar legend widget (Part 2 — widget)

## Issue

https://github.com/rolker/marine_colormap/issues/7

## Context

Part 1 (merged, PR #8) added `RangeModel` + `quality` palette to the Qt-free
core. Part 2 adds the interactive colorbar legend widget in a **new
`marine_colormap_widgets` ROS 2 package**, completing #7. The repo becomes
multi-package; CI (`build-and-test`) must build and test both packages.

Core API available: `RangeModel{Auto/Manual, update_auto, set_manual, reset,
lo()/hi()/mode(), normalize()}`. Handles MUST clamp (cannot cross); inverted
drag is prevented at the widget, not reversed — the model's `lo <= hi`
invariant (ADR-0001) is enforced at the UI layer.

## Approach

1. **ADR-0002: widget design** — write `docs/decisions/0002-colorbar-widget.md`
   covering: widget API (owns `RangeModel` vs. takes reference), Qt
   package-vs-target decision (separate package, not a Qt target in core),
   handle-clamp policy, `rangeChanged` signal contract, and reset-to-auto
   behaviour. Commit with the package skeleton (step 2).

2. **New `marine_colormap_widgets` package skeleton** — create
   `marine_colormap_widgets/package.xml` (ament_cmake, depends on
   `marine_colormap` + `Qt5Widgets`) and `marine_colormap_widgets/CMakeLists.txt`
   (find Qt5, ament targets, install, test scaffold). Commit together with ADR.

3. **`ColormapLegendWidget` header** —
   `marine_colormap_widgets/include/marine_colormap_widgets/colormap_legend_widget.hpp`:
   `class ColormapLegendWidget : public QWidget` with
   `Q_OBJECT`; internal `RangeModel`; `setPalette(int index)`, `setLut(...)`,
   `lo()/hi()/mode()`, `reset()` public API; `rangeChanged(float lo, float hi)`
   signal; `paintEvent`, `mousePressEvent`, `mouseMoveEvent`,
   `mouseReleaseEvent` overrides. No public `RangeModel*` reference — widget
   owns the model (simplest contract; consumers read `lo()/hi()` and the
   signal).

4. **`ColormapLegendWidget` implementation** —
   `marine_colormap_widgets/src/colormap_legend_widget.cpp`:
   - `paintEvent`: QPainter fills the colormap ramp (sample LUT or palette across
     widget width), draws the value axis with tick marks, draws draggable handle
     markers at `lo()` and `hi()` positions.
   - Mouse press: hit-test which handle (lo or hi) is under cursor; begin drag.
   - Mouse move during drag: compute candidate new value from x position; clamp
     so lo handle cannot exceed `hi() - epsilon` and hi handle cannot go below
     `lo() + epsilon`; call `model_.set_manual(new_lo, new_hi)`; emit
     `rangeChanged`; `update()`.
   - Double-click on widget background (not a handle): call `model_.reset()`;
     emit `rangeChanged`; `update()`. (Or expose a `reset()` slot — plan for
     both, implement slot for testability.)
   - `reset()` public slot: same as double-click path.

5. **Tests** — `marine_colormap_widgets/test/test_colormap_legend_widget.cpp`
   using `ament_cmake_gtest` + `QApplication` with `QT_QPA_PLATFORM=offscreen`:
   - *drag → Manual + signal*: simulate `mousePressEvent` on lo-handle pixel,
     `mouseMoveEvent` to a new x, `mouseReleaseEvent`; verify `mode() == Manual`,
     `rangeChanged` emitted with the expected values, and handles didn't cross.
   - *reset → Auto*: call `widget.reset()`; verify `mode() == Auto`.
   - *clamp prevents crossing*: drag lo handle past hi; verify `lo() < hi()`.

6. **CI multi-package verification** — confirm `build-and-test` workflow
   handles both packages (colcon discovers them automatically in the workspace;
   no CI file changes expected, but verify workflow file covers the new package
   before merging).

## Files to Change

| File | Change |
|------|--------|
| `docs/decisions/0002-colorbar-widget.md` | New ADR: widget design, Qt-package structure, clamp policy, signal contract |
| `marine_colormap_widgets/package.xml` | New package: ament_cmake, depends on marine_colormap + Qt5Widgets |
| `marine_colormap_widgets/CMakeLists.txt` | New: find Qt5, widget shared lib, install, gtest scaffold |
| `marine_colormap_widgets/include/marine_colormap_widgets/colormap_legend_widget.hpp` | New: `ColormapLegendWidget` header |
| `marine_colormap_widgets/src/colormap_legend_widget.cpp` | New: widget implementation |
| `marine_colormap_widgets/test/test_colormap_legend_widget.cpp` | New: offscreen-Qt drag + reset + clamp tests |

## Principles Self-Check

| Principle | Consideration |
|---|---|
| Human control and transparency | Draggable handles + reset-to-auto give operator explicit control; `rangeChanged` signal makes state visible to consumers |
| Capture decisions, not just implementations | ADR-0002 records widget API, package structure, clamp policy, and signal contract before implementation |
| A change includes its consequences | Tests included in same PR; CI multi-package build confirmed before merge |
| Only what's needed | Widget owns model (no bridge object); no consumer wiring in this PR (camp#142, rqt_marine_sonar deferred explicitly) |
| Test what breaks | Three tests cover the three risky interactions: drag→Manual, reset→Auto, clamp-prevents-crossing |
| Workspace vs. project separation | New package is in the `marine_colormap` project repo, not the workspace |

## ADR Compliance

| ADR | Triggered | How addressed |
|---|---|---|
| ADR-0001 (marine_colormap — range model) | Yes — widget must enforce `lo <= hi` | Clamp at widget; inverted drag prevented, not reversed (per ADR-0001 § Inverted range) |
| Workspace ADR-0001 (adopt ADRs) | Yes — widget design is a new decision | Write ADR-0002 as part of this PR |
| ADR-0008 (ROS 2 conventions) | Yes — new ROS 2 package | `package.xml` format 3, `ament_cmake`, proper deps, Apache-2.0 headers |

## Consequences

| If we change... | Also update... | Included in plan? |
|---|---|---|
| Add `marine_colormap_widgets` package | CI must build/test both packages | Yes — step 6 verifies before merge |
| Widget `rangeChanged` signal contract | Consumer wiring (camp#142, rqt_marine_sonar, rviz_sonar_image) | No — deferred, documented in ADR-0002 Consequences |
| Handle-clamp policy | ADR-0001 back-reference | Yes — ADR-0002 cites ADR-0001 § Inverted range |

## Open Questions

- [ ] Qt version: Qt5 or Qt6? (Jazzy ships Qt5; confirm CI container has Qt5Widgets dev headers; if Qt6 available, plan for both via CMake Qt-version compat.) Assume Qt5 unless CI environment says otherwise.
- [ ] Reset trigger UX: double-click on widget background vs. separate reset button? Plan implements a `reset()` slot testable without mouse simulation; the UX trigger is a separate UI detail the consumer can wire.

## Estimated Scope

Single PR (completes #7). Two commits minimum: (1) ADR + package skeleton, (2) widget implementation + tests. CI must go green before merge.
