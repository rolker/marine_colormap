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

// Renders every registered palette, plus the costmap lookup table, to an SVG.
//
// It walks the live registry rather than a hand-maintained list, so adding a
// palette shows up here with no edit to this file. That matters: a drawn-by-hand
// chart would misrepresent what we now ship -- `oleron` has a hard discontinuity
// at its shoreline and `hypsometric`'s stops are literal elevations, neither of
// which survives being redrawn as an even swatch.
//
// SVG because the core package has no image library and must not grow one; SVG
// is text this tool can emit directly. See docs/generate_palette_chart.sh for
// the PNG conversion used by the docs.

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "marine_colormap/occupancy.hpp"
#include "marine_colormap/palette.hpp"

namespace
{

constexpr int kWidth = 960;
constexpr int kPad = 24;
constexpr int kLabelHeight = 18;
constexpr int kRampHeight = 40;
constexpr int kRowGap = 16;
constexpr int kSamples = 480;  ///< horizontal resolution of each ramp

std::string hex(const marine_colormap::Rgba & c)
{
  const marine_colormap::Rgba8 c8 = marine_colormap::to_rgba8(c);
  char buf[8];
  std::snprintf(
    buf, sizeof(buf), "#%02x%02x%02x",
    static_cast<int>(c8.r), static_cast<int>(c8.g), static_cast<int>(c8.b));
  return std::string(buf);
}

/// Alpha matters: the costmap's free-space entry is fully transparent, and a
/// chart that painted it opaque would misdescribe the palette.
///
/// The stream is imbued with the classic locale because SVG requires '.' as the
/// decimal separator. A default-constructed stream follows the global locale,
/// so under e.g. de_DE this would emit `fill-opacity="0,502"` and produce an
/// invalid document. Same reason the geometry stream is imbued in main().
std::string alpha_attr(const marine_colormap::Rgba & c)
{
  if (c.a >= 0.999f) {return std::string();}
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os << " fill-opacity=\"" << std::fixed << std::setprecision(3) << c.a << "\"";
  return os.str();
}

/// One block of a swatch. Boundaries are computed from the block index so
/// adjacent blocks abut exactly and the last one lands on the ramp's right edge
/// instead of overshooting the border. A hairline overlap is added between
/// neighbours -- but never past the end -- so renderers do not leave seams.
void write_block(
  std::ostream & os, double x, int y, double step, int i, int count, int h,
  const marine_colormap::Rgba & c)
{
  const double x0 = x + i * step;
  const double x1 = x + (i + 1) * step;
  const double width = (i + 1 < count) ? (x1 - x0 + 0.5) : (x1 - x0);
  os << "<rect x=\"" << std::fixed << std::setprecision(2) << x0
     << "\" y=\"" << y << "\" width=\"" << width << "\" height=\"" << h
     << "\" fill=\"" << hex(c) << "\"" << alpha_attr(c) << "/>\n";
}

void escape_into(std::ostream & os, const std::string & text)
{
  for (const char ch : text) {
    switch (ch) {
      case '&': os << "&amp;"; break;
      case '<': os << "&lt;"; break;
      case '>': os << "&gt;"; break;
      case '"': os << "&quot;"; break;
      default: os << ch;
    }
  }
}

/// A checkerboard behind the ramps so transparency reads as transparency rather
/// than as whatever the page background happens to be.
void write_checker_defs(std::ostream & os)
{
  os << "<defs><pattern id=\"checker\" width=\"12\" height=\"12\" "
     << "patternUnits=\"userSpaceOnUse\">"
     << "<rect width=\"12\" height=\"12\" fill=\"#ffffff\"/>"
     << "<rect width=\"6\" height=\"6\" fill=\"#e8e8e8\"/>"
     << "<rect x=\"6\" y=\"6\" width=\"6\" height=\"6\" fill=\"#e8e8e8\"/>"
     << "</pattern></defs>\n";
}

void write_label(std::ostream & os, int x, int y, const std::string & text, bool bold)
{
  os << "<text x=\"" << x << "\" y=\"" << y << "\" font-size=\"13\" fill=\"#222\""
     << (bold ? " font-weight=\"600\"" : "") << ">";
  escape_into(os, text);
  os << "</text>\n";
}

void write_ramp_frame(std::ostream & os, int x, int y, int w, int h)
{
  os << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w << "\" height=\"" << h
     << "\" fill=\"url(#checker)\"/>\n";
}

void write_ramp_border(std::ostream & os, int x, int y, int w, int h)
{
  os << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w << "\" height=\"" << h
     << "\" fill=\"none\" stroke=\"#888\" stroke-width=\"1\"/>\n";
}

/// One swatch, sampled through `sample_at(fraction)` so the caller decides
/// whether the horizontal axis is normalized position or a data domain.
template<typename SampleFn>
void write_swatch(std::ostream & os, int x, int y, int w, int h, SampleFn sample_at)
{
  write_ramp_frame(os, x, y, w, h);
  const double step = static_cast<double>(w) / kSamples;
  for (int i = 0; i < kSamples; ++i) {
    const double t = (i + 0.5) / kSamples;
    write_block(os, x, y, step, i, kSamples, h, sample_at(t));
  }
  write_ramp_border(os, x, y, w, h);
}

/// A lookup table is not a ramp: its entries are keyed by absolute values, and
/// several of them match exactly one integer. Sampling at fractional midpoints
/// would miss those entirely -- a midpoint like -127.7 matches no entry at all
/// and would render as the transparent `unmapped` sentinel, turning the
/// single-value bands (unknown, free space, inscribed, lethal) into holes and
/// misdescribing the table. So draw one block per integer value instead.
void write_indexed_swatch(
  std::ostream & os, int x, int y, int w, int h,
  const marine_colormap::LookupTable & table, int first, int last)
{
  write_ramp_frame(os, x, y, w, h);
  const int count = last - first + 1;
  const double step = static_cast<double>(w) / count;
  for (int i = 0; i < count; ++i) {
    write_block(os, x, y, step, i, count, h, table.lookup(static_cast<float>(first + i)));
  }
  write_ramp_border(os, x, y, w, h);
}

std::string describe(const marine_colormap::Palette & p)
{
  std::ostringstream os;
  os << p.name() << "  (" << p.stops().size() << " stops";
  if (p.domain()) {
    if (p.domain()->has_natural_range()) {
      os << ", natural range " << static_cast<int>(*p.domain()->natural_min) << " to "
         << static_cast<int>(*p.domain()->natural_max) << " m";
    }
    if (p.domain()->shoreline_position) {
      os << ", shoreline at " << std::fixed << std::setprecision(2)
         << *p.domain()->shoreline_position;
    }
  }
  os << ")";
  return os.str();
}

}  // namespace

int main(int argc, char ** argv)
{
  const std::vector<marine_colormap::Palette> & palettes = marine_colormap::palettes();
  const marine_colormap::LookupTable costmap = marine_colormap::occupancy_costmap_table();

  const int rows = static_cast<int>(palettes.size()) + 1;
  const int row_height = kLabelHeight + kRampHeight + kRowGap;
  const int height = kPad * 2 + 28 + rows * row_height;
  const int ramp_width = kWidth - kPad * 2;

  std::ostringstream os;
  // SVG numbers must use '.' regardless of the environment's locale.
  os.imbue(std::locale::classic());
  os << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << kWidth << "\" height=\""
     << height << "\" viewBox=\"0 0 " << kWidth << " " << height
     << "\" font-family=\"system-ui, -apple-system, Segoe UI, Roboto, sans-serif\">\n";
  os << "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n";
  write_checker_defs(os);
  write_label(os, kPad, kPad + 12, "marine_colormap palettes", true);

  int y = kPad + 28;
  for (std::size_t i = 0; i < palettes.size(); ++i) {
    const marine_colormap::Palette & p = palettes[i];
    write_label(os, kPad, y + 13, std::to_string(i) + ".  " + describe(p), false);
    write_swatch(
      os, kPad, y + kLabelHeight, ramp_width, kRampHeight,
      [&p](double t) {return p.sample(static_cast<float>(t));});

    // Mark a declared shoreline so the hard break is visibly deliberate.
    if (p.domain() && p.domain()->shoreline_position) {
      const double sx = kPad + ramp_width * *p.domain()->shoreline_position;
      os << "<line x1=\"" << std::fixed << std::setprecision(2) << sx << "\" y1=\""
         << (y + kLabelHeight - 4) << "\" x2=\"" << sx << "\" y2=\""
         << (y + kLabelHeight + kRampHeight + 4)
         << "\" stroke=\"#c00\" stroke-width=\"1.5\"/>\n";
    }
    y += row_height;
  }

  // The costmap table is not a ramp: it is a fixed-domain lookup over int8, so
  // the axis here is the data value, not a normalized position.
  write_label(os, kPad, y + 13, "costmap lookup table  (int8 domain, -128 to 127)", false);
  write_indexed_swatch(os, kPad, y + kLabelHeight, ramp_width, kRampHeight, costmap, -128, 127);
  y += row_height;

  os << "<text x=\"" << kPad << "\" y=\"" << (y + 6)
     << "\" font-size=\"11\" fill=\"#666\">Checkerboard shows transparency. "
     << "Red rule marks a declared shoreline position.</text>\n";
  os << "</svg>\n";

  if (argc > 1) {
    std::FILE * f = std::fopen(argv[1], "w");
    if (f == nullptr) {
      std::cerr << "palette_chart: cannot open " << argv[1] << " for writing\n";
      return 1;
    }
    const std::string out = os.str();
    const std::size_t written = std::fwrite(out.data(), 1, out.size(), f);
    const bool closed_ok = (std::fclose(f) == 0);
    if (written != out.size() || !closed_ok) {
      std::cerr << "palette_chart: failed writing " << argv[1] << "\n";
      return 1;
    }
  } else {
    std::cout << os.str();
  }
  return 0;
}
