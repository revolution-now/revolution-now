/****************************************************************
**scratch.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: "Scratch" state for querying Lua behavior on value
*              value types.
*
*****************************************************************/
#include "scratch.hpp"

// luapp
#include "c-api.hpp"
#include "state.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lua.h"

using namespace std;

namespace lua {

state& scratch_state() {
  static state& st = []() -> state& {
    static state st;

    c_api C( st.main_cthread() );
    // Kill the global table since we're not supposed to every
    // use it on this scratch state.
    C.push( nil );
    C.rawseti( LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS );
    CHECK( C.stack_size() == 0 );
    // Verify.
    C.pushglobaltable();
    CHECK( C.type_of( -1 ) == e_lua_type::nil );
    C.pop();
    return st;
  }();
  return st;
}

} // namespace lua
