/****************************************************************
* viewport.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Handling of panning and zooming of the world
*              viewport.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "base-util.hpp"

#include <SDL.h>

namespace rn {

namespace viewport {

// Tiles touched by the viewport (tiles at the edge may only be
// partially visible).
ND Rect covered_tiles();

ND Rect get_render_src_rect();
ND Rect get_render_dest_rect();

void scale_zoom( double factor );
void pan( double down_up, double left_right, bool scale = false );

void center_on_tile( Coord coord );
ND bool is_tile_fully_visible( Coord coord );
void ensure_tile_surroundings_visible( Coord coord );

} // namespace viewport

} // namespace rn

