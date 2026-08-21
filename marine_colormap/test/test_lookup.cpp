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
#include <limits>
#include <vector>

#include "marine_colormap/lookup.hpp"

using marine_colormap::Breakpoint;
using marine_colormap::BreakpointMap;
using marine_colormap::Closure;
using marine_colormap::LookupEntry;
using marine_colormap::LookupTable;
using marine_colormap::Rgba;
using marine_colormap::Sentinels;
using marine_colormap::ValueRange;

namespace
{

constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
constexpr float kInf = std::numeric_limits<float>::infinity();

Rgba red() {return Rgba{1.0f, 0.0f, 0.0f, 1.0f};}
Rgba green() {return Rgba{0.0f, 1.0f, 0.0f, 1.0f};}
Rgba blue() {return Rgba{0.0f, 0.0f, 1.0f, 1.0f};}

void expect_color_eq(const Rgba & a, const Rgba & b)
{
  EXPECT_FLOAT_EQ(a.r, b.r);
  EXPECT_FLOAT_EQ(a.g, b.g);
  EXPECT_FLOAT_EQ(a.b, b.b);
  EXPECT_FLOAT_EQ(a.a, b.a);
}

/// The S-102 two-shade ladder, which is the shape this model exists to carry:
/// intertidal below zero, shallow up to the safety contour, deep beyond it.
LookupTable s102_two_shade(float safety_contour)
{
  return LookupTable(
    {
      LookupEntry{"Intertidal", ValueRange{0.0f, 0.0f, Closure::LtSemiInterval}, red(), {}},
      LookupEntry{
        "Shallow Water", ValueRange{0.0f, safety_contour, Closure::GeLtInterval}, green(), {}},
      LookupEntry{
        "Deep Water", ValueRange{safety_contour, 0.0f, Closure::GeSemiInterval}, blue(), {}},
    });
}

}  // namespace

// --- ValueRange closures ----------------------------------------------------

TEST(ValueRange, ClosedIntervalIncludesBothEnds)
{
  const ValueRange r{1.0f, 2.0f, Closure::ClosedInterval};
  EXPECT_TRUE(r.contains(1.0f));
  EXPECT_TRUE(r.contains(1.5f));
  EXPECT_TRUE(r.contains(2.0f));
  EXPECT_FALSE(r.contains(0.999f));
  EXPECT_FALSE(r.contains(2.001f));
}

TEST(ValueRange, OpenIntervalExcludesBothEnds)
{
  const ValueRange r{1.0f, 2.0f, Closure::OpenInterval};
  EXPECT_FALSE(r.contains(1.0f));
  EXPECT_TRUE(r.contains(1.5f));
  EXPECT_FALSE(r.contains(2.0f));
}

TEST(ValueRange, GeLtIntervalIncludesLowerOnly)
{
  const ValueRange r{1.0f, 2.0f, Closure::GeLtInterval};
  EXPECT_TRUE(r.contains(1.0f));
  EXPECT_FALSE(r.contains(2.0f));
}

TEST(ValueRange, GtLeIntervalIncludesUpperOnly)
{
  const ValueRange r{1.0f, 2.0f, Closure::GtLeInterval};
  EXPECT_FALSE(r.contains(1.0f));
  EXPECT_TRUE(r.contains(2.0f));
}

TEST(ValueRange, SemiIntervalsIgnoreTheUnusedBound)
{
  // `lower` is deliberately nonsense here: a semi-interval must not read it.
  const ValueRange lt{999.0f, 5.0f, Closure::LtSemiInterval};
  EXPECT_TRUE(lt.contains(-1e6f));
  EXPECT_TRUE(lt.contains(4.999f));
  EXPECT_FALSE(lt.contains(5.0f));

  const ValueRange le{999.0f, 5.0f, Closure::LeSemiInterval};
  EXPECT_TRUE(le.contains(5.0f));

  const ValueRange ge{5.0f, -999.0f, Closure::GeSemiInterval};
  EXPECT_TRUE(ge.contains(5.0f));
  EXPECT_TRUE(ge.contains(1e6f));
  EXPECT_FALSE(ge.contains(4.999f));

  const ValueRange gt{5.0f, -999.0f, Closure::GtSemiInterval};
  EXPECT_FALSE(gt.contains(5.0f));
}

TEST(ValueRange, SingleValueIsAClosedIntervalOfZeroWidth)
{
  const ValueRange r{-1.0f, -1.0f, Closure::ClosedInterval};
  EXPECT_TRUE(r.contains(-1.0f));
  EXPECT_FALSE(r.contains(-1.001f));
  EXPECT_FALSE(r.contains(-0.999f));
}

TEST(ValueRange, NonFiniteIsNeverContained)
{
  for (const auto c :
    {Closure::ClosedInterval, Closure::OpenInterval, Closure::GeLtInterval,
      Closure::GtLeInterval, Closure::LtSemiInterval, Closure::LeSemiInterval,
      Closure::GeSemiInterval, Closure::GtSemiInterval})
  {
    const ValueRange r{-10.0f, 10.0f, c};
    EXPECT_FALSE(r.contains(kNaN)) << "closure " << static_cast<int>(c);
    EXPECT_FALSE(r.contains(kInf)) << "closure " << static_cast<int>(c);
    EXPECT_FALSE(r.contains(-kInf)) << "closure " << static_cast<int>(c);
  }
}

TEST(ValueRange, AdjacentGeLtBandsPartitionTheBoundaryExactlyOnce)
{
  // The reason explicit closures exist: a sounding sitting exactly on the
  // safety contour must belong to exactly one band, by construction.
  const ValueRange shallow{0.0f, 30.0f, Closure::GeLtInterval};
  const ValueRange deep{30.0f, 100.0f, Closure::GeLtInterval};
  EXPECT_FALSE(shallow.contains(30.0f));
  EXPECT_TRUE(deep.contains(30.0f));
}

TEST(ValueRange, RampableRequiresTwoFiniteEndsAndPositiveWidth)
{
  EXPECT_TRUE((ValueRange{0.0f, 1.0f, Closure::GeLtInterval}).rampable());
  EXPECT_FALSE((ValueRange{1.0f, 1.0f, Closure::ClosedInterval}).rampable());
  EXPECT_FALSE((ValueRange{2.0f, 1.0f, Closure::ClosedInterval}).rampable());
  EXPECT_FALSE((ValueRange{0.0f, 1.0f, Closure::GeSemiInterval}).rampable());
  EXPECT_FALSE((ValueRange{0.0f, kInf, Closure::ClosedInterval}).rampable());
}

TEST(ValueRange, FractionClampsAndHandlesDegenerate)
{
  const ValueRange r{10.0f, 20.0f, Closure::ClosedInterval};
  EXPECT_FLOAT_EQ(0.0f, r.fraction(10.0f));
  EXPECT_FLOAT_EQ(0.5f, r.fraction(15.0f));
  EXPECT_FLOAT_EQ(1.0f, r.fraction(20.0f));
  EXPECT_FLOAT_EQ(0.0f, r.fraction(-100.0f));
  EXPECT_FLOAT_EQ(1.0f, r.fraction(100.0f));

  const ValueRange degenerate{5.0f, 5.0f, Closure::ClosedInterval};
  EXPECT_FLOAT_EQ(0.0f, degenerate.fraction(5.0f));
}

// --- LookupEntry ------------------------------------------------------------

TEST(LookupEntry, FlatEntryReturnsStartColorEverywhere)
{
  const LookupEntry e{"flat", ValueRange{0.0f, 10.0f, Closure::GeLtInterval}, red(), {}};
  expect_color_eq(red(), e.color_at(0.0f));
  expect_color_eq(red(), e.color_at(9.9f));
}

TEST(LookupEntry, RampInterpolatesAcrossItsOwnRange)
{
  const LookupEntry e{"ramp", ValueRange{0.0f, 10.0f, Closure::GeLtInterval}, red(), green()};
  expect_color_eq(red(), e.color_at(0.0f));
  const Rgba mid = e.color_at(5.0f);
  EXPECT_FLOAT_EQ(0.5f, mid.r);
  EXPECT_FLOAT_EQ(0.5f, mid.g);
}

TEST(LookupEntry, RampOnAnUnrampableRangeFallsBackToFlat)
{
  // An unbounded deep-water band with a ramp configured must still render.
  const LookupEntry e{"deep", ValueRange{30.0f, 0.0f, Closure::GeSemiInterval}, red(), green()};
  expect_color_eq(red(), e.color_at(1000.0f));
}

// --- LookupTable ------------------------------------------------------------

TEST(LookupTable, FirstMatchWinsOnOverlap)
{
  const LookupTable t({
    LookupEntry{"first", ValueRange{0.0f, 100.0f, Closure::GeLtInterval}, red(), {}},
    LookupEntry{"second", ValueRange{0.0f, 100.0f, Closure::GeLtInterval}, green(), {}},
  });
  expect_color_eq(red(), t.lookup(50.0f));
  ASSERT_TRUE(t.match(50.0f).has_value());
  EXPECT_EQ(0u, *t.match(50.0f));
}

TEST(LookupTable, S102TwoShadeLadderColorsEachBand)
{
  const LookupTable t = s102_two_shade(30.0f);
  expect_color_eq(red(), t.lookup(-2.0f));     // intertidal
  expect_color_eq(green(), t.lookup(0.0f));    // shallow, lower bound included
  expect_color_eq(green(), t.lookup(29.99f));
  expect_color_eq(blue(), t.lookup(30.0f));    // safety contour -> deep
  expect_color_eq(blue(), t.lookup(5000.0f));
}

TEST(LookupTable, MovingTheSafetyContourMovesTheBoundary)
{
  // The operator-settable break: same table shape, different contour.
  expect_color_eq(green(), s102_two_shade(30.0f).lookup(20.0f));
  expect_color_eq(blue(), s102_two_shade(10.0f).lookup(20.0f));
}

TEST(LookupTable, NonFiniteIsBadNotUnmapped)
{
  Sentinels s;
  s.bad = red();
  s.unmapped = green();
  const LookupTable t({
    LookupEntry{"band", ValueRange{0.0f, 10.0f, Closure::GeLtInterval}, blue(), {}},
  }, s);
  expect_color_eq(red(), t.lookup(kNaN));
  expect_color_eq(red(), t.lookup(kInf));
}

TEST(LookupTable, GapInsideTheDomainIsUnmappedNotBad)
{
  // The distinction VTK loses: a value with no matching entry is a different
  // thing from invalid data.
  Sentinels s;
  s.bad = red();
  s.unmapped = green();
  const LookupTable t({
    LookupEntry{"low", ValueRange{0.0f, 10.0f, Closure::GeLtInterval}, blue(), {}},
    LookupEntry{"high", ValueRange{20.0f, 30.0f, Closure::GeLtInterval}, blue(), {}},
  }, s);
  expect_color_eq(green(), t.lookup(15.0f));
}

TEST(LookupTable, OutsideTheDomainIsUnderOrOverNotUnmapped)
{
  Sentinels s;
  s.under = red();
  s.over = green();
  s.unmapped = blue();
  const LookupTable t({
    LookupEntry{"band", ValueRange{0.0f, 10.0f, Closure::GeLtInterval}, blue(), {}},
  }, s);
  expect_color_eq(red(), t.lookup(-1.0f));
  expect_color_eq(green(), t.lookup(11.0f));
}

TEST(LookupTable, UnsetUnderOverResolveToTheEndEntryColors)
{
  const LookupTable t({
    LookupEntry{"low", ValueRange{0.0f, 10.0f, Closure::GeLtInterval}, red(), {}},
    LookupEntry{"high", ValueRange{10.0f, 20.0f, Closure::GeLtInterval}, green(), blue()},
  });
  expect_color_eq(red(), t.lookup(-1.0f));   // first entry's start color
  expect_color_eq(blue(), t.lookup(21.0f));  // last entry's end color
}

TEST(LookupTable, AnUnboundedEntryMeansNothingIsEverOutside)
{
  Sentinels s;
  s.under = red();
  s.over = green();
  const LookupTable t = s102_two_shade(30.0f);
  EXPECT_FALSE(t.domain_min().has_value());
  EXPECT_FALSE(t.domain_max().has_value());
}

TEST(LookupTable, EmptyTableIsUnmappedNotACrash)
{
  Sentinels s;
  s.unmapped = green();
  const LookupTable t({}, s);
  EXPECT_TRUE(t.empty());
  expect_color_eq(green(), t.lookup(0.0f));
  EXPECT_FALSE(t.match(0.0f).has_value());
}

TEST(LookupTable, CategoricalSentinelValueIsANamedEntryNotBad)
{
  // Occupancy -1 is a legal, meaningful value; it must be a band, not NaN.
  Sentinels s;
  s.bad = red();
  const LookupTable t({
    LookupEntry{"Unknown", ValueRange{-1.0f, -1.0f, Closure::ClosedInterval}, green(), {}},
    LookupEntry{"Free", ValueRange{0.0f, 100.0f, Closure::ClosedInterval}, blue(), {}},
  }, s);
  expect_color_eq(green(), t.lookup(-1.0f));
  expect_color_eq(blue(), t.lookup(0.0f));
}

// --- BreakpointMap ----------------------------------------------------------

TEST(BreakpointMap, NoBreaksIsPlainLinearNormalization)
{
  const BreakpointMap m(0.0f, 10.0f, {});
  EXPECT_FLOAT_EQ(0.0f, m.normalize(0.0f));
  EXPECT_FLOAT_EQ(0.5f, m.normalize(5.0f));
  EXPECT_FLOAT_EQ(1.0f, m.normalize(10.0f));
}

TEST(BreakpointMap, OneBreakStretchesEachSideIndependently)
{
  // The asymmetric case from the vision: 80 m of water and 5 m of land, each
  // filling half the palette. This is GMT hinge behaviour.
  const BreakpointMap m(-80.0f, 5.0f, {Breakpoint{0.0f, 0.5f}});
  EXPECT_FLOAT_EQ(0.0f, m.normalize(-80.0f));
  EXPECT_FLOAT_EQ(0.25f, m.normalize(-40.0f));
  EXPECT_FLOAT_EQ(0.5f, m.normalize(0.0f));
  EXPECT_FLOAT_EQ(0.75f, m.normalize(2.5f));
  EXPECT_FLOAT_EQ(1.0f, m.normalize(5.0f));
}

TEST(BreakpointMap, TwoBreaksCarryShorelineAndSafetyContour)
{
  // Datum at 0 and a safety contour at -5, in a domain from -20 to +10.
  const BreakpointMap m(
    -20.0f, 10.0f, {Breakpoint{-5.0f, 0.3f}, Breakpoint{0.0f, 0.6f}});
  EXPECT_FLOAT_EQ(0.0f, m.normalize(-20.0f));
  EXPECT_FLOAT_EQ(0.3f, m.normalize(-5.0f));
  EXPECT_FLOAT_EQ(0.6f, m.normalize(0.0f));
  EXPECT_FLOAT_EQ(1.0f, m.normalize(10.0f));
  // Midpoint of the middle span maps to the midpoint of its colour share.
  EXPECT_FLOAT_EQ(0.45f, m.normalize(-2.5f));
}

TEST(BreakpointMap, BreaksAreSortedRegardlessOfInputOrder)
{
  const BreakpointMap m(
    -20.0f, 10.0f, {Breakpoint{0.0f, 0.6f}, Breakpoint{-5.0f, 0.3f}});
  ASSERT_EQ(2u, m.breaks().size());
  EXPECT_FLOAT_EQ(-5.0f, m.breaks()[0].value);
  EXPECT_FLOAT_EQ(0.0f, m.breaks()[1].value);
}

TEST(BreakpointMap, BreakAboveTheDomainClampsAndStillResolves)
{
  // "No land in view": the datum break sits above every sounding. The map must
  // stay monotonic and every value must still resolve — not throw, not invert.
  const BreakpointMap m(-80.0f, -10.0f, {Breakpoint{0.0f, 0.5f}});
  ASSERT_EQ(1u, m.breaks().size());
  EXPECT_FLOAT_EQ(-10.0f, m.breaks()[0].value);  // clamped onto hi
  EXPECT_FLOAT_EQ(0.0f, m.normalize(-80.0f));
  EXPECT_FLOAT_EQ(0.25f, m.normalize(-45.0f));
  EXPECT_FLOAT_EQ(0.5f, m.normalize(-10.0f));
  // Monotonic across the whole domain.
  float previous = -1.0f;
  for (float v = -80.0f; v <= -10.0f; v += 1.0f) {
    const float t = m.normalize(v);
    EXPECT_GE(t, previous);
    previous = t;
  }
}

TEST(BreakpointMap, BreakBelowTheDomainClampsToLo)
{
  const BreakpointMap m(10.0f, 20.0f, {Breakpoint{0.0f, 0.5f}});
  ASSERT_EQ(1u, m.breaks().size());
  EXPECT_FLOAT_EQ(10.0f, m.breaks()[0].value);
  EXPECT_FLOAT_EQ(0.5f, m.normalize(10.0f));
  EXPECT_FLOAT_EQ(1.0f, m.normalize(20.0f));
}

TEST(BreakpointMap, SeveralBreaksOutsideTheSameEndCollapseWithoutCrossing)
{
  const BreakpointMap m(
    -80.0f, -10.0f, {Breakpoint{0.0f, 0.5f}, Breakpoint{5.0f, 0.8f}});
  ASSERT_EQ(2u, m.breaks().size());
  EXPECT_FLOAT_EQ(-10.0f, m.breaks()[0].value);
  EXPECT_FLOAT_EQ(-10.0f, m.breaks()[1].value);
  EXPECT_LE(m.breaks()[0].position, m.breaks()[1].position);
  EXPECT_FLOAT_EQ(0.0f, m.normalize(-80.0f));
}

TEST(BreakpointMap, CrossingPositionsAreClampedMonotonic)
{
  // A caller supplying positions that run backwards gets a monotonic map, not
  // an inverted span or an exception.
  const BreakpointMap m(
    0.0f, 10.0f, {Breakpoint{2.0f, 0.8f}, Breakpoint{6.0f, 0.2f}});
  ASSERT_EQ(2u, m.breaks().size());
  EXPECT_GE(m.breaks()[1].position, m.breaks()[0].position);
  float previous = -1.0f;
  for (float v = 0.0f; v <= 10.0f; v += 0.5f) {
    const float t = m.normalize(v);
    EXPECT_GE(t, previous);
    previous = t;
  }
}

TEST(BreakpointMap, InvertedDomainIsSwapped)
{
  const BreakpointMap m(10.0f, 0.0f, {});
  EXPECT_FLOAT_EQ(0.0f, m.lo());
  EXPECT_FLOAT_EQ(10.0f, m.hi());
  EXPECT_FLOAT_EQ(0.5f, m.normalize(5.0f));
}

TEST(BreakpointMap, DegenerateDomainReturnsZero)
{
  const BreakpointMap m(5.0f, 5.0f, {Breakpoint{5.0f, 0.5f}});
  EXPECT_FLOAT_EQ(0.0f, m.normalize(5.0f));
  EXPECT_FLOAT_EQ(0.0f, m.normalize(0.0f));
}

TEST(BreakpointMap, BreakOnADomainEndpointCollapsesItsSpan)
{
  const BreakpointMap m(0.0f, 10.0f, {Breakpoint{0.0f, 0.4f}});
  EXPECT_FLOAT_EQ(0.4f, m.normalize(0.0f));
  EXPECT_FLOAT_EQ(0.7f, m.normalize(5.0f));
  EXPECT_FLOAT_EQ(1.0f, m.normalize(10.0f));
}

TEST(BreakpointMap, OutOfDomainValuesClampRatherThanExtrapolate)
{
  const BreakpointMap m(-80.0f, 5.0f, {Breakpoint{0.0f, 0.5f}});
  EXPECT_FLOAT_EQ(0.0f, m.normalize(-1000.0f));
  EXPECT_FLOAT_EQ(1.0f, m.normalize(1000.0f));
}

TEST(BreakpointMap, NonFiniteInputReturnsZero)
{
  const BreakpointMap m(-80.0f, 5.0f, {Breakpoint{0.0f, 0.5f}});
  EXPECT_FLOAT_EQ(0.0f, m.normalize(kNaN));
  EXPECT_FLOAT_EQ(0.0f, m.normalize(-kInf));
}

TEST(BreakpointMap, NonFiniteBreakValueIsClampedIntoTheDomain)
{
  const BreakpointMap m(-10.0f, 10.0f, {Breakpoint{kNaN, 0.5f}});
  ASSERT_EQ(1u, m.breaks().size());
  EXPECT_TRUE(std::isfinite(m.breaks()[0].value));
  EXPECT_GE(m.breaks()[0].value, m.lo());
  EXPECT_LE(m.breaks()[0].value, m.hi());
}

TEST(BreakpointMap, OutputIsAlwaysWithinUnitInterval)
{
  const BreakpointMap m(
    -80.0f, 5.0f, {Breakpoint{-40.0f, 0.2f}, Breakpoint{0.0f, 0.5f}});
  for (float v = -200.0f; v <= 200.0f; v += 0.7f) {
    const float t = m.normalize(v);
    EXPECT_GE(t, 0.0f);
    EXPECT_LE(t, 1.0f);
  }
}
