#include "base-util.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"

#include <SDL.h>
#include <SDL_image.h>

int main( int, char** ) {
  // initialize SDL
  rn::init_sdl();

  rn::create_window();
  rn::create_renderer();
  rn::load_sprites();
  rn::load_tile_maps();

  // This function expects Red, Green, Blue and Alpha as color
  // values
  SDL_SetRenderDrawColor( rn::g_renderer, 0, 0, 0, 255 );
  // Clear the window to the current drawing color.
  SDL_RenderClear( rn::g_renderer );

  rn::render_tile_map( "panel" );

  // Update the screen with rendering performed.
  SDL_RenderPresent( rn::g_renderer );

  // Set a delay before quitting
  //SDL_Delay( 2000 );

  bool running = true;

  while( running ) {
    if(SDL_Event event; SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          if( event.key.keysym.sym == SDLK_q ||
              event.key.keysym.sym == SDLK_RETURN ) {
            running = false;
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
