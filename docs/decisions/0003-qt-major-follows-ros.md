# ADR-0003: The widget's Qt major follows ROS's `qt_gui_cpp`

## Status

Accepted. Supersedes ADR-0002 on the choice of Qt (ADR-0002 chose Qt 5
because Jazzy ships Qt 5); the rest of ADR-0002 stands.

## Context

`marine_colormap_widgets` was written for Qt 5 (ADR-0002). On ROS 2 Rolling
(Ubuntu 26.04) ROS's own Qt packages (`qt_gui_cpp`, `rqt_gui_cpp`,
`python_qt_binding`) are Qt 6, while Qt 5 is still installable. The package
still built there, against Qt 5, so a consumer that also uses ROS's Qt
packages would mix Qt majors in one process. The defect was the Qt major,
not a compile break: the widget source was already Qt 6 ready.

What `qt_gui_cpp` tells a package that finds it (`qt_gui_cpp-extras.cmake`
in `ros-visualization/qt_gui_core`):

| Branch | Extras | Qt target after `find_package(qt_gui_cpp)` |
|---|---|---|
| `humble`, `jazzy`, `kilted` | `set(qt_gui_cpp_USE_QT_MAJOR_VERSION 5)` only | none |
| `rolling` (3.0.x) | `find_package(Qt6 REQUIRED COMPONENTS Widgets Core)`; no variable | `Qt6::Widgets` |
| `lyrical` | sets the variable from `python_qt_binding`, then finds that Qt | the matching one |

The variable is upstream's declared interface for downstream packages: it
was added in 2016 ("export information about using Qt5 so that downstream
packages can use the same version"). Rolling's `qt_gui_cpp` has already
removed it ("Removed Qt5", #345), so on Rolling the `Qt6::Widgets` target is
the only signal.

## Decision

**Follow ROS's Qt packages.** The widget uses the Qt major `qt_gui_cpp` uses
on the distro it is built for:

1. `qt_gui_cpp_USE_QT_MAJOR_VERSION`, if `qt_gui_cpp` sets it;
2. else Qt 6, if `qt_gui_cpp` has defined `Qt6::Widgets` (Rolling);
3. else configure stops with a fatal error that says what was expected.

Configure prints the Qt version and major it picked. The link and the
export use the versioned names (`Qt<N>::Widgets`, `Qt<N>Widgets`), never the
versionless `Qt::Widgets`, which aliases the first Qt major a consumer
happened to find and would let a disagreeing consumer resolve it silently;
with the versioned name CMake stops at generate time instead
(`INTERFACE_QT_MAJOR_VERSION ... does not agree`).

`test_same_qt_major` checks the result against ROS's own binary: every
`libQt<N>*` entry the built library needs must have the major of
`qt_gui_cpp`'s `libQt<N>Core` entry.

Rejected:

- **Qt 6 if installed, else Qt 5** (the rule of upstream rviz and rqt
  `lyrical`): it follows the machine, not ROS. Rolling here has both Qt 5
  and Qt 6 installed, and Jazzy CI installs both (below).
- **By distro name**, in CMake (`$ENV{ROS_DISTRO}`) or in `package.xml`
  (`condition="$ROS_DISTRO == ..."`): it restates by name a fact
  `qt_gui_cpp` already states, and goes stale at each new distro.
- **`if(TARGET Qt6::Widgets)` alone, else Qt 5**: correct on both distros
  today, but it picks Qt 5 because a target is absent, and would pick Qt 5
  on a `qt_gui_cpp` that states 6 without finding Qt.

## Consequences

- **Both Qt development keys, no condition.** `package.xml` lists
  `qtbase5-dev` and `qt6-base-dev`. Jazzy's `qt_gui_cpp` does not bring the Qt
  5 development package (its `qtbase5-dev` is a build_depend only), Rolling's
  brings `qt6-base-dev`, and nothing on both distros installs only the Qt in
  use without a distro condition. So `rosdep install` puts both on each
  distro, and the unused one costs disk space. The build uses only the one
  `qt_gui_cpp` names, so each CI job also tests that the other installed Qt is
  not picked.
- **`qtbase5-dev` must go when Rolling moves to an Ubuntu without Qt 5.** The
  unconditional key will then stop resolving there; that is when it is
  dropped (with Jazzy kept on its own branch if it still needs it).
- **A further upstream change fails loudly.** If `qt_gui_cpp` stops setting
  the variable and stops finding Qt 6 (or finds Qt 7), configure stops with
  the fatal error rather than guessing; the rule is then revisited.
- **Consumers must follow the same rule.** The exported Qt is Qt 6 on
  Rolling. A consumer that finds Qt 5 and links this library stops at CMake's
  generate step. camp (rolker/camp#248) must take the same rule before it
  builds on Rolling; on Jazzy nothing changes for it.
- `qt_gui_cpp` is a build_depend only: CMake reads it to learn the major;
  consumers follow the exported Qt, not `qt_gui_cpp`.

## References

- Issue: https://github.com/rolker/marine_colormap/issues/26
- Rolling port: rolker/unh_marine_autonomy#415
- ADR-0002 (this package — colorbar widget): `docs/decisions/0002-colorbar-widget.md`
- `ros-visualization/qt_gui_core`, `qt_gui_cpp/cmake/qt_gui_cpp-extras.cmake`
  on each distro branch.
- Workspace ADR-0012 (cross-reference addendums) in `project11/docs/decisions/`.
