/****************************************************************
**ext-std.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-22.
*
* Description: Lua push/get extensions for std library types.
*
*****************************************************************/
#include "ext-std.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

void lua_push( cthread L, monostate const& ) {
  lua_push( L, nil );
}

base::maybe<monostate> lua_get( cthread L, int idx,
                                tag<monostate> ) {
  c_api C( L );
  if( C.type_of( idx ) != type::nil ) return base::nothing;
  return monostate{};
}

} // namespace lua
