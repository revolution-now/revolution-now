#include "app-ctrl.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "linking.hpp"
#include "logger.hpp"
#include "lua-ui.hpp"
#include "lua.hpp"
#include "map-edit.hpp"
#include "map-gen.hpp"
#include "open-gl-test.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "util.hpp"

// Rds
#include "main.rds.hpp"

// base
#include "base/cli-args.hpp"
#include "base/error.hpp"
#include "base/keyval.hpp"

using namespace std;
using namespace base;

namespace rn {

wait<> test_ui() { return make_wait<>(); }
wait<> test_lua_ui() { return rn::lua_ui_test(); }

void run_map_gen_loop() {
  while( true ) {
    ascii_map_gen();
    fmt::print( "Press enter to regenerate...\n" );
    string s;
    cin >> s;
    if( s == "q" || s == "quit" ) break;
  }
}

rr::Renderer& renderer() {
  // This should be the only place where this function is called,
  // save for one or two other (hopefully temporary) hacks.
  return global_renderer_use_only_when_needed();
}

void full_init() {
  run_all_init_routines( e_log_level::debug );
  lua_reload();
  MapUpdater map_updater( GameState::terrain(), renderer() );
  run_lua_startup_main( map_updater );
}

void run( e_mode mode ) {
  switch( mode ) {
    case e_mode::game: {
      full_init();
      print_bar( '-', "[ Starting Game ]" );
      frame_loop( revolution_now(), renderer() );
      break;
    }
    case e_mode::map_editor: {
      full_init();
      print_bar( '-', "[ Starting Map Editor ]" );
      MapUpdater map_updater( GameState::terrain(), renderer() );
      frame_loop( map_editor( map_updater ), renderer() );
      break;
    }
    case e_mode::map_gen: {
      run_all_init_routines(
          e_log_level::debug,
          { e_init_routine::configs, e_init_routine::lua,
            e_init_routine::rng } );
      run_map_gen_loop();
      break;
    }
    case e_mode::test_ui: {
      full_init();
      frame_loop( test_ui(), renderer() );
      break;
    }
    case e_mode::test_lua_ui: {
      full_init();
      frame_loop( rn::test_lua_ui(), renderer() );
      break;
    }
    case e_mode::gl_test: {
      full_init();
      open_gl_test();
      break;
    }
  }
}

} // namespace rn

using namespace ::rn;

int main( int argc, char** argv ) {
  ProgramArguments args = base::parse_args_or_die_with_usage(
      vector<string>( argv + 1, argv + argc ) );

  auto mode = e_mode::game;
  if( args.key_val_args.contains( "mode" ) ) {
    UNWRAP_CHECK_MSG( m,
                      refl::enum_from_string<e_mode>(
                          args.key_val_args["mode"] ),
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
