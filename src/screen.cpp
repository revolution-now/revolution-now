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
#include "init.hpp"
#include "logging.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"

// C++ standard library
#include <optional>

using namespace std;

namespace rn {

::SDL_Window*   g_window   = nullptr;
::SDL_Renderer* g_renderer = nullptr;
Texture         g_texture_viewport;

Scale g_resolution_scale_factor{};
Delta g_drawing_origin{};
Rect  g_drawing_region{};

namespace {

W g_screen_width_tiles{11};
H g_screen_height_tiles{6};
W g_panel_width_tiles{6};

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

void init_screen() {
  query_video_stats();
  find_pixel_scale_factor();
}

} // namespace

REGISTER_INIT_ROUTINE( screen, init_screen, [] {} );

void find_pixel_scale_factor() {
  // We want ideally to have a tile whose side is this length in
  // inches. The algorithm that follows will try to find an
  // integer scaling factor (and associated maximal screen width
  // and height in tiles) such that a) tiles are square, tiles
  // side lengths are as close as possible to this length in
  // inches, and c) as much of the screen is covered as possible.
  // Note that we have opted to avoid scaling the tile grid by
  // non-integer values, and so that represents a key constraint
  // here. Hence we won't generally achieve the ideal tile size,
  // but should come close to it.
  constexpr double ideal_tile_size_at_1ft{.30}; // inches
  auto compute_viewer_distance = []( double monitor_size ) {
    // Determined empirically; viewer distance from screen seems
    // to scale linearly with screen size, down to a certain
    // minimum distance.
    constexpr double viewer_distance_multiplier{1.25};
    constexpr double viewer_distance_minimum{18}; // inches
    return std::max( viewer_distance_multiplier * monitor_size,
                     viewer_distance_minimum );
  };
  // Seems sensible for the tiles to be within these bounds.  The
  // scoring function used would otherwise select these in some
  // cases
  constexpr double minimum_perceived_tile_width{0.2}; // inches
  constexpr double maximum_perceived_tile_width{1.0}; // inches
  ////////////////////////////////////////////////

  auto table_row = []( auto possibility, auto resolution,
                       auto tile_size_screen, auto tile_size_1ft,
                       auto score ) {
    logger->debug( "{: ^18}{: ^18}{: ^18}{: ^18}{: ^18}",
                   possibility, resolution, tile_size_screen,
                   tile_size_1ft, score );
  };

  auto            dm = get_current_display_mode();
  optional<Delta> result;
  double          min_score    = -1.0 / 0.0; // make -infinity
  double          monitor_size = monitor_inches();

  logger->debug( "Computed Viewer Distance from Screen: {}in.",
                 compute_viewer_distance( monitor_size ) );

  double ddpi = monitor_ddpi();
  table_row( "Scale", "Resolution", "Tile-Size-Screen",
             "Tile-Size-@1ft", "Score" );
  auto bar = "------------------";
  table_row( bar, bar, bar, bar, bar );
  Opt<Scale> chosen_scale;

  constexpr Scale max_scale{10}; // somewhat arbitrary
  for( int factor = 1;; ++factor ) {
    Scale scale{factor};
    if( scale == max_scale ) break;

    Delta max_size =
        dm.size / scale - ( ( dm.size / scale ) % g_tile_scale );

    // Tile size in inches if it were measured on the surface of
    // the screen.
    double tile_size_actual =
        ( scale * g_tile_scale ).sx._ / ddpi;

    // Estimate the viewer's distance from the screen based on
    // its size and some other assumptions.
    double viewer_distance =
        compute_viewer_distance( monitor_size );
    constexpr double one_foot{12.0};
    // This is the apparent size in inches of a tile when it is
    // measure by a ruler that is placed one foot in front of the
    // viewer's eye.
    double perceived_size_1ft =
        tile_size_actual / viewer_distance * one_foot;

    // Essentially this gives less weight the further we move
    // away from the ideal.
    double score =
        -::abs( perceived_size_1ft - ideal_tile_size_at_1ft );
    // If the tile size is smaller than a minimum cutoff then
    // we will avoid selecting it by giving it a score smaller
    // than any other.
    if( perceived_size_1ft < minimum_perceived_tile_width ||
        perceived_size_1ft > maximum_perceived_tile_width )
      score = -1.0 / 0.0;

    table_row( factor, max_size, max_size / g_tile_scale,
               tile_size_actual, score );
    if( score >= min_score ) {
      result = max_size / g_tile_scale;

      min_score    = score;
      chosen_scale = scale;
    }
  }
  table_row( bar, bar, bar, bar, bar );
  CHECK( result, "Could not find a suitable scaling" );
  CHECK( chosen_scale.has_value() );
  auto const& delta = *result;
  logger->debug( "Optimal scale factor: {}", *chosen_scale );
  g_screen_width_tiles  = delta.w;
  g_screen_height_tiles = delta.h;
  logger->debug( "tiles: {}", delta );
  g_resolution_scale_factor = *chosen_scale;
  g_drawing_origin =
      ( dm.size - ( delta * *chosen_scale * g_tile_scale ) ) /
      Scale{2};
  g_drawing_region =
      Rect::from( Coord{} + g_drawing_origin,
                  dm.size - g_drawing_origin * Scale{2} );
  logger->debug( "drawing region: {}", g_drawing_region );
  logger->debug( "logical screen pixel dimensions: {}",
                 logical_screen_pixel_dimensions() );
}

Delta logical_screen_pixel_dimensions() {
  return {g_drawing_region.w / g_resolution_scale_factor.sx,
          g_drawing_region.h / g_resolution_scale_factor.sy};
}

Delta main_window_size() {
  return get_current_display_mode().size;
}

Delta screen_logical_size() {
  return logical_screen_pixel_dimensions();
}

Rect screen_logical_rect() {
  return Rect::from( {0_y, 0_x}, screen_logical_size() );
}

Delta screen_size_tiles() {
  return {g_screen_width_tiles, g_screen_height_tiles};
}

Delta viewport_size_pixels() {
  auto size_tiles =
      Delta{g_screen_width_tiles - g_panel_width_tiles,
            g_screen_height_tiles - 1};
  return size_tiles * g_tile_scale;
}

} // namespace rn
