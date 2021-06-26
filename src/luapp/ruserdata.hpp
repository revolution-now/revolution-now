/****************************************************************
**ruserdata.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              userdata.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "ext.hpp"
#include "indexer.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** userdata
*****************************************************************/
struct userdata : public any {
  using Base = any;

  using Base::Base;

  friend base::maybe<userdata> lua_get( cthread L, int idx,
                                        tag<userdata> );

  template<typename U>
  auto operator[]( U&& idx ) const noexcept {
    return indexer<U, userdata>( std::forward<U>( idx ), *this );
  }

  std::string name() const;
};

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::userdata );
