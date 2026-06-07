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
///   2. Upload `bake_lut(palette, TransferParams{}, N)` (identity transfer) as
///      an Nx1 RGBA8 1-D LUT texture.
///   3. Per fragment:
///        float t = marine_colormap_response(
///                    marine_colormap_normalize(value, u_min, u_max),
///                    u_gain, u_contrast);
///        color = texture(u_lut, vec2(t, 0.5));
///   4. Handle the below-floor / no-data sentinels in your own `main()` -- a
///      clamped LUT coordinate cannot represent them.
///
/// On GLES, declare `precision highp float;` -- dB-range data bands at mediump.
const char * colormap_glsl();

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__SHADER_HPP_
