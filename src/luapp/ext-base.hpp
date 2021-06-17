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
template<Stackable T>
struct type_traits<base::maybe<T>> {
  using M = base::maybe<T>;

  static constexpr int nvalues = nvalues_for<T>();

  static void push( cthread L, M const& m ) {
    if( m.has_value() )
      lua::push( L, *m );
    else {
      for( int i = 0; i < nvalues; ++i ) //
        lua::push( L, nil );
    }
  }

  static base::maybe<M> get( cthread L, int idx, tag<M> ) {
    return lua::get<T>( L, idx );
  }
};

} // namespace lua
