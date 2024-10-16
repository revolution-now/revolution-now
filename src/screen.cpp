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
#include "logger.hpp"
#include "menu.hpp"
#include "renderer.hpp" // FIXME: remove
#include "resolution.hpp"
#include "sdl.hpp"
#include "tiles.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/rn.rds.hpp"

// render
#include "render/renderer.hpp" // TODO: replace with IRenderer

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"

// C++ standard library
#include <cmath>

using namespace std;

namespace rl = ::base::rl;

namespace rn {

namespace {

gfx::Resolution g_resolution;

::SDL_Window* g_window = nullptr;

Delta g_screen_physical_size{};

auto g_pixel_format = ::SDL_PIXELFORMAT_RGBA8888;

// Cache is invalidated by setting to nothing.
maybe<Delta> main_window_physical_size_cache;

double monitor_diagonal_length( double ddpi, DisplayMode dm ) {
  double length =
      sqrt( pow( dm.size.w, 2.0 ) + pow( dm.size.h, 2.0 ) ) /
      ddpi;
  // Round to hearest 1/2 inch.
  return double( lround( length * 2.0 ) ) / 2.0;
}

// Get diagonal DPI of monitor. This seems to basically be the
// same as the horizontal and vertical DPIs.
double monitor_ddpi() {
  static float ddpi = [] {
    float res = 0.0; // diagonal DPI.
    bool  success =
        !::SDL_GetDisplayDPI( 0, &res, nullptr, nullptr );
    if( !success ) {
      lg.error( "could not get display dpi." );
      res = 145.0;
    }
    return res;
  }();
  return ddpi;
}

double monitor_inches() {
  return monitor_diagonal_length( monitor_ddpi(),
                                  current_display_mode() );
}

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

  lg.debug( "monitor diagonal length: {}in.", monitor_inches() );
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

  auto dm = current_display_mode().size;

  g_window =
      ::SDL_CreateWindow( config_rn.main_window.title.c_str(), 0,
                          0, dm.w, dm.h, flags );
  CHECK( g_window != nullptr, "failed to create window" );
}

void cleanup_screen() {
  if( g_window != nullptr ) SDL_DestroyWindow( g_window );
}

REGISTER_INIT_ROUTINE( screen );

void set_resolution( rr::Renderer&          renderer,
                     gfx::Resolution const& resolution ) {
  if( resolution.logical != g_resolution.logical )
    lg.info( "logical resolution changing to {}",
             resolution.logical );

  g_resolution = resolution;

  gfx::size const logical_in_physical_pixels =
      resolution.logical * resolution.scale;
  gfx::rect const physical_rect{ .origin = {},
                                 .size   = resolution.physical };
  auto const      viewport_origin = gfx::centered_in(
      logical_in_physical_pixels, physical_rect );
  gfx::rect const viewport_rect{
    .origin = viewport_origin,
    .size   = logical_in_physical_pixels };
  // TODO: this actually uses flipped coordinates; perhaps we
  // should indicate that.
  renderer.set_viewport( viewport_rect );
  renderer.set_logical_screen_size( resolution.logical );
}

} // namespace

void* main_os_window_handle() { return (void*)g_window; }

gfx::Resolution const& get_resolution() { return g_resolution; }

DisplayMode current_display_mode() {
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode( 0, &dm );
  DisplayMode res{
    { W{ dm.w }, H{ dm.h } }, dm.format, dm.refresh_rate };
  return res;
}

Delta whole_screen_physical_size() {
  return g_screen_physical_size;
}

Delta main_window_logical_size() {
  return Delta::from_gfx( g_resolution.logical );
}

Rect main_window_logical_rect() {
  return Rect::from_gfx(
      gfx::rect{ .origin = {}, .size = g_resolution.logical } );
}

int resolution_scale_factor() { return g_resolution.scale; }

Delta main_window_physical_size() {
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

bool toggle_fullscreen() {
  auto fullscreen = is_window_fullscreen();
  set_fullscreen( !fullscreen );
  // FIXME: we shouldn't need to call this here because ideally
  // when we exit full screen mode it should send an input event
  // through SDL; not sure why that is not happening. If it is
  // not happening then perhaps we should inject one ourselves.
  on_main_window_resized(
      // FIXME
      global_renderer_use_only_when_needed() );
  return !fullscreen;
}

void restore_window() { ::SDL_RestoreWindow( g_window ); }

void on_main_window_resized( rr::Renderer& renderer ) {
  // Invalidate cache.
  main_window_physical_size_cache = nothing;
  gfx::size const physical_size   = main_window_physical_size();
  lg.debug( "main window resizing to {}", physical_size );
  auto const recommended =
      recompute_best_logical_resolution( physical_size );
  if( recommended.has_value() )
    set_resolution( renderer, *recommended );
  else {
    // TODO: deal with this case. We should probably create a
    // Resolution object that keeps the current logical resolu-
    // tion but centers it properly in the current window as a
    // best effort attempt to keep things looking decent. We may
    // also want the omni layer to overlay a message that says
    // "window too small".
  }
}

} // namespace rn
