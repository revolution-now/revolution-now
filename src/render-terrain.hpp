/****************************************************************
**render-terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Renders individual terrain squares.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"

// render
#include "render/painter.hpp"

namespace rn {

/****************************************************************
** Rendering
*****************************************************************/
// This will fully render a map square with no units or colonies
// on it.
void render_terrain_square( rr::Painter& painter, Coord where,
                            Coord world_square );

// This one allows stretching the tile.
void render_terrain_square( rr::Painter& painter, Rect where,
                            Coord world_square );

} // namespace rn
