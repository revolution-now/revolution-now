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

#include "sdl-util.hpp"

#include "base-util.hpp"
#include "globals.hpp"
#include "global-constants.hpp"
#include "macros.hpp"

#include <iostream>
#include <vector>

using namespace std;

namespace rn {

namespace {

vector<SDL_Texture*> loaded_textures;

int g_screen_w;
int g_screen_h;

int g_screen_offset_x;
int g_screen_offset_y;

} // namespace

void init_sdl() {
  if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    DIE( "sdl could not initialize" );

  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode(0, &dm);
  g_screen_w = dm.w;
  g_screen_h = dm.h;

  if( g_screen_w > 1920)
    g_display_scaled = true;

  g_screen_offset_x = 64;
  g_screen_offset_y = 64;
}

// SDL_WINDOW_FULLSCREEN,    SDL_WINDOW_OPENGL,
// SDL_WINDOW_HIDDEN,        SDL_WINDOW_BORDERLESS,
// SDL_WINDOW_RESIZABLE,     SDL_WINDOW_MAXIMIZED,
// SDL_WINDOW_MINIMIZED,     SDL_WINDOW_INPUT_GRABBED,
// SDL_WINDOW_ALLOW_HIGHDPI, SDL_WINDOW_VULKAN.
void create_window() {

  int width = g_tile_width*g_viewport_width_tiles;
  int height = g_tile_height*g_viewport_height_tiles;

  width *= g_display_scaled ? 2 : 1;
  height *= g_display_scaled ? 2 : 1;

  auto flags = SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN;

  //flags |= SDL_WINDOW_ALLOW_HIGHDPI;

  g_window = SDL_CreateWindow( "Revolution Now",
    0, 0, width, height, flags );

  if( !g_window )
    DIE( "failed to create window" );
}

void print_display_mode( SDL_DisplayMode const& dm ) {
  cout << "  w...........: " << dm.w << "\n";
  cout << "  h...........: " << dm.h << "\n";
  cout << "  refresh_rate:" << dm.refresh_rate << "\n";
}

void print_rect( SDL_Rect const& r ) {
  cout << "  x: " << r.x << "\n";
  cout << "  y: " << r.y << "\n";
  cout << "  w: " << r.w << "\n";
  cout << "  h: " << r.h << "\n";
}

void print_video_stats() {

  SDL_DisplayMode dm;

  cout << "GetCurrentDisplayMode:\n";
  SDL_GetCurrentDisplayMode(0, &dm);
  print_display_mode( dm );

  cout << "GetDesktopDisplayMode:\n";
  SDL_GetDesktopDisplayMode( 0, &dm );
  print_display_mode( dm );

  cout << "GetDisplayMode:\n";
  SDL_GetDisplayMode( 0, 0, &dm );
  print_display_mode( dm );

  cout << "GetWindowDisplayMode:\n";
  SDL_GetWindowDisplayMode( g_window, &dm );
  print_display_mode( dm );

  int w, h;
  cout << "SDL_GL_GetDrawableSize:\n";
  SDL_GL_GetDrawableSize( g_window, &w, &h );
  cout << "  w: " << w << "\n";
  cout << "  h: " << h << "\n";

  cout << "SDL_GetRendererOutputSize:\n";
  SDL_GetRendererOutputSize( g_renderer, &w, &h );
  cout << "  w: " << w << "\n";
  cout << "  h: " << h << "\n";

  SDL_Rect r;
  cout << "GetDisplayBounds:\n";
  SDL_GetDisplayBounds( 0, &r );
  print_rect( r );

  float hdpi, vdpi;
  cout << "GetDisplayDPI:\n";
  SDL_GetDisplayDPI( 0, NULL, &hdpi, &vdpi );
  cout << "  x-dpi: " << hdpi << "\n";
  cout << "  y-dpi: " << vdpi << "\n";

  cout << "\n";
  cout << "g_display_scaled...........: " << g_display_scaled << "\n";
  cout << "g_screen_offset_x..........: " << g_screen_offset_x << "\n";
  cout << "g_screen_offset_y..........: " << g_screen_offset_y << "\n";
}

void create_renderer() {
  g_renderer = SDL_CreateRenderer( g_window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  if( !g_renderer )
    DIE( "failed to create renderer" );
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

void render_texture( SDL_Texture* texture, SDL_Rect source, SDL_Rect dest, double angle, SDL_RendererFlip flip ) {
  SDL_Rect dest_shifted = dest;
  dest_shifted.x += g_screen_offset_x;
  dest_shifted.y += g_screen_offset_y;
  if( SDL_RenderCopyEx( g_renderer, texture, &source, &dest_shifted,
                        angle, NULL, flip ) )
    DIE( "failed to render texture" );
}

} // namespace rn
