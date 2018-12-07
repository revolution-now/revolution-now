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
  (void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
                            4_x );
  (void)create_unit_on_map( e_unit_type::caravel, 2_y, 2_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 1_x );

  while( turn() != e_turn_result::quit ) {}

  font_test();
  gui::test_window();

  cleanup();
}

#define LOG_CONFIG( path )                     \
  console->info( TO_STRING( rn::path ) ": {}", \
                 util::to_string( rn::path ) )

int main( int /*unused*/, char** /*unused*/ ) try {
  init_logging( nullopt );
  // init_logging( spdlog::level::trace );

  load_configs();

  LOG_CONFIG( config_rn.fruit.apples );
  LOG_CONFIG( config_rn.fruit.oranges );
  LOG_CONFIG( config_rn.fruit.description );
  LOG_CONFIG( config_rn.fruit.hello.world );
  LOG_CONFIG( config_rn.hello );
  LOG_CONFIG( config_rn.one );
  LOG_CONFIG( config_rn.two );
  LOG_CONFIG( config_window.game_version );
  LOG_CONFIG( config_window.game_title );
  LOG_CONFIG( config_window.window_error.title );
  LOG_CONFIG( config_window.window_error.x_size );
  LOG_CONFIG( config_window.window_error.show );
  LOG_CONFIG( config_window.widths );

  auto x = 55_x;
  (void)x;

  console->critical( "hello" );
  console->info( "hello" );
  LOG_DEBUG( "this is some debug logging" );
  LOG_TRACE( "this is some trace logging" );

  // fmt::print( "Hello, {}!\n", "world" );
  // auto s = fmt::format( "this {} a {}.\n", "is", "test" );
  // fmt::print( s );

  // game();
  return 0;

} catch( exception_with_bt const& e ) {
  console->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    console->error( "SDL error: {}", sdl_error );
  print_stack_trace( e.st, 4 );
  cleanup();
} catch( exception const& e ) {
  console->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    console->error( "SDL error: {}", sdl_error );
  cleanup();
  return 1;
}
