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

// Tests for the anchored-breakpoint GPU path (issue #23).
//
// These cover the invariant the vision doc calls GPU PARITY: everything the CPU
// core can do, a baked-LUT consumer must be able to do too. BreakpointMap shipped
// CPU-only, so a GPU consumer could not pin a topo-bathy ramp at the shoreline —
// the motivating case in BreakpointMap's own documentation.
//
// The property that makes the whole approach work, and which these tests pin, is
// COMPOSITION: the shader keeps normalizing linearly, and the breakpoint map is
// folded into the bake. So for any value v, sampling the baked table at the
// shader's linear position must equal sampling the palette at map.normalize(v).
// If that ever stops holding, every GPU consumer silently renders wrong colours.
#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include "marine_colormap/colormap.hpp"
#include "marine_colormap/lookup.hpp"
#include "marine_colormap/palette.hpp"
#include "marine_colormap/transfer.hpp"

using marine_colormap::Breakpoint;
using marine_colormap::BreakpointMap;
using marine_colormap::Palette;
using marine_colormap::Rgba8;
using marine_colormap::TransferParams;
using marine_colormap::bake_lut;
using marine_colormap::bake_shoreline_anchored_lut;
using marine_colormap::find_palette;
using marine_colormap::has_shoreline;
using marine_colormap::to_rgba8;

namespace
{

const Palette & pal(const char * name)
{
  const Palette * p = find_palette(name);
  EXPECT_NE(p, nullptr) << name;
  return *p;
}

// The LUT index the GPU shader lands on for `value` — its linear normalize.
std::size_t shader_index(float value, float lo, float hi, std::size_t n)
{
  const float t = std::clamp((value - lo) / (hi - lo), 0.0f, 1.0f);
  return static_cast<std::size_t>(std::lround(t * static_cast<float>(n - 1)));
}

}  // namespace

// --- GPU parity: the baked table agrees with the CPU map ---------------------

TEST(AnchoredBake, BakedTableAgreesWithTheBreakpointMapAtEveryProbe)
{
  // The composition property, stated directly. Probe across the range: the
  // shader's linear lookup into the anchored table must match a direct CPU
  // sample through the map.
  const Palette & p = pal("oleron");
  const float lo = -60.0f, hi = 10.0f;
  const BreakpointMap map(lo, hi, {Breakpoint{-28.038f, 0.5f}});
  const std::vector<Rgba8> lut = bake_lut(p, TransferParams{}, 256, map);
  ASSERT_EQ(lut.size(), 256u);

  const float entry_width = (hi - lo) / 255.0f;
  for (float v = lo; v <= hi; v += 1.0f) {
    // Skip probes within one LUT entry of the hard shoreline discontinuity: the
    // colour there is ambiguous by one entry BY DESIGN (coincident stops), so a
    // probe that straddles it compares the two sides of a deliberate jump. The
    // break's LOCATION is asserted by TwoAnchoredBreaksLandAtTheRightDataValues.
    if (std::fabs(v - (-28.038f)) <= 2.0f * entry_width) {
      continue;
    }
    const Rgba8 expected = to_rgba8(p.sample(map.normalize(v)));
    const Rgba8 got = lut[shader_index(v, lo, hi, 256)];
    // One quantization step: the index is a discrete entry.
    ASSERT_NEAR(got.r, expected.r, 2) << "at value " << v;
    ASSERT_NEAR(got.g, expected.g, 2) << "at value " << v;
    ASSERT_NEAR(got.b, expected.b, 2) << "at value " << v;
  }
}

TEST(AnchoredBake, TwoAnchoredBreaksLandAtTheRightDataValues)
{
  // The case the single-hinge convenience cannot express: shoreline AND a safety
  // contour in one palette. This is what makes the BreakpointMap overload the
  // real primitive rather than a shoreline special case.
  //
  // NOTE ON WHAT IS ASSERTED. oleron carries a HARD DISCONTINUITY at its
  // shoreline (two coincident stops at t = 0.5): sample() returns the lower
  // stop's colour exactly at 0.5 and the upper stop's just above it. So "the
  // colour at the break" is ambiguous to within one LUT entry, and comparing a
  // rounded entry against the exact-boundary sample is not a well-posed test.
  // What IS well-posed, and what actually matters to a viewer, is WHERE the
  // discontinuity lands in the data: assert the palette's biggest colour jump
  // sits at the anchored value.
  const Palette & p = pal("oleron");
  const float lo = -60.0f, hi = 10.0f;
  const float shoreline_break = -28.0f;
  const BreakpointMap map(lo, hi, {Breakpoint{shoreline_break, 0.5f}, Breakpoint{-5.0f, 0.8f}});
  const std::vector<Rgba8> lut = bake_lut(p, TransferParams{}, 256, map);
  ASSERT_EQ(lut.size(), 256u);

  // Locate the largest adjacent-entry colour jump — the rendered shoreline.
  std::size_t jump_at = 0;
  int biggest = -1;
  for (std::size_t i = 1; i < lut.size(); ++i) {
    const int d = std::abs(static_cast<int>(lut[i].r) - static_cast<int>(lut[i - 1].r)) +
      std::abs(static_cast<int>(lut[i].g) - static_cast<int>(lut[i - 1].g)) +
      std::abs(static_cast<int>(lut[i].b) - static_cast<int>(lut[i - 1].b));
    if (d > biggest) {
      biggest = d;
      jump_at = i;
    }
  }
  ASSERT_GT(biggest, 60) << "expected a visible hard shoreline transition";

  const std::size_t expected_at = shader_index(shoreline_break, lo, hi, 256);
  EXPECT_LE(
    jump_at > expected_at ? jump_at - expected_at : expected_at - jump_at, std::size_t{1})
    << "shoreline discontinuity landed at entry " << jump_at << ", expected near "
    << expected_at;

  // The second break carries no discontinuity, so assert it the well-posed way:
  // it is in a smooth region, and the map's position for it is honoured.
  const Rgba8 expected_second = to_rgba8(p.sample(map.normalize(-5.0f)));
  const Rgba8 got_second = lut[shader_index(-5.0f, lo, hi, 256)];
  EXPECT_NEAR(got_second.r, expected_second.r, 4);
  EXPECT_NEAR(got_second.g, expected_second.g, 4);
  EXPECT_NEAR(got_second.b, expected_second.b, 4);
}

TEST(AnchoredBake, EmptyBreakpointMapEqualsThePlainBake)
{
  // A map with no breaks IS linear normalization, so the anchored overload must
  // degenerate exactly onto bake_lut(). Byte-for-byte, since any drift here
  // would recolour existing displays.
  const Palette & p = pal("hypsometric");
  const std::vector<Rgba8> plain = bake_lut(p, TransferParams{}, 256);
  const std::vector<Rgba8> viaMap =
    bake_lut(p, TransferParams{}, 256, BreakpointMap(-50.0f, 20.0f, {}));
  ASSERT_EQ(plain.size(), viaMap.size());
  for (std::size_t i = 0; i < plain.size(); ++i) {
    ASSERT_EQ(plain[i].r, viaMap[i].r) << "entry " << i;
    ASSERT_EQ(plain[i].g, viaMap[i].g) << "entry " << i;
    ASSERT_EQ(plain[i].b, viaMap[i].b) << "entry " << i;
    ASSERT_EQ(plain[i].a, viaMap[i].a) << "entry " << i;
  }
}

// --- The shoreline convenience and its gate ----------------------------------

TEST(AnchoredBake, ShorelineAnchorLandsOnTheDeclaredPosition)
{
  const Palette & p = pal("oleron");
  ASSERT_TRUE(has_shoreline(p));
  const float shoreline = *p.domain()->shoreline_position;

  // Deliberately asymmetric range: a table that ignored the anchor would put the
  // transition at the range midpoint and fail this.
  const float lo = -60.0f, hi = 10.0f, anchor = -28.038f;
  const std::vector<Rgba8> lut =
    bake_shoreline_anchored_lut(p, TransferParams{}, lo, hi, anchor, 256);
  const Rgba8 expected = to_rgba8(p.sample(shoreline));
  const Rgba8 got = lut[shader_index(anchor, lo, hi, 256)];
  EXPECT_NEAR(got.r, expected.r, 2);
  EXPECT_NEAR(got.g, expected.g, 2);
  EXPECT_NEAR(got.b, expected.b, 2);
}

TEST(AnchoredBake, FallbackIsByteIdenticalToPlainBake)
{
  // Three fallback routes, all of which must leave existing colours untouched:
  // no anchor, a non-finite anchor, and a palette with no declared shoreline.
  for (const char * name : {"grayscale", "viridis", "turbo", "oleron", "hypsometric"}) {
    const Palette & p = pal(name);
    const std::vector<Rgba8> baseline = bake_lut(p, TransferParams{}, 256);
    const std::vector<std::optional<float>> anchors = {
      std::nullopt,
      std::optional<float>(std::numeric_limits<float>::quiet_NaN()),
      std::optional<float>(std::numeric_limits<float>::infinity()),
    };
    for (const std::optional<float> & a : anchors) {
      const std::vector<Rgba8> got =
        bake_shoreline_anchored_lut(p, TransferParams{}, -50.0f, 20.0f, a, 256);
      ASSERT_EQ(got.size(), baseline.size()) << name;
      for (std::size_t i = 0; i < baseline.size(); ++i) {
        ASSERT_EQ(got[i].r, baseline[i].r) << name << " entry " << i;
        ASSERT_EQ(got[i].a, baseline[i].a) << name << " entry " << i;
      }
    }
  }
}

TEST(AnchoredBake, PaletteWithoutShorelineIsNeverWarped)
{
  // The gate: anchoring a general-purpose ramp around a shoreline it does not
  // declare would produce colours that mean nothing.
  for (const char * name : {"grayscale", "viridis", "turbo"}) {
    const Palette & p = pal(name);
    EXPECT_FALSE(has_shoreline(p)) << name;
    const std::vector<Rgba8> baseline = bake_lut(p, TransferParams{}, 256);
    const std::vector<Rgba8> got =
      bake_shoreline_anchored_lut(p, TransferParams{}, -50.0f, 20.0f, -28.038f, 256);
    for (std::size_t i = 0; i < baseline.size(); ++i) {
      ASSERT_EQ(got[i].r, baseline[i].r) << name << " entry " << i;
    }
  }
  EXPECT_TRUE(has_shoreline(pal("oleron")));
  EXPECT_TRUE(has_shoreline(pal("hypsometric")));
}

TEST(AnchoredBake, AnchorValueActuallyChangesTheTable)
{
  // Guards the coincidence case: a table that is anchored-shaped but ignores the
  // value would pass a single-anchor test whose range midpoint sits near it.
  const Palette & p = pal("oleron");
  const std::vector<Rgba8> a =
    bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, 10.0f, -40.0f, 256);
  const std::vector<Rgba8> b =
    bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, 10.0f, -10.0f, 256);
  bool differs = false;
  for (std::size_t i = 0; i < a.size() && !differs; ++i) {
    differs = (a[i].r != b[i].r) || (a[i].g != b[i].g) || (a[i].b != b[i].b);
  }
  EXPECT_TRUE(differs);
}

TEST(AnchoredBake, RangeDependence)
{
  // The documented consequence consumers must handle: unlike bake_lut(), this
  // table is a function of the range. If a consumer caches it by palette name
  // alone, a range change serves a stale table and renders wrong colours with no
  // crash and no log line.
  const Palette & p = pal("oleron");
  const std::vector<Rgba8> narrow =
    bake_shoreline_anchored_lut(p, TransferParams{}, -40.0f, 0.0f, -28.038f, 256);
  const std::vector<Rgba8> wide =
    bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, 10.0f, -28.038f, 256);
  bool differs = false;
  for (std::size_t i = 0; i < narrow.size() && !differs; ++i) {
    differs = (narrow[i].r != wide[i].r) || (narrow[i].b != wide[i].b);
  }
  EXPECT_TRUE(differs) << "anchored LUT must depend on the range";
}

// --- Degenerate inputs clamp; they never throw --------------------------------

TEST(AnchoredBake, DegenerateInputsAreSafe)
{
  const Palette & p = pal("oleron");
  std::vector<Rgba8> lut;
  // Anchor outside the range is the ORDINARY case (no land in view), not an error.
  ASSERT_NO_THROW(lut = bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, -10.0f, 1000.0f,
    256));
  EXPECT_EQ(lut.size(), 256u);
  ASSERT_NO_THROW(lut = bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, -10.0f, -1000.0f,
    256));
  EXPECT_EQ(lut.size(), 256u);
  // Boundary anchors.
  ASSERT_NO_THROW(lut = bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, 10.0f, -60.0f,
    256));
  ASSERT_NO_THROW(lut = bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, 10.0f, 10.0f,
    256));
  // Zero-width and inverted ranges.
  ASSERT_NO_THROW(lut = bake_shoreline_anchored_lut(p, TransferParams{}, 5.0f, 5.0f, 5.0f, 256));
  EXPECT_EQ(lut.size(), 256u);
  ASSERT_NO_THROW(lut = bake_shoreline_anchored_lut(p, TransferParams{}, 10.0f, -60.0f, -28.0f,
    256));
  EXPECT_EQ(lut.size(), 256u);
  // n < 1 clamps to 1, matching bake_lut().
  EXPECT_EQ(bake_shoreline_anchored_lut(p, TransferParams{}, -60.0f, 10.0f, -28.0f, 0).size(), 1u);
  EXPECT_EQ(bake_lut(p, TransferParams{}, 0, BreakpointMap(-60.0f, 10.0f, {})).size(), 1u);
}
