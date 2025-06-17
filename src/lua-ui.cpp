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
#include "gui.hpp"
#include "iengine.hpp"
#include "lua-wait.hpp"
#include "lua.hpp"
#include "menu-plane.hpp"
#include "omni.hpp"
#include "plane-stack.hpp"
#include "ui-enums.rds.hpp"
#include "window.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-monostate.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_lua_ui();
void linker_dont_discard_module_lua_ui() {}

namespace {

// This is to ensure the module gets registered; TODO: remove
// once some other methods are registered again above.
LUA_FN( dummy, void ) {}

} // namespace

wait<> lua_ui_test( IEngine& engine, Planes& planes ) {
  auto owner            = planes.push();
  PlaneGroup& new_group = owner.group;
  MenuPlane menu_plane( engine );
  OmniPlane omni( engine, menu_plane );
  WindowPlane window_plane( engine );
  new_group.omni   = omni;
  new_group.window = window_plane;

  RealGui gui( planes, engine.textometer() );

  lua::state st;
  lua_init( st ); // expensive.

  lua::table M = st["lua_ui"].as<lua::table>();

  M["message_box"] = [&]( string_view const msg ) -> wait<> {
    co_await window_plane.message_box( msg );
  };

  M["str_input_box"] = [&]( string_view const msg,
                            string_view const initial_text )
      -> wait<maybe<string>> {
    co_return co_await window_plane.str_input_box(
        msg, WindowCancelActions{}, initial_text );
  };

  M["ok_cancel"] =
      [&]( string_view const msg ) -> wait<ui::e_ok_cancel> {
    EnumChoiceConfig const config{ .msg = string( msg ) };
    co_return co_await gui.required_enum_choice<ui::e_ok_cancel>(
        config );
  };

  auto const test = st["require"]( "test" );

  try {
    auto const n = co_await lua_wait<maybe<int>>(
        test["some_ui_routine"], 42 );
    lg.info( "received {} from some_ui_routine.", n );
  } catch( lua_error_exception const& e ) {
    lg.error( "lua_error_exception caught: {}", e.what() );
  }
}

} // namespace rn
