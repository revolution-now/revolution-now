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
#include "init.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "tiles.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/rn.rds.hpp"

// render
#include "render/renderer.hpp" // TODO: replace with IRenderer

// video
#include "video/video_sdl.hpp"

// refl
#include "refl/to-str.hpp"

// gfx
#include "gfx/logical.hpp"
#include "gfx/resolution.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"

// C++ standard library
#include <cmath>

using namespace std;

namespace rl = ::base::rl;

namespace rn {

namespace {

using ::gfx::e_resolution;

vid::VideoSDL g_video;
vid::WindowHandle g_window;

gfx::Resolutions& g_resolutions() {
  static gfx::Resolutions r = [] {
    UNWRAP_CHECK_T( auto const display_mode,
                    g_video.display_mode() );
    gfx::size const physical_screen = display_mode.size;
    gfx::Monitor const monitor =
        monitor_properties( physical_screen, monitor_dpi() );
    return compute_resolutions( monitor,
                                main_window_physical_size() );
  }();
  return r;
}

// The purpose of this cache is so that we can have a consistent
// physical window size to report across a single frame. If the
// size is changed, the processing of that event will be deferred
// until the start of the next frame at which point this cache
// will be updated.
//
// Cache is invalidated by setting to nothing.
maybe<gfx::size> main_window_physical_size_cache;

void init_screen() {
  UNWRAP_CHECK_T( vid::DisplayMode const display_mode,
                  g_video.display_mode() );
  vid::WindowOptions options{
    .size = display_mode.size,
    .start_fullscreen =
        config_gfx.program_window.start_in_fullscreen };
  UNWRAP_CHECK_T( g_window, g_video.create_window( options ) );
}

void cleanup_screen() { g_video.destroy_window( g_window ); }

REGISTER_INIT_ROUTINE( screen );

// This is the logical resolution given to the renderer when we
// cannot find a supported resolution for the given window. This
// should not be used by actual game code; when no supported res-
// olution is found, there should be no rendering apart from a
// message telling the user to resize their window.
gfx::size logical_resolution_for_invalid_window_size() {
  return main_window_physical_size();
}

void on_logical_resolution_changed(
    rr::Renderer& renderer,
    gfx::Resolutions const& new_resolutions ) {
  if( new_resolutions.selected.available ) {
    auto const old_selected_resolution =
        g_resolutions().selected.rated.resolution;
    auto const& new_selected_resolution =
        new_resolutions.selected.rated.resolution;
    if( new_selected_resolution.logical !=
        old_selected_resolution.logical )
      lg.info( "logical resolution changing to {}x{}",
               new_selected_resolution.logical.w,
               new_selected_resolution.logical.h );
    if( g_resolutions().selected.available )
      input::update_mouse_pos_with_viewport_change(
          old_selected_resolution,
          new_resolutions.selected.rated.resolution );
  } else {
    lg.info( "no logical resolution found." );
  }
  g_resolutions()      = new_resolutions;
  auto const& selected = g_resolutions().selected;
  gfx::rect const viewport =
      selected.available
          ? selected.rated.resolution.viewport
          : gfx::rect{ .size = main_window_physical_size() };
  // Note this actually uses flipped coordinates where the origin
  // as at the lower left, but this still works.
  renderer.set_viewport( viewport );
  renderer.set_logical_screen_size( main_window_logical_size() );
}

gfx::size shrinkage_size() {
  auto const& resolution =
      g_resolutions().selected.rated.resolution;
  gfx::size const target_size =
      resolution.logical * resolution.scale;
  return target_size;
}

void set_fullscreen( bool fullscreen ) {
  g_video.set_fullscreen( g_window, fullscreen );
}

void set_pending_resolution(
    gfx::SelectedResolution const& selected_resolution ) {
  input::inject_resolution_event( selected_resolution );
}

} // namespace

maybe<gfx::MonitorDpi> monitor_dpi() {
  static auto const dpi = []() -> maybe<gfx::MonitorDpi> {
    // This will do a best-effort attempt at providing DPI info
    // from the underlying windowing API, and might return only
    // partial results, and/or incorrect results. So after we get
    // it we post-process it to try as best as possible to to de-
    // rive as many DPI components as possible what it gives us.
    auto const dpi = g_video.display_dpi();
    if( !dpi.has_value() ) {
      lg.warn( "could not get display dpi: {}", dpi.error() );
      return nothing;
    }
    double hdpi = dpi->horizontal;
    double vdpi = dpi->vertical;
    double ddpi = dpi->diagonal;
    lg.info(
        "monitor DPI: horizontal={}, vertical={}, diagonal={}",
        hdpi, vdpi, ddpi );
    if( hdpi != vdpi )
      lg.warn( "horizontal DPI not equal to vertical DPI.", hdpi,
               vdpi );

    // The above function may not provide all of the components,
    // and/or it may return inconsistent or invalid components.
    // Thus, we now do our best to reconstruct any missing or in-
    // valid ones if we have enough information.

    static auto good = []( double const d ) {
      // Any sensible DPI should surely be larger than 1 pixel
      // per inch. If not then it is probably invalid.
      return !isnan( d ) && d > 1.0;
    };
    static auto bad = []( double const d ) {
      return !good( d );
    };

    // Geometrically, the diagonal DPI should always be >= to the
    // cardinal DPIs. If it is not then one of the components may
    // not be reliable. In those cases that were observed, it was
    // the case that the diagonal one was the accurate one, so
    // let's nuke the cardinal one and hope that we'll be able to
    // recompute it below.
    if( bad( ddpi ) ) {
      lg.warn( "diagonal DPI is not available from hardware." );
    } else if( good( hdpi ) && hdpi <= ddpi ) {
      lg.warn(
          "horizontal DPI is less than diagonal DPI, thus one "
          "of them is inaccurate and must be discarded.  Will "
          "assume that the diagonal one is accurate." );
      hdpi = 0.0;
    } else if( good( vdpi ) && vdpi <= ddpi ) {
      lg.warn(
          "vertical DPI is less than diagonal DPI, thus one of "
          "them is inaccurate and must be discarded.  Will "
          "assume that the diagonal one is accurate." );
      vdpi = 0.0;
    }

    if( good( hdpi ) && bad( vdpi ) ) vdpi = hdpi;
    if( bad( hdpi ) && good( vdpi ) ) hdpi = vdpi;
    // At this point hdpi/vdpi are either both good or both bad.
    CHECK_EQ( good( hdpi ), good( vdpi ) );

    bool const have_cardinal = good( hdpi );
    bool const have_diagonal = good( ddpi );

    if( !have_cardinal )
      lg.warn(
          "neither horizontal nor vertical DPIs are "
          "available." );

    if( !have_cardinal && !have_diagonal ) {
      // Can't do anything here;
      lg.warn( "no DPI information can be obtained." );
      return nothing;
    }

    if( have_cardinal && have_diagonal )
      return gfx::MonitorDpi{
        .horizontal = hdpi, .vertical = vdpi, .diagonal = ddpi };

    if( !have_cardinal && have_diagonal ) {
      lg.warn(
          "cardinal DPI must be inferred from diagonal DPI." );
      // Assume a square; won't be perfect, but good enough.
      return gfx::MonitorDpi{ .horizontal = ddpi / 1.41421,
                              .vertical   = vdpi / 1.41421,
                              .diagonal   = ddpi };
    }

    if( have_cardinal && !have_diagonal ) {
      lg.warn(
          "diagonal DPI must be inferred from cardinal DPI." );
      return gfx::MonitorDpi{
        .horizontal = hdpi,
        .vertical   = vdpi,
        .diagonal   = sqrt( hdpi * hdpi + vdpi * vdpi ) };
    }

    lg.error( "something went wrong while computing DPI." );
    return nothing;
  }();
  return dpi;
}

void* main_os_window_handle() { return g_window.handle; }

maybe<gfx::Resolution const&> get_global_resolution() {
  auto const& selected = g_resolutions().selected;
  if( !selected.available ) return nothing;
  return selected.rated.resolution;
}

maybe<gfx::ResolutionScores const&>
get_global_resolution_scores() {
  auto const& selected = g_resolutions().selected;
  if( !selected.available ) return nothing;
  return selected.rated.scores;
}

gfx::size main_window_logical_size() {
  auto const& selected = g_resolutions().selected;
  return selected.available
             ? selected.rated.resolution.logical
             : logical_resolution_for_invalid_window_size();
}

gfx::rect main_window_logical_rect() {
  return gfx::rect{ .size = main_window_logical_size() };
}

maybe<e_resolution> main_window_named_logical_resolution() {
  return g_resolutions().selected.named;
}

gfx::size main_window_physical_size() {
  if( !main_window_physical_size_cache )
    main_window_physical_size_cache =
        g_video.window_size( g_window );
  return *main_window_physical_size_cache;
}

void hide_window() { g_video.hide_window( g_window ); }

bool is_window_fullscreen() {
  return g_video.is_window_fullscreen( g_window );
}

bool toggle_fullscreen() {
  bool const old_fullscreen = is_window_fullscreen();
  bool const new_fullscreen = !old_fullscreen;
  set_fullscreen( new_fullscreen );
  return new_fullscreen;
}

void restore_window() { g_video.restore_window( g_window ); }

bool can_shrink_window_to_fit() {
  if( is_window_fullscreen() ) return false;
  gfx::size const curr_size = g_video.window_size( g_window );
  return curr_size != shrinkage_size();
}

void shrink_window_to_fit() {
  if( !can_shrink_window_to_fit() ) {
    lg.warn( "cannot adjust window size." );
    return;
  }
  // In case the window is maximized this must be done first.
  restore_window();
  gfx::size const size = shrinkage_size();
  g_video.set_window_size( g_window, size );
}

void on_logical_resolution_changed(
    rr::Renderer& renderer,
    gfx::SelectedResolution const& selected_resolution ) {
  auto new_resolutions     = g_resolutions();
  new_resolutions.selected = selected_resolution;
  on_logical_resolution_changed( renderer, new_resolutions );
}

void on_main_window_resized( rr::Renderer& renderer ) {
  // Invalidate cache.
  main_window_physical_size_cache = nothing;
  gfx::size const physical_size   = main_window_physical_size();
  lg.debug( "main window resizing to {}", physical_size );
  gfx::Monitor const monitor =
      monitor_properties( physical_size, monitor_dpi() );
  auto const new_resolutions =
      compute_resolutions( monitor, physical_size );
  // FIXME: the logical resolution here can jump abruptly if the
  // user has previously cycled through the resolutions. Perhaps
  // the way to solve this is to get all of the resolutions here
  // and compare if the new available list is the same as the old
  // (except for the physical size, which must be removed from
  // the data structure before comparison) and, if it is, just
  // keep it and the idx constant.
  on_logical_resolution_changed( renderer, new_resolutions );
}

void cycle_resolution( int const delta ) {
  // Copy; cannot modify the global state directly.
  auto const& curr = g_resolutions();
  if( !curr.selected.available ) return;
  auto const& available = curr.ratings.available;
  if( available.empty() ) return;
  int idx = curr.selected.idx;
  // The "better" resolutions, which also tend to be more scaled
  // up (though not always) are at the start of the list, so for
  // "scaling up" we must go negative.
  idx += ( -delta );
  // Need to do this because the c++ modulus is the wrong type.
  while( idx < 0 ) idx += available.size();
  idx %= available.size();
  CHECK_LT( idx, ssize( available ) );
  set_pending_resolution( create_selected_available_resolution(
      curr.ratings, idx ) );
}

void set_resolution_idx_to_optimal() {
  // Copy; cannot modify the global state directly.
  auto const& curr = g_resolutions();

  if( curr.ratings.available.empty() ) return;
  set_pending_resolution( create_selected_available_resolution(
      curr.ratings, /*idx=*/0 ) );
}

maybe<int> get_resolution_idx() {
  if( !g_resolutions().selected.available ) return nothing;
  return g_resolutions().selected.idx;
}

maybe<int> get_resolution_cycle_size() {
  if( !g_resolutions().selected.available ) return nothing;
  return g_resolutions().ratings.available.size();
}

} // namespace rn
