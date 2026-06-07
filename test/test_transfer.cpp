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
