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

#include "marine_colormap/palette.hpp"

namespace
{

using marine_colormap::Rgba8;
using marine_colormap::to_rgba8;

void expect_color(const Rgba8 & c, int r, int g, int b, int a = 255)
{
  EXPECT_EQ(static_cast<int>(c.r), r);
  EXPECT_EQ(static_cast<int>(c.g), g);
  EXPECT_EQ(static_cast<int>(c.b), b);
  EXPECT_EQ(static_cast<int>(c.a), a);
}

}  // namespace

TEST(Palette, RegistryIsAppendOnlyAndNameKeyed)
{
  ASSERT_EQ(marine_colormap::palette_count(), 3u);
  const auto & names = marine_colormap::palette_names();
  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0], "grayscale");
  EXPECT_EQ(names[1], "bronze");
  EXPECT_EQ(names[2], "thermal");
  // Name -> index round-trips at stable indices.
  EXPECT_EQ(marine_colormap::palette_index("grayscale").value(), 0u);
  EXPECT_EQ(marine_colormap::palette_index("thermal").value(), 2u);
  EXPECT_FALSE(marine_colormap::palette_index("viridis").has_value());
}

TEST(Palette, FindAndIndexLookup)
{
  EXPECT_NE(marine_colormap::find_palette("bronze"), nullptr);
  EXPECT_EQ(marine_colormap::find_palette("nope"), nullptr);
  EXPECT_EQ(marine_colormap::palette(1).name(), "bronze");
  EXPECT_THROW(marine_colormap::palette(99), std::out_of_range);
}

TEST(Palette, GrayscaleEndpointsAndMidpoint)
{
  const auto & g = *marine_colormap::find_palette("grayscale");
  expect_color(to_rgba8(g.sample(0.0f)), 0, 0, 0);
  expect_color(to_rgba8(g.sample(1.0f)), 255, 255, 255);
  expect_color(to_rgba8(g.sample(0.5f)), 128, 128, 128);  // 127.5 rounds to 128
}

TEST(Palette, ThermalGoldenStops)
{
  // Canonical de-duplicated thermal ramp (ADR-0001). Locks the reference.
  const auto & t = *marine_colormap::find_palette("thermal");
  expect_color(to_rgba8(t.sample(0.0f)), 77, 77, 77);
  expect_color(to_rgba8(t.sample(1.0f)), 153, 10, 15);
  // Interior stop: index 6 of 12 -> t = 6/11.
  expect_color(to_rgba8(t.sample(6.0f / 11.0f)), 252, 179, 46);
}

TEST(Palette, BronzeGoldenEndpoints)
{
  const auto & b = *marine_colormap::find_palette("bronze");
  expect_color(to_rgba8(b.sample(0.0f)), 0, 0, 0);
  expect_color(to_rgba8(b.sample(1.0f)), 255, 225, 170);
}

TEST(Palette, SampleClampsOutOfRange)
{
  const auto & g = *marine_colormap::find_palette("grayscale");
  expect_color(to_rgba8(g.sample(-5.0f)), 0, 0, 0);
  expect_color(to_rgba8(g.sample(5.0f)), 255, 255, 255);
}
