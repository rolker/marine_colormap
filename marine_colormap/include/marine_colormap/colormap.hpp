// Copyright 2026 Roland Arsenault
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MARINE_COLORMAP__COLORMAP_HPP_
#define MARINE_COLORMAP__COLORMAP_HPP_

#include <cstddef>
#include <optional>
#include <vector>

#include "marine_colormap/color.hpp"
#include "marine_colormap/lookup.hpp"
#include "marine_colormap/palette.hpp"
#include "marine_colormap/transfer.hpp"

namespace marine_colormap
{

/// Map a scalar `value` to a color through `pal` and `p` on the CPU.
///
/// Stays in floating point until the caller chooses to quantize (no forced 8-bit
/// input quantization). Order: non-finite -> nodata_color; value < min and
/// has_below_color -> below_color; otherwise normalize -> gain/contrast ->
/// palette sample -> alpha. The nodata/below sentinels take precedence over the
/// alpha ramp -- they return their own color and alpha verbatim. The alpha ramp
/// (when enabled) is a function of the normalized data position, independent of
/// gain/contrast. By construction the in-range path equals
/// `bake_lut(...)[round(clamp(normalize(value, min, max)) * (N - 1))]` up to
/// quantization, so the CPU result and the GPU LUT path agree.
Rgba lookup(float value, const Palette & pal, const TransferParams & p);

/// Bake an `n`-entry 1-D lookup table over the normalized range [0, 1], applying
/// gain, contrast and the alpha ramp (but NOT min/max, which the GPU shader
/// applies as the normalize step before indexing, and NOT the below/no-data
/// sentinels, which are applied around the LUT). This is the texture the GPU
/// path uploads. `n` is clamped to at least 1.
std::vector<Rgba8> bake_lut(const Palette & pal, const TransferParams & p, std::size_t n);

/// Bake an `n`-entry LUT **through a `BreakpointMap`**, so anchored breakpoints
/// reach the GPU path (issue #23; the vision doc's GPU-parity invariant).
///
/// **How it works, and why the shader needs no change.** Entry `i` is filled
/// with the colour for the data value the shader's *existing linear* normalize
/// maps to `i / (n - 1)`; `map` then re-places that value in palette space. The
/// anchor therefore lives entirely in the bake: the GLSL side keeps computing
/// `t = (v - lo) / (hi - lo)` and sampling the LUT at `t`, exactly as before,
/// and the anchor costs nothing at render time.
///
/// **The one consequence consumers MUST handle: this LUT is range-dependent.**
/// An unanchored `bake_lut()` depends only on the palette, so consumers cache it
/// by palette name. This one is a function of `map`'s domain *and* its breaks,
/// so a cache keyed on the name alone will serve a stale table after a range
/// change — rendering wrong colours silently, with no crash and no log line.
/// Cache keys must include the range and the breakpoints. (Consumers with a
/// written record asserting LUT range-independence — e.g. camp's ADR-0008
/// Decision #2 — should amend it: that property holds only on the unanchored
/// path.)
///
/// The value sweep uses `map.lo()`/`map.hi()` rather than raw bounds, so an
/// inverted or non-finite domain (which `BreakpointMap` normalizes) cannot make
/// the sweep and the map disagree at the ends.
///
/// `p`'s gain/contrast/alpha-ramp apply exactly as in `bake_lut()`; `min`/`max`
/// are ignored here as they are there, since the breakpoint domain supersedes
/// them. `n` is clamped to at least 1.
std::vector<Rgba8> bake_lut(
  const Palette & pal, const TransferParams & p, std::size_t n, const BreakpointMap & map);

/// True when `pal` declares a `shoreline_position` — i.e. when anchoring it is
/// meaningful. UIs use this to offer an anchor control only where it does
/// something; `bake_shoreline_anchored_lut()` uses it as its gate.
bool has_shoreline(const Palette & pal);

/// Convenience for the topo-bathy case: pin the palette's declared shoreline to
/// an absolute data value over `[lo, hi]` (issue #23).
///
/// This is the single-hinge specialisation of the `BreakpointMap` overload above
/// — the GMT-style pivot. Use that overload directly for two or more anchored
/// breaks (shoreline *and* safety contour).
///
/// **Falls back to `bake_lut(pal, p, n)` byte-for-byte** when `anchor_value` is
/// absent or non-finite, or when `pal` declares no `shoreline_position`. The
/// palette gate is deliberate: warping a general-purpose ramp (grayscale,
/// viridis) around a "shoreline" it does not have would produce colours that
/// mean nothing, so anchoring such a palette is a no-op rather than a surprise.
///
/// Degenerate inputs clamp and never throw, per `BreakpointMap`'s contract. An
/// anchor outside `[lo, hi]` is the **ordinary** case, not an error: a survey
/// line with no land in view has its datum break above `hi`.
std::vector<Rgba8> bake_shoreline_anchored_lut(
  const Palette & pal, const TransferParams & p, float lo, float hi,
  std::optional<float> anchor_value, std::size_t n);

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__COLORMAP_HPP_
