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
#include "error.hpp"
#include "tx.hpp"

struct SDL_Renderer;

namespace rn {

extern ::SDL_Renderer* g_renderer;
extern Texture         g_texture_viewport;

extern Scale g_resolution_scale_factor;

// These are cheap to call because their values are cached and
// are only updated when either the main window is resized or if
// the user changes the scale factor.
//
// NOTE: you should not normally call this in most game code, in-
// stead you should go through the compositor in order to allow
// it to control the logical size of the screen seen by the rest
// of the game.
Delta main_window_logical_size();
Delta main_window_physical_size();
Rect  main_window_logical_rect();  // !! origin at (0,0)
Rect  main_window_physical_rect(); // !! origin at (0,0)

void on_main_window_resized();

void inc_resolution_scale();
void dec_resolution_scale();
void set_optimal_resolution_scale();

struct DisplayMode {
  Delta    size;
  uint32_t format;
  int      refresh_rate;
};
NOTHROW_MOVE( DisplayMode );

DisplayMode current_display_mode();

// Should not need this often.
Delta whole_screen_physical_size();

Delta max_texture_size();

void    hide_window();
ND bool is_window_fullscreen();
void    set_fullscreen( bool fullscreen );
// Returns true if the window is now fullscreen.
bool toggle_fullscreen();
void restore_window();

void* main_os_window_handle();

} // namespace rn
