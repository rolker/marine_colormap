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

#ifndef MARINE_COLORMAP__COLOR_HPP_
#define MARINE_COLORMAP__COLOR_HPP_

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace marine_colormap
{

/// Color representation contract (ADR-0001): channels are **sRGB-encoded display
/// values** in [0, 1] with **straight (non-premultiplied) alpha**. Interpolation
/// happens directly in this stored space (the convention the existing sonar
/// colormaps already use and the one matplotlib/turbo tables are published in).
struct Rgba
{
  float r{0.0f};
  float g{0.0f};
  float b{0.0f};
  float a{1.0f};
};

/// 8-bit form for LUT upload / display output. The only quantization step.
struct Rgba8
{
  std::uint8_t r{0};
  std::uint8_t g{0};
  std::uint8_t b{0};
  std::uint8_t a{255};
};

/// Per-channel linear interpolation, t in [0, 1] (clamped).
inline Rgba lerp(const Rgba & a, const Rgba & b, float t)
{
  t = std::clamp(t, 0.0f, 1.0f);
  return Rgba{
    a.r + (b.r - a.r) * t,
    a.g + (b.g - a.g) * t,
    a.b + (b.b - a.b) * t,
    a.a + (b.a - a.a) * t};
}

/// Quantize a float color to 8-bit, clamping to [0, 1] and rounding to nearest.
inline Rgba8 to_rgba8(const Rgba & c)
{
  auto q = [](float v) -> std::uint8_t {
      // A non-finite channel (NaN/inf from a malformed palette or transfer
      // param) would make std::lround undefined; map it to 0 deterministically.
      if (!std::isfinite(v)) {
        return 0;
      }
      return static_cast<std::uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
  return Rgba8{q(c.r), q(c.g), q(c.b), q(c.a)};
}

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__COLOR_HPP_
