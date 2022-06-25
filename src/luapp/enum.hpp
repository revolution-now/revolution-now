/****************************************************************
**enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-23.
*
* Description: Lua extension for reflected enums.
*
*****************************************************************/
#pragma once

// luapp
#include "ext.hpp"

// refl
#include "refl/query-enum.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace lua {

namespace detail {

// These so so that we don't have to include types.hpp here.

base::maybe<std::string> get_str_from_stack( cthread L,
                                             int     idx );

void push_str_to_stack( cthread L, std::string_view name );

}

template<refl::ReflectedEnum Enum>
base::maybe<Enum> lua_get( cthread L, int idx, tag<Enum> ) {
  base::maybe<std::string> m =
      detail::get_str_from_stack( L, idx );
  if( !m ) return base::nothing;
  return refl::enum_from_string<Enum>( *m );
}

template<refl::ReflectedEnum Enum>
void lua_push( cthread L, Enum val ) {
  detail::push_str_to_stack( L, refl::enum_value_name( val ) );
}

} // namespace lua
