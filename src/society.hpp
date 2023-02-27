/****************************************************************
**society.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-04.
*
* Description: Abstraction for nations and tribes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// Rds
#include "society.rds.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/pixel.hpp"

namespace rn {

struct SSConst;

// Checks the map square in question to see if there are any
// units, colonies, or indian dwellings there. If so then it re-
// turns the society to which they belong. Note that, if there is
// a result, there is a unique result, since a single square can
// never be occupied by entities from different societies, even
// ones that are at peace. Note that when a missionary is working
// in a native dwelling the missionary is technically not owned
// by the map, so in that case this will always return the tribe
// of the native dwelling.
maybe<Society> society_on_square( SSConst const& ss,
                                  Coord          coord );

// Gets the color associated with the society. E.g. this is the
// one that will be used to paint the flag color of a unit and
// the banner of a colony or native dwelling.
gfx::pixel flag_color_for_society( Society const& society );

} // namespace rn
