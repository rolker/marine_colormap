---
issue: 7
---

# Issue #7 — Colormap range model + colorbar legend widget (Part 2)

## Issue Review
**Status**: complete
**When**: 2026-06-29 12:00 +00:00
**By**: Claude Code Agent (Claude Sonnet)

**Issue**: #7
**Comment**: (best-effort post follows this entry; not recorded inline)
**Scope verdict**: well-scoped

### Actions
- [ ] Widget design ADR (marine_colormap `docs/decisions/0002-colorbar-widget.md`) must be written and committed as part of this PR — marine_colormap ADR-0001 explicitly committed "the widget's own design ADR lands in Part 2"; cover: widget API, Qt-package structure, handle-clamp policy, signal contract.
- [ ] Automated widget interaction tests are required (not optional) — confirm that offscreen-Qt (`QT_QPA_PLATFORM=offscreen` or `qOffscreenSurface`) works in the CI container; drag→Manual+range-change signal and reset→Auto must both be exercised automatically.
- [ ] New `marine_colormap_widgets` package must follow ROS 2 package conventions (workspace ADR-0008): proper `package.xml` with `ament_cmake` build type and Qt5/Qt6 + `marine_colormap` dependencies, `CMakeLists.txt` using `find_package(Qt5 ...)` patterns; confirm the repo's `build-and-test` CI handles the multi-package build.

## Plan Authored
**Status**: complete
**When**: 2026-06-29 15:00 +00:00
**By**: Claude Code Agent (Claude Sonnet)

**Plan**: `.agent/work-plans/issue-7/plan.md` at `75c076b`
**Branch**: feature/issue-7 at `75c076b`
**Phases**: single

### Open questions
- [ ] Qt version: Qt5 or Qt6? Assume Qt5 (Jazzy); confirm CI container has Qt5Widgets dev headers.
- [ ] Reset trigger UX: double-click vs. separate reset button? Plan implements a `reset()` slot testable without mouse simulation; UX trigger wired by consumer.

## Plan Review
**Status**: complete
**When**: 2026-06-29 08:05 +00:00
**By**: Claude Code Agent (Claude Opus)

**Plan**: `.agent/work-plans/issue-7/plan.md` at `75c076b`
**PR**: PR-less (`--issue` / file-path review; `gh` unauthenticated in this environment, so issue/PR metadata read from local plan + ADR-0001 + review-issue entry)
**Verdict**: changes-requested

### Findings
- [ ] (must-fix) CI does not auto-discover the new package — `.github/workflows/ci.yml` hardcodes `colcon build --packages-up-to marine_colormap` and `colcon test --packages-select marine_colormap`, so `marine_colormap_widgets` would be neither built nor tested in CI. The plan's claim that "colcon discovers them automatically in the workspace; no CI file changes expected" is wrong; `ci.yml` MUST be edited to cover the new package (e.g. `--packages-up-to marine_colormap_widgets` for build, add the package to `--packages-select` for test) and added to the Files-to-Change table. This is exactly review-issue Action 3 ("confirm the repo's build-and-test CI handles the multi-package build"). — `plan.md:104`, `plan.md:65-68`
- [ ] (suggestion) Qt SHARED lib with `Q_OBJECT`/signals needs moc — `CMakeLists.txt` step (plan.md:76) should enable `set(CMAKE_AUTOMOC ON)` (or `qt5_wrap_cpp`), else the widget fails to link (undefined vtable/signal symbols). Call it out so implementation doesn't rediscover it. — `plan.md:76`
- [ ] (suggestion) Declare Qt with rosdep-resolvable keys — `package.xml` should depend on `qtbase5-dev` (rosdep key) rather than the bare CMake name "Qt5Widgets", so `rosdep install --from-paths` in the `ros:jazzy-ros-core` CI container actually pulls the Qt dev headers + offscreen platform plugin needed for the offscreen test. — `plan.md:75`
- [ ] (suggestion) Update package README — the top-level `README.md` describes only the Qt-free core and teases the Part-2 widget as future; adding the package should update it to point at `marine_colormap_widgets` and ADR-0002 ("a change includes its consequences" — docs). Not in the Files-to-Change table. — `plan.md:70-79`

## Implementation
**Status**: complete
**When**: 2026-06-29 08:27 +00:00
**By**: Claude Code Agent (Claude Opus)

**Branch**: feature/issue-7 at `1add5c0` (3 ordered commits on top of `5ed4cf5`)

### Delivered
- **ADR-0002** (`docs/decisions/0002-colorbar-widget.md`): widget owns its
  `RangeModel` (no exposed reference), the separate-package decision + the
  **container-repo restructure** (below), handle-clamp policy (enforces ADR-0001
  `lo <= hi`; inverted drag prevented, not reversed), the
  `rangeChanged(float,float)` contract, and reset-to-Auto behavior.
- **New package `marine_colormap_widgets`** — `package.xml` (ament_cmake;
  `<depend>marine_colormap</depend>`; Qt via the rosdep key
  `<depend>qtbase5-dev</depend>`); `CMakeLists.txt` (`find_package(Qt5 ... Widgets)`,
  `set(CMAKE_AUTOMOC ON)`, SHARED widget lib, install/export, gtest scaffold with
  `ENV QT_QPA_PLATFORM=offscreen`).
- **`ColormapLegendWidget`** (header + cpp): `QWidget`+`Q_OBJECT` owning a
  `RangeModel`; `setPalette`/`setLut`/`setDomain`/`updateAuto`, `lo()/hi()/mode()`,
  `reset()` slot, `rangeChanged` signal; `paintEvent` (ramp via `model_.normalize`
  per column + value axis + lo/hi handle tabs); mouse press/move/release drag with
  cross-clamp → `set_manual` → `rangeChanged` → `update()`. Double-click also
  resets (UX; the slot is the tested contract).
- **Tests** (`test/test_colormap_legend_widget.cpp`, `ament_cmake_gtest` +
  `QApplication` under offscreen Qt): drag→Manual + signal emitted + handles
  didn't cross; `reset()`→Auto (+1 emission); drag-past-cross → `lo() < hi()`.

### Plan-review findings addressed (all 4)
- **(MUST-FIX) ci.yml** — now builds `--packages-up-to marine_colormap_widgets`
  and tests `--packages-select marine_colormap marine_colormap_widgets`; rosdep
  resolves `qtbase5-dev`.
- **(suggestion) `CMAKE_AUTOMOC ON`** — set; the Q_OBJECT header is also listed as
  a library source so AUTOMOC scans it (it lives in `include/`, not beside the
  `.cpp` — without this the lib's moc was empty and the test failed to link
  `staticMetaObject`/vtable).
- **(suggestion) `qtbase5-dev` rosdep key** — used in `package.xml` (not the bare
  CMake name).
- **(suggestion) README** — repo root README rewritten as a multi-package
  container overview pointing at both packages + ADR-0001/0002; the core README
  moved with the core and trimmed to a sibling pointer.

### Structural change (beyond the literal plan — required for correctness)
The plan assumed colcon would auto-discover a nested `marine_colormap_widgets/`.
It does **not**: colcon prunes a subtree once it identifies a package, so with the
core package at the repo root the widget package was invisible to `colcon list`
(verified) — it would be built/tested by neither CI nor the `ui_ws` layer. Fixed
by moving the core into a **`marine_colormap/` subdirectory**, making the repo a
container with two sibling packages (the `rqt_operator_tools` layout).
`find_package`/`#include` paths are install-space and unaffected.

### Build/test status (in-container, Qt5 + offscreen plugin present)
Clean from-scratch `colcon build --packages-up-to marine_colormap_widgets
--cmake-args -DBUILD_TESTING=ON` → **2 packages finished**. `colcon test
--packages-select marine_colormap marine_colormap_widgets
--return-code-on-test-failure` → **2 packages finished, no failures**.
`colcon test-result --verbose` → **132 tests, 0 errors, 0 failures, 17 skipped**.
Widget gtest: **tests="3" failures="0" errors="0"** (offscreen Qt ran in-container,
no host verification needed). Not pushed (host performs pushes).

### Next step
Open PR for #7 (Part 2). Deferred to consumer repos (per ADR-0002): wiring
`rangeChanged` into camp#142 / `rqt_marine_sonar` / `rviz_sonar_image`.

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-06-29 08:50 +00:00
**By**: Claude Code Agent (Claude Opus)
**Verdict**: approved

**Branch**: feature/issue-7 at `a7c260c`
**Mode**: pre-push
**Depth**: Deep (reason: 1374 insertions / 29 files, new ADR-0002, CI workflow change)
**Must-fix**: 0 | **Suggestions**: 5
**Round**: 1 | **Ship**: recommended — no must-fix; clean static analysis + two Deep adversarial passes, only minor robustness/cosmetic suggestions

### Findings
- [ ] (suggestion) Unguarded `palette_count()>0` assumption in `color_at` (std::clamp UB if ever 0) — `marine_colormap_widgets/src/colormap_legend_widget.cpp:152`
- [ ] (suggestion) `updateAuto()`/`reset()` emit `rangeChanged` unconditionally; `updateAuto` emits even as a Manual-mode no-op — `marine_colormap_widgets/src/colormap_legend_widget.cpp:81`
- [ ] (suggestion) `QMouseEvent::localPos()` deprecated since Qt 5.15; prefer `position()` — `marine_colormap_widgets/src/colormap_legend_widget.cpp:196`
- [ ] (suggestion) Test links `Qt5::Widgets` redundantly (already transitive via the lib target) — `marine_colormap_widgets/CMakeLists.txt:73`
- [ ] (suggestion) `ros:jazzy-ros-base` is a more conventional CI base than `ros-core` — `.github/workflows/ci.yml`

## Implementation
**Status**: complete
**When**: 2026-06-29 09:10 +00:00
**By**: Claude Code Agent (Claude Opus)
**Commit**: 39e65ee on `feature/issue-7`

Addressed all 5 suggestions from the Round-1 Local Review (pre-push, approved):

- **(suggestion) Guard palette count** — `color_at` (`colormap_legend_widget.cpp`)
  now early-returns a transparent `QColor(0,0,0,0)` when
  `marine_colormap::palette_count() == 0`, before the `std::clamp` index math.
  No more UB / crash on an empty or unset palette.
- **(suggestion) Emit `rangeChanged` only on change** — `updateAuto()` and
  `reset()` now snapshot `(lo, hi, mode)` before mutating the model and emit
  `rangeChanged` + repaint **only** if any of the three changed. `updateAuto()`
  in Manual mode (a model no-op) now neither emits nor repaints; `reset()`
  emits only when it actually leaves Manual (or otherwise changes state).
- **(suggestion) Modernize deprecated mouse API** — added a version-guarded
  `eventLocalPos()` helper: `QMouseEvent::position()` on Qt6, `localPos()` on
  Qt5. (Note: in Qt 5.15 `position()` does not exist and `localPos()` is not
  yet deprecated — a bare `position()` fails to compile against the Jazzy Qt5
  headers, so the guard is required to honor the suggestion while keeping the
  Qt5 build green.) Both call sites updated.
- **(suggestion) Drop redundant Qt link** — removed the explicit
  `Qt5::Widgets` from the test target's `target_link_libraries`; it is
  transitive via the widget lib's public link.
- **(suggestion) CI base image** — `ci.yml` base switched from
  `ros:jazzy-ros-core` to `ros:jazzy-ros-base`. rosdep still resolves
  `qtbase5-dev` + offscreen plugin on ros-base.

### Build/test status (in-container, Qt5 5.15.13 + offscreen plugin present)
Clean from-scratch `colcon build --packages-up-to marine_colormap_widgets
--cmake-args -DBUILD_TESTING=ON` → **2 packages finished**. `colcon test
--packages-select marine_colormap marine_colormap_widgets
--return-code-on-test-failure` → **2 packages finished, 100% (7/7 ctest) passed**.
`colcon test-result --verbose` → **132 tests, 0 errors, 0 failures, 17 skipped**.
Widget gtest (offscreen Qt): **tests="3" failures="0" errors="0"**. Not pushed
(host performs pushes).

### Next step
Open PR for #7 (Part 2) — Round-1 review approved; all 5 suggestions now resolved.

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-06-29 09:12 +00:00
**By**: Claude Code Agent (Claude Opus)
**Verdict**: approved

**Branch**: feature/issue-7 at `0edc04f`
**Mode**: pre-push
**Depth**: Deep (reason: new ADR-0002 + new ROS 2 package + multi-package CI change)
**Must-fix**: 0 | **Suggestions**: 1
**Round**: 2 | **Ship**: recommended — 0 must-fix; all 5 Round-1 suggestions resolved (commit 39e65ee); clean cpplint/cppcheck + two Deep adversarial passes; one claimed must-fix rejected as false positive

### Findings
- [ ] (suggestion) `updateAuto()` doesn't reconcile model extent with the widget domain; auto-extents wider than `setDomain()` clamp handles to axis edges (graceful, no UB) — consider documenting domain ⊇ extents or widening domain — `marine_colormap_widgets/src/colormap_legend_widget.cpp:93`

### Notes
- Rejected (false positive): Lens-B claim that `ament_export_dependencies(... Qt5Widgets)` must be `Qt5`. `Qt5WidgetsConfig.cmake` exists; `find_package(Qt5Widgets)` is valid and exporting the component is more correct than bare `Qt5` (which wouldn't define the `Qt5::Widgets` target).
