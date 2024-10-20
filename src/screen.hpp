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

// Revolution Now
#include "error.hpp"
#include "maybe.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rr {
struct Renderer;
}

namespace gfx {
struct Resolution;
}

namespace rn {

maybe<gfx::Resolution const&> get_global_resolution();

// NOTE: you should not normally call this in most game code, in-
// stead you should go through the compositor in order to allow
// it to control the logical size of each element on the screen.
gfx::size main_window_logical_size();
gfx::size main_window_physical_size();

// !! origin at (0,0)
gfx::rect main_window_logical_rect();

void on_main_window_resized( rr::Renderer& renderer );

void on_logical_resolution_changed(
    rr::Renderer&                 renderer,
    maybe<gfx::Resolution const&> resolution );

void cycle_resolution( int delta );

// Returns true if the window is now fullscreen.
bool toggle_fullscreen();

void* main_os_window_handle();

// FIXME: unused outside this module.
void    hide_window();
ND bool is_window_fullscreen();
void    set_fullscreen( bool fullscreen );
void    restore_window();

} // namespace rn
