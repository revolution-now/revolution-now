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
#include "sdl.hpp"
#include "tiles.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/rn.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"

// C++ standard library
#include <cmath>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

Delta g_resolution_scale_factor{ .w = 0, .h = 0 };

namespace {

::SDL_Window* g_window = nullptr;

Delta g_optimal_resolution_scale_factor{ .w = 0, .h = 0 };
Delta g_screen_physical_size{};

auto g_pixel_format = ::SDL_PIXELFORMAT_RGBA8888;

constexpr int min_scale_factor = 1;
constexpr int max_scale_factor = 10;

// Cache is invalidated by setting to nothing.
maybe<Delta> main_window_physical_size_cache;

/*
 *::SDL_DisplayMode find_fullscreen_mode() {
 *  ::SDL_DisplayMode dm;
 *  lg.debug( "available display modes:" );
 *  auto num_display_modes = ::SDL_GetNumDisplayModes( 0 );
 *  constexpr int min_x_res{1920};
 *  constexpr int min_y_res{1080};
 *  for( int i = 0; i < num_display_modes; ++i ) {
 *    ::SDL_GetDisplayMode( 0, i, &dm );
 *    if( dm.w % g_tile_width._ == 0 &&
 *        dm.h % g_tile_height._ == 0 ) {
 *      lg.debug( "{}x{}", dm.w, dm.h );
 *      if( dm.w >= min_x_res && dm.h >= min_y_res ) return dm;
 *    }
 *  }
 *  dm.w = dm.h = 0; // means we can't find one.
 *  return dm;
 *}
 */

// Do not use this from outside this module: it refers to the en-
// tire screen, even if the game is in an unmaximized window. In
// the vast majority of cases you want to use
// main_window_logical_size.
Delta screen_logical_size() {
  return g_screen_physical_size / g_resolution_scale_factor;
}

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
#if 0
  ScaleInfo optimal = scale_info( min_scale_factor );
  (void)&scale_score;
#else
  UNWRAP_CHECK(
      optimal, rl::ints( min_scale_factor, max_scale_factor + 1 )
                   .map( scale_info )
                   .min_by( scale_score ) );
#endif

  ///////////////////////////////////////////////////////////////
#if 0
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
#endif
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

void init_screen() {
  query_video_stats();
  find_pixel_scale_factor();

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

// MENU_ITEM_HANDLER(
//     scale_up, [] { inc_resolution_scale(); },
//     L0( g_resolution_scale_factor !=
//         Scale{ max_scale_factor } ) )
//
// MENU_ITEM_HANDLER(
//     scale_down, [] { dec_resolution_scale(); },
//     L0( g_resolution_scale_factor !=
//         Scale{ min_scale_factor } ) )
//
// MENU_ITEM_HANDLER(
//     scale_optimal, [] { set_optimal_resolution_scale(); },
//     L0( g_resolution_scale_factor !=
//         g_optimal_resolution_scale_factor ) )
//
// MENU_ITEM_HANDLER(
//     toggle_fullscreen,
//     [] {
//       auto is_fullscreen = toggle_fullscreen();
//       if( !is_fullscreen ) restore_window();
//     },
//     L0( true ) )
//
// MENU_ITEM_HANDLER(
//     restore_window,
//     [] {
//       if( is_window_fullscreen() ) {
//         toggle_fullscreen();
//         restore_window();
//       } else {
//         restore_window();
//       }
//     },
//     L0( true ) )

void on_logical_resolution_changed() {
  // Invalidate cache.
  main_window_physical_size_cache = nothing;

  auto logical_size = main_window_logical_size();
  lg.debug( "logical resolution changed to {}", logical_size );

  auto physical_size = main_window_physical_size();
  if( physical_size % g_resolution_scale_factor != Delta{} )
    lg.warn(
        "main window physical resolution not commensurate with "
        "scale factor." );
}

void on_renderer_scale_factor_changed() {
  lg.info( "scale factor changed: {}",
           g_resolution_scale_factor );
  on_logical_resolution_changed();
}

REGISTER_INIT_ROUTINE( screen );

} // namespace

void* main_os_window_handle() { return (void*)g_window; }

void set_resolution_scale( int const new_scale ) {
  int scale     = g_resolution_scale_factor.w;
  int old_scale = scale;
  scale         = new_scale;
  scale = clamp( scale, min_scale_factor, max_scale_factor );
  g_resolution_scale_factor.w = scale;
  g_resolution_scale_factor.h = scale;
  if( old_scale != scale ) on_renderer_scale_factor_changed();
}

void inc_resolution_scale() {
  int const curr_scale = g_resolution_scale_factor.w;
  set_resolution_scale( curr_scale + 1 );
}

void dec_resolution_scale() {
  int const curr_scale = g_resolution_scale_factor.w;
  set_resolution_scale( curr_scale - 1 );
}

void set_optimal_resolution_scale() {
  if( g_resolution_scale_factor !=
      g_optimal_resolution_scale_factor ) {
    g_resolution_scale_factor =
        g_optimal_resolution_scale_factor;
    on_renderer_scale_factor_changed();
  }
}

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
  return main_window_physical_size() / g_resolution_scale_factor;
}

Rect main_window_logical_rect() {
  return main_window_physical_rect() / g_resolution_scale_factor;
}

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

Rect main_window_physical_rect() {
  return Rect::from( Coord{}, main_window_physical_size() );
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
  return !fullscreen;
}

void restore_window() { ::SDL_RestoreWindow( g_window ); }

void on_main_window_resized() {
  lg.debug( "main window resizing." );
  on_logical_resolution_changed();
}

} // namespace rn
