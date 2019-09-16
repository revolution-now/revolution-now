/****************************************************************
**sol.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-15.
*
* Description: Includes sol2.  NOTE: Do not include this header
*              directly.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include <lua.h>

// Won't be needed in future versions.
#define SOL_CXX17_FEATURES 1

// Maybe can turn this off at some point?
#define SOL_ALL_SAFETIES_ON 1

#ifdef L
#  undef L
#  include "sol/sol.hpp"
#  define L( a ) []( auto const& _ ) { return a; }
#else
#  include "sol/sol.hpp"
#endif
