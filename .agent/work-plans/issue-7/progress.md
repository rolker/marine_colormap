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
