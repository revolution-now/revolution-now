#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
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

// clang-format off
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
// clang-format on

auto err_logger = spdlog::stderr_color_mt( "stderr" );
auto console    = spdlog::stdout_color_mt( "console" );

void stdout_example() {
  // create color multi threaded logger
  console->info( "Welcome to spdlog!" );
  console->error( "Some error message with arg: {}", 1 );

  err_logger->error( "Some error message" );

  // Formatting examples
  console->warn( "Easy padding in numbers like {:08d}",
                 12 ); // NOLINT
  console->critical(
      "Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: "
      "{0:b}",
      42 ); // NOLINT
  console->info( "Support for floats {:03.2f}",
                 1.23456 ); // NOLINT
  console->info( "Positional args are {1} {0}..", "too",
                 "supported" );
  console->info( "{:<30}", "left aligned" );

  spdlog::get( "console" )
      ->info(
          "loggers can be retrieved from a global registry "
          "using the spdlog::get(logger_name)" );

  // Runtime log levels
  spdlog::set_level(
      spdlog::level::info ); // Set global log level to info
  console->debug( "This message should not be displayed!" );
  console->set_level(
      spdlog::level::trace ); // Set specific logger's log level
  console->debug( "This message should be displayed.." );

  // Customize msg format for all loggers
  spdlog::set_pattern(
      "[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v" );
  console->info( "This an info message with custom format" );

  // Compile time log levels
  // define SPDLOG_DEBUG_ON or SPDLOG_TRACE_ON
  SPDLOG_TRACE( console,
                "Enabled only #ifdef SPDLOG_TRACE_ON..{} ,{}", 1,
                3.23 );
  SPDLOG_DEBUG( console,
                "Enabled only #ifdef SPDLOG_DEBUG_ON.. {} ,{}",
                1, 3.23 );
}

void game() {
  init_game();
  load_sprites();
  load_tile_maps();

  // CHECK( play_music_file( "assets/music/bonny-morn.mp3" ) );

  //(void)create_unit_on_map( e_unit_type::free_colonist, Y(2),
  // X(3) );
  (void)create_unit_on_map( e_unit_type::free_colonist, Y( 2 ),
                            X( 4 ) );
  (void)create_unit_on_map( e_unit_type::caravel, Y( 2 ),
                            X( 2 ) );
  //(void)create_unit_on_map( e_unit_type::caravel, Y(2), X(1) );

  while( turn() != e_turn_result::quit ) {}

  font_test();
  gui::test_window();

  cleanup();
}

#define LOG_CONFIG( path )                     \
  console->info( TO_STRING( rn::path ) ": {}", \
                 util::to_string( rn::path ) )

int main( int /*unused*/, char** /*unused*/ ) try {
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

  // fmt::print( "Hello, {}!\n", "world" );
  // auto s = fmt::format( "this {} a {}.\n", "is", "test" );
  // fmt::print( s );

  // stdout_example();
  // console->info( "fruit.oranges: {}",
  //               config<int>( "fruit.oranges" ) );

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
