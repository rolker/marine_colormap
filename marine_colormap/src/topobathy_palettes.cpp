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

// Topo-bathymetric palettes: a single ramp covering seafloor and land with a
// transition at the shoreline. Both carry a PaletteDomain so a consumer knows
// the elevations the colours were designed against and where the shoreline sits
// in colour space, instead of hard-coding it (see marine_colormap#15).

#include <vector>

#include "marine_colormap/color.hpp"
#include "marine_colormap/palette.hpp"

namespace marine_colormap
{
namespace detail
{

namespace
{
Rgba rgb8(int r, int g, int b)
{
  return Rgba{r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
}
}  // namespace

// ---------------------------------------------------------------------------
// oleron -- Fabio Crameri's Scientific Colour Maps, v8.0.0.
//
// License: MIT License
// Copyright (c) 2023, Fabio Crameri.
// Crameri, F. (2023). Scientific colour maps. Zenodo.
//   https://doi.org/10.5281/zenodo.1243862
// Rationale: Crameri, Shephard & Heron (2020), "The misuse of colour in science
//   communication", Nature Communications 11:5444.
//
// Transcribed from the GMT distribution's SCM/oleron.cpt, whose z-axis is
// normalised -1/+1 to place the hinge at zero -- so the shoreline lands at
// exactly 0.5 here. Perceptually uniform on each side, with a hard hue break at
// the shoreline expressed as two coincident stops.
//
// Sea runs dark navy to near-white pale blue (depth lightening toward shore);
// land jumps to dark green and lightens to cream. Note this is NOT the classic
// hypsometric look -- see `hypsometric` below for that.
// ---------------------------------------------------------------------------
std::vector<ColorStop> oleron_stops()
{
  return {
    {0.00000000f, rgb8(26, 38, 89)},
    {0.00392200f, rgb8(27, 40, 91)},
    {0.00784300f, rgb8(29, 41, 92)},
    {0.01176500f, rgb8(30, 43, 94)},
    {0.01568600f, rgb8(32, 44, 95)},
    {0.01960800f, rgb8(33, 46, 97)},
    {0.02352900f, rgb8(35, 47, 98)},
    {0.02745100f, rgb8(36, 49, 100)},
    {0.03137300f, rgb8(38, 50, 101)},
    {0.03529400f, rgb8(40, 52, 103)},
    {0.03921600f, rgb8(41, 53, 104)},
    {0.04313700f, rgb8(43, 55, 106)},
    {0.04705900f, rgb8(44, 56, 107)},
    {0.05098000f, rgb8(46, 58, 109)},
    {0.05490200f, rgb8(47, 59, 111)},
    {0.05882400f, rgb8(49, 61, 112)},
    {0.06274500f, rgb8(50, 63, 114)},
    {0.06666700f, rgb8(52, 64, 115)},
    {0.07058800f, rgb8(53, 66, 117)},
    {0.07451000f, rgb8(55, 67, 118)},
    {0.07843100f, rgb8(57, 69, 120)},
    {0.08235300f, rgb8(58, 71, 122)},
    {0.08627500f, rgb8(60, 72, 123)},
    {0.09019600f, rgb8(61, 74, 125)},
    {0.09411800f, rgb8(63, 75, 126)},
    {0.09803900f, rgb8(65, 77, 128)},
    {0.10196100f, rgb8(66, 79, 130)},
    {0.10588200f, rgb8(68, 80, 131)},
    {0.10980400f, rgb8(69, 82, 133)},
    {0.11372500f, rgb8(71, 84, 135)},
    {0.11764700f, rgb8(73, 85, 136)},
    {0.12156900f, rgb8(74, 87, 138)},
    {0.12549000f, rgb8(76, 89, 140)},
    {0.12941200f, rgb8(78, 90, 141)},
    {0.13333300f, rgb8(79, 92, 143)},
    {0.13725500f, rgb8(81, 94, 145)},
    {0.14117600f, rgb8(83, 95, 146)},
    {0.14509800f, rgb8(84, 97, 148)},
    {0.14902000f, rgb8(86, 99, 150)},
    {0.15294100f, rgb8(88, 100, 151)},
    {0.15686300f, rgb8(89, 102, 153)},
    {0.16078400f, rgb8(91, 104, 155)},
    {0.16470600f, rgb8(93, 105, 156)},
    {0.16862700f, rgb8(94, 107, 158)},
    {0.17254900f, rgb8(96, 109, 160)},
    {0.17647100f, rgb8(98, 111, 162)},
    {0.18039200f, rgb8(100, 112, 163)},
    {0.18431400f, rgb8(101, 114, 165)},
    {0.18823500f, rgb8(103, 116, 167)},
    {0.19215700f, rgb8(105, 117, 169)},
    {0.19607800f, rgb8(107, 119, 170)},
    {0.20000000f, rgb8(108, 121, 172)},
    {0.20392200f, rgb8(110, 123, 174)},
    {0.20784300f, rgb8(112, 125, 176)},
    {0.21176500f, rgb8(114, 126, 177)},
    {0.21568600f, rgb8(115, 128, 179)},
    {0.21960800f, rgb8(117, 130, 181)},
    {0.22352900f, rgb8(119, 132, 183)},
    {0.22745100f, rgb8(121, 133, 184)},
    {0.23137300f, rgb8(122, 135, 186)},
    {0.23529400f, rgb8(124, 137, 188)},
    {0.23921600f, rgb8(126, 139, 190)},
    {0.24313700f, rgb8(128, 141, 192)},
    {0.24705900f, rgb8(130, 142, 193)},
    {0.25098000f, rgb8(131, 144, 195)},
    {0.25490200f, rgb8(133, 146, 197)},
    {0.25882400f, rgb8(135, 148, 199)},
    {0.26274500f, rgb8(137, 150, 201)},
    {0.26666700f, rgb8(139, 151, 202)},
    {0.27058800f, rgb8(141, 153, 204)},
    {0.27451000f, rgb8(142, 155, 206)},
    {0.27843100f, rgb8(144, 157, 208)},
    {0.28235300f, rgb8(146, 159, 210)},
    {0.28627500f, rgb8(148, 161, 211)},
    {0.29019600f, rgb8(150, 162, 213)},
    {0.29411800f, rgb8(152, 164, 215)},
    {0.29803900f, rgb8(153, 166, 217)},
    {0.30196100f, rgb8(155, 168, 219)},
    {0.30588200f, rgb8(157, 170, 220)},
    {0.30980400f, rgb8(159, 172, 222)},
    {0.31372500f, rgb8(161, 173, 224)},
    {0.31764700f, rgb8(163, 175, 225)},
    {0.32156900f, rgb8(164, 177, 227)},
    {0.32549000f, rgb8(166, 179, 229)},
    {0.32941200f, rgb8(168, 181, 230)},
    {0.33333300f, rgb8(170, 183, 232)},
    {0.33725500f, rgb8(172, 184, 233)},
    {0.34117600f, rgb8(173, 186, 234)},
    {0.34509800f, rgb8(175, 188, 236)},
    {0.34902000f, rgb8(177, 189, 237)},
    {0.35294100f, rgb8(178, 191, 238)},
    {0.35686300f, rgb8(180, 193, 239)},
    {0.36078400f, rgb8(182, 194, 240)},
    {0.36470600f, rgb8(183, 196, 241)},
    {0.36862700f, rgb8(185, 198, 242)},
    {0.37254900f, rgb8(186, 199, 243)},
    {0.37647100f, rgb8(188, 201, 243)},
    {0.38039200f, rgb8(189, 202, 244)},
    {0.38431400f, rgb8(191, 203, 244)},
    {0.38823500f, rgb8(192, 205, 245)},
    {0.39215700f, rgb8(194, 206, 245)},
    {0.39607800f, rgb8(195, 208, 246)},
    {0.40000000f, rgb8(196, 209, 246)},
    {0.40392200f, rgb8(198, 210, 247)},
    {0.40784300f, rgb8(199, 212, 247)},
    {0.41176500f, rgb8(200, 213, 248)},
    {0.41568600f, rgb8(202, 214, 248)},
    {0.41960800f, rgb8(203, 216, 248)},
    {0.42352900f, rgb8(204, 217, 249)},
    {0.42745100f, rgb8(206, 218, 249)},
    {0.43137300f, rgb8(207, 220, 249)},
    {0.43529400f, rgb8(208, 221, 250)},
    {0.43921600f, rgb8(210, 222, 250)},
    {0.44313700f, rgb8(211, 224, 250)},
    {0.44705900f, rgb8(212, 225, 251)},
    {0.45098000f, rgb8(214, 226, 251)},
    {0.45490200f, rgb8(215, 228, 251)},
    {0.45882400f, rgb8(216, 229, 252)},
    {0.46274500f, rgb8(218, 230, 252)},
    {0.46666700f, rgb8(219, 232, 252)},
    {0.47058800f, rgb8(220, 233, 253)},
    {0.47451000f, rgb8(222, 234, 253)},
    {0.47843100f, rgb8(223, 236, 253)},
    {0.48235300f, rgb8(224, 237, 254)},
    {0.48627500f, rgb8(226, 238, 254)},
    {0.49019600f, rgb8(227, 240, 254)},
    {0.49411800f, rgb8(228, 241, 255)},
    {0.50000000f, rgb8(230, 242, 255)},
    {0.50000000f, rgb8(26, 76, 0)},
    {0.50588200f, rgb8(29, 77, 0)},
    {0.50980400f, rgb8(31, 78, 0)},
    {0.51372500f, rgb8(34, 79, 0)},
    {0.51764700f, rgb8(37, 79, 0)},
    {0.52156900f, rgb8(39, 80, 0)},
    {0.52549000f, rgb8(42, 81, 0)},
    {0.52941200f, rgb8(44, 81, 0)},
    {0.53333300f, rgb8(47, 82, 0)},
    {0.53725500f, rgb8(49, 83, 0)},
    {0.54117600f, rgb8(51, 84, 0)},
    {0.54509800f, rgb8(53, 84, 0)},
    {0.54902000f, rgb8(56, 85, 0)},
    {0.55294100f, rgb8(58, 86, 0)},
    {0.55686300f, rgb8(60, 86, 0)},
    {0.56078400f, rgb8(62, 87, 0)},
    {0.56470600f, rgb8(64, 87, 0)},
    {0.56862700f, rgb8(66, 88, 0)},
    {0.57254900f, rgb8(68, 89, 0)},
    {0.57647100f, rgb8(70, 89, 0)},
    {0.58039200f, rgb8(73, 90, 1)},
    {0.58431400f, rgb8(75, 91, 1)},
    {0.58823500f, rgb8(77, 92, 1)},
    {0.59215700f, rgb8(79, 92, 2)},
    {0.59607800f, rgb8(81, 93, 2)},
    {0.60000000f, rgb8(83, 94, 2)},
    {0.60392200f, rgb8(85, 95, 3)},
    {0.60784300f, rgb8(87, 96, 4)},
    {0.61176500f, rgb8(90, 96, 5)},
    {0.61568600f, rgb8(92, 97, 6)},
    {0.61960800f, rgb8(94, 98, 7)},
    {0.62352900f, rgb8(96, 99, 9)},
    {0.62745100f, rgb8(99, 100, 10)},
    {0.63137300f, rgb8(101, 102, 12)},
    {0.63529400f, rgb8(103, 103, 14)},
    {0.63921600f, rgb8(106, 104, 16)},
    {0.64313700f, rgb8(108, 105, 18)},
    {0.64705900f, rgb8(110, 106, 20)},
    {0.65098000f, rgb8(113, 108, 22)},
    {0.65490200f, rgb8(115, 109, 24)},
    {0.65882400f, rgb8(117, 110, 26)},
    {0.66274500f, rgb8(120, 112, 29)},
    {0.66666700f, rgb8(122, 113, 31)},
    {0.67058800f, rgb8(124, 114, 33)},
    {0.67451000f, rgb8(126, 116, 35)},
    {0.67843100f, rgb8(129, 117, 37)},
    {0.68235300f, rgb8(131, 118, 40)},
    {0.68627500f, rgb8(133, 120, 42)},
    {0.69019600f, rgb8(135, 121, 44)},
    {0.69411800f, rgb8(138, 123, 46)},
    {0.69803900f, rgb8(140, 124, 49)},
    {0.70196100f, rgb8(142, 125, 51)},
    {0.70588200f, rgb8(144, 127, 53)},
    {0.70980400f, rgb8(146, 128, 55)},
    {0.71372500f, rgb8(148, 130, 58)},
    {0.71764700f, rgb8(151, 131, 60)},
    {0.72156900f, rgb8(153, 132, 62)},
    {0.72549000f, rgb8(155, 134, 64)},
    {0.72941200f, rgb8(157, 135, 67)},
    {0.73333300f, rgb8(159, 137, 69)},
    {0.73725500f, rgb8(161, 138, 71)},
    {0.74117600f, rgb8(163, 140, 73)},
    {0.74509800f, rgb8(166, 141, 76)},
    {0.74902000f, rgb8(168, 143, 78)},
    {0.75294100f, rgb8(170, 144, 80)},
    {0.75686300f, rgb8(172, 146, 83)},
    {0.76078400f, rgb8(174, 147, 85)},
    {0.76470600f, rgb8(177, 149, 87)},
    {0.76862700f, rgb8(179, 151, 89)},
    {0.77254900f, rgb8(181, 152, 92)},
    {0.77647100f, rgb8(183, 154, 94)},
    {0.78039200f, rgb8(186, 156, 96)},
    {0.78431400f, rgb8(188, 157, 99)},
    {0.78823500f, rgb8(190, 159, 101)},
    {0.79215700f, rgb8(193, 161, 103)},
    {0.79607800f, rgb8(195, 163, 106)},
    {0.80000000f, rgb8(197, 164, 108)},
    {0.80392200f, rgb8(199, 166, 110)},
    {0.80784300f, rgb8(202, 168, 113)},
    {0.81176500f, rgb8(204, 170, 115)},
    {0.81568600f, rgb8(206, 172, 117)},
    {0.81960800f, rgb8(209, 173, 120)},
    {0.82352900f, rgb8(211, 175, 122)},
    {0.82745100f, rgb8(213, 177, 124)},
    {0.83137300f, rgb8(215, 179, 127)},
    {0.83529400f, rgb8(217, 181, 129)},
    {0.83921600f, rgb8(220, 182, 132)},
    {0.84313700f, rgb8(222, 184, 134)},
    {0.84705900f, rgb8(224, 186, 137)},
    {0.85098000f, rgb8(226, 188, 139)},
    {0.85490200f, rgb8(228, 190, 142)},
    {0.85882400f, rgb8(229, 192, 144)},
    {0.86274500f, rgb8(231, 194, 147)},
    {0.86666700f, rgb8(233, 196, 149)},
    {0.87058800f, rgb8(234, 197, 152)},
    {0.87451000f, rgb8(236, 199, 154)},
    {0.87843100f, rgb8(237, 201, 157)},
    {0.88235300f, rgb8(238, 203, 159)},
    {0.88627500f, rgb8(240, 205, 162)},
    {0.89019600f, rgb8(241, 206, 164)},
    {0.89411800f, rgb8(242, 208, 167)},
    {0.89803900f, rgb8(242, 210, 169)},
    {0.90196100f, rgb8(243, 212, 171)},
    {0.90588200f, rgb8(244, 213, 174)},
    {0.90980400f, rgb8(245, 215, 176)},
    {0.91372500f, rgb8(245, 217, 178)},
    {0.91764700f, rgb8(246, 218, 181)},
    {0.92156900f, rgb8(246, 220, 183)},
    {0.92549000f, rgb8(247, 222, 185)},
    {0.92941200f, rgb8(247, 223, 188)},
    {0.93333300f, rgb8(247, 225, 190)},
    {0.93725500f, rgb8(248, 226, 192)},
    {0.94117600f, rgb8(248, 228, 195)},
    {0.94509800f, rgb8(248, 230, 197)},
    {0.94902000f, rgb8(249, 231, 199)},
    {0.95294100f, rgb8(249, 233, 201)},
    {0.95686300f, rgb8(249, 234, 204)},
    {0.96078400f, rgb8(250, 236, 206)},
    {0.96470600f, rgb8(250, 238, 208)},
    {0.96862700f, rgb8(250, 239, 211)},
    {0.97254900f, rgb8(251, 241, 213)},
    {0.97647100f, rgb8(251, 243, 215)},
    {0.98039200f, rgb8(251, 244, 218)},
    {0.98431400f, rgb8(251, 246, 220)},
    {0.98823500f, rgb8(252, 248, 222)},
    {0.99215700f, rgb8(252, 249, 225)},
    {0.99607800f, rgb8(252, 251, 227)},
    {1.00000000f, rgb8(253, 253, 230)},
  };
}

// ---------------------------------------------------------------------------
// hypsometric -- the classic elevation-tinted cartographic ramp: cyan-green
// shallows, yellow-green lowlands, tans and browns rising, grey and white at
// altitude for rock and snow.
//
// Transcribed from GeoZui4D's default colour lookup table
// (`GeoZui4D/main/engine/TextureMaps/Clut.cpp`, `Clut::CreateDefault()`),
// Copyright 2000-2026 Center for Coastal and Ocean Mapping, University of New
// Hampshire, Apache-2.0 -- the same licence as this package.
//
// Chosen over vendoring GMT's `globe`, which is the usual source for this look
// but is LGPL-3+ and so cannot be shipped in an Apache-2.0 package.
//
// The original nodes are absolute elevations in metres from -6000 to +9000,
// preserved here as the palette's natural domain; the shoreline therefore sits
// at (0 - -6000) / 15000 = 0.4 rather than the middle. Unlike `oleron` the
// transition at zero is continuous in colour -- a strong hue shift from mint to
// yellow-green, not a hard step.
// ---------------------------------------------------------------------------
std::vector<ColorStop> hypsometric_stops()
{
  return {
    {0.00000000f, rgb8(17, 10, 59)},   //  -6000 m
    {0.06666667f, rgb8(18, 10, 59)},   //  -5000 m
    {0.13333333f, rgb8(23, 49, 111)},   //  -4000 m
    {0.20000000f, rgb8(20, 90, 140)},   //  -3000 m
    {0.23333333f, rgb8(27, 104, 164)},   //  -2500 m
    {0.26666667f, rgb8(30, 114, 179)},   //  -2000 m
    {0.30000000f, rgb8(29, 139, 196)},   //  -1500 m
    {0.33333333f, rgb8(27, 165, 210)},   //  -1000 m
    {0.36666667f, rgb8(28, 184, 224)},   //   -500 m
    {0.38666667f, rgb8(27, 204, 236)},   //   -200 m
    {0.39333333f, rgb8(27, 213, 241)},   //   -100 m
    {0.39666667f, rgb8(38, 223, 241)},   //    -50 m
    {0.39833333f, rgb8(49, 230, 236)},   //    -25 m
    {0.39933333f, rgb8(105, 242, 233)},   //    -10 m
    {0.40000000f, rgb8(161, 255, 230)},   //     +0 m
    {0.40333333f, rgb8(195, 209, 80)},   //    +50 m
    {0.40666667f, rgb8(226, 215, 102)},   //   +100 m
    {0.41333333f, rgb8(223, 196, 92)},   //   +200 m
    {0.42000000f, rgb8(211, 178, 81)},   //   +300 m
    {0.42666667f, rgb8(189, 150, 60)},   //   +400 m
    {0.43333333f, rgb8(163, 127, 47)},   //   +500 m
    {0.44000000f, rgb8(153, 118, 43)},   //   +600 m
    {0.44666667f, rgb8(143, 110, 39)},   //   +700 m
    {0.45333333f, rgb8(135, 104, 36)},   //   +800 m
    {0.46666667f, rgb8(128, 98, 33)},   //  +1000 m
    {0.50000000f, rgb8(117, 88, 29)},   //  +1500 m
    {0.53333333f, rgb8(98, 71, 24)},   //  +2000 m
    {0.66666667f, rgb8(203, 203, 203)},   //  +4000 m
    {1.00000000f, rgb8(220, 220, 220)},   //  +9000 m
  };
}

PaletteDomain oleron_domain()
{
  // No natural elevation range: oleron ships normalised -1/+1 as a stretchable
  // master, so it is meaningful over whatever range the consumer picks. What it
  // does know is where its shoreline lives.
  PaletteDomain d;
  d.shoreline_position = 0.5f;
  return d;
}

PaletteDomain hypsometric_domain()
{
  // Its stops are literal elevations, so the natural range is real.
  PaletteDomain d;
  d.natural_min = -6000.0f;
  d.natural_max = 9000.0f;
  d.shoreline_position = 0.4f;   // (0 - -6000) / 15000
  return d;
}

}  // namespace detail
}  // namespace marine_colormap
