#include "app-ctrl.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "linking.hpp"
#include "logger.hpp"
#include "lua-ui.hpp"
#include "lua.hpp"
#include "open-gl-perf-test.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "util.hpp"

// base
#include "base/cli-args.hpp"
#include "base/error.hpp"
#include "base/keyval.hpp"

using namespace rn;
using namespace std;
using namespace base;

enum class e_mode { game, ui_test, lua_ui_test, gl_perf };

// FIXME: this should be using reflection.
maybe<e_mode> mode_from_str( string_view key ) {
  return base::lookup(
      unordered_map<string_view, e_mode>{
          { "game", e_mode::game },
          { "ui_test", e_mode::ui_test },
          { "lua_ui_test", e_mode::lua_ui_test },
          { "gl_perf", e_mode::gl_perf },
      },
      key );
}

wait<> ui_test() { return make_wait<>(); }
wait<> lua_ui_test() { return rn::lua_ui_test(); }

void full_init() {
  run_all_init_routines( e_log_level::debug );
  lua_reload();
  run_lua_startup_main();
}

void run( e_mode mode ) {
  switch( mode ) {
    case e_mode::game: {
      full_init();
      print_bar( '-', "[ Starting Game ]" );
      rr::Renderer& renderer =
          global_renderer_use_only_when_needed();
      frame_loop( renderer, revolution_now() );
      break;
    }
    case e_mode::ui_test: {
      full_init();
      rr::Renderer& renderer =
          global_renderer_use_only_when_needed();
      frame_loop( renderer, ui_test() );
      break;
    }
    case e_mode::lua_ui_test: {
      full_init();
      rr::Renderer& renderer =
          global_renderer_use_only_when_needed();
      frame_loop( renderer, rn::lua_ui_test() );
      break;
    }
    case e_mode::gl_perf: {
      run_all_init_routines(
          e_log_level::debug,
          { e_init_routine::screen, e_init_routine::lua } );
      open_gl_perf_test();
      break;
    }
  }
}

int main( int argc, char** argv ) {
  ProgramArguments args = base::parse_args_or_die_with_usage(
      vector<string>( argv + 1, argv + argc ) );

  auto mode = e_mode::gl_perf;
  if( args.key_val_args.contains( "mode" ) ) {
    UNWRAP_CHECK_MSG( m,
                      mode_from_str( args.key_val_args["mode"] ),
                      "invalid program mode: `{}'.",
                      args.key_val_args["mode"] );
    mode = m;
  }

  linker_dont_discard_me();
  try {
    run( mode );
  } catch( exception_exit const& ) {}
  hide_window();
  print_bar( '-', "[ Shutting Down ]" );
  run_all_cleanup_routines();
  return 0;
}
