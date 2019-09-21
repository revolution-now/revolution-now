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
#include "turn.hpp"
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
  while( turn() != e_turn_result::quit ) {}

  // using namespace std::literals::chrono_literals;
  // while( input::is_any_key_down() ) {}

  // frame_loop( true, [] { return false; } );
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  linker_dont_discard_me();
  run_all_init_routines( nullopt );
  // run_all_init_routines( nullopt, {e_init_routine::lua} );
  lua::reload();
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
