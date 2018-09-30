#include "base-util.hpp"
#include "fonts.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "ownership.hpp"
#include "sdl-util.hpp"
#include "sound.hpp"
#include "tiles.hpp"
#include "turn.hpp"
#include "unit.hpp"
#include "viewport.hpp"
#include "world.hpp"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <iostream>

using namespace rn;
using namespace std;

int main( int, char** ) try {
  init_game();
  load_sprites();
  load_tile_maps();

  //play_music_file( "../music/bonny-morn.mp3" );

  //(void)create_unit_on_map( e_unit_type::free_colonist, Y(2), X(3) );
  (void)create_unit_on_map( e_unit_type::free_colonist, Y(2), X(4) );
  (void)create_unit_on_map( e_unit_type::caravel, Y(2), X(2) );
  //(void)create_unit_on_map( e_unit_type::caravel, Y(2), X(1) );

  while( turn() != e_turn_result::quit ) {}
  //(void)turn();

  font_test();

  cleanup();
  return 0;

} catch( exception const& e ) {
  cerr << e.what() << endl;
  cerr << "SDL_GetError: " << SDL_GetError()  << endl;
  cleanup();
  return 1;
}
