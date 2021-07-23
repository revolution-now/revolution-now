/****************************************************************
**co-lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-01.
*
* Description: Helpers for calling C++ coroutines from Lua.
*
*****************************************************************/
#include "co-lua.hpp"

// luapp
#include "luapp/c-api.hpp"
#include "luapp/state.hpp"

// Lua
#include "lua.h"

using namespace std;

namespace rn {

namespace {

int coro_continuation( lua_State* L, int status, lua_KContext ) {
  // Stack:
  //  -1 result (could be error)
  //  -2 message handler that C.pcallk pushed onto the stack
  //     which it wasn't able to pop.
  //  -3 set_error
  //  -4 set_result
  DCHECK( lua::c_api( L ).gettop() == 4 );
  // Here we are supposed to have the same set of parameters that
  // we had when we entered coro_runner.
  auto set_result = lua::get_or_luaerr<lua::rfunction>( L, 1 );
  auto set_error  = lua::get_or_luaerr<lua::rfunction>( L, 2 );
  // We get LUA_OK if there was no yielding (in which case this
  // continuation function was called manually by us below), or
  // LUA_YIELD if pcallk yielded in which Lua called this contin-
  // uation function. But LUA_YIELD here does not mean that the
  // function is still yielded, it just means that it yielded at
  // least once.
  if( status == LUA_OK || status == LUA_YIELD )
    set_result( lua::get<lua::any>( L, -1 ) );
  else // error
    set_error( lua::get<string>( L, -1 ) );
  return 0;
}

int coro_runner( lua_State* L ) {
  int nargs = lua::c_api( L ).gettop() - 3;

  // We must call lua_pcallk here and now lua_callk so that that
  // if an error happens after a yield it will call the continua-
  // tion which will set the exception in the downstream wait-
  // able. This will also cause errors that happen before the
  // first yield to go to the continuation, so we don't have to
  // rely on the lua_resume return values to tell us whether
  // there was such an error.
  lua::c_api( L ).pcallk( /*nargs=*/nargs, /*nresults=*/1,
                          /*ctx=*/0, /*k=*/coro_continuation );

  // Because we called pcallk and because we specified a continu-
  // ation, we only get here if the function finished without er-
  // rors and did not yield. At this point we just call the con-
  // tinuation function for convenience so that we don't have to
  // duplicate code.
  return coro_continuation( L, LUA_OK, 0 );
}

} // namespace

namespace internal {

lua::rthread create_runner_coro( lua::cthread L ) {
  auto st = lua::state::view( L );
  return st.thread.create_coro(
      st.cast<lua::rfunction>( &coro_runner ) );
}

void cleanup_coro( lua::rthread coro ) {
  (void)coro.resetthread();
  remove_lua_coroutine_if_queued( coro );
}

} // namespace internal

void linker_dont_discard_module_co_lua() {}

} // namespace rn
