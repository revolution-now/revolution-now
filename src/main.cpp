#include "base-util.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"
#include "viewport.hpp"
#include "world.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <iostream>

using namespace rn;

double movement_speed = 8.0;

void render() {
  ::SDL_SetRenderTarget( g_renderer, g_texture_world );
  SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  SDL_RenderClear( g_renderer );

  auto covered = viewport_covered_tiles();

  for( Y i = covered.y; i < covered.y+covered.h; ++i ) {
    for( X j = covered.x; j < covered.x+covered.w; ++j ) {
      auto s_ = square_at_safe( i, j);
      ASSERT( s_ );
      Square const& s = *s_;
      g_tile t = s.land ? g_tile::land : g_tile::water;
      render_sprite_grid( t, Y(0)+(i-covered.y), X(0)+(j-covered.x), 0, 0 );
    }
  }

  ::SDL_SetRenderTarget( g_renderer, NULL );
  SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  SDL_RenderClear( g_renderer );

  ::SDL_Rect src = to_SDL( viewport_get_render_src_rect() );
  ::SDL_Rect dest = to_SDL( viewport_get_render_dest_rect() );
  ::SDL_RenderCopy( g_renderer, g_texture_world, &src, &dest );

  render_tile_map( "panel" );

  SDL_RenderPresent( g_renderer );
}

int main( int, char** ) {
  init_game();
  load_sprites();
  load_tile_maps();

  render();

  // Set a delay before quitting
  //SDL_Delay( 2000 );

  bool running = true;

  while( running ) {
    if( ::SDL_Event event; SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED:
              render();
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case SDLK_q:
            case SDLK_RETURN:
              running = false;
              break;
            case ::SDLK_SPACE:
              toggle_fullscreen();
              break;
            case ::SDLK_LEFT:
              viewport_pan( 0, -movement_speed, false );
              render();
              break;
            case ::SDLK_RIGHT:
              viewport_pan( 0, movement_speed, false );
              render();
              break;
            case ::SDLK_DOWN:
              viewport_pan( movement_speed, 0, false );
              render();
              break;
            case ::SDLK_UP:
              viewport_pan( -movement_speed, 0, false );
              render();
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            viewport_scale_zoom( 0.98 );
          if( event.wheel.y > 0 )
            viewport_scale_zoom( 1.02 );
          render();
          break;
        default:
          break;
      }
    }
  }

  cleanup();
  return 0;
}
