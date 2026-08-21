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

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "marine_colormap/lookup.hpp"
#include "marine_colormap/palette.hpp"

using marine_colormap::Breakpoint;
using marine_colormap::BreakpointMap;
using marine_colormap::Palette;
using marine_colormap::Rgba;

namespace
{

/// Colours are stored as float; compare against the 8-bit source values.
void expect_rgb8(const Rgba & c, int r, int g, int b)
{
  EXPECT_NEAR(r / 255.0f, c.r, 1.0f / 512.0f);
  EXPECT_NEAR(g / 255.0f, c.g, 1.0f / 512.0f);
  EXPECT_NEAR(b / 255.0f, c.b, 1.0f / 512.0f);
}

float channel_distance(const Rgba & a, const Rgba & b)
{
  return std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b);
}

/// Takes a `const char *` rather than `const std::string &` so a call with a
/// literal does not create a temporary that -Wdangling-reference flags.
///
/// Never returns null: callers dereference the result immediately, and a null
/// there would segfault and bury the real assertion failure. A missing palette
/// registers a failure and falls back to a valid one so the test reports the
/// actual problem and keeps running.
const Palette * by_name(const char * n)
{
  const Palette * p = marine_colormap::find_palette(n);
  if (p == nullptr) {
    ADD_FAILURE() << "palette '" << n << "' is missing from the registry";
    return &marine_colormap::palette(0);
  }
  return p;
}

}  // namespace

// --- Registry stability ------------------------------------------------------

TEST(TopoBathyRegistry, AppendedWithoutDisturbingExistingIndices)
{
  // The registry is documented as append-only because consumers persist a
  // selection by name or index. Assert that invariant -- the existing names
  // still sit at their original indices, and the new ones were appended after
  // them -- rather than pinning the whole list, which would fail every time a
  // future palette is added without anything actually being wrong.
  const std::vector<std::string> original = {
    "grayscale", "bronze", "thermal", "viridis", "turbo", "quality"};
  const std::vector<std::string> & names = marine_colormap::palette_names();
  ASSERT_GE(names.size(), original.size() + 2);
  for (std::size_t i = 0; i < original.size(); ++i) {
    EXPECT_EQ(original[i], names[i]) << "index " << i << " moved";
  }
  ASSERT_TRUE(marine_colormap::palette_index("oleron").has_value());
  ASSERT_TRUE(marine_colormap::palette_index("hypsometric").has_value());
  EXPECT_EQ(6u, *marine_colormap::palette_index("oleron"));
  EXPECT_EQ(7u, *marine_colormap::palette_index("hypsometric"));
  EXPECT_EQ(names.size(), marine_colormap::palette_count());
}

TEST(TopoBathyRegistry, GeneralPurposeRampsStillCarryNoDomain)
{
  for (const char * n : {"grayscale", "bronze", "thermal", "viridis", "turbo", "quality"}) {
    EXPECT_FALSE(by_name(n)->domain().has_value()) << n;
  }
}

// --- oleron ------------------------------------------------------------------

TEST(Oleron, EndStopsMatchTheSourceTable)
{
  const Palette & p = *by_name("oleron");
  expect_rgb8(p.sample(0.0f), 26, 38, 89);      // deepest navy
  expect_rgb8(p.sample(1.0f), 253, 253, 230);   // highest cream
}

TEST(Oleron, HasAHardShorelineDiscontinuityAtTheMidpoint)
{
  const Palette & p = *by_name("oleron");
  // Exactly at the break, the sea side owns the boundary — matching the
  // GeLtInterval convention in lookup.hpp.
  expect_rgb8(p.sample(0.5f), 230, 242, 255);
  // Just above it we are on the land ramp, and the two are far apart.
  const Rgba just_above = p.sample(0.5f + 1e-3f);
  EXPECT_GT(channel_distance(p.sample(0.5f), just_above), 0.5f);
  EXPECT_LT(just_above.r, 0.5f);   // dark green, not pale blue
  EXPECT_GT(just_above.g, just_above.b);
}

TEST(Oleron, SeaSideLightensTowardTheShoreline)
{
  const Palette & p = *by_name("oleron");
  float previous = -1.0f;
  for (float t = 0.0f; t < 0.5f; t += 0.02f) {
    const Rgba c = p.sample(t);
    const float lightness = c.r + c.g + c.b;
    EXPECT_GT(lightness, previous) << "at t=" << t;
    previous = lightness;
  }
}

TEST(Oleron, DeclaresAShorelineButNoNaturalRange)
{
  // It ships normalised -1/+1 as a stretchable master, so claiming a natural
  // elevation range would be a fiction.
  const auto & d = by_name("oleron")->domain();
  ASSERT_TRUE(d.has_value());
  ASSERT_TRUE(d->shoreline_position.has_value());
  EXPECT_FLOAT_EQ(0.5f, *d->shoreline_position);
  EXPECT_FALSE(d->has_natural_range());
}

// --- hypsometric -------------------------------------------------------------

TEST(Hypsometric, NodesMatchTheGeoZuiTable)
{
  const Palette & p = *by_name("hypsometric");
  ASSERT_TRUE(p.domain().has_value());
  const auto & d = *p.domain();
  ASSERT_TRUE(d.has_natural_range());
  ASSERT_TRUE(d.natural_min.has_value());
  ASSERT_TRUE(d.natural_max.has_value());
  const auto at = [&](float metres) {
      return p.sample((metres - *d.natural_min) / (*d.natural_max - *d.natural_min));
    };
  expect_rgb8(at(-6000.0f), 17, 10, 59);      // abyssal navy
  expect_rgb8(at(0.0f), 161, 255, 230);       // shoreline mint
  expect_rgb8(at(500.0f), 163, 127, 47);      // brown uplands
  expect_rgb8(at(9000.0f), 220, 220, 220);    // snow
}

TEST(Hypsometric, RunsGreenThroughBrownToGreyWithAltitude)
{
  const Palette & p = *by_name("hypsometric");
  ASSERT_TRUE(p.domain().has_value());
  const auto & d = *p.domain();
  ASSERT_TRUE(d.has_natural_range());
  const auto at = [&](float m) {
      return p.sample((m - *d.natural_min) / (*d.natural_max - *d.natural_min));
    };
  const Rgba lowland = at(100.0f);
  const Rgba upland = at(1000.0f);
  const Rgba peak = at(9000.0f);
  EXPECT_GT(lowland.g, lowland.b);                  // yellow-green, not blue
  EXPECT_GT(upland.r, upland.b);                    // brown
  EXPECT_LT(upland.r + upland.g + upland.b, lowland.r + lowland.g + lowland.b);
  EXPECT_NEAR(peak.r, peak.g, 0.01f);               // grey/white: neutral
  EXPECT_NEAR(peak.g, peak.b, 0.01f);
  EXPECT_GT(peak.r, 0.8f);
}

TEST(Hypsometric, DeclaresItsNaturalRangeAndShoreline)
{
  const auto & d = by_name("hypsometric")->domain();
  ASSERT_TRUE(d.has_value());
  EXPECT_TRUE(d->has_natural_range());
  EXPECT_FLOAT_EQ(-6000.0f, *d->natural_min);
  EXPECT_FLOAT_EQ(9000.0f, *d->natural_max);
  ASSERT_TRUE(d->shoreline_position.has_value());
  EXPECT_FLOAT_EQ(0.4f, *d->shoreline_position);
}

// --- Composition: the actual target -----------------------------------------

TEST(TopoBathyComposition, ShorelineAnchoredOverAnAsymmetricDomain)
{
  // The worked example: 80 m of water and 5 m of land, each filling its half of
  // the palette. This is what the vision doc calls the GMT-hinge behaviour.
  const Palette & p = *by_name("oleron");
  ASSERT_TRUE(p.domain().has_value());
  ASSERT_TRUE(p.domain()->shoreline_position.has_value());
  const float shoreline = *p.domain()->shoreline_position;
  const BreakpointMap m(-80.0f, 5.0f, {Breakpoint{0.0f, shoreline}});

  EXPECT_FLOAT_EQ(0.0f, m.normalize(-80.0f));
  EXPECT_FLOAT_EQ(0.25f, m.normalize(-40.0f));
  EXPECT_FLOAT_EQ(0.5f, m.normalize(0.0f));
  EXPECT_FLOAT_EQ(1.0f, m.normalize(5.0f));

  // A sounding just below the surface is sea-coloured; just above is land.
  expect_rgb8(p.sample(m.normalize(0.0f)), 230, 242, 255);
  const Rgba land = p.sample(m.normalize(0.1f));
  EXPECT_GT(land.g, land.b);
}

TEST(TopoBathyComposition, SafetyContourNeedsNoNewLibraryCode)
{
  // Two anchored breaks: the shoreline at 0 and an operator-set safety contour
  // at -5 m, which BreakpointMap already supports.
  const Palette & p = *by_name("oleron");
  const BreakpointMap m(
    -40.0f, 5.0f, {Breakpoint{-5.0f, 0.30f}, Breakpoint{0.0f, 0.50f}});

  EXPECT_FLOAT_EQ(0.30f, m.normalize(-5.0f));
  EXPECT_FLOAT_EQ(0.50f, m.normalize(0.0f));
  // Water shallower than the safety contour occupies its own colour band.
  EXPECT_GT(m.normalize(-2.5f), 0.30f);
  EXPECT_LT(m.normalize(-2.5f), 0.50f);
  // And the palette still resolves everywhere.
  for (float v = -40.0f; v <= 5.0f; v += 0.5f) {
    const Rgba c = p.sample(m.normalize(v));
    EXPECT_FALSE(std::isnan(c.r));
  }
}

TEST(TopoBathyComposition, NoLandInViewKeepsTheWaterOnTheWaterHalf)
{
  // The degenerate case that matters on a survey line: the shoreline break is
  // above every sounding, so it clamps onto hi and the domain occupies only the
  // sea half of the palette rather than stretching into the land colours.
  const Palette & p = *by_name("oleron");
  const BreakpointMap m(-80.0f, -10.0f, {Breakpoint{0.0f, 0.5f}});
  EXPECT_FLOAT_EQ(0.5f, m.normalize(-10.0f));
  for (float v = -80.0f; v <= -10.0f; v += 1.0f) {
    const Rgba c = p.sample(m.normalize(v));
    EXPECT_GT(c.b, c.g) << "at " << v << " m: should still be a sea colour";
  }
}
