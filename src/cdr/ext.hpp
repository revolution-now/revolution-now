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

// cdr
#include "error.hpp"
#include "repr.hpp"

// C++ standard library
#include <concepts>

namespace cdr {

/****************************************************************
** ADL / Conversion helper tag
*****************************************************************/
// The fact that it is templated helps to prevent unwanted over-
// load ambiguities.
template<typename T>
struct tag_t {};

template<typename T>
inline constexpr tag_t<T> tag{};

/****************************************************************
** C++ ==> Canonical
*****************************************************************/
// For types that support converting themselves to the canonical
// data representation. If they support it, it is expected that
// such a conversion will always succeed.
template<typename T>
concept ToCanonical = requires( T const& o ) {
  // We need the `tag` here to get ADL to search the `cdr` name-
  // space; for most user-defined types this will not be neces-
  // sary since their to_canonical functions will be in their own
  // namespace (which will be searched), but it will be necessary
  // for builtin types or std types where we don't want to open
  // the std namespace.
  // clang-format off
  { to_canonical( o, tag<std::remove_const_t<T>> ) }
      -> std::same_as<value>;
  // clang-format on
};

// This one should be used to convert a value (i.e., don't call
// the one with the tag explicitly).
template<ToCanonical T>
value to_canonical( T const& o ) {
  // The function called below should be found via ADL.
  return to_canonical( o, tag<std::remove_const_t<T>> );
}

/****************************************************************
** Canonical ==> C++
*****************************************************************/
// For types that support converting themselves from the canon-
// ical data representation. Unlike `to_canonical`, this one may
// fail.
template<typename T>
concept FromCanonical = requires( value const& o ) {
  // Note that this from_canonical function is expected to per-
  // form any validation that is needed after the conversion.
  // clang-format off
  { from_canonical( o, tag<std::remove_const_t<T>> ) }
      -> std::same_as<result<std::remove_const_t<T>>>;
  // clang-format on
};

// Note that there is no top-level from_canonical function, un-
// like to_canonical. That is because you must use the cdr::con-
// verter instead because it ensures that errors are tracked au-
// tomatically.

/****************************************************************
** C++ <==> Canonical
*****************************************************************/
template<typename T>
concept Canonical = ToCanonical<T> && FromCanonical<T>;

} // namespace cdr
