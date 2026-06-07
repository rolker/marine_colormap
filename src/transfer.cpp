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

#include "marine_colormap/transfer.hpp"

#include <algorithm>
#include <cmath>

namespace marine_colormap
{

float normalize(float value, float lo, float hi)
{
  if (!(hi > lo)) {
    return 0.0f;  // degenerate range
  }
  return (value - lo) / (hi - lo);
}

float apply_response(float t, float gain, float contrast)
{
  t = std::clamp(t, 0.0f, 1.0f);
  t = std::clamp(t * gain, 0.0f, 1.0f);
  if (contrast > 0.0f && contrast != 1.0f) {
    t = std::pow(t, 1.0f / contrast);
  }
  return std::clamp(t, 0.0f, 1.0f);
}

}  // namespace marine_colormap
