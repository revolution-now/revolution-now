/****************************************************************
**lua-ui.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-19.
*
* Description: Exposes some UI functions to Lua.
*
*****************************************************************/
#include "lua-ui.hpp"

// Revolution Now
#include "co-lua.hpp"
#include "co-wait.hpp"
#include "logger.hpp"
#include "lua-wait.hpp"
#include "lua.hpp"
#include "plane-stack.hpp"
#include "window.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_lua_ui();
void linker_dont_discard_module_lua_ui() {}

namespace {

// LUA_AUTO_FN_NAMED( "message_box", ui::message_box_basic );
// LUA_AUTO_FN_NAMED( "str_input_box", ui::str_input_box );

// LUA_FN( ok_cancel, wait<string>, string_view msg ) {
//   ui::e_ok_cancel res = co_await ui::ok_cancel( msg );
//   co_return fmt::to_string( res );
// }

// This is to ensure the module gets registered; TODO: remove
// once some other methods are registered again above.
LUA_FN( dummy, void ) {}

} // namespace

wait<> lua_ui_test( Planes& planes ) {
  auto        owner     = planes.push();
  PlaneGroup& new_group = owner.group;
  WindowPlane window_plane;
  new_group.window = window_plane;

  lua::state st;
  lua_init( st ); // expensive.

  auto n = co_await lua_wait<maybe<int>>(
      st["test"]["some_ui_routine"], 42 );

  lg.info( "received {} from some_ui_routine.", n );
}

} // namespace rn
