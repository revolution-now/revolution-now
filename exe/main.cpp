#include "app-state.hpp"
#include "fmt-helper.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "linking.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "open-gl.hpp"
#include "screen.hpp"
#include "util.hpp"

using namespace rn;
using namespace std;

bool do_game = true;

int main( int /*unused*/, char** /*unused*/ ) {
  linker_dont_discard_me();
  print_bar( '=', "[ Revolution | Now ]" );
  try {
    if( do_game ) {
      run_all_init_routines( e_log_level::debug );
      lua::reload();
      lua::run_startup_main();
      print_bar( '-', "[ Starting Game ]" );
      // The action.
      frame_loop( revolution_now() );
    } else {
      run_all_init_routines(
          e_log_level::debug,
          { e_init_routine::screen, e_init_routine::lua } );
      test_open_gl();
    }

  } catch( exception_exit const& ) {}

  hide_window();
  print_bar( '-', "[ Shutting Down ]" );
  run_all_cleanup_routines();
  return 0;
}
