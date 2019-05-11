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
#include "aliases.hpp"
#include "cache.hpp"
#include "coord.hpp"
#include "errors.hpp"
#include "sdl-util.hpp" // TODO: get rid of this here

namespace rn {

extern ::SDL_Window*   g_window;
extern ::SDL_Renderer* g_renderer;
extern Texture         g_texture_viewport;

extern Scale g_resolution_scale_factor;

Delta main_window_logical_size();
Rect  main_window_logical_rect();
Delta main_window_physical_size();
Rect  main_window_physical_rect(); // origin at (0,0)

void on_main_window_resized();

void inc_resolution_scale();
void dec_resolution_scale();
void set_optimal_resolution_scale();

void on_renderer_scale_factor_changed();

struct DisplayMode {
  Delta  size;
  Uint32 format;
  int    refresh_rate;
};

DisplayMode current_display_mode();

// Shouldn't ever really need this except in special circum-
// stances.
Delta screen_logical_size();
Delta screen_physical_size();

void    hide_window();
ND bool is_window_fullscreen();
void    set_fullscreen( bool fullscreen );
// Returns true if the window is now fullscreen.
bool toggle_fullscreen();
void restore_window();

// At standard zoom, when tile size is (g_tile_width,
// g_tile_height), i.e., these are fixed and do not depend on any
// viewport state.
Delta viewport_size_pixels();

} // namespace rn
