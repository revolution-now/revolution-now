/****************************************************************
**enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-02-28.
*
* Description: Helper for reflected enums.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <string_view>

namespace rn {

template<typename Enum>
struct enum_traits;

template<typename Enum>
constexpr std::string_view enum_name( Enum val ) {
  return enum_traits<Enum>::value_name( val );
}

} // namespace rn