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

## Checkpoint
**Status**: complete
**When**: 2026-11-23 13:18 -05:00
**By**: Claude Code Agent (claude-fable-5-2)
**Decided-by**: host (the issue review found the issue ready to plan; its open actions are recommendations for the planner and none changes what the work is for or its scope)
**After**: issue-actions
**Decision**: proceed

Read: the Issue Review, status complete, no principle row needing action. Its recommendations go to the planner as input, each to be settled in the plan with its reason: how the Qt major is selected (from what `qt_gui_cpp` brings, per the owner's rule "Follow ROS's Qt packages", never from `$ROS_DISTRO`), confirmed for Jazzy by reading upstream; what `ament_export_dependencies` exports so camp gets the same Qt; how rosdep still installs Qt on each distro; CI showing which Qt each job built; the record of the decision next to ADR-0002; the stale Qt 5 wording; which base image the Rolling job can use. `cmake_minimum_required(VERSION 3.8)` is left alone and listed for a follow-up issue.

## Plan Authored
**Status**: complete
**When**: 2026-11-23 13:27 -05:00
**By**: Claude Code Agent (claude-opus-5-5)
**Plan**: `.agent/work-plans/issue-26/plan.md` at `57d08d2`

The widget takes its Qt major from `qt_gui_cpp` (`qt_gui_cpp_USE_QT_MAJOR_VERSION` where set, as on Jazzy; else `TARGET Qt6::Widgets`, as on Rolling; else a fatal error), links and exports the versioned `Qt<N>::Widgets`/`Qt<N>Widgets`, declares both `qtbase5-dev` and `qt6-base-dev` with no distro condition, and adds a test that compares the built library's libQt<N>Core with `qt_gui_cpp`'s. Four commits (code, test, docs with a new ADR-0003, two-job CI), each cherry-picked to `jazzy`. Qt 6 is built and tested here (one test-only deprecation warning), and the Jazzy path is exercised through a stand-in only, so CI is the only real Qt 5 check.

## Plan Review
**Status**: complete
**When**: 2026-11-23 13:34 -05:00
**By**: Claude Code Agent (claude-opus-5-5)
**Verdict**: ready

**Issue**: #26 — Port marine_colormap to ROS 2 Rolling; build marine_colormap_widgets against Qt 5 or Qt 6
**Plan**: `.agent/work-plans/issue-26/plan.md` at `57d08d2`
**Branch**: `feature/issue-26`

### Evaluation

| Dimension | Verdict | Notes |
|---|---|---|
| Scope | Good | Two files of code, one new test script, docs, one workflow; four commits, each cherry-pickable to `jazzy`. |
| Issue alignment | Good | Covers every "Done when" item and every issue-review recommendation (selection written out, configure message, ADR record, `cmake_minimum_required` left and listed, Qt 5 checked/not-checked statement in the PR). Serves: "The owner's Rolling port (rolker/unh_marine_autonomy#415) and camp's Qt 6 port (rolker/camp#248). This repository has no goals or design document." Checked: no goals or design document in the repo; nothing breaks a stated project rule. |
| File targeting | Good | All Qt 5 mentions found by grep are listed (CMake, `package.xml`, CI header, both READMEs, ADR-0002 Status line). camp is correctly left alone. |
| Consequences | Good | Checked independently: camp's Jazzy CI clones `marine_colormap` `-b jazzy` and does `find_package(Qt5 ...)` explicitly, so after the backport it only gains an unused `qt6-base-dev`; camp is not built on Rolling (`rolling/ui` has no build or install; `marine_simulation` exec_depends on camp but unh_marine_simulation has no CI and #119 builds only asv_*/mbes_sim). Two small gaps below (findings 3, 4). |
| Principle alignment | Good | Only what's needed: smallest selection plus a 30-line test. Test what breaks: the test's oracle is `qt_gui_cpp`'s own binary, not our CMake variable. Verify before claiming: Qt 5 is stated as CI/stand-in only. |
| ADR compliance | Good | Repo ADR-0002 superseded on the Qt choice by a new ADR-0003 with a Status-line pointer only (cross-reference addendum rule). Workspace ADR-0013: entries via the persistence script. ADR-0017: no design document. |
| ROS conventions | Good | Versioned `Qt<N>::Widgets` link and `Qt<N>Widgets` export, rosdep keys rather than CMake names, `qt_gui_cpp` as a build-only dependency (consumers follow the exported Qt, not `qt_gui_cpp`). |

### Verification done for this review

- **Selection rule.** `qt_gui_cpp-extras.cmake` read upstream: `humble`, `jazzy`, `kilted` contain only `set(qt_gui_cpp_USE_QT_MAJOR_VERSION 5)`; `rolling` (and installed 3.0.4) has `find_package(python_qt_binding REQUIRED)` + `find_package(Qt6 REQUIRED COMPONENTS Widgets Core)` and no variable; `lyrical` sets it from `python_qt_binding_QT_MAJOR_VERSION` then finds that Qt. The variable was added in 2016 (`db6de46e`, "export information about using Qt5 so that downstream packages can use the same version"), i.e. a deliberate downstream interface, and was removed on `rolling` by "Removed Qt5 (#345)". If Jazzy's variable disappeared, no `Qt6::Widgets` exists at that point (nothing before `find_package(qt_gui_cpp)` finds Qt) and configure stops with the fatal error; if Rolling's extras stopped finding Qt 6, the same. A silently wrong pick would need some earlier `find_package` to define `Qt6::Widgets` while `qt_gui_cpp` used Qt 5, and the step-2 test would then fail. On a machine with both Qt installed the rule never consults what is installed. A scratch configure here (`find_package(qt_gui_cpp)` + the rule) printed major 6, Qt 6.10.2, `$<TARGET_FILE:qt_gui_cpp::qt_gui_cpp>` = `/opt/ros/rolling/lib/qt_gui_cpp.so`, `CMAKE_OBJDUMP=/usr/bin/objdump`. An equally simple single rule exists (read `qt_gui_cpp::qt_gui_cpp`'s `INTERFACE_LINK_LIBRARIES`: `Qt5::Widgets` on Jazzy through the plain `target_link_libraries`, `Qt6::Widgets;Qt6::Core;...` on Rolling), but it parses a target property, so it is no more of an interface than the variable; the plan's rule is fine.
- **Versionless targets.** `Qt5WidgetsConfig.cmake:251` creates `Qt::Widgets` only `if(... AND NOT TARGET Qt::Widgets)`; Qt 6's `Qt6WidgetsVersionlessAliasTargets.cmake` creates aliases only for names not yet defined (`_qt_internal_check_multiple_inclusion`). First major found wins, as the plan says. A consumer's `find_package(marine_colormap_widgets)` runs `find_package(Qt5Widgets)` on Jazzy and `find_package(Qt6Widgets)` on Rolling through the ament export, and does not find `qt_gui_cpp`.
- **rosdep.** Jazzy `qt_gui_cpp/package.xml`: `qtbase5-dev` is `build_depend` only; `python_qt_binding` (jazzy) likewise; packages.ros.org noble: `ros-jazzy-qt-gui-cpp` Depends has `libqt5core5t64` and no `-dev` Qt; `ros-jazzy-python-qt-binding` brings `libpyside2-dev`/`pyqt5-dev`/`python3-pyqt5`, none of which depends on `qtbase5-dev` (Ubuntu noble index). Rolling `qt_gui_cpp` has `<build_export_depend>qt6-base-dev`. So no dependency `qt_gui_cpp` exports gives the Qt development package on both distros, and without a distro condition both keys are needed. Both keys resolve to themselves on `ubuntu:noble` and `ubuntu:resolute` (`rosdep resolve`). The offscreen plugin is in `libqt5gui5t64` / `libqt6gui6` (`dpkg -S`), dependencies of `qtbase5-dev` / `qt6-base-dev`.
- **Proof test.** Jazzy's `qt_gui_cpp.so` NEEDs `libQt5Core.so.5` (its shlibs-derived Depends lists `libqt5core5t64` and no Widgets library); Rolling's NEEDs `libQt6Core.so.6` only; our current library NEEDs Widgets, Gui and Core. `objdump` comes from `ros-dev-tools` → `ros-build-essential` → `build-essential` in both containers.
- **CI.** `ros:rolling-ros-core` and `ros:rolling-ros-core-resolute` have the same digest (`365c44cbb918`, 2026-11-12). `ros-jazzy-ros-core` already includes `ament_cmake_gtest`, `ament_lint_auto` and `ament_lint_common`; what `ros-base` adds (geometry2, rosbag2, urdf, kdl and the like) is used by neither package. mru_transform's `rolling` run with the same shape is green.
- Not rebuilt here: the planner's Qt 6 build, test counts and mutation check were not repeated; the scratch configure above covers the selection and the test's inputs on Rolling.

### Findings

1. **[Principle alignment — Test what breaks]** (low) The step-2 test requires exactly one `libQt<N>Core` in each NEEDED list. That catches Qt 5 Core next to Qt 6 Core, but not a library whose other Qt entries (`libQt5Widgets`, `libQt5Gui`) disagree with its Core. Comparing the major of every `libQt<N>*` entry of our library (all must equal `qt_gui_cpp`'s Core major) costs one regex and closes that case; keep the comparison against `qt_gui_cpp`'s Core, since its Widgets entry is dropped as-needed.
2. **[ROS conventions — CI]** (low) The planned `rosdep install ... --from-paths $(colcon list --paths-only --packages-up-to marine_colormap_widgets)` drops two things the model workflow has: `--base-paths src`, and `set -euo pipefail` plus the empty-list check, which turn a failing or empty `colcon list` into a job failure instead of a `rosdep` call with no paths. Copy the model's step as it is.
3. **[Consequences]** (low) ADR-0003 should record two facts this review found, so a later reader does not re-derive them: the variable is upstream's declared downstream interface (2016) but Rolling already removed it, so the `TARGET Qt6::Widgets` branch is Rolling's only path and the fatal error is what a further upstream change produces; and the unconditional `qtbase5-dev` will stop resolving on Rolling when Rolling moves to an Ubuntu without Qt 5, which is the point at which that key is dropped.
4. **[Consequences]** (low) The plan's camp paragraph says camp "is not built on Rolling anywhere today". True, but the manifest puts `camp` and `marine_colormap` in the `simulation` group and `marine_simulation` (`rolling` branch) has `<exec_depend>camp</exec_depend>`, so a future `--packages-up-to marine_simulation` on Rolling pulls camp in. Name that path in the PR's camp note; it already fails on Rolling through `rqt_gui_cpp`, so nothing changes in kind.

### Open questions (facts for the host)

- QMouseEvent deprecation: test-only, a warning not an error, and not in "Done when"; the port rule favours leaving it and listing it. The one-line form `QMouseEvent(type, pos, pos, button, buttons, Qt::NoModifier)` exists, undeprecated, in both Qt 5.15 and Qt 6, so fixing it is also safe; either is consistent with the rule.
- Jazzy image `ros-core`: drops nothing this repository uses; it matches the ported repositories and exposes undeclared dependencies. Supported.
- Rolling image `ros:rolling-ros-core`: identical to `-resolute` today and used by the green models; pinning only matters at Rolling's next Ubuntu move. Supported.

### Summary

The selection rule, the versioned export, the two rosdep keys and the binary-oracle test all hold up against the upstream sources, package indexes and the installed Rolling files; every way the rule can go wrong is a configure error or a test failure, not a silent wrong Qt. The findings are small and can be taken during implementation. Ready.

### Recommended Actions

- [ ] Make the step-2 check compare every `libQt<N>*` NEEDED entry of `marine_colormap_widgets` with `qt_gui_cpp`'s Core major.
- [ ] Use mru_transform's rosdep step verbatim (`--base-paths src`, `set -euo pipefail`, empty-list check).
- [ ] In ADR-0003, record that Rolling's `qt_gui_cpp` already dropped the variable, and when the unconditional `qtbase5-dev` key must go.
- [ ] In the PR's camp note, name `marine_simulation`'s `exec_depend` on camp as the path by which camp would be built on Rolling.

## Checkpoint
**Status**: complete
**When**: 2026-11-23 13:40 -05:00
**By**: Claude Code Agent (claude-fable-5-2)
**Decided-by**: host (the plan review's verdict is ready; its four findings are low and each is taken during implementation; none changes scope or the owner's Qt rule)
**After**: plan
**Decision**: proceed

Read: the Plan Review at `57d08d2`, verdict ready, four low findings, all taken by the implementer: (1) the proof test compares the major of every `libQt<N>*` entry of our library with `qt_gui_cpp`'s Core major; (2) the CI rosdep step is the model workflow's as it is (`--base-paths src`, `set -euo pipefail`, the empty-list check); (3) ADR-0003 records that `qt_gui_cpp_USE_QT_MAJOR_VERSION` is upstream's declared downstream interface but is already gone on Rolling, so the `Qt6::Widgets` branch is Rolling's only path and the fatal error is what a further upstream change gives, and that the unconditional `qtbase5-dev` key is dropped when Rolling's Ubuntu no longer has Qt 5; (4) the pull request's camp note names the `simulation` group path (`marine_simulation` has an `exec_depend` on camp). Host decisions on the planner's open questions: the test-only `QMouseEvent` constructor deprecation (Qt 6.4) is left alone and listed for the follow-up issue, under the owner's no-big-changes rule (the alternative was a one-line fix valid on both Qt majors); the Jazzy job moves to `ros:jazzy-ros-core` like the other ported repositories; the Rolling job uses `ros:rolling-ros-core` unpinned, like the models. Both Qt development keys are listed unconditionally because nothing on both distros installs only the Qt in use without a distro condition.

## Implementation
**Status**: complete
**When**: 2026-11-23 13:47 -05:00
**By**: Claude Code Agent (claude-opus-5-5)
**Branch**: feature/issue-26 at `e16a976`

The plan's four commits, in its order, as amended by the Checkpoint; each ends "Backport to jazzy.":

1. `16c0654` fix(widgets): take the Qt major from qt_gui_cpp — `find_package(qt_gui_cpp)`; `qt_gui_cpp_USE_QT_MAJOR_VERSION` if set, else `TARGET Qt6::Widgets` → 6, else a fatal error naming both expected signals; `find_package(Qt<N>)`, link `Qt<N>::Widgets`, export `Qt<N>Widgets`; configure prints `marine_colormap_widgets: Qt <ver> (major <N>, from qt_gui_cpp)`. `package.xml`: `qt_gui_cpp` build_depend, `qt6-base-dev` beside `qtbase5-dev` (no condition), description and comments.
2. `47549e0` test(widgets): check the library uses qt_gui_cpp's Qt major — `test/check_same_qt_major.cmake` + `ament_add_test(test_same_qt_major ...)`: objdump -p on our library and on `$<TARGET_FILE:qt_gui_cpp::qt_gui_cpp>`; exactly one `libQt<N>Core` in qt_gui_cpp's NEEDED list; **every** `libQt<N>*` entry of our library must have that major (review finding 1).
3. `ccbbaa8` docs: ADR-0003 (rule, rejected rules, both facts from the review: Rolling already removed the upstream variable so `Qt6::Widgets` is its only path and a further change gives the fatal error; `qtbase5-dev` is dropped when Rolling's Ubuntu has no Qt 5), ADR-0002 Status-line pointer only, both READMEs.
4. `e16a976` ci: jobs `jazzy` (`ros:jazzy-ros-core`) and `rolling` (`ros:rolling-ros-core`, `if: (github.base_ref || github.ref_name) != 'jazzy'`), triggers `rolling` and `jazzy`; rosdep step as mru_transform's (`--base-paths src`, `set -euo pipefail`, empty-list check, no `-r`); build with `--event-handlers console_cohesion+` so the Qt configure line is in the log; test step sources only `/opt/ros/<distro>`; header rewritten.

### Verification (Rolling, this machine: Ubuntu 26.04, Qt 5.15.18 and Qt 6.10.2 both installed)

- `./build.sh --parallel-workers 4 --packages-up-to marine_colormap marine_colormap_widgets`: `Summary: 2 packages finished [0.39s]` (configure line on the first build: `-- marine_colormap_widgets: Qt 6.10.2 (major 6, from qt_gui_cpp)`).
- `./test.sh --parallel-workers 4 --packages-select marine_colormap marine_colormap_widgets`: `100% tests passed, 0 tests failed out of 14`, `100% tests passed, 0 tests failed out of 8`, `Summary: 273 tests, 0 errors, 0 failures, 27 skipped` (planner's baseline 269; the new script adds `test_same_qt_major` and one case each to copyright and lint_cmake; the 27 skips are cppcheck, as before).
- `objdump -p libmarine_colormap_widgets.so`: `libQt6Widgets.so.6`, `libQt6Gui.so.6`, `libQt6Core.so.6`; no Qt 5. Exported extras: `Qt6Widgets`. Rolling's `qt_gui_cpp.so` NEEDs `libQt6Core.so.6`.
- Widget gtest run 10 times offscreen (`QT_QPA_PLATFORM=offscreen`): 10 × `[  PASSED  ] 5 tests.`
- Qt 6 is picked because of `qt_gui_cpp`: through a stand-in `qt_gui_cppConfig.cmake` (variable = 5, `qt_gui_cpp::qt_gui_cpp` imported from a scratch library NEEDing `libQt5Core.so.5`) the unchanged tree configured `Qt 5.15.18 (major 5, from qt_gui_cpp)`, linked `libQt5*` only, and passed the widget gtest 5/5 and `test_same_qt_major` (`Qt major 5 as qt_gui_cpp: libQt5Widgets.so.5;libQt5Gui.so.5;libQt5Core.so.5`). The same stand-in without the variable gave the fatal error.
- Proof test failing cases: (a) forced Qt 5 build on Rolling (`set(MCW_QT_MAJOR 5)` in a scratch copy): `Qt major mismatch: libQt5Widgets.so.5 is 5, qt_gui_cpp's is 6`; (b) a constructed mixed library NEEDing `libQt6Core.so.6` and `libQt5Widgets.so.5` (Core agrees, Widgets does not): `Qt major mismatch: libQt5Widgets.so.5 is 5, qt_gui_cpp's is 6`.
- Rolling job rehearsed under `env -i` (only `/opt/ros/rolling/setup.bash` sourced, a copy of the tree as `$GITHUB_WORKSPACE`, `../ws` beside it): Init rosdep (already initialised), Create workspace, Install ROS dependencies (`#All required rosdeps installed successfully`, both package paths listed), Build (configure line printed under console_cohesion+), Test, Test results (`Summary: 273 tests, 0 errors, 0 failures, 27 skipped`). Not rehearsed: `apt-get update` and the `ros-dev-tools` install (need root; installed here already), `actions/checkout`, `rosdep update` (the existing cache was used), the container image itself; `ROS_DISTRO=rolling` was passed by hand (the ros images set it); `--parallel-workers 4` was added locally only.

### Unverified on Jazzy

Not installed here (Ubuntu 26.04, no Docker). Unverified: `find_package(qt_gui_cpp)` and the variable on real Jazzy, `$<TARGET_FILE:qt_gui_cpp::qt_gui_cpp>` and its NEEDED list there, Qt 5.15.13 with GCC 13, rosdep resolving both Qt keys on noble in the container, and the `ros-core` image having everything the jobs use. Expected first Jazzy CI run: rosdep installs `qtbase5-dev` and `qt6-base-dev`; the build log shows `marine_colormap_widgets: Qt 5.15.x (major 5, from qt_gui_cpp)`; `test_same_qt_major` prints `Qt major 5 as qt_gui_cpp: ...libQt5...`; cppcheck runs for real (not skipped). The rolling job is the run rehearsed above.

### Not changed, as decided

The test-only `QMouseEvent` deprecation warning (Qt 6.4) at `test/test_colormap_legend_widget.cpp:62` is left alone (Checkpoint).

### camp between this merge and camp's own port (for the pull request)

Nothing changes on Jazzy: `qt_gui_cpp` says 5, the export is `Qt5Widgets` as today (rosdep also installs `qt6-base-dev`, unused). On Rolling, camp's configure stops at CMake's generate step with the Qt major disagreement (`INTERFACE_QT_MAJOR_VERSION of "Qt6::Widgets" does not agree with the value of QT_MAJOR_VERSION already determined`) for any camp target that links `marine_colormap_widgets` and camp's own `Qt5::` targets: a loud failure, not a silent mixed process, and the same state camp is already in on Rolling through `rqt_gui_cpp`. camp is not built on Rolling anywhere today (no `rolling` branch, no CI, no `rolling/ui` install), but the `simulation` group reaches it through `marine_simulation`'s `<exec_depend>camp</exec_depend>`, so a future `--packages-up-to marine_simulation` on Rolling would pull camp in and hit this. camp's own port is rolker/camp#248, which must take the same rule (ADR-0003).

### Left alone, for the follow-up issue

- `cmake_minimum_required(VERSION 3.8)` in both packages (CMake 4 deprecation warning).
- The Qt 6.4 `QMouseEvent` constructor deprecation at `marine_colormap_widgets/test/test_colormap_legend_widget.cpp:62` (both-version form: `QMouseEvent(type, pos, pos, button, buttons, Qt::NoModifier)`).
- cppcheck skipped on Rolling (`ament_cppcheck` with cppcheck 2.19.0, #415).
- `CONTRIBUTING.md` points at a missing `.agents/README.md` and says pull requests go to `jazzy`.
- The CI installs `ros-dev-tools` without `git`, so `actions/checkout` falls back to the REST API download (as before this change; nothing here needs a `.git`).

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-11-23 13:58 -05:00
**By**: Claude Code Agent (claude-opus-5-5)
**Verdict**: approved

**Branch**: feature/issue-26 at `401658c`
**Base**: origin/rolling
**Depth**: Deep (reason: 200+ changed lines; CI workflow is an override-trigger file)
**Must-fix**: 0 | **Suggestions**: 2
**Round**: 1 | **Ship**: recommended — no must-fix findings; remaining suggestions can be applied or tracked

Specialists: static analysis (xmllint ok, YAML parses, lint_cmake/copyright/xmllint tests pass; no yamllint/actionlint on this machine), governance, plan drift (all planned files changed, nothing extra; the test checks every libQt entry of the widget rather than requiring exactly one Core there, which is stricter on mixing and fine), Claude adversarial (no must-fix), Gemini (6 suggestions; 2 kept, rest false positive or covered), Codex (2 suggestions; 1 kept).

Checked against upstream qt_gui_core: jazzy extras set qt_gui_cpp_USE_QT_MAJOR_VERSION 5 only and jazzy exports qt_gui_cpp::qt_gui_cpp (ament_export_targets), so the Qt 5 branch is taken on Jazzy even with Qt 6 dev files installed and `$<TARGET_FILE:qt_gui_cpp::qt_gui_cpp>` resolves there; humble/kilted/lyrical/rolling extras match ADR-0003. Jazzy itself not built here; the first CI run is the first Jazzy evidence.

Re-run here: build of both packages ok; `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`; installed export names `Qt6::Widgets` only; check_same_qt_major.cmake fails on libQt5Widgets (mismatch) and libz (no libQt entry), passes on libQt6Widgets, against Rolling's qt_gui_cpp.so.

### Findings
- [ ] (suggestion) Guard `qt_gui_cpp_USE_QT_MAJOR_VERSION` against a defined-but-empty or non-numeric value (Lyrical assigns it from python_qt_binding) so it reaches the clear fatal error, e.g. `MATCHES "^[0-9]+$"`; harmless today since Lyrical's own find fails first (Gemini, Claude adversarial) — `marine_colormap_widgets/CMakeLists.txt:35`
- [ ] (suggestion) ADR wording: CMake's generate-time Qt major disagreement fires only when one target links both majors; a consumer that finds Qt 5 and links this library into a different target than its Qt 5 code is not stopped. Say "a consumer target that links both" (Codex) — `docs/decisions/0003-qt-major-follows-ros.md:80`

## Checkpoint
**Status**: complete
**When**: 2026-11-23 13:55 -05:00
**By**: Claude Code Agent (claude-fable-5-2)
**Decided-by**: host (the pre-push review approved round 1 with no must-fix; the Qt 5 path cannot be built on the development machine, so the pushed branch's first Jazzy run is the first real evidence for it)
**After**: publish
**Decision**: publish

Read: the Local Review (Pre-Push) at `401658c`, verdict approved, ship recommended, two suggestions open: guard `qt_gui_cpp_USE_QT_MAJOR_VERSION` against an empty or non-numeric value so it reaches the clear fatal error (`marine_colormap_widgets/CMakeLists.txt:35`); ADR-0003 should say the generate-time Qt major disagreement stops "a consumer target that links both" (`docs/decisions/0003-qt-major-follows-ros.md:80`). They stay open and go through triage after publish together with what the first CI run shows, so one fix pass serves both. This is a project issue, so the host runs no full pre-commit pass. The pull request says `Part of #26`.

## Integrated Review
**Status**: complete
**When**: 2026-11-23 13:58 -05:00
**By**: Claude Code Agent (claude-opus-5-5)

**PR**: #27 at `ca8ed12`
**Sources**: 2 (Local Review (Pre-Push) @ `401658c` (covers `ca8ed12` via bookkeeping), CI rollup run 45243563566 @ `ca8ed12`); no GitHub reviews, inline or conversation comments, no Copilot review requested and no Copilot check-run
**Cross-source confirmations**: 0
**CI**: all-pass

CI, first run for a pull request into `rolling` and first real Qt 5 evidence:
- build-and-test (jazzy), `ros:jazzy-ros-core`, 1m44s, pass. Configure: `marine_colormap_widgets: Qt 5.15.13 (major 5, from qt_gui_cpp)`. `test_same_qt_major` passed against `/opt/ros/jazzy/lib/libqt_gui_cpp.so`: `Qt major 5 as qt_gui_cpp: libQt5Widgets.so.5;libQt5Gui.so.5;libQt5Core.so.5`. Build `Summary: 2 packages finished [17.2s]`; test `Summary: 2 packages finished [4.99s]` (14/14 and 8/8 passed); `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`. cppcheck skipped (2.13.0, "known performance issues").
- build-and-test (rolling), `ros:rolling-ros-core`, 1m48s, pass. Configure: `marine_colormap_widgets: Qt 6.10.2 (major 6, from qt_gui_cpp)`. `test_same_qt_major` passed against `/opt/ros/rolling/lib/qt_gui_cpp.so`: `Qt major 6 as qt_gui_cpp: libQt6Widgets.so.6;libQt6Gui.so.6;libQt6Core.so.6`. Build `Summary: 2 packages finished [10.9s]`; test `Summary: 2 packages finished [3.68s]` (14/14 and 8/8 passed); `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`. cppcheck skipped (2.19.0, same reason).
- Note for the follow-up issue: cppcheck skips on Jazzy too (2.13.0), not only on Rolling.

### Findings
- [x] (suggestion, Local Review (Pre-Push) @ `401658c`) Valid. Today `if(DEFINED qt_gui_cpp_USE_QT_MAJOR_VERSION)` takes any value. Checked with CMake under `cmake_minimum_required(VERSION 3.8)`: an empty value makes the unquoted `set(MCW_QT_MAJOR ...)` unset the variable, so `find_package(Qt REQUIRED ...)` runs CMake's legacy FindQt module (CMP0084 OLD) and stops with "Could NOT find Qt3 (missing: QT_QT_LIBRARY QT_INCLUDE_DIR)"; `5.15` stops with "Could not find a package configuration file provided by \"Qt5.15\""; `6` where `qt_gui_cpp` failed to bring Qt 6 still calls `find_package(Qt6 REQUIRED ...)`, which finds `qt6-base-dev` (listed unconditionally) or fails with CMake's standard Qt6 message: the major is the one `qt_gui_cpp` named, so that case is correct. All three stop the build; the first two with a misleading message instead of the ADR-0003 fatal error. Smallest guard holding on both distros: replace the first condition with `if(qt_gui_cpp_USE_QT_MAJOR_VERSION MATCHES "^[0-9]+$")` (checked: `5` and `6` match; empty, `5.15` and undefined fall through, so Rolling still takes the `TARGET Qt6::Widgets` branch and Jazzy still takes `5`), and add the value to the fatal message ("qt_gui_cpp_USE_QT_MAJOR_VERSION='${qt_gui_cpp_USE_QT_MAJOR_VERSION}'") — `marine_colormap_widgets/CMakeLists.txt:35`
- [x] (suggestion, Local Review (Pre-Push) @ `401658c`) Valid. CMake's Qt major compatibility check (`COMPATIBLE_INTERFACE_STRING QT_MAJOR_VERSION`) is per target: it fires only when one target's link closure has both majors, so "A consumer that finds Qt 5 and links this library stops at CMake's generate step" overstates it. Reword to "A consumer target that links both this library and Qt 5 stops at CMake's generate step; a consumer that keeps them in separate targets is not stopped" — `docs/decisions/0003-qt-major-follows-ros.md:80`

### False positives

## Checkpoint
**Status**: complete
**When**: 2026-11-23 13:59 -05:00
**By**: Claude Code Agent (claude-fable-5-2)
**Decided-by**: host (two valid suggestions are open and each has a small, named fix; merging past open findings is the owner's answer only)
**After**: findings
**Decision**: address

Read: the Integrated Review of pull request 27 at `ca8ed12`: two valid suggestions, no false positives, CI all-pass (run 45243563566: Jazzy configures `Qt 5.15.13 (major 5, from qt_gui_cpp)` and Rolling `Qt 6.10.2 (major 6, from qt_gui_cpp)`; `test_same_qt_major` passes on both against the distro's own `qt_gui_cpp` library; 273 tests, 0 errors, 0 failures, 27 skipped on each). Addressed: (1) the selection accepts `qt_gui_cpp_USE_QT_MAJOR_VERSION` only when it is a whole number (`MATCHES "^[0-9]+$"`), and the fatal message gives the value it saw, so an empty or odd value reaches the ADR-0003 error instead of a misleading CMake one; one code commit for backport. (2) ADR-0003's consumer sentence says that a consumer target linking both this library and the other Qt major stops at CMake's generate step, and one that keeps them in separate targets is not stopped; one docs commit for backport. The triage's note that cppcheck skips on Jazzy too (2.13.0) goes into the follow-up issue draft.

## Implementation
**Status**: complete
**When**: 2026-11-23 14:00 -05:00
**By**: Claude Code Agent (claude-opus-5-5)

**PR**: #27 at `da97d6f`
**Addressed**: Integrated Review at `ca8ed12` (2026-11-23 13:58 -05:00)
**Commits**: ddb8c8b, da97d6f

### Actions
- [x] Qt major variable accepted only when it matches `^[0-9]+$`; the fatal error prints the value seen — `marine_colormap_widgets/CMakeLists.txt:35`
- [x] ADR-0003 consumer sentence: a consumer target linking both this library and the other Qt major stops at generate; separate targets are not stopped — `docs/decisions/0003-qt-major-follows-ros.md:80`

### Verification
Scratch configures (Rolling, stand-in `qt_gui_cppConfig.cmake` in the session scratchpad, removed):
- value `5`, no Qt 6: `Qt 5.15.18 (major 5, from qt_gui_cpp)`
- undefined, `Qt6::Widgets` present: `Qt 6.10.2 (major 6, from qt_gui_cpp)`; real tree via `./build.sh --parallel-workers 4 --packages-up-to marine_colormap marine_colormap_widgets`: same line, 2 packages finished
- empty, no Qt 6: fatal error `... got qt_gui_cpp_USE_QT_MAJOR_VERSION=''.`
- `5.15`, no Qt 6: fatal error `... qt_gui_cpp_USE_QT_MAJOR_VERSION='5.15'.`
- empty or `5.15` with `Qt6::Widgets` present: Qt 6 branch (`Qt 6.10.2`). Kept: the target exists only because `qt_gui_cpp` found Qt 6, so taking 6 follows ROS's Qt packages, and it is the smallest change the review named.

`./test.sh --parallel-workers 4 --packages-select marine_colormap marine_colormap_widgets`: `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`.
PR body camp paragraph already states the per-target case ("a camp target that links this library together with camp's own Qt 5 targets stops at CMake's generate step"); consistent, no edit needed.

## Local Review
**Status**: complete
**When**: 2026-11-23 14:03 -05:00
**By**: Claude Code Agent (claude-opus-5-5)
**Verdict**: approved
**Dispatch**: resumed (agent a7275ff554a09f8ff, resume 1 of 3)

**PR**: #27 at `53efa45`
**Depth**: Deep by whole-diff classification, run as a narrow check per handoff (reason: prior round had no must-fix; scope is the two pre-push suggestions and the fix diff `e16a976..da97d6f`, which touches `marine_colormap_widgets/CMakeLists.txt` and `docs/decisions/0003-qt-major-follows-ros.md` only)
**Must-fix**: 0 | **Suggestions**: 0
**Cross-model**: skipped (narrow check after a round with no must-fix, per handoff)

Checked:
- `ddb8c8b`: `if(qt_gui_cpp_USE_QT_MAJOR_VERSION MATCHES "^[0-9]+$")` closes the pre-push suggestion. An undefined name is compared as its literal text and does not match, so Rolling falls through to `TARGET Qt6::Widgets`. The fatal message now prints the value it saw.
- Set-but-invalid value with `Qt6::Widgets` present takes Qt 6: accepted. Before the check, only these packages are found:
  - `ament_cmake`, which defines no Qt targets.
  - `marine_colormap`, which finds only ament_cmake and exports no Qt.
  - `qt_gui_cpp`, whose exported deps are `pluginlib;TinyXML2` and whose Rolling extras find `python_qt_binding` (its extras only set a cache variable; the sip and shiboken helpers that mention Qt are not included by its config) and then Qt 6.

  So `Qt6::Widgets` can exist there only because `qt_gui_cpp` found Qt 6. That follows ROS's Qt packages, and `test_same_qt_major` still checks the result against `qt_gui_cpp`'s binary.
- `da97d6f`: the ADR-0003 consumer sentence now states the per-target scope of CMake's Qt major check, which closes the pre-push suggestion.
- CI run 45244390573 on `53efa45` passed both jobs.
  - Jazzy (1m25s):
    - `marine_colormap_widgets: Qt 5.15.13 (major 5, from qt_gui_cpp)`
    - `Qt major 5 as qt_gui_cpp: libQt5Widgets.so.5;libQt5Gui.so.5;libQt5Core.so.5`
    - `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`
  - Rolling (2m2s):
    - `marine_colormap_widgets: Qt 6.10.2 (major 6, from qt_gui_cpp)`
    - `Qt major 6 as qt_gui_cpp: libQt6Widgets.so.6;libQt6Gui.so.6;libQt6Core.so.6`
    - `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`

### Findings
- [ ] No issues found. LGTM.

## Integrated Review
**Status**: complete
**When**: 2026-11-23 14:06 -05:00
**By**: Claude Code Agent (claude-opus-5-5)
**Dispatch**: resumed (agent adb9dc09d038ec2d4, resume 1 of 3)

**PR**: #27 at `f4f8335`
**Sources**: 2 (Local Review @ `53efa45` (covers `f4f8335` via bookkeeping), CI rollup run 45245198134 @ `f4f8335`); no GitHub reviews, inline or conversation comments, no Copilot review requested and no Copilot check-run
**Cross-source confirmations**: 0
**CI**: all-pass

Both findings of the Integrated Review at `ca8ed12` are closed by code I read at this head. `ddb8c8b` changes the check to `if(qt_gui_cpp_USE_QT_MAJOR_VERSION MATCHES "^[0-9]+$")` and the fatal error now prints the value — `marine_colormap_widgets/CMakeLists.txt:35`. `da97d6f` limits the generate-step stop to "a consumer target that links both this library and the other Qt major" — `docs/decisions/0003-qt-major-follows-ros.md:81`. The pre-push entry at `401658c` is stale under `sources`: its two findings are the same two, so they are closed by the same commits. The Local Review at `53efa45` has no findings; its `- [ ] No issues found. LGTM.` box is not a finding.

CI run 45245198134 at `f4f8335` (same code as `53efa45`):
- build-and-test (jazzy), 1m20s, pass:
  - `marine_colormap_widgets: Qt 5.15.13 (major 5, from qt_gui_cpp)`
  - `test_same_qt_major` passed: `Qt major 5 as qt_gui_cpp: libQt5Widgets.so.5;libQt5Gui.so.5;libQt5Core.so.5`
  - Build `Summary: 2 packages finished [11.4s]`
  - Test `Summary: 2 packages finished [3.58s]` (14/14 and 8/8 passed)
  - `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`
  - cppcheck skipped (2.13.0)
- build-and-test (rolling), 1m26s, pass:
  - `marine_colormap_widgets: Qt 6.10.2 (major 6, from qt_gui_cpp)`
  - `test_same_qt_major` passed: `Qt major 6 as qt_gui_cpp: libQt6Widgets.so.6;libQt6Gui.so.6;libQt6Core.so.6`
  - Build `Summary: 2 packages finished [7.96s]`
  - Test `Summary: 2 packages finished [2.76s]` (14/14 and 8/8 passed)
  - `Summary: 273 tests, 0 errors, 0 failures, 27 skipped`
  - cppcheck skipped (2.19.0)

### Findings
- [x] No open findings: both earlier suggestions are fixed (`ddb8c8b`, `da97d6f`) and nothing new is raised at `f4f8335`.

### False positives
