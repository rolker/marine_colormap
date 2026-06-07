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

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-06-07 14:16 -0400
**By**: Claude Code Agent (Claude Opus 4.8 (1M context))
**Verdict**: changes-requested (minor)

**Branch**: feature/issue-1 at `f15a0be` (PR #2)
**Mode**: pre-push
**Depth**: Standard (new shared library, ~700 lines)
**Static analysis**: clean (cpplint/cppcheck/uncrustify/copyright pass) | **Adversarial**: Claude + Copilot
**Must-fix**: 2 | **Suggestions**: 4

### Findings
- [ ] (must-fix) to_rgba8: non-finite channel -> lround(NaN) is undefined; guard !isfinite -> 0 — `color.hpp:60`
- [ ] (must-fix) Palette ctor does not sort/validate stops; sample() assumes ascending t (public-API footgun) — `palette.cpp:25`
- [ ] (suggestion) Document apply_response exact clamp points as the GPU-port contract + add combined gain!=1 & contrast!=1 test — `transfer.cpp:33`
- [ ] (suggestion) Equivalence test only hits exact grid points; add off-grid values at +/-1 LSB tolerance (with alpha_ramp/gain on) — `test_colormap.cpp:88`
- [ ] (suggestion) Document sentinel precedence (below/nodata bypass alpha ramp) + alpha ramp is on normalized data position — `colormap.cpp:40,46`
- [ ] (suggestion) Document/guard empty palette (opaque black), gain<0 (clamps 0), contrast<=0 (linear) — `palette.cpp:32`,`transfer.cpp:35`

### Dismissed (false positives)
- Alpha CPU<->LUT divergence — both reviewers withdrew after tracing; consistent up to quantization.
- Palette::sample boundary clamp; registry thread-safety (magic statics); CMake clean export; normalize NaN (guarded by lookup isfinite).

## Integrated Review
**Status**: complete
**When**: 2026-06-07 14:30 -04:00
**By**: Claude Code Agent (Claude Opus 4.8 (1M context))

**PR**: #2 at `f15a0be`
**Sources**: 2 (Copilot R1 @ `f15a0be`, Local Review (Pre-Push) @ `f15a0be`)
**Cross-source confirmations**: 1
**CI**: all-pass (build-and-test + copilot-pull-request-reviewer)

### Findings
- [ ] (cross-confirmed: Copilot R1 + Local Review pre-push) Palette ctor stores stops unsorted; sample() assumes ascending t -> wrong colors for unsorted input; sort/validate in ctor + test — `src/palette.cpp:25-28`
- [ ] (valid, Copilot R1) test uses std::out_of_range without `#include <stdexcept>` (transitive-include fragility) — `test/test_palette.cpp:18`
- [ ] (must-fix, Local Review) to_rgba8 non-finite channel -> lround(NaN) undefined; guard !isfinite -> 0 — `color.hpp:60`
- [ ] (suggestion, Local Review) document apply_response clamp points as GPU-port contract + combined gain!=1 & contrast!=1 test — `transfer.cpp:33`
- [ ] (suggestion, Local Review) strengthen CPU==LUT equivalence test with off-grid values + tolerance — `test_colormap.cpp:88`
- [ ] (suggestion, Local Review) document sentinel precedence + alpha-ramp-on-normalized-position — `colormap.cpp:40,46`

### False positives
- none — both Copilot comments valid; both reviewers' earlier alpha CPU/LUT-divergence concern was already withdrawn in the pre-push review (consistent up to quantization).
