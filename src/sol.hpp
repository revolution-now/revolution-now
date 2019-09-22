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

#ifdef L
#  undef L
#  include "sol/sol.hpp"
#  define L( a ) []( auto const& _ ) { return a; }
#else
#  include "sol/sol.hpp"
#endif
