#include "base-util.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"

#include <SDL.h>
#include <SDL_image.h>

void render() {
  ::SDL_SetRenderTarget( rn::g_renderer, rn::g_texture_world );
  SDL_SetRenderDrawColor( rn::g_renderer, 0, 0, 0, 255 );
  SDL_RenderClear( rn::g_renderer );

  for( int i = 0; i < rn::g_viewport_width_tiles; ++i ) {
    for( int j = 0; j < rn::g_viewport_height_tiles; ++j ) {
      if( (i+j) % 4 == 0 )
        rn::render_sprite_grid( rn::g_tile::water, j, i, 0, 0 );
      else
        rn::render_sprite_grid( rn::g_tile::land, j, i, 0, 0 );
    }
  }

  ::SDL_Rect src; src.x = src.y = 0; src.w = src.h = 100;
  ::SDL_SetRenderDrawColor( rn::g_renderer, 60, 60, 60, 255 );
  ::SDL_RenderFillRect( rn::g_renderer, &src );
  src.x = src.y = 100;
  ::SDL_RenderFillRect( rn::g_renderer, &src );

  ::SDL_SetRenderTarget( rn::g_renderer, NULL );
  SDL_SetRenderDrawColor( rn::g_renderer, 0, 0, 0, 255 );
  SDL_RenderClear( rn::g_renderer );

  ::SDL_RenderCopy( rn::g_renderer, rn::g_texture_world, NULL, NULL );

  rn::render_tile_map( "panel" );

  SDL_RenderPresent( rn::g_renderer );
}

int main( int, char** ) {
  rn::init_game();
  rn::load_sprites();
  rn::load_tile_maps();

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
              rn::toggle_fullscreen();
              break;
          }
        default:
          break;
      }
    }
  }

  rn::cleanup();
  return 0;
}
