#include "app-ctrl.hpp"
#include "console.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "linking.hpp"
#include "logger.hpp"
#include "lua-ui.hpp"
#include "map-edit.hpp"
#include "map-gen.hpp"
#include "omni.hpp"
#include "open-gl-test.hpp"
#include "plane-stack.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "util.hpp"

// Rds
#include "main.rds.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/cli-args.hpp"
#include "base/error.hpp"
#include "base/keyval.hpp"

using namespace std;
using namespace base;

namespace rn {
namespace {

wait<> test_ui() { return make_wait<>(); }
wait<> test_lua_ui() { return rn::lua_ui_test(); }

rr::Renderer& renderer() {
  // This should be the only place where this function is called,
  // save for one or two other (hopefully temporary) hacks.
  return global_renderer_use_only_when_needed();
}

void full_init() { run_all_init_routines( e_log_level::debug ); }

void run( e_mode mode ) {
  Planes planes;
  switch( mode ) {
    case e_mode::game: {
      full_init();
      print_bar( '-', "[ Starting Game ]" );
      frame_loop( planes, revolution_now( planes ), renderer() );
      print_bar( '-', "[ Shutting Down ]" );
      break;
    }
    case e_mode::map_editor: {
      full_init();
      print_bar( '-', "[ Starting Map Editor ]" );
      frame_loop( planes, run_map_editor_standalone( planes ),
                  renderer() );
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
      frame_loop( planes, test_ui(), renderer() );
      break;
    }
    case e_mode::test_lua_ui: {
      full_init();
      frame_loop( planes, rn::test_lua_ui(), renderer() );
      break;
    }
    case e_mode::gl_test: {
      full_init();
      open_gl_test();
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
  hide_window();
  run_all_cleanup_routines();
  return 0;
}
