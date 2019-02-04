#include "config-files.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "fonts.hpp"
#include "geo-types.hpp"
#include "globals.hpp"
#include "image.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "loops.hpp"
#include "ownership.hpp"
#include "rand.hpp"
#include "sdl-util.hpp"
#include "sound.hpp"
#include "tiles.hpp"
#include "time.hpp"
#include "turn.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "viewport.hpp"
#include "window.hpp"
#include "world.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/io.hpp"

#include "absl/strings/str_split.h"

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#include <algorithm>
#include <functional>
#include <vector>

using namespace rn;
using namespace std;

namespace rn {

void game() {
  init_game();
  load_sprites();
  load_tile_maps();
  rng::init();

  // CHECK( play_music_file( "assets/music/bonny-morn.mp3" ) );

  for( Y y{1}; y < 1_y + 10_y; ++y ) {
    (void)create_unit_on_map( e_nation::dutch,
                              e_unit_type::soldier, y, 1_x );
    (void)create_unit_on_map( e_nation::french,
                              e_unit_type::soldier, y, 2_x );
    (void)create_unit_on_map( e_nation::dutch,
                              e_unit_type::soldier, y, 3_x );
    (void)create_unit_on_map( e_nation::french,
                              e_unit_type::soldier, y, 4_x );
    (void)create_unit_on_map( e_nation::dutch,
                              e_unit_type::soldier, y, 7_x );
    (void)create_unit_on_map( e_nation::french,
                              e_unit_type::soldier, y, 8_x );
    (void)create_unit_on_map( e_nation::dutch,
                              e_unit_type::soldier, y, 9_x );
    (void)create_unit_on_map( e_nation::french,
                              e_unit_type::soldier, y, 10_x );
    if( y._ % 2 == 0 ) {
      (void)create_unit_on_map( e_nation::dutch,
                                e_unit_type::caravel, y, 5_x );
      (void)create_unit_on_map( e_nation::french,
                                e_unit_type::privateer, y, 6_x );
    } else {
      (void)create_unit_on_map( e_nation::french,
                                e_unit_type::caravel, y, 5_x );
      (void)create_unit_on_map( e_nation::dutch,
                                e_unit_type::privateer, y, 6_x );
    }
  }

  //(void)create_unit_on_map(
  //    e_nation::spanish, e_unit_type::free_colonist, 4_y, 4_x
  //    );
  //(void)create_unit_on_map( e_nation::spanish,
  //                          e_unit_type::soldier, 4_y, 5_x );

  //(void)create_unit_on_map(
  //    e_nation::dutch, e_unit_type::free_colonist, 5_y, 4_x );
  //(void)create_unit_on_map( e_nation::dutch,
  //                          e_unit_type::soldier, 5_y, 5_x );

  while( turn() != e_turn_result::quit ) {}

  using namespace std::literals::chrono_literals;
  while( input::is_any_key_down() ) {}

  image_plane_set( e_image::old_world );
  image_plane_enable( true );
  frame_loop( true, [] { return input::is_any_key_down(); } );

  logger->info( "avg frame rate: {}", avg_frame_rate() );

  // font_test();
  cleanup();
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  init_logging( spdlog::level::debug );
  load_configs();
  game();
  return 0;

} catch( exception_exit const& ) {
  logger->info( "exiting due to exception_exit." );
  cleanup();
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
