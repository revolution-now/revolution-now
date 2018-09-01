#include "base-util.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
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

int main( int, char** ) {

  init_game();
  load_sprites();
  load_tile_maps();

  create_unit_on_map( g_unit_type::free_colonist, Y(2), X(3) );
  create_unit_on_map( g_unit_type::caravel, Y(2), X(2) );

  //play_music_file( "../music/bonny-morn.mp3" );

  while( turn() != k_turn_result::quit ) {}

  cleanup();
  return 0;
}
