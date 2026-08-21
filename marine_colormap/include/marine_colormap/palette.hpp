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

#ifndef MARINE_COLORMAP__PALETTE_HPP_
#define MARINE_COLORMAP__PALETTE_HPP_

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "marine_colormap/color.hpp"

namespace marine_colormap
{

/// One color stop at normalized position `t` in [0, 1].
struct ColorStop
{
  float t{0.0f};
  Rgba color{};
};

/// A named colormap: an ordered list of stops, sampled by piecewise-linear
/// interpolation. Stop positions are explicit so non-uniform ramps are
/// expressible; the built-ins are evenly spaced.
class Palette
{
public:
  Palette(std::string name, std::vector<ColorStop> stops);

  const std::string & name() const {return name_;}
  const std::vector<ColorStop> & stops() const {return stops_;}

  /// Color at normalized position `t` (clamped to [0, 1]). Values at or beyond
  /// the end stops return those stops' colors.
  Rgba sample(float t) const;

private:
  std::string name_;
  std::vector<ColorStop> stops_;
};

// --- Built-in palette registry ---------------------------------------------
//
// The registry order is **append-only and name-keyed**: indices never change as
// palettes are added, so consumers can persist a selection by name (preferred)
// or index without it silently re-mapping. Current order: 0=grayscale,
// 1=bronze, 2=thermal, 3=viridis, 4=turbo, 5=quality. (viridis/turbo are the
// canonical matplotlib tables; see perceptual_palettes.cpp. `quality` is the
// green->yellow->red uncertainty ramp from ADR-0001.) New palettes append at
// the end so existing indices stay stable.

/// All built-in palettes, in registry order.
const std::vector<Palette> & palettes();

/// Number of built-in palettes.
std::size_t palette_count();

/// Built-in palette names, in registry order.
const std::vector<std::string> & palette_names();

/// Index of a palette by name, or nullopt if unknown.
std::optional<std::size_t> palette_index(std::string_view name);

/// Palette by registry index. Throws std::out_of_range if out of range.
const Palette & palette(std::size_t index);

/// Palette by name, or nullptr if unknown (does not throw).
const Palette * find_palette(std::string_view name);

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__PALETTE_HPP_
