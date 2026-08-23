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

#include "marine_colormap/colormap.hpp"

#include <algorithm>
#include <cmath>

namespace marine_colormap
{

namespace
{

// Value-dependent alpha from the (clamped) normalized position. The result is
// clamped to [0, 1] so an out-of-range or non-finite alpha_min/alpha_max can't
// push Rgba.a outside its documented [0, 1] contract on the float path (the
// 8-bit LUT path is clamped again by to_rgba8).
float alpha_for(float normalized, const TransferParams & p)
{
  const float c = std::clamp(normalized, 0.0f, 1.0f);
  return std::clamp(p.alpha_min + (p.alpha_max - p.alpha_min) * c, 0.0f, 1.0f);
}

}  // namespace

Rgba lookup(float value, const Palette & pal, const TransferParams & p)
{
  if (!std::isfinite(value)) {
    return p.nodata_color;
  }
  if (p.has_below_color && value < p.min) {
    return p.below_color;
  }
  const float normalized = normalize(value, p.min, p.max);
  const float t = apply_response(normalized, p.gain, p.contrast);
  Rgba c = pal.sample(t);
  if (p.alpha_ramp) {
    c.a = alpha_for(normalized, p);
  }
  return c;
}

std::vector<Rgba8> bake_lut(const Palette & pal, const TransferParams & p, std::size_t n)
{
  if (n < 1) {
    n = 1;
  }
  std::vector<Rgba8> lut;
  lut.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    const float normalized = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
    const float t = apply_response(normalized, p.gain, p.contrast);
    Rgba c = pal.sample(t);
    if (p.alpha_ramp) {
      c.a = alpha_for(normalized, p);
    }
    lut.push_back(to_rgba8(c));
  }
  return lut;
}


std::vector<Rgba8> bake_lut(
  const Palette & pal, const TransferParams & p, std::size_t n, const BreakpointMap & map)
{
  if (n < 1) {
    n = 1;
  }
  std::vector<Rgba8> lut;
  lut.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    // The fraction the GPU's LINEAR normalize produces for this entry.
    const float f = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
    // Sweep the map's own (already normalized) domain, so an inverted or
    // non-finite input domain cannot make the sweep and the map disagree.
    const float value = map.lo() + (map.hi() - map.lo()) * f;
    // The breakpoint map replaces the linear position; gain/contrast then apply
    // exactly as in the unanchored bake, keeping the two paths consistent.
    const float t = apply_response(map.normalize(value), p.gain, p.contrast);
    Rgba c = pal.sample(t);
    if (p.alpha_ramp) {
      // Alpha stays a function of the LINEAR position, matching bake_lut(): the
      // ramp expresses "where in the display range am I", which the anchor does
      // not redefine.
      c.a = alpha_for(f, p);
    }
    lut.push_back(to_rgba8(c));
  }
  return lut;
}

bool has_shoreline(const Palette & pal)
{
  const std::optional<PaletteDomain> & domain = pal.domain();
  return domain.has_value() && domain->shoreline_position.has_value();
}

std::vector<Rgba8> bake_shoreline_anchored_lut(
  const Palette & pal, const TransferParams & p, float lo, float hi,
  std::optional<float> anchor_value, std::size_t n)
{
  // Gate: no anchor, an unusable anchor, or a palette with no declared shoreline
  // all fall back to the plain bake -- byte-identical, so anchoring can never
  // silently recolour a general-purpose ramp.
  if (!anchor_value || !std::isfinite(*anchor_value) || !has_shoreline(pal)) {
    return bake_lut(pal, p, n);
  }
  const float shoreline = *pal.domain()->shoreline_position;
  return bake_lut(pal, p, n, BreakpointMap(lo, hi, {Breakpoint{*anchor_value, shoreline}}));
}

}  // namespace marine_colormap
