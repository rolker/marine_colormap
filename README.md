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

## Palettes

`grayscale`, `bronze`, `thermal`, `viridis`, `turbo`. The perceptual ramps
`viridis` (van der Walt & Smith) and `turbo` (Mikhailov, Google) are the exact
**canonical 256-entry tables** as distributed by matplotlib — embedded in
`src/perceptual_palettes.cpp`, not hand-rolled.

## License

Apache-2.0.

Design: see ADR-0001 in the UNH Marine Autonomy Framework
(`unh_marine_autonomy/docs/decisions/0001-shared-scalar-colormap.md`).
