/****************************************************************
**variant.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-27.
*
* Description: Utilities for handling variants.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base-util
#include "base-util/variant.hpp"

// Scelta
#include <scelta.hpp>

// C++ standard library
#include <variant>

namespace rn {

template<typename Variant, typename... Funcs>
auto Switch( Variant const& v, Funcs... funcs ) {
  auto matcher = scelta::match( funcs... );
  return matcher( v );
}

} // namespace rn
