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

// Revolution Now
#include "co-lua-scheduler.hpp"
#include "lua.hpp"

// luapp
#include "luapp/c-api.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// Lua
#include "lua.h"

using namespace std;

namespace rn {

namespace {

LUA_MODULE();

LUA_STARTUP( lua::state& st ) {
  using W = waitable<lua::any>;
  auto ut = st.usertype.create<W>();

  ut["cancel"] = &W::cancel;
  ut["ready"]  = &W::ready;
  ut["get"]    = []( W& w ) { return w.get(); };

  ut["error"] = []( waitable<lua::any>& w ) -> maybe<string> {
    if( !w.has_exception() ) return nothing;
    return base::rethrow_and_get_msg( w.exception() );
  };

  ut["set_resume"] = []( W& w, lua::rthread coro ) {
    w.shared_state()->add_callback( [coro]( lua::any const& ) {
      queue_lua_coroutine( coro );
    } );
    w.shared_state()->set_exception_callback(
        [coro]( std::exception_ptr ) {
          queue_lua_coroutine( coro );
        } );
  };
  ut[lua::metatable_key]["__close"] =
      []( W& w, lua::any /*error_object*/ ) { w.cancel(); };
};

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

  lua::lua_valid res = lua::c_api( L ).pcallk(
      /*nargs=*/nargs,
      /*nresults=*/1, /*ctx=*/0, /*k=*/coro_continuation );

  // NOTE: we only get here if the function did not yield. At
  // this point we just call the continuation function for conve-
  // nience so that we don't have to duplicate code.
  return coro_continuation( L, res ? LUA_OK : LUA_ERRRUN, 0 );
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
