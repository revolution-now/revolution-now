/****************************************************************
**screen.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-18.
*
* Description: Handles screen resolution and scaling.
*
*****************************************************************/
#include "screen.hpp"

// Revolution Now
#include "error.hpp"
#include "iengine.hpp"
#include "input.hpp"
#include "tiles.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/rn.rds.hpp"

// render
#include "render/renderer.hpp" // TODO: replace with IRenderer

// video
#include "video/video-sdl.hpp" // FIXME: remove
#include "video/window.hpp"

// refl
#include "refl/to-str.hpp"

// gfx
#include "gfx/logical.hpp"
#include "gfx/monitor.hpp"
#include "gfx/resolution.hpp"

// base
#include "base/lambda.hpp"
#include "base/logger.hpp"
#include "base/range-lite.hpp"

// C++ standard library
#include <cmath>

using namespace std;

namespace rl = ::base::rl;

namespace rn {

namespace {

using ::gfx::e_resolution;

// This is the logical resolution given to the renderer when we
// cannot find a supported resolution for the given window. This
// should not be used by actual game code; when no supported res-
// olution is found, there should be no rendering apart from a
// message telling the user to resize their window.
gfx::size logical_resolution_for_invalid_window_size(
    vid::IVideo& video, vid::WindowHandle const& wh ) {
  return main_window_physical_size( video, wh );
}

void on_logical_resolution_changed(
    vid::IVideo& video, vid::WindowHandle const& wh,
    rr::Renderer& renderer, gfx::Resolutions& actual_resolutions,
    gfx::Resolutions const& new_resolutions ) {
  if( new_resolutions.selected.available ) {
    auto const old_selected_resolution =
        actual_resolutions.selected.rated.resolution;
    auto const& new_selected_resolution =
        new_resolutions.selected.rated.resolution;
    if( new_selected_resolution.logical !=
        old_selected_resolution.logical )
      lg.info( "logical resolution changing to {}x{}",
               new_selected_resolution.logical.w,
               new_selected_resolution.logical.h );
    if( actual_resolutions.selected.available )
      input::update_mouse_pos_with_viewport_change(
          old_selected_resolution,
          new_resolutions.selected.rated.resolution );
  } else {
    lg.info( "no logical resolution found." );
  }
  actual_resolutions   = new_resolutions;
  auto const& selected = actual_resolutions.selected;
  gfx::rect const viewport =
      selected.available
          ? selected.rated.resolution.viewport
          : gfx::rect{
              .size = main_window_physical_size( video, wh ) };
  // Note this actually uses flipped coordinates where the origin
  // as at the lower left, but this still works.
  renderer.set_viewport( viewport );
  renderer.set_logical_screen_size(
      main_window_logical_size( video, wh, new_resolutions ) );
}

} // namespace

// This will do a best-effort attempt at providing DPI info from
// the underlying windowing API, and might return only partial
// results, and/or incorrect results. So after we get it we
// post-process it to try as best as possible to to derive as
// many DPI components as possible what it gives us.
maybe<gfx::MonitorDpi> monitor_dpi( vid::IVideo& video ) {
  static auto const dpi = [&]() -> maybe<gfx::MonitorDpi> {
    auto dpi = video.display_dpi();
    if( !dpi.has_value() ) {
      lg.warn( "could not get display dpi: {}", dpi.error() );
      return nothing;
    }
    gfx::ProcessedMonitorDpi const processed =
        gfx::post_process_monitor_dpi( *dpi );
    return processed.dpi;
  }();
  return dpi;
}

maybe<gfx::Resolution const&> get_resolution( IEngine& engine ) {
  auto const& selected = engine.resolutions().selected;
  if( !selected.available ) return nothing;
  return selected.rated.resolution;
}

gfx::size main_window_logical_size(
    vid::IVideo& video, vid::WindowHandle const& wh,
    gfx::Resolutions const& resolutions ) {
  auto const& selected = resolutions.selected;
  return selected.available
             ? selected.rated.resolution.logical
             : logical_resolution_for_invalid_window_size( video,
                                                           wh );
}

gfx::rect main_window_logical_rect(
    vid::IVideo& video, vid::WindowHandle const& wh,
    gfx::Resolutions const& resolutions ) {
  return gfx::rect{
    .size = main_window_logical_size( video, wh, resolutions ) };
}

void on_logical_resolution_changed(
    vid::IVideo& video, vid::WindowHandle const& wh,
    rr::Renderer& renderer, gfx::Resolutions& resolutions,
    gfx::SelectedResolution const& selected_resolution ) {
  auto new_resolutions     = resolutions;
  new_resolutions.selected = selected_resolution;
  on_logical_resolution_changed( video, wh, renderer,
                                 resolutions, new_resolutions );
}

void on_main_window_resized( vid::IVideo& video,
                             vid::WindowHandle const& wh,
                             gfx::Resolutions& resolutions,
                             rr::Renderer& renderer ) {
  // Invalidate cache.
  vid::reset_window_size_cache();
  gfx::size const physical_size =
      main_window_physical_size( video, wh );
  lg.debug( "main window resizing to {}", physical_size );
  gfx::Monitor const monitor =
      monitor_properties( physical_size, monitor_dpi( video ) );
  auto const new_resolutions =
      compute_resolutions( monitor, physical_size );
  // FIXME: the logical resolution here can jump abruptly if the
  // user has previously cycled through the resolutions. Perhaps
  // the way to solve this is to get all of the resolutions here
  // and compare if the new available list is the same as the old
  // (except for the physical size, which must be removed from
  // the data structure before comparison) and, if it is, just
  // keep it and the idx constant.
  on_logical_resolution_changed( video, wh, renderer,
                                 resolutions, new_resolutions );
}

} // namespace rn
