---
issue: 1
---

# Issue #1 — marine_colormap: framework-agnostic scalar colormap core library

## Implementation
**Status**: complete (pending review)
**When**: 2026-06-07
**By**: Claude Code Agent (Claude Opus 4.8 (1M context))

**Branch**: feature/issue-1

Built the Tier-1 core per ADR-0001 (rolker/unh_marine_autonomy):

- `color.hpp` — `Rgba` (float, sRGB display encoding, straight alpha) + `Rgba8`
  + `to_rgba8` + `lerp`. Documents the color-representation contract.
- `palette.{hpp,cpp}` — `Palette` (explicit-position stops, piecewise-linear
  `sample`) + append-only / name-keyed registry. Built-ins: `grayscale`,
  `bronze`, `thermal` (de-duplicated canonical thermal). `viridis`/`turbo`
  deferred — see Decisions.
- `transfer.{hpp,cpp}` — `TransferParams` (min/max, gain, contrast/gamma, alpha
  ramp, below-floor + no-data/NaN sentinels) and the **shared pure**
  `normalize()` + `apply_response()` (CPU and the future GPU shader call the
  identical formula).
- `colormap.{hpp,cpp}` — CPU `lookup()` (float until output) and `bake_lut()`
  (N-entry 1-D LUT over normalized [0,1]; gain/contrast/alpha baked, min/max and
  sentinels applied around it — matches the ADR GPU split).
- ament_cmake package with clean export (no Qt/Ogre in public interface), CI
  workflow, CONTRIBUTING.

**Tests**: 19 GTest cases across palette / transfer / colormap, incl. golden
palette stops, transfer math, and a **CPU-lookup == baked-LUT** equivalence
check (the GPU-path guarantee). `colcon test`: 72 tests, 0 failures.

### Decisions (for review)
- [ ] **Canonical thermal** = the de-duplicated 12-stop ramp (the rviz_sonar_image
  ramp had an accidental duplicated stop). rviz will shift slightly on migration;
  golden test locks the reference. Confirm this is the intended canonical.
- [ ] **License = Apache-2.0** (matches the sibling marine_perception_tools and
  the seeded LICENSE), not the BSD-3 of the rviz/rqt sonar sources. Confirm.
- [ ] **viridis / turbo deferred** — not hand-rolled from memory (per the ADR
  review). To be added from canonical published tables (matplotlib viridis /
  Google turbo) in a follow-up; the registry is append-only so they slot in
  without renumbering. Sonar consumers only need grayscale/bronze/thermal.
