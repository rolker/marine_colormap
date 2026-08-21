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

#include "marine_colormap/lookup.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace marine_colormap
{

namespace
{

float clamp01(float t)
{
  if (!(t > 0.0f)) {return 0.0f;}  // also catches NaN
  return t > 1.0f ? 1.0f : t;
}

}  // namespace

bool unbounded_below(Closure c)
{
  return c == Closure::LtSemiInterval || c == Closure::LeSemiInterval;
}

bool unbounded_above(Closure c)
{
  return c == Closure::GeSemiInterval || c == Closure::GtSemiInterval;
}

bool ValueRange::contains(float value) const
{
  // NaN is never contained: every comparison below is false for NaN, but be
  // explicit rather than relying on that falling out, because the intent
  // (a NaN sounding is `bad`, not a band member) is worth stating.
  if (!std::isfinite(value)) {return false;}

  switch (closure) {
    case Closure::ClosedInterval:
      return value >= lower && value <= upper;
    case Closure::OpenInterval:
      return value > lower && value < upper;
    case Closure::GeLtInterval:
      return value >= lower && value < upper;
    case Closure::GtLeInterval:
      return value > lower && value <= upper;
    case Closure::LtSemiInterval:
      return value < upper;
    case Closure::LeSemiInterval:
      return value <= upper;
    case Closure::GeSemiInterval:
      return value >= lower;
    case Closure::GtSemiInterval:
      return value > lower;
  }
  return false;
}

bool ValueRange::rampable() const
{
  if (unbounded_below(closure) || unbounded_above(closure)) {return false;}
  if (!std::isfinite(lower) || !std::isfinite(upper)) {return false;}
  return upper > lower;
}

float ValueRange::fraction(float value) const
{
  if (!rampable()) {return 0.0f;}
  return clamp01((value - lower) / (upper - lower));
}

Rgba LookupEntry::color_at(float value) const
{
  if (!end_color.has_value() || !range.rampable()) {
    return start_color;
  }
  return lerp(start_color, *end_color, range.fraction(value));
}

LookupTable::LookupTable(std::vector<LookupEntry> entries, Sentinels sentinels)
: entries_(std::move(entries)), sentinels_(std::move(sentinels))
{
}

std::optional<float> LookupTable::domain_min() const
{
  if (entries_.empty()) {return std::nullopt;}
  float lowest = 0.0f;
  bool have = false;
  for (const auto & e : entries_) {
    // An entry unbounded below means nothing is ever `under` — the domain has
    // no floor, so report none rather than the lowest finite bound we saw.
    if (unbounded_below(e.range.closure)) {return std::nullopt;}
    if (!std::isfinite(e.range.lower)) {return std::nullopt;}
    if (!have || e.range.lower < lowest) {
      lowest = e.range.lower;
      have = true;
    }
  }
  return have ? std::optional<float>(lowest) : std::nullopt;
}

std::optional<float> LookupTable::domain_max() const
{
  if (entries_.empty()) {return std::nullopt;}
  float highest = 0.0f;
  bool have = false;
  for (const auto & e : entries_) {
    if (unbounded_above(e.range.closure)) {return std::nullopt;}
    if (!std::isfinite(e.range.upper)) {return std::nullopt;}
    if (!have || e.range.upper > highest) {
      highest = e.range.upper;
      have = true;
    }
  }
  return have ? std::optional<float>(highest) : std::nullopt;
}

std::optional<std::size_t> LookupTable::match(float value) const
{
  for (std::size_t i = 0; i < entries_.size(); ++i) {
    if (entries_[i].range.contains(value)) {return i;}
  }
  return std::nullopt;
}

Rgba LookupTable::lookup(float value) const
{
  // 1. Invalid data wins over everything.
  if (!std::isfinite(value)) {return sentinels_.bad;}

  if (entries_.empty()) {return sentinels_.unmapped;}

  // 2. Outside the whole domain -> under / over. Checked before the match loop
  //    so that a value below every band reports as `under` rather than falling
  //    through to `unmapped`; `unmapped` is reserved for gaps *inside* the
  //    domain, which is the distinction VTK loses.
  const auto dmin = domain_min();
  if (dmin.has_value() && value < *dmin) {
    return sentinels_.under.value_or(entries_.front().start_color);
  }
  const auto dmax = domain_max();
  if (dmax.has_value() && value > *dmax) {
    const auto & last = entries_.back();
    return sentinels_.over.value_or(last.end_color.value_or(last.start_color));
  }

  // 3. First match wins.
  if (const auto i = match(value)) {
    return entries_[*i].color_at(value);
  }

  // 4. Inside the domain, claimed by nobody.
  return sentinels_.unmapped;
}

BreakpointMap::BreakpointMap(float lo, float hi, std::vector<Breakpoint> breaks)
: lo_(lo), hi_(hi), breaks_(std::move(breaks))
{
  // Match RangeModel::set_manual(): an inverted domain is swapped rather than
  // left to collapse, so an operator dragging handles past each other doesn't
  // break rendering.
  if (hi_ < lo_) {std::swap(lo_, hi_);}

  // Clamp each break into the domain, then sort. Clamping first and sorting
  // second keeps the result monotonic even when several breaks fall outside
  // the same end — they collapse onto the endpoint together rather than
  // crossing. A break outside the domain is the ordinary "no land in view"
  // case, not an error.
  for (auto & b : breaks_) {
    if (!std::isfinite(b.value)) {b.value = lo_;}
    b.value = std::min(std::max(b.value, lo_), hi_);
    b.position = clamp01(b.position);
  }
  std::stable_sort(
    breaks_.begin(), breaks_.end(),
    [](const Breakpoint & a, const Breakpoint & b) {return a.value < b.value;});

  // Positions must be non-decreasing for the map to stay monotonic. A caller
  // that supplies crossing positions gets them clamped up to the running
  // maximum rather than an exception or an inverted span.
  float running = 0.0f;
  for (auto & b : breaks_) {
    b.position = std::max(b.position, running);
    running = b.position;
  }
}

float BreakpointMap::normalize(float value) const
{
  if (!std::isfinite(value)) {return 0.0f;}
  if (!(hi_ > lo_)) {return 0.0f;}  // degenerate domain, matching normalize()

  // Clamp rather than extrapolate: a piecewise map has no meaningful linear
  // extension past its ends.
  value = std::min(std::max(value, lo_), hi_);

  // Anchors are (lo_, 0), each (break.value, break.position), and (hi_, 1).
  // Walk to the last anchor at or below `value`, then interpolate to the next.
  //
  // Approaching a coincident anchor *from above* is what makes a break clamped
  // onto a domain endpoint behave correctly. With no land in view the datum
  // break clamps onto `hi_`, and the whole domain then maps to [0, break
  // position] — the water keeps the water half of the palette instead of
  // stretching across the land colours too.
  float span_lo = lo_;
  float pos_lo = 0.0f;
  std::size_t i = 0;
  for (; i < breaks_.size(); ++i) {
    if (breaks_[i].value > value) {break;}
    span_lo = breaks_[i].value;
    pos_lo = breaks_[i].position;
  }

  const float span_hi = (i < breaks_.size()) ? breaks_[i].value : hi_;
  const float pos_hi = (i < breaks_.size()) ? breaks_[i].position : 1.0f;

  // Zero-width span: every value in it resolves to the anchor we came from.
  const float width = span_hi - span_lo;
  if (!(width > 0.0f)) {return clamp01(pos_lo);}

  const float t = (value - span_lo) / width;
  return clamp01(pos_lo + t * (pos_hi - pos_lo));
}

}  // namespace marine_colormap
