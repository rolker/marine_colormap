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

Design: see ADR in the UNH Marine Autonomy Framework
(`unh_marine_autonomy/docs/decisions/`).
