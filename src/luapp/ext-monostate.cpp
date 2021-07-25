/****************************************************************
**ext-monostate.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-25.
*
* Description: Lua Stackable implementation for std::monostate.
*
*****************************************************************/
#include "ext-monostate.hpp"

// luapp
#include "c-api.hpp"
#include "types.hpp"

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
