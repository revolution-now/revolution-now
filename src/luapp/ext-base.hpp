/****************************************************************
**ext-base.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-16.
*
* Description: Lua push/get extensions for base library types.
*
*****************************************************************/
#pragma once

// luapp
#include "ext-userdata.hpp"
#include "ext.hpp"
#include "types.hpp"

// base
#include "base/maybe.hpp"

namespace lua {

/****************************************************************
** maybe
*****************************************************************/
template<typename T>
struct type_traits<base::maybe<T>> {
  using M = base::maybe<T>;

  static constexpr int nvalues = nvalues_for<T>();

  // Need an extra template parameter here so that this will work
  // with both cpp-owned and lua-owned T.
  template<typename U>
  static void push( cthread L, U&& m )
  requires Pushable<decltype( *std::forward<U>( m ) )> &&
           base::is_maybe_v<std::remove_cvref_t<U>>
  {
    if( m.has_value() )
      lua::push( L, *std::forward<U>( m ) );
    else {
      for( int i = 0; i < nvalues; ++i ) //
        lua::push( L, nil );
    }
  }

  static lua_expect<M> get( cthread L, int idx, tag<M> )
  requires Gettable<T>
  {
    if( type_of_idx( L, idx ) == type::nil )
      // Result will have a value because nil is a valid value
      // that can be converted to a maybe<T>.
      return M();
    // We don't want to just return the result of lua::get here
    // because then even if it fails, there will be a value in
    // the result. But we don't want this because if the value on
    // the stack is not nil, then it /must/ succeed in conversion
    // in order for the result to have a value.
    auto res = lua::get<T>( L, idx );
    if( !res.has_value() ) return res.error();
    return *res;
  }
};

template<typename T>
requires HasRefUserdataOwnershipModel<T>
struct type_traits<base::maybe<T>>
  : userdata_type_traits_cpp_owned<base::maybe<T>> {};

template<typename T>
requires HasValueUserdataOwnershipModel<T>
struct type_traits<base::maybe<T>>
  : userdata_type_traits_lua_owned<base::maybe<T>> {};

} // namespace lua
