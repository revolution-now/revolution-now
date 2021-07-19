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
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

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
    try {
      std::rethrow_exception( w.exception() );
    } catch( exception const& e ) { return e.what(); }
    SHOULD_NOT_BE_HERE;
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

} // namespace

namespace internal {

lua::rthread create_runner_coro( lua::cthread L ) {
  auto st = lua::state::view( L );
  // TODO: Find a way to preregister this.
  lua::rfunction runner =
      st["waitable"]["runner"].cast<lua::rfunction>();
  return st.thread.create_coro( runner );
}

void cleanup_coro( lua::rthread coro ) {
  (void)coro.resetthread();
  remove_lua_coroutine_if_queued( coro );
}

} // namespace internal

void linker_dont_discard_module_co_lua() {}

} // namespace rn
