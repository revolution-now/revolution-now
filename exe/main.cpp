#include "coord.hpp"
#include "errors.hpp"
#include "europort.hpp"
#include "fmt-helper.hpp"
#include "frame.hpp"
#include "fsm.hpp"
#include "init.hpp"
#include "input.hpp"
#include "linking.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "ownership.hpp"
#include "unit.hpp"
#include "window.hpp"

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
  UnitId id;

  id = create_unit_on_map(
      e_nation::spanish, e_unit_type::free_colonist, 2_y, 2_x );
  unit_from_id( id ).fortify();

  id = create_unit_on_map( e_nation::spanish,
                           e_unit_type::soldier, 2_y, 3_x );
  unit_from_id( id ).sentry();

  (void)create_unit_on_map( e_nation::spanish,
                            e_unit_type::privateer, 2_y, 6_x );

  id = create_unit_on_map( e_nation::english,
                           e_unit_type::soldier, 3_y, 2_x );
  unit_from_id( id ).fortify();

  id = create_unit_on_map( e_nation::english,
                           e_unit_type::soldier, 3_y, 3_x );
  unit_from_id( id ).sentry();

  id = create_unit_on_map( e_nation::english,
                           e_unit_type::privateer, 3_y, 6_x );

  // while( turn() != e_turn_result::quit ) {}

  // using namespace std::literals::chrono_literals;
  // while( input::is_any_key_down() ) {}

  frame_loop( true, [] { return false; } );
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  linker_dont_discard_me();
  run_all_init_routines( /*level=*/nullopt );
  lua::load_modules();
  lua::run_startup();

  game();

  // ui::window_test();

  // lua::test_lua();

  run_all_cleanup_routines();
  return 0;

} catch( exception_exit const& ) {
  lg.info( "exiting due to exception_exit." );
  run_all_cleanup_routines();
} catch( exception_with_bt const& e ) {
  lg.error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    lg.error( "SDL error (may be a false positive): {}",
              sdl_error );
  print_stack_trace( e.st, 4 );
  run_all_cleanup_routines();
} catch( exception const& e ) {
  lg.error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    lg.error( "SDL error (may be a false positive): {}",
              sdl_error );
  run_all_cleanup_routines();
  return 1;
}
