/****************************************************************
**function.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              functions.
*
*****************************************************************/
#pragma once

// luapp
#include "ref.hpp"

// base
#include "base/fmt.hpp"

namespace lua {

/****************************************************************
** rfunction
*****************************************************************/
struct rfunction : public reference {
  using Base = reference;

  using Base::Base;
};

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::rfunction );
