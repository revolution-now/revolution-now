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

double monitor_inches() {
  float ddpi;
  ASSERT( !::SDL_GetDisplayDPI( 0, &ddpi, NULL, NULL ) );
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
  bool monitor_size_small = monitor_inches() < 18;
  // This is a magic number chosen that seems to lead to good
  // behavior on monitors of varying sizes and aspect ratios.
  double ideal_mean_small = 17;
  ////////////////////////////////////////////////
  cout << "Finding max tile sizes:\n";
  auto dm = get_current_display_mode();
  optional<pair<H,W>> res;
  double min_weight = -1000000;
  int col = 16;
  cout << setw( 3 ) << "#" << setw( col ) << "Possibility" << setw( col )
       << "Eff. Resolution" << setw( col ) << "Total Tiles"
       << setw( col ) << "Geo Mean" << setw( col ) << "Weight"
       << "\n";
  int possibility = 1;
  int chosen = -1;
  double ideal_mean = monitor_size_small
                    ? ideal_mean_small
                    : ideal_mean_small*2;
  for( int scale = 10; scale >= 1; --scale ) {
    cout << setw( 3 ) << possibility;
    W max_width{dm.w/scale - ((dm.w/scale) % 32 )};
    H max_height{dm.h/scale - ((dm.h/scale) % 32 )};
    double geo_mean = ::sqrt( (max_height/32)._*(max_width/32)._ );
    ostringstream ss;
    ss << max_width/32 << "x" << max_height/32;
    cout << setw( col ) << ss.str();
    ss.str( "" );
    ss << max_width << "x" << max_height;
    cout << setw( col ) << ss.str();
    cout << setw( col ) << geo_mean << setw( col ) << geo_mean;
    double weight = (-::pow( geo_mean-ideal_mean, 2.0 ))/100.0*100;
    cout << setw( col ) << int( weight );
    cout << "\n";
    if( weight >= min_weight ) {
      res = {max_height/32,max_width/32};
      min_weight = weight;
      chosen = possibility;
    }
    ++possibility;
  }
  ASSERT( res );
  cout << "chose " << chosen << "\n";
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
