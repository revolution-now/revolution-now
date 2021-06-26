/****************************************************************
**rstring.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              strings.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** rstring
*****************************************************************/
struct rstring : public any {
  using Base = any;

  using Base::Base;

  std::string as_cpp() const;

  bool operator==( char const* s ) const;
  bool operator==( std::string_view s ) const;
  bool operator==( std::string const& s ) const;

  friend base::maybe<rstring> lua_get( cthread L, int idx,
                                       tag<rstring> );
};

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::rstring );
