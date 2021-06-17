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
#include "indexer.hpp"
#include "ref.hpp"

// base
#include "base/fmt.hpp"

namespace lua {

/****************************************************************
** table
*****************************************************************/
struct table : public reference {
  using Base = reference;

  using Base::Base;

  friend base::maybe<table> lua_get( cthread L, int idx,
                                     tag<table> );

  template<typename U>
  auto operator[]( U&& idx ) noexcept {
    return indexer<U, table>( std::forward<U>( idx ), *this );
  }
};

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::table );
