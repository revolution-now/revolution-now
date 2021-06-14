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

using namespace std;

namespace lua {

void push_stateless_lua_c_function( cthread       L,
                                    LuaCFunction* func,
                                    int           upvalues ) {
  c_api C( L );
  C.push( func, upvalues );
}

} // namespace lua
