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
#include "errors.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "ranges.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"

// C++ standard library
#include <cmath>
#include <optional>

using namespace std;

namespace rn {

::SDL_Window*   g_window   = nullptr;
::SDL_Renderer* g_renderer = nullptr;
Texture         g_texture_viewport;

Scale g_resolution_scale_factor{};
Delta g_screen_physical_size{};

namespace {

auto g_pixel_format = ::SDL_PIXELFORMAT_RGBA8888;

struct DisplayMode {
  Delta  size;
  Uint32 format;
  int    refresh_rate;
};

DisplayMode get_current_display_mode() {
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode( 0, &dm );
  DisplayMode res{
      {W{dm.w}, H{dm.h}}, dm.format, dm.refresh_rate};
  return res;
}

/*
 *::SDL_DisplayMode find_fullscreen_mode() {
 *  ::SDL_DisplayMode dm;
 *  logger->debug( "Available display modes:" );
 *  auto num_display_modes = ::SDL_GetNumDisplayModes( 0 );
 *  constexpr int min_x_res{1920};
 *  constexpr int min_y_res{1080};
 *  for( int i = 0; i < num_display_modes; ++i ) {
 *    ::SDL_GetDisplayMode( 0, i, &dm );
 *    if( dm.w % g_tile_width._ == 0 &&
 *        dm.h % g_tile_height._ == 0 ) {
 *      logger->debug( "{}x{}", dm.w, dm.h );
 *      if( dm.w >= min_x_res && dm.h >= min_y_res ) return dm;
 *    }
 *  }
 *  dm.w = dm.h = 0; // means we can't find one.
 *  return dm;
 *}
 */

double monitor_diagonal_length( float ddpi, DisplayMode dm ) {
  double length =
      sqrt( pow( dm.size.w._, 2.0 ) + pow( dm.size.h._, 2.0 ) ) /
      ddpi;
  // Round to hearest 1/2 inch.
  return double( lround( length * 2.0 ) ) / 2.0;
}

// Get diagonal DPI of monitor. This seems to basically be the
// same as the horizontal and vertical DPIs.
double monitor_ddpi() {
  float ddpi;
  CHECK( !::SDL_GetDisplayDPI( 0, &ddpi, nullptr, nullptr ) );
  return ddpi;
}

double monitor_inches() {
  return monitor_diagonal_length( monitor_ddpi(),
                                  get_current_display_mode() );
}

double const& viewer_distance_from_monitor() {
  static double distance = [] {
    // Determined empirically; viewer distance from screen seems
    // to scale linearly with screen size, down to a certain
    // minimum distance.
    constexpr double viewer_distance_multiplier{1.25};
    constexpr double viewer_distance_minimum{18}; // inches
    auto             res =
        std::max( viewer_distance_multiplier * monitor_inches(),
                  viewer_distance_minimum );
    logger->debug( "Computed Viewer Distance from Screen: {}in.",
                   res );
    return res;
  }();
  return distance;
};

void query_video_stats() {
  float ddpi, hdpi, vdpi;
  ::SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi );
  logger->debug( "GetDisplayDPI:" );
  logger->debug( "  ddpi: {}", ddpi );
  logger->debug( "  hdpi: {}", hdpi );
  logger->debug( "  vdpi: {}", vdpi );

  SDL_DisplayMode dm;

  auto dm_to_str = [&dm] {
    return fmt::format( "{}x{}@{}Hz, pf={}", dm.w, dm.h,
                        dm.refresh_rate,
                        ::SDL_GetPixelFormatName( dm.format ) );
  };
  (void)dm_to_str;

  logger->debug( "Default game pixel format: {}",
                 ::SDL_GetPixelFormatName( g_pixel_format ) );

  logger->debug( "GetCurrentDisplayMode: " );
  SDL_GetCurrentDisplayMode( 0, &dm );
  logger->debug( "  {}", dm_to_str() );
  if( g_pixel_format != dm.format ) {
    // g_pixel_format =
    //    static_cast<decltype( g_pixel_format )>( dm.format );
    // logger->debug( "Correcting game pixel format to {}",
    //               ::SDL_GetPixelFormatName( dm.format ) );
  }

  logger->debug( "GetDesktopDisplayMode: " );
  SDL_GetDesktopDisplayMode( 0, &dm );
  logger->debug( "  {}", dm_to_str() );

  logger->debug( "GetDisplayMode: " );
  SDL_GetDisplayMode( 0, 0, &dm );
  logger->debug( "  {}", dm_to_str() );

  SDL_Rect r;
  logger->debug( "GetDisplayBounds:" );
  SDL_GetDisplayBounds( 0, &r );
  logger->debug( "  {}", from_SDL( r ) );

  logger->debug( "Monitor Diagonal Length: {}in.",
                 monitor_inches() );
}

struct ScaleInfo {
  int    scale;
  double tile_size_on_screen_surface_inches;
  Delta  resolution;
  double tile_angular_size;
};

ScaleInfo scale_info( int scale_ ) {
  Scale scale{scale_};
  Delta resolution = get_current_display_mode().size / scale;

  // Tile size in inches if it were measured on the surface of
  // the screen.
  double tile_size_screen_surface =
      ( scale * g_tile_scale ).sx._ / monitor_ddpi();

  // Compute the angular size (this is what actually determines
  // how big it looks to the viewer).
  auto theta =
      2.0 * std::atan( ( tile_size_screen_surface / 2.0 ) /
                       viewer_distance_from_monitor() );

  return ScaleInfo{scale_, tile_size_screen_surface, resolution,
                   theta};
}

// Lower score is better.
double scale_score( ScaleInfo const& info ) {
  constexpr double ideal_tile_angular_size{.025}; // radians
  return ::abs( info.tile_angular_size -
                ideal_tile_angular_size );
}

// This function attempts to find an integer scale factor with
// which to scale the pixel size of the display so that a single
// tile has approximately a given size. The scale factor is an
// integer because we scale both dimensions by the same amount,
// and the scale factor is not a floating point number because we
// don't want any distortion of individual pixels which would
// arise in that situation.
void find_pixel_scale_factor() {
  auto scale_scores = rv::iota( 1, 11 ) //
                      | rv::transform( scale_info );

  // What we would like to do is use the following macro, but
  // this currently causes a compiler crash for GCC inside the
  // ranges library. Try again at a later time to use it.
  // ----
  // ASSIGN_CHECK_OPT( optimal,
  //                  scale_scores | min_by_key( scale_score ) );
  // ----
  // and delete this:
  optional<ScaleInfo> maybe_optimal;
  for( auto info : scale_scores ) {
    if( !maybe_optimal.has_value() ) maybe_optimal = info;
    if( scale_score( info ) < scale_score( *maybe_optimal ) )
      maybe_optimal = info;
  }
  CHECK( maybe_optimal.has_value() );
  auto optimal = *maybe_optimal;
  // ----

  ///////////////////////////////////////////////////////////////
  auto table_row = []( auto possibility, auto resolution,
                       auto tile_size_screen, auto tile_size_1ft,
                       auto score ) {
    logger->debug( "{: ^10}{: ^19}{: ^18}{: ^18}{: ^10}",
                   possibility, resolution, tile_size_screen,
                   tile_size_1ft, score );
  };

  table_row( "Scale", "Resolution", "Tile-Size-Screen",
             "Tile-Angular-Size", "Score" );
  auto bar = string( 86, '-' );
  logger->debug( bar );
  for( auto const& info : scale_scores ) {
    string chosen =
        ( info.scale == optimal.scale ) ? "=> " : "   ";
    table_row( chosen + to_string( info.scale ), info.resolution,
               info.tile_size_on_screen_surface_inches,
               info.tile_angular_size, scale_score( info ) );
  }
  logger->debug( bar );
  ///////////////////////////////////////////////////////////////

  g_resolution_scale_factor = Scale{optimal.scale};
  g_screen_physical_size =
      optimal.resolution * g_resolution_scale_factor;
  logger->debug( "screen physical size: {}",
                 g_screen_physical_size );
  logger->debug( "screen logical size: {}",
                 screen_logical_size() );

  // If this is violated then we have non-integer scaling.
  CHECK( g_screen_physical_size % Scale{optimal.scale} ==
         Delta{} );

  // For informational purposes
  if( get_current_display_mode().size % Scale{optimal.scale} !=
      Delta{} )
    logger->warn(
        "Desktop display resolution not commensurate with scale "
        "factor." );
}

void init_screen() {
  query_video_stats();
  find_pixel_scale_factor();
}

} // namespace

REGISTER_INIT_ROUTINE( screen, init_screen, [] {} );

Delta main_window_size() {
  return get_current_display_mode().size;
}

Delta screen_logical_size() {
  return g_screen_physical_size / g_resolution_scale_factor;
}

Rect screen_logical_rect() {
  return Rect::from( {0_y, 0_x}, screen_logical_size() );
}

Delta screen_physical_size() { return g_screen_physical_size; }

Rect screen_physical_rect() {
  return Rect::from( {0_y, 0_x}, screen_physical_size() );
}

Delta viewport_size_pixels() {
  // Subtract height of menu and width of panel.
  return screen_logical_size() - 16_h - 6_w * 32_sx;
}

} // namespace rn
