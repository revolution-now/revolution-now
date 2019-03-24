#include "config-files.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "fonts.hpp"
#include "frame.hpp"
#include "geo-types.hpp"
#include "image.hpp"
#include "init.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "rand.hpp"
#include "ranges.hpp"
#include "sdl-util.hpp"
#include "sound.hpp"
#include "terrain.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "time.hpp"
#include "turn.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "viewport.hpp"
#include "window.hpp"

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
  // CHECK( play_music_file( "assets/music/bonny-morn.mp3" ) );

  for( Y y{1}; y < 1_y + 1_y; ++y ) {
    (void)create_unit_on_map( e_nation::spanish,
                              e_unit_type::soldier, y, 2_x );
    (void)create_unit_on_map( e_nation::english,
                              e_unit_type::soldier, y, 3_x );
    (void)create_unit_on_map( e_nation::spanish,
                              e_unit_type::soldier, y, 8_x );
    (void)create_unit_on_map( e_nation::english,
                              e_unit_type::soldier, y, 9_x );
    if( y._ % 2 == 0 ) {
      (void)create_unit_on_map( e_nation::english,
                                e_unit_type::caravel, y, 5_x );
      (void)create_unit_on_map( e_nation::spanish,
                                e_unit_type::privateer, y, 6_x );
    } else {
      (void)create_unit_on_map( e_nation::spanish,
                                e_unit_type::caravel, y, 5_x );
      (void)create_unit_on_map( e_nation::english,
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
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  run_all_init_routines();
  ui::window_test();
  // game();
  // font_test();
  run_all_cleanup_routines();
  return 0;

} catch( exception_exit const& ) {
  logger->info( "exiting due to exception_exit." );
  run_all_cleanup_routines();
} catch( exception_with_bt const& e ) {
  logger->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    logger->error( "SDL error: {}", sdl_error );
  print_stack_trace( e.st, 4 );
  run_all_cleanup_routines();
} catch( exception const& e ) {
  logger->error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    logger->error( "SDL error: {}", sdl_error );
  run_all_cleanup_routines();
  return 1;
}
