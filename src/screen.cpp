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
#include "logger.hpp"
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

void set_pending_resolution(
    gfx::SelectedResolution const& selected_resolution ) {
  input::inject_resolution_event( selected_resolution );
}

} // namespace

maybe<gfx::MonitorDpi> monitor_dpi( vid::IVideo& video ) {
  static auto const dpi = [&]() -> maybe<gfx::MonitorDpi> {
    // This will do a best-effort attempt at providing DPI info
    // from the underlying windowing API, and might return only
    // partial results, and/or incorrect results. So after we get
    // it we post-process it to try as best as possible to to de-
    // rive as many DPI components as possible what it gives us.
    auto const dpi = video.display_dpi();
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

maybe<gfx::Resolution const&> get_global_resolution(
    IEngine& engine ) {
  auto const& selected = engine.resolutions().selected;
  if( !selected.available ) return nothing;
  return selected.rated.resolution;
}

maybe<gfx::ResolutionScores const&> get_global_resolution_scores(
    IEngine& engine ) {
  auto const& resolutions = engine.resolutions();
  auto const& selected    = resolutions.selected;
  if( !selected.available ) return nothing;
  return selected.rated.scores;
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

maybe<e_resolution> main_window_named_logical_resolution(
    gfx::Resolutions const& resolutions ) {
  return resolutions.selected.named;
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

void cycle_resolution( gfx::Resolutions const& resolutions,
                       int const delta ) {
  // Copy; cannot modify the global state directly.
  auto const& curr = resolutions;
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

void set_resolution_idx_to_optimal(
    gfx::Resolutions const& resolutions ) {
  auto const& curr = resolutions;
  if( curr.ratings.available.empty() ) return;
  set_pending_resolution( create_selected_available_resolution(
      curr.ratings, /*idx=*/0 ) );
}

maybe<int> get_resolution_idx(
    gfx::Resolutions const& resolutions ) {
  if( !resolutions.selected.available ) return nothing;
  return resolutions.selected.idx;
}

maybe<int> get_resolution_cycle_size(
    gfx::Resolutions const& resolutions ) {
  if( !resolutions.selected.available ) return nothing;
  return resolutions.ratings.available.size();
}

} // namespace rn
