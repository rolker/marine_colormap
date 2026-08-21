# marine_colormap (repo)

Shared, framework-agnostic scalar-field colormap library for marine viewers,
plus its Qt widgets. This repo is a **multi-package** colcon container: each
package lives in its own subdirectory (the repo root is not itself a package).

| Package | Subdir | Depends on | What it is |
|---|---|---|---|
| **`marine_colormap`** | [`marine_colormap/`](marine_colormap/) | — (Qt-free) | The core: named palettes, the transfer function (range-normalize → gain → contrast/gamma → alpha), a CPU `lookup()`, `bake_lut()` for the GPU 1-D LUT path, and the auto/manual `RangeModel`. No Qt / Ogre / GL dependencies. |
| **`marine_colormap_widgets`** | [`marine_colormap_widgets/`](marine_colormap_widgets/) | `marine_colormap` + Qt5 Widgets | `ColormapLegendWidget`: an interactive colorbar legend that paints the value→color mapping and lets an operator drag the min/max handles to pin a manual range. |

The core stays deliberately Qt-free (project ADR-0001 Tier 1); the Qt cost is
isolated to `marine_colormap_widgets`. See each package's README for API details.

## Direction

- [`docs/vision.md`](docs/vision.md) — where this library is heading: palette kinds,
  anchored breakpoints (shoreline / safety-contour / fixed-domain), the three UI
  tiers, the shading seam, on-disk palettes, and the build order. A roadmap, not a
  decision record — ADRs below win where they disagree.

## Decisions

- [`docs/decisions/0001-range-model.md`](docs/decisions/0001-range-model.md) —
  the auto/manual `RangeModel` beside `TransferParams`, the `lo <= hi` invariant,
  and the widget-tier package boundary.
- [`docs/decisions/0002-colorbar-widget.md`](docs/decisions/0002-colorbar-widget.md)
  — the `ColormapLegendWidget` API, the multi-package (container-repo) structure,
  the handle-clamp policy, and the `rangeChanged` signal contract.

## Build

```bash
colcon build --packages-up-to marine_colormap_widgets
colcon test  --packages-select marine_colormap marine_colormap_widgets
```

The widget test runs headless under the offscreen Qt platform plugin
(`QT_QPA_PLATFORM=offscreen`), pulled by the `qtbase5-dev` rosdep dependency.

## License

Apache-2.0.
