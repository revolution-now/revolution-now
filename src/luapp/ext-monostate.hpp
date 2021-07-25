/****************************************************************
**ext-monostate.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-25.
*
* Description: Lua Stackable implementation for std::monostate.
*
*****************************************************************/
#pragma once

#include "ext.hpp"

namespace lua {

// NOTE: unit tests for this are in test/ext-std.cpp.

/****************************************************************
** std::monostate
*****************************************************************/
// std::monostate just gets pushed as nil, and will only success-
// fully pop if there is a value of nil. The reason for this is
// that std::monostate is isomorphic to nil, in the sense that it
// is a type with just a single value.
void lua_push( cthread L, std::monostate const& );

base::maybe<std::monostate> lua_get( cthread L, int idx,
                                     tag<std::monostate> );

} // namespace lua
