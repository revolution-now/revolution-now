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
//
// This will give a compiler error if at least one alternative
// type does not inherit from the Base or does not do so
// publicly.
//
// This should only be called on variants that have a unique type
// list, although that is not enforced here (results will just
// not be meaninful).
//
// The result should always be non-null assuming that the variant
// is not in a valueless-by-exception state.
template<typename Base, typename... Args>
Base* variant_base_ptr( std::variant<Args...>& v ) {
  Base* res = nullptr;
  ( ( res = std::holds_alternative<Args>( v )
                ? static_cast<Base*>( std::get_if<Args>( &v ) )
                : res ),
    ... );
  return res;
}

} // namespace rn
