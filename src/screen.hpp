/****************************************************************
**screen.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-18.
*
* Description: Handles screen resolution and scaling.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "geo-types.hpp"
#include "sdl-util.hpp" // TODO: get rid of this here

namespace rn {

extern ::SDL_Window*   g_window;
extern ::SDL_Renderer* g_renderer;
extern Texture         g_texture_viewport;

extern Scale g_resolution_scale_factor;
// This origin will be a small distance away from the screen's
// origin and will skip a few pixels that are needed to make the
// integral scalling factor.
extern Delta g_drawing_origin;
// This will begin at the drawing origin above and go down to
// the opposite corner of the drawing area (but not always to the
// edge of the screen, for similiar reasons).
extern Rect g_drawing_region;

// This will be the drawing region with each axis scaled down by
// the resolution scale factor.
Delta logical_screen_pixel_dimensions();

Delta screen_size_tiles();

Delta main_window_size();

void find_pixel_scale_factor();

Delta screen_logical_size();
// Same but with origin at 0,0
Rect screen_logical_rect();

// At standard zoom, when tile size is (g_tile_width,
// g_tile_height), i.e., these are fixed and do not depend on any
// viewport state.
Delta viewport_size_pixels();

} // namespace rn
