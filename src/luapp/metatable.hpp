/****************************************************************
**metatable.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: For getting metatables of things.
*
*****************************************************************/
#pragma once

// luapp
#include "ext.hpp"
#include "rtable.hpp"

namespace lua {

namespace detail {

int get_stack_top( cthread L );

base::maybe<table> try_get_metatable( cthread L,
                                      int restore_stack_top_to );

} // namespace detail

template<Pushable T>
base::maybe<table> metatable_for( cthread L, T&& o ) {
  int top = detail::get_stack_top( L );
  lua::push( L, std::forward<T>( o ) );
  return detail::try_get_metatable( L, top );
}

template<typename T>
requires( Pushable<T>&& HasCthread<T> )
    base::maybe<table> metatable_for( T&& o ) {
  cthread L = o.this_cthread();
  return metatable_for( L, std::forward<T>( o ) );
}

void setmetatable( table tbl, table meta );

} // namespace lua
