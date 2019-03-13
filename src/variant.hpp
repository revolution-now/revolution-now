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

// Use this when all alternatives inherit from a common base
// class and you need a base-class pointer to the active member.
template<typename Base, typename... Args>
Base* variant_base_ptr( std::variant<Args...>& v ) {
  Base* res = nullptr;
  ( ( res = std::holds_alternative<Args>( v )
                ? (Base*)( std::get_if<Args>( &v ) )
                : res ),
    ... );
  return res;
}

} // namespace rn
