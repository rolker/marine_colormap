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

#include "marine_colormap/transfer.hpp"

using marine_colormap::apply_response;
using marine_colormap::normalize;
using marine_colormap::RangeMode;
using marine_colormap::RangeModel;

TEST(Transfer, NormalizeMapsRange)
{
  EXPECT_FLOAT_EQ(normalize(0.0f, 0.0f, 1.0f), 0.0f);
  EXPECT_FLOAT_EQ(normalize(1.0f, 0.0f, 1.0f), 1.0f);
  EXPECT_FLOAT_EQ(normalize(0.5f, 0.0f, 1.0f), 0.5f);
  EXPECT_FLOAT_EQ(normalize(-70.0f, -70.0f, 0.0f), 0.0f);   // dB-style range
  EXPECT_FLOAT_EQ(normalize(-35.0f, -70.0f, 0.0f), 0.5f);
}

TEST(Transfer, NormalizeReturnsRawBelowAndAbove)
{
  // Raw (unclamped) so callers can detect out-of-range.
  EXPECT_LT(normalize(-1.0f, 0.0f, 1.0f), 0.0f);
  EXPECT_GT(normalize(2.0f, 0.0f, 1.0f), 1.0f);
}

TEST(Transfer, NormalizeDegenerateRangeIsZero)
{
  EXPECT_FLOAT_EQ(normalize(5.0f, 5.0f, 5.0f), 0.0f);
  EXPECT_FLOAT_EQ(normalize(5.0f, 10.0f, 5.0f), 0.0f);  // hi <= lo
}

TEST(Transfer, ResponseIdentityAndClamp)
{
  EXPECT_FLOAT_EQ(apply_response(0.5f, 1.0f, 1.0f), 0.5f);
  EXPECT_FLOAT_EQ(apply_response(-1.0f, 1.0f, 1.0f), 0.0f);
  EXPECT_FLOAT_EQ(apply_response(2.0f, 1.0f, 1.0f), 1.0f);
}

TEST(Transfer, ResponseGainScalesAndClamps)
{
  EXPECT_FLOAT_EQ(apply_response(0.25f, 2.0f, 1.0f), 0.5f);
  EXPECT_FLOAT_EQ(apply_response(0.75f, 2.0f, 1.0f), 1.0f);  // 1.5 clamps
}

TEST(Transfer, ResponseContrastGamma)
{
  // contrast != 1 applies pow(t, 1/contrast).
  EXPECT_NEAR(apply_response(0.5f, 1.0f, 2.0f), std::pow(0.5f, 0.5f), 1e-5f);
  EXPECT_FLOAT_EQ(apply_response(0.0f, 1.0f, 2.0f), 0.0f);
  EXPECT_FLOAT_EQ(apply_response(1.0f, 1.0f, 2.0f), 1.0f);
}

TEST(Transfer, ResponseGainThenGammaClampOrder)
{
  // Combined gain != 1 AND contrast != 1: the documented contract is
  // gamma(clamp(t * gain)). With t*gain still in range, gamma sees the gained
  // value: 0.25 * 2 = 0.5, pow(0.5, 1/2) = 0.7071.
  EXPECT_NEAR(apply_response(0.25f, 2.0f, 2.0f), std::pow(0.5f, 0.5f), 1e-5f);
  // The clamp of t*gain happens BEFORE gamma: 0.75 * 2 = 1.5 -> clamp 1.0 ->
  // pow(1.0, 1/2) = 1.0. (A shader that gamma'd the un-clamped 1.5 would differ.)
  EXPECT_FLOAT_EQ(apply_response(0.75f, 2.0f, 2.0f), 1.0f);
  // Negative gain clamps to 0; non-positive contrast is passthrough (no gamma).
  EXPECT_FLOAT_EQ(apply_response(0.5f, -1.0f, 2.0f), 0.0f);
  EXPECT_FLOAT_EQ(apply_response(0.5f, 1.0f, 0.0f), 0.5f);
}

TEST(RangeModel, DefaultsToAutoUnitRange)
{
  RangeModel rm;
  EXPECT_EQ(rm.mode(), RangeMode::Auto);
  EXPECT_FLOAT_EQ(rm.lo(), 0.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 1.0f);
}

TEST(RangeModel, AutoTracksDataExtents)
{
  RangeModel rm;
  rm.update_auto(-70.0f, 0.0f);  // dB-style frame
  EXPECT_EQ(rm.mode(), RangeMode::Auto);
  EXPECT_FLOAT_EQ(rm.lo(), -70.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 0.0f);
  // A later frame replaces the extent (range follows the data).
  rm.update_auto(-50.0f, 10.0f);
  EXPECT_FLOAT_EQ(rm.lo(), -50.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 10.0f);
}

TEST(RangeModel, SetManualSwitchesModeAndPins)
{
  RangeModel rm;
  rm.set_manual(0.0f, 1.0f);
  EXPECT_EQ(rm.mode(), RangeMode::Manual);
  EXPECT_FLOAT_EQ(rm.lo(), 0.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 1.0f);
}

TEST(RangeModel, ManualIgnoresDataExceedingRange)
{
  // The #7 case: an outlier band (max 925) must not re-widen an operator's
  // pinned [0, 1]. update_auto() is a no-op while Manual.
  RangeModel rm;
  rm.set_manual(0.0f, 1.0f);
  rm.update_auto(0.16f, 925.0f);
  EXPECT_EQ(rm.mode(), RangeMode::Manual);
  EXPECT_FLOAT_EQ(rm.lo(), 0.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 1.0f);
  // A value past the pinned range still normalizes raw > 1 (caller clamps).
  EXPECT_GT(rm.normalize(925.0f), 1.0f);
}

TEST(RangeModel, NormalizeMatchesFreeFunction)
{
  RangeModel rm;
  rm.set_manual(-70.0f, 0.0f);
  for (float v : {-70.0f, -35.0f, 0.0f, 10.0f}) {
    EXPECT_FLOAT_EQ(rm.normalize(v), normalize(v, rm.lo(), rm.hi()));
  }
}

TEST(RangeModel, DegenerateRangeNormalizesToZero)
{
  RangeModel rm;
  rm.set_manual(5.0f, 5.0f);  // zero-width
  EXPECT_FLOAT_EQ(rm.normalize(5.0f), 0.0f);
  EXPECT_FLOAT_EQ(rm.normalize(99.0f), 0.0f);
}

TEST(RangeModel, ResetReturnsToAutoAndResumesTracking)
{
  RangeModel rm;
  rm.set_manual(0.0f, 1.0f);
  ASSERT_EQ(rm.mode(), RangeMode::Manual);
  rm.reset();
  EXPECT_EQ(rm.mode(), RangeMode::Auto);
  // Tracking resumes: a no-op while Manual now takes effect.
  rm.update_auto(-20.0f, 5.0f);
  EXPECT_FLOAT_EQ(rm.lo(), -20.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 5.0f);
}

TEST(RangeModel, ResetPreservesExtentUntilNextUpdate)
{
  // reset() returns to Auto but must NOT zero the range: lo()/hi() keep the
  // prior extent until the next update_auto() refreshes it from data.
  RangeModel rm;
  rm.set_manual(-70.0f, 0.0f);
  rm.reset();
  EXPECT_EQ(rm.mode(), RangeMode::Auto);
  EXPECT_FLOAT_EQ(rm.lo(), -70.0f);  // extent preserved, not reset to defaults
  EXPECT_FLOAT_EQ(rm.hi(), 0.0f);
  // Normalization still uses the preserved extent.
  EXPECT_FLOAT_EQ(rm.normalize(-35.0f), 0.5f);
}

TEST(RangeModel, SetManualSwapsInvertedRange)
{
  // An inverted range (hi, lo) must be swapped, not silently collapse every
  // sample to 0 via the degenerate-range guard. set_manual(1, 0) == [0, 1].
  RangeModel rm;
  rm.set_manual(1.0f, 0.0f);  // inverted
  EXPECT_FLOAT_EQ(rm.lo(), 0.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 1.0f);
  EXPECT_FLOAT_EQ(rm.normalize(0.25f), 0.25f);  // correct, not all-zero
  // dB-style inverted drag normalizes the same as the forward range.
  rm.set_manual(0.0f, -70.0f);  // inverted
  EXPECT_FLOAT_EQ(rm.lo(), -70.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 0.0f);
  EXPECT_FLOAT_EQ(rm.normalize(-35.0f), 0.5f);
}

TEST(RangeModel, AutoSwapsInvertedDataExtents)
{
  // update_auto() likewise normalizes an inverted [min, max].
  RangeModel rm;
  rm.update_auto(10.0f, -50.0f);  // inverted
  EXPECT_FLOAT_EQ(rm.lo(), -50.0f);
  EXPECT_FLOAT_EQ(rm.hi(), 10.0f);
}
