/****************************************************************
**adl-tag.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: Tag to be used to trigger ADL find things in base/
*              and also to avoid implicit conversions.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <type_traits>

namespace base {

template<typename T>
struct tag {
  static_assert( !std::is_reference_v<T> );
  static_assert( !std::is_const_v<T> || std::is_array_v<T> );
};

} // namespace base