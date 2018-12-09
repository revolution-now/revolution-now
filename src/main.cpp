#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "geo-types.hpp"
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
#include "fmt/ostream.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

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
  (void)create_unit_on_map( e_unit_type::free_colonist, 2_y,
                            4_x );
  (void)create_unit_on_map( e_unit_type::caravel, 2_y, 2_x );
  //(void)create_unit_on_map( e_unit_type::caravel, 2_y, 1_x );

  while( turn() != e_turn_result::quit ) {}

  // font_test();
  // gui::test_window();

  cleanup();
}

template<typename T>
ostream& operator<<( ostream& out, std::vector<T> v ) {
  out << "[";
  bool need_comma = false;
  for( auto const& item : v ) {
    if( need_comma ) out << ",";
    need_comma = true;
    out << item;
  }
  out << "]";
  return out;
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  init_logging( spdlog::level::debug );
  load_configs();
  // clang-format off
  logger->info( "config/rn.ucl.hello:                      {}", util::to_string( config_rn.hello                   ) );
  logger->info( "config/rn.ucl.one:                        {}", util::to_string( config_rn.one                     ) );
  logger->info( "config/rn.ucl.two:                        {}", util::to_string( config_rn.two                     ) );
  logger->info( "config/rn.ucl.fruit.apples:               {}", util::to_string( config_rn.fruit.apples            ) );
  logger->info( "config/rn.ucl.fruit.oranges:              {}", util::to_string( config_rn.fruit.oranges           ) );
  logger->info( "config/rn.ucl.fruit.grapes:               {}", util::to_string( config_rn.fruit.grapes            ) );
  logger->info( "config/rn.ucl.fruit.description:          {}", util::to_string( config_rn.fruit.description       ) );
  logger->info( "config/rn.ucl.fruit.hello.world:          {}", util::to_string( config_rn.fruit.hello.world       ) );
  logger->info( "config/window.ucl.game_title:             {}", util::to_string( config_window.game_title          ) );
  logger->info( "config/window.ucl.game_version:           {}", util::to_string( config_window.game_version        ) );
  logger->info( "config/window.ucl.coordinates:            {}", config_window.coordinates );
  logger->info( "config/window.ucl.grapes:                 {}", util::to_string( *config_window.grapes             ) );
  logger->info( "config/window.ucl.window_error.title:     {}", util::to_string( config_window.window_error.title  ) );
  logger->info( "config/window.ucl.window_error.x_size:    {}", util::to_string( config_window.window_error.x_size ) );
  logger->info( "config/window.ucl.window_error.show:      {}", util::to_string( config_window.window_error.show   ) );
  logger->info( "config/window.ucl.window_error.positions: {}", config_window.window_error.positions );
  logger->info( "config/window.ucl.window_error.world:     {}", util::to_string( *config_window.window_error.world ) );
  logger->info( "config/window.ucl.widths:                 {}", util::to_string( config_window.widths              ) );
  logger->info( "config/window.ucl.fruit->oranges:         {}", util::to_string( config_window.fruit->oranges      ) );

  // clang-format on
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
