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

// This is the object that should be used to call from_canonical
// and to_canonical, i.e., it is used to orchestrate a conversion
// process. It helps track errors and contains configuration op-
// tions for the conversion process.
struct converter;

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
concept ToCanonical = requires( T const& o, converter& conv ) {
  // clang-format off
  { to_canonical( conv, o, tag<std::remove_const_t<T>> ) }
      -> std::same_as<value>;
  // clang-format on
};

/****************************************************************
** Canonical ==> C++
*****************************************************************/
// For types that support converting themselves from the canon-
// ical data representation. Unlike `to_canonical`, this one may
// fail.
template<typename T>
concept FromCanonical = requires( value const& o,
                                  converter&   conv ) {
  // Note that this from_canonical function is expected to per-
  // form any validation that is needed after the conversion.
  // clang-format off
  { from_canonical( conv, o, tag<std::remove_const_t<T>> ) }
      -> std::same_as<result<std::remove_const_t<T>>>;
  // clang-format on
};

/****************************************************************
** C++ <==> Canonical
*****************************************************************/
// This one probably should not be used often; most of the time
// you need to require either ToCanonical or FromCanonical but
// not both. Only use this one if you really need to require
// both.
template<typename T>
concept Canonical = ToCanonical<T> && FromCanonical<T>;

} // namespace cdr
