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
#include "ext.hpp"
#include "types.hpp"

// base
#include "maybe.hpp"

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
  template<typename U> // clang-format off
  static void push( cthread L, U&& m )
    requires Pushable<decltype(*std::forward<U>( m ))> &&
             base::is_maybe_v<std::remove_cvref_t<U>> {
    // clang-format on
    if( m.has_value() )
      lua::push( L, *std::forward<U>( m ) );
    else {
      for( int i = 0; i < nvalues; ++i ) //
        lua::push( L, nil );
    }
  }

  // clang-format off
  static base::maybe<M> get( cthread L, int idx, tag<M> )
    requires Gettable<T> {
    // clang-format on
    return lua::get<T>( L, idx );
  }
};

} // namespace lua
