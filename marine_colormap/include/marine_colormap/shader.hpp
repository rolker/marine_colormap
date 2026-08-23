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

#ifndef MARINE_COLORMAP__SHADER_HPP_
#define MARINE_COLORMAP__SHADER_HPP_

namespace marine_colormap
{

/// GLSL source for the colormap math shared by GPU consumers (the Tier-2 GPU
/// path of ADR-0001). It is **version-agnostic** -- no `#version`, no sampler
/// declarations, no `precision` qualifier -- so each consumer prepends its own
/// preamble (desktop GL vs GLES) and writes its own `main()`. Shipping it as a
/// string keeps it in the GL-free core.
///
/// It defines two functions that mirror the CPU `normalize()` and
/// `apply_response()` (see transfer.hpp) **line for line**, which is what
/// guarantees the GPU result matches the CPU path:
///
///   float marine_colormap_normalize(float value, float lo, float hi);
///   float marine_colormap_response(float t, float gain, float contrast);
///
/// GPU pipeline (see README for the full recipe):
///   1. Upload the scalar field as an **R32F** texture -- full-precision input,
///      so the colormap is not bound to 8-bit data.
///   2. Upload `bake_lut(palette, TransferParams{}, N)` as an Nx1 RGBA8 1-D LUT
///      texture. The params MUST be identity here (`TransferParams{}` -- gain 1,
///      contrast 1, alpha_ramp off) because step 3 applies gain/contrast in the
///      shader; see "do not double-apply" below.
///   3. Per fragment:
///        float t = marine_colormap_response(
///                    marine_colormap_normalize(value, u_min, u_max),
///                    u_gain, u_contrast);
///        color = texture(u_lut, vec2(t, 0.5));
///   4. Handle the below-floor / no-data sentinels in your own `main()` -- a
///      clamped LUT coordinate cannot represent them.
///
/// Do not double-apply the transfer: either (a) bake an identity LUT and apply
/// gain/contrast via marine_colormap_response in the shader (above -- makes them
/// free uniforms), or (b) bake the real gain/contrast/alpha_ramp into the LUT
/// (`bake_lut(pal, params, N)`) and DROP marine_colormap_response, sampling the
/// LUT directly at the normalized value. Doing both applies gain/contrast twice.
///
/// ## Anchored breakpoints on the GPU (issue #23)
///
/// The helper above normalizes LINEARLY, which places a palette's land/sea
/// transition wherever the range midpoint happens to fall. To pin that
/// transition to an absolute data value — chart datum, a tide height, a safety
/// contour — do NOT add a breakpoint-aware normalize to the shader. Fold the
/// `BreakpointMap` into the **bake** instead:
///
///     bake_lut(pal, params, N, map)                    // general, N breaks
///     bake_shoreline_anchored_lut(pal, params, lo, hi, anchor, N)   // one hinge
///
/// Entry `i` of the resulting table already holds the colour for the data value
/// that the shader's existing linear `t` maps to `i / (N - 1)`, so **the GLSL
/// side needs no change at all** and the anchor costs nothing per fragment.
///
/// This also sidesteps the double-application hazard described just above:
/// there is no second transfer stage to apply twice.
///
/// **The one thing a consumer must handle: the anchored LUT is RANGE-DEPENDENT.**
/// An unanchored `bake_lut()` depends only on the palette, so consumers commonly
/// cache it keyed on the palette name. An anchored table is a function of the
/// range and the breakpoints as well, so that cache key must widen to include
/// them. Get it wrong and a range change serves a stale table — wrong colours,
/// no crash, no log line, which is the failure mode most likely to reach an
/// operator unnoticed. Consumers holding a written decision that asserts LUT
/// range-independence (e.g. camp's ADR-0008 Decision #2) should amend it: that
/// property holds only on the unanchored path.
///
/// On GLES, declare `precision highp float;` -- dB-range data bands at mediump.
const char * colormap_glsl();

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__SHADER_HPP_
