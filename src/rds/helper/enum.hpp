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
#include <concepts>
#include <string_view>

namespace rn {

template<typename Enum>
struct enum_traits;

template<typename Enum>
concept ReflectedEnum =
    // These checks are not exhaustive but should be sufficient.
    std::is_enum_v<Enum> &&
    std::is_same_v<typename enum_traits<Enum>::type, Enum> &&
    std::is_same_v<std::remove_cvref_t<
                       decltype( enum_traits<Enum>::type_name )>,
                   std::string_view>;

template<ReflectedEnum Enum>
constexpr std::string_view enum_name( Enum val ) {
  return enum_traits<Enum>::value_name( val );
}

} // namespace rn