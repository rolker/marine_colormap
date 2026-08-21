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

#include <limits>
#include <string>

#include "marine_colormap/occupancy.hpp"

using marine_colormap::LookupTable;
using marine_colormap::Rgba;
using marine_colormap::Rgba8;
using marine_colormap::occupancy_costmap_table;
using marine_colormap::to_rgba8;

namespace
{

/// rviz's own formulae, transcribed from
/// rviz_default_plugins/src/.../map/palette_builder.cpp, as the reference.
Rgba8 rviz_costmap(int value)
{
  if (value == 0) {return Rgba8{0, 0, 0, 0};}
  if (value >= 1 && value <= 98) {
    const unsigned char v = static_cast<unsigned char>(255 * value / 100);
    return Rgba8{v, 0, static_cast<unsigned char>(255 - v), 255};
  }
  if (value == 99) {return Rgba8{0, 255, 255, 255};}
  if (value == 100) {return Rgba8{255, 0, 255, 255};}
  if (value >= 101 && value <= 127) {return Rgba8{0, 255, 0, 255};}
  if (value == -1) {return Rgba8{0x70, 0x89, 0x86, 255};}
  // -128..-2 map to palette indices 128..254.
  const int idx = value + 256;
  return Rgba8{255, static_cast<unsigned char>(255 * (idx - 128) / (254 - 128)), 0, 255};
}

int diff(unsigned char a, unsigned char b)
{
  return (a > b) ? (a - b) : (b - a);
}

void expect_exact(int value, const Rgba8 & got, const Rgba8 & want)
{
  EXPECT_EQ(static_cast<int>(want.r), static_cast<int>(got.r)) << "R at " << value;
  EXPECT_EQ(static_cast<int>(want.g), static_cast<int>(got.g)) << "G at " << value;
  EXPECT_EQ(static_cast<int>(want.b), static_cast<int>(got.b)) << "B at " << value;
  EXPECT_EQ(static_cast<int>(want.a), static_cast<int>(got.a)) << "A at " << value;
}

}  // namespace

TEST(OccupancyCostmap, DistinguishedValuesAreBitExact)
{
  const LookupTable t = occupancy_costmap_table();
  for (const int v : {0, 99, 100, -1, 101, 127, -128, -2}) {
    expect_exact(v, to_rgba8(t.lookup(static_cast<float>(v))), rviz_costmap(v));
  }
}

TEST(OccupancyCostmap, LethalIsMagentaAndInscribedIsCyan)
{
  // Operators read these two on sight; state them plainly rather than only via
  // the reference implementation.
  const LookupTable t = occupancy_costmap_table();
  const Rgba8 lethal = to_rgba8(t.lookup(100.0f));
  EXPECT_EQ(255, static_cast<int>(lethal.r));
  EXPECT_EQ(0, static_cast<int>(lethal.g));
  EXPECT_EQ(255, static_cast<int>(lethal.b));

  const Rgba8 inscribed = to_rgba8(t.lookup(99.0f));
  EXPECT_EQ(0, static_cast<int>(inscribed.r));
  EXPECT_EQ(255, static_cast<int>(inscribed.g));
  EXPECT_EQ(255, static_cast<int>(inscribed.b));
}

TEST(OccupancyCostmap, FreeSpaceIsFullyTransparent)
{
  const LookupTable t = occupancy_costmap_table();
  const Rgba free_space = t.lookup(0.0f);
  EXPECT_FLOAT_EQ(0.0f, free_space.a);
}

TEST(OccupancyCostmap, RampsTrackRvizWithinOneEightBitLevel)
{
  // The ramp itself is exact; rviz truncates its integer division where this
  // library rounds at the end of the pipeline, so allow one level.
  const LookupTable t = occupancy_costmap_table();
  for (int v = 1; v <= 98; ++v) {
    const Rgba8 got = to_rgba8(t.lookup(static_cast<float>(v)));
    const Rgba8 want = rviz_costmap(v);
    EXPECT_LE(diff(got.r, want.r), 1) << "R at " << v;
    EXPECT_EQ(0, static_cast<int>(got.g)) << "G at " << v;
    EXPECT_LE(diff(got.b, want.b), 1) << "B at " << v;
    EXPECT_EQ(255, static_cast<int>(got.a)) << "A at " << v;
  }
  for (int v = -128; v <= -2; ++v) {
    const Rgba8 got = to_rgba8(t.lookup(static_cast<float>(v)));
    const Rgba8 want = rviz_costmap(v);
    EXPECT_EQ(255, static_cast<int>(got.r)) << "R at " << v;
    EXPECT_LE(diff(got.g, want.g), 1) << "G at " << v;
    EXPECT_EQ(0, static_cast<int>(got.b)) << "B at " << v;
  }
}

TEST(OccupancyCostmap, CostRampRunsBlueToRed)
{
  const LookupTable t = occupancy_costmap_table();
  const Rgba low = t.lookup(1.0f);
  const Rgba high = t.lookup(98.0f);
  EXPECT_GT(low.b, low.r);    // nearly free -> blue
  EXPECT_GT(high.r, high.b);  // nearly lethal -> red
}

TEST(OccupancyCostmap, EveryInt8ValueResolvesToARealEntry)
{
  // The whole point of a fixed-domain table: nothing falls through to a
  // sentinel, so no legal cell can render as an unexplained hole.
  const LookupTable t = occupancy_costmap_table();
  for (int v = -128; v <= 127; ++v) {
    EXPECT_TRUE(t.match(static_cast<float>(v)).has_value()) << "no entry for " << v;
  }
}

TEST(OccupancyCostmap, DomainCoversTheWholeInt8RangeInclusively)
{
  const LookupTable t = occupancy_costmap_table();
  ASSERT_TRUE(t.domain_min().has_value());
  ASSERT_TRUE(t.domain_max().has_value());
  EXPECT_FLOAT_EQ(-128.0f, *t.domain_min());
  EXPECT_FLOAT_EQ(127.0f, *t.domain_max());
  // ClosedInterval at both extremes, so under/over are unreachable in range.
  EXPECT_TRUE(t.domain_min_inclusive());
  EXPECT_TRUE(t.domain_max_inclusive());
}

TEST(OccupancyCostmap, UnknownIsANamedEntryNotTheBadSentinel)
{
  // -1 is a legal, meaningful value in this domain. Conflating it with invalid
  // data would be a category error -- and the two must be distinguishable.
  const LookupTable t = occupancy_costmap_table();
  const auto match = t.match(-1.0f);
  ASSERT_TRUE(match.has_value());
  EXPECT_EQ("Unknown", t.entries()[*match].label);

  const Rgba unknown = t.lookup(-1.0f);
  EXPECT_GT(unknown.a, 0.0f);  // opaque, unlike `bad`

  const Rgba bad = t.lookup(std::numeric_limits<float>::quiet_NaN());
  EXPECT_FLOAT_EQ(0.0f, bad.a);
}

TEST(OccupancyCostmap, EveryEntryIsLabelledForALegend)
{
  const LookupTable t = occupancy_costmap_table();
  ASSERT_FALSE(t.entries().empty());
  for (const auto & e : t.entries()) {
    EXPECT_FALSE(e.label.empty());
  }
}

TEST(OccupancyCostmap, NoNewMachineryWasNeededForAFixedDomain)
{
  // The vision document's hypothesis: a table keyed by absolute values owns its
  // domain intrinsically, so "fixed domain" needs no range model at all. If this
  // ever requires one, the hypothesis is wrong and the document needs updating.
  const LookupTable t = occupancy_costmap_table();
  const LookupTable copy = occupancy_costmap_table();
  // Same answers with no range object anywhere in the call.
  for (const int v : {-128, -1, 0, 50, 99, 100, 127}) {
    const Rgba a = t.lookup(static_cast<float>(v));
    const Rgba b = copy.lookup(static_cast<float>(v));
    EXPECT_FLOAT_EQ(a.r, b.r);
    EXPECT_FLOAT_EQ(a.a, b.a);
  }
}
