#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "sdl-util.hpp"
#include "sound.hpp"
#include "tiles.hpp"
#include "turn.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "viewport.hpp"
#include "window.hpp"
#include "world.hpp"

#include "base-util/string.hpp"

#include "absl/strings/str_split.h"
#include "fmt/format.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

using namespace rn;
using namespace std;

void game() {
  init_game();
  load_sprites();
  load_tile_maps();

  // CHECK( play_music_file( "assets/music/bonny-morn.mp3" ) );

  //(void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
  // 3_x );
  //(void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
  //                          4_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 2_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 1_x );

  // while( turn() != e_turn_result::quit ) {}

  // font_test();
  gui::test_window();

  cleanup();
}

int main( int /*unused*/, char** /*unused*/ ) try {
  init_logging( nullopt );
  load_configs();
  game();
  return 0;

} catch( exception_with_bt const& e ) {
  logger->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    logger->error( "SDL error: {}", sdl_error );
  print_stack_trace( e.st, 4 );
  cleanup();
} catch( exception const& e ) {
  logger->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    logger->error( "SDL error: {}", sdl_error );
  cleanup();
  return 1;
}
