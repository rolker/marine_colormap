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

#include "marine_colormap/shader.hpp"

namespace marine_colormap
{

// Mirrors transfer.cpp's normalize()/apply_response() exactly. If you change the
// CPU transfer math, change this in lockstep -- the whole point is CPU/GPU
// agreement. Kept version-agnostic (no #version / precision / samplers).
const char * colormap_glsl()
{
  return
    R"GLSL(
// marine_colormap shared math (mirrors the C++ normalize()/apply_response()).

// Map a value onto [0, 1] across [lo, hi]; raw (may be <0 or >1, caller's
// response() clamps). Degenerate range (hi <= lo) returns 0. Mirrors normalize().
float marine_colormap_normalize(float value, float lo, float hi)
{
  if (!(hi > lo)) {
    return 0.0;
  }
  return (value - lo) / (hi - lo);
}

// Apply gain then contrast/gamma to a normalized position. The clamp of
// t * gain happens BEFORE gamma (gain acts as a clip); gain < 0 clamps to 0;
// contrast <= 0 is passthrough. Mirrors apply_response().
float marine_colormap_response(float t, float gain, float contrast)
{
  t = clamp(t, 0.0, 1.0);
  t = clamp(t * gain, 0.0, 1.0);
  if (contrast > 0.0 && contrast != 1.0) {
    t = pow(t, 1.0 / contrast);
  }
  return clamp(t, 0.0, 1.0);
}
)GLSL";
}

}  // namespace marine_colormap
