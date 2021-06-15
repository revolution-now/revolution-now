/****************************************************************
**ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Declarations needed by types that want to use the
*              luapp extension points.
*
*****************************************************************/
#pragma once

// luapp
#include "cthread.hpp"

// base
#include "base/maybe.hpp"

namespace lua {

// NOTE: The stuff in this header file should be limited to the
// minimum necessary that another (non-luapp) header needs to in-
// clude in order to declare extension points for a custom type.
// It should not be necessary to include any other headers from
// the luapp library in the header file that declares the type in
// question, although the implemention of those extension point
// methods in its cpp file might rely on other facilities from
// luapp that it can pull in.

template<typename T>
struct tag {};

// All get( ... ) calls should go here, then this should dispatch
// using ADL. It will look in the ::lua namespace as well as the
// namespace associated wtih T, if any.
template<typename T>
base::maybe<T> get( cthread L, int idx ) noexcept {
  static constexpr tag<T> o{};
  return get( L, idx, o );
}

} // namespace lua
