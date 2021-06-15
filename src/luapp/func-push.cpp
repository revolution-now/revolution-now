/****************************************************************
**func-push.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: lua::push overload for C/C++ functions.
*
*****************************************************************/
#include "func-push.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/macros.hpp"

// Lua
#include "lua.h"

using namespace std;

namespace lua {

namespace detail {

void func_push_cpp_check_args( cthread L, int num_cpp_args ) {
  c_api C( L );
  int   num_lua_args = C.gettop();
  if( num_lua_args != num_cpp_args )
    throw_lua_error( L,
                     "Native function expected {} arguments, "
                     "but received {} from Lua.",
                     num_cpp_args, num_lua_args );
}

} // namespace detail

void push_stateless_lua_c_function( cthread       L,
                                    LuaCFunction* func,
                                    int           upvalues ) {
  c_api C( L );
  C.push( func, upvalues );
}

} // namespace lua
