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

#include "base-util.hpp"

#include <SDL.h>

namespace rn {

Rect get_viewport();

Rect viewport_covered_tiles();

::SDL_Rect viewport_get_render_src_rect();
::SDL_Rect viewport_get_render_dest_rect();

void viewport_scale_zoom( double factor );
void viewport_pan( double down_up, double left_right, bool scale = false );

} // namespace rn

