# Plan: Port marine_colormap to ROS 2 Rolling; build marine_colormap_widgets against Qt 5 or Qt 6

## Issue

https://github.com/rolker/marine_colormap/issues/26

## Context

`rolling` and `jazzy` are at the same commit (`82771cc`). On Rolling (Ubuntu 26.04, GCC 15, CMake 4.2.3,
Qt 5.15.18 and 6.10.2 both installed) both packages build and pass today (269 tests, 0 failures, 27
skipped), but `libmarine_colormap_widgets.so` links `libQt5*.so.5`: the defect is the Qt major, not a
compile break. The owner's rule: **follow ROS's Qt packages**. The widget takes the major that
`qt_gui_cpp` uses on the distro it is built for, never from `$ROS_DISTRO` and never from what is
installed. `marine_colormap` has no Qt and needs no change.

The **27 skipped tests** are all `cppcheck` (24 in `marine_colormap`, 3 in the widgets package):
`ament_cppcheck` skips every file on Rolling ("cppcheck 2.19.0 performance issues", #415). Qt 6 does
not change that, and on Jazzy cppcheck runs for real.

### Evidence gathered for this plan

**What `qt_gui_cpp` tells a package that finds it**, read from `ros-visualization/qt_gui_core`:

| Branch | `qt_gui_cpp-extras.cmake` | Qt target after `find_package(qt_gui_cpp)` |
|---|---|---|
| `jazzy` (2.7.6), also `humble`, `kilted` | `set(qt_gui_cpp_USE_QT_MAJOR_VERSION 5)` and nothing else | none: Qt 5 is not found; exported deps are `pluginlib;TinyXML2` only |
| `rolling` (3.0.4, installed here) | `find_package(python_qt_binding REQUIRED)`, `find_package(Qt6 REQUIRED COMPONENTS Widgets Core)`; no variable | `Qt6::Widgets` |
| `lyrical` | sets the variable from `python_qt_binding`, then `find_package(Qt${...})` | the matching one |

So the variable is the explicit answer wherever it exists, and `TARGET Qt6::Widgets` is the answer on
Rolling.

**Built against Qt 6 here** (a scratch copy outside the source tree, CMake as in step 1): configure
printed `marine_colormap_widgets: Qt 6.10.2 (major 6, from qt_gui_cpp)`. The library and the test link
`libQt6*.so.6` only. Tests: 269, 0 failures, 27 skipped (the widget gtest 5 of 5 under offscreen Qt 6).
AUTOMOC works unchanged. **The library source compiled without warnings.** The one new diagnostic is
in the test:
`test/test_colormap_legend_widget.cpp:62:60: warning: 'QMouseEvent::QMouseEvent(QEvent::Type, const QPointF&, Qt::MouseButton, Qt::MouseButtons, Qt::KeyboardModifiers, const QPointingDevice*)' is deprecated: Use another constructor [-Wdeprecated-declarations]`
(deprecated since Qt 6.4). No errors. The only Qt 5 or Qt 6 API split in the source is the
`position()`/`localPos()` spot in `colormap_legend_widget.cpp:45`, already guarded. The CMake
deprecation warning for `cmake_minimum_required(VERSION 3.8)` is unchanged.

**The Jazzy branch of the selection**, exercised with a stand-in `qt_gui_cppConfig.cmake` that does
what Jazzy's does (it sets the variable to 5 and defines `qt_gui_cpp::qt_gui_cpp`): configure picked
Qt 5.15.18, the library links `libQt5*.so.5`, and the widget gtest passed 5 of 5. This is the local
Qt 5.15.18, not Jazzy's 5.15.13 with GCC 13.

**The camp case, reproduced with a scratch consumer:** a target that finds `marine_colormap_widgets`
(Qt 6 build), then does `find_package(Qt5 ...)`, links `Qt5::Widgets` and the widget library, and has
AUTOMOC on. CMake stops at the generate step:
`CMake Error: The INTERFACE_QT_MAJOR_VERSION property of "Qt6::Widgets" does not agree with the value of QT_MAJOR_VERSION already determined`.
The same run showed that `Qt::Widgets` became an alias of `Qt6::Widgets`, the first major found.

## Approach

Each code commit below is written to build on Jazzy and Rolling. Each is cherry-picked to `jazzy` in
the same order, and `.agent/` records are never mixed into them.

1. **Select the Qt major from `qt_gui_cpp`** (`CMakeLists.txt`, `package.xml`). This replaces
   `find_package(Qt5 ...)`:
   ```cmake
   find_package(qt_gui_cpp REQUIRED)
   if(DEFINED qt_gui_cpp_USE_QT_MAJOR_VERSION)   # Humble to Kilted, Lyrical
     set(MCW_QT_MAJOR ${qt_gui_cpp_USE_QT_MAJOR_VERSION})
   elseif(TARGET Qt6::Widgets)                   # Rolling: its extras find Qt 6
     set(MCW_QT_MAJOR 6)
   else()
     message(FATAL_ERROR "Cannot tell which Qt major qt_gui_cpp uses")
   endif()
   find_package(Qt${MCW_QT_MAJOR} REQUIRED COMPONENTS Widgets)
   message(STATUS "marine_colormap_widgets: Qt ${Qt${MCW_QT_MAJOR}Widgets_VERSION} (major ${MCW_QT_MAJOR}, from qt_gui_cpp)")
   ```
   - Link `Qt${MCW_QT_MAJOR}::Widgets`, and export `ament_export_dependencies(marine_colormap Qt${MCW_QT_MAJOR}Widgets)`.
     Checked on the Qt 6 build: the exported extras say `marine_colormap;Qt6Widgets`, and the target's
     interface says `Qt6::Widgets`.
   - **The versioned target, not `Qt::Widgets`.** Qt 5.15 and Qt 6 both define `Qt::Widgets`, but
     only for the first major found (`Qt5WidgetsConfig.cmake:252` skips it when it already exists).
     Exporting the versionless name would let a consumer that found Qt 5 first resolve our interface
     to Qt 5 with no error. The versioned name fails loudly instead (see the camp case above).
   - **The alternative** is the issue review's form: `if(TARGET Qt6::Widgets)`, else Qt 5. It works
     on both distros today, but on Jazzy it picks Qt 5 because a target is absent, and it would pick
     Qt 5 on any future `qt_gui_cpp` that states the major without finding Qt. The variable is what
     ROS's package says.
   - In `package.xml`, `qt_gui_cpp` becomes a `<build_depend>` (only CMake reads it, and consumers
     follow the exported Qt). `qtbase5-dev` stays and `<depend>qt6-base-dev</depend>` is added, with
     no condition. Rosdep then installs both Qt development packages on both distros. The build uses
     the one `qt_gui_cpp` names, so CI on each distro also tests the case that must not pick wrong
     (the other Qt installed).
   - Why both Qt keys are needed: Rolling's `qt_gui_cpp` brings `qt6-base-dev` (apt Depends), but
     Jazzy's does not bring `qtbase5-dev`. Its `package.xml` has that key as `build_depend` only, and
     the `ros-jazzy-qt-gui-cpp` Depends list (packages.ros.org, noble) has runtime Qt 5 libraries only.
   - The offscreen platform plugin comes with each `-dev` package (`libqt5gui5t64`, `libqt6gui6`).
   - Both keys are in rosdistro `base.yaml` (`qtbase5-dev`: all Ubuntu; `qt6-base-dev`: all except
     focal), and `rosdep resolve` gives both on noble and resolute.
   - **The alternative** is `condition="$ROS_DISTRO == ..."` keys: a smaller install, but they
     restate by distro name the fact the rule takes from `qt_gui_cpp`. Rejected.
   - Update the wording that goes with this commit: the `package.xml` description ("Qt5 widgets")
     and the dependency comment, and the CMake comments that say Qt5.

2. **Test that the library uses the same Qt as ROS's** (new `test/check_same_qt_major.cmake`, about
   30 lines, with the Apache-2.0 header; one `ament_add_test(... GENERATE_RESULT_FOR_RETURN_CODE_ZERO)`).
   The script runs `${CMAKE_OBJDUMP} -p` on `$<TARGET_FILE:marine_colormap_widgets>` and on
   `$<TARGET_FILE:qt_gui_cpp::qt_gui_cpp>`. It requires exactly one `libQt<N>Core` in each NEEDED
   list and fails if the majors differ.
   - Core is compared because Rolling's `qt_gui_cpp.so` NEEDs `libQt6Core.so.6` only (Widgets was
     dropped as-needed). Jazzy's apt Depends list `libqt5core5t64`.
   - The oracle is ROS's own binary, not our CMake variable, so the test is not circular.
   - Checked here: it passes on the Qt 6 build. With the major forced to 5, it fails with
     `Qt major mismatch: 5 vs qt_gui_cpp's 6`. `lint_cmake` accepts the script.
   - `objdump` is in binutils, which the C++ toolchain already needs, so there is no new key.
   - Kept separate from step 1 so that commit stays the fix alone.
3. **Record the decision** (`docs/`).
   - New `docs/decisions/0003-qt-major-follows-ros.md`: what was decided, the rejected rules ("Qt 6 if
     installed", "by distro name", `$ROS_DISTRO` conditions), the two-Qt-keys consequence, and that
     camp must follow the same rule.
   - ADR-0002: only a Status-line pointer ("Qt choice superseded by ADR-0003"), per the
     cross-reference-addendum rule ADR-0001 cites. Changing "Qt5" inside ADR-0002's Decision would
     reword an accepted decision.
   - `README.md` table row and `marine_colormap/README.md:61`: "Qt 5 or Qt 6 (the major ROS's
     `qt_gui_cpp` uses)".
4. **CI** (`.github/workflows/ci.yml`, the same file on both branches, modelled on `mru_transform`'s
   `rolling` workflow).
   - Triggers: `rolling` and `jazzy`.
   - Job `jazzy` on `ros:jazzy-ros-core`, and job `rolling` on `ros:rolling-ros-core` (Docker Hub:
     the same push as `rolling-ros-core-resolute`, 2026-11-12, so Ubuntu 26.04) with
     `if: (github.base_ref || github.ref_name) != 'jazzy'`.
   - `rosdep install --ignore-src -y --from-paths $(colcon list --paths-only --packages-up-to marine_colormap_widgets)`
     without `-r`.
   - Build with `--event-handlers console_cohesion+`, so the configure line naming the Qt major is in
     each job's log (colcon hides `STATUS` output by default).
   - The test step sources only `/opt/ros/<distro>/setup.bash`, not `install/setup.bash`.
   - Headless: the tests already set `QT_QPA_PLATFORM=offscreen` through `ament_add_gtest(... ENV ...)`,
     so no X server is needed.
   - Rewrite the header comment, which describes the Qt5-only setup today.
   - Moving the Jazzy job from `ros-base` to `ros-core` matches the ported repositories and exposes
     undeclared dependencies (cause 11). The alternative is to keep `ros-base`, the smaller diff.

The pull request description carries what was and was not checked for Qt 5:

- Qt 5 on Jazzy: by the Jazzy CI job only, citing its configure line and the step 2 test.
- Qt 5 locally: through a stand-in for Jazzy's `qt_gui_cpp`.
- Not checked: no Jazzy machine and no Docker here.

It also lists what was found and left alone.

**A Jazzy CI failure would most likely be in the Jazzy-only path of steps 1 and 2:** `find_package(qt_gui_cpp)` on
Jazzy, or `$<TARGET_FILE:qt_gui_cpp::qt_gui_cpp>` and its NEEDED list there. Both are read from
upstream and simulated, not run. `find_package(qt_gui_cpp)` itself is exercised daily by camp's green
Jazzy CI (`8d44eb8`, `ros:jazzy-ros-core`).

## Files to Change

| File | Change |
|---|---|
| `marine_colormap_widgets/CMakeLists.txt` | Qt selection from `qt_gui_cpp`, versioned link and export, configure message (step 1); the Qt check test (step 2) |
| `marine_colormap_widgets/package.xml` | `qt_gui_cpp` build_depend, `qt6-base-dev`, description and comment (step 1) |
| `marine_colormap_widgets/test/check_same_qt_major.cmake` | New (step 2) |
| `docs/decisions/0003-qt-major-follows-ros.md`, `0002-colorbar-widget.md` (Status line), `README.md`, `marine_colormap/README.md` | Step 3 |
| `.github/workflows/ci.yml` | Step 4 |

## Prior Art

- The camp Qt 6 survey: the selection order was verified for `rqt_gui_cpp`, and the widget source was
  found Qt 6 ready.
- Upstream rviz and rqt `lyrical` branches. Their "Qt 6 if found" rule is rejected by the owner.
- `qt_gui_cpp`'s own `qt_gui_cpp_USE_QT_MAJOR_VERSION`, which is taken.
- `mru_transform`'s and `unh_marine_navigation`'s `rolling` CI, taken as the CI shape.
- Nothing new is built beyond the 30-line check.

## Serves

The owner's Rolling port (rolker/unh_marine_autonomy#415) and camp's Qt 6 port (rolker/camp#248).
This repository has no goals or design document.

## Design Picture

No change to how the parts fit. The recorded decision changes: ADR-0003 replaces ADR-0002's Qt 5 choice.

## Principles Self-Check

| Principle | Consideration |
|---|---|
| Only what's needed | Smallest CMake change. The test-side QMouseEvent deprecation and `cmake_minimum_required` are left and listed. The second Qt `-dev` key is a stated cost. |
| A change includes its consequences | `package.xml` description, comments, READMEs, ADR and CI header all change in the same PR. |
| Verify before claiming | Qt 6 is built and tested here. Qt 5 is checked by CI and a stand-in only, and the PR says so. |
| Test what breaks | Step 2 fails on the wrong major (mutation-checked here). |

## ADR Compliance

| ADR | Triggered | How addressed |
|---|---|---|
| This repo ADR-0002 | Yes | Superseded on the Qt choice by new ADR-0003, with a Status pointer only |
| Workspace ADR-0013 | Yes | Plan Authored entry; the records ride in the PR |
| Workspace ADR-0017 | No | No design document |

## Consequences

| If we change... | Also update... | Included? |
|---|---|---|
| Exported Qt is Qt 6 on Rolling | camp on Rolling (see below) | No: camp#248 |
| Qt keys | Jazzy installs also pull `qt6-base-dev` (unused there) | Stated in ADR-0003 |

**camp between this merge and its own port.** On Jazzy nothing changes: `qt_gui_cpp` says 5, and the
export is `Qt5Widgets` as today (apt also adds `qt6-base-dev`). On Rolling, camp's targets that link
`marine_colormap_widgets` and camp's own `Qt5::` targets stop at the CMake generate step with the
`INTERFACE_QT_MAJOR_VERSION` error reproduced above. That is a loud failure, not a silent mixed
process. camp on Rolling is already in that state for `rqt_helm_manager`, because `rqt_gui_cpp` is
Qt 6, and camp is not built on Rolling anywhere today: it has no `rolling` branch, and its CI and
`rolling/ui` have no install. So nothing that works now breaks.

**Found and left alone (for the follow-up issue):**

- `cmake_minimum_required(VERSION 3.8)` in both packages
- the Qt 6.4 `QMouseEvent` deprecation at `test/test_colormap_legend_widget.cpp:62` (a both-version
  form is `QMouseEvent(type, pos, pos, button, buttons, Qt::NoModifier)`)
- cppcheck skipped on Rolling
- `CONTRIBUTING.md` points at a missing `.agents/README.md` and says PRs go to `jazzy`

**#415 causes 1 to 16:** none apply.

- Searched both packages: no `ament_target_dependencies`, tf2, `CPL_CVSID`, Python, Nav2, geodesy,
  GDAL, `tf2_ros` or `rclcpp::Waitable`. Only std includes in `marine_colormap`.
- 9: `xmllint` passes on Rolling.
- 10: builds clean with GCC 15.
- 11: the CI without `-r` on `ros-core` is the check.
- 15 is not yet described in #415.

## Open Questions

- The QMouseEvent deprecation in the test: leave it and list it (recommended, as the port left
  `spin_some`), or fix it in one line, valid on both, as its own commit?
- The Jazzy CI image: `ros-core` like the other ports (recommended), or keep `ros-base`?
- The Rolling image tag: `ros:rolling-ros-core` like the models (recommended), or pin
  `-resolute`?

## Estimated Scope

Single PR into `rolling` with 4 code, docs and CI commits, then a cherry-pick PR to `jazzy`.
