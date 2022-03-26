/****************************************************************
**road.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: All things to do with roads.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"

// render
#include "render/fwd.hpp"

namespace rn {

/****************************************************************
** Road State
*****************************************************************/
// Must be a land square or check fail.
void set_road( Coord tile );
void clear_road( Coord tile );
bool has_road( Coord tile );

/****************************************************************
** Rendering
*****************************************************************/
void render_road_if_present( rr::Painter& painter, Coord where,
                             Coord world_tile );

} // namespace rn
