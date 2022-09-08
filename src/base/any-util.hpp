/****************************************************************
**any-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: Utilities for dealing with std::any.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "cc-specific.hpp"
#include "meta.hpp"
#include "variant.hpp"

// C++ standard library
#include <any>

namespace base {

// This function is able to take a std::any that holds either a
// variant type or the type of any of its contituents, and will
// return (by value) the variant. This needs special care because
// the default std::any_cast doesn't appear to realize that the
// alternative types can be converted to the variant type, and so
// will fail if the std::any holds an alternative type.
template<typename V>
V extract_variant_from_any( std::any const& a ) {
  maybe<V> res = {};
  FOR_CONSTEXPR_IDX( Idx, std::variant_size_v<V> ) {
    using Alt_t    = std::variant_alternative_t<Idx, V>;
    Alt_t const* p = std::any_cast<Alt_t const>( &a );
    if( p != nullptr ) {
      res = V{ *p };
      return true; // stop iterating.
    }
    return false; // continue iterating.
  };
  if( res.has_value() ) return *res;
  // At this point, the std::any does not contain any of the al-
  // ternatives, so check for the variant itself.
  V const* p = std::any_cast<V const>( &a );
  if( p != nullptr ) {
    res = V{ *p };
    return *res;
  }
  FATAL(
      "std::any expected to contain variant type {} or one of "
      "its alternatives; instead it contains a {}.",
      demangled_typename<V>(), demangle( a.type().name() ) );
}

} // namespace base
