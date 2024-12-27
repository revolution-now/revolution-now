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
#include "sdl-util.hpp"
#include "sdl.hpp"
#include "tiles.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/rn.rds.hpp"

// render
#include "render/renderer.hpp" // TODO: replace with IRenderer

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

struct DisplayMode {
  gfx::size size   = {};
  uint32_t format  = 0;
  int refresh_rate = 0;
};

DisplayMode current_display_mode() {
  SDL_DisplayMode dm;
  if( ::SDL_GetCurrentDisplayMode( 0, &dm ) < 0 ) {
    FATAL( "failed to get display mode info: {}",
           sdl_get_last_error() );
  }
  return DisplayMode{ .size         = { .w = dm.w, .h = dm.h },
                      .format       = dm.format,
                      .refresh_rate = dm.refresh_rate };
}

gfx::Resolutions& g_resolutions() {
  static gfx::Resolutions r = [] {
    auto const display_mode         = current_display_mode();
    gfx::size const physical_screen = display_mode.size;
    gfx::Monitor const monitor =
        monitor_properties( physical_screen, monitor_dpi() );
    return compute_resolutions( monitor,
                                main_window_physical_size() );
  }();
  return r;
}

::SDL_Window* g_window = nullptr;

auto g_pixel_format = ::SDL_PIXELFORMAT_RGBA8888;

// The purpose of this cache is so that we can have a consistent
// physical window size to report across a single frame. If the
// size is changed, the processing of that event will be deferred
// until the start of the next frame at which point this cache
// will be updated.
//
// Cache is invalidated by setting to nothing.
maybe<gfx::size> main_window_physical_size_cache;

#if 0
double const& viewer_distance_from_monitor() {
  static double distance = [] {
    // Determined empirically; viewer distance from screen seems
    // to scale linearly with screen size, down to a certain
    // minimum distance.
    constexpr double viewer_distance_multiplier{ 1.25 };
    constexpr double viewer_distance_minimum{ 18 }; // inches
    auto             res =
        std::max( viewer_distance_multiplier * monitor_inches(),
                  viewer_distance_minimum );
    lg.debug( "computed viewer distance from screen: {}in.",
              res );
    return res;
  }();
  return distance;
};
#endif

void query_video_stats() {
  float ddpi, hdpi, vdpi;
  // A warning from the SDL2 docs:
  //
  //   WARNING: This reports the DPI that the hardware reports,
  //   and it is not always reliable! It is almost always better
  //   to use SDL_GetWindowSize() to find the window size, which
  //   might be in logical points instead of pixels, and then
  //   SDL_GL_GetDrawableSize(), SDL_Vulkan_GetDrawableSize(),
  //   SDL_Metal_GetDrawableSize(), or SDL_Get-Renderer-Output-
  //   Size(), and compare the two values to get an actual
  //   scaling value between the two. We will be rethinking how
  //   high-dpi details should be managed in SDL3 to make things
  //   more consistent, reliable, and clear.
  //
  ::SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi );
  lg.debug( "GetDisplayDPI: {{ddpi={}, hdpi={}, vdpi={}}}.",
            ddpi, hdpi, vdpi );

  SDL_DisplayMode dm;

  auto dm_to_str = [&dm] {
    return fmt::format( "{}x{}@{}Hz, pf={}", dm.w, dm.h,
                        dm.refresh_rate,
                        ::SDL_GetPixelFormatName( dm.format ) );
  };
  (void)dm_to_str;

  lg.debug( "default game pixel format: {}",
            ::SDL_GetPixelFormatName( g_pixel_format ) );

  SDL_GetCurrentDisplayMode( 0, &dm );
  lg.debug( "GetCurrentDisplayMode: {}", dm_to_str() );
  if( g_pixel_format != dm.format ) {
    // g_pixel_format =
    //    static_cast<decltype( g_pixel_format )>( dm.format );
    // lg.debug( "correcting game pixel format to {}",
    //               ::SDL_GetPixelFormatName( dm.format ) );
  }

  SDL_GetDesktopDisplayMode( 0, &dm );
  lg.debug( "GetDesktopDisplayMode: {}", dm_to_str() );

  SDL_GetDisplayMode( 0, 0, &dm );
  lg.debug( "GetDisplayMode: {}", dm_to_str() );

  SDL_Rect r;
  SDL_GetDisplayBounds( 0, &r );
  lg.debug( "GetDisplayBounds: {}",
            Rect{ X{ r.x }, Y{ r.y }, W{ r.w }, H{ r.h } } );
}

#if 0
struct ScaleInfo {
  int    scale;
  double tile_size_on_screen_surface_inches;
  Delta  resolution;
  double tile_angular_size;
};
NOTHROW_MOVE( ScaleInfo );

ScaleInfo scale_info( int scale_ ) {
  Delta scale{ .w = scale_, .h = scale_ };
  Delta resolution = current_display_mode().size / scale;

  // Tile size in inches if it were measured on the surface of
  // the screen.
  double tile_size_screen_surface =
      ( scale * g_tile_delta ).w / monitor_ddpi();

  // Compute the angular size (this is what actually determines
  // how big it looks to the viewer).
  auto theta =
      2.0 * std::atan( ( tile_size_screen_surface / 2.0 ) /
                       viewer_distance_from_monitor() );

  return ScaleInfo{ scale_, tile_size_screen_surface, resolution,
                    theta };
}

// Lower score is better.
double scale_score( ScaleInfo const& info ) {
  return ::abs( info.tile_angular_size -
                config_rn.ideal_tile_angular_size );
}

// This function attempts to find an integer scale factor with
// which to scale the pixel size of the display so that a single
// tile has approximately a given size. The scale factor is an
// integer because we scale both dimensions by the same amount,
// and the scale factor is not a floating point number because we
// don't want any distortion of individual pixels which would
// arise in that situation.
void find_pixel_scale_factor() {
#  if 0
  ScaleInfo optimal = scale_info( min_scale_factor );
  (void)&scale_score;
#  else
  UNWRAP_CHECK(
      optimal, rl::ints( min_scale_factor, max_scale_factor + 1 )
                   .map( scale_info )
                   .min_by( scale_score ) );
#  endif

  ///////////////////////////////////////////////////////////////
#  if 0
  auto table_row = []( auto possibility, auto resolution,
                       auto tile_size_screen, auto tile_size_1ft,
                       auto score ) {
    lg.debug( "{: ^10}{: ^19}{: ^18}{: ^18}{: ^10}", possibility,
              resolution, tile_size_screen, tile_size_1ft,
              score );
  };

  table_row( "Scale", "Resolution", "Tile-Size-Screen",
             "Tile-Angular-Size", "Score" );
  auto bar = string( 86, '-' );
  lg.debug( bar );
  auto fmt_dbl = []( double d ) {
    return fmt::format( "{:.4}", d );
  };
  for( auto const& info : scale_scores ) {
    string chosen =
        ( info.scale == optimal.scale ) ? "=> " : "   ";
    table_row(
        chosen + to_string( info.scale ), info.resolution,
        fmt_dbl( info.tile_size_on_screen_surface_inches ),
        fmt_dbl( info.tile_angular_size ),
        fmt_dbl( scale_score( info ) ) );
  }
  lg.debug( bar );
#  endif
  ///////////////////////////////////////////////////////////////

  g_resolution_scale_factor =
      Delta{ .w = optimal.scale, .h = optimal.scale };
  g_optimal_resolution_scale_factor =
      Delta{ .w = optimal.scale, .h = optimal.scale };
  g_screen_physical_size =
      optimal.resolution * g_resolution_scale_factor;
  lg.info( "screen physical resolution: {}",
           g_screen_physical_size );
  lg.info( "screen logical  resolution: {}",
           screen_logical_size() );

  // If this is violated then we have non-integer scaling.
  CHECK( ( g_screen_physical_size %
           Delta{ .w = optimal.scale, .h = optimal.scale } ) ==
         Delta{} );

  // For informational purposes
  if( current_display_mode().size %
          Delta{ .w = optimal.scale, .h = optimal.scale } !=
      Delta{} )
    lg.warn(
        "Desktop display resolution not commensurate with scale "
        "factor." );
}
#endif

void init_screen() {
  query_video_stats();

  int flags = {};

  bool const start_in_fullscreen =
      config_gfx.program_window.start_in_fullscreen;

  flags |= ::SDL_WINDOW_SHOWN;
  flags |= ::SDL_WINDOW_OPENGL;

  if( start_in_fullscreen )
    flags |= ::SDL_WINDOW_FULLSCREEN_DESKTOP;

  // Not sure why, but this seems to be beneficial even when we
  // are starting in fullscreen desktop mode, since if it is not
  // set (this is on Linux) then when we leave fullscreen mode
  // the "restore" command doesn't work and the window remains
  // maximized.
  flags |= ::SDL_WINDOW_RESIZABLE;

  DisplayMode const dm = current_display_mode();

  g_window =
      ::SDL_CreateWindow( config_rn.main_window.title.c_str(), 0,
                          0, dm.size.w, dm.size.h, flags );
  CHECK( g_window != nullptr, "failed to create window" );
}

void cleanup_screen() {
  if( g_window != nullptr ) SDL_DestroyWindow( g_window );
}

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
  if( fullscreen == is_window_fullscreen() ) return;

  if( fullscreen ) {
    ::SDL_SetWindowFullscreen( g_window,
                               ::SDL_WINDOW_FULLSCREEN_DESKTOP );
  } else {
    ::SDL_SetWindowFullscreen( g_window, 0 );
    // This somehow gets erased when we go to fullscreen mode, so
    // it needs to be re-set each time.
    ::SDL_SetWindowResizable( g_window,
                              /*resizable=*/::SDL_TRUE );
  }
}

void set_pending_resolution(
    gfx::SelectedResolution const& selected_resolution ) {
  input::inject_resolution_event( selected_resolution );
}

} // namespace

maybe<gfx::MonitorDpi> monitor_dpi() {
  static maybe<gfx::MonitorDpi> const dpi =
      []() -> maybe<gfx::MonitorDpi> {
    float hdpi = 0.0; // horizontal dpi.
    float vdpi = 0.0; // vertical dpi.
    float ddpi = 0.0; // diagonal dpi.
    // A warning from the SDL2 docs:
    //
    //   WARNING: This reports the DPI that the hardware reports,
    //   and it is not always reliable! It is almost always
    //   better to use SDL_GetWindowSize() to find the window
    //   size, which might be in logical points instead of pix-
    //   els, and then SDL_GL_GetDrawableSize(), SDL_Vulkan_-Get-
    //   DrawableSize(), SDL_Metal_GetDrawableSize(), or SDL_Get-
    //   RendererOutputSize(), and compare the two values to get
    //   an actual scaling value between the two. We will be re-
    //   thinking how high-dpi details should be managed in SDL3
    //   to make things more consistent, reliable, and clear.
    //
    if( ::SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi ) < 0 ) {
      lg.warn( "could not get display dpi: {}",
               sdl_get_last_error() );
      return nothing;
    }
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

void* main_os_window_handle() { return (void*)g_window; }

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
  if( !main_window_physical_size_cache ) {
    CHECK( g_window != nullptr );
    int w{}, h{};
    ::SDL_GetWindowSize( g_window, &w, &h );
    main_window_physical_size_cache =
        Delta{ .w = W{ w }, .h = H{ h } };
  }
  return *main_window_physical_size_cache;
}

void hide_window() {
  if( g_window ) ::SDL_HideWindow( g_window );
}

// TODO: mac-os, does not seem to be able to detect when the user
// fullscreens a window.
bool is_window_fullscreen() {
  // This bit should always be set even if we're in the "desktop"
  // fullscreen mode.
  return ( ::SDL_GetWindowFlags( g_window ) &
           ::SDL_WINDOW_FULLSCREEN ) != 0;
}

bool toggle_fullscreen() {
  bool const old_fullscreen = is_window_fullscreen();
  bool const new_fullscreen = !old_fullscreen;
  set_fullscreen( new_fullscreen );
  return new_fullscreen;
}

void restore_window() { ::SDL_RestoreWindow( g_window ); }

bool can_shrink_window_to_fit() {
  if( is_window_fullscreen() ) return false;
  gfx::size const curr_size = [] {
    gfx::size res;
    ::SDL_GetWindowSize( g_window, &res.w, &res.h );
    return res;
  }();
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
  ::SDL_SetWindowSize( g_window, size.w, size.h );
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
