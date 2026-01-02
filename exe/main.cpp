/****************************************************************
**main.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2018-08-24.
*
* Description: The Revolution Begins.
*
*****************************************************************/
// rds
#include "main.rds.hpp"

// Revolution Now
#include "app-ctrl.hpp"
#include "engine.hpp"
#include "error.hpp"
#include "frame.hpp"
#include "interrupts.hpp"
#include "linking.hpp"
#include "lua-ui.hpp"
#include "map-edit.hpp"
#include "map-gen.hpp"
#include "plane-stack.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/cli-args.hpp"
#include "base/error.hpp"
#include "base/logger.hpp"
#include "base/scope-exit.hpp"
#include "base/stack-trace.hpp"

using namespace std;
using namespace base;

extern "C" char const* SDL_GetError();

namespace rn {
namespace {

wait<> test_ui() { return make_wait<>(); }
wait<> test_lua_ui( IEngine& engine, Planes& planes ) {
  return rn::lua_ui_test( engine, planes );
}

void run( e_mode mode ) {
  Planes planes;
  Engine engine;
  auto const cleanup_engine_on_abort = [&] {
    string const sdl_error = ::SDL_GetError();
    engine.deinit();
    if( !sdl_error.empty() )
      lg.error( "SDL error (may be a false positive): {}",
                sdl_error );
  };
  register_cleanup_callback_on_abort( cleanup_engine_on_abort );
  // If we are leaving this scope naturally then the engine will
  // clean up itself; the callback is only for check failing.
  SCOPE_EXIT { register_cleanup_callback_on_abort( nothing ); };
  init_logger( e_log_level::debug );
  switch( mode ) {
    case e_mode::game: {
      engine.init( e_engine_mode::game );
      print_bar( '-', "[ Starting Game ]" );
      frame_loop( engine, planes,
                  revolution_now( engine, planes ) );
      print_bar( '-', "[ Shutting Down ]" );
      break;
    }
    case e_mode::map_editor: {
      engine.init( e_engine_mode::map_editor );
      print_bar( '-', "[ Starting Map Editor ]" );
      frame_loop( engine, planes,
                  run_map_editor_standalone( engine, planes ) );
      break;
    }
    case e_mode::map_gen: {
      engine.init( e_engine_mode::console );
      ascii_map_gen( engine );
      break;
    }
    case e_mode::test_ui: {
      engine.init( e_engine_mode::ui_test );
      frame_loop( engine, planes, test_ui() );
      break;
    }
    case e_mode::test_lua_ui: {
      engine.init( e_engine_mode::ui_test );
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
  while( true ) {
    try {
      run( mode );
      break;
    } catch( exception_restart const& e ) {
      lg.info( "restarting game: {}", e.what() );
      continue;
    } catch( exception_hard_exit const& ) {
      lg.info( "received hard exit interrupt." );
      break;
    }
  }
  return 0;
}
