/****************************************************************
**thread.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              threads.
*
*****************************************************************/
#pragma once

// luapp
#include "ref.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** rthread
*****************************************************************/
struct rthread : public reference {
  using Base = reference;

  ::lua::cthread cthread() const noexcept { return L; }

  using Base::Base;
};

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::rthread );
