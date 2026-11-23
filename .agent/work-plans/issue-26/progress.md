---
issue: 26
---

# Issue #26 — Port marine_colormap to ROS 2 Rolling; build marine_colormap_widgets against Qt 5 or Qt 6

## Issue Review
**Status**: complete
**When**: 2026-11-23 13:25 -05:00
**By**: Claude Code Agent (claude-sonnet-5-5)

**Issue**: #26

### Scope Assessment

**Well-scoped?** Yes. One CMake file and one `package.xml` for the widgets package, one CI workflow, no source change expected. Acceptance ("Done when") is checkable, and the Qt rule is settled by the owner.
**Right repo?** Yes. Both packages and the CI live here; `camp` (rolker/camp#248) is correctly out of scope.
**Serves**: none written down for this project (the issue serves the owner's Rolling port, rolker/unh_marine_autonomy#415, and unblocks rolker/camp#248); the repo has no design document, only `docs/decisions/` ADR-0001 and ADR-0002 (ADR-0002 is the Qt5 widget decision, see below).
**Design picture?** No change to how the parts fit. The one thing that changes a recorded decision is ADR-0002 (Qt 5 widget), see ADR table.
**Dependencies**: none blocking. Blocks rolker/camp#248 (Qt 6 on Rolling). camp is the only consumer found under `rolling/*/src`: it does `find_package(marine_colormap)` and `find_package(marine_colormap_widgets)`, links `marine_colormap::marine_colormap` and `marine_colormap_widgets::marine_colormap_widgets` (CMakeLists.txt lines 369, 386) and uses `marine_colormap` types in the raster/vector layers. `marine_web_view` and `marine_simulation` only mention the name in a comment and a test that reads a file from this repo's source tree by path.

**State of the repository, checked.** Packages: `marine_colormap` (no Qt, no ROS beyond ament) and `marine_colormap_widgets`. CI: `.github/workflows/ci.yml`, one Jazzy job on `ros:jazzy-ros-base`, triggers `jazzy` only, `rosdep install ... -r`. Branches `rolling` and `jazzy` exist. On this machine (Rolling, GCC 15, CMake 4.2) `./build.sh --packages-up-to marine_colormap marine_colormap_widgets` builds both packages clean today and `./test.sh` passes (269 tests, 0 failures, 27 skipped across the workspace). The only output is the CMake `cmake_minimum_required(VERSION 3.8)` deprecation warning in both packages. The widget library links `libQt5*.so.5`, as the issue says: Rolling builds today, but with the wrong Qt. So the failure to fix is Qt 5 versus the Qt 6 that rqt/camp use on Rolling, not a compile break. Nothing was built against Qt 6.

### Principle Alignment

| Principle | Status | Notes |
|---|---|---|
| Only what's needed | Watch | The `qt_gui_cpp` build dependency exists only to pick the Qt major (cost accepted by owner). On a Jazzy CI `ros-base` image it also pulls `qt_gui_cpp`, `pluginlib`, `tinyxml2` and Qt 5 through rosdep; note the extra install time, no action. |
| A change includes its consequences | Watch | Text that says Qt5 must change with the code: `package.xml` description ("Qt5 widgets ..."), the comment in `package.xml` about `qtbase5-dev`, the CI file header comment and the Qt 5 wording in the READMEs and ADR-0002. See Consequences. |
| Verify before claiming | Watch | Qt 5 is checked by CI only and Qt 6 only locally; the "Done when" already requires saying so plainly. The Rolling CI job is the only automatic Qt 6 check, so it must run the widget test under `QT_QPA_PLATFORM=offscreen` (already set in the test) and must prove the Qt major chosen (for example print or assert it at configure time). |
| Test what breaks | Watch | Existing widget test exercises the Qt 6 build path only if the rolling job actually links Qt 6; add a configure-time check or a `ldd`/`QT_VERSION_MAJOR` assertion rather than trusting the selection. |

### ADR Applicability

| ADR | Triggered | Notes |
|---|---|---|
| This repo ADR-0002 (Qt5 widgets) | Yes | The decision says Qt 5; the port makes it "the Qt major of ROS's Qt packages". Record it as an addendum or new ADR in `docs/decisions/` as part of the work, not a silent change. |
| Workspace ADR-0013 (progress.md) | Yes | This entry; the plan and later phases append to the same timeline. |
| Workspace ADR-0017 (design is the current picture) | No | Project has no design document. |
| Other workspace ADRs | No | Nothing in the workspace layer changes. |

### Consequences

- `marine_colormap_widgets/package.xml`: description, the `qtbase5-dev` comment, and the dependency list. A conditional `<depend>` on `$ROS_DISTRO` would be a proxy for the same fact the rule avoids; the plan should say how rosdep gets the Qt development package (through `qt_gui_cpp`'s own dependency, or explicitly) so a Jazzy `rosdep install` still gets `qtbase5-dev` and its offscreen plugin.
- `ament_export_dependencies(marine_colormap Qt5Widgets)` must export the major actually chosen, or camp's `find_package(marine_colormap_widgets)` will pull the wrong Qt into its configure.
- `README.md`, `marine_colormap/README.md` and `docs/decisions/0002-colorbar-widget.md` mention Qt 5 (checked by grep); update what is stale.
- CI: the file header comment, `branches`, the Jazzy job gated to `jazzy`, a Rolling job gated to `rolling`, `rosdep install` without `-r` in both. Check that a `ros:rolling-ros-base` image exists for the Ubuntu 26.04 base before planning on it.
- The backport to `jazzy` is by cherry-pick, so the fix commits must not reference Rolling-only things the Jazzy CI cannot parse.

### Recommendations

- State in the plan how the selection is written: `find_package(qt_gui_cpp REQUIRED)` first, then `if(TARGET Qt6::Widgets)` pick Qt 6, else `find_package(Qt5 ...)`; link `Qt::Widgets` (defined by Qt 5.15 and Qt 6) or the `Qt${QT_VERSION_MAJOR}::Widgets` form; keep the existing `QT_VERSION` guard in `colormap_legend_widget.cpp`. The survey in the workspace notes verified this ordering for `rqt_gui_cpp`, and `qt_gui_cpp`'s Rolling extras file does `find_package(Qt6 REQUIRED ...)`; the plan should confirm the same holds for `qt_gui_cpp` on Rolling and read it from the Jazzy branch for Qt 5.
- Make the Qt-selection result visible (a `message(STATUS ...)` line) so CI logs show which major each job built, and have the issue's "what was and was not checked for Qt 5" statement cite it.
- Record ADR-0002's change (Qt major follows ROS's Qt packages) in the same pull request.
- Decide in the plan whether `cmake_minimum_required(VERSION 3.8)` is left alone (issue says seen and left; CMake 4.x only warns today) and list it in the follow-up issue with the other "seen and left" items.
- Keep the pull request's records: since the issue's records ride in the PR, the plan should say the PR description carries the Qt 5 checked / not checked statement.

### Actions
- [ ] State in the plan how the selection is written: `find_package(qt_gui_cpp REQUIRED)` first, then `if(TARGET Qt6::Widgets)` pick Qt 6, else `find_package(Qt5 ...)`; link `Qt::Widgets` (defined by Qt 5.15 and Qt 6) or the `Qt${QT_VERSION_MAJOR}::Widgets` form; keep the existing `QT_VERSION` guard in `colormap_legend_widget.cpp`. The survey in the workspace notes verified this ordering for `rqt_gui_cpp`, and `qt_gui_cpp`'s Rolling extras file does `find_package(Qt6 REQUIRED ...)`; the plan should confirm the same holds for `qt_gui_cpp` on Rolling and read it from the Jazzy branch for Qt 5.
- [ ] Make the Qt-selection result visible (a `message(STATUS ...)` line) so CI logs show which major each job built, and have the issue's "what was and was not checked for Qt 5" statement cite it.
- [ ] Record ADR-0002's change (Qt major follows ROS's Qt packages) in the same pull request.
- [ ] Decide in the plan whether `cmake_minimum_required(VERSION 3.8)` is left alone (issue says seen and left; CMake 4.x only warns today) and list it in the follow-up issue with the other "seen and left" items.
- [ ] Keep the pull request's records: since the issue's records ride in the PR, the plan should say the PR description carries the Qt 5 checked / not checked statement.
