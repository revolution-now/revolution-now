#include "app-ctrl.hpp"
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

enum class e_mode { game, ui_test, gl_test };

waitable<> ui_test() {
  ScopedPlanePush pusher( e_plane_config::black );
  return make_waitable<>();
}

void run( e_mode mode ) {
  switch( mode ) {
    case e_mode::game:
      run_all_init_routines( e_log_level::debug );
      lua_reload();
      run_lua_startup_main();
      print_bar( '-', "[ Starting Game ]" );
      // The action.
      frame_loop( revolution_now() );
      break;
    case e_mode::ui_test: //
      run_all_init_routines( e_log_level::debug );
      lua_reload();
      run_lua_startup_main();
      frame_loop( ui_test() );
      break;
    case e_mode::gl_test:
      run_all_init_routines(
          e_log_level::debug,
          { e_init_routine::screen, e_init_routine::lua } );
      test_open_gl();
      break;
  }
}

int main( int /*unused*/, char** /*unused*/ ) {
  linker_dont_discard_me();
  print_bar( '=', "[ Revolution | Now ]" );
  try {
    run( e_mode::game );
  } catch( exception_exit const& ) {}
  hide_window();
  print_bar( '-', "[ Shutting Down ]" );
  run_all_cleanup_routines();
  return 0;
}
