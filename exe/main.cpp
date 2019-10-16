#include "errors.hpp"
#include "fmt-helper.hpp"
#include "init.hpp"
#include "linking.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "serial.hpp"
#include "turn.hpp"

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
  // while( input::is_any_key_down() ) {}
}

} // namespace rn

int main( int /*unused*/, char** /*unused*/ ) try {
  linker_dont_discard_me();
  run_all_init_routines( e_log_level::debug );
  lua::reload();
  lua::run_startup_main();

  game();

  // serial::test_serial();

  run_all_cleanup_routines();
  return 0;

} catch( exception_exit const& ) {
  lg.info( "exiting due to exception_exit." );
  run_all_cleanup_routines();
  return 0;
} catch( exception_with_bt const& e ) {
  lg.error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    lg.error( "SDL error (may be a false positive): {}",
              sdl_error );
  print_stack_trace( e.st, 4 );
  run_all_cleanup_routines();
  return 1;
} catch( exception const& e ) {
  lg.error( e.what() );
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    lg.error( "SDL error (may be a false positive): {}",
              sdl_error );
  run_all_cleanup_routines();
  return 1;
}
