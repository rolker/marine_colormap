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

#ifndef MARINE_COLORMAP__LOOKUP_HPP_
#define MARINE_COLORMAP__LOOKUP_HPP_

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "marine_colormap/color.hpp"

namespace marine_colormap
{

/// Which endpoints of a `ValueRange` are part of the range.
///
/// These are the ISO 19103 closure types, reached here via S-100 Part 9's
/// coverage lookup (clause 9-12.7). Naming follows the standard rather than
/// C++ convention so that a portrayal catalogue maps across mechanically and
/// an S-102-literate reader recognises them on sight.
///
/// An explicit closure is what makes a break at a safety contour unambiguous:
/// with adjacent `GeLtInterval` bands, a sounding sitting exactly on the
/// contour belongs to the deeper band by construction rather than by whichever
/// comparison the renderer happened to write. S-102's shipped catalogue uses
/// `GeLtInterval` throughout for exactly this reason.
enum class Closure
{
  ClosedInterval,   ///< [lower, upper] — both endpoints included.
  OpenInterval,     ///< (lower, upper) — neither endpoint included.
  GeLtInterval,     ///< [lower, upper) — lower included. S-102's depth bands.
  GtLeInterval,     ///< (lower, upper] — upper included.
  LtSemiInterval,   ///< (-inf, upper) — `lower` unused.
  LeSemiInterval,   ///< (-inf, upper] — `lower` unused.
  GeSemiInterval,   ///< [lower, +inf) — `upper` unused.
  GtSemiInterval,   ///< (lower, +inf) — `upper` unused.
};

/// True when `c` leaves the range unbounded below (`lower` is unused).
bool unbounded_below(Closure c);

/// True when `c` leaves the range unbounded above (`upper` is unused).
bool unbounded_above(Closure c);

/// A numeric range with an explicit closure.
///
/// `float` rather than `double` deliberately: the whole transfer path and the
/// GLSL helper are float, and CPU/GPU agreement at a boundary is exactly what
/// the explicit closure exists to guarantee. Widening here would reintroduce
/// disagreement at the one place we most want it gone. (Precision note: float
/// resolves ~1 mm at 10 km, so survey depths and contour settings are
/// comfortable; a domain needing more than ~7 significant digits is not.)
///
/// A single value is expressed as `ClosedInterval` with `lower == upper`.
struct ValueRange
{
  float lower{0.0f};
  float upper{1.0f};
  Closure closure{Closure::GeLtInterval};

  /// True when `value` falls inside this range.
  ///
  /// Non-finite input is never contained — NaN compares false against every
  /// bound, and that is the behaviour we want: a NaN sounding is `bad`, not a
  /// member of some band. Infinite bounds are permitted and behave as written.
  bool contains(float value) const;

  /// Position of `value` within `[lower, upper]`, clamped to [0, 1]. Used to
  /// place a value along an entry's ramp. Returns 0 for a degenerate or
  /// unbounded range (a ramp needs two finite ends — see `LookupEntry`).
  float fraction(float value) const;

  /// True when both ends are finite and `upper > lower`, i.e. this range can
  /// carry a ramp rather than only a flat colour.
  bool rampable() const;
};

/// One entry of a lookup table: a labelled range carrying either a flat colour
/// or a two-endpoint ramp across that range.
///
/// This is S-100's `LookupEntry` + `CoverageColor` pair. `label` is not
/// decoration — it is the legend text for this band, and once several legends
/// share a screen (see `docs/vision.md`) it is what tells the operator which
/// quantity they are reading.
///
/// A ramp on a range that is not `rampable()` (unbounded, degenerate, or
/// inverted) falls back to the flat `start_color`. That is a deliberate
/// non-throwing degradation: an unbounded deep-water band with a ramp
/// configured should still render, not abort a chart.
struct LookupEntry
{
  std::string label;
  ValueRange range;
  Rgba start_color{0.0f, 0.0f, 0.0f, 1.0f};

  /// When set, the entry ramps linearly from `start_color` at `range.lower` to
  /// `end_color` at `range.upper`.
  std::optional<Rgba> end_color;

  /// Colour for `value`, assuming `range.contains(value)`.
  Rgba color_at(float value) const;
};

/// Colours for values that no entry claims.
///
/// Four distinct concepts, kept distinct on purpose. VTK collapses "no
/// matching entry" into its NaN colour, which makes a key that failed an exact
/// match indistinguishable from genuinely invalid data — a documented trap we
/// are not repeating. It matters concretely here: in an occupancy grid, -1 is
/// a *legal, meaningful* value (rviz's own source calls it "the legal -1
/// value"), so it belongs to a named entry, and `unmapped` is reserved for
/// values that fall in a real gap.
struct Sentinels
{
  /// Below every entry's domain. Unset resolves to the first entry's colour.
  std::optional<Rgba> under;

  /// Above every entry's domain. Unset resolves to the last entry's colour.
  std::optional<Rgba> over;

  /// Non-finite input (NaN / inf). Default fully transparent, matching
  /// `TransferParams::nodata_color` and matplotlib's `bad`.
  Rgba bad{0.0f, 0.0f, 0.0f, 0.0f};

  /// Inside the domain but claimed by no entry — a genuine gap in the table.
  /// Distinct from `bad`; see the note above.
  Rgba unmapped{0.0f, 0.0f, 0.0f, 0.0f};
};

/// An ordered, first-match lookup table: the shape S-100 Part 9 specifies for
/// portraying a continuous coverage, and the one structure that expresses
/// stepped, categorical and hard-break palettes without special cases.
///
/// Order is significant. Entries are tested in sequence and the first whose
/// range contains the value wins, so overlapping entries are legal and resolve
/// by precedence rather than being an error.
///
/// Sentinel precedence, in order: non-finite input is `bad`; a value outside
/// the whole domain is `under` / `over`; otherwise first match wins; a value
/// inside the domain that no entry claims is `unmapped`.
class LookupTable
{
public:
  LookupTable() = default;
  explicit LookupTable(std::vector<LookupEntry> entries, Sentinels sentinels = {});

  const std::vector<LookupEntry> & entries() const {return entries_;}
  const Sentinels & sentinels() const {return sentinels_;}
  void set_sentinels(Sentinels s) {sentinels_ = std::move(s);}

  bool empty() const {return entries_.empty();}

  /// Lowest finite `lower` across all entries. Unset when the table is empty
  /// or any entry is unbounded below (in which case nothing is ever `under`).
  std::optional<float> domain_min() const;

  /// Highest finite `upper` across all entries. Unset when the table is empty
  /// or any entry is unbounded above.
  std::optional<float> domain_max() const;

  /// Colour for `value`, following the precedence documented above. An empty
  /// table returns `sentinels().unmapped` for every finite input — a table
  /// with nothing in it has no opinion, and must not be a crash.
  Rgba lookup(float value) const;

  /// Index of the first entry containing `value`, or `std::nullopt`.
  std::optional<std::size_t> match(float value) const;

private:
  std::vector<LookupEntry> entries_;
  Sentinels sentinels_;
};

/// A breakpoint anchoring an absolute data value to a position in normalized
/// colour space.
///
/// `value` is where the break sits in the data (chart datum at 0.0, a safety
/// contour at whatever the operator set); `position` is where it sits in the
/// palette (0.5 for a two-sided topo-bathy ramp whose land and sea halves each
/// occupy half the colour table).
struct Breakpoint
{
  float value{0.0f};
  float position{0.5f};
};

/// A piecewise-linear map from data values onto normalized [0, 1] through N
/// anchored breakpoints — the generalisation of a GMT hinge.
///
/// With no breakpoints this is plain linear normalization, identical to the
/// free `normalize()`. With one it is a hard hinge: each side of the break is
/// stretched independently so both halves use their full share of the colour
/// range regardless of how asymmetric the data is. With two or more it carries
/// a shoreline *and* a safety contour in one palette, which is what no
/// surveyed system does off the shelf (GMT allows exactly one hinge; so does
/// matplotlib's `TwoSlopeNorm`).
///
/// **Degenerate inputs clamp; they never throw.** A break outside the active
/// domain is the ordinary case, not an error — a survey line with no land in
/// view has its datum break above `hi`. Breaks are clamped into `[lo, hi]` and
/// sorted, so the map stays monotonic and every value still resolves. A break
/// clamped onto an endpoint collapses its span to zero width; values there map
/// to that break's position.
///
/// Note what this class deliberately does *not* decide: whether `[lo, hi]`
/// itself follows the data (Auto) or is pinned (Manual/Fixed) is `RangeModel`'s
/// business. Re-stretching on a domain change is a range-policy question, not
/// a breakpoint question.
class BreakpointMap
{
public:
  BreakpointMap() = default;

  /// Build a map over `[lo, hi]`. Breakpoints may be supplied in any order and
  /// may lie outside `[lo, hi]`; both are handled (sorted, then clamped).
  /// An inverted `[lo, hi]` is swapped, matching `RangeModel::set_manual()`.
  BreakpointMap(float lo, float hi, std::vector<Breakpoint> breaks);

  float lo() const {return lo_;}
  float hi() const {return hi_;}

  /// The breakpoints as stored: sorted by value and clamped into `[lo, hi]`.
  const std::vector<Breakpoint> & breaks() const {return breaks_;}

  /// Map `value` onto [0, 1] through the anchored breaks. Result is clamped to
  /// [0, 1] — unlike the free `normalize()`, which returns a raw position —
  /// because a piecewise map has no meaningful linear extension past its ends.
  /// A degenerate domain (`hi <= lo`) returns 0, matching `normalize()`.
  /// Non-finite input returns 0.
  float normalize(float value) const;

private:
  float lo_{0.0f};
  float hi_{1.0f};
  std::vector<Breakpoint> breaks_;
};

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__LOOKUP_HPP_
