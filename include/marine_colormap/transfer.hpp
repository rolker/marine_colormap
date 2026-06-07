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

#ifndef MARINE_COLORMAP__TRANSFER_HPP_
#define MARINE_COLORMAP__TRANSFER_HPP_

#include "marine_colormap/color.hpp"

namespace marine_colormap
{

/// The value -> color transfer parameters (ADR-0001).
///
/// Pipeline order is fixed and documented: range normalize (min, max) -> gain
/// -> contrast/gamma -> palette sample -> alpha. `min`/`max` are the *only*
/// step the GPU performs as a uniform before indexing the baked LUT; gain,
/// contrast and the alpha ramp are baked into the LUT. The below/no-data
/// sentinels are applied *around* the LUT (a shader branch), not inside it.
struct TransferParams
{
  float min{0.0f};
  float max{1.0f};
  float gain{1.0f};
  float contrast{1.0f};  ///< gamma: applied as pow(t, 1/contrast) when != 1

  /// Optional value-dependent alpha: when enabled, alpha ramps linearly from
  /// alpha_min (at normalized 0) to alpha_max (at normalized 1). When disabled,
  /// the palette stop's own alpha is used.
  bool alpha_ramp{false};
  float alpha_min{0.0f};
  float alpha_max{1.0f};

  /// Distinct color for values below `min` (e.g. a white background sentinel).
  /// When false, below-range values clamp to the palette's first stop.
  bool has_below_color{false};
  Rgba below_color{1.0f, 1.0f, 1.0f, 1.0f};

  /// Color returned for non-finite input (NaN / inf). Default fully transparent.
  Rgba nodata_color{0.0f, 0.0f, 0.0f, 0.0f};
};

/// Pure normalization shared by the CPU path and the GPU shader: maps `value`
/// onto [0, 1] across [lo, hi]. Returns the **raw** position (may be < 0 or > 1;
/// callers clamp). A degenerate range (hi <= lo) returns 0. Keeping this a
/// single function is what guarantees CPU and GPU agree.
float normalize(float value, float lo, float hi);

/// Apply gain then contrast/gamma to a normalized position. The exact pipeline,
/// which a GPU shader MUST replicate verbatim for CPU/GPU agreement, is:
///   t = clamp01(t);
///   t = clamp01(t * gain);                       // clamp BEFORE gamma
///   if (contrast > 0 && contrast != 1) t = pow(t, 1 / contrast);
///   return clamp01(t);
/// The clamp of `t * gain` *before* the gamma curve is load-bearing: gain acts as
/// a clip (values pushed past 1 saturate). `gain < 0` clamps to 0; `contrast <= 0`
/// is treated as no gamma (passthrough).
float apply_response(float t, float gain, float contrast);

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__TRANSFER_HPP_
