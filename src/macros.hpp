/****************************************************************
**macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-28.
*
* Description: General macros used throughout the code.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#define ASSERT_NOTHROW_MOVING( type )                          \
  static_assert( std::is_nothrow_move_constructible_v<type> ); \
  static_assert( std::is_nothrow_move_assignable_v<type> )
