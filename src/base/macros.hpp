/****************************************************************
**macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: Macros.
*
*****************************************************************/
#pragma once

// TODO: remove this guard once we no longer depend on
// base-util/macros.hpp.
#ifndef STRING_JOIN
#  define STRING_JO1N( arg1, arg2 ) arg1##arg2
#  define STRING_JOIN( arg1, arg2 ) STRING_JO1N( arg1, arg2 )
#endif