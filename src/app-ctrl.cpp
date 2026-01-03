/****************************************************************
**app-ctrl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Handles the top-level game state machines.
*
*****************************************************************/
#include "app-ctrl.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "console.hpp"
#include "frame-count.hpp"
#include "gui.hpp"
#include "iengine.hpp"
#include "lua.hpp"
#include "main-menu.hpp"
#include "menu-plane.hpp"
#include "omni.hpp"
#include "plane-stack.hpp"
#include "screen.hpp"
#include "terminal.hpp"
#include "window.hpp"

// config
#include "config/gfx.rds.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/logger.hpp"
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::e_resolution;

// This needs to be a coroutine so that we can skip one frame be-
// fore setting the resolution, otherwise if we don't then we
// will be setting it before one complete frame runs (because
// this is called at the start of the very first coroutine method
// which runs eagerly, as coroutines do). And if we set it before
// one complete frame runs then we will be inserting our resolu-
// tion event into our event queue before the actual window re-
// size events (from the real window) get pumped in, which will
// cause our resolution to get overridden.
wait<> force_resolution( IEngine& engine,
                         e_resolution const named ) {
  co_await 1_frames;
  change_resolution_to_named_if_available( engine.resolutions(),
                                           named );
}

} // namespace

/****************************************************************
** Coroutine entry point.
*****************************************************************/
wait<> revolution_now( IEngine& engine, Planes& planes ) {
  auto const& resolution_override =
      config_gfx.logical_resolution.force_if_available;
  if( resolution_override.has_value() )
    co_await force_resolution( engine, *resolution_override );

  Terminal terminal;
  function<void( string_view )> const terminal_log =
      [&]( string_view const msg ) { terminal.log( msg ); };
  base::set_console_terminal( terminal_log );
  SCOPE_EXIT { base::set_console_terminal( nothing ); };

  // This lua state is the one that is operative outside of indi-
  // vidual games, i.e. during the menus.
  lua::state st;
  lua_init( engine, st );

  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  MenuPlane menu_plane( engine );
  OmniPlane omni_plane( engine, menu_plane );
  ConsolePlane console_plane( engine, terminal, st );
  WindowPlane window_plane( engine );
  group.omni    = omni_plane;
  group.console = console_plane;
  group.window  = window_plane;
  group.menu    = menu_plane;

  RealGui gui( planes, engine.textometer() );

  co_await run_main_menu( engine, planes, gui );
}

} // namespace rn
