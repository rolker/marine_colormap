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

#include "marine_colormap/palette.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace marine_colormap
{

namespace detail
{
// Defined in perceptual_palettes.cpp (generated canonical viridis/turbo tables).
std::vector<Rgba> viridis_colors();
std::vector<Rgba> turbo_colors();
}  // namespace detail

Palette::Palette(std::string name, std::vector<ColorStop> stops)
: name_(std::move(name)), stops_(std::move(stops))
{
  // sample() relies on stops being ordered by ascending t (it uses front()/
  // back() and a forward scan). Sort here so a caller passing unsorted stops
  // still interpolates correctly. stable_sort keeps equal-t stops in input
  // order, so a deliberate hard edge (two stops at the same t) is preserved.
  std::stable_sort(
    stops_.begin(), stops_.end(),
    [](const ColorStop & a, const ColorStop & b) {return a.t < b.t;});
}

Rgba Palette::sample(float t) const
{
  if (stops_.empty()) {
    return Rgba{};
  }
  t = std::clamp(t, 0.0f, 1.0f);
  if (stops_.size() == 1 || t <= stops_.front().t) {
    return stops_.front().color;
  }
  if (t >= stops_.back().t) {
    return stops_.back().color;
  }
  for (std::size_t i = 1; i < stops_.size(); ++i) {
    if (t <= stops_[i].t) {
      const ColorStop & a = stops_[i - 1];
      const ColorStop & b = stops_[i];
      const float span = b.t - a.t;
      const float f = (span > 0.0f) ? (t - a.t) / span : 0.0f;
      return lerp(a.color, b.color, f);
    }
  }
  return stops_.back().color;
}

namespace
{

Rgba rgb8(int r, int g, int b)
{
  return Rgba{r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
}

// Build a palette from evenly spaced colors (t = i / (n - 1)).
Palette even(std::string name, std::vector<Rgba> colors)
{
  std::vector<ColorStop> stops;
  stops.reserve(colors.size());
  const std::size_t n = colors.size();
  for (std::size_t i = 0; i < n; ++i) {
    const float t = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
    stops.push_back(ColorStop{t, colors[i]});
  }
  return Palette(std::move(name), std::move(stops));
}

// Canonical built-ins. Order is append-only (see palette.hpp). The `thermal`
// ramp is the de-duplicated sonar thermal (the rviz_sonar_image ramp had an
// accidental duplicated stop; this drops it). `bronze` and `grayscale` match the
// existing sonar definitions. viridis/turbo carry the canonical published
// matplotlib tables (see perceptual_palettes.cpp).
const std::vector<Palette> & registry()
{
  static const std::vector<Palette> kPalettes = {
    even("grayscale", {rgb8(0, 0, 0), rgb8(255, 255, 255)}),
    even("bronze", {
          rgb8(0, 0, 0), rgb8(60, 30, 10), rgb8(130, 75, 25),
          rgb8(200, 140, 70), rgb8(255, 225, 170)}),
    even("thermal", {
          rgb8(77, 77, 77), rgb8(5, 102, 242), rgb8(33, 23, 181),
          rgb8(38, 166, 138), rgb8(18, 156, 105), rgb8(161, 209, 61),
          rgb8(252, 179, 46), rgb8(250, 94, 153), rgb8(252, 48, 97),
          rgb8(219, 41, 51), rgb8(166, 51, 51), rgb8(153, 10, 15)}),
    even("viridis", detail::viridis_colors()),
    even("turbo", detail::turbo_colors()),
    // Bathy-uncertainty warning ramp (issue #7 operator requirement): a green
    // -> yellow -> red diverging stoplight, t=0 good -> t=0.5 caution ->
    // t=1 bad. Appended last to keep existing indices stable.
    even("quality", {rgb8(0, 170, 0), rgb8(255, 215, 0), rgb8(210, 0, 0)}),
  };
  return kPalettes;
}

const std::vector<std::string> & registry_names()
{
  static const std::vector<std::string> kNames = [] {
      std::vector<std::string> names;
      for (const Palette & p : registry()) {
        names.push_back(p.name());
      }
      return names;
    }();
  return kNames;
}

}  // namespace

const std::vector<Palette> & palettes()
{
  return registry();
}

std::size_t palette_count()
{
  return registry().size();
}

const std::vector<std::string> & palette_names()
{
  return registry_names();
}

std::optional<std::size_t> palette_index(std::string_view name)
{
  const std::vector<Palette> & ps = registry();
  for (std::size_t i = 0; i < ps.size(); ++i) {
    if (ps[i].name() == name) {
      return i;
    }
  }
  return std::nullopt;
}

const Palette & palette(std::size_t index)
{
  const std::vector<Palette> & ps = registry();
  if (index >= ps.size()) {
    throw std::out_of_range("marine_colormap::palette: index out of range");
  }
  return ps[index];
}

const Palette * find_palette(std::string_view name)
{
  const std::optional<std::size_t> idx = palette_index(name);
  return idx ? &registry()[*idx] : nullptr;
}

}  // namespace marine_colormap
