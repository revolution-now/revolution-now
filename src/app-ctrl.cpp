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
#include "logger.hpp"
#include "lua.hpp"
#include "main-menu.hpp"
#include "menu-plane.hpp"
#include "menu.hpp"
#include "omni.hpp"
#include "plane-stack.hpp"
#include "terminal.hpp"
#include "window.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Coroutine entry point.
*****************************************************************/
wait<> revolution_now( Planes& planes ) {
  lua::state st;
  lua_init( st );
  Terminal terminal( st );
  set_console_terminal( &terminal );
  SCOPE_EXIT { set_console_terminal( nullptr ); };
  lua::table::create_or_get( st["log"] )["console"] =
      [&]( string const& msg ) { terminal.log( msg ); };

  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  MenuPlane menu_plane;
  Menu2Plane menu2_plane;
  OmniPlane omni_plane( menu_plane );
  ConsolePlane console_plane( terminal );
  WindowPlane window_plane;
  group.omni    = omni_plane;
  group.console = console_plane;
  group.window  = window_plane;
  group.menu    = menu_plane;
  group.menu2   = menu2_plane;

  RealGui gui( planes );

  co_await run_main_menu( planes, gui );
}

} // namespace rn
