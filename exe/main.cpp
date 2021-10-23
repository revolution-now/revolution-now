#include "app-ctrl.hpp"
#include "fmt-helper.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "linking.hpp"
#include "logger.hpp"
#include "lua-ui.hpp"
#include "lua.hpp"
#include "open-gl-perf-test.hpp"
#include "open-gl.hpp"
#include "screen.hpp"
#include "util.hpp"

using namespace rn;
using namespace std;

enum class e_mode {
  game,
  ui_test,
  lua_ui_test,
  gl_test,
  gl_perf
};

waitable<> ui_test() { return make_waitable<>(); }
waitable<> lua_ui_test() { return rn::lua_ui_test(); }

void full_init() {
  run_all_init_routines( e_log_level::debug );
  lua_reload();
  run_lua_startup_main();
}

void run( e_mode mode ) {
  switch( mode ) {
    case e_mode::game:
      full_init();
      print_bar( '-', "[ Starting Game ]" );
      frame_loop( revolution_now() );
      break;
    case e_mode::ui_test: //
      full_init();
      frame_loop( ui_test() );
      break;
    case e_mode::lua_ui_test: //
      full_init();
      frame_loop( rn::lua_ui_test() );
      break;
    case e_mode::gl_test:
      run_all_init_routines(
          e_log_level::debug,
          { e_init_routine::screen, e_init_routine::lua } );
      test_open_gl();
      break;
    case e_mode::gl_perf:
      run_all_init_routines(
          e_log_level::debug,
          { e_init_routine::screen, e_init_routine::lua } );
      open_gl_perf_test();
      break;
  }
}

int main( int /*unused*/, char** /*unused*/ ) {
  linker_dont_discard_me();
  print_bar( '=', "[ Revolution | Now ]" );
  try {
    run( e_mode::gl_perf );
  } catch( exception_exit const& ) {}
  hide_window();
  print_bar( '-', "[ Shutting Down ]" );
  run_all_cleanup_routines();
  return 0;
}
