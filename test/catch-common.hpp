/****************************************************************
**catch-common.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-23.
*
* Description: To be included as the last header in each transla-
* tion unit.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base
#include "base/to-str.hpp"

// Catch2
#include "catch2/catch.hpp"

// C++ standard library
#include <ranges>
#include <string>

namespace Catch {

// Allow Catch2 to format anything that supports to_str. But not
// things that are ranges because Catch2 already has specializa-
// tions for those which would clash.
// clang-format off
template<base::Show T>
requires( !std::ranges::range<T> ) //
struct StringMaker<T> {
  // clang-format on
  static std::string convert( T const& value ) {
    return base::to_str( value );
  }
};

} // namespace Catch
