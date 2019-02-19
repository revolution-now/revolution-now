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

SDL_DisplayMode get_current_display_mode() {
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode( 0, &dm );
  return dm;
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

double monitor_diagonal_length( float             ddpi,
                                ::SDL_DisplayMode dm ) {
  double length =
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
      sqrt( pow( dm.w, 2.0 ) + pow( dm.h, 2.0 ) ) / ddpi;
  // Round to hearest 1/2 inch.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  return double( lround( length * 2.0 ) ) / 2.0;
}

// Get diagonal DPI of monitor. These seems to basically be the
// same as the horizontal and vertical DPIs.
double monitor_ddpi() {
  float ddpi;
  CHECK( !::SDL_GetDisplayDPI( 0, &ddpi, nullptr, nullptr ) );
  return ddpi;
}

double monitor_inches() {
  float           ddpi = monitor_ddpi();
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode( 0, &dm );
  return monitor_diagonal_length( ddpi, dm );
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

} // namespace

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

  // cout << "Finding optimal tile sizes:\n";
  auto            dm = get_current_display_mode();
  optional<Delta> result;
  double          min_score = -1.0 / 0.0; // make -infinity
  // constexpr int        col{18};
  double monitor_size = monitor_inches();
  // cout << "Computed Viewer Distance from Screen: "
  //     << compute_viewer_distance( monitor_size ) << "in.\n";
  double ddpi = monitor_ddpi();
  // cout << "\n  " << setw( 3 ) << "#" << setw( col )
  //     << "Possibility" << setw( col ) << "Resolution"
  //     << setw( col ) << "Tile-Size-Screen" << setw( col )
  //     << "Tile-Size-@1ft" << setw( col ) << "Score"
  //     << "\n";
  int chosen_scale = -1;

  constexpr int max_scale{10}; // somewhat arbitrary
  for( int scale = 1; scale <= max_scale; ++scale ) {
    // cout << "  " << setw( 3 ) << scale;

    W max_width{dm.w / scale -
                ( ( dm.w / scale ) % g_tile_width._ )};
    H max_height{dm.h / scale -
                 ( ( dm.h / scale ) % g_tile_height._ )};

    // ostringstream ss;
    // ss << max_width / g_tile_width << "x"
    //   << max_height / g_tile_height;
    // cout << setw( col ) << ss.str();
    // ss.str( "" );
    // ss << max_width << "x" << max_height;
    // cout << setw( col ) << ss.str();

    // Tile size in inches if it were measured on the surface of
    // the screen.
    double tile_size_actual = scale * g_tile_width._ / ddpi;
    // cout << setw( col ) << tile_size_actual;
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
    // cout << setw( col ) << perceived_size_1ft;

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
    // cout << setw( col ) << score;
    // cout << "\n";
    if( score >= min_score ) {
      result = {W( max_width / g_tile_width ),
                H( max_height / g_tile_height )};

      min_score    = score;
      chosen_scale = scale;
    }
  }
  CHECK( result, "Could not find a suitable scaling" );
  auto const& delta = *result;
  logger->debug( "Optimal scale factor: #{}", chosen_scale );
  g_screen_width_tiles  = delta.w;
  g_screen_height_tiles = delta.h;
  logger->debug( "tiles: {} x {}", delta.w, delta.h );
  g_resolution_scale_factor = Scale( chosen_scale );
  g_drawing_origin.w =
      ( dm.w -
        ( chosen_scale * ( delta.w * g_tile_width )._ ) ) /
      2;
  g_drawing_origin.h =
      ( dm.h -
        ( chosen_scale * ( delta.h * g_tile_height )._ ) ) /
      2;
  g_drawing_region.x = 0_x + g_drawing_origin.w;
  g_drawing_region.y = 0_y + g_drawing_origin.h;
  g_drawing_region.w = dm.w - g_drawing_origin.w * 2_sx;
  g_drawing_region.h = dm.h - g_drawing_origin.h * 2_sy;
  logger->debug( "w drawing region: {}", g_drawing_region );
  logger->debug( "logical screen pixel dimensions: {}",
                 logical_screen_pixel_dimensions() );
}

Delta logical_screen_pixel_dimensions() {
  return {g_drawing_region.w / g_resolution_scale_factor.sx,
          g_drawing_region.h / g_resolution_scale_factor.sy};
}

Delta main_window_size() {
  auto dm = get_current_display_mode();
  return {W{dm.w}, H{dm.h}};
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

void initialize_screen() { query_video_stats(); }

void cleanup_screen() {}

} // namespace rn
