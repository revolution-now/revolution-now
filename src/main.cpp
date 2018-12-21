#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "geo-types.hpp"
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

// base-util
#include "base-util/algo.hpp"
#include "base-util/io.hpp"
#include "base-util/string.hpp"

#include "absl/strings/str_split.h"

// {fmt}
#include "fmt/format.h"
#include "fmt/ostream.h"

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

using namespace rn;
using namespace std;

namespace rn {

void game() {
  init_game();
  load_sprites();
  load_tile_maps();

  // CHECK( play_music_file( "assets/music/bonny-morn.mp3" ) );

  //(void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
  //                          3_x );
  //(void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
  //                          4_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 2_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 1_x );

  // while( turn() != e_turn_result::quit ) {}

  // font_test();
  auto res =
      ui::yes_no( [] {}, "Would you like to make landfall?" );

  logger->info( "selected: {}", bool( res ) );
  logger->info( "bucket path: {}",
                bucket_path( Color{244, 179, 66, 255} ) );
  // grab_screen( "screen-shot-2.png" );

  // logger->info( "config_ui.window.border_color: {}",
  //              config_ui.window.border_color.to_string() );
  // logger->info(
  //    "config_ui.window.border_color: {}",
  //    config_ui.window.border_color.to_string( true ) );

  cleanup();
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  init_logging( spdlog::level::info );
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
