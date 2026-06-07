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
#include <vector>

#include "marine_colormap/color.hpp"
#include "marine_colormap/palette.hpp"
#include "marine_colormap/transfer.hpp"

namespace marine_colormap
{

/// Map a scalar `value` to a color through `pal` and `p` on the CPU.
///
/// Stays in floating point until the caller chooses to quantize (no forced 8-bit
/// input quantization). Order: non-finite -> nodata_color; value < min and
/// has_below_color -> below_color; otherwise normalize -> gain/contrast ->
/// palette sample -> alpha. By construction this equals
/// `bake_lut(...)[round(clamp(normalize(value, min, max)) * (N - 1))]` up to
/// quantization, so the CPU result and the GPU LUT path agree.
Rgba lookup(float value, const Palette & pal, const TransferParams & p);

/// Bake an `n`-entry 1-D lookup table over the normalized range [0, 1], applying
/// gain, contrast and the alpha ramp (but NOT min/max, which the GPU shader
/// applies as the normalize step before indexing, and NOT the below/no-data
/// sentinels, which are applied around the LUT). This is the texture the GPU
/// path uploads. `n` is clamped to at least 1.
std::vector<Rgba8> bake_lut(const Palette & pal, const TransferParams & p, std::size_t n);

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__COLORMAP_HPP_
