/****************************************************************
**rand-enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-21.
*
* Description: Helpers for picking random enum values.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "irand.hpp"

// refl
#include "refl/query-enum.hpp"

namespace rn {

// Given a reflected enum type it will return a random value out
// of all the possible values.
template<refl::ReflectedEnum Enum>
[[nodiscard]] Enum pick_one( IRand& rand ) {
  constexpr auto count = refl::enum_count<Enum>;
  static_assert( count > 0 );
  auto idx = rand.uniform_int( 0, count - 1 );
  return refl::enum_values<Enum>[idx];
}

} // namespace rn
