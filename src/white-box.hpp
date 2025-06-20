/****************************************************************
**white-box.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-06.
*
* Description: Handles things related to view mode and/or the
*              white inspection square that can be moved around
*              the map.
*
*****************************************************************/
#pragma once

// rds
#include "white-box.rds.hpp"

// ss
#include "ss/unit-id.hpp"

namespace rn {

struct SS;
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
gfx::point find_a_good_white_box_location(
    SSConst const& ss, gfx::rect covered_tiles );

// Note that this will return a value even when the white box is
// not visible.
gfx::point white_box_tile( SSConst const& ss );

// NOTE: most code should not call this method; instead changes
// to the white box tile should be fed to the white box tile
// thread s that animation state can be updated. That said, if
// you want to set the white box position when NOT in a mode
// where it is visible then you can call this.
void set_white_box_tile( SS& ss, gfx::point tile );

} // namespace rn
