/****************************************************************
**ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-23.
*
* Description: Concepts relating to convertibility to/from the
*              CDR (Canonical Data Representation).
*
*****************************************************************/
#pragma once

// CDR
#include "repr.hpp"

// base
#include "base/expect.hpp"

// C++ standard library
#include <concepts>

namespace cdr {

// ADL helper tag.
struct tag_t {};
inline constexpr tag_t tag{};

// For types that support converting themselves to the canonical
// data representation. If they support it, it is expected that
// such a conversion will always succeed.
template<typename T>
concept ToCanonical = requires( T const& o ) {
  // We need the `tag` here to get ADL to search the `cdr` name-
  // space; for most user-defined types this will not be neces-
  // sary, but it will be necessary for builtin types or std
  // types where we don't want to open the std namespace.
  { to_canonical( o, tag ) } -> std::same_as<T>;
};

// This one should be used to convert a value (i.e., don't call
// the one with the tag explicitly).
template<ToCanonical T>
value to_canonical( T const& o ) {
  // The function called below should be found via ADL.
  return to_canonical( o, tag );
}

// For types that support converting themselves from the canon-
// ical data representation. Unlike `to_canonical`, this one may
// fail.
template<typename T>
concept FromCanonical = requires( value const& o ) {
  // We don't need the `tag` here because the presence of both
  // the `o` and the `T` in the expression will be enough to get
  // ADL to search all of the namespaces that we need it to
  // (namely, T's namespace and the `cdr` namespace).
  //
  // Note that this canonical_to function is expected to perform
  // any validation that is needed after the conversion.
  { from_canonical<T>( o ) } -> std::same_as<base::expect<T>>;
};

// This one should be used to convert a value (i.e., don't call
// the one with the tag explicitly).
template<FromCanonical T>
base::expect<T> from_canonical( value const& v ) {
  // The function called below should be found via ADL.
  return from_canonical<T>( v, tag );
}

} // namespace cdr
