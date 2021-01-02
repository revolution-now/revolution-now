/****************************************************************
**waitable.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-28.
*
* Description: Generic awaitable for use with coroutines.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <string>

namespace rn {

// FIXME: Using this in practice needs to wait for clang to be
// ready with coroutines and to support using them with
// libstdc++.

/****************************************************************
** Testing
*****************************************************************/
std::string test_waitable();

} // namespace rn
