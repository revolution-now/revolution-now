/****************************************************************
**enum.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-23.
*
* Description: Lua extension for reflected enums.
*
*****************************************************************/
#include "enum.hpp"

// luapp
#include "types.hpp"

using namespace std;

namespace lua {

namespace detail {

base::maybe<std::string> get_str_from_stack( cthread L,
                                             int     idx ) {
  return ::lua::get<std::string>( L, idx );
}

void push_str_to_stack( cthread L, std::string_view name ) {
  ::lua::push( L, name );
}

} // namespace detail

} // namespace lua
