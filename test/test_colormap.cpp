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

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

#include "marine_colormap/colormap.hpp"

namespace
{

using marine_colormap::bake_lut;
using marine_colormap::find_palette;
using marine_colormap::lookup;
using marine_colormap::Rgba8;
using marine_colormap::to_rgba8;
using marine_colormap::TransferParams;

bool same8(const Rgba8 & a, const Rgba8 & b)
{
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

}  // namespace

TEST(Colormap, LookupEndpoints)
{
  const auto & t = *find_palette("thermal");
  TransferParams p;  // min 0, max 1, unit gain/contrast
  EXPECT_TRUE(same8(to_rgba8(lookup(0.0f, t, p)), Rgba8{77, 77, 77, 255}));
  EXPECT_TRUE(same8(to_rgba8(lookup(1.0f, t, p)), Rgba8{153, 10, 15, 255}));
}

TEST(Colormap, NonFiniteReturnsNoData)
{
  const auto & g = *find_palette("grayscale");
  TransferParams p;
  p.nodata_color = {0.0f, 0.0f, 0.0f, 0.0f};
  const auto nan = lookup(std::numeric_limits<float>::quiet_NaN(), g, p);
  EXPECT_FLOAT_EQ(nan.a, 0.0f);
  const auto inf = lookup(std::numeric_limits<float>::infinity(), g, p);
  EXPECT_FLOAT_EQ(inf.a, 0.0f);
}

TEST(Colormap, BelowColorSentinel)
{
  const auto & g = *find_palette("grayscale");
  TransferParams p;
  p.min = 0.0f;
  p.max = 1.0f;
  p.has_below_color = true;
  p.below_color = {1.0f, 1.0f, 1.0f, 1.0f};  // white below floor (rviz-style)
  EXPECT_TRUE(same8(to_rgba8(lookup(-0.1f, g, p)), Rgba8{255, 255, 255, 255}));
  // In range still maps normally (black at the floor).
  EXPECT_TRUE(same8(to_rgba8(lookup(0.0f, g, p)), Rgba8{0, 0, 0, 255}));
}

TEST(Colormap, BakeLutSizeAndEndpoints)
{
  const auto & t = *find_palette("thermal");
  TransferParams p;
  const auto lut = bake_lut(t, p, 256);
  ASSERT_EQ(lut.size(), 256u);
  EXPECT_TRUE(same8(lut.front(), Rgba8{77, 77, 77, 255}));
  EXPECT_TRUE(same8(lut.back(), Rgba8{153, 10, 15, 255}));
}

TEST(Colormap, BakeLutClampsSizeToAtLeastOne)
{
  const auto & g = *find_palette("grayscale");
  TransferParams p;
  EXPECT_EQ(bake_lut(g, p, 0).size(), 1u);
}

TEST(Colormap, CpuLookupMatchesLut)
{
  // The whole point of bake_lut: CPU lookup(value) equals the GPU path
  // LUT[normalize(value)] with no divergence. Use values that land on exact LUT
  // indices (k/255 over [0,1] with N=256) so the comparison is exact.
  const auto & t = *find_palette("thermal");
  TransferParams p;
  p.min = 0.0f;
  p.max = 1.0f;
  const auto lut = bake_lut(t, p, 256);
  for (int k : {0, 1, 50, 100, 170, 255}) {
    const float value = static_cast<float>(k) / 255.0f;
    EXPECT_TRUE(same8(to_rgba8(lookup(value, t, p)), lut[static_cast<std::size_t>(k)]))
      << "mismatch at k=" << k;
  }
}

TEST(Colormap, AlphaRampAppliesAcrossRange)
{
  const auto & g = *find_palette("grayscale");
  TransferParams p;
  p.alpha_ramp = true;
  p.alpha_min = 0.0f;
  p.alpha_max = 1.0f;
  EXPECT_NEAR(lookup(0.0f, g, p).a, 0.0f, 1e-5f);
  EXPECT_NEAR(lookup(1.0f, g, p).a, 1.0f, 1e-5f);
  EXPECT_NEAR(lookup(0.5f, g, p).a, 0.5f, 1e-5f);
  // Without the ramp, alpha stays opaque.
  TransferParams q;
  EXPECT_NEAR(lookup(0.5f, g, q).a, 1.0f, 1e-5f);
}

TEST(Colormap, CpuLookupMatchesLutOffGrid)
{
  // Stronger than CpuLookupMatchesLut, which only used exact grid points: sweep
  // values that do NOT land on a LUT index and confirm lookup(v) matches
  // lut[round(clamp(normalize(v)) * (N-1))] within 1 LSB. Grayscale keeps the
  // per-index delta tiny so the bound is meaningful; alpha_ramp exercises the
  // alpha path off-grid too.
  const auto & g = *find_palette("grayscale");
  TransferParams p;
  p.min = 0.0f;
  p.max = 1.0f;
  p.alpha_ramp = true;
  p.alpha_min = 0.0f;
  p.alpha_max = 1.0f;
  const std::size_t n = 256;
  const auto lut = bake_lut(g, p, n);
  auto close = [](std::uint8_t a, std::uint8_t b) {
      return std::abs(static_cast<int>(a) - static_cast<int>(b)) <= 1;
    };
  for (float v : {0.013f, 0.137f, 0.3017f, 0.5009f, 0.733f, 0.917f}) {
    const float nrm = std::clamp(marine_colormap::normalize(v, p.min, p.max), 0.0f, 1.0f);
    const auto idx = static_cast<std::size_t>(std::lround(nrm * static_cast<float>(n - 1)));
    const Rgba8 a = to_rgba8(lookup(v, g, p));
    const Rgba8 b = lut[idx];
    EXPECT_TRUE(close(a.r, b.r) && close(a.g, b.g) && close(a.b, b.b) && close(a.a, b.a))
      << "v=" << v;
  }
}
