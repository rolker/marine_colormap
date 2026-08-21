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

#include "marine_colormap/occupancy.hpp"

#include <vector>

namespace marine_colormap
{

namespace
{

Rgba rgb8(int r, int g, int b, int a = 255)
{
  return Rgba{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

/// Ramp endpoints are given as exact fractions rather than rounded 8-bit values
/// so the interpolated result reproduces rviz's formula exactly in float.
///
/// rviz's cost ramp is `v = 255 * i / 100`, i.e. the channel is `i / 100` of
/// full scale. Writing the endpoints as 0.01 and 0.98 makes the linear
/// interpolation across [1, 98] land on `i / 100` at every integer, instead of
/// accumulating the error we would get from starting at round(2.55)/255.
Rgba cost_ramp_at(int i)
{
  const float f = i / 100.0f;
  return Rgba{f, 0.0f, 1.0f - f, 1.0f};
}

/// rviz's illegal-negative ramp, indexed by the *unsigned* palette index:
/// `(255, 255 * (idx - 128) / 126, 0)` for idx in [128, 254]. In value space
/// idx = value + 256, so the green channel is `(value + 128) / 126`.
Rgba illegal_negative_ramp_at(int value)
{
  const float f = (value + 128) / 126.0f;
  return Rgba{1.0f, f, 0.0f, 1.0f};
}

}  // namespace

LookupTable occupancy_costmap_table()
{
  // Ordered low to high. The ranges are disjoint and contiguous, so first-match
  // ordering is not load-bearing here -- but reading in value order makes the
  // ladder obvious.
  std::vector<LookupEntry> entries = {
    // -128..-2: illegal negative, red -> yellow.
    LookupEntry{
      "Illegal (negative)",
      ValueRange{-128.0f, -2.0f, Closure::ClosedInterval},
      illegal_negative_ramp_at(-128),
      illegal_negative_ramp_at(-2)},

    // -1: unknown. A legal, meaningful value, hence a named entry.
    LookupEntry{
      "Unknown",
      ValueRange{-1.0f, -1.0f, Closure::ClosedInterval},
      rgb8(0x70, 0x89, 0x86),
      {}},

    // 0: free space, fully transparent so the chart shows through.
    LookupEntry{
      "Free space",
      ValueRange{0.0f, 0.0f, Closure::ClosedInterval},
      rgb8(0, 0, 0, 0),
      {}},

    // 1..98: cost, blue -> red.
    LookupEntry{
      "Cost",
      ValueRange{1.0f, 98.0f, Closure::ClosedInterval},
      cost_ramp_at(1),
      cost_ramp_at(98)},

    LookupEntry{
      "Inscribed",
      ValueRange{99.0f, 99.0f, Closure::ClosedInterval},
      rgb8(0, 255, 255),
      {}},

    LookupEntry{
      "Lethal",
      ValueRange{100.0f, 100.0f, Closure::ClosedInterval},
      rgb8(255, 0, 255),
      {}},

    // 101..127: values outside the documented 0..100 range but still positive.
    LookupEntry{
      "Illegal (positive)",
      ValueRange{101.0f, 127.0f, Closure::ClosedInterval},
      rgb8(0, 255, 0),
      {}},
  };

  Sentinels sentinels;
  // Non-finite input is invalid data, distinct from the `Unknown` entry above.
  sentinels.bad = rgb8(0, 0, 0, 0);
  // Unreachable for any int8 value -- the entries are contiguous across the
  // whole domain -- but transparent is the safe answer if a caller ever feeds
  // this table something out of band.
  sentinels.unmapped = rgb8(0, 0, 0, 0);

  return LookupTable(std::move(entries), sentinels);
}

}  // namespace marine_colormap
