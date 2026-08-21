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

/// Optional metadata describing the data domain a palette was designed for.
///
/// Most palettes have none — a sonar ramp is meaningful over whatever range the
/// operator picks. A topo-bathy palette is different: its colors mean specific
/// elevations, and its shoreline sits at a specific place in the color range.
/// Without this, every consumer would hard-code "the shoreline of `oleron` is at
/// 0.5", which is exactly the duplication this library exists to remove.
struct PaletteDomain
{
  /// The elevation range the colors were designed against, in metres, positive
  /// up — set only when the palette actually has one, and both bounds together.
  /// `hypsometric` does (its stops are literal elevations); `oleron` does not,
  /// being a stretchable master normalized to +/-1 and meaningful over any
  /// range. Use it as a natural default range, or to render at true scale.
  std::optional<float> natural_min;
  std::optional<float> natural_max;

  /// Where the land/sea transition sits in normalized color space, if the
  /// palette has one. Anchor this to a data value with `BreakpointMap` to make
  /// the shoreline land on the right depth (see `lookup.hpp`).
  std::optional<float> shoreline_position;

  /// True when both natural bounds are present and ordered.
  bool has_natural_range() const
  {
    return natural_min && natural_max && *natural_max > *natural_min;
  }
};

/// A named colormap: an ordered list of stops, sampled by piecewise-linear
/// interpolation. Stop positions are explicit, so non-uniform ramps are
/// expressible -- the sonar and perceptual built-ins are evenly spaced, while
/// the topo-bathy ones are not (`hypsometric`'s stops are literal elevations,
/// and `oleron` carries two coincident stops at t = 0.5 for its hard shoreline
/// break).
class Palette
{
public:
  Palette(std::string name, std::vector<ColorStop> stops);
  Palette(std::string name, std::vector<ColorStop> stops, PaletteDomain domain);

  const std::string & name() const {return name_;}
  const std::vector<ColorStop> & stops() const {return stops_;}

  /// The domain this palette was designed for, when it has one. Unset for the
  /// general-purpose ramps, which are meaningful over any range.
  const std::optional<PaletteDomain> & domain() const {return domain_;}

  /// Color at normalized position `t` (clamped to [0, 1]). Values at or beyond
  /// the end stops return those stops' colors.
  ///
  /// Two stops sharing the same `t` express a **hard discontinuity**: `sample()`
  /// returns the lower stop's color exactly at `t` and interpolates from the
  /// upper stop just above it. That matches the `GeLtInterval` convention in
  /// `lookup.hpp`, where the lower band owns the boundary.
  Rgba sample(float t) const;

private:
  std::string name_;
  std::vector<ColorStop> stops_;
  std::optional<PaletteDomain> domain_;
};

// --- Built-in palette registry ---------------------------------------------
//
// The registry order is **append-only and name-keyed**: indices never change as
// palettes are added, so consumers can persist a selection by name (preferred)
// or index without it silently re-mapping. Current order: 0=grayscale,
// 1=bronze, 2=thermal, 3=viridis, 4=turbo, 5=quality, 6=oleron,
// 7=hypsometric. (viridis/turbo are the canonical matplotlib tables, see
// perceptual_palettes.cpp; oleron and hypsometric are the topo-bathy ramps in
// topobathy_palettes.cpp, each with its own licence notice.) New palettes
// append at the end so existing indices stay stable.

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
