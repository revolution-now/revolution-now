#include "app-ctrl.hpp"
#include "console.hpp"
#include "engine.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "linking.hpp"
#include "logger.hpp"
#include "lua-ui.hpp"
#include "map-edit.hpp"
#include "map-gen.hpp"
#include "omni.hpp"
#include "plane-stack.hpp"
#include "tiles.hpp" // FIXME
#include "util.hpp"

// rds
#include "main.rds.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/cli-args.hpp"
#include "base/error.hpp"
#include "base/keyval.hpp"
#include "base/scope-exit.hpp"

using namespace std;
using namespace base;

namespace rn {
namespace {

wait<> test_ui() { return make_wait<>(); }
wait<> test_lua_ui( IEngine& engine, Planes& planes ) {
  return rn::lua_ui_test( engine, planes );
}

void full_init() { run_all_init_routines( e_log_level::debug ); }

void run( e_mode mode ) {
  Planes planes;
  Engine engine;
  switch( mode ) {
    case e_mode::game: {
      init_logger( e_log_level::debug );

      run_init_routine( engine, e_init_routine::configs );
      run_init_routine( engine, e_init_routine::sdl );

      engine.init( e_engine_mode::game );

      init_sprites( engine.renderer_use_only_when_needed() );
      run_init_routine( engine, e_init_routine::compositor );
      run_init_routine( engine, e_init_routine::sound );
      run_init_routine( engine, e_init_routine::midiseq );
      run_init_routine( engine, e_init_routine::tunes );
      run_init_routine( engine, e_init_routine::midiplayer );
      run_init_routine( engine, e_init_routine::oggplayer );
      run_init_routine( engine, e_init_routine::conductor );

      print_bar( '-', "[ Starting Game ]" );
      frame_loop( engine, planes,
                  revolution_now( engine, planes ) );
      print_bar( '-', "[ Shutting Down ]" );
      break;
    }
    case e_mode::map_editor: {
      full_init();
      print_bar( '-', "[ Starting Map Editor ]" );
      frame_loop( engine, planes,
                  run_map_editor_standalone( engine, planes ) );
      break;
    }
    case e_mode::map_gen: {
      run_all_init_routines( e_log_level::warn,
                             { e_init_routine::configs } );
      NOT_IMPLEMENTED;
      // ascii_map_gen();
      // break;
    }
    case e_mode::test_ui: {
      full_init();
      frame_loop( engine, planes, test_ui() );
      break;
    }
    case e_mode::test_lua_ui: {
      full_init();
      frame_loop( engine, planes,
                  rn::test_lua_ui( engine, planes ) );
      break;
    }
  }
}

} // namespace
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
  run_all_cleanup_routines();
  return 0;
}
