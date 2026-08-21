# marine_colormap — vision and roadmap

**Status: vision document, not a decision record.** Nothing here is binding. It exists so
that the small pieces we build next point in a consistent direction, and so that anyone
picking up one sub-issue can see the whole shape without reconstructing it from an issue
thread. Decisions get made in `docs/decisions/` as each area firms up, and where an ADR
disagrees with this document, **the ADR wins** — this file gets updated to match.

Umbrella issue: [#12](https://github.com/rolker/marine_colormap/issues/12). The prior-art
survey behind most of the reasoning here lives in four comments on that issue (Colin
Ware's work; fixed-domain and topo-bathy; format, editor and search path; and an addendum
on where S-100 is heading) — this document does not repeat them, it acts on them.

## Where we are today

The library models exactly one kind of colormap: a **continuous ramp over an
operator-adjustable range**. Six palettes (`grayscale`, `bronze`, `thermal`, `viridis`,
`turbo`, `quality`) in a compiled-in, name-keyed, append-only registry; a fixed-order
transfer function (normalize → gain → contrast/gamma → sample → alpha); an Auto/Manual
`RangeModel`; a CPU `lookup()`; `bake_lut()` for the GPU path; a GLSL helper mirroring the
CPU `normalize()` and `apply_response()` (palette sampling goes through the baked LUT
texture, and the below-floor and no-data sentinels are left to the consumer's own shader);
and one Qt widget, an interactive colorbar legend.

Consumers: camp, marine_sonar_widgets, rqt_operator_tools (`rqt_sonar_waterfall`, with
`rqt_marine_sonar` inheriting transitively), rviz_sonar_image, marine_perception_tools.

Every enhancement below stretches the one-kind-of-colormap assumption in a different
direction.

## The vision

A layered system, listed roughly in dependency order below except for the last entry. The
core stays free of Qt, Ogre and GL so that every consumer — including headless tests — can
use it. (Policy and configuration is listed last because it arrives last, but it sits
against the core model, not above the widgets — the UI tiers are thin skins over it.)

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
   allowed" is the fixed-domain policy surfacing in the UI, and every surveyed system that
   has this case — ParaView, which hides its Mapping Data group, and Foxglove, which drops
   the range controls for occupancy grids — **removes the controls rather than greying them
   out** when a palette owns its domain. An operator who can see a slider will eventually
   drag it.
2. **Adjustment panel** — the fuller usage-side UI: range, breakpoints, transfer
   parameters, stepped-vs-smooth, colour-profile selection.
3. **Authoring tool** — construct a new palette when none of the existing ones fit;
   import and export other formats; run the validation suite.

**All three are thin skins over a Qt-free policy model.** rviz will not use our Qt widgets
— it has its own property system — and neither would a Foxglove or web view. If the model
doesn't describe what is adjustable and what isn't, those consumers reimplement the policy
and drift, which is the duplication problem this library exists to solve.

### Legends, and the multi-layer problem

camp has no legend in the map view or the layer tree — its only legend today lives inside
the modal "Colormap range…" dialog, which already hosts `ColormapLegendWidget`. The guts of
a persistent legend belong here rather than in camp. But camp's situation constrains the
design in a way a single-view application does not:
**camp shows several layers at once, each with its own colormap and range, so one legend
parked in a corner is ambiguous — it cannot say which layer it describes.**

The shape that follows, borrowing from ParaView and QGIS:

- **A compact ramp strip painted per layer in the layer tree.** QGIS does this, and it is
  the cheapest possible answer to "which layer is which colour" — always visible, always
  unambiguous, no extra chrome, and it cannot drift out of sync with the layer list.
- **A per-layer "show legend" toggle** putting a full labelled legend on screen, with
  **several visible simultaneously**, each captioned with its layer name and units. This is
  ParaView's model: a scalar bar per representation, individually toggleable and
  positionable, labelled with the array it describes. Default to none pinned (or the
  selected layer only) so a busy chart does not start cluttered.
- The existing per-layer "Colormap range…" dialog stays as the *editing* surface; these are
  the *display* surfaces.

**The architectural consequence for this library is the important part: separate the
painting from the widget.** An item delegate paints; it should not own a `QWidget` per row.
(`setIndexWidget` and persistent editors technically can, but the cost and lifetime
management make that the wrong tool for a strip in every layer row.) So the ramp painter
must be usable standalone — draw this palette, with this range, into this `QPainter` and
rectangle — with the interactive `ColormapLegendWidget` as one consumer of that painter
rather than the only way to draw a legend. Both are driven by the same model. If we build
only a widget, camp has no reasonable way to put a ramp in its layer tree.

Two further consequences. **Units and quantity labels stop being optional** once more than
one legend is on screen — the operator has to be able to tell the metres from the decibels
at a glance, which reinforces carrying units in palette metadata and labels per lookup
entry. And **the legend needs a compact degenerate form**: a stepped or categorical legend
is a keyed swatch list, which is tall and will not fit a tree row, so the compact rendering
shows the ramp alone and the full keyed form appears only in the expanded legend.

### Anchored breakpoints — the unifying hypothesis

A palette is a sequence of segments separated by **N breakpoints, each optionally anchored
to an absolute data value**. Each span between anchored breaks is stretched independently
to fill its share of the colour table.

- **Zero anchored breaks** is today's behaviour: one ramp stretched edge to edge.
- **One** is a GMT-style hinge — a topo-bathy palette pivoted at the shoreline.
- **Two or more** gives shoreline *and* safety contour in a single colormap.
- **Fully anchored on both ends** is a fixed-domain palette — the costmap case — where
  there is no free range left for an operator to adjust.

If that last equivalence holds, **Themes 1 and 2 of #12 — fixed-domain palettes and the
topo-bathy pivot — are one mechanism, not two**, and we implement anchored breakpoints
once. That is a hypothesis to test early, because it determines how the work splits.

The **multi-break shape is borrowed, not invented**: S-100's coverage lookup is an ordered
list of absolute-valued intervals, and the shipped S-102 catalogue uses three
operator-settable breaks plus a break at zero. What is *not* available off the shelf is
multi-break **pivot** behaviour — independently stretching each span to fill its share of
the colour range. There, the surveyed systems stop at one: GMT allows exactly one hinge
per CPT, matplotlib's `TwoSlopeNorm` has exactly one centre, and ParaView rescales control
points proportionally, which would slide an anchored break off its data value. So the
design is S-100's interval structure with GMT-style pivot semantics inside each span, and
the edge cases of combining them are ours.

**S-100 already specifies this shape, and we should adopt it** (see "Alignment with S-100"
below). Its coverage portrayal model is an *ordered list of lookup entries, first match
wins*, each carrying a label, a numeric range with an **explicit interval closure**, and
either a flat colour or a start/end colour pair forming a ramp. That expresses discrete,
continuous and hard-break palettes in one structure. Explicit closures
(`geLtInterval`, `ltSemiInterval`, `geSemiInterval`, `closedInterval`) are worth copying
on their own merits: they make a break at the safety contour unambiguous and remove a
whole class of off-by-epsilon bugs at the boundary.

**Breaks are constants in whichever frame the field is expressed in** — see "Frames, not
live breakpoints" below. Expressed relative to the sea surface (`map_tide`), the shoreline break
sits at 0.0 and a safety break sits at the operator's chosen clearance, and neither has to
track the tide. That is a simplification over an earlier sketch in which breaks were bound
to live sources; the binding belongs upstream, on the data, not on the palette.

Degenerate cases must be handled deliberately rather than by throwing: a break outside the
active data range is the *ordinary* case for a survey line with no land in view. Clamp and
render the surviving spans, and never let "no land visible" change the colours of the
water. Breaks clamp rather than cross, reusing the legend widget's existing handle policy.

### Tide-aware depth display

The near-term driving requirement: a colormap with shoreline and safety-contour breaks,
used in camp **tide-aware** — shading computed against instantaneous water level rather
than a static vertical reference.

**This was built here, and the implementation is available to read.** Arsenault, Plumlee,
Smith, Ware, Brennan and Mayer, *"Fusing Information in a 3D Chart-of-the-Future Display"*
(US Hydro 2003, [scholars.unh.edu/ccom/267](https://scholars.unh.edu/ccom/267)) describes
GeoNav3D doing exactly this — *"By summing the instantaneous tide model and the digital
terrain model, a display can be created that represents the actual depth of the water over
a large area for a particular time."* The source is in GeoZui4D (`objects/DynamicTides.*`
and `objects/gutm.cpp`).

Three details from that implementation that change how we should design ours:

- **The tide is a surface, not a scalar.** `DynamicTides::getTideOffset(x, y, t)` resolves a
  per-location, per-time offset from a grid of time offsets and amplitude multipliers plus
  a station time series (the paper: 200 m cells, 6-minute steps, inverse-distance-weighted
  from CO-OPS zones). In an estuary the tide genuinely varies across the display, so a
  single global offset would be wrong.
- **Two tide *times* can coexist in one view.** Along a planned route, GeoNav3D colours the
  navigation corridor at each point's *estimated time of arrival* while the surrounding
  bathymetry uses the current time — the paper points out the resulting colour
  discontinuity at the corridor edge as a feature, not an artefact. So the adjustment is
  per-region as well as per-cell.
- **The shift is applied only below the sea surface.** From `gutm.cpp`:

  ```cpp
  landHeight = pos[Z] + currentH;
  if (landHeight < waterHeight)
      currentTexture->SetHeight(landHeight - waterHeight);  // depth below the water surface
  else
      currentTexture->SetHeight(landHeight);                // elevation stays datum-referenced
  ```

  Submerged terrain is re-expressed relative to the instantaneous water surface; emergent
  terrain keeps its true elevation. That is obviously right once stated — a hill's height
  does not change with the tide — and it is exactly what a topo-bathy palette needs, since
  zero in the shifted field *is* the shoreline.

Also worth knowing: GeoNav3D already had the arbitrary-breakpoint idea. The paper notes it
*"allows hundreds of color zones to be specified which do not have to be equally spaced"*,
with danger zones set from a given vessel's draft. The bands encode **under-keel
clearance** — blue safe, yellow warning, red danger — not raw depth.

`s57_tools` already applies a tide-offset correction for chart display, so tide plumbing
likely exists to reuse rather than build.

**It is also now normative IHO practice, which gives us a model to follow.** S-98 Edition
2.0.0 (October 2025), Appendix D, is mandatory for S-100 ECDIS to implement. Within it,
**Water Level Adjustment** is an operator-selectable function — off by default, with
permanent on-screen indication while active — and when it is on: *"The functionality and
portrayal of the safety contour, depth zone shades, safety depth and indication of isolated
dangers must use the adjusted depth."*

The important architectural lesson is **how** they do it: WLA adjusts the *depth values*
by the water level and then applies the **unchanged** portrayal. Tide-awareness is a
**domain shift, not a palette change.** We should do the same — keep the colormap static
and shift the data or the breakpoints — because it is simpler, it keeps the palette
stable across a tide cycle, and it aligns with the standard.

Their conservatism is worth borrowing wholesale for a robot boat: **shoalest wins** when
several cells or two adjacent time steps could apply, **nearest-neighbour rather than
interpolation** between grid values, and **refuse to adjust outside the water-level
temporal extent** rather than extrapolating. Their defaults are worth copying too: the
enhanced safety contour is on by default, while water-level adjustment is opt-in.

### Frames, not live breakpoints

Roland's framing, and it is the cleanest way to state the whole tide question: this is a
**reference-frame** problem. The workspace already has a settled position on marine
vertical frames, and this document defers to it —
[mru_transform#8](https://github.com/rolker/mru_transform/issues/8) (re-scoped 2026-08-20
to a REP-style marine frame-conventions doc) and **ADR-0010** in `unh_marine_autonomy`.

**Use that convention's terms, and note what it deliberately excludes:**

- **`map`** — ENU on the WGS84 **ellipsoid**. The entire runtime vertical world is
  GNSS-ellipsoidal (ADR-0010 D5).
- **`map_tide`** — the current sea surface in ellipsoidal height, **self-measured** by
  `sea_surface_estimator`. The only runtime vertical datum reference.
- **`waterline`** — a **static URDF frame on the vessel**, where the hull meets the water.
  Not a property of the terrain.

**There is no `chart_datum` runtime frame, and that is a decision, not an omission.**
ADR-0010 D5 removes it: there are no tide tables, gauge feeds or datum grids in the
navigation loop, and a single-offset datum frame was spatially wrong anyway, since the
MLLW-to-ellipsoid separation is itself a varying surface. Datum conversion happens **at
import**, through the ROS-free `marine_vertical_datum` library, not at runtime. Roland's
re-scope of mru#8 states it plainly: *"Our experience does not support having datum frames
in the TF tree... The tide frame is still valuable."*

So for a live display the frame that matters is **`map_tide`**, and the useful scalar field
is terrain height relative to it — depth below the current sea surface. Under-keel
clearance is that minus the vessel's draft, which is a vehicle property reached through the
`waterline` frame rather than a frame of its own.

**Terminology hazard worth naming**: `waterline` in the frame convention is the *vessel's*
waterline. Elsewhere in this document "waterline" means the land/sea boundary in the terrain
— the zero of a tide-shifted field. They are different things. This document should say
**"shoreline break"** for the terrain feature and reserve `waterline` for the vessel frame.

The consequence for this library is a **narrowing of scope, and a welcome one**:

- **`marine_colormap` should never know about tides, datums or drafts.** It colours a scalar
  field. Which frame that field is expressed in is the consumer's business.
- **Breakpoints are constants in the chosen frame.** Express the field relative to
  `map_tide` and the shoreline break is 0.0 and the safety break is the operator's chosen
  clearance — permanently. No live binding, no per-tick palette mutation, no LUT
  invalidation.
- **The transform is where the tide-awareness lives**, applied once, upstream, so the
  colours, the numeric readouts, the contour extraction and any alarms all agree. That is
  the same conclusion S-98 Appendix D reaches by a different route.

**Two tiers of sea surface, and they are complementary rather than competing.** GeoNav3D
used a *modelled* tide surface varying across the display; ADR-0010 D5 uses a *single
measured* `map_tide` at the vehicle. Both are right, for different jobs.

Roland's framing (2026-08-21): **think of it as a local costmap versus a global costmap.**

- **Measured `map_tide` is the local tier.** Robot-centric, small extent, high accuracy,
  derived from the RTK GNSS we carry for ocean mapping. It is what the immediate navigation
  loop should trust, and D5's exclusion of tide tables and gauge feeds is scoped to *that
  loop*.
- **A modelled tide surface is the global tier.** Wide extent, lower precision, able to
  express **spatial** variation across an estuary and **temporal** variation along a planned
  path. Entirely appropriate in camp and other operator tools — and, Roland notes, in parts
  of the robot doing **long-term planning**, which have the same wide-area, ahead-of-time
  character as an operator display.

So the two coexist without contradiction: D5 keeps models out of the navigation loop, not
out of the system. The GeoNav3D corridor feature — colouring a planned route at each point's
estimated time of arrival while its surroundings show the present — is squarely a global-tier
capability, and it is the one that most obviously wants a model.

For this library, none of that changes anything: it colours a scalar field, and which tier
produced the frame the field sits in is the consumer's business. It does raise one idea
worth remembering when the composition stage arrives — a depth adjusted by a *modelled* tide
far from any measurement is lower-confidence than one adjusted by a measured surface
alongside the boat, and that is exactly the kind of thing the state-modulation channel
(saturation encoding confidence) exists to express. Speculative, but it would fall out of
mechanisms we already intend to build.

### Alignment with S-100

We already consume S-102, so it is worth knowing where the standard is heading. Findings
below are from primary IHO documents; details and citations are in the S-100 comment on
[#12](https://github.com/rolker/marine_colormap/issues/12).

**S-100 has a first-class continuous-coverage portrayal model** (Part 9, clause 9-12.7,
"The Coverage package"), and it is close to what we want: an ordered, first-match list of
`LookupEntry` records, each with a `label` for the legend, a numeric range with an explicit
closure, and a `CoverageColor` that is either one flat colour or a `startColor`/`endColor`
pair forming a ramp. The S-52 colour-token model survives and is generalised: tokens
resolved through named palettes carrying **both CIE xyY — with Y as absolute luminance in
cd/m² — and sRGB**. Rules are Lua (Part 9a) or XSLT (Part 9); the shipped S-101 and S-102
catalogues have both moved to Lua.

Four consequences for us:

- **S-102 does not use a continuous ramp.** Despite being gridded continuous bathymetry,
  its portrayal catalogue is eighteen lines of Lua reproducing S-52 banded depth shading
  with hard breaks at operator-set `ShallowContour`, `SafetyContour` and `DeepContour`,
  plus an intertidal band below zero. If our topo-bathy palette exposes those same
  parameter names and the same constraints
  (`ShallowContour ≤ SafetyContour ≤ DeepContour`, a two-shade/four-shade switch), an
  S-102-literate hydrographer reads our configuration with no explanation, and importing a
  real S-102 catalogue later becomes mechanical.
- **Above chart datum we are on our own.** S-102 gives exactly one band below zero and no
  elevation ramp. There is no IHO topographic colour scheme to conform to, so the land
  half of a topo-bathy palette is a free design choice.
- **S-102 uncertainty portrayal is unspecified** — the rule emits a null instruction, and
  uncertainty is consumed numerically to widen danger check areas rather than rendered.
  That is open territory, and it is where [camp#145](https://github.com/rolker/camp/issues/145)
  (defaulting an uncertainty band to the `quality` ramp) is heading anyway.
- **Interpolating ramps in CIE xyY, component-wise including alpha, is the normative S-100
  rule.** We interpolate in sRGB today. Note that xyY is *not* a perceptually uniform
  space, so this is a **conformance** argument rather than a perceptual upgrade — and it
  sits awkwardly beside this document's own insistence on L\*-modulated shading and ΔE
  metrics. Worth deciding deliberately rather than by default.

**Licensing: load catalogues at runtime; do not vendor IHO colour tables.** IHO
publications are copyright IHO with commercial exploitation requiring written permission,
and the IHO GitHub repositories almost all carry no licence file at all, which means all
rights reserved. Runtime loading is also architecturally cleaner and mirrors how S-100
itself separates catalogues from software — an S-102 portrayal catalogue is a handful of
Lua rules plus one colour-profile XML, and is trivially loadable. Note also that the S-52
Annex A:100 presentation library for S-100 ECDIS is **not** publicly available; it ships
only under the S-100 Security Scheme.

Timeline, for context: S-100 ECDIS became voluntary on 1 January 2026 and is mandated for
new installations from 1 January 2029. The data models are stable and shipping; the
portrayal side is visibly thinner, and S-98's tide-aware machinery sits *outside* Part 9 by
the IHO's own admission. We are not going against the grain — we are working in a gap the
IHO has itself flagged as unresolved.

### The shading seam

Shading belongs in a **companion library**
([rolker/unh_marine_autonomy#326](https://github.com/rolker/unh_marine_autonomy/issues/326)),
not here — it needs a height field, a light model, surface gradients and eventually cast
shadows, none of which should enter a dependency-free colormap core.

But colormapping and shading compete for the same luminance channel, so the *composition*
step belongs here:

- Composing a scalar value and a shade factor into a final colour must happen in a
  **perceptual space — modulating L\***, not multiplying RGB, which darkens *and*
  desaturates. This is the most common way shaded colour maps are gotten wrong. The
  principle is Kovesi's: *"to achieve the perception of a coloured surface being shaded the
  luminance of the colours need to be modulated by the relief shading"*
  ([arXiv:1509.03700](https://arxiv.org/abs/1509.03700)) — frequently misattributed to
  Ware, so cite it correctly.
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

### State modulation: dimming and highlighting

A colormapped item should be able to render **less prominent** or **more prominent** than
its neighbours. "Dimmed" is the shorthand, but the requirement is about *prominence*, not
about luminance specifically — which matters, because the right channel turns out not to be
luminance. Two motivating cases, both real:

- **Selection** — indicating which surface or layer is currently selected.
- **LOD staleness** — an LOD-capable display showing coarse tiles while fine data loads can
  dim the temporary tiles, hinting to the operator that what they're looking at is not the
  final answer yet.

This is **the same mechanism as the shading seam above**: take the colour a palette
produced and modulate it by a scalar factor. Shading's factor comes from a light model;
this one comes from item state. That argues for generalising the composition stage from
"shading" to **state modulation**, with shading as one contributor — and it means the two
must compose, since a coarse tile can also be hill-shaded.

**The obvious implementation — reducing luminance — is the wrong one**, and it fails twice
over:

- **Ware's own data says use saturation, not lightness.** The bivariate study (Ware, Samsel,
  Rogers, Navratil, Mohammed, EuroVis 2020) tested exactly foreground-versus-background
  separation and found saturation separation gave the lowest error (0.072, against 0.087 for
  light/dark and 0.090 for hue), with high-saturation-over-low-saturation scoring 4.65 for
  pattern clarity against 1.37 for light-on-dark. **Light/dark was the worst of the three
  strategies tested.** So *recede* should mean desaturate, and *stand out* should mean raise
  saturation.
- **Luminance dimming silently no-ops at night.** In the S-102 night palette deep water is
  already at L = 0 — literal black. You cannot darken black. A modulation defined only in
  luminance would work by day, do nothing at night, and give an operator no staleness hint
  in exactly the conditions where they are most reliant on the display.

Highlighting has a further problem: brightening runs straight into the **luminance headroom**
constraint already noted for draping. A palette using the full luminance range has nothing
left to brighten with. Saturation avoids that, and an outline or border remains available as
a non-colour channel.

Note this converges with something the vision already contemplates. Ware's own bathymetry
practice in GGGS uses **high saturation for measured multibeam and low saturation for
predicted or interpolated background**, with unmapped areas in dark grey. "Dim the coarse
tiles" is the same idea — **saturation encoding data quality or confidence** — arriving from
a different direction. Worth treating them as one concept rather than two features.

Design questions for later: whether state is a single combined factor supplied by the
consumer or a set of named contributors the library composes; whether a dimmed layer's
legend swatch dims with it (probably yes, for consistency); and whether dimming alone is a
strong enough channel to carry "this data is provisional" on a safety-relevant display, or
whether it should be a hint accompanying something more explicit.

### Colour profiles (day / dusk / night)

S-52 ships five palettes, but **every S-100 portrayal catalogue actually shipped uses
three** — `Day`, `Dusk`, `Night` (verified in the S-101, S-102 and S-111 catalogues). The
S-52 day triad has not been carried forward. Three is the right target.

The *structure* is identical across variants; only the colour values change. So this is a
**variant axis on an otherwise-identical palette definition** — metadata in the format,
not a new mechanism — and it composes with everything else: a multi-break topo-bathy
palette can carry a night variant without any break logic changing. Selection is **global
display state**, not a per-layer choice, so it lives somewhere different from the
per-layer palette selection.

**Copy the IHO's discipline for building the variants**: chromaticity (x, y) is held
**constant per token across all three palettes, and only luminance varies.** In the
shipped S-102 profile the shallow→deep ladder is a monotonic luminance ramp by day
(35→45→55→65→80 cd/m²) that **inverts at night**, where deep water goes to literal black
and only the shallow, dangerous end retains any luminance. A naive "darken everything"
transform does not reproduce that, and the inversion is the entire point of a night
palette. Dark adaptation on a bridge at night is a real operational need, not a nicety.

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

**A tension to resolve at step 6, not to discover then**: the model we are adopting is
interval-and-closure based precisely because a plain stop list cannot express a hard
discontinuity — but ParaView's `RGBPoints`, the leading native-format candidate, *is* a
stop list. Coincident stops and a namespaced extension can carry the breaks, but how
cleanly a candidate format represents a discontinuity should be an explicit criterion when
the format is chosen, not an afterthought.

The stop-based-versus-segment-based question that would otherwise precede any parser is
**settled by adopting S-100's lookup-entry shape**: a list of intervals with explicit
closures *is* a segment model, and it expresses a hard discontinuity natively — which a
plain stop list cannot, and a shoreline break *is* one. Krita's "stop handle as a UI
fiction over a segment model" remains the pattern if we later want a simple stop-style
editing UI over it.

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
- **GPU parity is a goal we are adopting, not a property we already have.** Today the GLSL
  helper covers normalize and response only, and sentinel handling is the consumer's job —
  so this is a gap to close as the model grows, and new capability should not widen it. The
  likely mechanism for anchored breaks: keep the baked LUT
  uniform in *normalized* space as today, and do the piecewise data-to-normalized mapping
  in the shader with break positions as uniforms — which also means dragging a safety
  contour needs no LUT re-bake.
- **Fail soft.** A malformed palette file warns, names the file and the error, and is
  skipped. A bad palette dropped on a boat must never take down the map app.
- **Authoring never ships to a boat.** No runtime consumer depends on the authoring tool.
- **Existing consumers keep working** through every step.

## Readiness

Settled enough to build on: the layered architecture; the lookup-entry model shape and its
explicit interval closures (now backed by a standard rather than invented here); breakpoint
semantics and the differing lifetimes of palette-fixed, live and operator-set breaks;
tide-awareness as a domain shift; colour profiles as metadata with three variants built by
varying luminance at fixed chromaticity; the usage-versus-authoring packaging boundary; the
painter-versus-widget split; and the invariants.

Still genuinely open: the on-disk format choice, shadowing policy for user palettes, xyY
versus sRGB interpolation, legend placement specifics in camp, and where the authoring tool
lives.

**None of the open items blocks step 1**, because step 1 is an in-memory model with no file
format, no UI and no persistence. Legend placement becomes blocking at step 5; the format,
interpolation-space and shadowing questions at step 6; and the authoring tool's home at
step 8. By the time each bites, the model will have taught us things that ought to inform
it anyway. That is the argument for starting now rather than deciding everything first.

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
5. **Legends**: split the ramp painter out from the widget, add the compact per-layer form
   and the multi-legend display, and add variants for stepped, categorical and multi-break
   palettes. camp is the driving consumer — it has no legend at all today.
6. **File loading**: format decision + ADR, search path, registry with provenance and
   reload.
7. **Composition stage** in the transfer pipeline, CPU and GLSL: shading, plus state
   modulation for dim and highlight. Coordinated with unh_marine_autonomy#326.
8. **Authoring tool**, last — by then the model, I/O and validation metrics all exist and
   the tool is largely assembly.

Per-quantity defaults and day/dusk/night selection slot in after step 6, once palettes can
come from configuration.

## Open questions

- Does the anchored-breakpoint mechanism really unify fixed-domain and pivot palettes, or
  do they diverge once we build them?
- Native format: ParaView JSON, or JSON plus first-class `.cpt`?
- Shadowing policy for user palettes over built-ins: forbidden, warned, or namespaced?
- Legend placement in camp: ramp strips in the layer tree, pinned legends in the view, a
  dock, or some combination — and what the default is on a busy chart.
- State modulation: one combined factor from the consumer, or named contributors the
  library composes? And is dimming a strong enough channel on its own for provisional data?
- A multi-break legend needs non-linear tick placement and break markers; are the break
  markers draggable in the compact form, or only in the expanded one?
- What does a consumer do when a persisted palette name is no longer installed? Silently
  falling back to grayscale on a boat is a bad failure.
- Do we adopt S-100's CIE xyY component-wise ramp interpolation, or keep interpolating in
  sRGB? (Answered for us if we want catalogue-level S-100 compatibility; a real choice
  otherwise.)
- How far do we take S-100 alignment — mirror the lookup-entry shape and parameter names
  only, or aim to actually load a signed S-102 portrayal catalogue at runtime?
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
- Ware, Stone & Szafir (2023). "Rainbow Colormaps Are Not All Bad." *IEEE CG&A* 43(3),
  88–93. (Crossref lists three authors; some indexes add T.-M. Rhyne — check before citing
  formally.)
- Ware, Mayer, Johnson, Jakobsson, Ferrini (2020). "A global geographic grid system for
  visualizing bathymetry." *GI* 9(2), 375–384.
- Brennan, Ware, … Arsenault, Glang (2003). "Electronic Chart of the Future: The Hampton
  Roads Demonstration Project." *US HYDRO 2003*.
- Kovesi, P. (2015). "Good Colour Maps: How to Design Them." arXiv:1509.03700.
- Crameri, Shephard, Heron (2020). "The misuse of colour in science communication."
  *Nature Communications* 11:5444.
- IHO S-52 Edition 6.1.1 (2015), Presentation Library Appendix 2.
- IHO S-100 Edition 5.2.1 (December 2025), Part 9 / Part 9a (Portrayal), esp. clause
  9-12.7 "The Coverage package"; Part 16a (Harmonised Portrayal).
- IHO S-98 Edition 2.0.0 (October 2025), Appendix D — Enhanced Safety Contour and Water
  Level Adjustment.
- IHO S-102 Edition 3.0.0 (December 2024) and Portrayal Catalogue 3.0.0; S-104 Edition
  2.0.0 (no portrayal catalogue by design); S-111 Edition 2.0.0 and Portrayal Catalogue.
