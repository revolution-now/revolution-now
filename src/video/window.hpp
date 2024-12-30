/****************************************************************
**window.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-29.
*
* Description: Helpers for managing the program window.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

namespace gfx {
struct Resolution;
}

namespace vid {

struct IVideo;
struct WindowHandle;

gfx::size main_window_physical_size( IVideo& video,
                                     WindowHandle const& wh );

void hide_window( IVideo& video, WindowHandle const& wh );

bool is_window_fullscreen( IVideo& video,
                           WindowHandle const& wh );

bool toggle_fullscreen( IVideo& video, WindowHandle const& wh );

void restore_window( IVideo& video, WindowHandle const& wh );

bool can_shrink_window_to_fit(
    IVideo& video, WindowHandle const& wh,
    gfx::Resolution const& resolution );

void shrink_window_to_fit( IVideo& video, WindowHandle const& wh,
                           gfx::Resolution const& resolution );

// FIXME: should probably get rid of this mechanism.
void reset_window_size_cache();

} // namespace vid
