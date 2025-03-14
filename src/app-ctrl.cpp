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
#include "gui.hpp"
#include "iengine.hpp"
#include "lua.hpp"
#include "main-menu.hpp"
#include "menu-plane.hpp"
#include "omni.hpp"
#include "plane-stack.hpp"
#include "terminal.hpp"
#include "window.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/logger.hpp"
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Coroutine entry point.
*****************************************************************/
wait<> revolution_now( IEngine& engine, Planes& planes ) {
  lua::state st;
  lua_init( st );
  Terminal terminal( st );
  function<void( string_view )> const terminal_log =
      [&]( string_view const msg ) { terminal.log( msg ); };
  base::set_console_terminal( terminal_log );
  SCOPE_EXIT { base::set_console_terminal( nothing ); };
  lua::table::create_or_get( st["log"] )["console"] =
      [&]( string const& msg ) { terminal.log( msg ); };

  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  MenuPlane menu_plane( engine );
  OmniPlane omni_plane( engine, menu_plane );
  ConsolePlane console_plane( engine, terminal );
  WindowPlane window_plane( engine );
  group.omni    = omni_plane;
  group.console = console_plane;
  group.window  = window_plane;
  group.menu    = menu_plane;

  RealGui gui( planes, engine.textometer() );

  co_await run_main_menu( engine, planes, gui );
}

} // namespace rn
