#include "auto-pad.hpp"
#include "conductor.hpp"
#include "coord.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "fonts.hpp"
#include "frame.hpp"
#include "image.hpp"
#include "init.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "midiplayer.hpp"
#include "midiseq.hpp"
#include "mplayer.hpp"
#include "oggplayer.hpp"
#include "ownership.hpp"
#include "rand.hpp"
#include "ranges.hpp"
#include "screen.hpp"
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
  // auto id1 = create_unit_on_map(
  //    e_nation::spanish, e_unit_type::free_colonist, 2_y, 2_x
  //    );
  // unit_from_id( id1 ).fortify();

  // auto id2 = create_unit_on_map(
  //    e_nation::spanish, e_unit_type::soldier, 2_y, 3_x );
  // unit_from_id( id2 ).sentry();

  //(void)create_unit_on_map( e_nation::spanish,
  //                          e_unit_type::privateer, 2_y, 6_x );

  // auto id3 = create_unit_on_map(
  //    e_nation::english, e_unit_type::soldier, 3_y, 2_x );
  // unit_from_id( id3 ).fortify();

  // auto id4 = create_unit_on_map(
  //    e_nation::english, e_unit_type::soldier, 3_y, 3_x );
  // unit_from_id( id4 ).sentry();

  //(void)create_unit_on_map( e_nation::english,
  //                          e_unit_type::privateer, 3_y, 6_x );

  // while( turn() != e_turn_result::quit ) {}

  // using namespace std::literals::chrono_literals;
  // while( input::is_any_key_down() ) {}

  // image_plane_set( e_image::old_world );
  // image_plane_enable( true );
  frame_loop( true, [] { return input::is_q_down(); } );
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  run_all_init_routines( e_init_routine::europe );
  // run_all_init_routines();

  // conductor::test();

  game();
  // ui::window_test();

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
