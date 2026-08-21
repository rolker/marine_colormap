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

#ifndef MARINE_COLORMAP__OCCUPANCY_HPP_
#define MARINE_COLORMAP__OCCUPANCY_HPP_

#include "marine_colormap/lookup.hpp"

namespace marine_colormap
{

/// The rviz "costmap" colour scheme for a published `nav_msgs/OccupancyGrid`,
/// as a `LookupTable` over the `int8` domain [-128, 127].
///
/// This is the **fixed-domain** case: the table is keyed by absolute data
/// values, so it owns its domain intrinsically and there is nothing for an
/// operator to rescale. No range model is involved, and none should be.
///
/// Colours track rviz's `palette_builder.cpp` deliberately: the discrete values
/// (free space, inscribed, lethal, unknown, the illegal bands) are exact, and
/// the two ramps agree to within one 8-bit level -- see the precision note at
/// the end. Operators recognise magenta-is-lethal and cyan-is-inscribed on
/// sight, and a deviation would read as a bug in the field rather than as a
/// design choice:
///
/// | value    | meaning          | colour                      |
/// |----------|------------------|-----------------------------|
/// | 0        | free space       | fully transparent           |
/// | 1..98    | cost             | blue -> red ramp            |
/// | 99       | inscribed        | cyan (0,255,255)            |
/// | 100      | lethal           | magenta (255,0,255)         |
/// | 101..127 | illegal positive | green (0,255,0)             |
/// | -128..-2 | illegal negative | red -> yellow ramp          |
/// | -1       | unknown          | blue-grey (112,137,134)     |
///
/// **-1 is a named entry, not the `bad` sentinel.** In this domain it is a
/// legal, meaningful value -- rviz's own source calls it "the legal -1 value" --
/// so conflating it with invalid data would be a category error. `bad` remains
/// reserved for genuinely non-finite input.
///
/// The entries are contiguous and cover the whole domain, so `unmapped` is
/// unreachable for any `int8` value, and so are `under` and `over`. Both are
/// left unset deliberately, which resolves them to the colour of the entry
/// owning each extreme -- the two "illegal" bands. A value outside the `int8`
/// domain is a caller error, and showing it in rviz's own illegal-value colours
/// makes it visible rather than letting it render transparent and be mistaken
/// for "no data here".
///
/// **Trap worth knowing**: nav2's *internal* costmap is `uint8` with
/// `NO_INFORMATION = 255`, `LETHAL_OBSTACLE = 254` and
/// `INSCRIBED_INFLATED_OBSTACLE = 253`; `costmap_2d_publisher.cpp` squeezes
/// those to 99 / 100 / -1 on publish. This table is for the **published
/// OccupancyGrid**. A `nav2_msgs/Costmap` table would need different sentinel
/// values.
///
/// Precision note: rviz computes its ramps with truncating integer division
/// while this library works in float, so a converted 8-bit channel can differ
/// by one level at some values. The ramp itself is exact -- the difference is
/// the rounding convention at the very end of the pipeline.
LookupTable occupancy_costmap_table();

}  // namespace marine_colormap

#endif  // MARINE_COLORMAP__OCCUPANCY_HPP_
