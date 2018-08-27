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
#include <ostream>
#include <vector>

#include "sdl-util.hpp"

#include "base-util.hpp"
#include "globals.hpp"
#include "global-constants.hpp"
#include "macros.hpp"

using namespace std;

namespace rn {

namespace {

vector<::SDL_Texture*> loaded_textures;

ostream& operator<<( ostream& out, ::SDL_DisplayMode const& dm ) {
  return (out << dm.w << "x" << dm.h << "[" << dm.refresh_rate << "Hz]");
}

ostream& operator<<( ostream& out, ::SDL_Rect const& r ) {
  return (out << "(" << r.x << "," << r.y << "," << r.w << "," << r.h << ")");
}

SDL_DisplayMode get_current_display_mode() {
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode(0, &dm);
  return dm;
}

} // namespace

void init_game() {
  rn::init_sdl();
  rn::create_window();
  //rn::print_video_stats();
  rn::create_renderer();
}

void init_sdl() {
  if( ::SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    DIE( "sdl could not initialize" );
}

void create_window() {
  auto flags = ::SDL_WINDOW_SHOWN | ::SDL_WINDOW_RESIZABLE;
  auto dm = get_current_display_mode();

  g_window = ::SDL_CreateWindow( string( g_window_title ).c_str(),
    0, 0, dm.w/3*2, dm.h/3*2, flags );
  if( !g_window )
    DIE( "failed to create window" );
}

void print_video_stats() {
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
}

void create_renderer() {
  g_renderer = SDL_CreateRenderer( g_window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  if( !g_renderer )
    DIE( "failed to create renderer" );

  int width = g_tile_width*g_viewport_width_tiles;
  int height = g_tile_height*g_viewport_height_tiles;
  cout << "logical renderer width : " << width << "\n";
  cout << "logical renderer height: " << height << "\n";

  ::SDL_RenderSetLogicalSize( g_renderer, width, height );

  // +1 tile because we may need to draw a bit in excess of the
  // viewport window in order to facilitate smooth scrolling,
  // though we shouldn't need more than 1 extra tile.
  width = g_tile_width*(g_world_viewport_width_tiles + 1);
  height = g_tile_height*(g_world_viewport_height_tiles + 1);
  g_texture_world = ::SDL_CreateTexture( g_renderer,
      SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height );
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

void cleanup() {
  unload_textures();
  SDL_DestroyRenderer( g_renderer );
  SDL_DestroyWindow( g_window );
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
