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

#include <stdexcept>

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
  // Append-only contract: the first three indices are stable and name-keyed;
  // count may grow as palettes are appended (e.g. viridis/turbo), so assert
  // >= 3 rather than an exact count that would break on every addition.
  ASSERT_GE(marine_colormap::palette_count(), 3u);
  const auto & names = marine_colormap::palette_names();
  ASSERT_GE(names.size(), 3u);
  EXPECT_EQ(names[0], "grayscale");
  EXPECT_EQ(names[1], "bronze");
  EXPECT_EQ(names[2], "thermal");
  EXPECT_EQ(names[3], "viridis");
  EXPECT_EQ(names[4], "turbo");
  // Name -> index round-trips at stable indices.
  EXPECT_EQ(marine_colormap::palette_index("grayscale").value(), 0u);
  EXPECT_EQ(marine_colormap::palette_index("thermal").value(), 2u);
  EXPECT_EQ(marine_colormap::palette_index("viridis").value(), 3u);
  EXPECT_EQ(marine_colormap::palette_index("turbo").value(), 4u);
  EXPECT_EQ(marine_colormap::palette_index("quality").value(), 5u);
  EXPECT_FALSE(marine_colormap::palette_index("plasma").has_value());
}

TEST(Palette, ViridisAndTurboGoldenEndpoints)
{
  // Canonical matplotlib endpoints (8-bit), locking the embedded tables.
  const auto & v = *marine_colormap::find_palette("viridis");
  expect_color(to_rgba8(v.sample(0.0f)), 68, 1, 84);
  expect_color(to_rgba8(v.sample(1.0f)), 253, 231, 37);
  const auto & t = *marine_colormap::find_palette("turbo");
  expect_color(to_rgba8(t.sample(0.0f)), 48, 18, 59);
  expect_color(to_rgba8(t.sample(1.0f)), 122, 4, 3);
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

TEST(Palette, QualityWarningRamp)
{
  // Bathy-uncertainty warning ramp (issue #7): green (good) -> yellow (caution)
  // -> red (bad). Assert the dominant channel at each anchor rather than exact
  // RGB so the ramp's hue intent is locked without pinning tweakable values.
  const auto * q = marine_colormap::find_palette("quality");
  ASSERT_NE(q, nullptr);

  const Rgba8 good = to_rgba8(q->sample(0.0f));
  EXPECT_GT(static_cast<int>(good.g), static_cast<int>(good.r));  // greenish
  EXPECT_GT(static_cast<int>(good.g), static_cast<int>(good.b));

  const Rgba8 caution = to_rgba8(q->sample(0.5f));
  EXPECT_GT(static_cast<int>(caution.r), 150);  // yellowish: high R and G,
  EXPECT_GT(static_cast<int>(caution.g), 150);  // low B
  EXPECT_LT(static_cast<int>(caution.b), 100);

  const Rgba8 bad = to_rgba8(q->sample(1.0f));
  EXPECT_GT(static_cast<int>(bad.r), static_cast<int>(bad.g));  // reddish
  EXPECT_GT(static_cast<int>(bad.r), static_cast<int>(bad.b));
}

TEST(Palette, SampleClampsOutOfRange)
{
  const auto & g = *marine_colormap::find_palette("grayscale");
  expect_color(to_rgba8(g.sample(-5.0f)), 0, 0, 0);
  expect_color(to_rgba8(g.sample(5.0f)), 255, 255, 255);
}

TEST(Palette, ConstructorSortsUnsortedStops)
{
  using marine_colormap::ColorStop;
  using marine_colormap::Palette;
  using marine_colormap::Rgba;
  // Stops supplied out of order: red at t=1 before black at t=0. The ctor must
  // sort them so sample() interpolates black -> red, not the reverse.
  Palette p("custom", {
    ColorStop{1.0f, Rgba{1.0f, 0.0f, 0.0f, 1.0f}},
    ColorStop{0.0f, Rgba{0.0f, 0.0f, 0.0f, 1.0f}}});
  expect_color(to_rgba8(p.sample(0.0f)), 0, 0, 0);
  expect_color(to_rgba8(p.sample(1.0f)), 255, 0, 0);
  expect_color(to_rgba8(p.sample(0.5f)), 128, 0, 0);
}
