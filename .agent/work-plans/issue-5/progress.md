---
issue: 5
---

# Issue #5 — Tier 2: shared GLSL colormap source + GPU usage contract

## Implementation
**Status**: complete (pending review)
**When**: 2026-06-07
**By**: Claude Code Agent (Claude Opus 4.8 (1M context))

**Branch**: feature/issue-5

First Tier-2 (GPU) phase per ADR-0001 (rolker/unh_marine_autonomy): the
*shared shader source*, not a renderer. The runtime renderer (Qt-GL vs Ogre vs
QGraphicsScene) can't be shared; the colormap **math** and the **LUT** can.

- `shader.hpp` — `const char * colormap_glsl()` accessor + a full doc comment of
  the GPU pipeline (R32F scalar texture, `bake_lut` Nx1 RGBA8 1-D LUT,
  normalize→response→sample, sentinels handled in the consumer's `main()`,
  `precision highp float` on GLES).
- `shader.cpp` — the GLSL string (raw literal) defining
  `marine_colormap_normalize(value, lo, hi)` and
  `marine_colormap_response(t, gain, contrast)`, mirroring `transfer.cpp`'s
  `normalize()` / `apply_response()` **line for line** (degenerate `!(hi > lo)`
  → 0; `clamp(t * gain, 0.0, 1.0)` before the gamma branch gated on
  `contrast > 0.0 && contrast != 1.0`). This is what guarantees CPU/GPU parity.
- Version-agnostic by design: no `#version`, no `precision`, no `sampler`
  declarations — each consumer prepends its own preamble and writes `main()`.
- README "GPU (GLSL) usage" section documents the 4-step recipe.

**Tests**: 4 GL-free GTest cases (`test_shader.cpp`): source non-empty, defines
the two contract function signatures, carries the load-bearing ops
(degenerate-range guard, clip-before-gamma, gamma branch), and is
version-agnostic. `colcon test`: 97 tests, 0 failures.

### Deferred (by design)
- Real GLSL **compile** + **numeric parity** vs the CPU path needs an offscreen
  GL context; deferred to the first GPU consumer (rqt QOpenGLWidget waterfall).
  This package stays GL-free, so the validation lives where the GL context does.
- Remaining Tier-2 work (separate issues, post-freeze): rqt QOpenGLWidget GPU
  waterfall, rviz Ogre material, CAMP GL viewport (camp#63).
