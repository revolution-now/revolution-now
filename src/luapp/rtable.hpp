/****************************************************************
**rtable.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua tables.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "call.hpp"
#include "indexer.hpp"

// base
#include "base/fmt.hpp"
#include "base/macros.hpp"

namespace lua {

/****************************************************************
** table
*****************************************************************/
struct table : public any {
  using Base = any;

  using Base::Base;

  friend base::maybe<table> lua_get( cthread L, int idx,
                                     tag<table> );

  template<typename U>
  auto operator[]( U&& idx ) noexcept {
    return indexer<U, table>( std::forward<U>( idx ), *this );
  }

  template<Pushable... Args>
  any operator()( Args&&... args );

  template<GettableOrVoid R = void, Pushable... Args>
  R call( Args&&... args );

  template<GettableOrVoid R = void, Pushable... Args>
  error_type_for_return_type<R> pcall( Args&&... args );

  template<typename IndexT, typename Predecessor>
  static table create_or_get(
      indexer<IndexT, Predecessor>&& idxr ) {
    cthread L = idxr.this_cthread();
    lua::push( L, idxr );
    table res = pop_or_create_table( L );
    idxr      = res;
    return res;
  }

private:
  static table pop_or_create_table( cthread L );
};

/****************************************************************
** Template implementations.
*****************************************************************/
template<Pushable... Args>
any table::operator()( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<any>( L, FWD( args )... );
}

template<GettableOrVoid R, Pushable... Args>
R table::call( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<R>( L, FWD( args )... );
}

template<GettableOrVoid R, Pushable... Args>
error_type_for_return_type<R> table::pcall( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_safe_and_get<R>( L, FWD( args )... );
}

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::table );
