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
#include "coord.hpp"
#include "sdl-util.hpp" // TODO: get rid of this here

namespace rn {

extern ::SDL_Window*   g_window;
extern ::SDL_Renderer* g_renderer;
extern Texture         g_texture_viewport;

extern Scale g_resolution_scale_factor;

Delta main_window_size();

Delta screen_logical_size();
Rect  screen_logical_rect();
Delta screen_physical_size();
Rect  screen_physical_rect();

// At standard zoom, when tile size is (g_tile_width,
// g_tile_height), i.e., these are fixed and do not depend on any
// viewport state.
Delta viewport_size_pixels();

} // namespace rn
