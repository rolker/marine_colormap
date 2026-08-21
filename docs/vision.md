# marine_colormap — vision and roadmap

**Status: vision document, not a decision record.** Nothing here is binding. It exists so
that the small pieces we build next point in a consistent direction, and so that anyone
picking up one sub-issue can see the whole shape without reconstructing it from an issue
thread. Decisions get made in `docs/decisions/` as each area firms up, and where an ADR
disagrees with this document, **the ADR wins** — this file gets updated to match.

Umbrella issue: [#12](https://github.com/rolker/marine_colormap/issues/12). The prior-art
survey behind most of the reasoning here lives in three comments on that issue (Colin
Ware's work; fixed-domain and topo-bathy; format, editor and search path) — this document
does not repeat them, it acts on them.

## Where we are today

The library models exactly one kind of colormap: a **continuous ramp over an
operator-adjustable range**. Six palettes (`grayscale`, `bronze`, `thermal`, `viridis`,
`turbo`, `quality`) in a compiled-in, name-keyed, append-only registry; a fixed-order
transfer function (normalize → gain → contrast/gamma → sample → alpha); an Auto/Manual
`RangeModel`; a CPU `lookup()`; `bake_lut()` for the GPU path; a GLSL helper that mirrors
the CPU math line for line; and one Qt widget, an interactive colorbar legend.

Consumers: camp, marine_sonar_widgets, rqt_operator_tools (`rqt_sonar_waterfall`, with
`rqt_marine_sonar` inheriting transitively), rviz_sonar_image, marine_perception_tools.

Every enhancement below stretches the one-kind-of-colormap assumption in a different
direction.

## The vision

A layered system. Each layer depends only on the ones above it, and the top layer stays
free of Qt, Ogre and GL so that every consumer — including headless tests — can use it.

**Core model (Qt-free).** Palette *kinds*: continuous ramps as today, plus **stepped** and
**categorical/indexed** palettes. **Anchored breakpoints** — see below. Sentinel colours
with a defined vocabulary (`under`, `over`, `bad`, `unmapped`). A range *policy* rather
than a range flag. The perceptually-correct shading composition step. Palette metadata:
provenance, units, natural domain, luminance headroom, and **colour-profile variants**
(day / dusk / night).

**I/O and registry.** A native on-disk palette format, import of the formats the
geoscience and hydrographic world already speaks, a layered search path (environment
override → user → site → ROS packages → compiled-in), first-wins-per-name resolution,
provenance reporting, fail-soft parsing, and `reload()`.

**GPU parity.** Everything the core does, the GLSL helper must be able to do, so that the
baked-LUT consumers are never second-class. See "Invariants".

**Usage widgets (Qt).** The legend, in variants that suit each palette kind, plus the two
usage-side UI tiers below.

**Authoring tool.** A standalone application for constructing palettes that don't exist
yet, with validation and import/export. Deliberately *not* a runtime dependency of
anything that ships to a boat.

**Policy and configuration.** Per-quantity defaults — which palette, range and breakpoints
a given kind of data gets by default — and display-condition selection for day/dusk/night.

### The three UI tiers

The boundary between tiers 2 and 3 is **usage versus authoring**, and it is load-bearing
for packaging: tiers 1 and 2 ship to every consumer and end up on the boat; tier 3 is a
workstation tool and must never appear in camp's or rviz's dependency chain.

1. **Selector** — a small widget for client applications: choose a palette, adjust the
   basics *if the palette allows it*, with an "Advanced" button leading to tier 2. "If
   allowed" is the fixed-domain policy surfacing in the UI, and the prior art is unanimous
   that the controls should be **removed rather than greyed out** when a palette owns its
   domain. An operator who can see a slider will eventually drag it.
2. **Adjustment panel** — the fuller usage-side UI: range, breakpoints, transfer
   parameters, stepped-vs-smooth, colour-profile selection.
3. **Authoring tool** — construct a new palette when none of the existing ones fit;
   import and export other formats; run the validation suite.

**All three are thin skins over a Qt-free policy model.** rviz will not use our Qt widgets
— it has its own property system — and neither would a Foxglove or web view. If the model
doesn't describe what is adjustable and what isn't, those consumers reimplement the policy
and drift, which is the duplication problem this library exists to solve.

### Anchored breakpoints — the unifying hypothesis

A palette is a sequence of segments separated by **N breakpoints, each optionally anchored
to an absolute data value**. Each span between anchored breaks is stretched independently
to fill its share of the colour table.

- **Zero anchored breaks** is today's behaviour: one ramp stretched edge to edge.
- **One** is a GMT-style hinge — a topo-bathy palette pivoted at chart datum.
- **Two or more** gives shoreline *and* safety contour in a single colormap.
- **Fully anchored on both ends** is a fixed-domain palette — the costmap case — where
  there is no free range left for an operator to adjust.

If that last equivalence holds, **Themes 1 and 2 of #12 are one mechanism, not two**, and
we implement anchored breakpoints once. That is a hypothesis to test early, because it
determines how the work splits.

No surveyed system supports more than one hinge: GMT allows exactly one per CPT,
matplotlib's `TwoSlopeNorm` has exactly one centre, and ParaView rescales control points
proportionally, which would slide an anchored break off its data value. **This is our
synthesis of GMT's hinge with S-52's mariner-configurable depth classes, not a borrow.**
We own the edge cases.

Breaks have **different lifetimes**, which is why they need to be bindable to a source
rather than being plain numbers in a palette file:

| Break | Lifetime | Example |
|---|---|---|
| Chart datum | Fixed palette data | 0.0 |
| Visible waterline | Live, moves with tide | current water level |
| Safety contour | Operator setting, tide-adjusted | "keep 2 m under the keel" |

Degenerate cases must be handled deliberately rather than by throwing: a break outside the
active data range is the *ordinary* case for a survey line with no land in view. Clamp and
render the surviving spans, and never let "no land visible" change the colours of the
water. Breaks clamp rather than cross, reusing the legend widget's existing handle policy.

### Tide-aware depth display

The near-term driving requirement: a colormap with shoreline and safety-contour breaks,
used in camp **tide-aware** — shading computed against instantaneous water level rather
than static chart datum.

This is not a new idea here. **Brennan, Ware, Alexander, Armstrong, Mayer, Huff, Calder,
Smith, Plumlee, Arsenault and Glang, "Electronic Chart of the Future: The Hampton Roads
Demonstration Project" (US HYDRO 2003)** demonstrated tide-aware, time-aware depth display
treating chart datum as a live quantity. `s57_tools` already applies a tide-offset
correction for chart display, so tide plumbing likely exists to reuse rather than build.

### The shading seam

Shading belongs in a **companion library**
([rolker/unh_marine_autonomy#326](https://github.com/rolker/unh_marine_autonomy/issues/326)),
not here — it needs a height field, a light model, surface gradients and eventually cast
shadows, none of which should enter a dependency-free colormap core.

But colormapping and shading compete for the same luminance channel, so the *composition*
step belongs here:

- Composing a scalar value and a shade factor into a final colour must happen in a
  **perceptual space — modulating L\***, not multiplying RGB, which darkens *and*
  desaturates. This is the most common way shaded colour maps are gotten wrong.
- It enters the transfer pipeline as a **defined stage at a defined position**, and is
  mirrored in the GLSL helper so CPU and GPU agree.
- Palettes declare whether they have left **luminance headroom** for draping. A palette
  spanning the full luminance range has nothing left for shading to modulate.

Designing that seam now costs little. Retrofitting a pipeline stage into a shipped
transfer function on both CPU and GPU paths costs a great deal.

Ware's 2025 result — stepped colormaps are read more accurately but interfere much more
with perception of surface shape — means **stepped-versus-smooth is a per-view runtime
toggle, not a build-time palette property**. The same colormap object serves "read the
depth here" and "see the morphology" modes, and the right answer differs.

### Colour profiles (day / dusk / night)

S-52 ships DAY_BRIGHT, DAY_WHITEBACK, DAY_BLACKBACK, DUSK and NIGHT. The *structure* of
the depth classes is identical across all five; only the colour values change. So this is
a **variant axis on an otherwise-identical palette definition** — metadata in the format,
not a new mechanism — and it composes with everything else: a multi-hinge topo-bathy
palette can carry a night variant without any break logic changing.

Note the selection is **global display state**, not a per-layer choice, so it lives
somewhere different from the per-layer palette selection. Dark adaptation on a bridge at
night is a real operational need, not a nicety.

### Format, registry, and where palettes live

Leaning toward the **ParaView/VTK preset JSON schema** as the native format, with
namespaced extensions for what it doesn't cover (breakpoints, colour profiles, units,
luminance headroom). It is the only surveyed candidate carrying machine-readable
provenance — `Creator`, `Source`, **`License`** — it already has the categorical fields
(`IndexedColors`, `Annotations`) that fixed-domain palettes need, and it is BSD-3 so its
starter set is vendorable into this Apache-2.0 repo.

Caveat to weigh before committing: the "our users already run it" argument is weak here —
CCOM's desktop tools are QPS Fledermaus and Qimera, Caris and QGIS, not ParaView. A third
option is native JSON with **GMT `.cpt` as a first-class bidirectional format** rather
than import-only, accepting lossiness for multi-break palettes. `.cpt` is what the
hydrographic world speaks and the only common format carrying an absolute z-range with a
hinge.

A model decision precedes any parser: **stop-based or segment-based?** A plain stop list
cannot express a hard discontinuity, and a shoreline break *is* one. GIMP's `.ggr` is
segment-based and can; Krita's "stop handle as a UI fiction over a segment model" is the
bridge if we want a simple UI over a discontinuity-capable model.

**Licensing is a real constraint, verified**: GMT's own palettes (`globe`, `relief`,
`etopo1`, `gebco`, …) are **LGPL-3+** and must not be vendored into this Apache-2.0
package. Clean sources are Crameri's Scientific Colour Maps (MIT — `oleron`, `bukavu`),
Thyng's cmocean (MIT — including a hinged `topo`), viridis (CC0), ColorBrewer
(Apache-style, with an acknowledgement string), and ParaView's presets (BSD-3).

Search path, highest precedence first: `$MARINE_COLORMAP_PATH` → `$XDG_DATA_HOME` → 
`$XDG_DATA_DIRS` → package-contributed palettes via `ament_index` → compiled-in.
**Merge across names, first-wins per name.** There is **no existing ROS 2 convention for
user-overridable resources** — we are establishing local convention, and the ADR should
say so plainly.

### Validation

The authoring tool should report, not veto. Ware, Stone and Szafir's "Rainbow Colormaps
Are Not All Bad" makes a blanket anti-rainbow rule indefensible; non-monotonic lightness,
grayscale collapse and colour-vision ambiguity are objective and should be reported
regardless of hue path.

Checks worth building, from the survey: monotonic L\* with an explicit opt-out for pivoted
palettes; non-monotonicity in at least one opponent channel (Ware 1988 — this is what
makes value-reading robust to simultaneous contrast); the Bujack/Ware assessment measures
(σv, σV, v̄, V̄, v_min, tr_min) reported under ΔE₇₆, ΔE₀₀ and ΔE_CAM02-UCS because the
metrics disagree; Kovesi's `sineramp` test image; colour-vision simulation via
libDaltonLens (public domain; note Brettel 1997 is the only algorithm correct for
tritanopia); and — from Ware's 2025 paper — a **draped preview over a hill-shaded
surface**, since that is the condition our colormaps are actually used in.

Two cautions. A stepped palette has zero local speed within a band, so it **fails local
invertibility by construction** — the validator must special-case discrete maps rather
than report them broken. And deliberate non-uniformity is normal in this domain; a
uniformity metric must not veto a survey-domain colormap that was made non-uniform on
purpose.

## Invariants

Things that must survive every change below:

- **The core stays Qt-free / Ogre-free / GL-free.** rviz and headless tests depend on it.
- **The built-in registry stays append-only and name-keyed**, because consumers persist
  selections by name. User-supplied palettes must not be able to break that guarantee for
  built-ins — shadowing policy is an open decision, not an accident.
- **GPU parity is not optional.** Anything the CPU path can express, the GLSL helper must
  express identically. The likely mechanism for anchored breaks: keep the baked LUT
  uniform in *normalized* space as today, and do the piecewise data-to-normalized mapping
  in the shader with break positions as uniforms — which also means dragging a safety
  contour needs no LUT re-bake.
- **Fail soft.** A malformed palette file warns, names the file and the error, and is
  skipped. A bad palette dropped on a boat must never take down the map app.
- **Authoring never ships to a boat.** No runtime consumer depends on the authoring tool.
- **Existing consumers keep working** through every step.

## Sequencing

Ordered so each step is independently useful and derisks the next.

1. **Anchored breakpoints in the core**, with the topo-bathy palette as the driving
   consumer — the near-term requirement. Qt-free, fully testable, no UI. Establishes
   palette kinds, sentinel vocabulary, and the range *policy*.
2. **Tide-aware shoreline + safety contour in camp**, consuming step 1. The first
   user-visible payoff and the thing actually needed now.
3. **The costmap palette** as the second consumer of the same mechanism — validating the
   fully-anchored degenerate case, and migrating camp's hand-rolled
   `occupancy_grid.cpp` ramp. rviz gives an exact reference, so acceptance is objective.
4. **Tier-1 selector widget** — cheap, immediately visible in every consumer, and it
   forces the policy-drives-UI question early rather than late.
5. **Legend variants** for stepped, categorical and multi-break palettes. *(Under-explored
   — see Open questions.)*
6. **File loading**: format decision + ADR, search path, registry with provenance and
   reload.
7. **Shading composition** in the transfer pipeline, CPU and GLSL, coordinated with
   unh_marine_autonomy#326.
8. **Authoring tool**, last — by then the model, I/O and validation metrics all exist and
   the tool is largely assembly.

Per-quantity defaults and day/dusk/night selection slot in after step 6, once palettes can
come from configuration.

## Open questions

- Does the anchored-breakpoint mechanism really unify fixed-domain and pivot palettes, or
  do they diverge once we build them?
- Stop-based or segment-based palette model?
- Native format: ParaView JSON, or JSON plus first-class `.cpt`?
- Shadowing policy for user palettes over built-ins: forbidden, warned, or namespaced?
- **Legend design for the new palette kinds — the least-explored area of this vision.** A
  stepped or categorical legend is a keyed swatch list, not a continuous bar with two
  handles; a multi-break legend needs non-linear ticks and break markers.
- What does a consumer do when a persisted palette name is no longer installed? Silently
  falling back to grayscale on a boat is a bad failure.
- Where do S-100 portrayal conventions land, and do S-102 (bathymetric surface) or S-104
  (water level) constrain any of this? *(Under investigation.)*
- Does the authoring tool live in this repo or its own?

## References

Full citations are in the prior-art comments on
[#12](https://github.com/rolker/marine_colormap/issues/12). The load-bearing ones:

- Ware, C. (1988). "Color sequences for univariate maps." *IEEE CG&A* 8(5), 41–49.
- Ware, C. (2025). "Colormaps for Shaded Surfaces: Stepped vs Smooth." *IEEE TVCG* 31(7).
- Ware, Turton, Samsel, Bujack, Rogers (2017). "Evaluating the Perceptual Uniformity of
  Color Sequences for Feature Discrimination." *EuroRV³*.
- Bujack, Turton, Samsel, Ware, Rogers, Ahrens (2018). "The Good, the Bad, and the Ugly."
  *IEEE TVCG* 24(1), 923–933.
- Ware, Samsel, Rogers, Navratil, Mohammed (2020). "Designing Pairs of Colormaps for
  Visualizing Bivariate Scalar Fields." *EuroVis Short Papers*.
- Ware, Stone, Szafir (2023). "Rainbow Colormaps Are Not All Bad." *IEEE CG&A* 43(3).
- Ware, Mayer, Johnson, Jakobsson, Ferrini (2020). "A global geographic grid system for
  visualizing bathymetry." *GI* 9(2), 375–384.
- Brennan, Ware, … Arsenault, Glang (2003). "Electronic Chart of the Future: The Hampton
  Roads Demonstration Project." *US HYDRO 2003*.
- Kovesi, P. (2015). "Good Colour Maps: How to Design Them." arXiv:1509.03700.
- Crameri, Shephard, Heron (2020). "The misuse of colour in science communication."
  *Nature Communications* 11:5444.
- IHO S-52 Edition 6.1.1 (2015), Presentation Library Appendix 2.
