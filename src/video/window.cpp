/****************************************************************
**window.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-29.
*
* Description: Helpers for managing the program window.
*
*****************************************************************/
#include "window.hpp"

// video
#include "ivideo.hpp"

// gfx
#include "gfx/logical.rds.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace vid {

namespace {

using ::base::lg;
using ::base::maybe;

// The purpose of this cache is so that we can have a consistent
// physical window size to report across a single frame. If the
// size is changed, the processing of that event will be deferred
// until the start of the next frame at which point this cache
// will be updated.
//
// Cache is invalidated by setting to nothing.
maybe<gfx::size> g_window_size_cache;

gfx::size shrinkage_size( gfx::Resolution const& resolution ) {
  gfx::size const target_size =
      resolution.logical * resolution.scale;
  return target_size;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void reset_window_size_cache() { g_window_size_cache.reset(); }

gfx::size main_window_physical_size( IVideo& video,
                                     WindowHandle const& wh ) {
  if( !g_window_size_cache )
    g_window_size_cache = video.window_size( wh );
  return *g_window_size_cache;
}

void hide_window( IVideo& video, WindowHandle const& wh ) {
  video.hide_window( wh );
}

bool is_window_fullscreen( IVideo& video,
                           WindowHandle const& wh ) {
  return video.is_window_fullscreen( wh );
}

bool toggle_fullscreen( IVideo& video, WindowHandle const& wh ) {
  bool const old_fullscreen = is_window_fullscreen( video, wh );
  bool const new_fullscreen = !old_fullscreen;
  video.set_fullscreen( wh, new_fullscreen );
  return new_fullscreen;
}

void restore_window( IVideo& video, WindowHandle const& wh ) {
  video.restore_window( wh );
}

bool can_shrink_window_to_fit(
    IVideo& video, WindowHandle const& wh,
    gfx::Resolution const& resolution ) {
  if( is_window_fullscreen( video, wh ) ) return false;
  gfx::size const curr_size = video.window_size( wh );
  return curr_size != shrinkage_size( resolution );
}

void shrink_window_to_fit( IVideo& video, WindowHandle const& wh,
                           gfx::Resolution const& resolution ) {
  if( !can_shrink_window_to_fit( video, wh, resolution ) ) {
    lg.warn( "cannot adjust window size." );
    return;
  }
  // In case the window is maximized this must be done first.
  restore_window( video, wh );
  gfx::size const size = shrinkage_size( resolution );
  video.set_window_size( wh, size );
}

} // namespace vid
