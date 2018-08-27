#include "base-util.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"

#include <SDL.h>
#include <SDL_image.h>

void render() {
  // This function expects Red, Green, Blue and Alpha as color
  // values
  SDL_SetRenderDrawColor( rn::g_renderer, 0, 0, 0, 255 );
  // Clear the window to the current drawing color.
  SDL_RenderClear( rn::g_renderer );

  for( int i = 0; i < 48; ++i ) {
    for( int j = 0; j < 26; ++j ) {
      if( (i+j) % 2 == 0 )
        rn::render_sprite_grid( rn::g_tile::water, j, i, 0, 0 );
      else
        rn::render_sprite_grid( rn::g_tile::land, j, i, 0, 0 );
    }
  }

  //rn::render_tile_map( "panel" );

  // Update the screen with rendering performed.
  SDL_RenderPresent( rn::g_renderer );
}

int main( int, char** ) {
  // initialize SDL
  rn::init_sdl();

  rn::create_window();
  rn::print_video_stats();
  rn::create_renderer();
  rn::load_sprites();
  rn::load_tile_maps();

  render();

  // Set a delay before quitting
  //SDL_Delay( 2000 );

  bool running = true;

  while( running ) {
    if(SDL_Event event; SDL_PollEvent(&event)) {
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
