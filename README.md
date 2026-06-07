# marine_colormap

Shared, framework-agnostic scalar-field colormap library for marine viewers.

Maps a scalar value through a named palette and a transfer function
(range-normalize → gain → contrast/gamma → alpha) to a color, as the single
source of truth for appearance across:

- **rqt** plugins (Qt) — e.g. `rqt_sonar_waterfall`
- **rviz** displays (Ogre) — e.g. `rviz_sonar_image`
- **CAMP** (`QGraphicsScene`)

The core carries **no Qt / Ogre / GL dependencies**: it provides the palettes,
the transfer function, a CPU `lookup()`, and `bake_lut()` (the 1-D LUT the GPU
path uploads as a texture). Consumers convert the plain color type to
`QColor` / `Ogre::ColourValue` at their boundary.

## API

- `marine_colormap/palette.hpp` — `Palette` + the named-palette registry
  (`palette(name|index)`, `palette_names()`, `palette_index()`). The registry is
  **append-only / name-keyed**: persist a selection by name so adding palettes
  never renumbers a saved choice.
- `marine_colormap/transfer.hpp` — `TransferParams` (min/max, gain,
  contrast/gamma, alpha ramp, below-floor and no-data sentinels) plus the shared
  pure `normalize()` / `apply_response()` (CPU and the GPU shader use the same
  formula).
- `marine_colormap/colormap.hpp` — `lookup(value, palette, params)` (CPU) and
  `bake_lut(palette, params, n)` (the 1-D LUT for the GPU path). By construction
  `lookup(v)` equals the LUT indexed at `normalize(v, min, max)`.
- `marine_colormap/shader.hpp` — `colormap_glsl()`: GL-free GLSL source (the
  GPU/Tier-2 math), see below.

## GPU (GLSL) usage

For the >8-bit / GPU path, `colormap_glsl()` returns version-agnostic GLSL
defining `marine_colormap_normalize()` and `marine_colormap_response()`, which
mirror the CPU `normalize()` / `apply_response()` so the GPU result matches the
CPU path. The renderer plumbing (texture upload, samplers, `main()`) is
per-substrate (Qt-GL, Ogre) and lives in each consumer; only the math + LUT are
shared. Recipe:

1. Upload the scalar field as an **R32F** texture — full-precision input, so the
   colormap is *not* bound to 8-bit data.
2. Upload `bake_lut(palette, marine_colormap::TransferParams{}, N)` as an Nx1
   **RGBA8** 1-D LUT texture. **`TransferParams{}` (identity — gain 1, contrast 1,
   `alpha_ramp` off) is required here**, because step 3 applies gain/contrast in
   the shader. See the "don't double-apply" note below.
3. Prepend your own `#version` (and `precision highp float;` on GLES — dB-range
   data bands at mediump), declare your samplers, then in `main()`:
   ```glsl
   float t = marine_colormap_response(
               marine_colormap_normalize(value, u_min, u_max), u_gain, u_contrast);
   color = texture(u_lut, vec2(t, 0.5));
   ```
4. Handle below-floor / no-data sentinels in your own `main()` (a clamped LUT
   coordinate can't represent them).

> **Don't apply the transfer twice.** There are two equivalent splits — pick one:
> - **Shader transfer (above):** bake the LUT with **identity** params
>   (`TransferParams{}`) and apply gain/contrast via `marine_colormap_response`
>   in `main()`. This makes gain/contrast/range free uniforms (no re-bake) — the
>   recommended split for interactive viewers.
> - **Baked transfer:** bake the LUT with your real gain/contrast/`alpha_ramp`
>   (`bake_lut(pal, params, N)`) and **drop `marine_colormap_response`** from the
>   shader (sample the LUT directly at `normalize(...)`). Re-bake on any
>   gain/contrast change.
>
> Doing both — a non-identity baked LUT *and* the shader response — applies
> gain/contrast twice.

> Numeric parity of the shader response vs the CPU path is validated at the first
> GPU consumer (rqt_sonar_waterfall#48: an offscreen-GL test), not in this
> package (which is GL-free).

## Palettes

`grayscale`, `bronze`, `thermal`, `viridis`, `turbo`. The perceptual ramps
`viridis` (van der Walt & Smith) and `turbo` (Mikhailov, Google) are the exact
**canonical 256-entry tables** as distributed by matplotlib — embedded in
`src/perceptual_palettes.cpp`, not hand-rolled.

## License

Apache-2.0.

Design: see ADR-0001 in the UNH Marine Autonomy Framework
(`unh_marine_autonomy/docs/decisions/0001-shared-scalar-colormap.md`).
