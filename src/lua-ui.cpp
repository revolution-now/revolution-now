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
#include "lua.hpp"
#include "waitable-coro.hpp"
#include "window.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_lua_ui() {}

namespace {

LUA_MODULE();

LUA_FN( message_box, waitable<lua::any>, string_view msg ) {
  lua::state& st = lua_global_state();
  co_await ui::message_box( msg );
  co_return st.cast<lua::any>( lua::nil );
}

LUA_FN( ok_cancel, waitable<lua::any>, string_view msg ) {
  lua::state&     st  = lua_global_state();
  ui::e_ok_cancel res = co_await ui::ok_cancel( msg );
  switch( res ) {
    case ui::e_ok_cancel::ok:
      co_return st.cast<lua::any>( "ok" );
    case ui::e_ok_cancel::cancel:
      co_return st.cast<lua::any>( "cancel" );
  }
}

LUA_FN( str_input_box, waitable<lua::any>, string_view title,
        string_view msg, string_view initial_text ) {
  lua::state&   st = lua_global_state();
  maybe<string> res =
      co_await ui::str_input_box( title, msg, initial_text );
  co_return st.cast<lua::any>( res );
}

} // namespace

} // namespace rn
