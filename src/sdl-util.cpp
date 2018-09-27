/****************************************************************
* sdl-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Interface for calling SDL functions
*
*****************************************************************/
#include <iostream>
#include <vector>

#include "sdl-util.hpp"

#include "base-util.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "sound.hpp"

#include <SDL_mixer.h>

#include <iomanip>
#include <sstream>

using namespace std;

ostream& operator<<( ostream& out, ::SDL_Rect const& r ) {
  return (out << "(" << r.x << "," << r.y << "," << r.w << "," << r.h << ")");
}

namespace rn {

namespace {

vector<::SDL_Texture*> loaded_textures;

ostream& operator<<( ostream& out, ::SDL_DisplayMode const& dm ) {
  return (out << dm.w << "x" << dm.h << "[" << dm.refresh_rate << "Hz]");
}

SDL_DisplayMode get_current_display_mode() {
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode(0, &dm);
  return dm;
}

} // namespace

::SDL_Rect to_SDL( Rect const& rect ) {
  ::SDL_Rect res;
  res.x = rect.x._;
  res.y = rect.y._;
  res.w = rect.w._;
  res.h = rect.h._;
  return res;
}

void init_game() {
  rn::init_sdl();
  rn::create_window();
  rn::print_video_stats();
  rn::create_renderer();
}

void init_sdl() {
  if( ::SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    DIE( "sdl could not initialize" );

  // Open Audio device
  if( Mix_OpenAudio( 44100, AUDIO_S16SYS, 2, 2048 ) != 0 ) {
    cerr << "Mix_OpenAudio ERROR: " << ::Mix_GetError() << endl;
    DIE( "could not open audio" );
  }
  // Set Volume
  ::Mix_VolumeMusic( 10 );
}

SDL_DisplayMode find_fullscreen_mode() {
  ::SDL_DisplayMode dm;
  cout << "Available display modes:\n";
  auto num_display_modes = ::SDL_GetNumDisplayModes( 0 );
  for( int i = 0; i < num_display_modes; ++i ) {
    ::SDL_GetDisplayMode( 0, i, &dm );
    if( dm.w % 32 == 0 && dm.h % 32 == 0 ) {
      cout << dm.w << "x" << dm.h << "\n";
      if( dm.w >= 1920 && dm.h >= 1080 )
        return dm;
    }
  }
  dm.w = dm.h = 0; // means we can't find one.
  return dm;
}

void create_window() {
  auto flags = ::SDL_WINDOW_SHOWN | ::SDL_WINDOW_RESIZABLE |
               ::SDL_WINDOW_FULLSCREEN_DESKTOP;

  //auto fullscreen_mode = find_fullscreen_mode();
  //if( !fullscreen_mode.w )
  //  DIE( "cannot find appropriate fullscreen mode" );

  auto dm = get_current_display_mode();

  g_window = ::SDL_CreateWindow( string( g_window_title ).c_str(),
    0, 0, dm.w, dm.h, flags );
  if( !g_window )
    DIE( "failed to create window" );

  //::SDL_SetWindowDisplayMode( g_window, &fullscreen_mode );
}

double monitor_diagonal_length( float ddpi, ::SDL_DisplayMode dm ) {
  double length = sqrt( pow( dm.w, 2.0 ) + pow( dm.h, 2.0 ) )/ddpi;
  // Round to hearest 1/2 inch.
  return double(int(length*2.0+.5))/2.0;
}

// Get diagonal DPI of monitor. These seems to basically be the
// same as the horizontal and vertical DPIs.
double monitor_ddpi() {
  float ddpi;
  ASSERT( !::SDL_GetDisplayDPI( 0, &ddpi, NULL, NULL ) );
  return ddpi;
}

double monitor_inches() {
  float ddpi = monitor_ddpi();
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode(0, &dm);
  return monitor_diagonal_length( ddpi, dm );
}

void print_video_stats() {
  float ddpi, hdpi, vdpi;
  ::SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi );
  cout << "GetDisplayDPI:\n";
  cout << "  ddpi: " << ddpi << "\n";
  cout << "  hdpi: " << hdpi << "\n";
  cout << "  vdpi: " << vdpi << "\n";

  SDL_DisplayMode dm;

  cout << "GetCurrentDisplayMode:\n";
  SDL_GetCurrentDisplayMode(0, &dm);
  cout << "  " << dm << "\n";

  cout << "GetDesktopDisplayMode:\n";
  SDL_GetDesktopDisplayMode( 0, &dm );
  cout << "  " << dm << "\n";

  cout << "GetDisplayMode:\n";
  SDL_GetDisplayMode( 0, 0, &dm );
  cout << "  " << dm << "\n";

  SDL_Rect r;
  cout << "GetDisplayBounds:\n";
  SDL_GetDisplayBounds( 0, &r );
  cout << "  " << r << "\n";

  cout << "Monitor Diagonal Length: " << monitor_inches() << "in." << "\n";
}

pair<H,W> find_max_tile_sizes() {
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
  double ideal_tile_size_at_1ft = .25; // inches
  auto compute_viewer_distance = []( double monitor_size ){
    // Determined empirically; viewer distance from screen seems
    // to scale linearly with screen size, down to a certain
    // minimum distance.
    return max( 1.25*monitor_size, 18.0 );
  };
  // Seems sensible for the tiles to be within these bounds.  The
  // scoring function used would otherwise select these in some
  // cases
  double minimum_perceived_tile_width = 0.2; // inches
  double maximum_perceived_tile_width = 2.0; // inches
  ////////////////////////////////////////////////

  cout << "Finding optimal tile sizes:\n";
  auto dm = get_current_display_mode();
  optional<pair<H,W>> res;
  double min_score = -1.0/0.0; // make -infinity
  int col = 18;
  double monitor_size = monitor_inches();
  cout << "Computed Viewer Distance from Screen: "
       << compute_viewer_distance( monitor_size ) << "in.\n";
  double ddpi = monitor_ddpi();
  cout << "\n  " << setw( 3 ) << "#" << setw( col ) << "Possibility"
       << setw( col ) << "Resolution" << setw( col )
       << "Tile-Size-Screen" << setw( col ) << "Tile-Size-@1ft"
       << setw( col ) << "Score" << "\n";
  int chosen_scale = -1;
  for( int scale = 1; scale <= 10; ++scale ) {
    cout << "  " << setw( 3 ) << scale;
    W max_width{dm.w/scale - ((dm.w/scale) % 32 )};
    H max_height{dm.h/scale - ((dm.h/scale) % 32 )};
    ostringstream ss;
    ss << max_width/32 << "x" << max_height/32;
    cout << setw( col ) << ss.str();
    ss.str( "" );
    ss << max_width << "x" << max_height;
    cout << setw( col ) << ss.str();

    // Tile size in inches if it were measured on the surface of
    // the screen.
    double tile_size_actual = scale*g_tile_width._/ddpi;
    cout << setw( col ) << tile_size_actual;
    // Estimate the viewer's distance from the screen based on
    // its size and some other assumptions.
    double viewer_distance =
      compute_viewer_distance( monitor_size );
    double one_foot = 12.0;
    // This is the apparent size in inches of a tile when it is
    // measure by a ruler that is placed one foot in front of the
    // viewer's eye.
    double perceived_size_1ft =
      tile_size_actual/viewer_distance*one_foot;
    cout << setw( col ) << perceived_size_1ft;

    // Parabola, concave-downward, centered on ideal value.
    // Essentially this gives less weight the further we move
    // away from the ideal.
    double score = -::abs(
        perceived_size_1ft-ideal_tile_size_at_1ft );//, 2.0 );
    // If the tile size is smaller than a minimum cutoff then
    // we will avoid selecting it by giving it a score smaller
    // than any other.
    if( perceived_size_1ft < minimum_perceived_tile_width ||
        perceived_size_1ft > maximum_perceived_tile_width )
      score = -1.0/0.0;
    cout << setw( col ) << score;
    cout << "\n";
    if( score >= min_score ) {
      res = {max_height/32,max_width/32};
      min_score = score;
      chosen_scale = scale;
    }
  }
  ASSERT( res );
  cout << "\n  Optimal: #" << chosen_scale << "\n";
  return *res;
}

void create_renderer() {
  g_renderer = SDL_CreateRenderer( g_window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  if( !g_renderer )
    DIE( "failed to create renderer" );

  auto screen_sizes = find_max_tile_sizes();
  set_screen_width_tiles( screen_sizes.second );
  set_screen_height_tiles( screen_sizes.first );

  W width = g_tile_width._*screen_width_tiles();
  H height = g_tile_height._*screen_height_tiles();
  //cout << "logical renderer width : " << width << "\n";
  //cout << "logical renderer height: " << height << "\n";

  ::SDL_RenderSetLogicalSize( g_renderer, width._, height._ );
  ::SDL_RenderSetIntegerScale( g_renderer, ::SDL_TRUE );

  // +1 tile because we may need to draw a bit in excess of the
  // viewport window in order to facilitate smooth scrolling,
  // though we shouldn't need more than 1 extra tile for that
  // purpose.  The *2 is so that we can accomodate the maximum
  // zoomed-out level which is .5.
  width = g_tile_width._*2*(viewport_width_tiles() + 1);
  height = g_tile_height._*2*(viewport_height_tiles() + 1);
  g_texture_world = ::SDL_CreateTexture( g_renderer,
      SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width._, height._ );
}

SDL_Texture* load_texture( const char* file ) {
  SDL_Surface* pTempSurface = IMG_Load( file );
  if( !pTempSurface )
    DIE( "failed to load image" );
  SDL_Texture* texture =
    SDL_CreateTextureFromSurface( rn::g_renderer, pTempSurface );
  if( !texture )
    DIE( "failed to create texture" );
  SDL_FreeSurface( pTempSurface );
  loaded_textures.push_back( texture );
  return texture;
}

void unload_textures() {
  for( auto texture : loaded_textures )
    SDL_DestroyTexture( texture );
}

// All the functions in this method should not cause problems
// even if their corresponding initialization routines were
// not successfully run.
void cleanup() {
  cleanup_sound();
  unload_textures();
  if( g_renderer )
    SDL_DestroyRenderer( g_renderer );
  if( g_window )
    SDL_DestroyWindow( g_window );
  // Not clear if this actually quits the program; but it does
  // deinitialize everything.
  SDL_Quit();
}

void render_texture( SDL_Texture* texture, SDL_Rect source, SDL_Rect dest,
                     double angle, SDL_RendererFlip flip ) {
  SDL_Rect dest_shifted = dest;
  if( SDL_RenderCopyEx( g_renderer, texture, &source, &dest_shifted,
                        angle, NULL, flip ) )
    DIE( "failed to render texture" );
}

// TODO: mac-os, does not seem to be able to detect when the user
// fullscreens a window.
bool is_window_fullscreen() {
  // This bit should always be set even if we're in the "desktop"
  // fullscreen mode.
  return (::SDL_GetWindowFlags( g_window ) & ::SDL_WINDOW_FULLSCREEN);
}

void set_fullscreen( bool fullscreen ) {
  bool already = is_window_fullscreen();
  if( !(fullscreen ^ already) )
    return;

  // Must only contain one of the following values.
  ::Uint32 flags = fullscreen ? ::SDL_WINDOW_FULLSCREEN_DESKTOP
                                : 0;
  ::SDL_SetWindowFullscreen( g_window, flags );
}

void toggle_fullscreen() {
  set_fullscreen( !is_window_fullscreen() );
}

} // namespace rn
