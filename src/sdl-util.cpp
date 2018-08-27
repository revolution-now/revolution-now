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

ostream& operator<<( ostream& out, SDL_DisplayMode const& dm ) {
  return (out << dm.w << "x" << dm.h << "[" << dm.refresh_rate << "Hz]");
}

} // namespace

void init_sdl() {
  if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    DIE( "sdl could not initialize" );

  //SDL_DisplayMode dm;
  //SDL_GetCurrentDisplayMode(0, &dm);
  //g_screen_w = dm.w;
  //g_screen_h = dm.h;

  //if( g_screen_w > 1920)
    //g_display_scaled = true;
  g_display_scaled = false;
}

SDL_DisplayMode find_fullscreen_mode() {
  SDL_DisplayMode dm;

	cout << "Available display modes:\n";
  auto num_display_modes = SDL_GetNumDisplayModes( 0 );
	for( int i = 0; i < num_display_modes; ++i ) {
		SDL_GetDisplayMode( 0, i, &dm );
		if( dm.w % 32 == 0 && dm.h % 32 == 0 ) {
			cout << dm.w << "x" << dm.h << "\n";
			if( dm.w >= 1920 && dm.h >= 1080 )
				return dm;
		}
	}
  dm.w = dm.h = 0; // means we can't find one.
	return dm;
}

// SDL_WINDOW_FULLSCREEN,    SDL_WINDOW_OPENGL,
// SDL_WINDOW_HIDDEN,        SDL_WINDOW_BORDERLESS,
// SDL_WINDOW_RESIZABLE,     SDL_WINDOW_MAXIMIZED,
// SDL_WINDOW_MINIMIZED,     SDL_WINDOW_INPUT_GRABBED,
// SDL_WINDOW_ALLOW_HIGHDPI, SDL_WINDOW_VULKAN.
void create_window() {

  int width = g_tile_width*g_viewport_width_tiles;
  int height = g_tile_height*g_viewport_height_tiles;

  cout << "computed width.: " << width << "\n";
  cout << "computed height: " << height << "\n";

  auto flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	auto fullscreen_mode = find_fullscreen_mode();
	if( !fullscreen_mode.w )
		DIE( "cannot find appropriate fullscreen mode" );

  //flags |= SDL_WINDOW_ALLOW_HIGHDPI;
  flags |= ::SDL_WINDOW_FULLSCREEN_DESKTOP;

  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode(0, &dm);

  g_window = SDL_CreateWindow( "Revolution Now",
    0, 0, dm.w/2, dm.h/2, flags );

  if( !g_window )
    DIE( "failed to create window" );

  //SDL_DisplayMode dm_desktop;
  //SDL_GetDesktopDisplayMode(0, &dm);

  //cout << "desktop display mode: " << dm_desktop << "\n";
	//SDL_SetWindowDisplayMode( g_window, &dm_desktop );
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
}

void create_renderer() {
  g_renderer = SDL_CreateRenderer( g_window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  if( !g_renderer )
    DIE( "failed to create renderer" );

  //::SDL_Rect vp;
  //vp.x = vp.y = 0;
  //vp.w = vp.h = 100;
  //::SDL_RenderSetViewport( g_renderer, &vp );

  ::SDL_RenderSetLogicalSize( g_renderer, 824, 480 );

  ::SDL_RenderSetScale( g_renderer, 2, 2 );
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
  if( SDL_RenderCopyEx( g_renderer, texture, &source, &dest_shifted,
                        angle, NULL, flip ) )
    DIE( "failed to render texture" );
}

SDL_DisplayMode get_current_display_mode() {
  SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);
  return dm;
}

bool is_window_fullscreen() {
  auto flags = ::SDL_GetWindowFlags( g_window );
  return (flags & ::SDL_WINDOW_FULLSCREEN) ||
         (flags & ::SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void set_fullscreen( bool fullscreen ) {
  bool already = is_window_fullscreen();
  if( fullscreen && already )
    return;
  if( !fullscreen && !already )
    return;

  ::Uint32 flags = fullscreen ? ::SDL_WINDOW_FULLSCREEN_DESKTOP
                                : 0;

  //if( fullscreen )
  //  ::SDL_SetWindowBordered( g_window, ::SDL_FALSE );

  ::SDL_SetWindowFullscreen( g_window, flags );

  //if( !fullscreen )
  //  ::SDL_SetWindowBordered( g_window, ::SDL_TRUE );

  cout << "changed mode to: " << get_current_display_mode() << "\n";
}

void toggle_fullscreen() {
  if( is_window_fullscreen() )
    set_fullscreen( false );
  else
    set_fullscreen( true );
}

} // namespace rn
