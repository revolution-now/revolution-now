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
#include "tx.hpp"

// SDL
#include "SDL.h"       // FIXME: get rid of this.
#include "SDL_image.h" // FIXME: get rid of this.

namespace rn {

extern ::SDL_Renderer* g_renderer;
extern Texture         g_texture_viewport;

extern Scale g_resolution_scale_factor;

// These are cheap to call because their values are cached and
// are only updated when either the main window is resized or if
// the user changes the scale factor.
Delta main_window_logical_size();
Delta main_window_physical_size();
Rect  main_window_logical_rect();  // !! origin at (0,0)
Rect  main_window_physical_rect(); // !! origin at (0,0)

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

// Should not need this often.
Delta whole_screen_physical_size();

void    hide_window();
ND bool is_window_fullscreen();
void    set_fullscreen( bool fullscreen );
// Returns true if the window is now fullscreen.
bool toggle_fullscreen();
void restore_window();

} // namespace rn
